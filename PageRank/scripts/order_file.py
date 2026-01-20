import pandas as pd
import argparse

# Handle command-line arguments
def parse_arguments():
    parser = argparse.ArgumentParser(description="Program to sort a CSV file by the first column.")
    parser.add_argument("input_file", help="Path to the input CSV file")
    parser.add_argument("output_file", help="Path to the output CSV file")
    parser.add_argument("num_iter", type=int, help="Number of iterations (not used in sorting)")
    return parser.parse_args()

# Main function
def main():
    # Parse command-line arguments
    args = parse_arguments()
    input_file = args.input_file  # Path to the input file
    output_file = args.output_file  # Path to the output file
    num_iter = args.num_iter  # Number of iterations (not used in sorting)

    # 1. Read the input CSV file into a DataFrame
    df = pd.read_csv(input_file, header=None)  # Read CSV without headers

    # 2. Group the data into chunks of 21 rows
    grouped_data = [df.iloc[i:i+7*num_iter] for i in range(0, len(df), 7*num_iter)]  # Slice DataFrame into 21-row chunks

    # 3. Sort each group by the first column (index 0)
    sorted_groups = [group.sort_values(by=0) for group in grouped_data]  # Sort each group by the first column

    # 4. Concatenate the sorted groups back into a single DataFrame
    sorted_df = pd.concat(sorted_groups, ignore_index=True)  # Combine sorted groups into one DataFrame

    # 5. Save the sorted DataFrame to the output CSV file with fixed float format
    sorted_df.to_csv(output_file, index=False, header=False, float_format='%.2f')  # Save with 2 decimal places

    print(f"The sorted data has been successfully saved to {output_file}.")  # Inform the user that the file was saved

# Run the program
if __name__ == "__main__":
    main()
