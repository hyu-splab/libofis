import csv
import argparse
import os

parser = argparse.ArgumentParser(description="Process execution time CSV file.")
parser.add_argument('input_file', type=str, help="Input CSV file path")
parser.add_argument("num_iter", type=int, help="Number of iterations")
args = parser.parse_args()

thread_times = {}

with open(args.input_file, 'r') as infile:
    csv_reader = csv.reader(infile)
    for row in csv_reader:
        threads = int(row[2])  
        exec_time = float(row[4]) 
        
        if threads not in thread_times:
            thread_times[threads] = []
        thread_times[threads].append(exec_time)

output_dir = './figures'

if not os.path.exists(output_dir):
    os.makedirs(output_dir)

output_file = f'{output_dir}/cpu.txt'

with open(output_file, 'w') as outfile:
    outfile.write("#Threads\texec\n")
    
    for threads in sorted(thread_times.keys()):
        avg_time = sum(thread_times[threads]) / len(thread_times[threads])
        outfile.write(f"{threads}\t{avg_time:.4f}\n")

print(f"Results saved to {output_file}")