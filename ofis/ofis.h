#ifndef OFIS_H
#define OFIS_H

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <dpu.h>

/**
 * @brief resolving the (virtual) address of the target variable
 * @param set the identifier of the DPU set
 * @param init_var pointer to store the address
 * @param symbol the DPU symbol where the transfer starts
 * @return whether the initialization was successfully done
 */
dpu_error_t
OFIS_init_set_var(struct dpu_set_t set, uint32_t* init_var, const char* symbol);

/**
 * @brief Set OFIS_dpu_state in a given DPU
 * @param rank the identifier of the Rank
 * @param dpu_id target dpu's id
 * @param state value for OFIS_dpu_state
 */
dpu_error_t
OFIS_set_state_dpu(struct dpu_set_t rank, uint8_t dpu_id, uint8_t state);

/**
 * @brief Set OFIS_dpu_state in all DPUs in a given IG
 * @param rank the identifier of the Rank
 * @param ig_id target ig's id
 * @param state value for OFIS_dpu_state
 */
dpu_error_t
OFIS_set_state_ig(struct dpu_set_t rank, uint8_t ig_id,  uint8_t state);

/**
 * @brief Set OFIS_dpu_state in all DPUs in a given rank
 * @param rank the identifier of the Rank
 * @param state value for OFIS_dpu_state
 */
dpu_error_t
OFIS_set_state_rank(struct dpu_set_t rank, uint8_t state);

/**
 * @brief Return DPUs (bitmap) with OFIS_dpu_state = 1 in the rank
 * @param rank the identifier of the Rank
 * @param result pointer to store result bitmap
 * @return no. of finished DPUs
 */
uint32_t
OFIS_get_finished_dpu(struct dpu_set_t rank, uint64_t* results);

/**
 * @brief Return IGs (bitmap) with OFIS_dpu_state = 1 in the rank
 * @param rank the identifier of the Rank
 * @param result pointer to store result bitmap
 * @return no. of finished IGs
 */
uint32_t
OFIS_get_finished_ig(struct dpu_set_t rank, uint8_t* results);

/**
 * @brief Return 1 only if OFIS_dpu_state = 1 for all DPUs in the rank
 * @param rank the identifier of the Rank
 * @return whether all DPUs in the rank finished
 */
uint32_t
OFIS_get_finished_rank(struct dpu_set_t rank);

/**
 * @brief Return a virtual DPUset that contains only a specified rank
 * @param set the identifier of the DPU set
 * @param rank_id target rank's id
 * @return return a virtual DPUset
 */
struct dpu_set_t
OFIS_get_rank(struct dpu_set_t set, uint32_t rank_id);

/**
 * @brief Request the boot of all the DPUs in a DPU set without polling threads
 * @param dpu_set the identifier of the DPU set we want to boot
 */
dpu_error_t
OFIS_dpu_launch(struct dpu_set_t dpu_set);

/**
 * @brief Create and execute per-rank OFIS threads, in parallel
 * @param nr_thread number of OFIS threads
 * @param exec_func function to call
 * @param args arguments to pass to the thread function
 */
int
OFIS_parallel_exec(uint32_t nr_thread, void *(exec_func)(void*), void **args);

// dpu_error_t
// OFIS_set_mux_dpu(struct dpu_set_t rank, uint8_t dpu_id, bool dir);

/**
 * @brief Allow CPU or DPU to access MRAMs in a given IG
 * @param rank the identifier of the Rank
 * @param ig_id target ig's id
 * @param dir direction of MUX (Host or DPU)
 */
dpu_error_t
OFIS_set_mux_ig(struct dpu_set_t rank, uint8_t ig_id, bool dir);

/**
 * @brief Allow CPU or DPU to access MRAMs in a given IG
 * @param rank the identifier of the Rank
 * @param dir direction of MUX (Host or DPU)
 */
dpu_error_t
OFIS_set_mux_rank(struct dpu_set_t rank, bool dir);

/**
 * @brief Prepare a parallel transfer to/from marked DPUs in the bitmap
 * @param dpu the identifier of the DPU
 * @param bitmap bitmap for target DPUs 
 * @param buffer pointer to the host buffer
 * @return whether the given DPU is a target
 */
bool
OFIS_prepare_xfer_dpu(struct dpu_set_t dpu, uint64_t bitmap, void* buffer);

/**
 * @brief Prepare a parallel transfer to/from marked DPUs in the bitmap
 * @param dpu the identifier of the DPU
 * @param bitmap bitmap for target IGs 
 * @param buffer pointer to the host buffer
 * @return whether the given DPU is in the target IGs
 */
bool
OFIS_prepare_xfer_ig(struct dpu_set_t dpu, uint8_t bitmap, void* buffer);

#endif // DPU_H
