#ifndef _COMMON_H_
#define _COMMON_H_

typedef struct{
    uint32_t nr_rows;
    uint32_t nr_cols;
    uint32_t nnz;
    uint32_t max_nr_rows;
    uint32_t max_nnz;
} dpu_args_t;

#endif