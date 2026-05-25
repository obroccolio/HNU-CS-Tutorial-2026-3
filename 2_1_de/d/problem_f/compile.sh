#!/bin/bash

# 编译所有36个C文件
cd /Users/geyinhao/Desktop/code/2_1_de/problem_f

echo "🔨 开始编译36个C文件..."
count=0

for file in f_block_*.c; do
    exe_name="${file%.c}"
    gcc -O2 -march=native -o "$exe_name" "$file" 2>&1
    if [ -f "$exe_name" ]; then
        ((count++))
        echo "✓ 编译 $exe_name"
    else
        echo "✗ 编译失败：$file"
    fi
done

echo ""
echo "✅ 编译完成：$count/36 个可执行文件"
ls -1 f_block_* | grep -v "\.c$" | wc -l
