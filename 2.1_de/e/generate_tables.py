#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Rectangle
from matplotlib import rcParams

# 设置中文字体
rcParams['font.sans-serif'] = ['Arial Unicode MS', 'SimHei', 'DejaVu Sans']
rcParams['axes.unicode_minus'] = False

# 创建数据
data = {
    '矩阵规模': [1001, 1010, 1020, 1023, 1024, 1025, 1030, 1040, 1050, 1060, 1100, 1200, 1300, 1400, 1500],
    'ijk耗时(ms)': [757.51, 781.33, 772.80, 776.85, 834.89, 785.07, 801.51, 829.66, 837.61, 872.07, 976.37, 1308.48, 1641.63, 2074.74, 2577.40],
    'ijkijk耗时(ms)': [360.40, 347.14, 362.52, 380.22, 739.78, 399.00, 377.22, 386.49, 398.07, 411.35, 460.39, 599.05, 762.36, 958.25, 1179.28],
    'ijk GFLOPS': [2.6481, 2.6373, 2.7464, 2.7563, 2.5722, 2.7434, 2.7267, 2.7116, 2.7641, 2.7315, 2.7264, 2.6412, 2.6766, 2.6452, 2.6189],
    'ijkijk GFLOPS': [5.5660, 5.9360, 5.8545, 5.6315, 2.9029, 5.3980, 5.7936, 5.8209, 5.8161, 5.7907, 5.7821, 5.7692, 5.7637, 5.7271, 5.7238],
}

df = pd.DataFrame(data)
df['加速比'] = (df['ijk耗时(ms)'] / df['ijkijk耗时(ms)']).round(2)
df['性能比'] = (df['ijkijk GFLOPS'] / df['ijk GFLOPS']).round(2)

# 标记 1024
df['备注'] = df['矩阵规模'].apply(lambda x: '🔴 缓存陷阱' if x == 1024 else ('✓ 基准' if x == 1023 else ''))

print("=" * 150)
print("问题 e：ijk vs ijkijk 分块乘法 - 完整性能对比表")
print("=" * 150)
print()
print(df.to_string(index=False))
print()
print("=" * 150)

# 关键统计
print("\n【关键数据统计】\n")
print(f"ijk 直接乘法：")
print(f"  - 平均耗时：{df['ijk耗时(ms)'].mean():.2f} ms")
print(f"  - 平均 GFLOPS：{df['ijk GFLOPS'].mean():.4f}")
print(f"  - 耗时范围：{df['ijk耗时(ms)'].min():.2f} ~ {df['ijk耗时(ms)'].max():.2f} ms")

print(f"\nijkijk 分块乘法：")
print(f"  - 平均耗时：{df['ijkijk耗时(ms)'].mean():.2f} ms")
print(f"  - 平均 GFLOPS：{df['ijkijk GFLOPS'].mean():.4f}")
print(f"  - 耗时范围：{df['ijkijk耗时(ms)'].min():.2f} ~ {df['ijkijk耗时(ms)'].max():.2f} ms")

print(f"\n加速比统计：")
print(f"  - 平均加速比：{df['加速比'].mean():.2f}x")
print(f"  - 最佳加速比：{df['加速比'].max():.2f}x (规模 {df.loc[df['加速比'].idxmax(), '矩阵规模']:.0f})")
print(f"  - 最差加速比：{df['加速比'].min():.2f}x (规模 {df.loc[df['加速比'].idxmin(), '矩阵规模']:.0f})")

print(f"\n1024 缓存陷阱分析：")
idx_1024 = df[df['矩阵规模'] == 1024].index[0]
idx_1023 = df[df['矩阵规模'] == 1023].index[0]
idx_1025 = df[df['矩阵规模'] == 1025].index[0]

