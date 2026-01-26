#ifndef _MATRIX_H_
#define _MATRIX_H_

#include <assert.h>
#include <stdio.h>

#include "common.h"
#include "utils.h"

#include <stdbool.h>
#include <math.h>   
#include <string.h> 

#define VAL_DT_TOLERANCE 1e-6

/**
 * @brief RBDCSR matrix format 
 * 2D-partitioned matrix with equally-wide vertical tiles and CSR on each vertical tile
 */
struct RBDCSRMatrix {
    uint32_t nrows; 
    uint32_t ncols; 
    uint32_t nnz;   
    uint32_t npartitions;   
    uint32_t horz_partitions;   
    uint32_t vert_partitions; 
    uint32_t tile_width; 
    uint32_t *nnzs_per_vert_partition;  
    uint32_t *drowptr; 
    uint32_t *dcolind;  
    val_dt *dval;       
};

/**
 * @brief Saves an RBDCSRMatrix to a binary file.
 * @param filename The name of the file to save to.
 * @param A The matrix to save.
 * @return 0 on success, -1 on error.
 */
int save_rbdcsr_matrix(const char* filename, struct RBDCSRMatrix *A) {
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open file %s for writing.\n", filename);
        return -1;
    }


    fwrite(&A->nrows, sizeof(uint32_t), 1, fp);
    fwrite(&A->ncols, sizeof(uint32_t), 1, fp);
    fwrite(&A->nnz, sizeof(uint32_t), 1, fp); 
    fwrite(&A->npartitions, sizeof(uint32_t), 1, fp);
    fwrite(&A->horz_partitions, sizeof(uint32_t), 1, fp);
    fwrite(&A->vert_partitions, sizeof(uint32_t), 1, fp);
    fwrite(&A->tile_width, sizeof(uint32_t), 1, fp);

    fwrite(A->nnzs_per_vert_partition, sizeof(uint32_t), A->vert_partitions, fp);
    fwrite(A->drowptr, sizeof(uint32_t), (uint32_t)(A->nrows + 1) * A->vert_partitions, fp);
    fwrite(A->dcolind, sizeof(uint32_t), A->nnz, fp);
    fwrite(A->dval, sizeof(val_dt), A->nnz, fp);

    fclose(fp);
    printf("Successfully saved matrix to %s\n", filename);
    return 0;
}

/**
 * @brief Loads an RBDCSRMatrix from a binary file.
 * @param filename The name of the file to load from.
 * @return A pointer to the loaded matrix, or NULL on error.
 */
struct RBDCSRMatrix* load_rbdcsr_matrix(const char* filename) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open file %s for reading.\n", filename);
        return NULL;
    }

    struct RBDCSRMatrix *A = (struct RBDCSRMatrix *) malloc(sizeof(struct RBDCSRMatrix));

    fread(&A->nrows, sizeof(uint32_t), 1, fp);
    fread(&A->ncols, sizeof(uint32_t), 1, fp);
    fread(&A->nnz, sizeof(uint32_t), 1, fp);
    fread(&A->npartitions, sizeof(uint32_t), 1, fp);
    fread(&A->horz_partitions, sizeof(uint32_t), 1, fp);
    fread(&A->vert_partitions, sizeof(uint32_t), 1, fp);
    fread(&A->tile_width, sizeof(uint32_t), 1, fp);

    A->nnzs_per_vert_partition = (uint32_t *) malloc(A->vert_partitions * sizeof(uint32_t));
    A->drowptr = (uint32_t *) malloc((uint32_t)(A->nrows + 1) * A->vert_partitions * sizeof(uint32_t));
    A->dcolind = (uint32_t *) malloc(A->nnz * sizeof(uint32_t));
    A->dval = (val_dt *) malloc(A->nnz * sizeof(val_dt));

    fread(A->nnzs_per_vert_partition, sizeof(uint32_t), A->vert_partitions, fp);
    fread(A->drowptr, sizeof(uint32_t), (uint32_t)(A->nrows + 1) * A->vert_partitions, fp);
    fread(A->dcolind, sizeof(uint32_t), A->nnz, fp);
    fread(A->dval, sizeof(val_dt), A->nnz, fp);

    fclose(fp);
    printf("Successfully loaded matrix from %s\n", filename);
    return A;
}

