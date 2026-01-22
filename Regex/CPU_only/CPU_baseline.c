#define _GNU_SOURCE
#include "make_DFA.h"
#include <numa.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define MAX_THREADS 64

struct timeval start, end;

int total_data_size;
int regex_rule_amount = 1;
int nr_thread = 11;

int dfa_index;
int current_dfa_idx = 0;
int current_dfa_loc = 0;

int DFA_info[58000000 / 8];
int DFA_info_size[295];

char target_data[2048][1 << 19];

int end_flag;

int state_amount;
int char_set_size;
#ifdef LOCAL_IMT
__thread int tl_index_mapping_table[128];
#endif
int index_mapping_table[128];
int acceptable_states_bitmap[300];
int *transition_table = NULL;

pthread_barrier_t data_sync_point;
pthread_barrier_t work_sync_point;

int **result = NULL;
atomic_int result_index;
long int global_result_index;

double get_time_difference(struct timeval start, struct timeval end) {
  double time_difference = (end.tv_sec - start.tv_sec) + (double)((end.tv_usec - start.tv_usec)) / 1000000;

  return time_difference;
}

int find_index(char input) {
#ifdef LOCAL_IMT
  return (tl_index_mapping_table[input] != -1) ? tl_index_mapping_table[input] : tl_index_mapping_table[127];
#else
  return (index_mapping_table[input] != -1) ? index_mapping_table[input] : index_mapping_table[127];
#endif
}

int DFA_data_setting() {
  state_amount = DFA_info[current_dfa_loc];
  char_set_size = DFA_info[current_dfa_loc + 1];

  if (state_amount == -99 && char_set_size == -99)
    return 1;

  current_dfa_loc += 2;

  memcpy(index_mapping_table, &DFA_info[current_dfa_loc], 128 * sizeof(int));
  current_dfa_loc += 128;

  int bitmap_size = ((state_amount + 31) >> 5);
  if (bitmap_size & 1)
    bitmap_size++;

  memcpy(acceptable_states_bitmap, &DFA_info[current_dfa_loc], bitmap_size * sizeof(int));
  current_dfa_loc += bitmap_size;

  free(transition_table);
  int table_size = state_amount * char_set_size;
  if (table_size & 1)
    table_size++;

  transition_table = (int *)malloc(sizeof(int) * table_size);
  if (!transition_table) {
    printf("Memory allocation failed for transition_table\n");
    return -1;
  }

  memcpy(transition_table, &DFA_info[current_dfa_loc], table_size * sizeof(int));
  current_dfa_loc += table_size;

  return 0;
}

void data_partitioning_for_check() {
  FILE* fp = fopen("../dataset/Text/text.txt", "r");
  if(fp == NULL){
    printf("file open error\n");
    exit(0);
  }

  int buffer_size = (1 << 19);
  int read_size = buffer_size;
  for(int i = 0; i < total_data_size * 2; i++){
    size_t result = fread(target_data[i], 1, read_size, fp);

    if(result == 0 && feof(fp)){
      break;
    }
  }
  fclose(fp);
}


int is_acceptable(int state_num) {
  int bitmap_arr_index = (state_num - 1) / 32;
  int bit_index = (state_num - 1) % 32;

  return (acceptable_states_bitmap[bitmap_arr_index] & (1 << bit_index));
}

void InitResult() {
  result = (int **)malloc(sizeof(int *) * regex_rule_amount);
  for (int i = 0; i < regex_rule_amount; ++i) {
    result[i] = (int *)malloc(sizeof(int) * 2147483648 * 2);
  }
}

void LoadAllDFA() {
  dfa_index = 0;
  for (int i = 0; i < regex_rule_amount; i++) {
    char DFA_info_file_name[100] = {};
    sprintf(DFA_info_file_name, "../dataset/DFA_info/DFA_%d.csv", i);
    DFA_info_size[i] = make_BUF(&DFA_info[dfa_index], DFA_info_file_name);
    if (DFA_info_size[i] < 0) {
      printf("Error loading DFA %d\n", i);
      exit(1);
    }
    dfa_index += DFA_info_size[i];
  }
  DFA_info[dfa_index++] = -99;
  DFA_info[dfa_index++] = -99;
}