ijk_change = (df.loc[idx_1024, 'ijk耗时(ms)'] - df.loc[idx_1023, 'ijk耗时(ms)']) / df.loc[idx_1023, 'ijk耗时(ms)'] * 100
block_change = (df.loc[idx_1024, 'ijkijk耗时(ms)'] - df.loc[idx_1023, 'ijkijk耗时(ms)'] ) / df.loc[idx_1023, 'ijkijk耗时(ms)'] * 100
recovery = (df.loc[idx_1025, 'ijkijk耗时(ms)'] - df.loc[idx_1024, 'ijkijk耗时(ms)']) / df.loc[idx_1024, 'ijkijk耗时(ms)'] * 100

print(f"  - ijk 性能下降：{ijk_change:+.1f}%")
print(f"  - ijkijk 性能下降：{block_change:+.1f}% ⚠️")
print(f"  - 从 1024 到 1025 的恢复率：{recovery:+.1f}%")

print("\n" + "=" * 150 + "\n")

# 生成表格图像
fig, ax = plt.subplots(figsize=(18, 10))
ax.axis('tight')
ax.axis('off')

# 准备表格数据
table_data = []
table_data.append(['矩阵规模', 'ijk 耗时\n(ms)', 'ijkijk 耗时\n(ms)', 'ijk\nGFLOPS', 'ijkijk\nGFLOPS', '加速比\n(倍数)', '性能比\n(倍数)', '备注'])

for idx, row in df.iterrows():
    table_data.append([
        f"{int(row['矩阵规模'])}",
        f"{row['ijk耗时(ms)']:.2f}",
        f"{row['ijkijk耗时(ms)']:.2f}",
        f"{row['ijk GFLOPS']:.4f}",
        f"{row['ijkijk GFLOPS']:.4f}",
        f"{row['加速比']:.2f}x",
        f"{row['性能比']:.2f}x",
        row['备注']
    ])

# 创建表格
table = ax.table(cellText=table_data, cellLoc='center', loc='center', 
                colWidths=[0.08, 0.10, 0.12, 0.10, 0.12, 0.10, 0.10, 0.12])

table.auto_set_font_size(False)
table.set_fontsize(9)
table.scale(1, 2.2)

# 对表格进行着色
for i in range(len(table_data)):
    for j in range(len(table_data[0])):
        cell = table[(i, j)]
        
        # 表头行
        if i == 0:
            cell.set_facecolor('#2F81F7')
            cell.set_text_props(weight='bold', color='white', fontsize=10)
        # 1024 行（缓存陷阱）
        elif i == 5:  # 1024 是第5行（从1开始计数）
            cell.set_facecolor('#FFE5E5')
            cell.set_text_props(weight='bold', color='#CC0000')
        # 1023 行（基准）
        elif i == 4:  # 1023 是第4行
            cell.set_facecolor('#E5F5E5')
            cell.set_text_props(weight='bold')
        # 偶数行着色
        elif i % 2 == 0:
            cell.set_facecolor('#F5F5F5')
        
        # 加速比列
        if j == 5 and i > 0:
            speedup = float(table_data[i][5].replace('x', ''))
            if speedup < 1.5:
                cell.set_facecolor('#FFE5E5')
                cell.set_text_props(color='#CC0000', weight='bold')
            elif speedup > 2.2:
                cell.set_facecolor('#E5FFE5')
                cell.set_text_props(color='#00AA00', weight='bold')

plt.title('问题 e：ijk 直接乘法 vs ijkijk 分块乘法性能对比表\n（块大小 64×64，-O2 编译优化）', 
         fontsize=14, fontweight='bold', pad=20)

plt.savefig('newe_完整对比表格.png', dpi=300, bbox_inches='tight', facecolor='white')
print("✓ 已保存表格图像: newe_完整对比表格.png\n")

# 生成简化摘要表
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

# 左表：关键规模对比
key_sizes = [1023, 1024, 1025]
key_data = df[df['矩阵规模'].isin(key_sizes)].copy()

ax1.axis('tight')
ax1.axis('off')

