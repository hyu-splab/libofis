#!/bin/bash

python3 ./scripts/order_file.py ./result/wram.csv ./result/wram_order.csv "$1"
# python3 ./scripts/order_file.py ./result/mram.csv ./result/mram_order.csv "$1"

python3 ./scripts/analyze_sdk.py ./result/wram_order.csv "$1"
# python3 ./scripts/analyze_sdk.py ./result/mram_order.csv "$1"