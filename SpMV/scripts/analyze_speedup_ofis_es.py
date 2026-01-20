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

def process_files(a_type, b_type, output_file):
    speedup_results = {value: [] for value in speed_values}
    
    for value in speed_values:
        input_file_a = f'./figures/{a_type}-{value}.txt'
        input_file_b = f'./figures/{b_type}-{value}.txt'
        
        data_a = read_total_time(input_file_a)
        data_b = read_total_time(input_file_b)

        for i in range(len(dpus)):
            a_time = data_a[i]
            b_time = data_b[i]
            
            speedup = b_time / a_time
            formatted_speedup = "{:.2f}".format(speedup)
            speedup_results[value].append(formatted_speedup)

    with open(output_file, 'w') as outfile:
        header = ["#DPUs"] + [str(value) for value in speed_values]
        outfile.write('\t'.join(header) + '\n')
        
        for i, dpus_value in enumerate(dpus):
            row = [str(dpus_value)] + [str(speedup_results[value][i]) for value in speed_values]
            outfile.write('\t'.join(row) + '\n')

    print(f"{output_file} saved")

process_files("ofis256-rank", "es256", './figures/speedup-ofis256-rank-es256.txt')

process_files("ofis512-rank", "es512", './figures/speedup-ofis512-rank-es512.txt')

process_files("ofis512-ig", "es512", './figures/speedup-ofis512-ig-es512.txt')
