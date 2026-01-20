import os
import numpy as np
import csv
import argparse

parser = argparse.ArgumentParser(description="Process input file.")
parser.add_argument('input_file', type=str, help="Input file path")
parser.add_argument("num_iter", type=int, help="Number of iterations (not used in sorting)")
args = parser.parse_args()

input_file = args.input_file
num_iter = args.num_iter

data = []
with open(input_file, 'r') as infile:
    csv_reader = csv.reader(infile)
    for row in csv_reader:
        data.append([float(x) for x in row])

def calculate_group_averages(group):
    averages = []
    for col in range(len(group[0])): 
        col_values = [row[col] for row in group]
        avg = np.mean(col_values)  
        averages.append(round(avg, 4)) 
    return averages

final_output = []

for i in range(0, len(data), num_iter):
    group = data[i:i+num_iter] 
    group_averages = calculate_group_averages(group)
    final_output.append(group_averages)

output_dir = './figures'

if not os.path.exists(output_dir):
    os.makedirs(output_dir)

if 'wram' in input_file:
    output_file = f'{output_dir}/wram.txt'
elif 'mram' in input_file:
    output_file = f'{output_dir}/mram.txt'
else:
    output_file = f'{output_dir}/output.txt' 

with open(output_file, 'w') as outfile:
    outfile.write("#SGs\t4096\t5120\t6144\t7168\t8192\t9216\t10240\n")
    
    for file_idx in range(0,5):
        if file_idx==0: outfile.write('333SP\t')
        elif file_idx==1: outfile.write('AS365\t')
        elif file_idx==2: outfile.write('M6\t')
        elif file_idx==3: outfile.write('NLR\t')
        else: outfile.write('rgg_n\t')

        for row in final_output[file_idx * 7 : (file_idx + 1) * 7]:
            outfile.write(str('%.2f'%row[3]) + "\t")
        outfile.write('\n')

print(f"{output_file} saved")
