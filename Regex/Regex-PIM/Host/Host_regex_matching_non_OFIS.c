#include <assert.h>
#include <dpu.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <dpu_log.h>
#include "../DFA_library/make_DFA.h"
#include <omp.h>

int* result[2048];
int result_matched_num[2048][2];

#define MAX_DFA_NUM 64
#define MAX_TEXT_NUM 2048
#define DFA_INFO_BUFFER_SIZE (256 * 1024)
#define TARGET_TEXT_SIZE (512 * 1024)

int* DFA_info[MAX_DFA_NUM];
int DFA_info_size[MAX_DFA_NUM];

char target_data[2048][1 << 19];

typedef struct Timer{
    struct timeval start[32];
    struct timeval end[32];
    double time[32];
} Timer;

Timer execution_timer;

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

#define ALLOC_TIME 0
#define LOAD_TIME 1
#define COPY_IN_TIME 2
#define RUN_TIME 3
#define COPY_OUT_TIME 4
#define MERGE_TIME 5

int index;


long long int matching_amount_per_DFA[MAX_DFA_NUM];


void target_data_load(int target_data_size)
{
    for(int i = 0; i < target_data_size * 2; i++)
    {
        char test_file_name[100] = {};
        sprintf(test_file_name, "../dataset/Text/text_%d.txt", i);
        FILE* fp = fopen(test_file_name, "r");
        if(fp == NULL)
        {
            printf("file %d open error\n", i);
            exit(0);
        }

        fgets(target_data[i], 1 << 19, fp);
        fclose(fp);
    }
}

void matching_start(int dpu_amount, int target_data_size)
{

    int data_per_dpu = TARGET_TEXT_SIZE;

    // DPU alloc
    struct dpu_set_t  set, dpu;
    int each_dpu = 0;

start_timer(&execution_timer, ALLOC_TIME);
    DPU_ASSERT(dpu_alloc(dpu_amount, NULL, &set));
end_timer(&execution_timer, ALLOC_TIME);



// DPU binary load
start_timer(&execution_timer, LOAD_TIME);
    DPU_ASSERT(dpu_load(set, "./bin/DPU_non_ofis", NULL));
end_timer(&execution_timer, LOAD_TIME);

// DPU copy-in
start_timer(&execution_timer, COPY_IN_TIME);
    DPU_FOREACH(set, dpu, each_dpu)
    {
        int assigned_DFA_num = each_dpu / (dpu_amount/64);
        DPU_ASSERT(dpu_prepare_xfer(dpu, DFA_info[assigned_DFA_num]));
    }

    int max_DFA_info_size = 0;
    for(int i = 0; i < MAX_DFA_NUM; i++)
    {
        if(max_DFA_info_size < DFA_info_size[i])
        {
            max_DFA_info_size = DFA_info_size[i];
        }
    }
    if (max_DFA_info_size % 2 != 0) {
        max_DFA_info_size++; 
    }
    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "MRAM_DFA_set", 0, max_DFA_info_size * 4, DPU_XFER_DEFAULT));
end_timer(&execution_timer, COPY_IN_TIME);



// Target_data transfer and matching
    for(int i = 0; i < MAX_TEXT_NUM; i += (dpu_amount/64))
    {
        // printf("%dth loop\n", i/(dpu_amount/64));
        start_timer(&execution_timer, COPY_IN_TIME);
            DPU_FOREACH(set, dpu, each_dpu)
            {
                DPU_ASSERT(dpu_prepare_xfer(dpu, target_data[i + each_dpu % (dpu_amount/64)]));
            }
            DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "MRAM_target_text", 0, data_per_dpu, DPU_XFER_DEFAULT));
        end_timer(&execution_timer, COPY_IN_TIME);
        
        // DPU launch
        start_timer(&execution_timer, RUN_TIME);
            DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));
        end_timer(&execution_timer, RUN_TIME);

        // get matching result
        start_timer(&execution_timer, COPY_OUT_TIME);
            // get matching amount
            DPU_FOREACH(set, dpu, each_dpu)
            {
                DPU_ASSERT(dpu_prepare_xfer(dpu, result_matched_num[each_dpu]));
            }
            DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_FROM_DPU, "MRAM_output_metadata", 0, 8, DPU_XFER_DEFAULT));


            // matched 횟수의 최대값 결정
            int matched_max = -1;
            for(int j = 0; j < dpu_amount; j++)
            {
                if(matched_max < result_matched_num[j][0]) matched_max = result_matched_num[j][0];
            }
            
            int* temp_result[2048] = {};
            for(int j = 0; j < dpu_amount; j++)
            {
                temp_result[j] = (int*)malloc(sizeof(int) * matched_max);    
            }

            // copy-out
            DPU_FOREACH(set, dpu, each_dpu)
            {
                DPU_ASSERT(dpu_prepare_xfer(dpu, temp_result[each_dpu]));
            }
            DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_FROM_DPU, "MRAM_output_buffer", 0, 4 * matched_max, DPU_XFER_DEFAULT));
        end_timer(&execution_timer, COPY_OUT_TIME);      



        int precalculated_offsets[1024];

        // 1. position calculation
        start_timer(&execution_timer, MERGE_TIME);
        #pragma omp parallel for num_threads(16)
        for (int group = 0; group < 64; group++) 
        {
            int base_idx = group * (dpu_amount/64);
            
            
            long long current_offset = matching_amount_per_DFA[group];

            for (int i = 0; i < (dpu_amount/64); i++) 
            {
                int real_idx = base_idx + i;
                
                precalculated_offsets[real_idx] = current_offset;
                
                current_offset += (long long int)result_matched_num[real_idx][0];
            }
            
            matching_amount_per_DFA[group] = current_offset;
        }

        // 2. data copy
        #pragma omp parallel for num_threads(16)
        for(int index_for_res_collection = 0; index_for_res_collection < dpu_amount; index_for_res_collection++)
        {
            int dfa_idx = index_for_res_collection / (dpu_amount/64);
            
            int start_index = precalculated_offsets[index_for_res_collection];
            int result_amount = result_matched_num[index_for_res_collection][0];

            for(int i = 0; i < result_amount; i++)
            {
                result[dfa_idx][start_index + i] = temp_result[index_for_res_collection][i];
            }
        }
        end_timer(&execution_timer, MERGE_TIME);

        

        for(int j = 0; j < dpu_amount; j++)
        {
            free(temp_result[j]);
        }
    }

    
    dpu_free(set);
    
}


