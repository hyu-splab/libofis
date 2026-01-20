#ifndef __STDINT_H
    #define __STDINT_H
    #include <stdint.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <omp.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>

#include <dpu.h>
#include <dpu_log.h>

#define NUM_THREAD 1

typedef struct timespec timespec;
inline void cur_Time(timespec* t) {
    clock_gettime(CLOCK_MONOTONIC, t);
}
inline float elapsed_Time(timespec st, timespec ed) {
    return 1000.0f * (ed.tv_sec - st.tv_sec) + 0.000001f * (ed.tv_nsec - st.tv_nsec);
}

// typedef struct timeval timeval;
// inline float elapsed_Time(timeval st, timeval ed) {
//     return 1000.0f * (ed.tv_sec - st.tv_sec) + 0.001f * (ed.tv_usec - st.tv_usec);
// }
// inline void cur_Time(timeval *t) {
//     gettimeofday(t, NULL);
// }


void prepare_final_BFS();
uint32_t* final_BFS(const uint32_t* intra_nnz, const uint32_t* intra_ptr, const uint16_t* intra_idx, const uint16_t max_icn, const uint64_t upB);
void finalize_BFS();

void prepare_final_SSSP();
uint32_t* final_SSSP(const uint16_t* intra_nnz, const uint16_t* intra_ptr, const uint16_t* intra_idx, const uint16_t* intra_val, const uint16_t max_icn, const uint16_t upB);
void finalize_SSSP();

float* global_BC(float* time);
void finalize_BC(uint8_t* cent_SG, float* cent_Global, int max_node, float* time);

float* global_PR(float* time);
void finalize_PR(float* rank_SG, float* rank_Global);

float* page_rank();
uint32_t* server_PR_int();
float* server_PR_float();
float* local_PR_float();
void local_PR_single_float(float* local_rank);
void result_fusion(float* server_rank, float* local_rank);

void analyze_sg();
void refinement(float* server_rank, float* local_rank);
void normalization(float* local_rank);
