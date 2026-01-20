#include "../../../header/host.h"
#include "../../../header/common.h"

#define NUM_THREAD 32
#define DF 0.85f
#define SF 0.15f
#define EPSILON 5.96e-8f

#define SCALE 10000
#define DAMPING 8500
#define ITER 50

float* page_rank(){
    float* page_rank = calloc(mat.nr, sizeof(float));
    float* add = calloc(mat.nr, sizeof(float));
    float check = 0.0f;

    float scale = 1.0f;
    float damping = 0.85f;

    for(int i = 0; i < mat.nr; ++i){
        page_rank[i] = scale / mat.nr;
        check += page_rank[i];
    }
    // printf("before entire page rank: %f\n", check);

    for(int i = 0; i < ITER; ++i){
        for(int n = 0; n < mat.nr; ++n){
            add[n] = 0.0f;
        }
        for(int n = 0; n < mat.nr; ++n){
            int num_edge = mat.ptr[n + 1] - mat.ptr[n];
            if(num_edge > 0){
                float move = page_rank[n] / num_edge;
                for(int e = mat.ptr[n]; e < mat.ptr[n + 1]; ++e){
                    uint32_t dst = mat.idx[e];
                    add[dst] += move;
                }
            } 
        }

        check = 0.0f;
        for(int n = 0; n < mat.nr; ++n){
            float new_rank = ((scale - damping) / mat.nr) + (damping * add[n]);
            page_rank[n] = new_rank;
            check += page_rank[n];
        }
        // printf("[%d] %f\n", i, check);
    }

    free(add);
    return page_rank;
}

void analyze_sg(){

    num_ICEs_sg = calloc(mat.nr, sizeof(int*));
    for(int i = 0; i < mat.nr; ++i){
        num_ICEs_sg[i] = calloc(num_SGs, sizeof(int));
    }

    for(int sg = 0; sg < num_SGs; ++sg){
        uint32_t num_sg_edge = 0;
        for(int n = sg_Sizes[sg]; n < sg_Sizes[sg + 1]; ++n){
            for(int e = mat.ptr[n]; e < mat.ptr[n + 1]; ++e){
                int dst = mat.idx[e];
                if(mat.sg_info[dst] != sg){
                    num_sg_edge++;
                    num_ICEs_sg[dst][sg]++;
                }
            }
        }
        SGs[sg].num_ICE = num_sg_edge;
    }
}

uint32_t* server_PR_int(){
    uint32_t* add;
    uint32_t* server_rank;

    add = calloc(num_SGs, sizeof(uint32_t));
    server_rank = calloc(num_SGs, sizeof(uint32_t));

    uint32_t check = 0;
    for(int server = 0; server < num_SGs; ++server){
        server_rank[server] = SCALE / num_SGs;
        check += server_rank[server];
    }
    // printf("before SCALE: %d, total: %d\n", SCALE, check);

    for(int i = 0; i < ITER; ++i){
        for(int sg = 0; sg < num_SGs; ++sg)
            add[sg] = 0;
        for(int sg = 0; sg < num_SGs; ++sg){
            if(SGs[sg].num_ICE > 0){
                uint32_t move = server_rank[sg] / SGs[sg].num_ICE;
                for(int n = sg_Sizes[sg]; n < sg_Sizes[sg + 1]; ++n){
                    for(int e = mat.ptr[n]; e < mat.ptr[n + 1]; ++e){
                        int dst = mat.idx[e];
                        if(mat.sg_info[dst] != sg)
                            add[mat.sg_info[dst]] += move;
                    }
                }
            }
        }

        check = 0;
        for(int sg = 0; sg < num_SGs; ++sg){
            uint32_t new_rank = ((SCALE - DAMPING) / num_SGs) + ((DAMPING * add[sg]) / SCALE);
            server_rank[sg] = new_rank;
            check += server_rank[sg];
        }
        // printf("[%d] after SCALE: %d, total: %d\n", i, SCALE, check);
    }

    free(add);
    return server_rank;
}

float* local_PR_float(){
    float* add;
    float* local_rank;

    float scale = 1.0f;
    float damping = 0.85f;

    add = calloc(mat.nr, sizeof(float));
    local_rank = calloc(mat.nr, sizeof(float));
    uint32_t* conns = calloc(mat.nr, sizeof(uint32_t));

    float check = 0.0f;
    for(int sg = 0; sg < num_SGs; ++sg){
        uint32_t num_node = sg_Sizes[sg + 1] - sg_Sizes[sg];
        for(int n = sg_Sizes[sg]; n < sg_Sizes[sg + 1]; ++n){
            local_rank[n] = scale / num_node;
            check += local_rank[n];
        }
    }    
    // printf("before SCALE: %f, total: %f\n", scale, check);

    for(int i = 0; i < ITER; ++i){
        for(int sg = 0; sg < num_SGs; ++sg){
            uint32_t num_node = sg_Sizes[sg + 1] - sg_Sizes[sg];
            for(int n = sg_Sizes[sg]; n < sg_Sizes[sg + 1]; ++n){
                add[n] = 0.0f;
            }
            for(int n = sg_Sizes[sg]; n < sg_Sizes[sg + 1]; ++n){
                int con_edge = 0;
                for(int e = mat.ptr[n]; e < mat.ptr[n + 1]; ++e){
                    uint32_t dst = mat.idx[e];
                    if(mat.sg_info[dst] == sg){
                        conns[con_edge++] = dst;
                    }
                }

                if(con_edge > 0){
                    float move = local_rank[n] / con_edge;
                    for(int e = 0; e < con_edge; ++e){
                        add[conns[e]] += move;
                    }
                }
            }

            check = 0.0f;
            for(int n = sg_Sizes[sg]; n < sg_Sizes[sg + 1]; ++n){
                float new_rank = ((scale - damping) / num_node) + (damping * add[n]);
                local_rank[n] = new_rank;
                check += local_rank[n];
            }
            // if(i < 10)printf("[%d] [%d] %f\n", i, sg, check);
        }
    }

    free(add);
    return local_rank;
}

