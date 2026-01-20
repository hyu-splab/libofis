#!/bin/bash

# python3 ./scripts/analyze_ofis.py ./SpMV_OFIS/results/OFIS_ig256.csv "$1"
python3 ./scripts/analyze_ofis.py ./SpMV_OFIS/results/OFIS_ig512.csv "$1"
python3 ./scripts/analyze_ofis.py ./SpMV_OFIS/results/OFIS_rank256.csv "$1"
python3 ./scripts/analyze_ofis.py ./SpMV_OFIS/results/OFIS_rank512.csv "$1"
