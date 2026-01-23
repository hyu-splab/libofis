import argparse
import os

parser = argparse.ArgumentParser(description="Create comparison table.")
args = parser.parse_args()

def read_value(filename, dpu_size=None, col_name='exec'):
    with open(filename, 'r') as f:
        lines = f.readlines()
        header = lines[0].strip().split('\t')
        col_idx = header.index(col_name)
        
        for line in lines[1:]:
            parts = line.strip().split('\t')
            if dpu_size is None: 
                return float(parts[col_idx])
            elif int(parts[0]) == dpu_size:
                return float(parts[col_idx])
    return None

ig_unit = read_value('./figures/ofis-ig.txt', dpu_size=1024, col_name='exec')
rank_unit = read_value('./figures/ofis-rank.txt', dpu_size=1024, col_name='exec')
regex_pim = read_value('./figures/non-ofis.txt', dpu_size=1024, col_name='total')
cpu = read_value('./figures/cpu.txt', dpu_size=None, col_name='exec')

output_dir = './figures'

if not os.path.exists(output_dir):
    os.makedirs(output_dir)

output_file = f'{output_dir}/combined.txt'

with open(output_file, 'w') as outfile:
    outfile.write("Type\tComputation\n")
    outfile.write(f"CPU\t{cpu:.4f}\n")
    outfile.write(f"IG\t{ig_unit:.4f}\n")
    outfile.write(f"Rank\t{rank_unit:.4f}\n")
    outfile.write(f"R-PIM\t{regex_pim:.4f}\n")


print(f"Comparison table saved to {output_file}")