bool compare_rbdcsr_matrices(struct RBDCSRMatrix *A, struct RBDCSRMatrix *B) {
    // 1. 기본 정보(스칼라 값) 비교
    if (A->nrows != B->nrows || A->ncols != B->ncols || A->nnz != B->nnz ||
        A->vert_partitions != B->vert_partitions || A->horz_partitions != B->horz_partitions ||
        A->tile_width != B->tile_width) {
        printf("Error: Scalar matrix properties do not match.\n");
        return false;
    }

    if (memcmp(A->nnzs_per_vert_partition, B->nnzs_per_vert_partition, A->vert_partitions * sizeof(uint32_t)) != 0) {
        printf("Error: nnzs_per_vert_partition arrays do not match.\n");
        return false;
    }

    size_t drowptr_size = (size_t)(A->nrows + 1) * A->vert_partitions * sizeof(uint32_t);
    if (memcmp(A->drowptr, B->drowptr, drowptr_size) != 0) {
        printf("Error: drowptr arrays do not match.\n");
        return false;
    }

    if (memcmp(A->dcolind, B->dcolind, (size_t)A->nnz * sizeof(uint32_t)) != 0) {
        printf("Error: dcolind arrays do not match.\n");
        return false;
    }

#if FP32 || FP64
    for (uint64_t i = 0; i < A->nnz; i++) {
        if (fabs(A->dval[i] - B->dval[i]) > VAL_DT_TOLERANCE) {
            printf("Error: dval arrays do not match at index %llu.\n", i);
            return false;
        }
    }
#else
    if (memcmp(A->dval, B->dval, (size_t)A->nnz * sizeof(val_dt)) != 0) {
        printf("Error: dval arrays do not match.\n");
        return false;
    }
#endif

    return true;
}

