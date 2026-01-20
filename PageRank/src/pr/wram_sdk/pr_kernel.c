/**
 * @file pr_kernel.c
 * @author Taehyeong Park (taehyeongpark@yonsei.ac.kr)
 * @brief Graph-partitioning BFS on UPMEM PIM
 * 
 * @copyright Copyright (c) 2023
 */

#include <attributes.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <defs.h>
#include <mram.h>
#include <mram_unaligned.h>
#include <alloc.h>
#include <barrier.h>
#include <mutex.h>
#include <seqread.h>
#include "../../../header/common.h"

#define ITER 50

BARRIER_INIT(barrier, NR_TASKLETS);


#define GROUP 4 // tasklet group for add array, must factor of NR_TASKLETS
MUTEX_INIT(mutex0); // tasklet 0 ~ 3
MUTEX_INIT(mutex1); // tasklet 4 ~ 7
MUTEX_INIT(mutex2); // tasklet 8 ~ 11
MUTEX_INIT(mutex3); // tesklet 12 ~ 15


__host argument_pr arg;
__host float rank[1500];
__host uint32_t ptr[1500];
float* add;
float norm_sum; // Used when Normalization
uint32_t offset = NR_TASKLETS;

void mutex_group_lock(uint32_t tid_group){
    switch (tid_group)
    {
    case 0:
        mutex_lock(mutex0);
        break;
    case 1:
        mutex_lock(mutex1);
        break;
    case 2:
        mutex_lock(mutex2);
        break;
    case 3:
        mutex_lock(mutex3);
        break;
    default:
        break;
    }
}

void mutex_group_unlock(uint32_t tid_group){
    switch (tid_group)
    {
    case 0:
        mutex_unlock(mutex0);
        break;
    case 1:
        mutex_unlock(mutex1);
        break;
    case 2:
        mutex_unlock(mutex2);
        break;
    case 3:
        mutex_unlock(mutex3);
        break;
    default:
        break;
    }
}

