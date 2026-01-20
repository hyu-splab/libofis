#!/bin/bash

python3 ./scripts/order_file.py ./result/ofis_dpu.csv ./result/ofis_dpu_order.csv "$1"
python3 ./scripts/order_file.py ./result/ofis_ig.csv ./result/ofis_ig_order.csv "$1"

python3 ./scripts/analyze_ofis.py ./result/ofis_dpu_order.csv "$1"
python3 ./scripts/analyze_ofis.py ./result/ofis_ig_order.csv "$1"
