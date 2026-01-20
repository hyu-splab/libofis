#!/bin/bash

if [ ! -d "../results" ]; then
    mkdir ../results
fi

if [ ! -d "./bin" ]; then
    mkdir ./bin
fi

./compile_non_ofis.sh
./compile_ofis_ig.sh
./compile_ofis_rank.sh

num_iter=$1
for ((i=0; i<num_iter; i++)); do
    
    for ((j=1024; j>=64; j/=2)); do
        
        echo "Test $i, ig, DPU $j"
        sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/Host_regex_matching_OFIS_ig $j 1024 64 $((j / 64))  "../results/result_ofis_ig.csv"
        
        echo "Test $i, rank, DPU $j"
        sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/Host_regex_matching_OFIS_rank $j 1024 64 $((j / 64)) "../results/result_ofis_rank.csv"

        echo "Test $i, non_ofis, DPU $j"
        sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/Host_regex_matching_non_OFIS $j 1024 64 "../results/result_non_ofis.csv"
    done
done