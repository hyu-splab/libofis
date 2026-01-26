#!/bin/bash

num_iter=$1
./OFIS_rank512.sh $num_iter
./OFIS_ig512.sh $num_iter
./OFIS_rank256.sh $num_iter
./CPU_pthread.sh $num_iter