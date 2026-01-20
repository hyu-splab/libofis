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

void free_COO(struct COO_format* COO_A){
    free(COO_A->nnz_of_rows);
    free(COO_A->elems);
    free(COO_A);
}

void free_CSR_2D(struct CSR_2D_format* CSR_A){
    free(CSR_A->row_ptr);
    free(CSR_A->col_idx);
    free(CSR_A->values);
    free(CSR_A->nnz_per_part);
    free(CSR_A);
}

int comparator(const void* a, const void* b){
    if(((struct elem_t*)a)->row_idx < ((struct elem_t*)b)->row_idx) 
        return -1;
    else if(((struct elem_t*)a)->row_idx > ((struct elem_t*)b)->row_idx) 
        return 1;
    else
        return ((struct elem_t*)a)->col_idx - ((struct elem_t*)b)->col_idx;
}

uint32_t sort_COO(struct COO_format* COO_A){
    qsort(COO_A->elems, COO_A->nnz, sizeof(struct elem_t), comparator);

    int curr_row = COO_A->elems[0].row_idx;
    int count = 0;
    int acc = 0;

    for(int i = 0; i < COO_A->nnz; ++i){
        if(COO_A->elems[i].row_idx == curr_row)
            count++;
        else{
            acc += count;
            COO_A->nnz_of_rows[curr_row] = count;
            curr_row = COO_A->elems[i].row_idx;
            count = 1;
        }
    }
    acc += count;
    COO_A->nnz_of_rows[curr_row] = count;

    return (acc == COO_A->nnz);
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
            return_coo->elems[idx].values = (val_dt)(idx % 4 + 1);
            idx++;
        }
    }

    fclose(fp);
    return return_coo;
}

struct CSR_2D_format* COO_to_CSR_2D(struct COO_format* COO_A, uint32_t nr_horiz, uint32_t nr_vert, uint32_t* scale_factor){

    uint32_t max_nnz = 0;

    struct CSR_2D_format* return_CSR;
    return_CSR = (struct CSR_2D_format*)malloc(sizeof(struct CSR_2D_format));

    return_CSR->nr_rows = COO_A->nr_rows;
    return_CSR->nr_cols = COO_A->nr_cols;
    return_CSR->nnz = COO_A->nnz;

    return_CSR->nr_part = nr_horiz * nr_vert;
    return_CSR->nr_horiz = nr_horiz;
    return_CSR->nr_vert = nr_vert;

    return_CSR->height = return_CSR->nr_rows / nr_horiz;
    if(return_CSR->nr_rows % nr_horiz != 0){
        return_CSR->height++;
    }
    return_CSR->width = return_CSR->nr_cols / nr_vert;
    if(return_CSR->nr_cols % nr_vert != 0){
        return_CSR->width++;
    }

    size_t row_ptr_size = (uint64_t)(return_CSR->height + 2) * (uint64_t)return_CSR->nr_part;
    return_CSR->nnz_per_part = (uint32_t*)calloc((nr_horiz * nr_vert), sizeof(uint32_t));
    return_CSR->row_ptr = (uint32_t*)calloc(row_ptr_size, sizeof(uint32_t));
    return_CSR->col_idx = (uint32_t*)calloc(return_CSR->nnz + 10000000, sizeof(uint32_t));
    return_CSR->values = (val_dt*)calloc(return_CSR->nnz + 10000000, sizeof(val_dt));

    uint32_t p_row_idx, p_col_idx, p_idx;
    uint32_t local_row, local_col;

    // Count each row_part's nnz
    for(uint32_t i = 0; i < COO_A->nnz; ++i){
        p_row_idx = COO_A->elems[i].row_idx / return_CSR->height;
        p_col_idx = COO_A->elems[i].col_idx / return_CSR->width;
        p_idx = p_row_idx * nr_vert + p_col_idx;
        return_CSR->nnz_per_part[p_idx]++;
        local_row = COO_A->elems[i].row_idx - p_row_idx * return_CSR->height;
        uint64_t cal_idx = (uint64_t)p_idx * (return_CSR->height + 1) + (uint64_t)local_row + 1;
        return_CSR->row_ptr[cal_idx]++;
    }

    // Acc all row_part's nnz
    for(uint32_t i = 0; i < return_CSR->nr_part; ++i){
        uint32_t acc = 0;
        for(uint32_t j = 0; j <= return_CSR->height; ++j){
            uint64_t cal_idx = (uint64_t)i * (return_CSR->height + 1) + (uint64_t)j;
            acc += return_CSR->row_ptr[cal_idx];
            return_CSR->row_ptr[cal_idx] = acc;
        }
    }

