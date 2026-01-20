#!/bin/bash

if [ ! -d "./result" ]; then
    mkdir ./result
fi

num_iter=$1
mode=2
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/333SP.mtx.pimgt.metis.4096 4096 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/333SP.mtx.pimgt.metis.5120 5120 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/333SP.mtx.pimgt.metis.6144 6144 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/333SP.mtx.pimgt.metis.7168 7168 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/333SP.mtx.pimgt.metis.8192 8192 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/333SP.mtx.pimgt.metis.9216 9216 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/333SP.mtx.pimgt.metis.10240 10240 1024 $mode
done

for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/AS365.mtx.pimgt.metis.4096 4096 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/AS365.mtx.pimgt.metis.5120 5120 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/AS365.mtx.pimgt.metis.6144 6144 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/AS365.mtx.pimgt.metis.7168 7168 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/AS365.mtx.pimgt.metis.8192 8192 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/AS365.mtx.pimgt.metis.9216 9216 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/AS365.mtx.pimgt.metis.10240 10240 1024 $mode
done

for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/M6.mtx.pimgt.metis.4096 4096 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/M6.mtx.pimgt.metis.5120 5120 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/M6.mtx.pimgt.metis.6144 6144 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/M6.mtx.pimgt.metis.7168 7168 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/M6.mtx.pimgt.metis.8192 8192 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/M6.mtx.pimgt.metis.9216 9216 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/M6.mtx.pimgt.metis.10240 10240 1024 $mode
done

for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/NLR.mtx.pimgt.metis.4096 4096 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/NLR.mtx.pimgt.metis.5120 5120 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/NLR.mtx.pimgt.metis.6144 6144 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/NLR.mtx.pimgt.metis.7168 7168 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/NLR.mtx.pimgt.metis.8192 8192 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/NLR.mtx.pimgt.metis.9216 9216 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/NLR.mtx.pimgt.metis.10240 10240 1024 $mode
done

for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/rgg_n_2_21_s0.mtx.pimgt.metis.4096 4096 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/rgg_n_2_21_s0.mtx.pimgt.metis.5120 5120 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/rgg_n_2_21_s0.mtx.pimgt.metis.6144 6144 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/rgg_n_2_21_s0.mtx.pimgt.metis.7168 7168 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/rgg_n_2_21_s0.mtx.pimgt.metis.8192 8192 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/rgg_n_2_21_s0.mtx.pimgt.metis.9216 9216 1024 $mode
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/pr_ofis dataset/rgg_n_2_21_s0.mtx.pimgt.metis.10240 10240 1024 $mode
done