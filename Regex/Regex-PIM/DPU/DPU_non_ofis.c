#include <mram.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <alloc.h>
#include "defs.h"
#include "mutex.h"
#include "barrier.h"

#define DFA_SET_MAX_SIZE (58 * 1024 * 1024/8)
#define BYTE_ALIGN(x) ((x % 8) ? x + 8 - (x % 8) : x)

#define MAX_CHUNK_NUM 256

#define DEBUG 0
#define CHUNK_REPLACE 0

MUTEX_INIT(global_val_mutex);
MUTEX_INIT(result_mutex);

BARRIER_INIT(my_barrier, NR_TASKLETS);
BARRIER_INIT(end_barrier, NR_TASKLETS);
BARRIER_INIT(chunk_replace_start_barrier, NR_TASKLETS);
BARRIER_INIT(chunk_replace_end_barrier, NR_TASKLETS);

// MRAM variables
__mram_noinit int32_t MRAM_DFA_set[DFA_SET_MAX_SIZE]; // 호스트에서 받은 DFA에 대한 정보
__mram_noinit int8_t MRAM_target_text[1 << 19]; // 호스트에서 받은 text에 대한 정보
__mram_noinit int32_t MRAM_output_buffer[(1 << 20) + 1024]; // 호스트로 전송할 매칭 결과
__mram_noinit int32_t MRAM_output_metadata[2]; // 호스트로 전송할 매칭 결과에 대한 메타데이터. 0번칸에 (매칭 횟수 * 2)가 저장됨.

// WRAM variables
__dma_aligned int32_t states_num;
__dma_aligned int32_t character_set_size;
__dma_aligned int32_t index_mapping_table[128];
__dma_aligned int32_t* acceptable_state_bitmap;
__dma_aligned int32_t transition_table[8 * 1024];
__dma_aligned int8_t partitioned_data[2][2048]; // target data를 분할하여 저장하는 buffer
__dma_aligned int32_t partitioned_chunk_num[2]; // 각각의 버퍼에 저장된 청크가 몇번째 청크인지 저장
__dma_aligned int32_t result[512];

__dma_aligned int32_t last_DFA_index = 0;
__dma_aligned int32_t current_DFA_index = 0;
__dma_aligned int32_t result_index;


__dma_aligned int32_t global_matching_index; // 인덱스 접근용 공유 변수
__dma_aligned int32_t barrier_access_counter[11];
__dma_aligned int32_t end_flag = 0;
__dma_aligned int32_t current_buffer = 0;



void print_DFA_fixed_size_data();
void print_acceptable_state_bitmap(int bitmap_size);
void print_transition_table(int table_size);

void matching_shared_val(int tasklet_id);

void chunk_replace(int chunk_num) // 청크 교환
{
    if(MAX_CHUNK_NUM <= chunk_num)
    {
        for(int i = 0; i < 2048; i++)
        {
            partitioned_data[chunk_num % 2][i] = -1;
        }
    }

    mram_read( (const __mram_ptr void*) (MRAM_target_text + 2048 * chunk_num), (void *) (partitioned_data[(chunk_num) % 2]), 2048); // 청크 교체
    partitioned_chunk_num[chunk_num % 2] = chunk_num; // 청크 번호 업데이트
}


