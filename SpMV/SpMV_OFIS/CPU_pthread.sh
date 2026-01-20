#!/bin/bash

if [ ! -d "./bin" ]; then
    mkdir ./bin
fi

if [ ! -d "./results" ]; then
    mkdir ./results
fi

gcc -std=c99 -fopenmp -pthread -DTYPE=FP32 -D_GNU_SOURCE CPU_pthread.c -o ./bin/CPU_pthread

datasets=(ba_sparse_matrix_100 ba_sparse_matrix_200 ba_sparse_matrix_400 ba_sparse_matrix_600 ba_sparse_matrix_800 ba_sparse_matrix_1000)
# datasets_1=(dcsr_100 dcsr_200 dcsr_400 dcsr_600 dcsr_800 dcsr_1000)
# threads=(1 2 4 8 16)
threads=32
num_iter=$1
file_type=$2

for dataset in "${datasets[@]}"
do
    for ((i=1;i<=num_iter;i++))
    do
        ./bin/CPU_pthread $threads $dataset 1
    done
done
