#ifndef _PARAMS_H_
#define _PARAMS_H_

#include <getopt.h>
#include <unistd.h>
#include <string.h>

typedef struct Params {
    char* fileName;
    unsigned int nr_partitions;
    unsigned int nthreads;
    unsigned int max_nranks;
    unsigned int nr_dpus;
} Params;

static void usage() {
    fprintf(stderr,
            "\nUsage:  ./program [options]"
            "\n"
            "\nOptions:"
            "\n    -h        help"
            "\n    -f <F>    Input matrix file name (default=roadNet-TX.mtx)"
            "\n    -v <V>    # of vertical partitions in the matrix (default=2)"
            "\n    -n <N>    # of OpenMP threads in merge step (default=1)"
            "\n    -s <S>    max # of ranks of PIM-enabled DIMMs (default=40)"
            "\n");
}

static char *strremove(char *str, const char *sub) {
    size_t len = strlen(sub);
    if (len > 0) {
        char *p = str;
        while ((p = strstr(p, sub)) != NULL) {
            memmove(p, p + len, strlen(p + len) + 1);
        }
    }
    return str;
}

static struct Params input_params(int argc, char **argv) {
    struct Params p;

    // Set default input matrix
    char *rel_dir = "spmv/2D/RBDCSR";
    char *abs_dir = (char *) malloc(1024);
    abs_dir = getcwd(abs_dir, 1024);
    p.fileName =  strcat(strremove(abs_dir, rel_dir), (char *)"inputs/roadNet-TX.mtx");
    p.nr_partitions = 2;
    p.nthreads = 1;
    p.max_nranks = 40;

    int opt;
    while((opt = getopt(argc, argv, "h:f:v:n:s:d:")) >= 0) {
        switch(opt) {
            case 'h':
                usage();
                exit(0);
                break;
            case 'f': p.fileName      = optarg; break;
            case 'v': p.nr_partitions= atoi(optarg); break;
            case 'n': p.nthreads      = atoi(optarg); break;
            case 's': p.max_nranks    = atoi(optarg); break;
            case 'd': p.nr_dpus       = atoi(optarg); break;
            default:
                      fprintf(stderr, "\nUnrecognized option!\n");
                      usage();
                      exit(0);
        }
    }

    free(abs_dir);
    return p;
}


#endif