table_data_key = [['规模', 'ijk(ms)', 'ijkijk(ms)', '加速比']]
for idx, row in key_data.iterrows():
    table_data_key.append([
        f"{int(row['矩阵规模'])}",
        f"{row['ijk耗时(ms)']:.1f}",
        f"{row['ijkijk耗时(ms)']:.1f}",
        f"{row['加速比']:.2f}x"
    ])

table1 = ax1.table(cellText=table_data_key, cellLoc='center', loc='center', 
                  colWidths=[0.2, 0.25, 0.25, 0.2])
table1.auto_set_font_size(False)
table1.set_fontsize(11)
table1.scale(1, 2.5)

for i in range(len(table_data_key)):
    for j in range(len(table_data_key[0])):
        cell = table1[(i, j)]
        if i == 0:
            cell.set_facecolor('#2F81F7')
            cell.set_text_props(weight='bold', color='white', fontsize=12)
        elif i == 2:  # 1024 行
            cell.set_facecolor('#FFE5E5')
            cell.set_text_props(weight='bold', color='#CC0000', fontsize=11)
        else:
            cell.set_facecolor('#E5F5E5' if i == 1 or i == 3 else 'white')

ax1.set_title('关键规模对比：1023/1024/1025', fontsize=12, fontweight='bold', pad=15)

# 右表：性能摘要
ax2.axis('tight')
ax2.axis('off')

summary_data = [
    ['指标', 'ijk 直接乘法', 'ijkijk 分块乘法'],
    ['平均耗时(ms)', f"{df['ijk耗时(ms)'].mean():.1f}", f"{df['ijkijk耗时(ms)'].mean():.1f}"],
    ['最快(ms)', f"{df['ijk耗时(ms)'].min():.1f}", f"{df['ijkijk耗时(ms)'].min():.1f}"],
    ['最慢(ms)', f"{df['ijk耗时(ms)'].max():.1f}", f"{df['ijkijk耗时(ms)'].max():.1f}"],
    ['平均 GFLOPS', f"{df['ijk GFLOPS'].mean():.2f}", f"{df['ijkijk GFLOPS'].mean():.2f}"],
    ['性能范围', f"{df['ijk GFLOPS'].min():.2f}~{df['ijk GFLOPS'].max():.2f}", f"{df['ijkijk GFLOPS'].min():.2f}~{df['ijkijk GFLOPS'].max():.2f}"],
    ['平均加速比', '-', f"{df['加速比'].mean():.2f}x"],
    ['1024 下的加速比', '-', f"{df[df['矩阵规模']==1024]['加速比'].values[0]:.2f}x"],
]

table2 = ax2.table(cellText=summary_data, cellLoc='center', loc='center',
                  colWidths=[0.35, 0.325, 0.325])
table2.auto_set_font_size(False)
table2.set_fontsize(10)
table2.scale(1, 2.2)

for i in range(len(summary_data)):
    for j in range(len(summary_data[0])):
        cell = table2[(i, j)]
        if i == 0:
            cell.set_facecolor('#2F81F7')
            cell.set_text_props(weight='bold', color='white', fontsize=11)
        elif i == 7:  # 1024 行
            cell.set_facecolor('#FFE5E5')
            cell.set_text_props(color='#CC0000', weight='bold')
        elif i % 2 == 0:
            cell.set_facecolor('#F5F5F5')

ax2.set_title('性能摘要统计', fontsize=12, fontweight='bold', pad=15)

plt.suptitle('问题 e 性能数据摘要', fontsize=14, fontweight='bold', y=0.98)
plt.tight_layout(rect=[0, 0, 1, 0.96])
plt.savefig('newe_摘要对比表.png', dpi=300, bbox_inches='tight', facecolor='white')
print("✓ 已保存摘要表格: newe_摘要对比表.png\n")
plt.close('all')

print("✅ 所有表格已生成完成！")