int matching(int partition_num, int start_index, int start_state) {
  int current_state = start_state;
  char input;
  int bitmap_arr_index;
  int bit_index;

  for (int i = start_index; i < (1 << 19); i++) {

    if (i - start_index > (1 << 11)) {
      return 0;
    }

    if (current_state == 0) {
      return 0;
    }

    bitmap_arr_index = (current_state - 1) >> 5;
    bit_index = (current_state - 1) & 31;

    if (acceptable_states_bitmap[bitmap_arr_index] & (1 << bit_index)) {
      return i;
    }

    input = target_data[partition_num][i];
    int idx =
#ifdef LOCAL_IMT
        (tl_index_mapping_table[input] != -1) ? tl_index_mapping_table[input] : tl_index_mapping_table[127];
#else
        (index_mapping_table[input] != -1) ? index_mapping_table[input] : index_mapping_table[127];
#endif
    if (idx == -1)
      current_state = 0;
    else
      current_state = transition_table[(current_state - 1) * char_set_size + idx];
  }

  return -current_state;
}

void *worker_thread(void *arg) {
  int thread_id = *(int *)arg;

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);

#ifdef ONE_CPU
  // in pim server,
  // 0-15, 32-47 ->  0-15
  // 16-31, 48-63 -> 32-47
  int cpu_id = (thread_id & 15) | ((thread_id & 16) << 1);
  CPU_SET(cpu_id, &cpuset);
#else
  CPU_SET(thread_id, &cpuset);
#endif

  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

  int local_result_index;
  while (1) {
    if (thread_id == 0) {
      global_result_index += atomic_load(&result_index);
      atomic_store(&result_index, 0);
      end_flag = DFA_data_setting();
    }
    pthread_barrier_wait(&data_sync_point);
    if (end_flag)
      break;

#ifdef LOCAL_IMT
    memcpy(tl_index_mapping_table, index_mapping_table, sizeof(tl_index_mapping_table));
#endif

    for (int idx = thread_id; idx < total_data_size * (1 << 20); idx += nr_thread) {
      int test_num = idx / (1 << 19);
      int test_offset = idx % (1 << 19);

      int matching_result = matching(test_num, test_offset, 1);
      if (matching_result > 0) {
        local_result_index = atomic_fetch_add(&result_index, 2);
        result[current_dfa_idx][local_result_index] = idx;
        result[current_dfa_idx][local_result_index + 1] = idx + (matching_result - test_offset);
      }
    }

    pthread_barrier_wait(&work_sync_point);

    // if (thread_id == 0) {
    //   printf("iter %d/%d, nr_result: %d\n", current_dfa_idx, regex_rule_amount, result_index);
    //   current_dfa_idx++;
    // }
  }

  return NULL;
}

int main(int argc, char *argv[]) {
  if (argc < 2 || argc > 4) {
    printf("Usage: %s <data size(MB)> [DFA_NUM] [num_threads]\n", argv[0]);
    return 1;
  }

  total_data_size = atoi(argv[1]);
  if (total_data_size <= 0) {
    printf("Error: Data size must be a positive integer.\n");
    return 1;
  }

  if (argc >= 3) {
    regex_rule_amount = atoi(argv[2]);
    if (regex_rule_amount <= 0) {
      printf("Error: DFA_NUM must be a positive integer.\n");
      return 1;
    }
  }

  if (argc == 4) {
    nr_thread = atoi(argv[3]);
    if (nr_thread <= 0) {
      printf("Error: num_threads must be a positive integer.\n");
      return 1;
    }
  }

  FILE *fp = fopen("../results/CPU_baseline.csv", "a");

  InitResult();
  data_partitioning_for_check();
  LoadAllDFA();

  gettimeofday(&start, NULL);

  pthread_t threads[MAX_THREADS];
  int thread_ids[MAX_THREADS];

  if (pthread_barrier_init(&data_sync_point, NULL, nr_thread) != 0 ||
      pthread_barrier_init(&work_sync_point, NULL, nr_thread) != 0) {
    perror("Failed to initialize barriers");
    exit(1);
  }

  for (int i = 0; i < nr_thread; ++i) {
    thread_ids[i] = i;
    pthread_create(&threads[i], NULL, worker_thread, &thread_ids[i]);
  }

  for (int i = 0; i < nr_thread; ++i) {
    pthread_join(threads[i], NULL);
  }

  gettimeofday(&end, NULL);

  fprintf(fp, "%d, %d, %d, %ld, %lf\n", total_data_size, regex_rule_amount, nr_thread, global_result_index,
          get_time_difference(start, end));

  fclose(fp);

  for (int i = 0; i < regex_rule_amount; ++i) {
    free(result[i]);
  }
  free(result);
  free(transition_table);
  pthread_barrier_destroy(&data_sync_point);
  pthread_barrier_destroy(&work_sync_point);
  return 0;
}
