#!/bin/bash

if [ ! -d "./bin" ]; then
    mkdir ./bin
fi

if [ ! -d "./results" ]; then
    mkdir ./results
fi

dpu-upmem-dpurte-clang -DTYPE=FP32 -DNR_TASKLETS=16 -o ./bin/spmv_2D_dpu CSR_dpu.c
gcc --std=c99 -fopenmp CSR_host_OFIS_rank.c -o ./bin/CSR_host `dpu-pkg-config --cflags --libs dpu` -DTYPE=FP32 -DNR_TASKLETS=16 -pthread -D_GNU_SOURCE

num_iter=$1
# ------------------------------------------------------------------------------------------------------
nr_part=256
datsets=(100 200 400 600 800 1000)
dpu_counts=(64 128 256 512 1024)

for dataset in "${datsets[@]}"
do
    for nr_dpu in "${dpu_counts[@]}"
    do
        nr_thread=$(expr $nr_dpu / 64)
        for ((i=1;i<=num_iter;i++))
        do
            sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr256_${dataset}"
        done
    done
done
# ------------------------------------------------------------------------------------------------------