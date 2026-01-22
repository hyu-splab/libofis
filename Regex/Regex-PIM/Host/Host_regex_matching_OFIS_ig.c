#include <assert.h>
#include <dpu.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <dpu_log.h>
#include "../DFA_library/make_DFA.h"

#include <dpu_management.h>
#include <ofis.h>
#include <sys/time.h>
#include <pthread.h>
#include <sched.h>

#define OPEN_TO_DPU 0
#define OPEN_TO_HOST 1

#define OFIS_DPU_BINARY "./bin//DPU_ofis"
#define NON_OFIS_DPU_BINARY "./bin/DPU_non_ofis"

int* result[2048];
int result_matched_num[2048][2];

#define MAX_DFA_NUM 64
#define MAX_TEXT_NUM 2048
#define DFA_INFO_BUFFER_SIZE (256 * 1024)
#define TARGET_TEXT_SIZE (512 * 1024)

int* DFA_info[MAX_DFA_NUM];
int DFA_info_size[MAX_DFA_NUM];

char target_data[2048][1 << 19];

struct timeval start, end;

double alloc_time;
double load_time;
double copy_in_time;
double run_time;
double copy_out_time;



typedef struct Timer{
    struct timeval start[32];
    struct timeval end[32];
    double time[32];
} Timer;

Timer OFIS_execution_timer;

void init_timer(Timer* timer){
    for(int i = 0; i < 32; i++)
        timer->time[i] = 0.0;
}

void start_timer(Timer* timer, int i){
    gettimeofday(&timer->start[i], NULL);
}

void end_timer(Timer* timer, int i){
    gettimeofday(&timer->end[i], NULL);
    timer->time[i] += (timer->end[i].tv_sec - timer->start[i].tv_sec) + ((double)(timer->end[i].tv_usec - timer->start[i].tv_usec) / 1000000);
}


long long int matching_amount_per_DFA[MAX_DFA_NUM];

double get_time_difference(struct timeval start, struct timeval end)
{
    double time_difference = ( end.tv_sec - start.tv_sec ) + (double)(( end.tv_usec - start.tv_usec )) / 1000000;

    return time_difference;
}


void load_target_data(int target_data_size)
{
    FILE* fp = fopen("../dataset/Text/text.txt", "rb"); 
    
    if(fp == NULL)
    {
        printf("merged file open error\n");
        exit(0);
    }

    int buffer_size = (1 << 19);
    int read_size = buffer_size; 

    for(int i = 0; i < target_data_size * 2; i++)
    {
        size_t result = fread(target_data[i], 1, read_size, fp);

        if(result == 0 && feof(fp)) 
        {
            break; 
        }
    }

    fclose(fp);
}



void load_DFA_info(int regex_rule_amount)
{
    for(int i = 0; i < regex_rule_amount; i++)
    {
        DFA_info[i] = (int*)malloc(sizeof(int) * (DFA_INFO_BUFFER_SIZE));    
if (DFA_info[i] == NULL) {
            printf("[Fatal Error] Malloc failed for DFA %d\n", i);
            exit(1);
        }


        int local_DFA_index = 0;
        char DFA_info_file_name[100] = {};
        sprintf(DFA_info_file_name, "../dataset/DFA_info/DFA_%d.csv", i);

        FILE* check_fp = fopen(DFA_info_file_name, "r");
        if (check_fp == NULL) {
            printf("[Fatal Error] Cannot open file: %s (DFA index: %d)\n", DFA_info_file_name, i);
            exit(1);
        }
        fclose(check_fp);

        DFA_info_size[i] = make_BUF(DFA_info[i], DFA_info_file_name);
        
        if (DFA_info_size[i] <= 0) {
            printf("[Fatal Error] DFA %d size is invalid: %d\n", i, DFA_info_size[i]);
            exit(1);
        }

        if (DFA_info_size[i] * sizeof(int) >= DFA_INFO_BUFFER_SIZE)
        {
            printf("Error: DFA %d size exceeds buffer! (%d)\n", i, DFA_info_size[i]);
            exit(1);
        }
        local_DFA_index = DFA_info_size[i];    
        DFA_info[i][local_DFA_index] = -99;
        DFA_info[i][local_DFA_index + 1] = -99;
        DFA_info_size[i]+=2;
    }
}

