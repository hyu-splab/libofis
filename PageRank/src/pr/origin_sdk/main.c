#include "../../../header/host.h"
#include "../../../header/common.h"

#define EPSILON 5.96e-8f

void initialize(int, char **);

char name[64];
csr mat;
int num_SGs;
int* sg_Sizes;
sgInfo* SGs;
int* ICNs;
int* mapping_intra;
// int* mapping_global;
int* num_ICEs;
int* ICEs;
char* is_ICN;
argument* args;
int mode;

// num_ICEs_sg[i][m] means #edge from server m to page i (in server n != m)
int** num_ICEs_sg;
int num_DPUs;

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
    DPU_ASSERT(dpu_load(set, "./bin/pr_mram_kernel", NULL));
    cur_Time(&ed);
    time_dpu_alloc = elapsed_Time(st, ed);

    // Analyze the structure of ICNs
    mapping_ICNs();

    // Analyze hyperlink between ICNs
    analyze_sg();
    float* server_rank = server_PR_float();
    float* refine_vec = calloc(mat.nr, sizeof(float));

    refinement(server_rank, refine_vec);

    uint32_t max_node, max_icn;
    uint32_t max_edge;
    argument_pr* dpu_args = malloc(num_SGs * sizeof(argument_pr));
    set_Argument_PR(dpu_args, &max_node, &max_icn, &max_edge);
    
    for(int i = 0; i < num_SGs; ++i){
        dpu_args[i].serv_rank = server_rank[i];
    }

    printf("max node: %d, max_edge: %d\n", max_node, max_edge);

    // offset setting for transfer data
    uint32_t refine_size = ALIGN2(mat.nr) * sizeof(float);
    uint32_t out_size = max_node * sizeof(float);
    uint32_t ptr_size = (max_node + 2) * sizeof(uint32_t);
    uint32_t idx_size = ALIGN2(mat.ne) * sizeof(uint32_t);

    uint32_t refine_base = 0;
    uint32_t out_base = refine_base + refine_size;
    uint32_t ptr_base = out_base + out_size;
    uint32_t idx_base = ptr_base + ptr_size;

    // Clean MRAM
    float* buffer = calloc(max_node * num_SGs, sizeof(float));
    dpu_broadcast_to(set, DPU_MRAM_HEAP_POINTER_NAME, out_base, buffer, out_size, DPU_XFER_DEFAULT);

    uint32_t p_count = 0;
    float time_h2d_first = 0;
    time_h2d = 0;
    time_kernel = 0;
    time_d2h = 0;
    cur_Time(&st);
    DPU_ASSERT(dpu_broadcast_to(set, DPU_MRAM_HEAP_POINTER_NAME, refine_base, refine_vec, refine_size, DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_broadcast_to(set, DPU_MRAM_HEAP_POINTER_NAME, idx_base, mat.idx, idx_size, DPU_XFER_DEFAULT));
    cur_Time(&ed);
    time_h2d_first += elapsed_Time(st, ed);

    float* result_rank = malloc(out_size * num_SGs);
    while(p_count < num_SGs){
        cur_Time(&st);
        DPU_FOREACH(set, dpu, i) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_args + p_count + i));
        }
        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "arg", 0, sizeof(argument_pr), DPU_XFER_DEFAULT));

        DPU_FOREACH(set, dpu, i) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, SGs[p_count + i].ptr));
        }
        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, ptr_base, ptr_size, DPU_XFER_DEFAULT));
        
        cur_Time(&ed);
        time_h2d += elapsed_Time(st, ed);

        // Kernel Execution
        cur_Time(&st);
        DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));
        cur_Time(&ed);
        time_kernel += elapsed_Time(st, ed);
        // puts("DPU Finished");
        
        cur_Time(&st);
        DPU_FOREACH(set, dpu, i) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, result_rank + ((p_count + i) * max_node)));
        }
        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_FROM_DPU, "rank", 0, out_size, DPU_XFER_DEFAULT));
        cur_Time(&ed);
        time_d2h += elapsed_Time(st, ed);

        p_count += num_DPUs;
    }    
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
    printf("\tH-to-D Transfer:\t%.2f ms\n", time_h2d);
    printf("\tSubgraph Analysis(PIM):\t%.2f ms\n", time_kernel);
    printf("\tD-to-H Transfer:\t%.2f ms\n", time_d2h);
    // printf("\tTotal Execution(CPU):\t%.2f ms\n\n", time_CPU);
    printf("\tTotal Execution(PIM):\t%.2f ms\n\n", time_h2d + time_kernel + time_d2h);
    printf("===================================================\n");

    FILE *fp = fopen("result/mram.csv", "a");
    fprintf(fp, "%d, %.2f, %.2f, %.2f\n", num_SGs, time_h2d, time_d2h, time_h2d + time_d2h + time_kernel);
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
    }
    else if (argc == 4) {
        strcpy(name, argv[1]);
        num_SGs = atoi(argv[2]);
        num_DPUs = atoi(argv[3]);
        mode = 1;
    }
    else if (argc == 5) {
        strcpy(name, argv[1]);
        num_SGs = atoi(argv[2]);
        num_DPUs = atoi(argv[3]);
        mode = atoi(argv[4]);
    }
    else {
        puts("\nHow to Use:\n1) ./bin/bc input_data");
        puts("2) ./bin/bc input_data num_SGs");
        puts("3) ./bin/bc input_data num_SGs num_DPUs\n");
        puts("4) ./bin/bc input_data num_SGs num_DPUs mode\n");
        puts("mode = 0: Serial Execution");
        puts("       1: Parallel Execution (Default)\n");

        exit(0);
    }
}