void load_DFA_info(int regex_rule_amount)
{
    for(int i = 0; i < regex_rule_amount; i++)
    {
        DFA_info[i] = (int*)malloc(sizeof(int) * (DFA_INFO_BUFFER_SIZE));    

        int local_DFA_index = 0;
        char DFA_info_file_name[100] = {};
        sprintf(DFA_info_file_name, "../dataset/DFA_info/DFA_%d.csv", i);
        DFA_info_size[i] = make_BUF(DFA_info[i], DFA_info_file_name);
        // printf("DFA_%d size: %d\n", i, DFA_info_size[i]);
        local_DFA_index = DFA_info_size[i];    
        DFA_info[i][local_DFA_index] = -99;
        DFA_info[i][local_DFA_index + 1] = -99;
        DFA_info_size[i]+=2;
    }
}




int main(int argc, char* argv[]){

    if(argc != 5)
    {
        printf("Usage: %s [DPU amount] [Target data size(MB)] [Regex rule amount] [result file name]\n", argv[0]);
        return 0;
    }

    
    int dpu_amount = atoi(argv[1]);
    int target_data_size = atoi(argv[2]);
    int regex_rule_amount = atoi(argv[3]);

    char* result_file_name = argv[4];

    // timer initialization
    init_timer(&execution_timer);

    // load DFA information
    load_DFA_info(regex_rule_amount);

    // Setting number of transfered DFAs per DPU
    int DFAs_per_DPU[1024] = {};
    for(int i = 0; i < 1024; i++)
    {
        DFAs_per_DPU[i] = 1;
    }

    target_data_load(target_data_size);
    
    // result array dynamic allocation
    for(int index_for_malloc = 0; index_for_malloc < MAX_DFA_NUM; index_for_malloc++)
    {
        result[index_for_malloc] = (int*)malloc(sizeof(int) * 2200000000);   
    }
    
    matching_start(dpu_amount, target_data_size);

    // free result buffer
    for(int index_for_free = 0; index_for_free < MAX_DFA_NUM; index_for_free++)
    {
        if (result[index_for_free] != NULL)
        {
            free(result[index_for_free]);
            result[index_for_free] = NULL; 
        }
    }
    
    for(int index_for_res_collection = 0; index_for_res_collection < MAX_DFA_NUM; index_for_res_collection++)
    {
        printf("%d: %lld\n", index_for_res_collection, matching_amount_per_DFA[index_for_res_collection] - (DFAs_per_DPU[index_for_res_collection] * 2 + 2) * MAX_TEXT_NUM);
    }


    printf("alloc_time,%lf\n", execution_timer.time[0]);
    printf("load_time,%lf\n", execution_timer.time[1]);
    printf("copy_in_time,%lf\n", execution_timer.time[2]);
    printf("run_time,%lf\n", execution_timer.time[3]);
    printf("copy_out_time,%lf\n", execution_timer.time[4]);
    printf("merge_time,%lf\n", execution_timer.time[5]);

    // File open
    FILE *fp = fopen(result_file_name, "a");

    // Exeption
    if (fp == NULL) {
        perror("File is not opened\n");
    } else {
        fprintf(fp, "alloc_time,%lf\n", execution_timer.time[0]);
        fprintf(fp, "load_time,%lf\n", execution_timer.time[1]);
        fprintf(fp, "copy_in_time,%lf\n", execution_timer.time[2]);
        fprintf(fp, "run_time,%lf\n", execution_timer.time[3]);
        fprintf(fp, "copy_out_time,%lf\n", execution_timer.time[4]);
        fprintf(fp, "merge_time,%lf\n", execution_timer.time[5]);

        fclose(fp);
    }

}