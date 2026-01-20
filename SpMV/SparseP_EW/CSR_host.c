#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dpu.h>
#include <dpu_log.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>
#include <math.h>
#include <omp.h>

#include "includes/common.h"
#include "includes/matrix.h"
#include "includes/params.h"
#include "includes/partition.h"
#include "includes/timer.h"
#include "includes/utils.h"

// Define the DPU Binary path as DPU_BINARY here.
#ifndef DPU_BINARY
#define DPU_BINARY "./bin/spmv_dpu"
#endif

#define DPU_CAPACITY (64 << 20) // A DPU's capacity is 64 MB

/*
 * Main Structures:
 * 1. Matrices
 * 2. Input vector
 * 3. Output vector
 * 4. Help structures for data partitioning
 */
static struct RBDCSRMatrix* A;
static struct COOMatrix* B;
static val_dt* x;
static val_dt* z;
static val_dt* y;
static struct partition_info_t *part_info;


/**
 * @brief Specific information for each DPU
 */
struct dpu_info_t {
    uint32_t rows_per_dpu; 
    uint32_t cols_per_dpu; 
    uint32_t rows_per_dpu_pad; 
    uint32_t prev_rows_dpu; 
    uint32_t prev_nnz_dpu; 
    uint32_t nnz; 
    uint32_t nnz_pad; 
    uint32_t ptr_offset; 
};

struct dpu_info_t *dpu_info;


//////////////////////////////////////////////////////////////////////////////
void print_dpu_info(struct dpu_info_t _dpu_info)
{
    printf("rows_per_dpu: %d\n", _dpu_info.rows_per_dpu);
    printf("cols_per_dpu: %d\n", _dpu_info.cols_per_dpu);
    printf("rows_per_dpu_pad: %d\n", _dpu_info.rows_per_dpu_pad);
    printf("prev_rows_dpu: %d\n", _dpu_info.prev_rows_dpu);
    printf("prev_nnz_dpu: %d\n", _dpu_info.prev_nnz_dpu);
    printf("nnz: %d\n", _dpu_info.nnz);
    printf("nnz_pad: %d\n", _dpu_info.nnz_pad);
    printf("ptr_offset: %d\n", _dpu_info.ptr_offset);
}
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief find the dpus_per_row_partition
 * @param factor n to create partitions
 * @param column_partitions to create vert_partitions 
 * @param horz_partitions to return the 2D partitioning
 */
void find_partitions(uint32_t n, uint32_t *horz_partitions, uint32_t vert_partitions) {
    uint32_t dpus_per_vert_partition = n / vert_partitions;
    *horz_partitions = dpus_per_vert_partition;
}

/**
 * @brief initialize input vector 
 * @param pointer to input vector and vector size
 */
void init_vector(val_dt* vec, uint32_t size) {
    for(unsigned int i = 0; i < size; ++i) {
        vec[i] = (val_dt) (i%4+1);
    }
}

/**
 * @brief compute output in the host CPU
 */ 
static void spmv_host(val_dt* y, struct RBDCSRMatrix *A, val_dt* x) {
    uint64_t total_nnzs = 0;
    for (uint32_t c = 0; c < A->vert_partitions; c++) {
        for(uint32_t rowIndx = 0; rowIndx < A->nrows; ++rowIndx) {
            val_dt sum = 0;
            uint32_t ptr_offset = c * (A->nrows + 1);
            uint32_t col_offset = c * A->tile_width;
            for(uint32_t n = A->drowptr[ptr_offset + rowIndx]; n < A->drowptr[ptr_offset + rowIndx + 1]; n++) {
                uint32_t colIndx = A->dcolind[total_nnzs];    
                val_dt value = A->dval[total_nnzs++];    
                sum += x[col_offset + colIndx] * value;
            }
            y[rowIndx] += sum;
        }
    }
}


/**
 * @brief main of the host application
 */
