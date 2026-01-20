#ifndef _COMMON_H_
#define _COMMON_H_

/* Structures used by both the host and the dpu to communicate information */
typedef struct {
    uint32_t nrows;
    uint32_t max_rows;
    uint32_t max_nnz_ind;
    uint32_t tcols;
    uint32_t nnz_pad;
    uint32_t nnz_offset;
    uint32_t start_row[NR_TASKLETS];
    uint32_t rows_per_tasklet[NR_TASKLETS];
} dpu_arguments_t;

void print_dpu_arguments(dpu_arguments_t _input_args)
{
    printf("nrows: %d\n", _input_args.nrows);
    printf("max_rows: %d\n", _input_args.max_rows);
    printf("max_nnz_ind: %d\n", _input_args.max_nnz_ind);
    printf("tcols: %d\n", _input_args.tcols);
    printf("nnz_pad: %d\n", _input_args.nnz_pad);
    printf("nnz_offset: %d\n", _input_args.nnz_offset);

    for(int i = 0; i < NR_TASKLETS; i++)
    {
        printf("Tasklet %d\n", i);
        printf("start_row: %d, rows_per_tasklet: %d\n", _input_args.start_row[i], _input_args.rows_per_tasklet[i]);
    }
}

#endif