int partition_index_per_DFA[MAX_DFA_NUM];
pthread_mutex_t partition_index_per_DFA_mutex[MAX_DFA_NUM];

typedef struct{
    uint32_t thread_id;
    uint32_t assigned_DFA_numbers[4];

    struct dpu_set_t rank;
} thread_args;


int get_current_index_per_DFA(int DFA_num)
{
    pthread_mutex_lock(&partition_index_per_DFA_mutex[DFA_num]);

        int index = partition_index_per_DFA[DFA_num];
        partition_index_per_DFA[DFA_num] += 16;

    pthread_mutex_unlock(&partition_index_per_DFA_mutex[DFA_num]);
    return index;
}


void save_local_result(int matched, int* pointer)
{
    *pointer += matched;
}

pthread_mutex_t global_result_save_mutex[MAX_DFA_NUM];

void save_global_result(int matched, int DFA_num, int* temp_buffer)
{
    pthread_mutex_lock(&global_result_save_mutex[DFA_num]);
        int current_index = matching_amount_per_DFA[DFA_num];
        matching_amount_per_DFA[DFA_num] += (long long int) matched;
    pthread_mutex_unlock(&global_result_save_mutex[DFA_num]);

    for(int i = current_index; i < current_index + matched; i++)
    {
        result[DFA_num][i] = temp_buffer[i - current_index];
    }
}


int get_next_DFA_num()
{
    int min_DFA_num = -1;
    int min_DFA_index = MAX_TEXT_NUM;
    for(int i = 0; i < MAX_DFA_NUM; i++)
    {
        int temp_index = partition_index_per_DFA[i];
        if(temp_index < MAX_TEXT_NUM)
        {
            if(min_DFA_index >= temp_index)
            {
                min_DFA_num = i;
                min_DFA_index = temp_index;
            }
        }
    }
    return min_DFA_num;
}


