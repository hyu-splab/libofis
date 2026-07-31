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

    // Type change
    val_dt values;
};

struct COO_format{
    uint32_t nr_rows;
    uint32_t nr_cols;
    uint32_t nnz;
    uint32_t* nnz_of_rows;
    struct elem_t* elems;
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

/**
 * @brief Saves an CSRMatrix to a binary file.
 * @param filename The name of the file to save to.
 * @param A The matrix to save.
 * @return 0 on success, -1 on error.
 */
int save_dcsr_matrix(const char* filename, struct CSR_2D_format *A){
    FILE *fp = fopen(filename, "wb");
    if(fp == NULL){
        fprintf(stderr, "Error: Could not open file %s for writing.\n", filename);
        return -1;
    }

    // 1. Write basic information of DCSR to file
    fwrite(&A->nr_rows, sizeof(uint32_t), 1, fp); 
    fwrite(&A->nr_cols, sizeof(uint32_t), 1, fp);
    fwrite(&A->nnz, sizeof(uint32_t), 1, fp);    
    fwrite(&A->nr_horiz, sizeof(uint32_t), 1, fp); 
    fwrite(&A->nr_vert, sizeof(uint32_t), 1, fp);
    fwrite(&A->nr_part, sizeof(uint32_t), 1, fp);     
    fwrite(&A->height, sizeof(uint32_t), 1, fp);
    fwrite(&A->width, sizeof(uint32_t), 1, fp);  

    size_t row_ptr_size = (uint64_t)(A->height + 2) * (uint64_t)A->nr_part;
    // 2. Write array data of DCSR to file
    fwrite(A->row_ptr, sizeof(uint32_t), row_ptr_size, fp);
    fwrite(A->col_idx, sizeof(uint32_t), A->nnz, fp);
    fwrite(A->values, sizeof(val_dt), A->nnz, fp);
    fwrite(A->nnz_per_part, sizeof(uint32_t), A->nr_part, fp);

    fclose(fp);
    printf("Successfully saved matrix to %s\n", filename);
    return 0;
}
/**
 * @brief Loads an DCSR from a binary file.
 * @param filename The name of the file to load from.
 * @return A pointer to the loaded matrix, or NULL on error.
 */
struct CSR_2D_format* load_dcsr_matrix(const char* filename){
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open file %s for reading.\n", filename);
        return NULL;
    }

    struct CSR_2D_format *A = (struct CSR_2D_format*)malloc(sizeof(struct CSR_2D_format));

    // 1. Read basic information of DCSR from file
    fread(&A->nr_rows, sizeof(uint32_t), 1, fp); 
    fread(&A->nr_cols, sizeof(uint32_t), 1, fp);
    fread(&A->nnz, sizeof(uint32_t), 1, fp);    
    fread(&A->nr_horiz, sizeof(uint32_t), 1, fp); 
    fread(&A->nr_vert, sizeof(uint32_t), 1, fp);
    fread(&A->nr_part, sizeof(uint32_t), 1, fp);     
    fread(&A->height, sizeof(uint32_t), 1, fp);
    fread(&A->width, sizeof(uint32_t), 1, fp);  

    // Read array data of DCSR from file
    size_t row_ptr_size = (uint64_t)(A->height + 2) * (uint64_t)A->nr_part;
    A->row_ptr = (uint32_t*)calloc(row_ptr_size, sizeof(uint32_t));
    A->col_idx = (uint32_t*)calloc(A->nnz, sizeof(uint32_t));
    A->values = (val_dt*)calloc(A->nnz, sizeof(val_dt));
    A->nnz_per_part = (uint32_t*)calloc(A->nr_part, sizeof(uint32_t));

    fread(A->row_ptr, sizeof(uint32_t), row_ptr_size, fp);
    fread(A->col_idx, sizeof(uint32_t), A->nnz, fp);
    fread(A->values, sizeof(val_dt), A->nnz, fp);
    fread(A->nnz_per_part, sizeof(uint32_t), A->nr_part, fp);

    fclose(fp);
    printf("Successfully loaded matrix from %s\n", filename);
    return A;
}


void free_COO(struct COO_format* coo_mat){
    free(coo_mat->nnz_of_rows);
    free(coo_mat->elems);
    free(coo_mat);
}

