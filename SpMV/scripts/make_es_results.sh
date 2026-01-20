#!/bin/bash

python3 ./scripts/analyze_es.py ./SparseP_ES/results/ES_256.csv "$1"
python3 ./scripts/analyze_es.py ./SparseP_ES/results/ES_512.csv "$1"
