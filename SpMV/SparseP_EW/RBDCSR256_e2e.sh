#!/bin/bash

if [ ! -d "./bin" ]; then
    mkdir ./bin
fi

if [ ! -d "./results" ]; then
    mkdir ./results
fi

dpu-upmem-dpurte-clang -DTYPE=FP32 -DNR_TASKLETS=16 -o ./bin/spmv_dpu CSR_dpu.c
gcc --std=c99 -fopenmp CSR_host.c -o ./bin/CSR_host `dpu-pkg-config --cflags --libs dpu` -DTYPE=FP32 -DNR_TASKLETS=16

num_iter=$1
file_type=$2
# ----------------------------------------------------------------------------------------------------
datasets=(100 200 400 600 800 1000)
dpu_counts=(1024)

for dataset in "${datasets[@]}"
do
    for nr_dpu in "${dpu_counts[@]}"
    do
        for ((i=1;i<=num_iter;i++))
        do
            if [ $file_type -eq 0 ]; then
                sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host -f"../dataset/rbdcsr256_$dataset" -s"16" -d"$nr_dpu" -v"256" -t$file_type
            else 
                sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host -f"../dataset/ba_sparse_matrix_$dataset.txt" -s"16" -d"$nr_dpu" -v"256" -t$file_type
            fi
        done
    done
done    
# ----------------------------------------------------------------------------------------------------