int main(int argc, char **argv) {

    struct Params p = input_params(argc, argv);

    struct dpu_set_t dpu_set, dpu;
    uint32_t nr_of_dpus; 
    uint32_t nr_of_ranks; 

    uint32_t NR_DPUS = p.nr_dpus;

    // Allocate DPUs and load binary
    DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &dpu_set));
    DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY, NULL));
    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_of_dpus));
    DPU_ASSERT(dpu_get_nr_ranks(dpu_set, &nr_of_ranks));
    printf("[INFO] Allocated %d DPU(s)\n", nr_of_dpus);
    printf("[INFO] Allocated %d Rank(s)\n", nr_of_ranks);
    printf("[INFO] Allocated %d TASKLET(s) per DPU\n", NR_TASKLETS);



    unsigned int i;
    // Timer for measurements
    Timer timer;
    resetTimer(&timer);

    int file_type = p.file_type;

    uint32_t horz_partitions, vert_partitions;
    if(p.nr_partitions == 256){
        horz_partitions = 128 * 256;
        vert_partitions = 2;
    }else{
        horz_partitions = 256 * 512;
        vert_partitions = 2;
    }
    
    // Case 1: normal Matrix File-----------------------------------------------------------
    if(file_type == 1){
        // Load Sparse Matrix in RBDCSR format
        B = readCOOMatrix_rev(p.fileName);

        // B = readCOOMatrix(p.fileName);
        // sortCOOMatrix(B);

        printf("[INFO] %dx%d Matrix Partitioning\n\n", horz_partitions, vert_partitions);
        A = convert_to_RBDCSR(B);

        startTimer(&timer, 1);
        partition_CSR_EW(A, horz_partitions, vert_partitions);
        stopTimer(&timer, 1);
        freeCOOMatrix(B);
    }
    //-------------------------------------------------------------------------------------
    // Case 2: RBDCSR File for fast test-----------------------------------------------------------
    else if(file_type == 0){
        A = load_rbdcsr_matrix(p.fileName);
    }

