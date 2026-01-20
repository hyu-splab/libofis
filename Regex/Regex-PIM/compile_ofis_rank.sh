#!/bin/sh

# DPU binary compile
dpu-upmem-dpurte-clang -DNR_TASKLETS=11 -o ./bin/DPU_ofis ./DPU/DPU_ofis.c

# Host application compile
g++ -c ./DFA_library/make_DFA_1.cpp -std=c++1y
g++ -c ./DFA_library/make_BUF.cpp -std=c++1y
gcc --std=c99 -O2 -g -fopenmp -c ./Host/Host_regex_matching_OFIS_rank.c `dpu-pkg-config --cflags --libs dpu` -DNR_TASKLETS=16 -pthread -D_GNU_SOURCE
gcc -o ./bin/Host_regex_matching_OFIS_rank Host_regex_matching_OFIS_rank.o make_DFA_1.o make_BUF.o `dpu-pkg-config --cflags --libs dpu` -fopenmp -std=c11 -lstdc++ 
rm *.o