void* OFIS_DPU_fct(void* arg)
{
    // thread_args copy
    thread_args* args = (thread_args*)arg;

    // thread args member variables copy
    uint32_t thread_id = args->thread_id;
    uint32_t rank_id = thread_id;
    uint32_t assigned_DFA_numbers[4] = {};
    
    assigned_DFA_numbers[0] = args->assigned_DFA_numbers[0];
    assigned_DFA_numbers[1] = args->assigned_DFA_numbers[1];
    assigned_DFA_numbers[2] = args->assigned_DFA_numbers[2];
    assigned_DFA_numbers[3] = args->assigned_DFA_numbers[3];
    

    struct dpu_set_t rank = args->rank;

    // Set affinity to thread
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(thread_id, &cpu_set);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set);



    struct dpu_set_t dpu;
    int each_dpu;
    uint32_t rank_idx = 64 * rank_id;

    // Initial data transfer
    DPU_FOREACH(rank, dpu, each_dpu)
    {
        int __assigned_DFA_num = assigned_DFA_numbers[(each_dpu % 8) / 2];
        DPU_ASSERT(dpu_prepare_xfer(dpu, DFA_info[__assigned_DFA_num]));
    }

    int max_DFA_info_size = 0;
    for(int i = 0; i < 4; i++)
    {
        int __assigned_DFA_num = assigned_DFA_numbers[i];
        if(max_DFA_info_size < DFA_info_size[__assigned_DFA_num])
        {
            max_DFA_info_size = DFA_info_size[__assigned_DFA_num];
        }
    }
    if (max_DFA_info_size % 2 != 0) {
        max_DFA_info_size++; 
    }
    DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_TO_DPU, "MRAM_DFA_set", 0, max_DFA_info_size * 4, DPU_XFER_DEFAULT));
    
    // initial target data transfer
    int local_target_data_index[4] = {};
    local_target_data_index[0] = get_current_index_per_DFA(assigned_DFA_numbers[0]);
    local_target_data_index[1] = get_current_index_per_DFA(assigned_DFA_numbers[1]);
    local_target_data_index[2] = get_current_index_per_DFA(assigned_DFA_numbers[2]);
    local_target_data_index[3] = get_current_index_per_DFA(assigned_DFA_numbers[3]);

    DPU_FOREACH(rank, dpu, each_dpu)
    {
        int __assigned_target_data_num = 0;
        int my_ig_num = each_dpu % 8;

        __assigned_target_data_num = local_target_data_index[my_ig_num / 2] + 8 * (my_ig_num % 2) + (each_dpu / 8);

        DPU_ASSERT(dpu_prepare_xfer(dpu, target_data[__assigned_target_data_num]));
    }
    DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_TO_DPU, "MRAM_target_text", 0, TARGET_TEXT_SIZE, DPU_XFER_DEFAULT));

    // DPU binary launch
    OFIS_dpu_launch(rank);

    // DPU binary termination check
    uint8_t finish_ig;
    uint32_t finish_count = 0;

    // Terminated IG Unit tracking array
    int is_terminated[4] = {0, 0, 0, 0};

    while (finish_count < 8)
    {
        // Read all check_flags of rank
        uint32_t nr_finish_ig = 0;
        do{
            nr_finish_ig = OFIS_get_finished_ig(rank, &finish_ig);
        }while(nr_finish_ig == 0);
        


        uint32_t change_state = -1; // check_flag send to DPU
    
        // Set MUX for the CPU (To Read & Write data)
        for(int i = 0; i < 8; ++i)
        {
            if(finish_ig & (1 << i))
                OFIS_set_mux_ig(rank, i, OPEN_TO_HOST);
        }

        // Read Interim results from all DPUs in the finish_igs
        DPU_FOREACH(rank, dpu, each_dpu)
        {
            OFIS_prepare_xfer_ig(dpu, finish_ig, result_matched_num[64 * rank_id + each_dpu]);
        }
        DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_FROM_DPU, "MRAM_output_metadata", 0, 8, DPU_XFER_DEFAULT));        

        int matched_max = 0;
        for(int index_for_matched_max = 0; index_for_matched_max < 64; index_for_matched_max++)
        {
            int __my_ig_num = index_for_matched_max % 8;
            
            if(finish_ig & (1 << __my_ig_num))
            {
                if(result_matched_num[64 * rank_id + index_for_matched_max][0] > matched_max)
                {
                    matched_max = result_matched_num[64 * rank_id + index_for_matched_max][0];
                }
            }
        }

        // temp result dynamic allocation
        int* temp_result[2048] = {};
        for(int index_for_malloc = 0; index_for_malloc < 64; index_for_malloc++)
        {
            int __my_ig_num = index_for_malloc % 8;
            
            if(finish_ig & (1 << __my_ig_num))
            {
                temp_result[64 * rank_id + index_for_malloc] = (int*)malloc(sizeof(int) * matched_max);
            }   
        }

        DPU_FOREACH(rank, dpu, each_dpu)
        {
            OFIS_prepare_xfer_ig(dpu, finish_ig, temp_result[64 * rank_id + each_dpu]);
        }
        DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_FROM_DPU, "MRAM_output_buffer", 0, 4 * matched_max, DPU_XFER_DEFAULT));


        for(int index_for_res_collection = 0; index_for_res_collection < 64; index_for_res_collection++)
        {
            int __my_ig_num = index_for_res_collection % 8;

            if(finish_ig & (1 << __my_ig_num))
            {
                int result_size = result_matched_num[64 * rank_id + index_for_res_collection][0];
                int matched_DFA_num = assigned_DFA_numbers[__my_ig_num / 2];

                save_global_result(result_matched_num[64 * rank_id + index_for_res_collection][0], assigned_DFA_numbers[__my_ig_num / 2], temp_result[64 * rank_id + index_for_res_collection]);
            }
        }

        for(int index_for_free = 0; index_for_free < 64; index_for_free++)
        {
            int __my_ig_num = index_for_free % 8;
            if(finish_ig & (1 << __my_ig_num))
            {
                int idx = 64 * rank_id + index_for_free;
                if (temp_result[idx] != NULL) {
                    free(temp_result[idx]);
                    temp_result[idx] = NULL; 
                }
            }
        }

        // New data transfer logic
        int finished_ig[4] = {};
        for(int ig_unit_num = 0; ig_unit_num < 4; ig_unit_num++)
        {
            if((finish_ig & (1 << (2 * ig_unit_num))) && (finish_ig & (1 << (2 * ig_unit_num + 1))) && !is_terminated[ig_unit_num])
            {
                finished_ig[ig_unit_num] = 1;
            }
        }
        
        //  Determin next DFA num or text index
        int to_be_ended_ig[4] = {};

        for(int ig_unit_num = 0; ig_unit_num < 4; ig_unit_num++)
        {
            if(finished_ig[ig_unit_num])
            {
                int new_local_target_data_index = get_current_index_per_DFA(assigned_DFA_numbers[ig_unit_num]);

                if(new_local_target_data_index >= MAX_TEXT_NUM)
                {
                    while(1)
                    {
                        
                        int new_DFA_num = get_next_DFA_num();

                        if(new_DFA_num == -1)
                        {
                            to_be_ended_ig[ig_unit_num] = 1;
                            break;
                        }

                        new_local_target_data_index = get_current_index_per_DFA(new_DFA_num);
                        if(new_local_target_data_index < MAX_TEXT_NUM)
                        {
                            assigned_DFA_numbers[ig_unit_num] = new_DFA_num;
                            
                            DPU_FOREACH(rank, dpu, each_dpu)
                            {
                                int my_ig_num = each_dpu % 8;
                                
                                if(my_ig_num / 2 == ig_unit_num)
                                {
                                    OFIS_prepare_xfer_ig(dpu, finish_ig, DFA_info[new_DFA_num]);
                                }
                            }
                            DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_TO_DPU, "MRAM_DFA_set", 0, DFA_info_size[assigned_DFA_numbers[ig_unit_num]] * 4, DPU_XFER_DEFAULT)); // 전송 성공 여부 체크
                            
                            local_target_data_index[ig_unit_num] = new_local_target_data_index;

                            break;
                        }
                    }
                }
                else
                {
                    local_target_data_index[ig_unit_num] = new_local_target_data_index;
                }
            }
        }
    

        for(int ig_unit_num = 0; ig_unit_num < 4; ig_unit_num++)
        {
            if(to_be_ended_ig[ig_unit_num])
            {
                is_terminated[ig_unit_num] = 1;

                // Set MUX for DPUs
                OFIS_set_mux_ig(rank, ig_unit_num * 2, OPEN_TO_DPU);
                // Trigger DPU binary to continue iteration
                OFIS_set_state_ig(rank, ig_unit_num * 2, 0);
                
                // Set MUX for DPUs
                OFIS_set_mux_ig(rank, ig_unit_num * 2 + 1, OPEN_TO_DPU);
                // Trigger DPU binary to continue iteration
                OFIS_set_state_ig(rank, ig_unit_num * 2 + 1, 0);
            
                finish_count += 2;
            }

            else if (finished_ig[ig_unit_num])
            {
                DPU_FOREACH(rank, dpu, each_dpu)
                {
                    int __assigned_target_data_num = 0;
                    int my_ig_num = each_dpu % 8;
                    
                    if(my_ig_num / 2 == ig_unit_num)
                    {
                        __assigned_target_data_num = local_target_data_index[my_ig_num / 2] + 8 * (my_ig_num % 2) + (each_dpu / 8);
                        OFIS_prepare_xfer_ig(dpu, finish_ig, target_data[__assigned_target_data_num]);
                    }
                }
                DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_TO_DPU, "MRAM_target_text", 0, TARGET_TEXT_SIZE, DPU_XFER_DEFAULT));
                // Set MUX for DPUs
                OFIS_set_mux_ig(rank, ig_unit_num * 2, OPEN_TO_DPU);
                OFIS_set_state_ig(rank, ig_unit_num * 2, 2);
                OFIS_set_mux_ig(rank, ig_unit_num * 2 + 1, OPEN_TO_DPU);
                OFIS_set_state_ig(rank, ig_unit_num * 2 + 1, 2);
            }
        }
    }
}