int main() {
    float scale = 1.0f;
    float damping = 0.85f;

    if (arg.num_node == 0) return 0;
    uint32_t tid = me();
    uint32_t tid_group = tid / GROUP;

    uint32_t refine_size = ALIGN2(arg.tot_node) * sizeof(float);
    uint32_t out_size = arg.max_node * sizeof(float);
    uint32_t ptr_size = (arg.max_node + 2) * sizeof(uint32_t);

    uint32_t refine_base = (uint32_t)(DPU_MRAM_HEAP_POINTER);
    uint32_t idx_base = refine_base + refine_size;

    uint32_t *current_edge;

    if (tid == 0) {
        mem_reset();
        add = mem_alloc(MUL4_ALIGN8(arg.num_node * (NR_TASKLETS / GROUP)));
    }
    barrier_wait(&barrier);

    uint32_t start = (arg.num_node / NR_TASKLETS) * tid;
    uint32_t end = (arg.num_node / NR_TASKLETS) * (tid + 1);
    if (tid == NR_TASKLETS - 1) end = arg.num_node;

    for (int n = tid; n < arg.num_node; n += NR_TASKLETS) {
        rank[n] = scale / arg.num_node;
    }

    uint32_t start_edge = ptr[0];

    seqreader_buffer_t cache_idx = seqread_alloc();
    seqreader_t seq_idx;

    // 1. Calculate Local Rank
    for (int i = 0; i < ITER; ++i) {

        if(tid == tid_group * GROUP){
            for (int node = 0; node < arg.num_node; ++node)
                add[tid_group * arg.num_node + node] = 0;
        }
        barrier_wait(&barrier);
        for(uint32_t node = start; node < end; ++node){
            uint32_t node_addr_aligned = ADDR_ALIGN8((uint32_t)node << 2);
            uint32_t num_edge;
            num_edge = ptr[node + 1] - ptr[node];
            current_edge = seqread_init(cache_idx, (__mram_ptr void *)(idx_base + ((ptr[node]) << 2)), &seq_idx);

            uint32_t con_edge = 0;
            for (int e = 0; e < num_edge; ++e) {
                int32_t node_dst = (int32_t)(*current_edge) - (int32_t)arg.start_node;
                current_edge = seqread_get(current_edge, sizeof(*current_edge), &seq_idx);
                if (node_dst >= 0 && node_dst < arg.num_node) con_edge++;
            }
            if (con_edge > 0) {
                current_edge = seqread_init(cache_idx, (__mram_ptr void *)(idx_base + ((ptr[node]) << 2)), &seq_idx);
                float move = rank[node] / con_edge;
                for (int e = 0; e < num_edge; ++e) {
                    int32_t node_dst = (int32_t)(*current_edge) - (int32_t)arg.start_node;
                    current_edge = seqread_get(current_edge, sizeof(*current_edge), &seq_idx);
                    if (node_dst >= 0 && node_dst < arg.num_node){
                        mutex_group_lock(tid_group);
                        add[tid_group * arg.num_node + node_dst] += move;
                        mutex_group_unlock(tid_group);
                    }
                }
            }
        }
        barrier_wait(&barrier);

        for (int node = start; node < end; ++node) {
            float acc = 0.0f;
            for (int i = 0; i < (NR_TASKLETS / GROUP); ++i) {
                acc += add[i * arg.num_node + node];
            }
            float new_rank = ((scale - damping) / arg.num_node) + (damping * acc);
            rank[node] = new_rank;
        }
        barrier_wait(&barrier);
    }

    // 2. Refinement
    seqreader_buffer_t cache_ref = seqread_alloc();
    seqreader_t seq_ref;
    float* current_ref;
    for(int node = start; node < end; ++node){
        if(node == start)
            current_ref = seqread_init(cache_ref, (__mram_ptr void*)(refine_base + ((arg.start_node + node) << 2)), &seq_ref);
        else
            current_ref = seqread_get(current_ref, sizeof(*current_ref), &seq_ref);
        rank[node] += (float)(*current_ref);
    }
    barrier_wait(&barrier);

    // 3. Normalization
    if(tid == 0){
        for(int node = 0; node < arg.num_node; ++node){
            norm_sum += rank[node];
        }
    }
    barrier_wait(&barrier);
    for(int node = start; node < end; ++node){
        rank[node] /= norm_sum;
    }
    barrier_wait(&barrier);

    // 4. Local Page Rank Single Float
    if(tid == tid_group * GROUP){
        for (int node = 0; node < arg.num_node; ++node)
            add[tid_group * arg.num_node + node] = 0;
    }
    barrier_wait(&barrier);
    for(uint32_t node = start; node < end; ++node){
        uint32_t node_addr_aligned = ADDR_ALIGN8((uint32_t)node << 2);
        uint32_t num_edge;
        num_edge = ptr[node + 1] - ptr[node];
        current_edge = seqread_init(cache_idx, (__mram_ptr void *)(idx_base + ((ptr[node]) << 2)), &seq_idx);
        uint32_t con_edge = 0;
        for (int e = 0; e < num_edge; ++e) {
            int32_t node_dst = (int32_t)(*current_edge) - (int32_t)arg.start_node;
            current_edge = seqread_get(current_edge, sizeof(*current_edge), &seq_idx);
            if (node_dst >= 0 && node_dst < arg.num_node) con_edge++;
        }
        if (con_edge > 0) {
            current_edge = seqread_init(cache_idx, (__mram_ptr void *)(idx_base + ((ptr[node]) << 2)), &seq_idx);
            float move = rank[node] / con_edge;
            for (int e = 0; e < num_edge; ++e) {
                int32_t node_dst = (int32_t)(*current_edge) - (int32_t)arg.start_node;
                current_edge = seqread_get(current_edge, sizeof(*current_edge), &seq_idx);
                if (node_dst >= 0 && node_dst < arg.num_node){
                    mutex_group_lock(tid_group);
                    add[tid_group * arg.num_node + node_dst] += move;
                    mutex_group_unlock(tid_group);
                }
            }
        }
    }
    barrier_wait(&barrier);

    for (int node = start; node < end; ++node) {
        float acc = 0.0f;
        for (int i = 0; i < (NR_TASKLETS / GROUP); ++i) {
            acc += add[i * arg.num_node + node];
        }
        float new_rank = ((scale - damping) / arg.num_node) + (damping * acc);
        rank[node] = new_rank;
    }
    barrier_wait(&barrier);

    // 5. Result Fusion
    for(int node = start; node < end; ++node){
        rank[node] *= arg.serv_rank;
    }

    norm_sum = 0;
    barrier_wait(&barrier);

    return 0;
}
