#include "../../../header/host.h"
#include "../../../header/common.h"

#include <ofis.h>
#include <pthread.h>

#define EPSILON 5.96e-8f

void initialize(int, char **);

char name[64];
csr mat;
int num_SGs;
int num_DPUs;
int* sg_Sizes;
sgInfo* SGs;
int* ICNs;
int* mapping_intra;
// int* mapping_global;
int* num_ICEs;
int* ICEs;
char* is_ICN;
argument* args;

argument_pr* dpu_args;
int mode;
uint32_t mode_exec; // mode selection bit for OFIS execution

// num_ICEs_sg[i][m] means #edge from server m to page i (in server n != m)
int** num_ICEs_sg;
float* refine_vec;
float* result_rank;

uint32_t p_count;
pthread_mutex_t p_count_mutex;

typedef struct{
    uint32_t thread_id;
    struct dpu_set_t rank;
    uint32_t max_node;
} thread_args;

#define MODE_RANK 0
#define MODE_IG 1
#define MODE_DPU 2

void* thread_fct_dpu(void* arg){
    thread_args* t_args = (thread_args*)arg;
    uint32_t thread_id = t_args->thread_id;
    uint32_t rank_id = t_args->thread_id;

    struct dpu_set_t rank = t_args->rank;
    struct dpu_set_t dpu;
    uint32_t each_dpu;
    uint32_t dpu_idx = 64 * rank_id;

    uint32_t sg_pos[64];    // arr to save SG info

    // offset setting for transfer data
    uint32_t out_size = t_args->max_node * sizeof(float);
    uint32_t ptr_size = (t_args->max_node + 2) * sizeof(uint32_t);

    DPU_FOREACH(rank, dpu, each_dpu){
        uint32_t idx = dpu_idx + each_dpu;
        DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_args + idx));
        sg_pos[each_dpu] = idx;
    }
    DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_TO_DPU, "arg", 0, sizeof(argument_pr), DPU_XFER_DEFAULT));

    DPU_FOREACH(rank, dpu, each_dpu){
        uint32_t idx = dpu_idx + each_dpu;
        DPU_ASSERT(dpu_prepare_xfer(dpu, SGs[idx].ptr));
    }
    DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_TO_DPU, "ptr", 0, ptr_size, DPU_XFER_DEFAULT));

    OFIS_dpu_launch(rank);

    uint32_t finish_count = 0;
    uint64_t finish_dpu = 0;
    while(finish_count < 64){
        uint32_t nr_finish_dpu = 0;
        do{
            nr_finish_dpu = OFIS_get_finished_dpu(rank, &finish_dpu);
        }while(nr_finish_dpu == 0);

        uint32_t change_state = -1; // OFIS_dpu_state send to DPU

        DPU_FOREACH(rank, dpu, each_dpu){
            if(OFIS_prepare_xfer_dpu(dpu, finish_dpu, result_rank + (sg_pos[each_dpu] * t_args->max_node))){
            }
        }
        DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_FROM_DPU, "rank", 0, out_size, DPU_XFER_DEFAULT));

        pthread_mutex_lock(&p_count_mutex);
        if(p_count >= num_SGs){
            pthread_mutex_unlock(&p_count_mutex);
            change_state = 0;
            finish_count += nr_finish_dpu;
        }else{
            uint32_t curr_p_count = p_count;
            p_count += nr_finish_dpu;
            pthread_mutex_unlock(&p_count_mutex);

            change_state = 2;

            uint32_t part_idx = curr_p_count;
            DPU_FOREACH(rank, dpu, each_dpu){
                if(part_idx >= num_SGs) continue;
                if(OFIS_prepare_xfer_dpu(dpu, finish_dpu, dpu_args + part_idx)){
                    sg_pos[each_dpu] = part_idx;
                    part_idx++;
                }
            }
            DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_TO_DPU, "arg", 0, sizeof(argument_pr), DPU_XFER_DEFAULT));

            DPU_FOREACH(rank, dpu, each_dpu){
                uint32_t idx = sg_pos[each_dpu];
                if(idx >= num_SGs) continue;
                OFIS_prepare_xfer_dpu(dpu, finish_dpu, SGs[idx].ptr);
            }
            DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_TO_DPU, "ptr", 0, ptr_size, DPU_XFER_DEFAULT));
        }
        for(int i = 0; i < 64; ++i){
            if(finish_dpu & (1ULL << i)){   // must use 1ULL instead of 1 (because of type of 1: int32_t (buffer overflow can be caused))
                OFIS_set_state_dpu(rank, i, change_state);
            }
        }
    }
    return NULL;
}

