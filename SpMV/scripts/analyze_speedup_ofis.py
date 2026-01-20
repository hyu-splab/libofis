import os
import numpy as np
import csv

speed_values = [100, 200, 400, 600, 800, 1000]

dpus = [64, 128, 256, 512, 1024]

def read_total_time(file_path):
    total_times = []
    with open(file_path, 'r') as infile:
        csv_reader = csv.reader(infile, delimiter='\t')
        next(csv_reader)
        for row in csv_reader:
            total_times.append(float(row[5]))
    return total_times

speedup_results = {value: [] for value in speed_values}

for value in speed_values:
    input_file_ig = f'./figures/ofis512-ig-{value}.txt'
    input_file_rank = f'./figures/ofis512-rank-{value}.txt'
    
    ofis_ig_data = read_total_time(input_file_ig)
    ofis_rank_data = read_total_time(input_file_rank)

    for i in range(len(dpus)):
        rank_time = ofis_rank_data[i]
        ig_time = ofis_ig_data[i]
        
        speedup = round(rank_time / ig_time, 2)
        speedup_results[value].append(speedup)

header = ["#DPUs"] + [str(value) for value in speed_values]

output_file = './figures/speedup-ofis-rank-chip.txt'

with open(output_file, 'w') as outfile:
    outfile.write('\t'.join(header) + '\n')
    
    for i, dpus_value in enumerate(dpus):
        row = [str(dpus_value)] + [str(speedup_results[value][i]) for value in speed_values]
        outfile.write('\t'.join(row) + '\n')

print(f"{output_file} saved")
