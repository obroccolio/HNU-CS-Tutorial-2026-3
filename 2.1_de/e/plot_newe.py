#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import matplotlib.pyplot as plt
import numpy as np
from matplotlib import rcParams

# 设置中文字体
rcParams['font.sans-serif'] = ['Arial Unicode MS', 'SimHei', 'DejaVu Sans']
rcParams['axes.unicode_minus'] = False

# 数据
sizes = np.array([1001, 1010, 1020, 1023, 1024, 1025, 1030, 1040, 1050, 1060, 1100, 1200, 1300, 1400, 1500])
ijk_time = np.array([757.51, 781.33, 772.80, 776.85, 834.89, 785.07, 801.51, 829.66, 837.61, 872.07, 976.37, 1308.48, 1641.63, 2074.74, 2577.40])
block_time = np.array([360.40, 347.14, 362.52, 380.22, 739.78, 399.00, 377.22, 386.49, 398.07, 411.35, 460.39, 599.05, 762.36, 958.25, 1179.28])
ijk_gflops = np.array([2.6481, 2.6373, 2.7464, 2.7563, 2.5722, 2.7434, 2.7267, 2.7116, 2.7641, 2.7315, 2.7264, 2.6412, 2.6766, 2.6452, 2.6189])
block_gflops = np.array([5.5660, 5.9360, 5.8545, 5.6315, 2.9029, 5.3980, 5.7936, 5.8209, 5.8161, 5.7907, 5.7821, 5.7692, 5.7637, 5.7271, 5.7238])
speedup = ijk_time / block_time

# 创建图1：耗时对比
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10))

# 图1a：耗时
ax1.plot(sizes, ijk_time, 'o-', linewidth=2.5, markersize=8, label='ijk 直接乘法', color='#FF6B6B')
ax1.plot(sizes, block_time, 's-', linewidth=2.5, markersize=8, label='ijkijk 分块乘法', color='#4ECDC4')

# 标记 1024
ax1.axvline(x=1024, color='red', linestyle='--', alpha=0.5, linewidth=2, label='1024 (2^10)')
ax1.axvspan(1023, 1025, alpha=0.1, color='red')

# 添加注释
ax1.annotate('1024 性能跳变', xy=(1024, 834.89), xytext=(1024, 950),
            arrowprops=dict(arrowstyle='->', color='red', lw=2),
            fontsize=11, color='red', fontweight='bold', ha='center')
ax1.annotate('1024 性能崩溃！', xy=(1024, 739.78), xytext=(1050, 850),
            arrowprops=dict(arrowstyle='->', color='red', lw=2),
            fontsize=11, color='red', fontweight='bold')

ax1.set_xlabel('矩阵规模 (n×n)', fontsize=12, fontweight='bold')
ax1.set_ylabel('执行时间 (ms)', fontsize=12, fontweight='bold')
ax1.set_title('问题 e：不同矩阵规模下的执行时间对比', fontsize=14, fontweight='bold', pad=15)
ax1.legend(fontsize=11, loc='upper left')
ax1.grid(True, alpha=0.3)
ax1.set_xticks(sizes)
ax1.set_xticklabels([str(s) for s in sizes], rotation=45, ha='right')

# 图1b：加速比
ax2.plot(sizes, speedup, 'D-', linewidth=2.5, markersize=8, color='#95E1D3', label='ijkijk / ijk 加速比')
ax2.axhline(y=2.0, color='green', linestyle=':', alpha=0.5, linewidth=2, label='理想 2x 加速')
ax2.axvline(x=1024, color='red', linestyle='--', alpha=0.5, linewidth=2)
ax2.axvspan(1023, 1025, alpha=0.1, color='red')

# 添加注释加速比
ax2.annotate('1024: 加速比暴跌至 1.13x', xy=(1024, 1.13), xytext=(1040, 1.4),
            arrowprops=dict(arrowstyle='->', color='red', lw=2),
            fontsize=11, color='red', fontweight='bold')

ax2.set_xlabel('矩阵规模 (n×n)', fontsize=12, fontweight='bold')
ax2.set_ylabel('加速比', fontsize=12, fontweight='bold')
ax2.set_title('分块乘法相对直接乘法的加速比（ijkijk / ijk）', fontsize=13, fontweight='bold', pad=15)
ax2.legend(fontsize=11, loc='lower right')
ax2.grid(True, alpha=0.3)
ax2.set_xticks(sizes)
ax2.set_xticklabels([str(s) for s in sizes], rotation=45, ha='right')
ax2.set_ylim([0.8, 2.5])

plt.tight_layout()
plt.savefig('newe_性能对比.png', dpi=300, bbox_inches='tight')
print("✓ 已保存: newe_性能对比.png")
plt.close()

# 创建图2：GFLOPS 对比
fig, ax = plt.subplots(figsize=(12, 7))

width = 30
x = sizes

bars1 = ax.bar(x - width/2, ijk_gflops, width, label='ijk 直接乘法', color='#FF6B6B', alpha=0.8)
bars2 = ax.bar(x + width/2, block_gflops, width, label='ijkijk 分块乘法', color='#4ECDC4', alpha=0.8)

# 标记 1024
ax.axvline(x=1024, color='red', linestyle='--', linewidth=2.5, alpha=0.6)
ax.axvspan(1023-width, 1025+width, alpha=0.08, color='red')

# 添加数值标签（仅在 1024 附近）
for i, (size, ijk_g, block_g) in enumerate(zip(sizes, ijk_gflops, block_gflops)):
    if 1023 <= size <= 1025:
        ax.text(size - width/2, ijk_g + 0.15, f'{ijk_g:.2f}', ha='center', fontsize=9, fontweight='bold')
        ax.text(size + width/2, block_g + 0.15, f'{block_g:.2f}', ha='center', fontsize=9, fontweight='bold')

