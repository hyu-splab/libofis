#!/bin/bash

num_iter=$1
file_type=$2  # 0 for dcsr files, 1 for ba_sparse_matrix files
./DCSR512.sh $num_iter $file_type
./DCSR256.sh $num_iter $file_type