#include <assert.h>
#include <dpu.h>
#include <dpu_management.h>
#include <ofis.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include "./includes/types.h"
#include "./includes/common.h"
#include "./includes/utils.h"

#include <sys/time.h>
#include <pthread.h>
#include <sched.h>
// #include <omp.h>

#ifndef DPU_BINARY
#define DPU_BINARY "./bin/spmv_2D_dpu"
#endif

#define OPEN_TO_DPU 0
#define OPEN_TO_HOST 1

#define MAX_N_DPU 1280
#define MAX_N_RANK 20
#define MAX_N_THREAD 20

uint32_t p_count; // to count nr_part sent to DPUs
pthread_mutex_t p_count_mutex;
pthread_mutex_t result_mutex;

typedef struct Timer{
    struct timeval start[16];
    struct timeval end[16];
    double time[16];
} Timer;

Timer global_exec_timer;
Timer global_send_timer;
Timer global_retr_timer;

void spmv_host(val_dt* output_vec, struct CSR_2D_format* A, val_dt* input_vec){
    uint64_t acc = 0;
    for(int i = 0; i < A->nr_horiz; ++i){
        for(int j = 0; j < A->nr_vert; ++j){
            for(int k = 0; k < A->height; ++k){
                val_dt sum = 0;

                uint64_t ptr_offset = (uint64_t)i * (uint64_t)A->nr_vert + (uint64_t)j;
                ptr_offset *= (uint64_t)(A->height + 1);
                uint32_t row_offset = i * A->height;
                uint32_t col_offset = j * A->width;
                for(uint32_t n = A->row_ptr[ptr_offset + k]; n < A->row_ptr[ptr_offset + k + 1]; ++n){
                    uint32_t col_idx = A->col_idx[acc];
                    val_dt value = A->values[acc++];
                    sum += input_vec[col_offset + col_idx] * value;
                }
                output_vec[row_offset + k] += sum;
            }
        }
    }
}

struct dpu_info_t{
    uint32_t nr_rows;
    uint32_t start_row;
    uint32_t start_idx;
    uint32_t nnz;
    uint64_t start_row_ptr;
};
struct dpu_info_t* dpu_info;
dpu_args_t* input_args;

val_dt* output_vec;

void init_timer(Timer* timer, int i){
    timer->time[i] = 0.0;
}

void start_timer(Timer* timer, int i){
    gettimeofday(&timer->start[i], NULL);
}

void end_timer(Timer* timer, int i){
    gettimeofday(&timer->end[i], NULL);
    timer->time[i] += (timer->end[i].tv_sec - timer->start[i].tv_sec) + ((double)(timer->end[i].tv_usec - timer->start[i].tv_usec) / 1000000);
}

typedef struct{
    uint32_t thread_id;
    uint32_t max_nr_rows_dpu;
    uint32_t width_pad;
    uint32_t max_nnz;

    struct dpu_set_t rank;
    struct CSR_2D_format* csr_m;

    val_dt* input_vec;
} thread_args;

