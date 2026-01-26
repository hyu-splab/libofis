#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include "./includes/types.h"
#include "./includes/common.h"
#include "./includes/utils.h"

#include <assert.h>
#include <sys/time.h>
#include <pthread.h>

#include <sched.h>
// #include <omp.h>

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

typedef struct {
    uint32_t thread_id;
    struct CSR_2D_format* csr_m;
    val_dt* input_vec;
    val_dt* output_vec;
    uint32_t start_row;
    uint32_t end_row;
} thread_args;

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

void* spmv_worker(void* args){
    thread_args* arg = (thread_args*)args;

    for(uint32_t i = arg->start_row; i < arg->end_row; ++i){
        val_dt sum = 0.0;
        for(uint32_t j = arg->csr_m->row_ptr[i]; j < arg->csr_m->row_ptr[i + 1]; ++j){
            sum += arg->csr_m->values[j] * arg->input_vec[arg->csr_m->col_idx[j]];
        }
        arg->output_vec[i] = sum;
    }

    pthread_exit(NULL);
}

void spmv_thread(struct CSR_2D_format* csr_m, val_dt* input_vec, val_dt* output_vec, int num_threads){
    pthread_t threads[num_threads];
    thread_args args[num_threads];

    uint32_t chunk = csr_m->nr_rows / num_threads;

    for(int t = 0; t < num_threads; ++t){
        args[t].csr_m = csr_m;
        args[t].input_vec = input_vec;
        args[t].output_vec = output_vec;
        args[t].start_row = t * chunk;
        args[t].end_row = (t == num_threads - 1) ? csr_m->nr_rows : (t + 1) * chunk;

        pthread_create(&threads[t], NULL, spmv_worker, &args[t]);
    }

    for(int t = 0; t < num_threads; ++t){
        pthread_join(threads[t], NULL);
    }
}

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

int main(int argc, char** argv){

    uint32_t nr_thread = atoi(argv[1]);

    char path[50] = "../dataset/";

    char filename[256];
    strcat(path, argv[2]);
    strcpy(filename, path);
    
    Timer timer;

    struct CSR_2D_format* csr_m;
    csr_m = load_csr(filename);

    if(csr_m == NULL){
        printf("data exceed MRAM\n");
        exit(1);
    }

    printf("File name: %s\n", filename);

    for(int i = 0; i < 5; ++i){
        init_timer(&timer, i);
    }
    uint32_t rows_pad = csr_m->nr_horiz * csr_m->height;
    uint32_t cols_pad = csr_m->nr_vert * csr_m->width;
    uint32_t width_pad = csr_m->width;
    if(rows_pad % 2 != 0) rows_pad += 1;
    if(cols_pad % 2 != 0) cols_pad += 1;
    if(width_pad % 2 != 0) width_pad += 1;

    // Allocate input / output vector
    val_dt* input_vec = (val_dt*)malloc(cols_pad * sizeof(val_dt));

    // Initialize input vector
    for(uint32_t i = 0; i < cols_pad; ++i){
        input_vec[i] = (val_dt)(i % 4 + 1);
    }

    val_dt* output_vec = (val_dt*)calloc(csr_m->nr_rows, sizeof(val_dt));

    start_timer(&timer, 0);
    spmv_thread(csr_m, input_vec, output_vec, 64);
    end_timer(&timer, 0);

    val_dt* host_check_vec = (val_dt*)calloc(csr_m->nr_rows, sizeof(val_dt));
    start_timer(&timer, 1);
    spmv_host(host_check_vec, csr_m, input_vec);
    end_timer(&timer, 1);

    val_dt* output_vec2 = (val_dt*)calloc(csr_m->nr_rows, sizeof(val_dt));

    bool result = true;
    uint32_t err_count = 0;
    for(uint32_t i = 0; i < csr_m->nr_rows; ++i){
        if(fabs(host_check_vec[i] - output_vec[i]) > 1e-6){
            result = false;
            err_count += 1;
        }
    }

    val_dt sum_host = 0, sum_check = 0;
    val_dt sum_host2 = 0;
    for(uint32_t i = 0; i < csr_m->nr_rows; ++i){
        sum_host += output_vec[i];
        sum_check += host_check_vec[i];
        sum_host2 += output_vec2[i];
    }
    if(!result){
        printf("multi_host:\t %lf\n", sum_host);
        printf("host:\t %lf\n", sum_check);
        printf("err:\t %d\n", err_count);
    }

    if(result) printf("Result:\t\t \033[34mCorrect\033[0m\n");
    else printf("Result:\t\t \033[31mIncorrect\033[0m\n");
    free(host_check_vec);

    printf("--------------------------------------\n");

    char result_filename[50];
    strcpy(result_filename, "./results/CPU_pthread.csv");

    FILE* fp2 = fopen(result_filename, "a");

    // file print
    if(fp2 == NULL) exit(1);

    // nr_thread | CPU exec
    fprintf(fp2, "%d, %f\n", nr_thread, timer.time[0]);
    
    fclose(fp2);
    
    // Deallocation
    free(input_vec);
    free(output_vec);
    free_CSR_2D(csr_m);

    return 0;
}