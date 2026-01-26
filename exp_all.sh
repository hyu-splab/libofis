#!/bin/bash

num_iter=$1
cd PageRank
./pg_test_all.sh $num_iter
cd ..

cd SpMV
./spmv_test_all.sh $num_iter
cd ..

cd Regex
./regex_test_all.sh $num_iter
cd ..

if [ ! -d "./graphs" ]; then
    mkdir ./graphs
fi

gnuplot gnuplot-script.plt