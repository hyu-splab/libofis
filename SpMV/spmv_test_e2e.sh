#!/bin/bash

num_iter=$1
file_type=$2

# SpMV_OFIS test
cd SpMV_OFIS/
./OFIS_ig512_e2e.sh $num_iter $file_type
./CPU_pthread.sh $num_iter $file_type
cd ..

# SparseP_ES test
cd SparseP_ES/
./DCSR512_e2e.sh $num_iter $file_type
./DCSR256_e2e.sh $num_iter $file_type
cd ..

# SparseP_EW test
cd SparseP_EW/
./RBDCSR256_e2e.sh $num_iter $file_type
cd ..

# Generate figures
if [ $file_type -eq 1 ]; then
    ./scripts/make_cpu_results.sh $num_iter
    ./scripts/make_es_e2e_results.sh $num_iter
    ./scripts/make_ofis_e2e_results.sh $num_iter
    ./scripts/make_ew_e2e_results.sh $num_iter
    python3 ./scripts/analyze_e2e.py
fi

