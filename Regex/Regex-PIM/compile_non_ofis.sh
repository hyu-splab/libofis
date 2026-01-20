# DPU binary compile
dpu-upmem-dpurte-clang -DNR_TASKLETS=11 -o ./bin/DPU_non_ofis ./DPU/DPU_non_ofis.c

# Host application compile
g++ -c ./DFA_library/make_DFA_1.cpp -std=c++1y
g++ -c ./DFA_library/make_BUF.cpp -std=c++1y
gcc --std=c99 -O2 -g -fopenmp -c ./Host/Host_regex_matching_non_OFIS.c `dpu-pkg-config --cflags --libs dpu`
gcc -o ./bin/Host_regex_matching_non_OFIS Host_regex_matching_non_OFIS.o make_DFA_1.o make_BUF.o `dpu-pkg-config --cflags --libs dpu` -std=c11 -lstdc++ -fopenmp

rm *.o