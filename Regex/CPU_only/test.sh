#!/bin/bash

if [ ! -d "../results" ]; then
    mkdir ../results
fi

EXECUTABLES=("CPU_baseline")

#Threads
THREADS=($1)

#Text size
TEXT_SIZES=(1024)

#Rules
RULES=(64)

#Repeat
REPETITIONS=$2

initialize_result_file() {
    local filename="$1"
    touch "$filename"
    chown $(id -u):$(id -g) "$filename"
    chmod 644 "$filename"
}

initialize_result_file "../results/CPU_baseline.csv"

for ((rep=1; rep<=REPETITIONS; rep++)); do
    for exe in "${EXECUTABLES[@]}"; do
        for size in "${TEXT_SIZES[@]}"; do
            for rule in "${RULES[@]}"; do
                for thread in "${THREADS[@]}"; do
                    sudo sync
                    sudo sh -c "echo 3 > /proc/sys/vm/drop_caches"
                    sudo nice -n -10 ./$exe $size $rule $thread
                    initialize_result_file "../results/CPU_baseline.csv"
                done
            done
        done
    done
done

echo "Experiments completed successfully"