void* thread_fct_ig(void* arg){
    thread_args* t_args = (thread_args*)arg;
    uint32_t thread_id = t_args->thread_id;
    uint32_t rank_id = t_args->thread_id;

    struct dpu_set_t rank = t_args->rank;
    struct dpu_set_t dpu;
    uint32_t each_dpu;
    uint32_t dpu_idx = 64 * rank_id;

    uint32_t sg_pos[64];    // arr to save SG info

    // offset setting for transfer data
    uint32_t out_size = t_args->max_node * sizeof(float);
    uint32_t ptr_size = (t_args->max_node + 2) * sizeof(uint32_t);

    DPU_FOREACH(rank, dpu, each_dpu){
        uint32_t idx = dpu_idx + each_dpu;
        DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_args + idx));
        sg_pos[each_dpu] = idx;
    }
    DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_TO_DPU, "arg", 0, sizeof(argument_pr), DPU_XFER_DEFAULT));

    DPU_FOREACH(rank, dpu, each_dpu){
        uint32_t idx = dpu_idx + each_dpu;
        DPU_ASSERT(dpu_prepare_xfer(dpu, SGs[idx].ptr));
    }
    DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_TO_DPU, "ptr", 0, ptr_size, DPU_XFER_DEFAULT));

    OFIS_dpu_launch(rank);

    uint32_t finish_count = 0;
    uint8_t finish_ig;
    while(finish_count < 64){
        uint32_t nr_finish_ig = 0;
        do{
            nr_finish_ig = OFIS_get_finished_ig(rank, &finish_ig);
        }while(nr_finish_ig == 0);

        uint32_t change_state = -1; // OFIS_dpu_state send to DPU

        DPU_FOREACH(rank, dpu, each_dpu){
            OFIS_prepare_xfer_ig(dpu, finish_ig, result_rank + (sg_pos[each_dpu] * t_args->max_node));
        }
        DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_FROM_DPU, "rank", 0, out_size, DPU_XFER_DEFAULT));

        pthread_mutex_lock(&p_count_mutex);
        if(p_count >= num_SGs){
            pthread_mutex_unlock(&p_count_mutex);
            change_state = 0;
            finish_count += 8 * nr_finish_ig;
        }else{
            uint32_t curr_p_count = p_count;
            p_count += 8 * nr_finish_ig;
            pthread_mutex_unlock(&p_count_mutex);

            change_state = 2;

            uint32_t part_idx = curr_p_count;
            DPU_FOREACH(rank, dpu, each_dpu){
                if(OFIS_prepare_xfer_ig(dpu, finish_ig, dpu_args + part_idx)){
                    sg_pos[each_dpu] = part_idx;
                    part_idx++;
                }
            }
            DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_TO_DPU, "arg", 0, sizeof(argument_pr), DPU_XFER_DEFAULT));

            DPU_FOREACH(rank, dpu, each_dpu){
                uint32_t idx = sg_pos[each_dpu];
                OFIS_prepare_xfer_ig(dpu, finish_ig, SGs[idx].ptr);
            }
            DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_TO_DPU, "ptr", 0, ptr_size, DPU_XFER_DEFAULT));
        }
        for(int i = 0; i < 8; ++i){
            if(finish_ig & (1 << i)){
                OFIS_set_state_ig(rank, i, change_state);
            }
        }
    }
    return NULL;
}

