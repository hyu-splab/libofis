# On-the-Fly host-PIM Interaction Scheme (OFIS)
On-the-Fly host-PIM Interaction Scheme (OFIS) that enables PIM applications to use an on-demand data assignment policy, overcoming the barrier synchronization of the Bulk-Synchronous Parallel (BSP) programming model used in the current commercial PIM system (UPMEM PIM).

**libofis** provides various APIs to facilitate the development of OFIS-enabled applications.

## APIs for OFIS
libofis, currently integrated with the libdpu.so of the standard UPMEM SDK, provides various APIs to facilitate the development of OFIS-enabled PIM applications:

- APIs for Rank-unit DPU Management: In principle, OFIS manages DPUs in a DPUset in a rank unit, providing rank-unit DPU management APIs
    - `OFIS_get_rank()` returns a virtual DPUset containing only the specified rank. OFIS-specific parameters: DPUset and rank index
    - `OFIS_dpu_launch()` boots DPUs in a given rank w/o creating Polling threads. OFIS-specific parameters: Target rank (vDPUset)
    - `OFIS_parallel_exec()` creates and executes per-rank OFIS threads, in parallel. OFIS-specific parameters: Thread function and args

- DPU Monitoring APIs: read state variables (OFIS_dpu_state) of all DPUs in a given rank using the WRAM Parallel Access feature
    - `OFIS_get_finished_dpu()` returns finished DPUs (OFIS_dpu_state = 1) in a given rank in a bitmap format. OFIS-specific parameters: Target rank and bitmap
    - `OFIS_get_finished_ig()` returns finished IGs (interleaving group) in a given rank in a bitmap format. OFIS-specific parameters: Target rank and bitmap
    - `OFIS_get_finished_rank()` returns 1 only if all DPUs in a given rank have finished. Otherwise, returns 0. OFIS-specific parameters: Target rank

- APIs for DPU Binary Triggering: inform the DPU binary of the future action to take by setting the state variable (OFIS_dpu_state)
    - `OFIS_set_state_dpu()` sets the state variable in a given DPU. OFIS-specific parameters: Target DPU and value
    - `OFIS_set_state_ig()` sets the state variables of all DPUs in a given IG. OFIS-specific parameters: Target IG and value
    - `OFIS_set_state_rank()` sets the state variables of all DPUs in a given rank. OFIS-specific parameters: Target rank and value 

    DPU Binary Triggering APIs also use the WRAM Parallel Access feature.
- MUX control APIs for MRAM data transfers: set MUX bits in an IG or rank to a specific value
    - `OFIS_set_mux_ig()` allows either CPU or DPU to access MRAMs in a given IG exclusively. OFIS-specific parameters: Target IG and MUX bits
    - `OFIS_set_mux_rank()` allows either CPU or DPU to access MRAMs in a given rank exclusively. OFIS-specific parameters: Target rank and MUX bits
 
    The MUX control APIs eventually invoke the SDK-internal MUX control function after selecting target DPUs.
- APIs for Multi-granular Data Transfers: allocate buffers **only** to specified DPUs/IGs in a rank
    - `OFIS_prepare_xfer_dpu()` prepares a parallel transfer to/from marked DPUs in the bitmap. OFIS-specific parameters: Bitmap for DPUs
    - `OFIS_prepare_xfer_ig()` prepares a parallel transfer to/from marked IGs in the bitmap. OFIS-specific parameters: Bitmap for IGs

    The above parallel transfer preparation APIs are modified versions of the standard API, dpu_prepare_xfer(), so that real data buffers are allocated only to the designated DPUs, referring to the bitmaps returned by the DPU monitoring APIs.
    When the standard parallel transfer API (`dpu_push_xfer()`) is called after allocating buffers using the above APIs, a parallel data transfer is performed only to the specified DPUs or IGs in a rank. For a rank-unit data transfer, use the standard preparation API, with a target rank (vDPUset) specified.

Note) `OFIS_get_finished_dpu()`, `OFIS_set_state_dpu()`, and `OFIS_prepare_xfer_dpu()` can be used only for W-OFIS.

# Example Applications
This repository includes three OFIS-enabled PIM applications to deliver use cases of libofis:
1. **SpMV** (Sparse Matrix-Vector Multiplication): demonstrates how to use M-OFIS (OFIS with MRAM-based on-the-fly interactions)
2. **PR** (Distributed PageRank Computation): demonstrates how to use W-OFIS (OFIS with WRAM-based on-the-fly interactions)
3. **Regex** (Regular Expression Matching): demonstrates how to use M-OFIS (OFIS with MRAM-based on-the-fly interactions)

Each application provides multiple implementation versions to compare the OFIS-enabled case with OFIS-oblivious cases.
1. **SpMV**
   - SparseP-ES: OFIS-oblivious version with 2D equally-sized data partitioning
   - SparseP-EW: OFIS-oblivious version with 2D equally-wide, load-balanced data partitioning
   - SpMV-OFIS: OFIS-enabled version with 2D equally-sized data partitioning
   - CPU-only: CPU version without PIM
2. **PR**
   - PR-WRAM: OFIS-oblivious version that uses WRAM for a node information store
   - PR-OFIS: OFIS-enabled version of the PR-WRAM 
