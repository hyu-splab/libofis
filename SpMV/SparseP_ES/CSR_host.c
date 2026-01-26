#include <assert.h>
#include <dpu.h>
#include <dpu_management.h>
#include <dpu_runner.h>
#include <ufi/ufi_ci.h>
#include <ufi/ufi_ci_commands.h>
#include <ufi/ufi_config.h>
#include <ufi/ufi.h>
#include <ufi/ufi_runner.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include "./includes/utils.h"
#include "./includes/types.h"
#include "./includes/common.h"
#include <omp.h>

#include <sys/time.h>

#ifndef DPU_BINARY
#define DPU_BINARY "./bin/spmv_2D_dpu"
#endif

#define SIN_VER 0
#define OMP_VER 1

// Things that need to be transmitted
// 1. Matrices
// 2. Input vector

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

typedef struct Timer{
    struct timeval start[20];
    struct timeval end[20];
    double time[20];
} Timer;

struct dpu_info_t{
    uint32_t nr_rows;
    uint32_t start_row;
    uint32_t start_idx;
    uint32_t nnz;
    uint64_t start_row_ptr;
};
struct dpu_info_t* dpu_info;

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

int compare_CSR(struct CSR_2D_format* csr1, struct CSR_2D_format* csr2){
    if(csr1->nr_rows != csr2->nr_rows){
        printf("nr_row: %d %d\n", csr1->nr_rows, csr2->nr_rows);
    }
    if(csr1->nr_cols != csr2->nr_cols){
        printf("nr_col: %d %d\n", csr1->nr_cols, csr2->nr_cols);
    }
    if(csr1->nnz != csr2->nnz){
        printf("nnz: %d %d\n", csr1->nnz, csr2->nnz);
    }
    int row_cnt = 0, col_cnt = 0, val_cnt = 0;
    for(uint32_t i = 0; i < csr1->nr_rows + 1; ++i){
        if(csr1->row_ptr[i] != csr2->row_ptr[i]) row_cnt++;
    }
    for(uint32_t i = 0; i < csr1->nnz; ++i){
        if(csr1->col_idx[i] != csr2->col_idx[i]) col_cnt++;
        if(fabs(csr1->values[i] - csr2->values[i]) > 0.0001) val_cnt++;
    }
    printf("row: %d, col: %d, val: %d\n", row_cnt, col_cnt, val_cnt);

    if(csr1->height != csr2->height){
        printf("height: %d %d\n", csr1->height, csr2->height);
    }
    if(csr1->width != csr2->width){
        printf("width: %d %d\n", csr1->width, csr2->width);
    }
    if(csr1->nr_horiz != csr2->nr_horiz){
        printf("nr_horiz: %d %d\n", csr1->nr_horiz, csr2->nr_horiz);
    }
    if(csr1->nr_vert != csr2->nr_vert){
        printf("nr_vert: %d %d\n", csr1->nr_vert, csr2->nr_vert);
    }
    if(csr1->nr_part != csr2->nr_part){
        printf("nr_part: %d %d\n", csr1->nr_part, csr2->nr_part);
    }
    int nnz_per_part_cnt = 0;
    for(uint32_t i = 0; i < csr1->nr_part; ++i){
        if(csr1->nnz_per_part[i] != csr2->nnz_per_part[i]) nnz_per_part_cnt++;
    }
    printf("nnz_per_part: %d\n", nnz_per_part_cnt);

    return 1;
}