int main(int argc, char* argv[]){

    if(argc != 6)
    {
        printf("Usage: %s [DPU amount] [Target data size(MB)] [Regex rule amount] [Number of threads] [result file name]\n", argv[0]);
        return 0;
    }

    
    int dpu_amount = atoi(argv[1]);
    int target_data_size = atoi(argv[2]);
    int regex_rule_amount = atoi(argv[3]);
    int nr_threads = atoi(argv[4]);
    char* result_file_name = argv[5];

    // mutex initializaion
    for(int i = 0; i < MAX_DFA_NUM; i++)
    {
        pthread_mutex_init(&partition_index_per_DFA_mutex[i], NULL);
        pthread_mutex_init(&global_result_save_mutex[i], NULL);
    }

    // timer initialization
    init_timer(&OFIS_execution_timer);

    // load DFA information
    load_DFA_info(regex_rule_amount);


    // load target data 
    load_target_data(target_data_size);

    // result array dynamic allocation
    for(int index_for_malloc = 0; index_for_malloc < MAX_DFA_NUM; index_for_malloc++)
    {
        result[index_for_malloc] = (int*)malloc(sizeof(int) * 2200000000);   
    }

    int DFAs_per_DPU[1024] = {};
    for(int i = 0; i < 1024; i++)
    {
        DFAs_per_DPU[i] = 1;
    }

    // DPU alloc
    struct dpu_set_t  set, dpu;
    int each_dpu = 0;

    gettimeofday(&start, NULL);
        DPU_ASSERT(dpu_alloc(dpu_amount, NULL, &set));
    gettimeofday(&end, NULL);
    alloc_time += get_time_difference(start, end);


    // DPU binary load
    gettimeofday(&start, NULL);
        DPU_ASSERT(dpu_load(set, OFIS_DPU_BINARY, NULL));
    gettimeofday(&end, NULL);
    load_time += get_time_difference(start, end);

    // thread args setting
    thread_args* args[nr_threads];

    for(int i = 0; i < nr_threads; i++)
    {
        args[i] = (thread_args*) malloc(sizeof(thread_args));
        
        args[i]->thread_id = i;
        for(int j = 0; j < 4; j++)
        {
            args[i]->assigned_DFA_numbers[j] = (i * 4 + j);
        }
        args[i]->rank = OFIS_get_rank(set, i);
    }

    start_timer(&OFIS_execution_timer, 0);
    // OFIS prallel execution
    OFIS_parallel_exec(nr_threads, OFIS_DPU_fct, (void**)args);
    end_timer(&OFIS_execution_timer, 0);

    // matching result check
    for(int i = 0; i < 64; i++)
    {
        if (i == 0) printf("matching result\n");
        printf("DFA %d: %lld\n", i, matching_amount_per_DFA[i] - (DFAs_per_DPU[i] * 2 + 2) * MAX_TEXT_NUM); // 구현 상 한번에 
    }

    // free result buffer
    for(int index_for_free = 0; index_for_free < MAX_DFA_NUM; index_for_free++)
    {
        if (result[index_for_free] != NULL)
        {
            free(result[index_for_free]);
            result[index_for_free] = NULL; 
        }
    }

    printf("execution time,%lf\n", OFIS_execution_timer.time[0]);

    FILE *fp = fopen(result_file_name, "a");

    if (fp == NULL) {
        perror("File is not opened\n");
    } else {
        fprintf(fp, "execution time,%lf\n", OFIS_execution_timer.time[0]);
        fclose(fp);
    }


}