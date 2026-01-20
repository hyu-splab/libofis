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
 * @brief COO matrix format 
 */
struct COOMatrix {
    uint32_t nrows; 
    uint32_t ncols; 
    uint32_t nnz;   
    uint32_t *rows; 
    struct elem_t *nnzs;   
};

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


/**
 * @brief read matrix from input fileName in COO format 
 * @param filename to read matrix (mtx format)
 */
struct COOMatrix *readCOOMatrix(const char* fileName) {

    
    struct COOMatrix *cooMtx;
    cooMtx = (struct COOMatrix *) malloc(sizeof(struct COOMatrix));
    FILE* fp = fopen(fileName, "r");
    uint32_t rowindx, colindx;
    int32_t val;
    char *line; 
    char *token; 
    line = (char *) malloc(1000 * sizeof(char));
    int done = false;
    uint64_t i = 0;

    printf("%s\n", fileName);    

    if(fp == NULL)
    {
        printf("file doesn\'t open\n");
    }

    while(fgets(line, 1000, fp) != NULL){
        token = strtok(line, " ");

        if(token[0] == '%'){
            ;
        } else if (done == false) {
            cooMtx->nrows = atoi(token);
            token = strtok(NULL, " ");
            cooMtx->ncols = atoi(token);
            token = strtok(NULL, " ");
            cooMtx->nnz = strtoull(token, NULL, 10); 
            printf("[INFO] %s: %u Rows, %u Cols, %u NNZs\n", strrchr(fileName, '/')+1, cooMtx->nrows, cooMtx->ncols, cooMtx->nnz);
            if((cooMtx->nrows % (8 / byte_dt)) != 0) { // Padding needed
                cooMtx->nrows += ((8 / byte_dt) - (cooMtx->nrows % (8 / byte_dt)));
            }
            if((cooMtx->ncols % (8 / byte_dt)) != 0) { // Padding needed
                cooMtx->ncols += ((8 / byte_dt) - (cooMtx->ncols % (8 / byte_dt)));
            }

            cooMtx->rows = (uint32_t *) calloc((cooMtx->nrows), sizeof(uint32_t));
            cooMtx->nnzs = (struct elem_t *) calloc((cooMtx->nnz+8), sizeof(struct elem_t));
            done = true;
        } else {
            // rowindx = atoi(token);
            // token = strtok(NULL, " ");
            // colindx = atoi(token);
            // token = strtok(NULL, " ");
            
            colindx = atoi(token);
            token = strtok(NULL, " ");
            rowindx = atoi(token);
            token = strtok(NULL, " ");
            // val = (val_dt) (rand()%4 + 1);
            val = (val_dt) (i % 4 + 1);

            cooMtx->nnzs[i].rowind = rowindx - 1; // Convert indexes to start at 0
            cooMtx->nnzs[i].colind = colindx - 1; // Convert indexes to start at 0
            cooMtx->nnzs[i].val = val; 
            i++;
        }
    }

    free(line);
    fclose(fp);
    return cooMtx;
}

struct COOMatrix *readCOOMatrix_rev(const char* fileName) {

    
    struct COOMatrix *cooMtx;
    cooMtx = (struct COOMatrix *) malloc(sizeof(struct COOMatrix));
    FILE* fp = fopen(fileName, "r");
    uint32_t rowindx, colindx;
    int32_t val;
    char *line; 
    char *token; 
    line = (char *) malloc(1000 * sizeof(char));
    int done = false;
    uint64_t i = 0;

    printf("%s\n", fileName);    

    if(fp == NULL)
    {
        printf("file doesn\'t open\n");
    }

    while(fgets(line, 1000, fp) != NULL){
        token = strtok(line, " ");

        if(token[0] == '%'){
            ;
        } else if (done == false) {
            cooMtx->ncols = atoi(token);
            token = strtok(NULL, " ");
            cooMtx->nrows = atoi(token);
            token = strtok(NULL, " ");
            cooMtx->nnz = strtoull(token, NULL, 10); 
            printf("[INFO] %s: %u Rows, %u Cols, %u NNZs\n", strrchr(fileName, '/')+1, cooMtx->nrows, cooMtx->ncols, cooMtx->nnz);
            if((cooMtx->nrows % (8 / byte_dt)) != 0) { // Padding needed
                cooMtx->nrows += ((8 / byte_dt) - (cooMtx->nrows % (8 / byte_dt)));
            }
            if((cooMtx->ncols % (8 / byte_dt)) != 0) { // Padding needed
                cooMtx->ncols += ((8 / byte_dt) - (cooMtx->ncols % (8 / byte_dt)));
            }

            cooMtx->rows = (uint32_t *) calloc((cooMtx->nrows), sizeof(uint32_t));
            cooMtx->nnzs = (struct elem_t *) calloc((cooMtx->nnz+8), sizeof(struct elem_t));
            done = true;
        } else {
            // rowindx = atoi(token);
            // token = strtok(NULL, " ");
            // colindx = atoi(token);
            // token = strtok(NULL, " ");
            
            colindx = atoi(token);
            token = strtok(NULL, " ");
            rowindx = atoi(token);
            token = strtok(NULL, " ");
            // val = (val_dt) (rand()%4 + 1);
            val = (val_dt) (i % 4 + 1);

            cooMtx->nnzs[i].rowind = rowindx - 1; // Convert indexes to start at 0
            cooMtx->nnzs[i].colind = colindx - 1; // Convert indexes to start at 0
            cooMtx->nnzs[i].val = val; 
            i++;
        }
    }

