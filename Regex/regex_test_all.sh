#!/bin/bash

num_iter=$1

# PIM test
cd Regex-PIM/
./execution.sh $num_iter
cd ..

# CPU test
cd CPU_only/
make
./test.sh 16 $num_iter
make clean
cd ..

./scripts/make_cpu_results.sh $num_iter
./scripts/make_non_ofis_result.sh $num_iter
./scripts/make_ofis_results.sh $num_iter
./scripts/make_speedup.sh
python3 ./scripts/analyze_e2e.py