#!/bin/bash

python3 ./scripts/analyze_ofis_ig.py ./results/result_ofis_ig.csv "$1"
python3 ./scripts/analyze_ofis_rank.py ./results/result_ofis_rank.csv "$1"