int get_DFA_info() //  DFA_set에서 데이터 추출
{
    // fixed_data_copy
    states_num = MRAM_DFA_set[current_DFA_index];
    character_set_size = MRAM_DFA_set[current_DFA_index + 1];
    current_DFA_index += 2;

    if(states_num == -99 && character_set_size == -99) return 1; // DFA_set의 끝

    mram_read( (const __mram_ptr void*) (MRAM_DFA_set + current_DFA_index), (void *) index_mapping_table, 4 * 128);
    current_DFA_index += 128;
    
    #if DEBUG
        print_DFA_fixed_size_data(); // fixed size data의 추출이 완료되었는지 확인
    #endif

    // acceptable_state_bitmap_build
    // 32개의 state의 acceptability를 int32_t 형 변수 하나로 표현
    int32_t bitmap_size;
    bitmap_size = ((states_num + 31) >> 5); // ceil(states_num / 32)과 똑같은 역할
    if(bitmap_size & 1) bitmap_size++; // bitmap의 크기는 byte alignment때문에 항상 짝수여야 함.
    acceptable_state_bitmap = (int32_t *) mem_alloc(4 * bitmap_size); // bitmap 동적할당
    mram_read( (const __mram_ptr void*) (MRAM_DFA_set + current_DFA_index), (void *) acceptable_state_bitmap, 4 * bitmap_size); // bitmap 복사
    current_DFA_index += bitmap_size;
    
    #if DEBUG
        print_acceptable_state_bitmap(bitmap_size); // acceptable_state_bitmap의 추출이 완료되었는지 확인
    #endif


    // transition_table_build
    int32_t table_size = states_num * character_set_size;
    if(table_size & 1) table_size++;

    #if DEBUG
        printf("table_size: %d\n", table_size);
    #endif

    for(int i = 0; i < table_size; i += 512)
    {
        if(table_size - i > 512)
        {
            mram_read( (const __mram_ptr void*) (MRAM_DFA_set + current_DFA_index), (void *) (transition_table + i), 2048);
            current_DFA_index += 512;
        }
        else
        {
            mram_read( (const __mram_ptr void*) (MRAM_DFA_set + current_DFA_index), (void *) (transition_table + i), 4 * (table_size - i));
            current_DFA_index += table_size - i;
        }
    }

    #if DEBUG
        print_transition_table(table_size); // transition table의 추출이 완료되었는지 확인
    #endif

    return 0;
}

int is_acceptable_print(int state_num)
{
    int bitmap_arr_index = (state_num - 1) / 32;
    int bit_index = (state_num - 1) % 32;

    printf("bitmap_arr_index: %d\n", bitmap_arr_index);
    printf("bit_index: %d\n", bit_index);

    for(int i = 0; i < 14; i++)
    {
        printf("%d\n", acceptable_state_bitmap[bitmap_arr_index] / (1 << i));
    }

    return (acceptable_state_bitmap[bitmap_arr_index] & (1 << bit_index));
}


int is_acceptable(int state_num)
{
    int bitmap_arr_index = (state_num - 1) / 32;
    int bit_index = (state_num - 1) % 32;

    return (acceptable_state_bitmap[bitmap_arr_index] & (1 << bit_index));
}

int find_index(char input)
{
    if(index_mapping_table[input] != -1) return index_mapping_table[input];
    else return index_mapping_table[127];
}