    // Copy col_idx & values from COO to CSR_2D
    uint32_t* nnz_idx = (uint32_t*)calloc(return_CSR->nr_part, sizeof(uint32_t));
    uint32_t* local_nnz = (uint32_t*)calloc(return_CSR->nr_part, sizeof(uint32_t));
    uint32_t acc = 0;
    for(uint32_t i = 0; i < return_CSR->nr_part; ++i){
        nnz_idx[i] = acc;
        acc += return_CSR->nnz_per_part[i];

        // to test size of partitioned CSR
        uint32_t nnz_pad = return_CSR->nnz_per_part[i];
        if(nnz_pad % 2 != 0) nnz_pad += 1;
        if(nnz_pad > max_nnz) max_nnz = nnz_pad;
    }

    // Check if max size of partitioned CSR > 64MB (MRAM tolerance) 
    unsigned long int total_bytes;
    uint32_t max_row = return_CSR->height + 1;
    if(max_row % 2 != 0) max_row += 1;
    uint32_t width_pad = return_CSR->width;
    if(width_pad % 2 != 0) width_pad += 1;    
    total_bytes = (max_row * sizeof(uint32_t)) + (max_nnz * sizeof(uint32_t)) * 2 + (width_pad * sizeof(uint32_t));
    printf("max size: %ld-MB, (%d, %d)\n", total_bytes >> 20, max_row, max_nnz);
    // if((total_bytes >> 20) >= 64)
    //     return NULL;

    // uint32_t scale = total_bytes / (8 << 20) + 1; // control unit of calculation
    // if(scale != 1){
    //     free_CSR_2D(return_CSR);
    //     printf("before size: %ld MB, (%d, %d)\n", total_bytes >> 20, max_row, max_nnz);
    //     *scale_factor = scale;
    //     return_CSR = COO_to_CSR_2D_scale(COO_A, nr_horiz, nr_vert * scale);
    // }else{
        uint32_t scale = return_CSR->nr_part / 1024;
        *scale_factor = scale;
        for(uint32_t i = 0; i < COO_A->nnz; ++i){
            p_row_idx = COO_A->elems[i].row_idx / return_CSR->height;
            p_col_idx = COO_A->elems[i].col_idx / return_CSR->width;   
            p_idx = p_row_idx * nr_vert + p_col_idx;
            local_col = COO_A->elems[i].col_idx - p_col_idx * return_CSR->width; 

            return_CSR->col_idx[nnz_idx[p_idx] + local_nnz[p_idx]] = local_col;
            return_CSR->values[nnz_idx[p_idx] + local_nnz[p_idx]] = COO_A->elems[i].values;
            local_nnz[p_idx]++;    
        }
    // }

    free(nnz_idx);
    free(local_nnz);
    
    return return_CSR;
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

struct CSR_2D_format* convert_to_CSR(struct COO_format* coo_mat){
    struct CSR_2D_format* csr_m;
    csr_m = (struct CSR_2D_format*)malloc(sizeof(struct CSR_2D_format));

    csr_m->nr_rows = coo_mat->nr_rows;
    csr_m->nr_cols = coo_mat->nr_cols;
    csr_m->nnz = coo_mat->nnz;

    csr_m->nr_horiz = 1;
    csr_m->nr_vert = 1;
    csr_m->nr_part = 1;
    csr_m->height = csr_m->nr_rows;
    csr_m->width = csr_m->nr_cols;

    csr_m->row_ptr = (uint32_t*)calloc((csr_m->nr_rows + 1), sizeof(uint32_t));
    csr_m->col_idx = (uint32_t*)calloc(csr_m->nnz, sizeof(uint32_t));
    csr_m->values = (val_dt*)calloc(csr_m->nnz, sizeof(val_dt));
    csr_m->nnz_per_part = (uint32_t*)calloc(1, sizeof(uint32_t));

    for(uint32_t i = 0; i < coo_mat->nnz; ++i){
        csr_m->row_ptr[coo_mat->elems[i].row_idx + 1]++;
    }

    for(uint32_t i = 0; i < csr_m->nr_rows; ++i){
        csr_m->row_ptr[i + 1] += csr_m->row_ptr[i];
    }

    uint32_t* row_counter = (uint32_t*)calloc(csr_m->nr_rows, sizeof(uint32_t));
    for(uint32_t i = 0; i < coo_mat->nnz; ++i){
        uint32_t row = coo_mat->elems[i].row_idx;
        uint32_t idx = csr_m->row_ptr[row] + row_counter[row];

        csr_m->col_idx[idx] = coo_mat->elems[i].col_idx;
        csr_m->values[idx] = coo_mat->elems[i].values;
        row_counter[row]++;
    }

    free(row_counter);
    return csr_m;
}

#endif

