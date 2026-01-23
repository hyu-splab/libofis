import pandas as pd

numbers = [100, 200, 400, 600, 800, 1000]

for num in numbers:
    file1 = pd.read_csv(f'./figures/cpu-{num}.txt', sep='\s+',  comment='#', 
                        names=['#Threads', 'exec'])
    file2 = pd.read_csv(f'./figures/ew256-{num}-e2e.txt', sep='\s+', comment='#', 
                        names=['#DPUs', 'transfer', 'exec', 'retrieve', 'postprocessing', 'preprocessing', 'e2e'])
    file3 = pd.read_csv(f'./figures/es512-{num}-e2e.txt', sep='\s+', comment='#', 
                        names=['#DPUs', 'transfer', 'exec', 'retrieve', 'postprocessing', 'preprocessing', 'e2e'])
    file4 = pd.read_csv(f'./figures/ofis512-ig-{num}-e2e.txt', sep='\s+', comment='#', 
                        names=['#DPUs', 'exec', 'postprocessing', 'preprocessing', 'e2e'])
    
    data = {
        'Type': ['CPU', 'OFIS(IG)', 'ES', 'EW'],
        'CPU': [file1['exec'].iloc[0], 0, 0, 0],
        'DPU': [0, 0, file3['exec'].iloc[0], file2['exec'].iloc[0]],
        'Input': [0, 0, file3['transfer'].iloc[0], file2['transfer'].iloc[0]],
        'Partition': [0, file4['preprocessing'].iloc[0], file3['preprocessing'].iloc[0], file2['preprocessing'].iloc[0]],
        'Output': [0, 0, file3['retrieve'].iloc[0], file2['retrieve'].iloc[0]],
        'ofis': [0, file4['exec'].iloc[0], 0, 0] 
    }
    
    df = pd.DataFrame(data)
    
    df['Computation'] = df['CPU'] + df['DPU'] + df['Input'] + df['Output'] + df['ofis']
    
    df_simple = df[['Type', 'Computation', 'Partition']].round(4)
    
    # output_file = f'./figures/combined-{num}-e2e.txt'
    # df.to_csv(output_file, sep='\t', index=False)
    
    output_file_simple = f'./figures/combined-{num}-e2e.txt'
    df_simple.to_csv(output_file_simple, sep='\t', index=False)
    
    print(f"Created {output_file_simple}")