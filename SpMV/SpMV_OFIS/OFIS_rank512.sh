#!/bin/bash

if [ ! -d "./bin" ]; then
    mkdir ./bin
fi

if [ ! -d "./results" ]; then
    mkdir ./results
fi

dpu-upmem-dpurte-clang -DTYPE=FP32 -DNR_TASKLETS=16 -o ./bin/spmv_2D_dpu CSR_dpu.c
gcc --std=c99 -fopenmp CSR_host_OFIS_rank.c -o ./bin/CSR_host `dpu-pkg-config --cflags --libs dpu` -DTYPE=FP32 -DNR_TASKLETS=16 -pthread -D_GNU_SOURCE

num_iter=$1
# ----------------------------------------------------------------------------------------------------
nr_part=512
nr_dpu=64
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_100"
done

nr_dpu=128
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_100"
done

nr_dpu=256
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_100"
done

nr_dpu=512
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_100"
done

nr_dpu=1024
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_100"
done

# ------------------------------------------------------------------------------------------------------

nr_dpu=64
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_200"
done

nr_dpu=128
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_200"
done

nr_dpu=256
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_200"
done

nr_dpu=512
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_200"
done

nr_dpu=1024
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_200"
done

# ------------------------------------------------------------------------------------------------------

nr_dpu=64
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_400"
done

nr_dpu=128
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_400"
done

nr_dpu=256
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_400"
done

nr_dpu=512
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_400"
done

nr_dpu=1024
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_400"
done

# ------------------------------------------------------------------------------------------------------

nr_dpu=64
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_600"
done

nr_dpu=128
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_600"
done

nr_dpu=256
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_600"
done

nr_dpu=512
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_600"
done

nr_dpu=1024
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_600"
done

# ------------------------------------------------------------------------------------------------------
nr_dpu=64
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_800"
done

nr_dpu=128
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_800"
done

nr_dpu=256
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_800"
done

nr_dpu=512
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_800"
done

nr_dpu=1024
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_800"
done

# ------------------------------------------------------------------------------------------------------
nr_dpu=64
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_1000"
done

nr_dpu=128
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_1000"
done

nr_dpu=256
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_1000"
done

nr_dpu=512
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_1000"
done

nr_dpu=1024
nr_thread=$(expr $nr_dpu / 64)
for ((i=1;i<=num_iter;i++))
do
    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./bin/CSR_host $nr_dpu $nr_part $nr_thread "dcsr_1000"
done