void partition_CSR_EW(struct RBDCSRMatrix* csr_m, int nr_horiz, int nr_vert){
    uint32_t* old_drowptr = csr_m->drowptr;
    uint32_t* old_dcolind = csr_m->dcolind;
    val_dt* old_dval = csr_m->dval;
    free(csr_m->nnzs_per_vert_partition);

    csr_m->npartitions = nr_vert;
    csr_m->horz_partitions = nr_horiz;
    csr_m->vert_partitions = nr_vert;
    csr_m->tile_width = csr_m->ncols / nr_vert;
    if(csr_m->ncols % nr_vert != 0)
        csr_m->tile_width++;

    csr_m->nnzs_per_vert_partition = (uint32_t*)calloc(csr_m->npartitions, sizeof(uint32_t));
    csr_m->drowptr = (uint32_t*)calloc((csr_m->nrows + 2) * csr_m->npartitions, sizeof(uint32_t));
    csr_m->dcolind = (uint32_t*)malloc((csr_m->nnz + 2) * sizeof(uint32_t));
    csr_m->dval = (val_dt*)calloc((csr_m->nnz + 8), sizeof(val_dt));

    int num_threads = 64;
    
    // Thread-local 버퍼 (파티션 수가 적을 때 필수!)
    uint32_t** thread_nnzs = (uint32_t**)calloc(num_threads, sizeof(uint32_t*));
    uint32_t** thread_drowptr = (uint32_t**)calloc(num_threads, sizeof(uint32_t*));
    
    for(int t = 0; t < num_threads; t++){
        thread_nnzs[t] = (uint32_t*)calloc(csr_m->npartitions, sizeof(uint32_t));
        thread_drowptr[t] = (uint32_t*)calloc((csr_m->nrows + 1) * csr_m->npartitions, sizeof(uint32_t));
    }

    // 첫 번째 패스: 각 스레드가 로컬로 카운트
    #pragma omp parallel num_threads(64)
    {
        int tid = omp_get_thread_num();
        uint32_t* my_nnzs = thread_nnzs[tid];
        uint32_t* my_drowptr = thread_drowptr[tid];
        
        #pragma omp for schedule(static)
        for(uint32_t row = 0; row < csr_m->nrows; ++row){
            for(uint32_t idx = old_drowptr[row]; idx < old_drowptr[row + 1]; ++idx){
                uint32_t col = old_dcolind[idx];
                uint32_t p_col = col / csr_m->tile_width;
                
                my_nnzs[p_col]++;
                my_drowptr[p_col * (csr_m->nrows + 1) + row]++;
            }
        }
    }
    
    // Thread-local 결과 합치기
    for(int t = 0; t < num_threads; t++){
        for(uint32_t p = 0; p < csr_m->npartitions; p++){
            csr_m->nnzs_per_vert_partition[p] += thread_nnzs[t][p];
        }
        for(uint32_t i = 0; i < (csr_m->nrows + 1) * csr_m->npartitions; i++){
            csr_m->drowptr[i] += thread_drowptr[t][i];
        }
        free(thread_nnzs[t]);
        free(thread_drowptr[t]);
    }
    free(thread_nnzs);
    free(thread_drowptr);

    // Prefix sum
    #pragma omp parallel for num_threads(64)
    for(uint32_t p = 0; p < csr_m->npartitions; ++p){
        uint32_t sumBeforeNextRow = 0;
        for(uint32_t rowIndx = 0; rowIndx < csr_m->nrows; ++rowIndx){
            uint32_t sumBeforeRow = sumBeforeNextRow;
            sumBeforeNextRow += csr_m->drowptr[p * (csr_m->nrows + 1) + rowIndx];
            csr_m->drowptr[p * (csr_m->nrows + 1) + rowIndx] = sumBeforeRow;
        }
        csr_m->drowptr[p * (csr_m->nrows + 1) + csr_m->nrows] = sumBeforeNextRow;
    }

    // Global offset 계산
    uint64_t *global_nnzs = (uint64_t*)calloc(csr_m->npartitions, sizeof(uint64_t));
    uint64_t total_nnzs = 0;
    for(uint32_t p = 0; p < csr_m->npartitions; ++p){
        global_nnzs[p] = total_nnzs;
        total_nnzs += csr_m->nnzs_per_vert_partition[p];
    }

    // 두 번째 패스: Thread-local write position 사용
    uint32_t*** thread_write_pos = (uint32_t***)calloc(num_threads, sizeof(uint32_t**));
    for(int t = 0; t < num_threads; t++){
        thread_write_pos[t] = (uint32_t**)calloc(csr_m->npartitions, sizeof(uint32_t*));
        for(uint32_t p = 0; p < csr_m->npartitions; p++){
            thread_write_pos[t][p] = (uint32_t*)calloc(csr_m->nrows, sizeof(uint32_t));
            // 각 행의 시작 위치로 초기화
            for(uint32_t row = 0; row < csr_m->nrows; row++){
                thread_write_pos[t][p][row] = csr_m->drowptr[p * (csr_m->nrows + 1) + row];
            }
        }
    }

    #pragma omp parallel num_threads(64)
    {
        int tid = omp_get_thread_num();
        
        #pragma omp for schedule(static)
        for(uint32_t row = 0; row < csr_m->nrows; ++row){
            for(uint32_t idx = old_drowptr[row]; idx < old_drowptr[row + 1]; ++idx){
                uint32_t col = old_dcolind[idx];
                uint32_t p_col = col / csr_m->tile_width;
                uint32_t local_col = col - p_col * csr_m->tile_width;

                uint32_t write_pos = thread_write_pos[tid][p_col][row];
                thread_write_pos[tid][p_col][row]++;
                
                uint64_t global_idx = global_nnzs[p_col] + write_pos;
                csr_m->dcolind[global_idx] = local_col;
                csr_m->dval[global_idx] = old_dval[idx];
            }
        }
    }

    // Cleanup
    for(int t = 0; t < num_threads; t++){
        for(uint32_t p = 0; p < csr_m->npartitions; p++){
            free(thread_write_pos[t][p]);
        }
        free(thread_write_pos[t]);
    }
    free(thread_write_pos);
    free(global_nnzs);
    free(old_drowptr);
    free(old_dcolind);
    free(old_dval);
}

