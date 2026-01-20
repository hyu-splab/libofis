#include "../header/common.h"

int total_ICN = 0;

// Only Analyze ICNs
void analyze_ICNs2() {
    int prev_SG = 0;
    for (int n = 0; n < mat.nr; ++n) {
        int cur_SG = mat.sg_info[n];

#pragma unroll
        for (int e = mat.ptr[n]; e < mat.ptr[n + 1]; ++e) {
            int dst = mat.idx[e];
            if (mat.sg_info[dst] != cur_SG) {
                if (is_ICN[n] == 0) {
                    is_ICN[n] = 1;
                    total_ICN++;
                    ICNs[sg_Sizes[cur_SG] + SGs[cur_SG].num_ICN] = n;
                    mapping_intra[n] = SGs[cur_SG].num_ICN;
                    SGs[cur_SG].num_ICN++;
                }
                if (is_ICN[dst] == 0) {
                    int to_SG = mat.sg_info[dst];
                    is_ICN[dst] = 1;
                    total_ICN++;
                    ICNs[sg_Sizes[to_SG] + SGs[to_SG].num_ICN] = dst;
                    mapping_intra[dst] = SGs[to_SG].num_ICN;
                    SGs[to_SG].num_ICN++;
                }
            }
        }
    }
}

void mapping_ICNs() {
    for (int sg = 0; sg < num_SGs; ++sg) {
        for (int n = sg_Sizes[sg]; n < sg_Sizes[sg + 1]; ++n) {
            assert(mat.sg_info[n] == sg);
            for (int e = mat.ptr[n]; e < mat.ptr[n + 1]; ++e) {
                int dst = mat.idx[e];
                if (mat.sg_info[dst] != sg) {
                    assert(is_ICN[n]);
                }
            }
            if (is_ICN[n]) {
                total_ICN++;
                ICNs[sg_Sizes[sg] + SGs[sg].num_ICN] = n;
                mapping_intra[n] = SGs[sg].num_ICN++;
            }
        }
    }

    for (int sg = 0; sg < num_SGs; ++sg) {
        for (int n = sg_Sizes[sg]; n < sg_Sizes[sg] + SGs[sg].num_ICN; ++n) {
            assert(is_ICN[ICNs[n]]);
        }
    }
}

void analyze_ICNs() {
    int prev_SG = 0;
    for (int n = 0; n < mat.nr; ++n) {
        int cur_SG = mat.sg_info[n];

#pragma unroll
        for (int e = mat.ptr[n]; e < mat.ptr[n + 1]; ++e) {
            int dst = mat.idx[e];
            if (mat.sg_info[dst] != cur_SG) {
                if (is_ICN[n] == 0) {
                    is_ICN[n] = 1;
                    total_ICN++;
                    ICNs[sg_Sizes[cur_SG] + SGs[cur_SG].num_ICN] = n;
                    mapping_intra[n] = SGs[cur_SG].num_ICN;
                    SGs[cur_SG].num_ICN++;
                }
                if (is_ICN[dst] == 0) {
                    int to_SG = mat.sg_info[dst];
                    is_ICN[dst] = 1;
                    total_ICN++;
                    ICNs[sg_Sizes[to_SG] + SGs[to_SG].num_ICN] = dst;
                    mapping_intra[dst] = SGs[to_SG].num_ICN;
                    SGs[to_SG].num_ICN++;
                }
            }
        }
    }
}

int analyze_ICEs() {
    int total_ICE = 0;
#pragma omp parallel for num_threads(8)
    for (int sg = 0; sg < num_SGs; ++sg) {
        for (int icn = 0; icn < SGs[sg].num_ICN; ++icn) {
            int node = ICNs[sg_Sizes[sg] + icn];
            int edge_offset = 0;
            for (int e = mat.ptr[node]; e < mat.ptr[node + 1]; ++e) {
                if (mat.sg_info[mat.idx[e]] != sg) {
                    ICEs[mat.ptr[node] + edge_offset++] = mat.idx[e];
                }
            }
            num_ICEs[node] = edge_offset;
        }
    }

    return total_ICE;
}

