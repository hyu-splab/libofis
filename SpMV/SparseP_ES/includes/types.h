#ifndef _TYPES_H_
#define _TYPES_H_

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

struct elem_t {
    uint32_t row_idx;
    uint32_t col_idx;
    val_dt values;
};

struct CSR_2D_format{
    uint32_t nr_rows;
    uint32_t nr_cols;
    uint32_t nnz;
    uint32_t* row_ptr;
    uint32_t* col_idx;
    val_dt* values;
    uint32_t nr_horiz;
    uint32_t nr_vert;
    uint32_t nr_part;
    uint32_t height;
    uint32_t width;
    uint32_t* nnz_per_part;
};

void free_CSR_2D(struct CSR_2D_format* CSR_A){
    free(CSR_A->row_ptr);
    free(CSR_A->col_idx);
    free(CSR_A->values);
    free(CSR_A->nnz_per_part);
    free(CSR_A);
}

struct CSR_2D_format* load_csr(const char* filename){
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open file %s for reading.\n", filename);
        return NULL;
    }

    struct CSR_2D_format *A = (struct CSR_2D_format*)malloc(sizeof(struct CSR_2D_format));

    // Read basic information
    fread(&A->nr_rows, sizeof(uint32_t), 1, fp);
    fread(&A->nr_cols, sizeof(uint32_t), 1, fp);
    fread(&A->nnz, sizeof(uint32_t), 1, fp);
    fread(&A->nr_horiz, sizeof(uint32_t), 1, fp);
    fread(&A->nr_vert, sizeof(uint32_t), 1, fp);
    fread(&A->nr_part, sizeof(uint32_t), 1, fp);
    fread(&A->height, sizeof(uint32_t), 1, fp);
    fread(&A->width, sizeof(uint32_t), 1, fp);

    // Allocate arrays
    size_t row_ptr_size = (uint64_t)(A->height + 2) * (uint64_t)A->nr_part;
    A->row_ptr = (uint32_t*)calloc(row_ptr_size, sizeof(uint32_t));
    A->col_idx = (uint32_t*)calloc(A->nnz, sizeof(uint32_t));
    A->values = (val_dt*)calloc(A->nnz, sizeof(val_dt));
    A->nnz_per_part = (uint32_t*)calloc(A->nr_part, sizeof(uint32_t));

    // Read arrays
    fread(A->row_ptr, sizeof(uint32_t), row_ptr_size, fp);
    fread(A->col_idx, sizeof(uint32_t), A->nnz, fp);
    fread(A->values, sizeof(val_dt), A->nnz, fp);
    fread(A->nnz_per_part, sizeof(uint32_t), A->nr_part, fp);

    fclose(fp);
    printf("Successfully loaded CSR matrix from %s\n", filename);
    return A;
}

