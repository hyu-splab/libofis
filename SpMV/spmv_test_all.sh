#!/bin/bash

num_iter=$1
# SparseP_ES test
cd SparseP_ES/
./test_all.sh $num_iter
cd ..

# SparseP_EW test
cd SparseP_EW/
./RBDCSR256.sh $num_iter
cd ..

# SpMV_OFIS test
cd SpMV_OFIS/
./test_all.sh $num_iter
cd ..

# Generate figures
./scripts/make_ofis_results.sh $num_iter
./scripts/make_es_results.sh $num_iter
./scripts/make_ew_results.sh $num_iter
./scripts/make_speedup_results.sh

# Generate e2e figures
./scripts/make_cpu_results.sh $num_iter
./scripts/make_es_e2e_results.sh $num_iter
./scripts/make_ofis_e2e_results.sh $num_iter
./scripts/make_ew_e2e_results.sh $num_iter
python3 ./scripts/analyze_e2e.py