int matching(/*int start_index, int start_state*/int chunk_num, int start_index, int start_state, int tasklet_id)
{
    // if(partitioned_data[current_buffer][start_index] == -1) return -1;
    if(partitioned_data[chunk_num % 2][start_index] == -1) return -1; // data의 끝을 넘어가는 경우 더 진행하지 않고 return 해야함.


    int current_state = start_state;

    //현재 chunk_num과 내 chunk_num 비교
    if(partitioned_chunk_num[chunk_num % 2] < chunk_num)
    {
        #if CHUNK_REPLACE
            printf("Chunk_replace_1 start barrier, tasklet id: %d, access_count: %d, chunk_num: %d, start_index: %d\n", tasklet_id, barrier_access_counter[tasklet_id]++, chunk_num, start_index);
        #endif

        barrier_wait(&chunk_replace_start_barrier);
        if(tasklet_id == 0)
        {
            chunk_replace(chunk_num);
            #if CHUNK_REPLACE
                printf("chunk replace 1: %d\n", chunk_num);
            #endif
        }

        #if CHUNK_REPLACE
            barrier_wait(&chunk_replace_end_barrier);
            printf("Chunk_replace_1 end barrier, tasklet id: %d, access_count: %d\n", tasklet_id, barrier_access_counter[tasklet_id]++);
        #endif
        barrier_wait(&chunk_replace_end_barrier);
    }


    for(int i = start_index; i < 2048; i++)
    {
        // if(chunk_num == 0 && start_index == 0)
        // {
        //     printf("state: %d\n", current_state);
        // }
        if(current_state == 0) // current_state가 trap state일 경우
        {
            return 0;
        }

        if(is_acceptable(current_state)) // acceptable state인 경우 매칭한 길이를 return
        {
            return i - start_index;
        }

        else // 다음 state로 transition
        {
            // if(current_state != 1)
            // {
            //     printf("%d ", current_state);
            // }
            int found_index = find_index(partitioned_data[(chunk_num) % 2][i]);

            // if(chunk_num == 0 && start_index == 0)
            // {
            //     printf("found: %d\n", found_index);
            // }

            if(found_index == -1) current_state = 0;
            else    current_state = transition_table[(current_state - 1) * character_set_size + found_index];
        }
    }

    if(current_state == 0) //마지막 chunk의 마지막 인덱스에서 시작한 경우 에러를 일으킬 수 있음. 
    {
        #if DEBUG
            // printf("Early return, tasklet id: %d, chunk_num: %d, start_index: %d\n", tasklet_id, chunk_num, start_index);
        #endif
        return 0;
    }

    //다음 chunk_num과 현재 chunk_num 비교
    if(partitioned_chunk_num[(chunk_num + 1) % 2] < partitioned_chunk_num[(chunk_num) % 2])
    {
        if(chunk_num == MAX_CHUNK_NUM - 1) return 0;
        #if CHUNK_REPLACE
            printf("Chunk_replace_2 start barrier, tasklet id: %d, access_count: %d, chunk_num: %d, start_index: %d\n", tasklet_id, barrier_access_counter[tasklet_id]++, chunk_num, start_index);
        #endif
        barrier_wait(&chunk_replace_start_barrier);

        if(tasklet_id == 0)
        {
            chunk_replace(chunk_num + 1);
            #if CHUNK_REPLACE
                printf("chunk replace 2: %d\n", chunk_num + 1);
            #endif
        }
        #if CHUNK_REPLACE
            barrier_wait(&chunk_replace_end_barrier);
            printf("Chunk_replace_2 end barrier, tasklet id: %d, access_count: %d\n", tasklet_id, barrier_access_counter[tasklet_id]++);
        #endif
        barrier_wait(&chunk_replace_end_barrier);
    }

    for(int i = 0; i <= start_index; i++)
    {
        
        if(partitioned_data[(chunk_num + 1) % 2][i] == -1)
        {
            // printf("\n");
            return 0; // data의 끝을 넘어가는 경우 더 진행하지 않고 return 해야함.
        }
        if(current_state == 0) // trap state
        {
            // printf("\n");
            return 0;
        }

        if(is_acceptable(current_state)) // acceptable state인 경우 매칭한 길이를 return
        {
            // printf("\n");
            return (2048 - start_index) + i;
        }

        else // 다음 state로 transition
        {   
            // if(current_state != 1)
            // {
            //     printf("%d ", current_state);
            // }
            int found_index = find_index(partitioned_data[(chunk_num + 1) % 2][i]);

            if(found_index == -1) current_state = 0;
            else    current_state = transition_table[(current_state - 1) * character_set_size + found_index];
        }
    }

    return 0;
    
}


