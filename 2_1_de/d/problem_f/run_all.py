#!/usr/bin/env python3
import subprocess
import os

os.chdir('/Users/geyinhao/Desktop/code/2_1_de/problem_f')

# 按块级顺序排列
block_orders = ['ijk', 'jik', 'kij', 'ikj', 'jki', 'kji']
elem_orders = ['ijk', 'jik', 'kij', 'ikj', 'jki', 'kji']

results = []
results.append("块级|元素级|不转置(ms)|转置后(ms)|转置(ms)|乘法(ms)|加速比|性能提升(%)")

count = 0
for b in block_orders:
    for e in elem_orders:
        exe = f'f_b{b}_e{e}'
        if os.path.isfile(exe):
            try:
                output = subprocess.check_output([f'./{exe}'], stderr=subprocess.STDOUT, text=True, timeout=60)
                results.append(output.strip())
                count += 1
                print(f"✓ {exe}")
            except Exception as ex:
                print(f"✗ {exe}: {ex}")
        else:
            print(f"? {exe} not found")

print(f"\n✅ 完成 {count}/36 个程序运行\n")

# 保存结果
with open('results.txt', 'w') as f:
    f.write('\n'.join(results) + '\n')

print(f"📊 结果已保存到 results.txt")
print(f"\n前10行：")
for i, line in enumerate(results[:11]):
    print(line)
