#!/bin/bash

if [ ! -d "./bin" ]; then
    mkdir ./bin
fi

if [ ! -d "./results" ]; then
    mkdir ./results
fi

gcc -std=c99 -fopenmp -pthread -DTYPE=FP32 -D_GNU_SOURCE CPU_pthread.c -o ./bin/CPU_pthread

datasets=(ba_sparse_matrix_100 ba_sparse_matrix_200 ba_sparse_matrix_400 ba_sparse_matrix_600 ba_sparse_matrix_800 ba_sparse_matrix_1000)
threads=32
num_iter=$1

for dataset in "${datasets[@]}"
do
    for ((i=1;i<=num_iter;i++))
    do
        ./bin/CPU_pthread $threads $dataset
    done
done