ax.set_xlabel('矩阵规模 (n×n)', fontsize=12, fontweight='bold')
ax.set_ylabel('计算性能 (GFLOPS)', fontsize=12, fontweight='bold')
ax.set_title('问题 e：不同矩阵规模下的计算性能（GFLOPS）', fontsize=14, fontweight='bold', pad=15)
ax.legend(fontsize=11)
ax.grid(True, alpha=0.3, axis='y')
ax.set_xticks(x)
ax.set_xticklabels([str(s) for s in sizes], rotation=45, ha='right')

# 添加 1024 处的标注
ax.annotate('1024 处性能崩溃\nijk: 2.57G → block: 2.90G', 
            xy=(1024, 2.9), xytext=(1024, 5.5),
            arrowprops=dict(arrowstyle='->', color='red', lw=2),
            fontsize=11, color='red', fontweight='bold', ha='center',
            bbox=dict(boxstyle='round,pad=0.5', facecolor='yellow', alpha=0.3))

plt.tight_layout()
plt.savefig('newe_GFLOPS对比.png', dpi=300, bbox_inches='tight')
print("✓ 已保存: newe_GFLOPS对比.png")
plt.close()

# 创建图3：关键点放大（1001-1050）
fig, ax = plt.subplots(figsize=(12, 7))

mask = sizes <= 1050
sizes_zoom = sizes[mask]
ijk_time_zoom = ijk_time[mask]
block_time_zoom = block_time[mask]

ax.plot(sizes_zoom, ijk_time_zoom, 'o-', linewidth=3, markersize=10, label='ijk 直接乘法', color='#FF6B6B')
ax.plot(sizes_zoom, block_time_zoom, 's-', linewidth=3, markersize=10, label='ijkijk 分块乘法', color='#4ECDC4')

# 关键点标注
for i, size in enumerate(sizes_zoom):
    if size in [1023, 1024, 1025]:
        ax.plot(size, ijk_time_zoom[i], 'o', markersize=15, color='#FF6B6B', markeredgewidth=3, markerfacecolor='none')
        ax.plot(size, block_time_zoom[i], 's', markersize=15, color='#4ECDC4', markeredgewidth=3, markerfacecolor='none')

# 标记 1024
ax.axvline(x=1024, color='red', linestyle='--', linewidth=3, alpha=0.5)
ax.axvspan(1023.5, 1024.5, alpha=0.1, color='red')

# 标注关键数据
ax.annotate(f'1023: {ijk_time_zoom[-3]:.1f}ms', xy=(1023, ijk_time_zoom[-3]), xytext=(1020, 720),
            arrowprops=dict(arrowstyle='->', color='#FF6B6B', lw=1.5), fontsize=10, color='#FF6B6B', fontweight='bold')
ax.annotate(f'1024: {ijk_time_zoom[-2]:.1f}ms\n+7.5%', xy=(1024, ijk_time_zoom[-2]), xytext=(1026, 900),
            arrowprops=dict(arrowstyle='->', color='red', lw=2), fontsize=10, color='red', fontweight='bold',
            bbox=dict(boxstyle='round,pad=0.5', facecolor='yellow', alpha=0.4))
ax.annotate(f'1025: {ijk_time_zoom[-1]:.1f}ms', xy=(1025, ijk_time_zoom[-1]), xytext=(1028, 750),
            arrowprops=dict(arrowstyle='->', color='#FF6B6B', lw=1.5), fontsize=10, color='#FF6B6B', fontweight='bold')

ax.annotate(f'1023: {block_time_zoom[-3]:.1f}ms', xy=(1023, block_time_zoom[-3]), xytext=(1015, 340),
            arrowprops=dict(arrowstyle='->', color='#4ECDC4', lw=1.5), fontsize=10, color='#4ECDC4', fontweight='bold')
ax.annotate(f'1024: {block_time_zoom[-2]:.1f}ms\n+94.5%!', xy=(1024, block_time_zoom[-2]), xytext=(1026, 600),
            arrowprops=dict(arrowstyle='->', color='red', lw=2), fontsize=10, color='red', fontweight='bold',
            bbox=dict(boxstyle='round,pad=0.5', facecolor='yellow', alpha=0.4))
ax.annotate(f'1025: {block_time_zoom[-1]:.1f}ms', xy=(1025, block_time_zoom[-1]), xytext=(1028, 430),
            arrowprops=dict(arrowstyle='->', color='#4ECDC4', lw=1.5), fontsize=10, color='#4ECDC4', fontweight='bold')

ax.set_xlabel('矩阵规模 (n×n)', fontsize=12, fontweight='bold')
ax.set_ylabel('执行时间 (ms)', fontsize=12, fontweight='bold')
ax.set_title('问题 e：1024 附近的性能跳变（放大视图）', fontsize=14, fontweight='bold', pad=15)
ax.legend(fontsize=11, loc='upper left')
ax.grid(True, alpha=0.3)
ax.set_xticks(sizes_zoom)
ax.set_xticklabels([str(s) for s in sizes_zoom], rotation=45, ha='right')

plt.tight_layout()
plt.savefig('newe_1024放大视图.png', dpi=300, bbox_inches='tight')
print("✓ 已保存: newe_1024放大视图.png")
plt.close()

print("\n✅ 所有图表已生成完成！")
print("\n生成的文件：")
print("  1. newe_性能对比.png - 耗时和加速比对比")
print("  2. newe_GFLOPS对比.png - 计算性能对比")
print("  3. newe_1024放大视图.png - 1024 附近的关键跳变")