int main(){
	int32_t tasklet_id = me();
    barrier_access_counter[tasklet_id] = 0;
    int32_t rule_num = 0;

    if(tasklet_id == 0) // 초기 데이터 세팅
    {
        mem_reset();

        #if DEBUG
            printf("table_size: %d\nchunk_num: %d\n", MRAM_DFA_set[0], MRAM_DFA_set[1]);
        #endif

        last_DFA_index = 0;
        current_DFA_index = 0;
        result_index = 0;
        current_buffer = 0;
        global_matching_index = 0;
        end_flag = 0;
        MRAM_output_metadata[0] = 0;
        MRAM_output_metadata[1] = 0;


        #if DEBUG
            printf("chunk num: %d %d\n", partitioned_chunk_num[0], partitioned_chunk_num[1]);
        #endif
    }
    
    // printf("First setting barrier, tasklet id: %d, access_count: %d\n", tasklet_id, barrier_access_counter[tasklet_id]++);
    barrier_wait(&my_barrier);


    // // test
    // int matching_result = matching(0, tasklet_id, 1, tasklet_id); // matching 실행
    // printf("tasklet: %d, matching_result: %d\n", tasklet_id, matching_result);
    // return 0;
    // // test end


    // 공유 변수를 이용한 매칭

    while(1)
    {
        if(tasklet_id == 0) // 새로운 rule을 이용한 매칭
        {
            // rule 번호 result 버퍼에 저장
            result[result_index % 512] = rule_num;
            result[result_index % 512 + 1] = -1;

            rule_num++;
            result_index += 2;

            // 매칭 준비
            global_matching_index = 0; // global index 초기화
            end_flag = get_DFA_info(); // DFA 정보 추출

            chunk_replace(0);
            chunk_replace(1);

        }
        barrier_wait(&my_barrier);
        if(end_flag)
        {
            MRAM_output_buffer[result_index - 2] = -99;
            break; // 마지막 DFA였다면 break
        }
        matching_shared_val(tasklet_id); // 매칭 수행

        // 매칭 끝을 의미하는 베리어
        #if DFBUG
            printf("Matching end barrier, tasklet id: %d\n", tasklet_id);
        #endif
        barrier_wait(&end_barrier);

        // 남은 결과 저장
        if(tasklet_id == 0)
        {
            int rest_result = result_index % 512;
            if(rest_result) mram_write((const void *)result, (__mram_ptr void *)(MRAM_output_buffer + result_index - rest_result), 4 * rest_result);
        }
        
        #if DFBUG
            printf("Loop end barrier, tasklet id: %d\n", tasklet_id);
        #endif
        barrier_wait(&end_barrier);
    }

    if(tasklet_id == 0) // 메타 데이터 저장
    {
        MRAM_output_metadata[0] = result_index;
        MRAM_output_metadata[1] = -1;
    }

    #if DEBUG
        printf("Release last barrier 1, tasklet id: %d, access_count: %d\n", tasklet_id, barrier_access_counter[tasklet_id]++);
        barrier_wait(&chunk_replace_start_barrier);
        printf("Release last barrier 2, tasklet id: %d, access_count: %d\n", tasklet_id, barrier_access_counter[tasklet_id]++);
        barrier_wait(&chunk_replace_end_barrier);
    #endif

    #if DFBUG
        printf("Program end barrier, tasklet id: %d\n", tasklet_id);
    #endif
    barrier_wait(&end_barrier);
}


void print_DFA_fixed_size_data()
{
    printf("states_num: %d\n", states_num);
    printf("character_set_size: %d\n", character_set_size);

    printf("index_mapping_table\n");
    for(int i = 0; i < 8; i++)
    {
        for(int j = 0; j < 16; j++)
        {
            printf("%d ", index_mapping_table[16 * i + j]);
        }
        printf("\n");
    }
}

void print_acceptable_state_bitmap(int bitmap_size)
{
    printf("acceptable_state_bitmap\n");
    for(int i = 0; i < bitmap_size; i++)
    {
        int temp = acceptable_state_bitmap[i];
        printf("%d ", temp);
    }
    printf("\n");
}

void print_transition_table(int table_size)
{
    printf("transition_table\n");

    for(int i = 0; i < states_num; i++)
    {
        for(int j = 0; j < character_set_size; j++)
        {
            printf("%d ", transition_table[character_set_size * i + j]);
        }
        printf("\n");
    }
}

void matching_shared_val(int tasklet_id)
{
    // 공유 변수를 이용한 매칭
    int my_matching_index = 0;
    while(global_matching_index < MAX_CHUNK_NUM * 2048)
    {
        mutex_lock(global_val_mutex);
            my_matching_index = global_matching_index; // 공유 인덱스 변수 복사
            global_matching_index++;                   // 공유 인덱스 변수 증가
        mutex_unlock(global_val_mutex);

        if(my_matching_index >= MAX_CHUNK_NUM * 2048) break; // 최대 인덱스 접근시 종료

        int my_chunk_num = my_matching_index / 2048;
        int my_matching_start_index = my_matching_index % 2048;

        int matching_result = matching(my_chunk_num, my_matching_start_index, 1, tasklet_id); // matching 실행

        if(matching_result == -1) // text의 크기를 넘어간 경우
        {
            return;
        }
        if(matching_result > 0) // matching에 성공한 경우
        {
            mutex_lock(result_mutex);
                result[result_index % 512] = my_matching_index;                             // 매칭 시작 위치 저장
                result[result_index % 512 + 1] = my_matching_index + matching_result - 1;   // 매칭 끝 위치 저장
                result_index += 2;                                                          // result index 증가
                
                
                if(result_index % 512 == 0)
                {
                    mram_write((const void *)result, (__mram_ptr void *)(MRAM_output_buffer + result_index - 512), 2048); // result 버퍼가 가득 찬 경우 결과 저장 
                }
            mutex_unlock(result_mutex);
        }
    }
}