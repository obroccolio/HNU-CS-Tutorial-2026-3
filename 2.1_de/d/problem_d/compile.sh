#!/bin/bash

cd /Users/geyinhao/Desktop/code/2_1_de/problem_d

echo "======================================"
echo "编译所有版本 (6个程序 × 3个优化级别)"
echo "======================================"

LOOPS=("ijk" "jik" "kij" "ikj" "jki" "kji")
OPTS=("O1" "O2" "O3")

for LOOP in "${LOOPS[@]}"; do
    for OPT in "${OPTS[@]}"; do
        echo "编译: d_${LOOP} -${OPT}..."
        gcc -${OPT} -march=native -o d_${LOOP}_${OPT} d_${LOOP}.c
    done
done

echo ""
echo "✓ 编译完成！总共18个可执行文件"
