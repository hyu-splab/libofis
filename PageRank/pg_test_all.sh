#!/bin/bash

if [ ! -d "./bin" ]; then
    mkdir ./bin
fi

if [ ! -d "./obj" ]; then
    mkdir ./obj
fi

make 

num_iter=$1
# ./scripts/mram_exec.sh num_iter
# ./scripts/ofis_rank_exec.sh num_iter
./scripts/ofis_dpu_exec.sh $num_iter
./scripts/ofis_ig_exec.sh $num_iter
./scripts/wram_exec.sh $num_iter

./scripts/make_ofis_result.sh $num_iter
./scripts/make_sdk_result.sh $num_iter
./scripts/make_speedup_result.sh