void* thread_fct(void* arg){

    thread_args* args = (thread_args*)arg;
    uint32_t thread_id = args->thread_id;
    uint32_t rank_id = thread_id;

    // Set affinity to thread
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(thread_id, &cpu_set);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set);

    init_timer(&global_exec_timer, rank_id);
    init_timer(&global_send_timer, rank_id);
    init_timer(&global_retr_timer, rank_id);

    uint32_t each_dpu;
    struct CSR_2D_format* csr_m = args->csr_m;

    uint32_t nr_dpus_per_thread = 64;
    val_dt* input_vec = args->input_vec;
   
    struct dpu_set_t dpu;
    struct dpu_set_t rank = args->rank;

    uint32_t part_dpu[MAX_N_DPU];

    // offset setting for send data
    uint32_t out_size = args->max_nr_rows_dpu * sizeof(uint32_t);
    uint32_t vec_size = args->width_pad * sizeof(uint32_t);
    uint32_t row_size = out_size;
    uint32_t col_size = args->max_nnz * sizeof(uint32_t);
    uint32_t val_size = col_size;

    uint32_t out_base = 0;
    uint32_t vec_base = out_base + out_size;
    uint32_t row_base = vec_base + vec_size;
    uint32_t col_base = row_base + row_size;
    uint32_t val_base = col_base + col_size;

    uint32_t max_nnz_part = 0;

    uint32_t rank_idx = 64 * rank_id;
    start_timer(&global_send_timer, rank_id);
    // 1-1. Send DPU_INPUT_ARGS
    DPU_FOREACH(rank, dpu, each_dpu){
        uint32_t idx = rank_idx + each_dpu;
        part_dpu[idx] = idx;
        input_args[idx].max_nr_rows = args->max_nr_rows_dpu;
        input_args[idx].max_nnz = args->max_nnz;
        if(dpu_info[idx].nnz > max_nnz_part) max_nnz_part = dpu_info[idx].nnz;
        DPU_ASSERT(dpu_prepare_xfer(dpu, input_args + idx));
    }
    DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_TO_DPU, "DPU_INPUT_ARGS", 0, sizeof(dpu_args_t), DPU_XFER_DEFAULT));

    // 1-2. Send row_ptr
    DPU_FOREACH(rank, dpu, each_dpu){
        uint32_t idx = rank_idx + each_dpu;
        DPU_ASSERT(dpu_prepare_xfer(dpu, csr_m->row_ptr + dpu_info[idx].start_row_ptr));
    }
    DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, row_base, row_size, DPU_XFER_DEFAULT));

    if(max_nnz_part % 2 != 0) max_nnz_part += 1;
    uint32_t xfer_size = max_nnz_part * sizeof(uint32_t);
    
    // 1-3. Send col_idx
    DPU_FOREACH(rank, dpu, each_dpu){
        uint32_t idx = rank_idx + each_dpu;
        DPU_ASSERT(dpu_prepare_xfer(dpu, csr_m->col_idx + dpu_info[idx].start_idx));
    }
    DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, col_base, xfer_size, DPU_XFER_DEFAULT));

    // 1-4. Send values
    DPU_FOREACH(rank, dpu, each_dpu){
        uint32_t idx = rank_idx + each_dpu;
        DPU_ASSERT(dpu_prepare_xfer(dpu, csr_m->values + dpu_info[idx].start_idx));
    }
    DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, val_base, xfer_size, DPU_XFER_DEFAULT));

    // 1-5. Send input_vector
    DPU_FOREACH(rank, dpu, each_dpu){
        uint32_t idx = rank_idx + each_dpu;
        uint32_t start_p_col = idx % csr_m->nr_vert;
        DPU_ASSERT(dpu_prepare_xfer(dpu, input_vec + start_p_col * csr_m->width));
    }
    DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, vec_base, vec_size, DPU_XFER_DEFAULT));
    end_timer(&global_send_timer, rank_id);
    start_timer(&global_exec_timer, rank_id);
    // Boot DPUs without the Polling Threads
    OFIS_dpu_launch(rank);

    int finish_count = 0;
    while (finish_count < nr_dpus_per_thread){

        // Read all OFIS_dpu_state of rank
        uint32_t is_rank_finish = 0;
        do{
            is_rank_finish = OFIS_get_finished_rank(rank);
        }while(is_rank_finish == 0);

        end_timer(&global_exec_timer, rank_id);

        uint32_t change_state = -1; // OFIS_dpu_state send to DPU

        // Set MUX for the CPU (To Read & Write data)
        OFIS_set_mux_rank(rank, OPEN_TO_HOST);
        
        start_timer(&global_retr_timer, rank_id);
        // Read Interim results from all DPUs in the Rank
        DPU_FOREACH(rank, dpu, each_dpu){
            uint32_t idx = rank_idx + each_dpu;
            DPU_ASSERT(dpu_prepare_xfer(dpu, output_vec + (part_dpu[idx] * args->max_nr_rows_dpu)));
        }
        dpu_push_xfer(rank, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, out_base, out_size, DPU_XFER_DEFAULT);
        end_timer(&global_retr_timer, rank_id);

        pthread_mutex_lock(&p_count_mutex);
        if (p_count >= csr_m->nr_part){ // Trigger DPU Binary to exit
            pthread_mutex_unlock(&p_count_mutex);
            change_state = 0;
            finish_count += 64;
        }else{
            uint32_t curr_p_count = p_count;
            p_count += nr_dpus_per_thread;
            pthread_mutex_unlock(&p_count_mutex);

            start_timer(&global_send_timer, rank_id);
            change_state = 2;
            uint32_t max_nnz = 0;

            // Transfer additional partitions to DPUs in the rank
            // Send DPU_INPUT_ARGS
            DPU_FOREACH(rank, dpu, each_dpu){
                uint32_t idx = rank_idx + each_dpu;
                input_args[curr_p_count + each_dpu].max_nr_rows = args->max_nr_rows_dpu;
                input_args[curr_p_count + each_dpu].max_nnz = args->max_nnz;
                part_dpu[idx] = curr_p_count + each_dpu;
                if (dpu_info[curr_p_count + each_dpu].nnz > max_nnz)
                    max_nnz = dpu_info[curr_p_count + each_dpu].nnz;
                DPU_ASSERT(dpu_prepare_xfer(dpu, input_args + curr_p_count + each_dpu));
            }
            dpu_push_xfer(rank, DPU_XFER_TO_DPU, "DPU_INPUT_ARGS", 0, sizeof(dpu_args_t), DPU_XFER_DEFAULT);

            // Send row_ptr
            DPU_FOREACH(rank, dpu, each_dpu){
                uint32_t idx = curr_p_count + each_dpu;
                DPU_ASSERT(dpu_prepare_xfer(dpu, csr_m->row_ptr + dpu_info[idx].start_row_ptr));
            }
            dpu_push_xfer(rank, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, row_base, row_size, DPU_XFER_DEFAULT);

            // Send col_idx
            if (max_nnz % 2 != 0)
                max_nnz += 1;
            uint32_t xfer_size = max_nnz * sizeof(uint32_t);
            DPU_FOREACH(rank, dpu, each_dpu){
                uint32_t idx = curr_p_count + each_dpu;
                DPU_ASSERT(dpu_prepare_xfer(dpu, csr_m->col_idx + dpu_info[idx].start_idx));
            }
            dpu_push_xfer(rank, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, col_base, xfer_size, DPU_XFER_DEFAULT);

            // Send values
            DPU_FOREACH(rank, dpu, each_dpu){
                uint32_t idx = curr_p_count + each_dpu;
                DPU_ASSERT(dpu_prepare_xfer(dpu, csr_m->values + dpu_info[idx].start_idx));
            }
            dpu_push_xfer(rank, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, val_base, xfer_size, DPU_XFER_DEFAULT);

            // Send input_vec
            DPU_FOREACH(rank, dpu, each_dpu){
                uint32_t idx = curr_p_count + each_dpu;
                DPU_ASSERT(dpu_prepare_xfer(dpu, input_vec + (idx % csr_m->nr_vert) * csr_m->width));
            }
            dpu_push_xfer(rank, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, vec_base, vec_size, DPU_XFER_DEFAULT);
            end_timer(&global_send_timer, rank_id);
        }

        if(change_state == 2) start_timer(&global_exec_timer, rank_id);
        // Set MUX for DPUs
        OFIS_set_mux_rank(rank, OPEN_TO_DPU);
        // Trigger DPU binary to continue iteration
        OFIS_set_state_rank(rank, change_state);
    }
    return NULL;
}

