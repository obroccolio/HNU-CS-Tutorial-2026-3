#!/bin/bash

cd /Users/geyinhao/Desktop/code/2_1_de/problem_d

LOOPS=("ijk" "jik" "kij" "ikj" "jki" "kji")
OPTS=("O1" "O2" "O3")

> results.txt

echo "======================================"
echo "运行所有18个版本"
echo "======================================"

for OPT in "${OPTS[@]}"; do
    echo "【优化级别: -${OPT}】" >> results.txt
    echo "" >> results.txt
    
    for LOOP in "${LOOPS[@]}"; do
        EXEC="d_${LOOP}_${OPT}"
        if [ -f "$EXEC" ]; then
            echo "运行: $EXEC"
            ./$EXEC >> results.txt
        fi
    done
    echo "========================================" >> results.txt
    echo "" >> results.txt
done

echo ""
echo "✓ 运行完成！结果已保存到 results.txt"