void local_PR_single_float(float* local_rank){
    float* add;
    float scale = 1.0f;
    float damping = 0.85f;

    add = calloc(mat.nr, sizeof(float));
    uint32_t* conns = calloc(mat.nr, sizeof(uint32_t));

    float check = 0.0f;
    for(int sg = 0; sg < num_SGs; ++sg){
        uint32_t num_node = sg_Sizes[sg + 1] - sg_Sizes[sg];
        for(int n = sg_Sizes[sg]; n < sg_Sizes[sg + 1]; ++n){
            check += local_rank[n];
        }
    }    
    // printf("before SCALE: %f, total: %f\n", scale, check);

    for(int sg = 0; sg < num_SGs; ++sg){
        uint32_t num_node = sg_Sizes[sg + 1] - sg_Sizes[sg];
        for(int n = sg_Sizes[sg]; n < sg_Sizes[sg + 1]; ++n){
            add[n] = 0.0f;
        }
        for(int n = sg_Sizes[sg]; n < sg_Sizes[sg + 1]; ++n){
            int con_edge = 0;
            for(int e = mat.ptr[n]; e < mat.ptr[n + 1]; ++e){
                uint32_t dst = mat.idx[e];
                if(mat.sg_info[dst] == sg){
                    conns[con_edge++] = dst;
                }
            }
            if(con_edge > 0){
                float move = local_rank[n] / con_edge;
                for(int e = 0; e < con_edge; ++e){
                    add[conns[e]] += move;
                }
            }
        }

        check = 0.0f;
        for(int n = sg_Sizes[sg]; n < sg_Sizes[sg + 1]; ++n){
            float new_rank = ((scale - damping) / num_node) + (damping * add[n]);
            local_rank[n] = new_rank;
            check += local_rank[n];
        }
        // if(i < 10)printf("[%d] [%d] %f\n", i, sg, check);
    }

    free(add);
}

void result_fusion(float* server_rank, float* local_rank){
    for(int sg = 0; sg < num_SGs; ++sg){
        for(int n = sg_Sizes[sg]; n < sg_Sizes[sg + 1]; ++n){
            local_rank[n] *= server_rank[sg];
        }
    }
}

float* server_PR_float(){
    float* add;
    float* server_rank;

    float scale = 1.0f;
    float damping = 0.85f;


    add = calloc(num_SGs, sizeof(float));
    server_rank = calloc(num_SGs, sizeof(float));

    float check = 0.0f;
    for(int server = 0; server < num_SGs; ++server){
        server_rank[server] = scale / num_SGs;
        check += server_rank[server];
    }
    // printf("before SCALE: %f, total: %f\n", scale, check);

    for(int i = 0; i < ITER; ++i){
        for(int sg = 0; sg < num_SGs; ++sg)
            add[sg] = 0.0f;
        for(int sg = 0; sg < num_SGs; ++sg){
            if(SGs[sg].num_ICE > 0){
                float move = server_rank[sg] / SGs[sg].num_ICE;
                for(int n = sg_Sizes[sg]; n < sg_Sizes[sg + 1]; ++n){
                    for(int e = mat.ptr[n]; e < mat.ptr[n + 1]; ++e){
                        int dst = mat.idx[e];
                        if(mat.sg_info[dst] != sg)
                            add[mat.sg_info[dst]] += move;
                    }
                }
            }
        }

        check = 0.0f;
        for(int sg = 0; sg < num_SGs; ++sg){
            float new_rank = ((scale - damping) / num_SGs) + ((damping * add[sg]));
            server_rank[sg] = new_rank;
            check += server_rank[sg];
            // printf("[%d] [%d] %f\n", i, sg, server_rank[sg]);
        }

        // printf("[%d] after SCALE: %f, total: %f\n", i, scale, check);
    }

    free(add);
    return server_rank;
}

// void refinement(float* server_rank, float* local_rank){
//     for(int sg = 0; sg < num_SGs; ++sg){
//         for(int n = sg_Sizes[sg]; n < sg_Sizes[sg + 1]; ++n){
//             for(int sg2 = 0; sg2 < num_SGs; ++sg2){
//                 if(sg2 == sg) continue;
//                 local_rank[n] += (server_rank[sg2]/server_rank[sg]) * ((float)num_ICEs_sg[n][sg2] / (float)SGs[sg2].num_ICE);
//             }
//         }
//     }
// }