    free(line);
    fclose(fp);
    return cooMtx;
}

/** 
 * brief Comparator for Quicksort
 */
int comparator(const void *a, const void *b) {
    if (((struct elem_t *)a)->rowind < ((struct elem_t *)b)->rowind) {
        return -1;
    } else if (((struct elem_t *)a)->rowind > ((struct elem_t *)b)->rowind) {
        return 1;
    } else {
        return (((struct elem_t *)a)->colind - ((struct elem_t *)b)->colind);
    }
}

/**
 * @brief Sort CooMatrix
 * @param matrix in COO format
 */
void sortCOOMatrix(struct COOMatrix *cooMtx) {

    qsort(cooMtx->nnzs, cooMtx->nnz, sizeof(struct elem_t), comparator);

    int prev_row = cooMtx->nnzs[0].rowind;
    int cur_nnz = 0;
    for(unsigned int n = 0; n < cooMtx->nnz; n++) {
        if(cooMtx->nnzs[n].rowind == prev_row)
            cur_nnz++;
        else {
            cooMtx->rows[prev_row] = cur_nnz;
            prev_row = cooMtx->nnzs[n].rowind;
            cur_nnz = 1;
        }
    }
    cooMtx->rows[prev_row] = cur_nnz;

    cur_nnz = 0;
    for(unsigned int r = 0; r < cooMtx->nrows; r++) {
        cur_nnz += cooMtx->rows[r];
    }
    assert(cur_nnz == cooMtx->nnz);
}

/**
 * @brief deallocate matrix in COO format 
 * @param matrix in COO format
 */
void freeCOOMatrix(struct COOMatrix *cooMtx) {
    free(cooMtx->rows);
    free(cooMtx->nnzs);
    free(cooMtx);
}

/**
 * @brief convert matrix from COO to RBDCSR format 
 * @param matrix in COO format
 * @param horz_partitions, vert_partitions
 */
struct RBDCSRMatrix *coo2rbdcsr(struct COOMatrix *cooMtx, int horz_partitions, int vert_partitions) {

    struct RBDCSRMatrix *rbdcsrMtx;
    rbdcsrMtx = (struct RBDCSRMatrix *) malloc(sizeof(struct RBDCSRMatrix));

    rbdcsrMtx->nrows = cooMtx->nrows;
    rbdcsrMtx->ncols = cooMtx->ncols;
    rbdcsrMtx->nnz = cooMtx->nnz;

    rbdcsrMtx->npartitions = vert_partitions;
    rbdcsrMtx->horz_partitions = horz_partitions;
    rbdcsrMtx->vert_partitions = vert_partitions;
    rbdcsrMtx->tile_width = cooMtx->ncols / vert_partitions;
    if (cooMtx->ncols % vert_partitions != 0)
        rbdcsrMtx->tile_width++;

    // Count nnzs per partition
    rbdcsrMtx->nnzs_per_vert_partition = (uint32_t *) calloc((rbdcsrMtx->npartitions), sizeof(uint32_t));

    uint32_t p_row, p_col, local_col;
    rbdcsrMtx->drowptr = (uint32_t *) calloc((rbdcsrMtx->nrows + 2) * rbdcsrMtx->npartitions, sizeof(uint32_t));
    rbdcsrMtx->dcolind = (uint32_t *) malloc((rbdcsrMtx->nnz + 2) * sizeof(uint32_t));
    rbdcsrMtx->dval = (val_dt *) calloc((rbdcsrMtx->nnz + 8),  sizeof(val_dt)); // Padding needed

    for (uint32_t n = 0; n < cooMtx->nnz; n++) {
        p_row = cooMtx->nnzs[n].rowind; 
        p_col = cooMtx->nnzs[n].colind / rbdcsrMtx->tile_width;
        rbdcsrMtx->nnzs_per_vert_partition[p_col]++;
        rbdcsrMtx->drowptr[p_col * (rbdcsrMtx->nrows + 1) + p_row]++;
    }