int main(int argc, char** argv){

    // if use BA data
    char path[50] = "../dataset/";

    char type_result[50] = "_8.csv";
    char filename[256];
    char result_filename[256];
    strcpy(result_filename, argv[3]);
    strcat(path, argv[3]);
    strcat(result_filename, type_result);
    strcpy(filename, path);

    struct dpu_set_t set, dpu;
    uint32_t nr_dpus = atoi(argv[1]);
    uint32_t vert_param = atoi(argv[2]);

    Timer timer;

    struct CSR_2D_format* csr_m;

    uint32_t nr_horiz_part = vert_param;
    uint32_t nr_vert_part = vert_param;

    csr_m = load_csr(filename);

    init_timer(&timer, 15);
    start_timer(&timer, 15);
    partition_CSR_ES(csr_m, nr_horiz_part, nr_vert_part);
    end_timer(&timer, 15);

    printf("Partition time: %.6f s\n", timer.time[15]);

    if(csr_m == NULL){
        printf("data exceed MRAM\n");
        exit(1);
    }
    uint32_t scale_factor;
    scale_factor = csr_m->nr_part / nr_dpus;

    printf("File name: %s\n", filename);

    uint32_t rows_pad = csr_m->nr_horiz * csr_m->height;
    uint32_t cols_pad = csr_m->nr_vert * csr_m->width;
    uint32_t width_pad = csr_m->width;
    if(rows_pad % 2 != 0) rows_pad += 1;
    if(cols_pad % 2 != 0) cols_pad += 1;
    if(width_pad % 2 != 0) width_pad += 1;

    // Allocate input vector
    val_dt* input_vec = (val_dt*)malloc(cols_pad * sizeof(val_dt));
    val_dt* output_vec;

    // Initialize input vector
    for(uint32_t i = 0; i < cols_pad; ++i){
        input_vec[i] = (val_dt)(i % 4 + 1);
    }

    uint32_t max_nr_rows_dpu = 0, max_nnz = 0;

    dpu_info = (struct dpu_info_t*)malloc(nr_dpus * sizeof(struct dpu_info_t));
    dpu_args_t* input_args = (dpu_args_t*)malloc(nr_dpus * sizeof(dpu_args_t));

    uint32_t acc = 0;

    for(int i = 0; i < 10; ++i){
        init_timer(&timer, i);
    }

    // DPU Alloc and Load 
    init_timer(&timer, 12);
    start_timer(&timer, 12);
    DPU_ASSERT(dpu_alloc(nr_dpus, NULL, &set));
    DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));
    end_timer(&timer, 12);

    init_timer(&timer, 11); // for measure all_processing time
    start_timer(&timer, 11);
    
    uint64_t avg_max_nnz = 0; // to calc average max_nnz during full iterations
    for(uint32_t iter = 0; iter < scale_factor; ++iter){
        max_nr_rows_dpu = 0;
        max_nnz = 0;
        uint32_t each_dpu;
        DPU_FOREACH(set, dpu, each_dpu){
            uint32_t p_idx = each_dpu + nr_dpus * iter;

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

            dpu_info[each_dpu].nr_rows = nr_rows;
            dpu_info[each_dpu].start_row = start_row;
            dpu_info[each_dpu].start_idx = start_idx;
            dpu_info[each_dpu].nnz = nnz;
            dpu_info[each_dpu].start_row_ptr = ((uint64_t)start_p_row * (uint64_t)csr_m->nr_vert + (uint64_t)start_p_col) * (uint64_t)(csr_m->height + 1);
            input_args[each_dpu].nr_rows = nr_rows;
            input_args[each_dpu].nr_cols = width_pad;
            input_args[each_dpu].nnz = nnz;
        }

        if(max_nr_rows_dpu % 2 != 0)
            max_nr_rows_dpu++;
        if(max_nnz % 2 != 0)
            max_nnz++;

        unsigned long int total_bytes = 0;
        total_bytes = (max_nr_rows_dpu * sizeof(uint32_t)) * 2 + (max_nnz * sizeof(uint32_t)) * 2 + (width_pad * sizeof(uint32_t));
        if(total_bytes > (64 << 20)){
            printf("Data exceeded MRAM\n");
            exit(1);
        }
    
        avg_max_nnz += max_nnz;
        // offset setting for send & retrieve data
        uint32_t out_size = max_nr_rows_dpu * sizeof(uint32_t);
        uint32_t vec_size = width_pad * sizeof(uint32_t);
        uint32_t row_size = max_nr_rows_dpu * sizeof(uint32_t);
        uint32_t col_size = max_nnz * sizeof(uint32_t);
        uint32_t val_size = max_nnz * sizeof(val_dt);

        uint32_t out_base = 0;
        uint32_t vec_base = out_base + out_size;
        uint32_t row_base = vec_base + vec_size;
        uint32_t col_base = row_base + row_size;
        uint32_t val_base = col_base + col_size;

        start_timer(&timer, 0);
        start_timer(&timer, 7);
        DPU_FOREACH(set, dpu, each_dpu){
            input_args[each_dpu].max_nr_rows = max_nr_rows_dpu;
            input_args[each_dpu].max_nnz = max_nnz;
            DPU_ASSERT(dpu_prepare_xfer(dpu, input_args + each_dpu));
        }
        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGS", 0, sizeof(dpu_args_t), DPU_XFER_DEFAULT));
        end_timer(&timer, 7);

        // Send rowptr
        start_timer(&timer, 8);
        DPU_FOREACH(set, dpu, each_dpu){
            DPU_ASSERT(dpu_prepare_xfer(dpu, csr_m->row_ptr + dpu_info[each_dpu].start_row_ptr));
        }
        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, row_base, row_size, DPU_XFER_DEFAULT));
        end_timer(&timer, 8);

        // Send col_idx
        // csr_m->col_idx = (uint32_t*)realloc(csr_m->col_idx, (csr_m->nnz + max_nnz) * sizeof(uint32_t));
        start_timer(&timer, 9);
        DPU_FOREACH(set, dpu, each_dpu){
            DPU_ASSERT(dpu_prepare_xfer(dpu, csr_m->col_idx + dpu_info[each_dpu].start_idx));
        }
        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, col_base, col_size, DPU_XFER_DEFAULT));
        end_timer(&timer, 9);

        // Send values
        // csr_m->values = (val_dt*)realloc(csr_m->values, (csr_m->nnz + max_nnz) * sizeof(uint32_t));
        start_timer(&timer, 10);
        DPU_FOREACH(set, dpu, each_dpu){
            DPU_ASSERT(dpu_prepare_xfer(dpu, csr_m->values + dpu_info[each_dpu].start_idx));
        }
        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, val_base, val_size, DPU_XFER_DEFAULT));
        end_timer(&timer, 10);
        end_timer(&timer, 0);

        // 1. Send input_vector
        start_timer(&timer, 1);
        DPU_FOREACH(set, dpu, each_dpu){
            uint32_t p_idx = each_dpu + nr_dpus * iter;
            uint32_t start_p_col = p_idx % csr_m->nr_vert;
            DPU_ASSERT(dpu_prepare_xfer(dpu, input_vec + start_p_col * csr_m->width));
        }
        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, vec_base, vec_size, DPU_XFER_DEFAULT));
        end_timer(&timer, 1);
        // 2. Launch DPU
        start_timer(&timer, 2);
        // DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));
        dpu_launch(set,DPU_SYNCHRONOUS);
        end_timer(&timer, 2);

        // 3. Get the results
        if(iter == 0) output_vec = (val_dt*)malloc(csr_m->nr_part * max_nr_rows_dpu * sizeof(uint32_t));

        start_timer(&timer, 3);
        DPU_FOREACH(set, dpu, each_dpu){
            uint32_t p_idx = each_dpu + nr_dpus * iter;
            DPU_ASSERT(dpu_prepare_xfer(dpu, output_vec + (p_idx * max_nr_rows_dpu)));
        }
        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, out_base, out_size, DPU_XFER_DEFAULT));
        end_timer(&timer, 3);
    }
    end_timer(&timer, 11);

    DPU_ASSERT(dpu_free(set));   

    val_dt* result_vec = (val_dt*)calloc(csr_m->nr_rows, sizeof(uint32_t));
    uint32_t i, j;
    start_timer(&timer, 4);
