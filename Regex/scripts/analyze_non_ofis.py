import csv
import argparse
import os

parser = argparse.ArgumentParser(description="Process execution time CSV file.")
parser.add_argument('input_file', type=str, help="Input CSV file path")
parser.add_argument("num_iter", type=int, help="Number of iterations")
args = parser.parse_args()

dpu_sizes = [1024, 512, 256, 128, 64]

execution_times = {dpu: {'transfer': [], 'exec': [], 'retrieve': [], 'postprocessing': [], 'total': []} for dpu in dpu_sizes}

with open(args.input_file, 'r') as infile:
    csv_reader = csv.reader(infile)
    lines = list(csv_reader)
    
    for i in range(0, len(lines), 30):
        for dpu_idx in range(5):
            dpu_size = dpu_sizes[dpu_idx]
            start_line = i + (dpu_idx * 6)
            
            if start_line + 5 < len(lines):
                alloc_time = float(lines[start_line][1])
                load_time = float(lines[start_line + 1][1])
                copy_in_time = float(lines[start_line + 2][1])   # transfer
                run_time = float(lines[start_line + 3][1])        # exec
                copy_out_time = float(lines[start_line + 4][1])   # retrieve
                merge_time = float(lines[start_line + 5][1])      # postprocessing
                
                total_time = alloc_time + load_time + copy_in_time + run_time + copy_out_time + merge_time
                
                execution_times[dpu_size]['transfer'].append(copy_in_time)
                execution_times[dpu_size]['exec'].append(run_time)
                execution_times[dpu_size]['retrieve'].append(copy_out_time)
                execution_times[dpu_size]['postprocessing'].append(merge_time)
                execution_times[dpu_size]['total'].append(total_time)

output_dir = './figures'

if not os.path.exists(output_dir):
    os.makedirs(output_dir)

output_file = f'{output_dir}/non-ofis.txt'

with open(output_file, 'w') as outfile:
    outfile.write("#DPUs\ttransfer\texec\tretrieve\tpostprocessing\ttotal\n")
    
    for dpu_size in [64, 128, 256, 512, 1024]:
        avg_transfer = sum(execution_times[dpu_size]['transfer']) / len(execution_times[dpu_size]['transfer'])
        avg_exec = sum(execution_times[dpu_size]['exec']) / len(execution_times[dpu_size]['exec'])
        avg_retrieve = sum(execution_times[dpu_size]['retrieve']) / len(execution_times[dpu_size]['retrieve'])
        avg_postprocessing = sum(execution_times[dpu_size]['postprocessing']) / len(execution_times[dpu_size]['postprocessing'])
        avg_total = sum(execution_times[dpu_size]['total']) / len(execution_times[dpu_size]['total'])
        
        outfile.write(f"{dpu_size}\t{avg_transfer:.4f}\t{avg_exec:.4f}\t{avg_retrieve:.4f}\t{avg_postprocessing:.4f}\t{avg_total:.4f}\n")

print(f"Results saved to {output_file}")