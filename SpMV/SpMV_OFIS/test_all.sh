#!/bin/bash

num_iter=$1
./OFIS_rank512.sh $num_iter
./OFIS_ig512.sh $num_iter
./OFIS_dpu512.sh $num_iter
./OFIS_rank256.sh $num_iter
./OFIS_ig256.sh $num_iter
./OFIS_dpu256.sh $num_iter