3. **Regex**
   - Regex-PIM: OFIS-oblivious version
   - Regex-OFIS: OFIS-enabled version, either a rank- or IG-unit
   - CPU-only: CPU version without PIM

## System Requirements
- UPMEM PIM DIMMs (at least ten PIM DIMMs)
- Two Intel Xeon Gold 6226R CPUs
- 256 GB DRAM
- Ubuntu 20.04.6 LTS
- UPMEM SDK ver 2024.2.0 (you can install it from `https://sdk.upmem.com/2024.2.0/01_Install.html`)
    - If there's any problem, download it from sdk/files directory in this repository
- **>= 250 GB of free disk space** for datasets

## Files
- ofis/      # **ofis.h** and **libofis.so**
- ofis-source/    # source code for libofis
- sdk/      # upmem sdk v2024.2.0
- SpMV/     # source code for SpMV applications (both OFIS-enabled and OFIS-oblivious versions)
- PageRank/ # source code for PR applications (both OFIS-enabled and OFIS-oblivious versions)
- Regex/    # source code for Regex applications (both OFIS-enabled and OFIS-oblivious versions)
- README.md # readme file for using OFIS
- download_dataset.sh    # Script file for downloading datasets for applications
- exp_all.sh    # Script file for executing all applications
- exp_e2e.sh    # Script file for End-to-End experiments
- gnuplot-script.plt    # Script file for plotting graphs (gnuplot). gnuplot must have been installed in your system.

## Install and Setup libofis
```bash
cd ofis/
cp libofis.so $UPMEM_HOME/lib
cp ofis.h $UPMEM_HOME/include/dpu

cd $UPMEM_HOME/lib
ln -sfn libofis.so libdpu.so.0.0
ln -sfn libdpu.so.0.0 libdpu.so
```
Or if you want to modify the OFIS source code,
you can find it in the ofis-source directory.
```bash
cp ofis/ofis.h $UPMEM_HOME/include/dpu
cd ofis-source/backends/
modify code in backends/api/src/api/dpu_memory.c or dpu_runner.c
./load.sh

cd $UPMEM_HOME/lib
ln -sfn libofis.so libdpu.so.0.0
ln -sfn libdpu.so.0.0 libdpu.so
```

## Setup UPMEM SDK (When use provided files)
```bash
cd sdk/files
tar -zxvf upmem-2024.2.0-Linux-x86_64.tar.gz
tar -zxvf upmem-src-2024.2.0-Linux-x86_64.tar.gz # option, when modify sdk
```

## Download Dataset for test
```bash
./download_dataset.sh
```
After download, datasets are placed under `PageRank/dataset` and `SpMV/dataset`.
The data for the Regex experiments already exists in `Regex/dataset`.
Datasets used in the PR experiments is as follows.

| Dataset | Node | Edge | Size (MB) |
|---------|------------|-------------|-----------|
| 333SP | 3,712,816 | 11,108,633 | 164 |
| AS365 | 3,799,276 | 11,368,076 | 168 |
| M6 | 3,501,777 | 10,501,936 | 154 |
| NLR | 4,163,764 | 12,487,976 | 185 |
| rgg_n_2_21_s0 | 2,097,153 | 14,487,995 | 207 |

**Note: Ensure >= 250 GB of free disk space before downloading.**

## Experiments (How to Run)
1. SpMV
```bash
cd SpMV
./spmv_test_all.sh $(num_iter) # e.g. ./spmv_test_all.sh 1
```
   Raw results:
   - `SpMV_OFIS/results/...` (IG-unit, Rank-unit)
   - `SparseP_ES/results/...`
   - `SparseP_EW/results/...`

   Organized results: `SpMV/figures/...`

2. PageRank
```bash
cd PageRank
./pg_test_all.sh $(num_iter) # e.g. ./pg_test_all.sh 1
```
   Raw results: `PageRank/results/...`
   
   Organized results: `PageRank/figures/...`

3. Regex
```bash
cd Regex
./regex_test_all.sh $(num_iter) # e.g. ./regex_test_all.sh 1
```

   Raw results: `Regex/results/...`

   Organized results: `Regex/figures/...`

3. Run All
```bash
./exp_all.sh $(num_iter) # e.g. ./exp_all.sh 1
```
   Runs all experiments and generates plots(eps) under `graphs/`

4. End to End (including Data Partitioning)
```bash
./exp_e2e.sh $(num_iter) $(E2E_mode) # e.g. ./exp_e2e.sh 1 1
```
   Runs SpMV & Regex experiments that show end-to-end results
   Raw Results:
   - `SpMV_OFIS/results/OFIS_ig512_e2e.csv`
   - `SpMV_OFIS/results/CPU_pthread.csv`
   - `SparseP_ES/results/ES_256_e2e.csv`
   - `SparseP_ES/results/ES_512_e2e.csv`
   - `SparseP_EW/results/EW_256_e2e.csv`

   Organized results: `SpMV/figures/{file}-e2e.txt`

## Generating Figures

To generate figures, uncomment the desired figure section in the gnuplot script and run:
```bash
gnuplot gnuplot-script.plt
```

By default, all figure sections are included. To generate only specific figures:
1. Comment out (add `#` at the beginning of each line) the sections you don't need
2. Uncomment (remove `#`) only the figure sections you want to generate
3. Run the gnuplot command

   