/**
 * @brief deallocate matrix in RBDCSR format 
 * @param matrix in RBDCSR format
 */ 
void freeRBDCSRMatrix(struct RBDCSRMatrix *rbdcsrMtx) {
    free(rbdcsrMtx->nnzs_per_vert_partition);
    free(rbdcsrMtx->drowptr);
    free(rbdcsrMtx->dcolind);
    free(rbdcsrMtx->dval);
    free(rbdcsrMtx);
}

struct RBDCSRMatrix* load_csr(const char* filename){
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open file %s for reading.\n", filename);
        return NULL;
    }

    uint32_t nr_rows, nr_cols, nnz;
    uint32_t nr_horiz, nr_vert, nr_part;
    uint32_t height, width;

    // Read basic information
    fread(&nr_rows, sizeof(uint32_t), 1, fp);
    fread(&nr_cols, sizeof(uint32_t), 1, fp);
    fread(&nnz, sizeof(uint32_t), 1, fp);
    fread(&nr_horiz, sizeof(uint32_t), 1, fp);
    fread(&nr_vert, sizeof(uint32_t), 1, fp);
    fread(&nr_part, sizeof(uint32_t), 1, fp);
    fread(&height, sizeof(uint32_t), 1, fp);
    fread(&width, sizeof(uint32_t), 1, fp);

    // Check if it's basic CSR (no partition)
    if(nr_part != 1){
        fprintf(stderr, "Warning: File has nr_part=%u (expected 1 for basic CSR)\n", nr_part);
    }

    // Allocate RBDCSRMatrix
    struct RBDCSRMatrix *A = (struct RBDCSRMatrix*)malloc(sizeof(struct RBDCSRMatrix));

    A->nrows = nr_rows;
    A->ncols = nr_cols;
    A->nnz = nnz;
    A->npartitions = 1;
    A->horz_partitions = 1;
    A->vert_partitions = 1;
    A->tile_width = nr_cols;

    // Allocate arrays
    size_t row_ptr_size = (uint64_t)(height + 2) * (uint64_t)nr_part;
    uint32_t* temp_row_ptr = (uint32_t*)calloc(row_ptr_size, sizeof(uint32_t));

    A->drowptr = (uint32_t*)calloc(nr_rows + 2, sizeof(uint32_t));
    A->dcolind = (uint32_t*)calloc(nnz + 2, sizeof(uint32_t));
    A->dval = (val_dt*)calloc(nnz + 8, sizeof(val_dt));
    A->nnzs_per_vert_partition = (uint32_t*)calloc(1, sizeof(uint32_t));

    // Read arrays from file
    fread(temp_row_ptr, sizeof(uint32_t), row_ptr_size, fp);
    fread(A->dcolind, sizeof(uint32_t), nnz, fp);
    fread(A->dval, sizeof(val_dt), nnz, fp);
    fread(A->nnzs_per_vert_partition, sizeof(uint32_t), nr_part, fp);

    // Copy to drowptr (should be same for basic CSR with nr_part=1)
    for(uint32_t i = 0; i <= nr_rows; ++i){
        A->drowptr[i] = temp_row_ptr[i];
    }
    free(temp_row_ptr);

    fclose(fp);
    printf("Successfully loaded CSR matrix from %s and converted to RBDCSRMatrix\n", filename);
    printf("Matrix: rows=%u, cols=%u, nnz=%u\n", A->nrows, A->ncols, A->nnz);
    return A;
}


#endif