void set_Argument(argument* args, uint32_t* max_node, uint32_t* max_icn, uint32_t* max_edge) {
    uint32_t max_node_ = 0;
    uint32_t max_icn_ = 0;
    uint32_t min_node_ = 0xffff;
    uint32_t min_icn_ = 0xffff;
    uint32_t max_edge_ = 0;
    uint32_t min_edge_ = 0xffffffff;
    uint32_t total_ice_ = 0;

    for (int i = 0; i < num_SGs; ++i) {
        if (max_node_ < SGs[i].num_node) max_node_ = SGs[i].num_node;
        if (max_icn_ < SGs[i].num_ICN) max_icn_ = SGs[i].num_ICN;
        if (min_node_ > SGs[i].num_node) min_node_ = SGs[i].num_node;
        if (min_icn_ > SGs[i].num_ICN) min_icn_ = SGs[i].num_ICN;
        if (max_edge_ < SGs[i].num_edge) max_edge_ = SGs[i].num_edge;
        if (min_edge_ > SGs[i].num_edge) min_edge_ = SGs[i].num_edge;
    }

    for (int i = 0; i < num_SGs; ++i) {
        args[i].start_node = sg_Sizes[i];
        args[i].max_edge = max_edge_;
        args[i].max_node = max_node_;
        args[i].max_icn = max_icn_;
        args[i].num_node = (uint32_t)(SGs[i].num_node);
		args[i].num_icn = (uint32_t)(SGs[i].num_ICN);
    }

    *max_node = max_node_;
    *max_edge = max_edge_;
    *max_icn = max_icn_;

    printf("\n  -- Partitioning Result\n\tTotal Number of Nodes: %d\n\tTotal Number of ICNs : %d \n\tRatio of ICNs: %.2f %%\n\
\tNumber of Nodes in SG: %d ~ %d (Avg: %d)\n\tNumber of ICNs  in SG: %d ~ %d (Avg: %d)\n", \
mat.nr, total_ICN, 100.0f * total_ICN / mat.nr, min_node_, max_node_, mat.nr / num_SGs, min_icn_, max_icn_, total_ICN / num_SGs);
}

void set_Argument_PR(argument_pr* args, uint32_t* max_node, uint32_t* max_icn, uint32_t* max_edge) {
    uint32_t max_node_ = 0;
    uint32_t max_icn_ = 0;
    uint32_t min_node_ = 0xffff;
    uint32_t min_icn_ = 0xffff;
    uint32_t max_edge_ = 0;
    uint32_t min_edge_ = 0xffffffff;
    uint32_t avg_iter = 0;

    for (int i = 0; i < num_SGs; ++i) {
        if (max_node_ < SGs[i].num_node) max_node_ = SGs[i].num_node;
        if (max_icn_ < SGs[i].num_ICN) max_icn_ = SGs[i].num_ICN;
        if (min_node_ > SGs[i].num_node) min_node_ = SGs[i].num_node;
        if (min_icn_ > SGs[i].num_ICN) min_icn_ = SGs[i].num_ICN;
        if (max_edge_ < SGs[i].num_edge) max_edge_ = SGs[i].num_edge;
        if (min_edge_ > SGs[i].num_edge) min_edge_ = SGs[i].num_edge;
    }
    avg_iter /= num_SGs;

    for (int i = 0; i < num_SGs; ++i) {
        args[i].start_node = sg_Sizes[i];
        args[i].max_node = ALIGN2(max_node_);
        args[i].num_node = (uint32_t)(SGs[i].num_node);
        args[i].iter = avg_iter;
        args[i].tot_node = mat.nr;
    }

    *max_node = ALIGN2(max_node_);
    *max_edge = ALIGN2(max_edge_);
    *max_icn = max_icn_;

    // printf("\n  -- Partitioning Result\n\tTotal Number of Nodes: %d\n\tTotal Number of ICNs : %d \n\tRatio of ICNs: %.2f %%\n\
\tNumber of Nodes in SG: %d ~ %d (Avg: %d)\n\tNumber of ICNs  in SG: %d ~ %d (Avg: %d)\n\n", \
mat.nr, total_ICN, 100.0f * total_ICN / mat.nr, min_node_, max_node_, mat.nr / num_SGs, min_icn_, max_icn_, total_ICN / num_SGs);
}
