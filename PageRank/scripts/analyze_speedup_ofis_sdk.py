import sys
import numpy as np
import pandas as pd

if len(sys.argv) != 4:
    print("Usage: python ratio_simple_T.py <t1.txt> <t2.txt> <output.tsv>")
    sys.exit(1)

t1_path, t2_path, out_path = sys.argv[1], sys.argv[2], sys.argv[3]
df1 = pd.read_csv(t1_path, sep=r"\s+", engine="python", index_col=0)
df2 = pd.read_csv(t2_path, sep=r"\s+", engine="python", index_col=0)

ratio = (df1.astype(float) / df2.astype(float)).replace([np.inf, -np.inf], np.nan).round(2).T
ratio.index.name = "#DPUs"
ratio.columns.name = df1.index.name or "#SGs"
ratio.to_csv(out_path, sep="\t", float_format="%.2f")
print(f"{out_path}-saved")
