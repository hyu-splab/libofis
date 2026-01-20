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

num_files = len(final_output)

a_to_value = {1: 100, 2: 200, 3: 400, 4: 600, 5: 800, 6: 1000}

output_dir = './figures'

if not os.path.exists(output_dir):
    os.makedirs(output_dir)

for file_index in range(num_files):
    a_value = (file_index % 6) + 1 

    if 'CPU_pthread' in input_file:
        output_file = f'{output_dir}/cpu-{a_to_value[a_value]}.txt'

    with open(output_file, 'w') as outfile:
        outfile.write("#Threads\texec\n")
        
        for row in final_output[file_index * num_iter : (file_index + 1) * num_iter]:
            # a, c, e, d, b, g - order rearrange
            reordered_row = [int(row[0]), "{:.4f}".format(row[1])]
            
            outfile.write('\t'.join(map(str, reordered_row)) + "\n")

    print(f"{output_file} saved")