void free_CSR_2D(struct CSR_2D_format* csr_mat){
    free(csr_mat->row_ptr);
    free(csr_mat->col_idx);
    free(csr_mat->values);
    free(csr_mat->nnz_per_part);
    free(csr_mat);
}

int comparator(const void* a, const void* b){
    if(((struct elem_t*)a)->row_idx < ((struct elem_t*)b)->row_idx)
        return -1;
    else if(((struct elem_t*)a)->row_idx > ((struct elem_t*)b)->row_idx)
        return 1;
    else    
        return ((struct elem_t*)a)->col_idx - ((struct elem_t*)b)->col_idx;
}

uint32_t sort_COO(struct COO_format* coo_mat){
    // sorting(ascending) elements of COO matrix
    qsort(coo_mat->elems, coo_mat->nnz, sizeof(struct elem_t), comparator);

    int curr_row = coo_mat->elems[0].row_idx;
    int count = 0;
    int acc = 0;

    // counting elements in each row
    for(int i = 0; i < coo_mat->nnz; ++i){
        if(coo_mat->elems[i].row_idx == curr_row)
            count++;
        else{
            acc += count;
            coo_mat->nnz_of_rows[curr_row] = count;
            curr_row = coo_mat->elems[i].row_idx;
            count = 1;
        }
    }

    acc += count;
    coo_mat->nnz_of_rows[curr_row] = count;

    return (acc = coo_mat->nnz);
}

struct COO_format* get_COO_matrix_rev(char* filename){
    struct COO_format* return_coo;
    return_coo = (struct COO_format*)malloc(sizeof(struct COO_format));

    FILE* fp = fopen(filename, "r");
    if(fp == NULL){
        printf("file open err\n");
        exit(1);
    }

    char line[1000];

    int info_flag = 1;
    uint32_t idx = 0;

    while(fgets(line, sizeof(line), fp) != NULL){

        if(line[0] == '%'){
            continue;
        }
        
        if(info_flag){
            uint32_t nr_cols, nr_rows, nnz;
            if(sscanf(line, "%d %d %d", &nr_cols, &nr_rows, &nnz) != 3){
                continue;
            }
            return_coo->nr_cols = nr_cols;
            return_coo->nr_rows = nr_rows;
            return_coo->nnz = nnz;

            if(return_coo->nr_rows % 2 != 0)
                return_coo->nr_rows += 1;
            if(return_coo->nr_cols % 2 != 0)
                return_coo->nr_cols += 1;

            return_coo->nnz_of_rows = (uint32_t*)calloc(return_coo->nr_rows, sizeof(uint32_t));
            return_coo->elems = (struct elem_t*)calloc(return_coo->nnz, sizeof(struct elem_t));

            info_flag = 0;
        }else{
            uint32_t col_idx, row_idx;
            if(sscanf(line, "%d %d", &col_idx, &row_idx) != 2){
                continue;
            }

            // save values with -1 (to start from 0)
            return_coo->elems[idx].row_idx = row_idx - 1;
            return_coo->elems[idx].col_idx = col_idx - 1;

            // Type change
            // return_coo->elems[idx].values = (uint32_t)(rand() % 4 + 1);
            // return_coo->elems[idx].values = (float)rand() / RAND_MAX * (max - min);
            return_coo->elems[idx].values = (val_dt)(idx % 4 + 1);
            idx++;
        }
    }

    fclose(fp);
    return return_coo;
}

struct CSR_2D_format* COO_to_CSR_2D_scale(struct COO_format* coo_mat, uint32_t nr_horiz, uint32_t nr_vert){
    struct CSR_2D_format* return_csr;
    return_csr = (struct CSR_2D_format*)malloc(sizeof(struct CSR_2D_format));

    return_csr->nr_rows = coo_mat->nr_rows;
    return_csr->nr_cols = coo_mat->nr_cols;
    return_csr->nnz = coo_mat->nnz;

    return_csr->nr_part = nr_horiz * nr_vert;
    return_csr->nr_horiz = nr_horiz;
    return_csr->nr_vert = nr_vert;