#if SIN_VER
    uint32_t check_idx = 0;
    for(i = 0; i < nr_dpus * scale_factor; ++i){
        uint32_t row_p_idx = i / csr_m->nr_vert;
        uint32_t col_p_idx = i % csr_m->nr_vert;
        uint32_t start_row = row_p_idx * csr_m->height;
        
        for(j = 0; j < csr_m->height; ++j){
            uint32_t row = start_row + j;
            if(row >= csr_m->nr_rows) break;
            result_vec[row] += output_vec[i * max_nr_rows_dpu + j];
            check_idx++;
        }
    }
#endif

#if OMP_VER

    // omp ver
#pragma omp parallel for num_threads(64) shared (csr_m, result_vec, max_nr_rows_dpu) private(i,j)
    for(i = 0; i < csr_m->nr_part; ++i){
        uint32_t row_p_idx = i / csr_m->nr_vert;
        uint32_t col_p_idx = i % csr_m->nr_vert;
        uint32_t start_row = row_p_idx * csr_m->height;
        
        for(j = 0; j < csr_m->height; ++j){
            uint32_t row = start_row + j;
            if(row >= csr_m->nr_rows) break;
            #pragma omp atomic
                result_vec[row] += output_vec[i * max_nr_rows_dpu + j];
        }
    }