    // Normalize local rowptrs within each partition 
    for (uint32_t p = 0; p < rbdcsrMtx->npartitions; p++) {
        uint32_t sumBeforeNextRow = 0;
        for(unsigned int rowIndx = 0; rowIndx < rbdcsrMtx->nrows; ++rowIndx) {
            uint32_t sumBeforeRow = sumBeforeNextRow;
            sumBeforeNextRow += rbdcsrMtx->drowptr[p * (rbdcsrMtx->nrows + 1) + rowIndx];
            rbdcsrMtx->drowptr[p * (rbdcsrMtx->nrows + 1) + rowIndx] = sumBeforeRow;
        }
        rbdcsrMtx->drowptr[p * (rbdcsrMtx->nrows + 1) + rbdcsrMtx->nrows] = sumBeforeNextRow;
    }

    // Fill nnzs to partitions
    uint64_t *global_nnzs = (uint64_t *) calloc(rbdcsrMtx->npartitions, sizeof(uint64_t));
    uint64_t *local_nnzs = (uint64_t *) calloc(rbdcsrMtx->npartitions, sizeof(uint64_t));
    uint64_t total_nnzs = 0;
    for (uint32_t p = 0; p < rbdcsrMtx->npartitions; p++) {
        global_nnzs[p] = total_nnzs;
        total_nnzs += rbdcsrMtx->nnzs_per_vert_partition[p];
    }

    for (uint64_t n = 0; n < cooMtx->nnz; n++) {
        p_row = cooMtx->nnzs[n].rowind; 
        p_col = cooMtx->nnzs[n].colind / rbdcsrMtx->tile_width;
        local_col =  cooMtx->nnzs[n].colind - (p_col * rbdcsrMtx->tile_width); 
        rbdcsrMtx->dcolind[global_nnzs[p_col] + local_nnzs[p_col]] = local_col; 
        rbdcsrMtx->dval[global_nnzs[p_col] + local_nnzs[p_col]] = cooMtx->nnzs[n].val;
        local_nnzs[p_col]++;
    }

    free(global_nnzs);
    free(local_nnzs);

    return rbdcsrMtx;
}


struct RBDCSRMatrix* convert_to_RBDCSR(struct COOMatrix* coo_mat){
    struct RBDCSRMatrix* csr_m;
    csr_m = (struct RBDCSRMatrix*)malloc(sizeof(struct RBDCSRMatrix));

    csr_m->nrows = coo_mat->nrows;
    csr_m->ncols = coo_mat->ncols;
    csr_m->nnz = coo_mat->nnz;

    csr_m->npartitions = 1;
    csr_m->horz_partitions = 1;
    csr_m->vert_partitions = 1;
    csr_m->tile_width = csr_m->ncols;

    csr_m->nnzs_per_vert_partition = (uint32_t*)calloc(csr_m->vert_partitions, sizeof(uint32_t));
    csr_m->nnzs_per_vert_partition[0] = csr_m->nnz;

    csr_m->drowptr = (uint32_t*)calloc(csr_m->nrows + 2, sizeof(uint32_t));
    csr_m->dcolind = (uint32_t*)calloc(csr_m->nnz + 2, sizeof(uint32_t));
    csr_m->dval = (val_dt*)calloc(csr_m->nnz + 8, sizeof(val_dt));

    for(uint32_t n = 0; n < coo_mat->nnz; ++n){
        csr_m->drowptr[coo_mat->nnzs[n].rowind]++;
    }

    uint32_t sumBeforeNextRow = 0;
    for(uint32_t rowIndx = 0; rowIndx < csr_m->nrows; ++rowIndx){
        uint32_t sumBeforeRow = sumBeforeNextRow;
        sumBeforeNextRow += csr_m->drowptr[rowIndx];
        csr_m->drowptr[rowIndx] = sumBeforeRow;
    }
    csr_m->drowptr[csr_m->nrows] = sumBeforeNextRow;

    uint64_t* row_counter = (uint64_t*)calloc(csr_m->nrows, sizeof(uint64_t));
    for(uint64_t n = 0; n < coo_mat->nnz; ++n){
        uint32_t row = coo_mat->nnzs[n].rowind;
        uint64_t idx = csr_m->drowptr[row] + row_counter[row];

        csr_m->dcolind[idx] = coo_mat->nnzs[n].colind;
        csr_m->dval[idx] = coo_mat->nnzs[n].val;
        row_counter[row]++;
    }

    free(row_counter);
    return csr_m;
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


#endif