    return_csr->height = return_csr->nr_rows / nr_horiz;
    if(return_csr->nr_rows % nr_horiz != 0)
        return_csr->height++;
    return_csr->width = return_csr->nr_cols / nr_vert;
    if(return_csr->nr_cols % nr_vert != 0)
        return_csr->width++;

    return_csr->row_ptr = (uint32_t*)calloc((return_csr->height + 2) * return_csr->nr_part, sizeof(uint32_t));
    return_csr->col_idx = (uint32_t*)calloc(return_csr->nnz, sizeof(uint32_t));

    // Type change
    // return_csr->values = (uint32_t*)calloc(return_csr->nnz, sizeof(uint32_t));
    return_csr->values = (float*)calloc(return_csr->nnz, sizeof(float));

    return_csr->nnz_per_part = (uint32_t*)calloc((nr_horiz * nr_vert), sizeof(uint32_t));

    uint32_t p_row_idx, p_col_idx, p_idx;
    uint32_t local_row, local_col;

    // count each row_part's nnz
    for(uint32_t i = 0; i < coo_mat->nnz; ++i){
        p_row_idx = coo_mat->elems[i].row_idx / return_csr->height;
        p_col_idx = coo_mat->elems[i].col_idx / return_csr->width;
        p_idx = p_row_idx * nr_vert + p_col_idx;
        return_csr->nnz_per_part[p_idx]++;
        local_row = coo_mat->elems[i].row_idx - p_row_idx * return_csr->height;
        return_csr->row_ptr[p_idx * (return_csr->height + 1) + local_row + 1]++;
    }

    // acc all row_part's nnz
    for(uint32_t i = 0; i < return_csr->nr_part; ++i){
        uint32_t acc = 0;
        for(uint32_t j = 0; j <= return_csr->height; ++j){
            acc += return_csr->row_ptr[i * (return_csr->height + 1) + j];
            return_csr->row_ptr[i * (return_csr->height + 1) + j] = acc;
        }
    }

    // copy col_idx & values from COO to CSR_2D
    uint32_t* nnz_idx = (uint32_t*)calloc(return_csr->nr_part, sizeof(uint32_t));
    uint32_t* local_nnz = (uint32_t*)calloc(return_csr->nr_part, sizeof(uint32_t));
    uint32_t acc = 0;
    for(uint32_t i = 0; i < return_csr->nr_part; ++i){
        nnz_idx[i] = acc;
        acc += return_csr->nnz_per_part[i];
    }

    for(uint32_t i = 0; i < coo_mat->nnz; ++i){
        p_row_idx = coo_mat->elems[i].row_idx / return_csr->height;
        p_col_idx = coo_mat->elems[i].col_idx / return_csr->width;
        p_idx = p_row_idx * nr_vert + p_col_idx;
        local_col = coo_mat->elems[i].col_idx - p_col_idx * return_csr->width;

        return_csr->col_idx[nnz_idx[p_idx] + local_nnz[p_idx]] = local_col;
        return_csr->values[nnz_idx[p_idx] + local_nnz[p_idx]] = coo_mat->elems[i].values;
        local_nnz[p_idx]++;
    }

    free(nnz_idx);
    free(local_nnz);

    return return_csr;
}

struct CSR_2D_format* COO_to_CSR_2D(struct COO_format* coo_mat, uint32_t nr_horiz, uint32_t nr_vert, uint32_t* scale_factor){
    uint32_t max_nnz = 0;

    struct CSR_2D_format* return_csr;
    return_csr = (struct CSR_2D_format*)malloc(sizeof(struct CSR_2D_format));

    return_csr->nr_rows = coo_mat->nr_rows;
    return_csr->nr_cols = coo_mat->nr_cols;
    return_csr->nnz = coo_mat->nnz;

    return_csr->nr_part = nr_horiz * nr_vert;
    return_csr->nr_horiz = nr_horiz;
    return_csr->nr_vert = nr_vert;

    return_csr->height = return_csr->nr_rows / nr_horiz;
    if(return_csr->nr_rows % nr_horiz != 0)
        return_csr->height++;
    return_csr->width = return_csr->nr_cols / nr_vert;
    if(return_csr->nr_cols % nr_vert != 0)
        return_csr->width++;