void refinement(float* server_rank, float* local_rank) {
    #pragma omp parallel for num_threads(64)
    for (int sg = 0; sg < num_SGs; ++sg) {
        for (int n = sg_Sizes[sg]; n < sg_Sizes[sg + 1]; ++n) {
            float temp = 0.0f;
            for (int sg2 = 0; sg2 < num_SGs; ++sg2) {
                if (sg2 == sg) continue;
                temp += (server_rank[sg2] / server_rank[sg]) * ((float)num_ICEs_sg[n][sg2] / (float)SGs[sg2].num_ICE);
            }
            #pragma omp atomic
            local_rank[n] += temp;
        }
    }
}


void normalization(float* local_rank){
    for(int sg = 0; sg < num_SGs; ++sg){
        float local_sum = 0.0f;
        for(int n = sg_Sizes[sg]; n < sg_Sizes[sg + 1]; ++n){
            local_sum += local_rank[n];
        }
        for(int n = sg_Sizes[sg]; n < sg_Sizes[sg + 1]; ++n){
            local_rank[n] /= local_sum;
        }      
    }
}

float* global_PR(float* time) {
    float* add = malloc(mat.nr * NUM_THREAD << 2);
	float* global_rank = malloc(mat.nr << 2);
	float* rank_prev = malloc(mat.nr << 2);
	memset(rank_prev, 0, mat.nr << 2);

    float init = 1.0f / total_ICN;
    for (int i = 0; i < mat.nr; ++i) {
        if (is_ICN[i]) global_rank[i] = init;
    }
    init = SF / total_ICN;

    float cur_change = 1.0f, before_change = 1.0f;
    int iter = 0;
    for (; iter < 50 && cur_change > EPSILON && before_change > EPSILON; ++iter) {
        memset(add, 0, mat.nr << 2);
#pragma omp parallel for num_threads(NUM_THREAD)
        for (int sg = 0; sg < num_SGs; ++sg) {
            int tid = omp_get_thread_num();
            for (int icn = sg_Sizes[sg]; icn < sg_Sizes[sg] + SGs[sg].num_ICN; ++icn) {
                int node = ICNs[icn];

                if (num_ICEs[node]) {
                    float move = (global_rank[node] - init) / num_ICEs[node];
                    // for (int e = ptr[node]; e < ptr[node + 1]; ++e) {
                        // printf("%d %d\n", num_ICEs[node], nnz[node]);
                    for (int e = mat.ptr[node]; e < mat.ptr[node] + num_ICEs[node]; ++e) {
                        add[tid * mat.nr + ICEs[e]] += move;
                    }
                }
                // else {
                //     float move = (global_rank[node] - init) / (total_ICN - 1);
                //     if (move > -EPSILON && move < EPSILON) {
                //         for (int i = 0; i < mat.nr; ++i) {
                //             if (is_ICN[i] && i != node) add[i] += move;
                //         }
                //     }
                // }
            }
        }
#pragma omp parallel for num_threads(NUM_THREAD)
        for (int sg = 0; sg < num_SGs; ++sg) {
            for (int icn = sg_Sizes[sg]; icn < sg_Sizes[sg] + SGs[sg].num_ICN; ++icn) {
                int node = ICNs[icn];
                for (int j = 1; j < NUM_THREAD; ++j) {
                    add[node] += add[node + j * mat.nr];
                }
            }
        }
        cur_change = 0.0f;
        before_change = 0.0f;
        for (int i = 0; i < mat.nr; ++i) {
            if (is_ICN[i]) {
                float new_rank = init + add[i];
                cur_change += global_rank[i] - new_rank > 0 ? global_rank[i] - new_rank : new_rank - global_rank[i];
                before_change += rank_prev[i] - new_rank > 0 ? rank_prev[i] - new_rank : new_rank - rank_prev[i];
                rank_prev[i] = global_rank[i];
                global_rank[i] = init + add[i];
            }
        }
        // printf("Iteration %d: %f\n", iter, cur_change);
    }

	return global_rank;
}

void finalize_PR(float* rank, float* rank_Global) {
    // Local PageRank Refinement
    float* rank_SG = calloc(mat.nr, 4);
    float local_rank = 0.0f;
    for (int sg = 0; sg < num_SGs; ++sg) {
        for (int icn = sg_Sizes[sg]; icn < sg_Sizes[sg] + SGs[sg].num_ICN; ++icn) {
            int node = ICNs[icn];
            rank_SG[sg] += rank_Global[node];
        }
        local_rank += rank_SG[sg];
    }

    printf("local_rank org:\t %f\n", local_rank);

    for (int sg = 0; sg < num_SGs; ++sg) {
        for (int i = sg_Sizes[sg]; i < sg_Sizes[sg + 1]; ++i) {
            rank[i] = rank_SG[sg] * rank[i];
        }
    }
}