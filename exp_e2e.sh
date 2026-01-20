#!/bin/bash

num_iter=$1
file_type=$2

cd SpMV
./spmv_test_e2e.sh $num_iter $file_type
cd ..

cd Regex
./regex_test_all.sh $num_iter
cd ..

if [ ! -d "./graphs" ]; then
    mkdir ./graphs
fi