// // RBDCSRMatrix Data Check
//     printf("nrows: %d\nncols: %d\nnnz: %d\nnpartitions: %d\nhorz_partitions: %d\nvert_partitions: %d\ntile_width: %d\n", A->nrows, A->ncols, A->nnz, A->npartitions, A->horz_partitions, A->vert_partitions, A->tile_width);
//     for(int i = 0; i < A->npartitions; i++)
//     {
//         printf("%u ", A->nnzs_per_vert_partition[i]);
//     }
//     printf("\n");
    
    // Initialize partition data
    startTimer(&timer, 7);
    part_info = partition_init(A, nr_of_dpus, p.max_nranks, NR_TASKLETS);
    stopTimer(&timer, 7);

    struct dpu_set_t rank;
    uint32_t each_rank;
    DPU_RANK_FOREACH(dpu_set, rank, each_rank){
        uint32_t nr_dpus_in_rank;
        DPU_ASSERT(dpu_get_nr_dpus(rank, &nr_dpus_in_rank));
        part_info->active_dpus_per_rank[each_rank+1] = nr_dpus_in_rank;
    }

    uint32_t sum = 0;
    for(uint32_t i=0; i < p.max_nranks+1; i++) {
        part_info->accum_dpus_ranks[i] = part_info->active_dpus_per_rank[i] + sum;
        sum += part_info->active_dpus_per_rank[i];
    }

    // for(int i = 0; i <= nr_of_ranks; i++)
    // {
    //     printf("%d ", part_info->accum_dpus_ranks[i]);
    // }
    // printf("\n");


    // Initialize help data - Padding needed
    uint32_t ncols_pad = A->ncols;
    uint32_t tile_width_pad = A->tile_width;
    uint32_t nrows_pad = A->nrows;
    if (ncols_pad % (8 / byte_dt) != 0)
        ncols_pad = ncols_pad + ((8 / byte_dt) - (ncols_pad % (8 / byte_dt)));
    if (tile_width_pad % (8 / byte_dt) != 0)
        tile_width_pad = tile_width_pad + ((8 / byte_dt) - (tile_width_pad % (8 / byte_dt)));
    if (nrows_pad % (8 / byte_dt) != 0)
        nrows_pad = nrows_pad + ((8 / byte_dt) - (nrows_pad % (8 / byte_dt)));

    // Allocate input vector
    x = (val_dt *) malloc(ncols_pad * sizeof(val_dt)); 

    // Allocate output vector
    z = (val_dt *) calloc(nrows_pad, sizeof(val_dt)); 

    // Initialize input vector with arbitrary data
    init_vector(x, ncols_pad);

    // Load-balance nnzs among DPUs of the same vertical partition
    startTimer(&timer, 8);
    partition_by_nnz(A, part_info);
    stopTimer(&timer, 8);

    // Initialize help data
    dpu_info = (struct dpu_info_t *) malloc(horz_partitions * vert_partitions * sizeof(struct dpu_info_t)); 
    dpu_arguments_t *input_args = (dpu_arguments_t *) malloc(horz_partitions * vert_partitions * sizeof(dpu_arguments_t));
    // Max limits for parallel transfers
    uint64_t max_rows_per_dpu = 0;
    uint64_t max_nnz_ind_per_dpu = 0;
    uint64_t max_nnz_val_per_dpu = 0;
    uint64_t max_rows_per_tasklet = 0;



    uint64_t total_nnzs = 0;
    i = 0;

    for(i = 0; i < horz_partitions * vert_partitions; i++) {
        // Find padding for rows and non-zero elements needed for CPU-DPU transfers
        uint32_t tile_horz_indx = i % A->horz_partitions; 
        uint32_t tile_vert_indx = i / A->horz_partitions; 
        uint32_t rows_per_dpu = part_info->row_split[tile_vert_indx * (A->horz_partitions + 1) + tile_horz_indx + 1] - part_info->row_split[tile_vert_indx * (A->horz_partitions + 1) + tile_horz_indx];
        uint32_t prev_rows_dpu = part_info->row_split[tile_vert_indx * (A->horz_partitions + 1) + tile_horz_indx];

        // Pad data to be transfered
        uint32_t rows_per_dpu_pad = rows_per_dpu + 1;
        if (rows_per_dpu_pad % (8 / byte_dt) != 0)
            rows_per_dpu_pad += ((8 / byte_dt) - (rows_per_dpu_pad % (8 / byte_dt)));
#if INT64 || FP64
        if (rows_per_dpu_pad % 2 == 1)
            rows_per_dpu_pad++;
#endif
        if (rows_per_dpu_pad > max_rows_per_dpu)
            max_rows_per_dpu = rows_per_dpu_pad;

        unsigned int nnz, nnz_ind_pad, nnz_val_pad;
        nnz = A->drowptr[tile_vert_indx * (A->nrows + 1) + prev_rows_dpu + rows_per_dpu] - A->drowptr[tile_vert_indx * (A->nrows + 1) + prev_rows_dpu];
        if (nnz % 2 != 0)
            nnz_ind_pad = nnz + 1;
        else
            nnz_ind_pad = nnz;
        if (nnz % (8 / byte_dt) != 0)
            nnz_val_pad = nnz + ((8 / byte_dt) - (nnz % (8 / byte_dt)));
        else
            nnz_val_pad = nnz;

#if INT64 || FP64
        if (nnz_ind_pad % 2 == 1)
            nnz_ind_pad++;
        if (nnz_val_pad % 2 == 1)
            nnz_val_pad++;
#endif
        if (nnz_ind_pad > max_nnz_ind_per_dpu)
            max_nnz_ind_per_dpu = nnz_ind_pad;
        if (nnz_val_pad > max_nnz_val_per_dpu)
            max_nnz_val_per_dpu = nnz_val_pad;

        uint32_t prev_nnz_dpu = total_nnzs;
        total_nnzs += nnz;

        // Keep information per DPU
        dpu_info[i].rows_per_dpu = rows_per_dpu;
        dpu_info[i].cols_per_dpu = A->tile_width;
        dpu_info[i].prev_rows_dpu = prev_rows_dpu;
        dpu_info[i].prev_nnz_dpu = prev_nnz_dpu;
        dpu_info[i].nnz = nnz;
        dpu_info[i].nnz_pad = nnz_ind_pad;
        dpu_info[i].ptr_offset = tile_vert_indx * (A->nrows + 1) + prev_rows_dpu;

        // Find input arguments per DPU
        input_args[i].nrows = rows_per_dpu;
        input_args[i].tcols = tile_width_pad; 
        input_args[i].nnz_pad = nnz_ind_pad;
        input_args[i].nnz_offset = A->drowptr[tile_vert_indx * (A->nrows + 1) + prev_rows_dpu];

        // Load-balance nnz across tasklets 
        if(i == 0) startTimer(&timer, 9);
        else startTimer_total(&timer, 9);
        partition_tsklt_by_nnz(A, part_info, i, rows_per_dpu, nnz, tile_vert_indx * (A->nrows + 1) + prev_rows_dpu, NR_TASKLETS);
        stopTimer(&timer, 9);

        uint32_t t;
        for (t = 0; t < NR_TASKLETS; t++) {
            // Find input arguments per tasklet
            input_args[i].start_row[t] = part_info->row_split_tasklet[t]; 
            input_args[i].rows_per_tasklet[t] = part_info->row_split_tasklet[t+1] - part_info->row_split_tasklet[t];
            if (input_args[i].rows_per_tasklet[t] > max_rows_per_tasklet)
                max_rows_per_tasklet = input_args[i].rows_per_tasklet[t];
        }


    }
    assert(A->nnz == total_nnzs && "wrong balancing");
    // for(int i = 0 ; i < 256 * 256; i++)
    // {
    //     printf("========Partition %d========\n", i);
    //     print_dpu_info(dpu_info[i]);
    //     printf("\n");
    //     print_dpu_arguments(input_args[i]);
    // }

    for(int dpu_iter = 0; dpu_iter < horz_partitions * vert_partitions; dpu_iter += NR_DPUS)
    {
    DPU_RANK_FOREACH(dpu_set, rank, each_rank){
        uint32_t max_rows_cur_rank = 0;
        uint32_t nr_dpus_in_rank;
        DPU_ASSERT(dpu_get_nr_dpus(rank, &nr_dpus_in_rank));
        uint32_t start_dpu = part_info->accum_dpus_ranks[each_rank];

        

        for (uint32_t k = 0; k < nr_dpus_in_rank; k++) {
            if (start_dpu + k >= nr_of_dpus)
                break;

            int _dpu_index = 0;
            if(start_dpu + k < NR_DPUS / 2)
            {
                _dpu_index = dpu_iter / 2 + start_dpu + k;
            }
            else
            {
                _dpu_index = dpu_iter / 2 + horz_partitions * vert_partitions / 2 + start_dpu + k - NR_DPUS/2;
            }

            if (dpu_info[_dpu_index].rows_per_dpu > max_rows_cur_rank)
                max_rows_cur_rank =  dpu_info[_dpu_index].rows_per_dpu;

        }
        if (max_rows_cur_rank % 2 != 0)
            max_rows_cur_rank++;
        if (max_rows_cur_rank % (8 / byte_dt)  != 0) 
            max_rows_cur_rank += ((8 / byte_dt) - (max_rows_cur_rank % (8 / byte_dt)));
        part_info->max_rows_per_rank[each_rank] = (uint32_t) max_rows_cur_rank;
    }



    // Initializations for parallel transfers with padding needed
    if (max_rows_per_dpu % 2 != 0)
        max_rows_per_dpu++;
    if (max_rows_per_dpu % (8 / byte_dt) != 0)
        max_rows_per_dpu += ((8 / byte_dt) - (max_rows_per_dpu % (8 / byte_dt)));
    if (max_nnz_ind_per_dpu % 2 != 0)
        max_nnz_ind_per_dpu++;
    if (max_nnz_val_per_dpu % (8 / byte_dt) != 0)
        max_nnz_val_per_dpu += ((8 / byte_dt) - (max_nnz_val_per_dpu % (8 / byte_dt)));
    if (max_rows_per_tasklet % (8 / byte_dt) != 0)
        max_rows_per_tasklet += ((8 / byte_dt) - (max_rows_per_tasklet % (8 / byte_dt)));

        
    // Re-allocations for padding needed
    // A->drowptr = (uint32_t *) realloc(A->drowptr, (max_rows_per_dpu * (uint64_t) nr_of_dpus * sizeof(uint32_t)));
    // A->dcolind = (uint32_t *) realloc(A->dcolind, (max_nnz_ind_per_dpu * nr_of_dpus * sizeof(uint32_t)));
    // A->dval = (val_dt *) realloc(A->dval, (max_nnz_val_per_dpu * nr_of_dpus * sizeof(val_dt)));
    // x = (val_dt *) realloc(x, (uint64_t) ((uint64_t) A->vert_partitions * (uint64_t) tile_width_pad) * (uint64_t) sizeof(val_dt)); 
    // y = (val_dt *) malloc((uint64_t) ((uint64_t) nr_of_dpus * (uint64_t) max_rows_per_dpu) * (uint64_t) sizeof(val_dt)); 
    uint64_t total_logical_dpus = (uint64_t)A->horz_partitions * A->vert_partitions; // 256 * 128 * 2
    A->drowptr = (uint32_t *) realloc(A->drowptr, (max_rows_per_dpu * total_logical_dpus * sizeof(uint32_t)));
    A->dcolind = (uint32_t *) realloc(A->dcolind, (max_nnz_ind_per_dpu * total_logical_dpus * sizeof(uint32_t)));
    A->dval = (val_dt *) realloc(A->dval, (max_nnz_val_per_dpu * total_logical_dpus * sizeof(val_dt)));
    x = (val_dt *) realloc(x, (uint64_t) ((uint64_t) A->vert_partitions * (uint64_t) tile_width_pad) * (uint64_t) sizeof(val_dt));
    y = (val_dt *) malloc((uint64_t) (total_logical_dpus * (uint64_t) max_rows_per_dpu) * (uint64_t) sizeof(val_dt));


    // Count total number of bytes to be transfered in MRAM of DPU
    unsigned long int total_bytes;
    total_bytes = ((max_rows_per_dpu) * sizeof(uint32_t)) + (max_nnz_ind_per_dpu * sizeof(uint32_t)) + (max_nnz_val_per_dpu * sizeof(val_dt)) + (tile_width_pad * sizeof(val_dt)) + (max_rows_per_dpu * sizeof(val_dt));
    assert(total_bytes <= DPU_CAPACITY && "Bytes needed exceeded MRAM size");

    
    
   // Copy input arguments to DPUs
    i = 0;
    DPU_FOREACH(dpu_set, dpu, i) {
        int _dpu_index = 0;
        if(i < NR_DPUS / 2)
        {
            _dpu_index = dpu_iter / 2 + i;
        }
        else
        {
            _dpu_index = dpu_iter / 2 + horz_partitions * vert_partitions / 2 + i - NR_DPUS/2;
        }
        input_args[_dpu_index].max_rows = max_rows_per_dpu; 
        input_args[_dpu_index].max_nnz_ind = max_nnz_ind_per_dpu; 
        DPU_ASSERT(dpu_prepare_xfer(dpu, input_args + _dpu_index));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS", 0, sizeof(dpu_arguments_t), DPU_XFER_DEFAULT));

    // Copy input matrix to DPUs
    startTimer_total(&timer, 0);

    // Copy Rowptr 
    i = 0;
    DPU_FOREACH(dpu_set, dpu, i) {
        int _dpu_index = 0;
        if(i < NR_DPUS / 2)
        {
            _dpu_index = dpu_iter / 2 + i;
        }
        else
        {
            _dpu_index = dpu_iter / 2 + horz_partitions * vert_partitions / 2 + i - NR_DPUS/2;
        }
        DPU_ASSERT(dpu_prepare_xfer(dpu, A->drowptr + dpu_info[_dpu_index].ptr_offset));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, (max_rows_per_dpu * sizeof(val_dt) + tile_width_pad * sizeof(val_dt)), max_rows_per_dpu * sizeof(uint32_t), DPU_XFER_DEFAULT));

    // Copy Colind
    i = 0;
    DPU_FOREACH(dpu_set, dpu, i) {
        int _dpu_index = 0;
        if(i < NR_DPUS / 2)
        {
            _dpu_index = dpu_iter / 2 + i;
        }
        else
        {
            _dpu_index = dpu_iter / 2 + horz_partitions * vert_partitions / 2 + i - NR_DPUS/2;
        }
        DPU_ASSERT(dpu_prepare_xfer(dpu, A->dcolind + dpu_info[_dpu_index].prev_nnz_dpu));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, max_rows_per_dpu * sizeof(val_dt) + tile_width_pad * sizeof(val_dt) + max_rows_per_dpu * sizeof(uint32_t), max_nnz_ind_per_dpu * sizeof(uint32_t), DPU_XFER_DEFAULT));

    // Copy Values
    i = 0;
    DPU_FOREACH(dpu_set, dpu, i) {
        int _dpu_index = 0;
        if(i < NR_DPUS / 2)
        {
            _dpu_index = dpu_iter / 2 + i;
        }
        else
        {
            _dpu_index = dpu_iter / 2 + horz_partitions * vert_partitions / 2 + i - NR_DPUS/2;
        }
        DPU_ASSERT(dpu_prepare_xfer(dpu, A->dval + dpu_info[_dpu_index].prev_nnz_dpu));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, max_rows_per_dpu * sizeof(val_dt) + tile_width_pad * sizeof(val_dt) + max_rows_per_dpu * sizeof(uint32_t) + max_nnz_ind_per_dpu * sizeof(uint32_t), max_nnz_val_per_dpu * sizeof(val_dt), DPU_XFER_DEFAULT));
    stopTimer(&timer, 0);


    // Copy input vector  to DPUs
    if(dpu_iter == 0)  startTimer_total(&timer, 2);
    else startTimer_total(&timer, 3);
    i = 0;
    DPU_FOREACH(dpu_set, dpu, i) {
        int _dpu_index = 0;
        if(i < NR_DPUS / 2)
        {
            _dpu_index = dpu_iter / 2 + i;
        }
        else
        {
            _dpu_index = dpu_iter / 2 + horz_partitions * vert_partitions / 2 + i - NR_DPUS/2;
        }
        uint32_t logical_dpu_idx = _dpu_index;
        uint32_t tile_vert_indx = logical_dpu_idx / A->horz_partitions; 
        DPU_ASSERT(dpu_prepare_xfer(dpu, x + tile_vert_indx * A->tile_width));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, max_rows_per_dpu * sizeof(val_dt), tile_width_pad * sizeof(val_dt), DPU_XFER_DEFAULT));
    if(dpu_iter == 0)  stopTimer(&timer, 2);
    else stopTimer(&timer, 3);


    
    // Run kernel on DPUs
    startTimer_total(&timer, 4);
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    stopTimer(&timer, 4);