#endif
    end_timer(&timer, 4);

    val_dt* host_vec = (val_dt*)calloc(rows_pad, sizeof(val_dt));
    init_timer(&timer, 5);
    start_timer(&timer, 5);
    spmv_host(host_vec, csr_m, input_vec);
    end_timer(&timer, 5);
    bool result = true;
    val_dt sum = 0;
    for(uint32_t i = 0; i < csr_m->nr_rows; ++i){
        // if(host_vec[i] != result_vec[i]) result = false;
        sum += result_vec[i];
        if(fabs(host_vec[i] - result_vec[i]) > 1e-6) result = false;
    }

    if(result) printf("Result:\t\t \033[34mCorrect\033[0m\n");
    else printf("Result:\t\t \033[31mIncorrect\033[0m\n");
    free(host_vec);

    // printf("Alloc & Load:\t %lf\n", timer.time[12]);
    // printf("Send Matrix:\t %lf\n", timer.time[0]);
    // printf("(args, rows, cols, values): %lf %lf %lf %lf\n", timer.time[7], timer.time[8], timer.time[9], timer.time[10]);
    // printf("Send input_vec:\t %lf\n", timer.time[1]);
    // printf("DPU Exec:\t %lf\n", timer.time[2]);
    // printf("Retrieve:\t %lf\n", timer.time[3]);
    // printf("Merge results:\t %lf\n", timer.time[4]);
    // printf("Total_time:\t %lf\n", tot_time);
    // printf("Entire_time:\t %lf\n", timer.time[11]);
    // printf("Host(CPU):\t %lf\n", timer.time[5]);
    // printf("total max_nnz: %ld\n", avg_max_nnz);
    printf("-------------------------------\n\n");
    double tot_time = 0;
    for(int i = 0; i < 5; i++){
        tot_time += timer.time[i];
    }
    double send_time = timer.time[0] + timer.time[1];
    // FILE* fp2 = fopen(result_filename, "a");

    char output_filename[50], output_filename_e2e[50];
    if (nr_horiz_part == 256){
        strcpy(output_filename, "./results/ES_256.csv");
        strcpy(output_filename_e2e, "./results/ES_256_e2e.csv");
    }else if (nr_horiz_part == 512){
        strcpy(output_filename, "./results/ES_512.csv");
        strcpy(output_filename_e2e, "./results/ES_512_e2e.csv");
    }

    FILE* fp2 = fopen(output_filename, "a");
    FILE* fp3 = fopen(output_filename_e2e, "a");

    /* timer info
    0: load matrix
    1: load input
    2: DPU execution
    3: retrieve
    4: merge
    12: alloc & load
    15: partition time
    */

    if(fp2 == NULL || fp3 == NULL) exit(1);
    // nr_dpus | load_matrix | load_input | DPU_EXEC | retrieve_time | merge_time | tot_time
    fprintf(fp2, "%d, %f, %f, %f, %f, %f, %f\n", nr_dpus, timer.time[0], timer.time[1], timer.time[2], timer.time[3], timer.time[4], tot_time);

    double end_time = timer.time[11] + timer.time[12] + timer.time[15];
    // nr_dpus | partition_time | load_matrix | load_input | DPU_EXEC | retrieve_time | merge_time | E2E time
    fprintf(fp3, "%d, %f, %f, %f, %f, %f, %f, %f\n", nr_dpus, timer.time[15], timer.time[0], timer.time[1], timer.time[2], timer.time[3], timer.time[4], end_time);

    fclose(fp2);
    fclose(fp3);
    
    // Deallocation
    free(input_vec);
    free(output_vec);
    free(result_vec);
    free_CSR_2D(csr_m);
    free(dpu_info);
    free(input_args);

    return 0;
}