void partition_CSR_ES(struct CSR_2D_format* csr_m, uint32_t nr_horiz, uint32_t nr_vert){
    uint32_t max_nnz = 0;

    // data backup
    uint32_t* old_row_ptr = csr_m->row_ptr;
    uint32_t* old_col_idx = csr_m->col_idx;
    val_dt* old_values = csr_m->values;
    free(csr_m->nnz_per_part);

    csr_m->nr_part = nr_horiz * nr_vert;
    csr_m->nr_horiz = nr_horiz;
    csr_m->nr_vert = nr_vert;

    csr_m->height = csr_m->nr_rows / nr_horiz;
    if(csr_m->nr_rows % nr_horiz != 0)
        csr_m->height++;
    csr_m->width = csr_m->nr_cols / nr_vert;
    if(csr_m->nr_cols % nr_vert != 0)
        csr_m->width++;

    size_t row_ptr_size = (uint64_t)(csr_m->height + 2) * (uint64_t)csr_m->nr_part;
    csr_m->row_ptr = (uint32_t*)calloc(row_ptr_size, sizeof(uint32_t));
    csr_m->col_idx = (uint32_t*)calloc(csr_m->nnz, sizeof(uint32_t));
    csr_m->values = (val_dt*)calloc(csr_m->nnz, sizeof(val_dt));
    csr_m->nnz_per_part = (uint32_t*)calloc((nr_horiz * nr_vert), sizeof(uint32_t));

    uint32_t p_row_idx, p_col_idx, p_idx;
    uint32_t local_row, local_col;

    #pragma omp parallel for num_threads(64) private(p_row_idx, p_col_idx, p_idx, local_row)
    for(uint32_t row = 0; row < csr_m->nr_rows; ++row){
        p_row_idx = row / csr_m->height;
        local_row = row - p_row_idx * csr_m->height;
        for(uint32_t idx = old_row_ptr[row]; idx < old_row_ptr[row + 1]; ++idx){
            uint32_t col = old_col_idx[idx];

            p_col_idx = col / csr_m->width;
            p_idx = p_row_idx * nr_vert + p_col_idx;

            #pragma omp atomic
            csr_m->nnz_per_part[p_idx]++;

            uint64_t cal_idx = (uint64_t)p_idx * (csr_m->height + 1) + (uint64_t)local_row + 1;
            
            #pragma omp atomic
            csr_m->row_ptr[cal_idx]++; // +1 because row_ptr start from 0
        }
    }

    #pragma omp parallel for num_threads(64)
    for(uint32_t i = 0; i < csr_m->nr_part; ++i){
        uint32_t acc = 0;
        for(uint32_t j = 0; j <= csr_m->height; ++j){
            uint64_t cal_idx = (uint64_t)i * (csr_m->height + 1) + (uint64_t)j;
            acc += csr_m->row_ptr[cal_idx];
            csr_m->row_ptr[cal_idx] = acc;
        }
    }

    uint32_t* nnz_idx = (uint32_t*)calloc(csr_m->nr_part, sizeof(uint32_t));
    uint32_t* local_nnz = (uint32_t*)calloc(csr_m->nr_part, sizeof(uint32_t));
    uint32_t acc = 0;

    for(uint32_t i = 0; i < csr_m->nr_part; ++i){
        nnz_idx[i] = acc;
        acc += csr_m->nnz_per_part[i];
        
        if(acc > csr_m->nnz) {
            printf("ERROR: After partition %u, acc=%u > nnz=%u\n", i, acc, csr_m->nnz);
        }

        uint32_t nnz_pad = csr_m->nnz_per_part[i];
        if(nnz_pad % 2 != 0) nnz_pad += 1;
        if(nnz_pad > max_nnz) max_nnz = nnz_pad;
    }

    #pragma omp parallel num_threads(64)
    {
        uint32_t* row_part_count = (uint32_t*)calloc(csr_m->nr_part, sizeof(uint32_t));
        uint32_t* used_parts = (uint32_t*)malloc(csr_m->nr_part * sizeof(uint32_t));
        
        #pragma omp for private(p_row_idx, p_col_idx, p_idx, local_row, local_col)
        for(uint32_t row = 0; row < csr_m->nr_rows; ++row){
            uint32_t num_used = 0;
            
            p_row_idx = row / csr_m->height;
            local_row = row - p_row_idx * csr_m->height;
            
            for(uint32_t idx = old_row_ptr[row]; idx < old_row_ptr[row + 1]; ++idx){
                uint32_t col = old_col_idx[idx];
                p_col_idx = col / csr_m->width;
                p_idx = p_row_idx * nr_vert + p_col_idx;
                local_col = col - p_col_idx * csr_m->width;
                
                if(row_part_count[p_idx] == 0) {
                    used_parts[num_used++] = p_idx;
                }
                
                uint64_t row_start_in_part = csr_m->row_ptr[(uint64_t)p_idx * (csr_m->height + 1) + local_row];
                uint32_t write_pos = nnz_idx[p_idx] + row_start_in_part + row_part_count[p_idx];
                
                csr_m->col_idx[write_pos] = local_col;
                csr_m->values[write_pos] = old_values[idx];
                row_part_count[p_idx]++;
            }
            
            for(uint32_t i = 0; i < num_used; ++i) {
                row_part_count[used_parts[i]] = 0;
            }
        }
        
        free(row_part_count);
        free(used_parts);
    }

    free(nnz_idx);
    free(local_nnz);
    free(old_row_ptr);
    free(old_col_idx);
    free(old_values);
}

#endif