void* thread_fct_rank(void* arg){
    thread_args* t_args = (thread_args*)arg;
    uint32_t thread_id = t_args->thread_id;
    uint32_t rank_id = t_args->thread_id;

    struct dpu_set_t rank = t_args->rank;
    struct dpu_set_t dpu;
    uint32_t each_dpu;
    uint32_t dpu_idx = 64 * rank_id;

    uint32_t sg_pos[64];    // arr to save SG info

    // offset setting for transfer data
    uint32_t out_size = t_args->max_node * sizeof(float);
    uint32_t ptr_size = (t_args->max_node + 2) * sizeof(uint32_t);

    DPU_FOREACH(rank, dpu, each_dpu){
        uint32_t idx = dpu_idx + each_dpu;
        DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_args + idx));
        sg_pos[each_dpu] = idx;
    }
    DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_TO_DPU, "arg", 0, sizeof(argument_pr), DPU_XFER_DEFAULT));

    DPU_FOREACH(rank, dpu, each_dpu){
        uint32_t idx = dpu_idx + each_dpu;
        DPU_ASSERT(dpu_prepare_xfer(dpu, SGs[idx].ptr));
    }
    DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_TO_DPU, "ptr", 0, ptr_size, DPU_XFER_DEFAULT));

    OFIS_dpu_launch(rank);

    uint32_t finish_count = 0;

    while(finish_count < 64){
        uint32_t is_rank_finish = 0;
        do{
            is_rank_finish = OFIS_get_finished_rank(rank);
        }while(is_rank_finish == 0);

        uint32_t change_state = -1; // OFIS_dpu_state send to DPU

        DPU_FOREACH(rank, dpu, each_dpu){
            DPU_ASSERT(dpu_prepare_xfer(dpu, result_rank + (sg_pos[each_dpu] * t_args->max_node)));
        }
        DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_FROM_DPU, "rank", 0, out_size, DPU_XFER_DEFAULT));

        pthread_mutex_lock(&p_count_mutex);
        if(p_count >= num_SGs){
            pthread_mutex_unlock(&p_count_mutex);
            change_state = 0;
            finish_count += 64;
        }else{
            uint32_t curr_p_count = p_count;
            p_count += 64;
            pthread_mutex_unlock(&p_count_mutex);

            change_state = 2;

            DPU_FOREACH(rank, dpu, each_dpu){
                uint32_t idx = curr_p_count + each_dpu;
                DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_args + idx));
                sg_pos[each_dpu] = idx;
            }
            DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_TO_DPU, "arg", 0, sizeof(argument_pr), DPU_XFER_DEFAULT));

            DPU_FOREACH(rank, dpu, each_dpu){
                uint32_t idx = sg_pos[each_dpu];
                DPU_ASSERT(dpu_prepare_xfer(dpu, SGs[idx].ptr));
            }
            DPU_ASSERT(dpu_push_xfer(rank, DPU_XFER_TO_DPU, "ptr", 0, ptr_size, DPU_XFER_DEFAULT));
        }
        OFIS_set_state_rank(rank, change_state);
    }
    return NULL;
}

