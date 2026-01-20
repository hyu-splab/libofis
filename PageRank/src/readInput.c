/** 
 * Read .mtx input file and convert to CSR format
 * Args: {fileName1, fileName2, mat->ptr, mat->idx, mat->val flags, #rows, #elements}
 * Read whole matrix and sort with from data.
 * Divide Matrix A to small blocks based of number of elements.
 * Convert each block to CSR format then let main process to compute them.
 */

#include "../header/common.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void read_CSR(const char *input_File) {
    char input[64];
    char target[8];
    sprintf(target, "%d", num_SGs);
    strcpy(input, input_File);
//    strcat(input, ".");
//    strcat(input, target);
    assert(access(input, F_OK) >= 0);

    FILE *fp;
    fp = fopen(input, "rb");
    int row, pooled_row, edge, pooled_edge;
    fread(&row, sizeof(int), 1, fp);
    fread(&pooled_row, sizeof(int), 1, fp);
    fread(&edge, sizeof(int), 1, fp);
    fread(&pooled_edge, sizeof(int), 1, fp);
    mat.nr = row;
    mat.ne = edge;

    // mat.ptr = (int*)malloc((((mat.nr + 1) >> 1) + 1) << 4);
    // mat.idx = (int*)malloc(((mat.ne >> 1) + 1) << 4);
    mat.nnz = (int*)malloc(pooled_row << 2);
    mat.ptr = (int*)malloc(pooled_row << 2);
    mat.idx = (int*)malloc(pooled_edge << 2);
    mat.sg_info = malloc(mat.nr * sizeof(int));
    sg_Sizes = malloc((num_SGs + 1) << 2);
    is_ICN = malloc(pooled_row);

    fread(mat.nnz, sizeof(int), mat.nr, fp);
    fread(mat.ptr, sizeof(int), mat.nr + 1, fp);
    fread(mat.idx, sizeof(int), mat.ne, fp);
    fread(sg_Sizes, sizeof(int), num_SGs + 1, fp);
    fread(is_ICN, 1, mat.nr, fp);
    fclose(fp);

    printf("\n  -- Input File: %s\n\tNumber of Nodes:\t%d\n\tNumber of Edges:\t%d\n\n", input_File, mat.nr, mat.ne);

    int node_offset = 0;
    int edge_offset = 0;
    SGs = malloc(num_SGs * sizeof(sgInfo));
    for (int i = 0; i < num_SGs; ++i) {
        int num_node = sg_Sizes[i + 1] - sg_Sizes[i];

        SGs[i].num_node = num_node;
        SGs[i].num_edge = mat.ptr[node_offset + num_node] - mat.ptr[node_offset]; 
        SGs[i].num_ICN = 0;
        SGs[i].ptr = mat.ptr + node_offset;
        SGs[i].idx = mat.idx + edge_offset;
        SGs[i].val = mat.val + edge_offset;
        node_offset += num_node;
        edge_offset = mat.ptr[node_offset];
    }

    int cur_SG = 0;
    for (int n = 0; n < row; ++n) {
        if (n == sg_Sizes[cur_SG + 1]) cur_SG++;
        mat.sg_info[n] = cur_SG;
    }

    ICNs = malloc(pooled_row << 2);
    num_ICEs = malloc(row << 2);
    ICEs = malloc(edge << 2);
    memset(ICEs, 0xff, edge << 2);
    mapping_intra = malloc(row << 2);
    memset(mapping_intra, 0xff, row << 2);
    // mapping_global = malloc(row << 2);
    // memset(mapping_global, 0xff, row << 2);
}

