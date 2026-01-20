#!/bin/bash

python3 ./scripts/analyze_es_e2e.py ./SparseP_ES/results/ES_256_e2e.csv "$1"
python3 ./scripts/analyze_es_e2e.py ./SparseP_ES/results/ES_512_e2e.csv "$1"