int main(int argc, char** argv){

    uint32_t nr_dpus = atoi(argv[1]);
    uint32_t nr_thread = atoi(argv[3]);

    // if use BA data
    char path[50] = "../dataset/";
    char type[50] = ".txt";

    char filename[256];
    strcat(path, argv[4]);
    strcat(path, type);
    strcpy(filename, path);

    uint32_t nr_ranks = nr_dpus / 64 + (nr_dpus % 64 == 0 ? 0 : 1);

    Timer timer;

    // Load sparse matrix in CSR format
    // init_timer(&timer, 0);
    // struct COO_format* coo_m = get_COO_matrix_rev(filename);
    // printf("get coo done\n");
    // uint32_t bool_test = sort_COO(coo_m); // too slow to sort
    // struct CSR_2D_format* csr_m = COO_to_CSR_2D(coo_m, nr_horiz_part, nr_vert_part, &scale_factor);
    // start_timer(&timer, 0);

    uint32_t nr_horiz_part = atoi(argv[2]);
    uint32_t nr_vert_part = nr_horiz_part;

    struct CSR_2D_format* csr_m = load_dcsr_matrix(filename);

    if(csr_m == NULL){
        printf("data exceed MRAM\n");
        exit(1);
    }

    printf("nr_parts: %d\n", csr_m->nr_part);
    // free_COO(coo_m);
    // end_timer(&timer, 0);

    // printf("Convert to CSR: %lf\n", timer.time[0]);
    printf("File name: %s\n", filename);

    for(int i = 1; i < 6; ++i){
        init_timer(&timer, i);
    }
    start_timer(&timer, 1);
    uint32_t rows_pad = csr_m->nr_horiz * csr_m->height;
    uint32_t cols_pad = csr_m->nr_vert * csr_m->width;
    uint32_t width_pad = csr_m->width;
    if(rows_pad % 2 != 0) rows_pad += 1;
    if(cols_pad % 2 != 0) cols_pad += 1;
    if(width_pad % 2 != 0) width_pad += 1;

    // Allocate input / output vector
    val_dt* input_vec = (val_dt*)malloc(cols_pad * sizeof(uint32_t));

    p_count = nr_dpus;

    // Initialize input vector
    for(uint32_t i = 0; i < cols_pad; ++i){
        input_vec[i] = (val_dt)(i % 4 + 1); 
    }

    uint32_t max_nr_rows_dpu = 0, max_nnz = 0;
    uint32_t acc = 0; // to find start_idx (for col_idx & values)

    dpu_info = (struct dpu_info_t*)malloc(csr_m->nr_part * sizeof(struct dpu_info_t));
    input_args = (dpu_args_t*)malloc(csr_m->nr_part * sizeof(dpu_args_t));

    // input parameter Initialization
    for(uint32_t p_idx = 0; p_idx < csr_m->nr_part; ++p_idx){
        uint32_t nr_rows = csr_m->height;
        uint32_t start_p_row = p_idx / csr_m->nr_vert;
        uint32_t start_p_col = p_idx % csr_m->nr_vert;
        uint32_t start_row = start_p_row * csr_m->height;
        uint32_t nnz = csr_m->nnz_per_part[p_idx];

        uint32_t rows_pad_dpu = nr_rows + 1;
        if(rows_pad_dpu % 2 != 0) rows_pad_dpu += 1;
        if(rows_pad_dpu > max_nr_rows_dpu) max_nr_rows_dpu = rows_pad_dpu;

        uint32_t nnz_pad = nnz;
        if(nnz_pad % 2 != 0) nnz_pad += 1;
        if(nnz_pad > max_nnz) max_nnz = nnz_pad;

        uint32_t start_idx = acc;
        acc += nnz;

        dpu_info[p_idx].nr_rows = nr_rows;
        dpu_info[p_idx].start_row = start_row;
        dpu_info[p_idx].start_idx = start_idx;
        dpu_info[p_idx].nnz = nnz;
        dpu_info[p_idx].start_row_ptr = ((uint64_t)start_p_row * (uint64_t)csr_m->nr_vert + (uint64_t)start_p_col) * (uint64_t)(csr_m->height + 1);
    
        input_args[p_idx].nr_rows = nr_rows;
        input_args[p_idx].nr_cols = width_pad;
        input_args[p_idx].nnz = nnz;
    }

    if(max_nr_rows_dpu % 2 != 0)
        max_nr_rows_dpu += 1;
    if(max_nnz % 2 != 0)
        max_nnz += 1;

    csr_m->col_idx = (uint32_t*)realloc(csr_m->col_idx, (csr_m->nnz + max_nnz) * sizeof(uint32_t));
    csr_m->values = (val_dt*)realloc(csr_m->values, (csr_m->nnz + max_nnz) * sizeof(uint32_t));   
    output_vec = (val_dt*)malloc(csr_m->nr_part * max_nr_rows_dpu * sizeof(val_dt));

    end_timer(&timer, 1);

    // 1. Initialize the first data (for all DPUs)
    struct dpu_set_t dpu, set;
    uint32_t each_dpu;

    // DPU Alloc and Load
    start_timer(&timer, 2);
    dpu_alloc(nr_dpus, NULL, &set);
    dpu_load(set, DPU_BINARY, NULL);
    end_timer(&timer, 2);

    start_timer(&timer, 3);
    pthread_mutex_init(&p_count_mutex, NULL);
    pthread_mutex_init(&result_mutex, NULL);

    thread_args* args[nr_thread];
    for(int i = 0; i < nr_thread; ++i){
        args[i] = (thread_args*)malloc(sizeof(thread_args));
        args[i]->thread_id = i;
        args[i]->csr_m = csr_m;
        args[i]->input_vec = input_vec;
        args[i]->max_nr_rows_dpu = max_nr_rows_dpu;
        args[i]->max_nnz = max_nnz;
        args[i]->width_pad = width_pad;
        args[i]->rank = OFIS_get_rank(set, i);
    }

    OFIS_parallel_exec(nr_thread, thread_fct, (void**)args);
    end_timer(&timer, 3);

    dpu_free(set);

    val_dt* result_vec = (val_dt*)calloc(csr_m->nr_rows, sizeof(val_dt));
    start_timer(&timer, 4);

    uint32_t i, j;
#pragma omp parallel for num_threads(64) shared (csr_m, result_vec, max_nr_rows_dpu) private(i,j)
    for(i = 0; i < csr_m->nr_part; ++i){
        uint32_t row_p_idx = i / csr_m->nr_vert;
        uint32_t start_row = row_p_idx * csr_m->height;

        for(j = 0; j < csr_m->height; ++j){
            uint32_t row = start_row + j;
            if(row >= csr_m->nr_rows) break;
            #pragma omp atomic
                result_vec[row] += output_vec[i * max_nr_rows_dpu + j];
        }
    }

    end_timer(&timer, 4);
    printf("dpu(%d, %d):\t %u\n", nr_dpus, nr_vert_part, csr_m->nnz);

    val_dt* host_vec = (val_dt*)calloc(rows_pad, sizeof(uint32_t));

    start_timer(&timer, 5);
    spmv_host(host_vec, csr_m, input_vec);
    end_timer(&timer, 5);

    bool result = true;
    uint32_t err_count = 0;
    for(uint32_t i = 0; i < csr_m->nr_rows; ++i){
        // if(host_vec[i] != result_vec[i]){
        if(fabs(host_vec[i] - result_vec[i]) > 1e-6){
            result = false;
            err_count += 1;
        }
    }

    val_dt sum_dpu = 0, sum_host = 0;
    for(uint32_t i = 0; i < csr_m->nr_rows; ++i){
        sum_dpu += result_vec[i];
        sum_host += host_vec[i];
    }

    if(!result){
        printf("dpu:\t %lf\n", sum_dpu);
        printf("host:\t %lf\n", sum_host);
        printf("err:\t %d\n", err_count);
    }
    
    if(result) printf("Result:\t\t \033[34mCorrect\033[0m\n");
    else printf("Result:\t\t \033[31mIncorrect\033[0m\n");
    free(host_vec);

    // printf("Data prepare:\t %lf\n", timer.time[1]);
    // printf("Alloc & Load:\t %lf\n", timer.time[2]);
    // printf("DPU Exec:\t %lf\n", timer.time[3]);
    // printf("Merge results:\t %lf\n", timer.time[4]);
    // printf("Total_time:\t %lf\n", tot_time);
    // printf("Host(CPU):\t %lf\n", timer.time[5]);
    printf("--------------------------------------\n");

    double tot_time = 0;
    tot_time = timer.time[3] + timer.time[4];

    char result_filename[50];
    if (nr_horiz_part == 256)
        strcpy(result_filename, "./results/OFIS_rank256.csv");
    else if (nr_horiz_part == 512)
        strcpy(result_filename, "./results/OFIS_rank512.csv");

    FILE* fp2 = fopen(result_filename, "a");

    double avg_exec_time = 0;
    double max_exec_time = -1;
    double min_exec_time = global_exec_timer.time[0];

    double avg_send_time = 0;
    double max_send_time = -1;
    double min_send_time = global_send_timer.time[0];

    double avg_retr_time = 0;
    double max_retr_time = -1;
    double min_retr_time = global_retr_timer.time[0];

    for(int i = 0; i < nr_thread; ++i){
        avg_exec_time += global_exec_timer.time[i];
        if(global_exec_timer.time[i] > max_exec_time) max_exec_time = global_exec_timer.time[i];
        if(global_exec_timer.time[i] < min_exec_time) min_exec_time = global_exec_timer.time[i];
        
        avg_send_time += global_send_timer.time[i];
        if(global_send_timer.time[i] > max_send_time) max_send_time = global_send_timer.time[i];
        if(global_send_timer.time[i] < min_send_time) min_send_time = global_send_timer.time[i];

        avg_retr_time += global_retr_timer.time[i];
        if(global_retr_timer.time[i] > max_retr_time) max_retr_time = global_retr_timer.time[i];
        if(global_retr_timer.time[i] < min_retr_time) min_retr_time = global_retr_timer.time[i];
    }
    avg_exec_time /= nr_thread;
    avg_send_time /= nr_thread;
    avg_retr_time /= nr_thread;

    // file print
    if(fp2 == NULL) exit(1);
    // nr_dpus | merge_time | avg_send | avg_retrieve | avg_exec | DPU_EXEC | Total
    fprintf(fp2, "%d, %f, %f, %f, %f, %f, %f\n", nr_dpus, timer.time[4], avg_send_time, avg_retr_time, avg_exec_time, timer.time[3], tot_time);
    
    fclose(fp2);
    
    // Deallocation
    pthread_mutex_destroy(&result_mutex);
    pthread_mutex_destroy(&p_count_mutex);
    free(input_vec);
    free(output_vec);
    free(result_vec);
    free_CSR_2D(csr_m);
    free(dpu_info);
    free(input_args);
    for(int i = 0; i < nr_thread; ++i){
        free(args[i]);
    }

    return 0;
}