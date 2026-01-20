#!/bin/bash

python3 ./scripts/analyze_speedup_ofis_sdk.py ./figures/wram.txt ./figures/ofis-dpu.txt ./figures/speedup-pr-dpu.txt
python3 ./scripts/analyze_speedup_ofis_sdk.py ./figures/wram.txt ./figures/ofis-ig.txt ./figures/speedup-pr-ig.txt