int main(int argc, char **argv) {
    // Clear cache
	// pid_t pid = fork();
	// if (pid == 0) execlp("sh", "sh", "-c", "echo 3 > /proc/sys/vm/drop_caches", NULL);
	// else wait(NULL);

    // Initialize and read input
    initialize(argc, argv);
    read_CSR(name);

    timespec st, ed;
    float time_dpu_alloc, time_analyze, time_h2d, time_kernel, time_host, time_d2h, time_global, time_CPU;

    // Allocate DPUs
    dpu_set set, dpu;
    uint32_t i;
    cur_Time(&st);
    DPU_ASSERT(dpu_alloc(num_DPUs, "backend=hw", &set));
    DPU_ASSERT(dpu_load(set, "./bin/pr_ofis_kernel", NULL));
    cur_Time(&ed);
    time_dpu_alloc = elapsed_Time(st, ed);

    // Analyze the structure of ICNs
    mapping_ICNs();

    // Analyze hyperlink between ICNs
    analyze_sg();
    float* server_rank = server_PR_float();
    refine_vec = calloc(mat.nr, sizeof(float));
    refinement(server_rank, refine_vec);

    // MRAM transfer
    // offset setting for transfer data
    uint32_t refine_size = ALIGN2(mat.nr) * sizeof(float);
    uint32_t idx_size = ALIGN2(mat.ne) * sizeof(uint32_t);
    // To transfer to MRAM
    uint32_t refine_base = 0;
    uint32_t idx_base = refine_base + refine_size;

    cur_Time(&st);
    DPU_ASSERT(dpu_broadcast_to(set, DPU_MRAM_HEAP_POINTER_NAME, refine_base, refine_vec, refine_size, DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_broadcast_to(set, DPU_MRAM_HEAP_POINTER_NAME, idx_base, mat.idx, idx_size, DPU_XFER_DEFAULT));
    cur_Time(&ed);
    time_h2d = elapsed_Time(st, ed);

    // Argument setting
    uint32_t max_node, max_icn;
    uint32_t max_edge;
    dpu_args = malloc(num_SGs * sizeof(argument_pr));
    set_Argument_PR(dpu_args, &max_node, &max_icn, &max_edge);

    for(int i = 0; i < num_SGs; ++i){
        dpu_args[i].serv_rank = server_rank[i];
    }

    printf("max node: %d, max_edge: %d\n", max_node, max_edge);

    uint32_t out_size = max_node * sizeof(float);
    result_rank = malloc(out_size * num_SGs);

    pthread_mutex_init(&p_count_mutex, NULL);
    p_count = num_DPUs;

    uint32_t nr_thread = num_DPUs / 64;
    thread_args* t_args[nr_thread];
    for(int i = 0; i < nr_thread; ++i){
        t_args[i] = (thread_args*)malloc(sizeof(thread_args));
        t_args[i]->thread_id = i;
        t_args[i]->max_node = max_node;
        t_args[i]->rank = OFIS_get_rank(set, i);
    }

    cur_Time(&st);
    switch (mode_exec)
    {
    case MODE_RANK:
        OFIS_parallel_exec(nr_thread, thread_fct_rank, (void**)t_args);
        break;
    case MODE_IG:
        OFIS_parallel_exec(nr_thread, thread_fct_ig, (void**)t_args);
        break;
    case MODE_DPU:
        OFIS_parallel_exec(nr_thread, thread_fct_dpu, (void**)t_args);
        break;
    default:
        break;
    }
    cur_Time(&ed);
    time_kernel = elapsed_Time(st, ed);
    dpu_free(set);

    float* page_rank_dpu = calloc(mat.nr, sizeof(float));
    int n_idx = 0;
    for(int sg = 0; sg < num_SGs; ++sg){
        for(int n = 0; n < SGs[sg].num_node; ++n){
            page_rank_dpu[n_idx++] = result_rank[sg * max_node + n];
        }
    }
    free(result_rank);

    // host version
    cur_Time(&st);
    // 1. Calc local PR
    float* local_rank = local_PR_float();
    // 2. Refinement
    for(int i = 0; i < mat.nr; ++i){
        local_rank[i] += refine_vec[i];
    }
    // 3. Normalization
    normalization(local_rank);
    // 4. Single PR for high accuracy
    local_PR_single_float(local_rank);
    // 5. Result Fusion
    result_fusion(server_rank, local_rank);
    cur_Time(&ed);
    time_CPU = elapsed_Time(st, ed);

    float check_dpu = 0.0f, check_host = 0.0f;

    float sg_sum_dpu = 0.0f, sg_sum_host = 0.0f;
    float temp_dpu = 0.0f, temp_host = 0.0f;
    for(int sg = 0; sg < num_SGs; ++sg){
        float sum_dpu = 0.0f;
        float sum_host = 0.0f;
        for(int n = sg_Sizes[sg]; n < sg_Sizes[sg + 1]; ++n){
            check_dpu += page_rank_dpu[n];
            check_host += local_rank[n];

            sum_host += local_rank[n];
            sum_dpu += page_rank_dpu[n];
        }
        // printf("[%d] DPU: %lf\tHOST: %lf\n", sg, sum_dpu, sum_host);

        sg_sum_dpu = check_dpu - temp_dpu;
        temp_dpu = check_dpu;

        sg_sum_host = check_host - temp_host;
        temp_host = check_host;
        // printf("[%d, %d] %f %f\n", sg, args[sg].num_node, sg_sum_dpu, sg_sum_host);
    }

    if(fabs(check_host - check_dpu) <= 1e-6) printf("Result:\t\t \033[34mCorrect\033[0m\n");
    else printf("Result:\t\t \033[31mIncorrect\033[0m\n");

    // DEBUG
    // printf("only host ver\t%f\n", check_host);
    // printf("dpu ver:\t%f\n", check_dpu);

    puts("\n  -- Execution Time Log");
    // printf("\tAllocate %d DPUs:\t%.2f ms\n", num_SGs, time_dpu_alloc);
    // printf("\tSubgraph Analysis(CPU):\t%.2f ms\n", time_CPU);
    // printf("\tSubgraph Analysis(PIM):\t%.2f ms\n", time_kernel);
    printf("\tTotal Execution(PIM):\t%.2f ms\n\n",time_kernel);
    printf("===================================================\n");
    
    FILE* fp;
    if(mode_exec == MODE_RANK)
        fp = fopen("result/ofis_rank.csv", "a");
    else if(mode_exec == MODE_IG)
        fp = fopen("result/ofis_ig.csv", "a");
    else if(mode_exec == MODE_DPU)
        fp = fopen("result/ofis_dpu.csv", "a");
    fprintf(fp, "%d, %.2f\n", num_SGs, time_kernel);
    fclose(fp);
    
    free(mat.nnz);
    free(mat.ptr);
    free(mat.idx);
    free(mat.val);
    free(mat.sg_info);
    free(sg_Sizes);
    free(SGs);
	free(dpu_args);
    free(server_rank);
    free(refine_vec);
    free(local_rank);
    free(page_rank_dpu);

    return 0;
}

void initialize(int argc, char **argv) {
    if (argc == 2) {
        strcpy(name, argv[1]);
        num_SGs = 256;
        num_DPUs = 256;
        mode = 1;
        mode_exec = 0;  // default mode: OFIS_rank
    }
    else if (argc == 5) {
        strcpy(name, argv[1]);
        num_SGs = atoi(argv[2]);
        num_DPUs = atoi(argv[3]);
        mode_exec = atoi(argv[4]);
        mode = 1;
    }
    else if (argc == 6) {
        strcpy(name, argv[1]);
        num_SGs = atoi(argv[2]);
        num_DPUs = atoi(argv[3]);
        mode_exec = atoi(argv[4]);
        mode = atoi(argv[5]);
    }
    else {
        puts("\nHow to Use:\n1) ./bin/bc input_data");
        puts("2) ./bin/bc input_data num_SGs");
        puts("3) ./bin/bc input_data num_SGs num_DPUs\n");
        puts("4) ./bin/bc input_data num_SGs num_DPUs ofis_mode\n");
        puts("5) ./bin/bc input_data num_SGs num_DPUs ofis_mode mode\n");
        puts("mode = 0: Serial Execution");
        puts("       1: Parallel Execution (Default)\n");

        exit(0);
    }
}

