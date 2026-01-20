#include <stdio.h>
#include <stdint.h>
#include <mram.h>
#include <alloc.h>
#include <defs.h>
#include <mutex.h>
#include <barrier.h>
#include <seqread.h>
#include "./includes/common.h"
#include "./includes/utils.h"

__host dpu_args_t DPU_INPUT_ARGS;

BARRIER_INIT(my_barrier, NR_TASKLETS);
uint32_t nnz_offset;

int find_start(uint32_t tasklet_id, uint32_t nr_rows){
    int bool_pad = 0;
    if(nr_rows % 2 != 0){
        nr_rows++;
        bool_pad = 1;
    }

    uint32_t base_row = (nr_rows / NR_TASKLETS) & ~1;
    uint32_t remain_row = nr_rows - (base_row * NR_TASKLETS);

    uint32_t return_val = tasklet_id * base_row + (tasklet_id <= remain_row / 2 ? tasklet_id * 2 : remain_row);
    if(bool_pad && (tasklet_id == NR_TASKLETS)) return_val -= -1;

    return return_val;
}

int main(){
    uint32_t tasklet_id = me();

    if(tasklet_id == 0){
        mem_reset();
    }
    barrier_wait(&my_barrier);

    // Load input arguments
    uint32_t nr_rows = DPU_INPUT_ARGS.nr_rows;
    uint32_t max_nr_rows = DPU_INPUT_ARGS.max_nr_rows;
    uint32_t max_nnz = DPU_INPUT_ARGS.max_nnz;
    uint32_t nr_cols = DPU_INPUT_ARGS.nr_cols;
    uint32_t nnz = DPU_INPUT_ARGS.nnz;

    // Find each start address in MRAM
    uint32_t vec_size = nr_cols * sizeof(uint32_t);
    uint32_t out_size = max_nr_rows * sizeof(uint32_t);
    uint32_t row_size = max_nr_rows * sizeof(uint32_t);
    uint32_t col_size = max_nnz * sizeof(uint32_t);
    uint32_t val_size = max_nnz * sizeof(uint32_t);

    uint32_t out_base = (uint32_t)(DPU_MRAM_HEAP_POINTER);
    uint32_t vec_base = out_base + out_size;
    uint32_t row_base = vec_base + vec_size;
    uint32_t col_base = row_base + row_size;
    uint32_t val_base = col_base + col_size; 

    uint32_t start_row = find_start(tasklet_id, nr_rows);
    uint32_t rows_per_tasklets = find_start(tasklet_id + 1, nr_rows) - start_row;

    if(start_row + rows_per_tasklets > nr_rows){
        rows_per_tasklets = nr_rows - start_row;
    }

    if(rows_per_tasklets == 0){
        goto EXIT;
    }
    
    // Get first rowptr
    uint64_t temp;
    mram_read((__mram_ptr void const*) row_base, (void*)(&temp), 8);
    nnz_offset = (uint32_t) temp;
    row_base += start_row * sizeof(uint32_t);

    seqreader_buffer_t cache_rowptr = seqread_alloc();
    seqreader_t sr_rowptr;

    uint32_t* current_row = seqread_init(cache_rowptr, (__mram_ptr void*)row_base, &sr_rowptr);
    uint32_t prev_row = *current_row;

    col_base += (prev_row - nnz_offset) * sizeof(uint32_t);
    val_base += (prev_row - nnz_offset) * sizeof(uint32_t);
    out_base += start_row * sizeof(uint32_t);

    seqreader_buffer_t cache_col_idx = seqread_alloc();
    seqreader_t sr_col_idx;

    seqreader_buffer_t cache_value = seqread_alloc();
    seqreader_t sr_values;

    uint32_t *current_col_idx = seqread_init(cache_col_idx, (__mram_ptr void*)col_base, &sr_col_idx);
    val_dt *current_value = seqread_init(cache_value, (__mram_ptr void*)val_base, &sr_values);

    val_dt* cache_input_vec = mem_alloc(8);
    val_dt* cache_output_vec = mem_alloc(8);

    val_dt acc; // result of current_row * input_vec
    uint32_t write_idx = 0;
    for(uint32_t i = start_row; i < start_row + rows_per_tasklets; ++i){
        current_row = seqread_get(current_row, sizeof(*current_row), &sr_rowptr);
        
        acc = 0;
        for(int j = 0; j < *current_row - prev_row; ++j){

            if(((*current_col_idx) & 1) == 0){ // when *current_col_idx is even
                mram_read((__mram_ptr void const*) (vec_base + (*current_col_idx) * sizeof(uint32_t)), cache_input_vec, 8);
                acc += (*current_value) * cache_input_vec[0];
            }else{ // when *current_col_idx is odd
                mram_read((__mram_ptr void const*) (vec_base + ((*current_col_idx) - 1) * sizeof(uint32_t)), cache_input_vec, 8);
                acc += (*current_value) * cache_input_vec[1];
            }
            
            current_col_idx = seqread_get(current_col_idx, sizeof(*current_col_idx), &sr_col_idx);
            current_value = seqread_get(current_value, sizeof(*current_value), &sr_values);
        }
        if(write_idx != 1){
            cache_output_vec[write_idx] = acc;
            write_idx++;
        }else{
            cache_output_vec[write_idx] = acc;
            write_idx = 0;
            mram_write(cache_output_vec, (__mram_ptr void*)out_base, 2 * sizeof(uint32_t));
            out_base += 2 * sizeof(uint32_t);
        }
        prev_row = *current_row;
    }

    if(write_idx != 0){
        for(int i = write_idx; i < 2; ++i){
            cache_output_vec[i] = 0;
        }
        mram_write(cache_output_vec, (__mram_ptr void*)out_base, 8);
    }

EXIT:
    return 0;
}