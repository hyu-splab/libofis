#ifndef __STDINT_H
    #define __STDINT_H
    #include <stdint.h>
#endif
#include <assert.h>

#ifndef NR_TASKLETS
    #define NR_TASKLETS 11
#endif


#define ALIGN8(x) ((((x) + 7) >> 3) << 3)
#define ALIGN4(x) ((((x) + 3) >> 2) << 2)
#define ALIGN2(x) ((((x) + 1) >> 1) << 1)
#define MUL4_ALIGN8(x) ((((x) + 1) >> 1) << 3)
#define MUL2_ALIGN8(x) ((((x) + 3) >> 2) << 3)
#define ADDR_ALIGN8(x) (((x) >> 3) << 3)

#define MAX(a, b) (a > b ? a : b)

typedef struct dpu_set_t dpu_set;

struct edgeData {
    int from;
    int to;
    int val;
};
typedef struct edgeData edgeData;

struct csr {
    int *nnz;
    int *ptr;
    int *idx;
    int *val;
    int *sg_info;
    int nr;
    int nc;
    int ne;
};
typedef struct csr csr;

struct sgInfo {
    int *ptr;
    int *idx;
    int *val;
    int num_node;
    int num_edge;
    int num_ICN;
    int num_ICE;
};
typedef struct sgInfo sgInfo;

struct argument {
    uint32_t start_node;
    uint32_t max_edge;
    uint32_t max_node;
    uint32_t max_icn;
    uint32_t num_node;
    uint32_t num_icn;
};
typedef struct argument argument;

struct argument_pr {
    uint32_t start_node;
    uint32_t max_node;
    uint32_t num_node;
    uint32_t iter;
    uint32_t tot_node;
    float serv_rank;
};
typedef struct argument_pr argument_pr;

void read_CSR(const char *input_File);
void set_Argument(argument* args, uint32_t* max_node, uint32_t* max_icn, uint32_t* max_edge);
void mapping_ICNs();
void analyze_ICNs();
void analyze_ICNs2();
int analyze_ICEs();
void set_Argument_PR(argument_pr* args, uint32_t* max_node, uint32_t* max_icn, uint32_t* max_edge);

extern argument *args;
extern csr mat;
extern int num_SGs;
extern int* sg_Sizes;
extern sgInfo* SGs;
extern int* ICNs;
extern int* mapping_intra;
extern int* num_ICEs;
extern int* ICEs;
extern char* is_ICN;
extern int total_ICN;

extern int** num_ICEs_sg;