#if LOG
    // Display DPU Log (default: disabled)
    DPU_FOREACH(dpu_set, dpu) {
        DPU_ASSERT(dpulog_read_for_dpu(dpu.dpu, stdout));
    }
#endif

    // Retrieve results for output vector from DPUs
    startTimer_total(&timer, 5);

    // Fine-grained data transfers in the output vector at rank granularity
    i = 0;
    DPU_FOREACH(dpu_set, dpu, i) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, y + (i * max_rows_per_dpu)));
    }
    i = 0;
    //struct dpu_set_t rank;
    DPU_RANK_FOREACH(dpu_set, rank) {
        DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, part_info->max_rows_per_rank[i] * sizeof(val_dt), DPU_XFER_ASYNC));
        i++;
    }
    DPU_ASSERT(dpu_sync(dpu_set));
    stopTimer(&timer, 5);




// Merge partial results to the host CPU
startTimer_total(&timer, 6);
uint32_t c, r, t, i;

DPU_FOREACH(dpu_set, dpu, i) {
    int _dpu_index = 0;
    if(i < NR_DPUS/2)
    {
        _dpu_index = dpu_iter / 2 + i;
    }
    else
    {
        _dpu_index = dpu_iter / 2 + horz_partitions * vert_partitions / 2 + i - NR_DPUS/2;
    }
    uint32_t logical_dpu_idx = _dpu_index;

    if (logical_dpu_idx >= A->horz_partitions * A->vert_partitions) break;

    c = logical_dpu_idx / A->horz_partitions;
    r = logical_dpu_idx % A->horz_partitions;

    uint32_t start_row = part_info->row_split[c * (A->horz_partitions + 1) + r];
    uint32_t rows_to_merge = part_info->row_split[c * (A->horz_partitions + 1) + r + 1] - start_row;

    #pragma omp parallel for num_threads(p.nthreads) shared(z, y, rows_to_merge, max_rows_per_dpu, start_row) private(t)
    for (t = 0; t < rows_to_merge; t++) {
        z[start_row + t] += y[i * max_rows_per_dpu + t];
    }
}
stopTimer(&timer, 6);




    // printf("Retrieve Output Vector: Iter %d", dpu_iter / NR_DPUS);
    // printTimer(&timer, 3);

}

    // Print timing results
    // printf("\n");
    // printf("Load Matrix");
    // printTimer(&timer, 0);
    // printf("Load Input Vector (dup x)");
    // printTimer(&timer, 2);
    // printf("Load Input Vector (dup O)");
    // printTimer(&timer, 3);
    // printf("Kernel");
    // printTimer(&timer, 4);
    // printf("Retrieve Output Vector");
    // printTimer(&timer, 5);
    // printf("Merge Partial Results");
    // printTimer(&timer, 6);

    char output_filename[50];
    if(p.nr_partitions == 256){
        if(file_type == 0) strcpy(output_filename, "./results/EW_256.csv");
        if(file_type == 1) strcpy(output_filename, "./results/EW_256_e2e.csv");
    }else if (p.nr_partitions == 512){
        if(file_type == 0) strcpy(output_filename, "./results/EW_512.csv");
        if(file_type == 1) strcpy(output_filename, "./results/EW_512_e2e.csv");
    }
    FILE* fp2 = fopen(output_filename, "a");

    if(fp2 == NULL) exit(1);
    double total_time = timer.time[0] + timer.time[3] + timer.time[4] + timer.time[5] + timer.time[6];
    // nr_dpus | load_matrix | load_input | DPU_EXEC | retrieve_time | merge_time | tot_time
    if(file_type == 0) fprintf(fp2, "%d, %f, %f, %f, %f, %f, %f\n", NR_DPUS, (timer.time[0] / 1000000), (timer.time[2] / 1000000), (timer.time[4] / 1000000), (timer.time[5] / 1000000), (timer.time[6] / 1000000), total_time / 1000000);
    else{
        double partition_time = timer.time[1] + timer.time[7] + timer.time[8] + timer.time[9];
        double end_time = total_time + partition_time;
        // nr_dpus | partition_time | load_matrix | load_input | DPU_EXEC | retrieve_time | merge_time | E2E time
        fprintf(fp2, "%d, %f, %f, %f, %f, %f, %f, %f\n", NR_DPUS, (partition_time / 1000000), (timer.time[0] / 1000000), (timer.time[3] / 1000000), (timer.time[4] / 1000000), (timer.time[5] / 1000000), (timer.time[6] / 1000000), end_time / 1000000);
    }
    fclose(fp2);

#if CHECK_CORR
    // Check output
    // startTimer_total(&timer, 4);
    val_dt *y_host = (val_dt *) calloc(nrows_pad, sizeof(val_dt)); 
    spmv_host(y_host, A, x);

    val_dt sum_dpu = 0, sum_host = 0;
    
    bool status = true;
    i = 0;
    for (i = 0; i < A->nrows; i++) {
        sum_dpu += z[i];
        sum_host += y_host[i];
        // printf("row %d: %f %f\n",i ,y_host[i], z[i]);
        if(y_host[i] != z[i]) {
            // printf("row %d: %f %f\n",i ,y_host[i], z[i]);
            status = false;
        }
    }
    // printf("%f %f\n", sum_host, sum_dpu);

    if (status) {
        printf("Result:\t\t \033[34mCorrect\033[0m\n");
    } else {
        printf("Result:\t\t \033[31mIncorrect\033[0m\n");
    }
    printf("-------------------------------\n\n");

    free(y_host);
#endif


    // Deallocation
    freeRBDCSRMatrix(A);
    free(x);
    free(z);
    free(y);
    partition_free(part_info);

    DPU_ASSERT(dpu_free(dpu_set));
    return 0;

}
