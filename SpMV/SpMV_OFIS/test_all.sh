#!/bin/bash

num_iter=$1
file_type=$2
./OFIS_rank512.sh $num_iter
./OFIS_ig512.sh $num_iter $file_type
./OFIS_rank256.sh $num_iter
./OFIS_ig256.sh $num_iter
./CPU_baseline.sh $num_iter $file_type