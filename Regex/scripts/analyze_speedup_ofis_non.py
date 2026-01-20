import argparse
import os

parser = argparse.ArgumentParser(description="Calculate speedup from non-ofis baseline.")
parser.add_argument('non_ofis_file', type=str, help="non-ofis.txt file path")
parser.add_argument('ofis_ig_file', type=str, help="ofis-ig.txt file path")
parser.add_argument('ofis_rank_file', type=str, help="ofis-rank.txt file path")
args = parser.parse_args()

def read_data(filename, col_name):
    data = {}
    with open(filename, 'r') as f:
        lines = f.readlines()
        header = lines[0].strip().split('\t')
        col_idx = header.index(col_name)
        
        for line in lines[1:]:
            parts = line.strip().split('\t')
            dpu = int(parts[0])
            value = float(parts[col_idx])
            data[dpu] = value
    return data

non_ofis_total = read_data(args.non_ofis_file, 'total')

ofis_ig_exec = read_data(args.ofis_ig_file, 'exec')

ofis_rank_exec = read_data(args.ofis_rank_file, 'exec')

output_dir = './figures'

if not os.path.exists(output_dir):
    os.makedirs(output_dir)

output_file = f'{output_dir}/speedup.txt'

with open(output_file, 'w') as outfile:
    outfile.write("#DPUs\tig\trank\n")
    
    for dpu_size in [64, 128, 256, 512, 1024]:
        speedup_ig = non_ofis_total[dpu_size] / ofis_ig_exec[dpu_size]
        speedup_rank = non_ofis_total[dpu_size] / ofis_rank_exec[dpu_size]
        
        outfile.write(f"{dpu_size}\t{speedup_ig:.2f}\t{speedup_rank:.2f}\n")

print(f"Speedup results saved to {output_file}")