    size_t row_ptr_size = (uint64_t)(return_csr->height + 2) * (uint64_t)return_csr->nr_part;
    return_csr->row_ptr = (uint32_t*)calloc(row_ptr_size, sizeof(uint32_t));
    return_csr->col_idx = (uint32_t*)calloc(return_csr->nnz, sizeof(uint32_t));

    // Type change
    // return_csr->values = (uint32_t*)calloc(return_csr->nnz, sizeof(uint32_t));
    return_csr->values = (val_dt*)calloc(return_csr->nnz, sizeof(float));

    return_csr->nnz_per_part = (uint32_t*)calloc((nr_horiz * nr_vert), sizeof(uint32_t));

    uint32_t p_row_idx, p_col_idx, p_idx;
    uint32_t local_row, local_col;

    // count each row_part's nnz
    for(uint32_t i = 0; i < coo_mat->nnz; ++i){
        p_row_idx = coo_mat->elems[i].row_idx / return_csr->height;
        p_col_idx = coo_mat->elems[i].col_idx / return_csr->width;
        p_idx = p_row_idx * nr_vert + p_col_idx;
        return_csr->nnz_per_part[p_idx]++;
        local_row = coo_mat->elems[i].row_idx - p_row_idx * return_csr->height;
        uint64_t cal_idx = (uint64_t)p_idx * (return_csr->height + 1) + (uint64_t)local_row + 1;
        return_csr->row_ptr[cal_idx]++; // +1 because row_ptr start from 0
    }

    // acc all row_ptr's nnz
    for(uint32_t i = 0; i < return_csr->nr_part; ++i){
        uint32_t acc = 0;
        for(uint32_t j = 0; j <= return_csr->height; ++j){
            uint64_t cal_idx = (uint64_t)i * (return_csr->height + 1) + (uint64_t)j;
            acc += return_csr->row_ptr[cal_idx];
            return_csr->row_ptr[cal_idx] = acc;
        }
    }

    // Copy col_idx & values from COO to CSR_2D
    uint32_t* nnz_idx = (uint32_t*)calloc(return_csr->nr_part, sizeof(uint32_t));
    uint32_t* local_nnz = (uint32_t*)calloc(return_csr->nr_part, sizeof(uint32_t));
    uint32_t acc = 0;
    for(uint32_t i = 0; i < return_csr->nr_part; ++i){
        nnz_idx[i] = acc;
        acc += return_csr->nnz_per_part[i];

        // to test size of partitioned CSR
        uint32_t nnz_pad = return_csr->nnz_per_part[i];
        if(nnz_pad % 2 != 0) nnz_pad += 1;
        if(nnz_pad > max_nnz) max_nnz = nnz_pad;
    }

    // Check if max size of partitioned CSR > 64-MB (MRAM tolerance)
    unsigned long int total_bytes;
    uint32_t max_row = return_csr->height + 1;
    if(max_row % 2 != 0) max_row += 1;
    uint32_t width_pad = return_csr->width;
    if(width_pad % 2 != 0) width_pad += 1;
    total_bytes = (max_row * sizeof(uint32_t)) + (max_nnz * sizeof(uint32_t)) * 2 + (width_pad * sizeof(uint32_t));
    printf("max size: %ld-MB, (%d, %d)\n", total_bytes >> 20, max_row, max_nnz);
    uint32_t scale = total_bytes / (8 << 20) + 1; // control unit of calculation
    *scale_factor = scale;
    for(uint32_t i = 0; i < coo_mat->nnz; ++i){
        p_row_idx = coo_mat->elems[i].row_idx / return_csr->height;
        p_col_idx = coo_mat->elems[i].col_idx / return_csr->width;
        p_idx = p_row_idx * nr_vert + p_col_idx;
        local_col = coo_mat->elems[i].col_idx - p_col_idx * return_csr->width;

        return_csr->col_idx[nnz_idx[p_idx] + local_nnz[p_idx]] = local_col;
        return_csr->values[nnz_idx[p_idx] + local_nnz[p_idx]] = coo_mat->elems[i].values;
        local_nnz[p_idx]++;
    }

    free(nnz_idx);
    free(local_nnz);

    return return_csr;
}

#endif