import csv
import argparse
import os

parser = argparse.ArgumentParser(description="Process execution time CSV file.")
parser.add_argument('input_file', type=str, help="Input CSV file path")
parser.add_argument("num_iter", type=int, help="Number of iterations (not used in sorting)")
args = parser.parse_args()

dpu_sizes = [1024, 512, 256, 128, 64]

execution_times = {dpu: [] for dpu in dpu_sizes}

with open(args.input_file, 'r') as infile:
    csv_reader = csv.reader(infile)
    lines = list(csv_reader)
    
    for i in range(0, len(lines), 5):
        for j in range(5):
            if i + j < len(lines):
                dpu_size = dpu_sizes[j]
                exec_time = float(lines[i + j][1])
                execution_times[dpu_size].append(exec_time)

output_dir = './figures'

if not os.path.exists(output_dir):
    os.makedirs(output_dir)

output_file = f'{output_dir}/ofis-rank.txt'

with open(output_file, 'w') as outfile:
    outfile.write("#DPUs\texec\n")
    
    for dpu_size in [64, 128, 256, 512, 1024]:
        avg_time = sum(execution_times[dpu_size]) / len(execution_times[dpu_size])
        outfile.write(f"{dpu_size}\t{avg_time:.4f}\n")

print(f"Results saved to {output_file}")