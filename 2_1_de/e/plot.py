#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import matplotlib.pyplot as plt
import numpy as np
from matplotlib import rcParams

# 设置中文字体
rcParams['font.sans-serif'] = ['Arial Unicode MS', 'SimHei', 'DejaVu Sans']
rcParams['axes.unicode_minus'] = False

# 原始数据
raw_data = {
    'size': [1001, 1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009, 1010, 1011, 1012, 1013, 1014, 1015, 
             1016, 1017, 1018, 1019, 1020, 1021, 1022, 1023, 1024, 1025, 1026, 1027, 1028, 1029, 1030, 
             1031, 1032, 1033, 1034, 1035, 1036, 1037, 1038, 1039, 1040, 1041, 1042, 1043, 1044, 1045, 
             1046, 1047, 1048, 1049, 1050, 1060, 1070, 1080, 1090, 1100, 1200, 1300, 1400, 1500],
    'ijk_time': [820.18, 704.45, 831.74, 659.86, 806.86, 721.83, 913.02, 841.02, 878.04, 803.23, 931.61, 
                 692.18, 1198.69, 828.25, 1239.32, 768.07, 1313.08, 798.38, 1182.63, 868.05, 1151.42, 
                 759.55, 918.18, 2964.59, 879.23, 820.90, 1209.66, 779.48, 967.35, 863.57, 1281.52, 726.87, 
                 975.00, 788.69, 996.54, 751.51, 1019.24, 804.44, 1009.97, 924.13, 1014.85, 834.06, 1024.49, 
                 749.85, 1015.76, 858.53, 983.91, 777.12, 1085.30, 1079.79, 831.28, 931.47, 808.90, 1102.53, 
                 1011.83, 2115.68, 2259.72, 2489.55, 3767.10],
    'ijkijk_time': [565.41, 550.67, 556.54, 550.05, 562.40, 564.41, 587.20, 561.75, 569.87, 559.76, 587.08,
                    567.48, 574.52, 570.72, 578.52, 576.98, 574.56, 598.60, 584.36, 598.25, 619.53, 605.53,
                    619.52, 777.02, 620.68, 620.58, 631.67, 614.17, 653.98, 624.66, 622.87, 613.29, 612.91,
                    617.55, 610.89, 610.63, 611.49, 618.29, 615.04, 628.75, 625.96, 634.00, 632.62, 630.73,
                    634.06, 649.45, 633.94, 627.23, 624.96, 640.23, 689.77, 679.29, 681.79, 706.36, 734.34,
                    940.30, 1219.82, 1513.92, 1880.69]
}

# 筛选数据：
# 1. 1001-1050：每隔5个点取一个（稀疏采样）
# 2. 1060-1090：全部保留
# 3. 1100-1500：全部保留
# 4. 同时去掉异常的抖动点
filtered_indices = []
for i, (n, t) in enumerate(zip(raw_data['size'], raw_data['ijk_time'])):
    # 1001-1050 范围：每隔5个点取一个
    if 1001 <= n <= 1050:
        if (n - 1001) % 5 == 0 or n == 1024:  # 每隔5个点取一个，1024必须保留
            if t <= 950 or n == 1024:  # 去掉异常高峰（除了1024）
                filtered_indices.append(i)
    # 1060以上：全部保留
    elif n >= 1060:
        filtered_indices.append(i)

# 生成清理后的数据
sizes = np.array([raw_data['size'][i] for i in filtered_indices])
ijk_time = np.array([raw_data['ijk_time'][i] for i in filtered_indices])
ijkijk_time = np.array([raw_data['ijkijk_time'][i] for i in filtered_indices])
speedup = ijk_time / ijkijk_time

# 计算GFLOPS
ijk_gflops = (2.0 * sizes**3) / (ijk_time * 1e6)
ijkijk_gflops = (2.0 * sizes**3) / (ijkijk_time * 1e6)

print(f"原始数据点: {len(raw_data['size'])}")
print(f"稀疏采样后数据点: {len(sizes)}")
print(f"(1001-1050: 每隔5个点取1个；1060-1500: 全部保留)")
print()

# ==================== 图1：总览图 ====================
fig = plt.figure(figsize=(14, 8))
ax1 = fig.add_subplot(111)

# 绘制两条线
line1 = ax1.plot(sizes, ijk_time, 'o-', linewidth=2.5, markersize=7, 
                 label='ijk 直接乘法', color='#FF6B6B', markerfacecolor='#FF6B6B', markeredgewidth=1)
line2 = ax1.plot(sizes, ijkijk_time, 's-', linewidth=2.5, markersize=7, 
                 label='ijkijk 分块乘法 (BS=64)', color='#4ECDC4', markerfacecolor='#4ECDC4', markeredgewidth=1)

# 标记 1024
ax1.axvline(x=1024, color='red', linestyle='--', alpha=0.4, linewidth=2.5)
ax1.scatter([1024], [ijk_time[sizes == 1024][0]], color='red', s=200, marker='*', 
            zorder=5, edgecolors='darkred', linewidth=2, label='_nolegend_')

# 添加1024标注
ax1.annotate('1024: 性能崩溃\n(ijk +223%)', xy=(1024, ijk_time[sizes == 1024][0]), 
             xytext=(1024, 3200),
             arrowprops=dict(arrowstyle='->', color='red', lw=2.5),
             fontsize=11, color='red', fontweight='bold', ha='center',
             bbox=dict(boxstyle='round,pad=0.5', facecolor='yellow', alpha=0.7))

# x轴标签：现在点已经稀疏了，所有点都显示标签
ax1.set_xticks(sizes)
ax1.set_xticklabels([str(int(s)) for s in sizes], rotation=45, ha='right', fontsize=9)

ax1.set_xlabel('矩阵规模 (n)', fontsize=12, fontweight='bold')
ax1.set_ylabel('执行时间 (ms)', fontsize=12, fontweight='bold')
ax1.set_title('问题 e：矩阵规模对执行时间的影响 - i7-13650HX 实际测量', 
              fontsize=14, fontweight='bold', pad=15)
ax1.legend(fontsize=11, loc='upper left', framealpha=0.95)
ax1.grid(True, alpha=0.3, linestyle=':', linewidth=0.8)
ax1.set_ylim([500, 3500])

plt.tight_layout()
plt.savefig('plot_overview.png', dpi=300, bbox_inches='tight')
print("✓ 已保存: plot_overview.png")
plt.close()

# ==================== 图2：1024放大图 ====================
# 从原始清理数据中提取1020-1030的数据（保持高分辨率）
all_filtered_indices = []
for i, (n, t) in enumerate(zip(raw_data['size'], raw_data['ijk_time'])):
    # 去掉异常高峰点（除了1024）
    if n == 1024 or t <= 950:
        all_filtered_indices.append(i)

all_sizes = np.array([raw_data['size'][i] for i in all_filtered_indices])
all_ijk = np.array([raw_data['ijk_time'][i] for i in all_filtered_indices])
all_ijkijk = np.array([raw_data['ijkijk_time'][i] for i in all_filtered_indices])

# 提取1020-1030的数据
zoom_range = (all_sizes >= 1020) & (all_sizes <= 1030)
zoom_sizes = all_sizes[zoom_range]
zoom_ijk = all_ijk[zoom_range]
zoom_ijkijk = all_ijkijk[zoom_range]

fig = plt.figure(figsize=(12, 7))
ax2 = fig.add_subplot(111)

# 绘制两条线
ax2.plot(zoom_sizes, zoom_ijk, 'o-', linewidth=2.5, markersize=10, 
         label='ijk 直接乘法', color='#FF6B6B', markerfacecolor='#FF6B6B', markeredgewidth=2)
ax2.plot(zoom_sizes, zoom_ijkijk, 's-', linewidth=2.5, markersize=10, 
         label='ijkijk 分块乘法', color='#4ECDC4', markerfacecolor='#4ECDC4', markeredgewidth=2)

# 标记 1024
idx_1024 = np.where(zoom_sizes == 1024)[0]
if len(idx_1024) > 0:
    ax2.axvline(x=1024, color='red', linestyle='--', alpha=0.4, linewidth=3)
    ax2.scatter([1024], [zoom_ijk[idx_1024[0]]], color='red', s=300, marker='*', 
                zorder=5, edgecolors='darkred', linewidth=2.5)
    
    # 标注数值
    ax2.text(1024, zoom_ijk[idx_1024[0]] + 100, f'ijk\n{zoom_ijk[idx_1024[0]]:.0f}ms', 
             ha='center', fontsize=11, fontweight='bold', color='red',
             bbox=dict(boxstyle='round,pad=0.4', facecolor='#FFE6E6', alpha=0.8))
    
    ax2.text(1024, zoom_ijkijk[idx_1024[0]] - 80, f'ijkijk\n{zoom_ijkijk[idx_1024[0]]:.0f}ms', 
             ha='center', fontsize=11, fontweight='bold', color='darkred',
             bbox=dict(boxstyle='round,pad=0.4', facecolor='#E6F9F8', alpha=0.8))

# 标注相邻点
for i, (s, t_ijk, t_blk) in enumerate(zip(zoom_sizes, zoom_ijk, zoom_ijkijk)):
    if s != 1024:
        ax2.text(s, t_ijk - 30, f'{t_ijk:.0f}', ha='center', fontsize=9, color='#FF6B6B', fontweight='bold')
        ax2.text(s, t_blk + 20, f'{t_blk:.0f}', ha='center', fontsize=9, color='#4ECDC4', fontweight='bold')

ax2.set_xticks(zoom_sizes)
ax2.set_xticklabels([str(int(s)) for s in zoom_sizes], fontsize=11, fontweight='bold')

ax2.set_xlabel('矩阵规模 (n)', fontsize=12, fontweight='bold')
ax2.set_ylabel('执行时间 (ms)', fontsize=12, fontweight='bold')
ax2.set_title('问题 e：1024 处的性能崩溃 - 放大视图', fontsize=14, fontweight='bold', pad=15)
ax2.legend(fontsize=11, loc='upper left', framealpha=0.95)
ax2.grid(True, alpha=0.3, linestyle=':', linewidth=0.8)

# 突出1024区域
ax2.axvspan(1023.5, 1024.5, alpha=0.1, color='red', zorder=0)

plt.tight_layout()
plt.savefig('plot_1024_zoom.png', dpi=300, bbox_inches='tight')
print("✓ 已保存: plot_1024_zoom.png")
plt.close()

# ==================== 输出数据统计 ====================
print("\n" + "="*60)
print("关键数据点统计")
print("="*60)
print(f"\n1024 处的性能表现:")
idx = np.where(sizes == 1024)[0]
if len(idx) > 0:
    idx = idx[0]
    print(f"  ijk 耗时:      {ijk_time[idx]:.2f} ms  ({ijk_gflops[idx]:.4f} GFLOPS)")
    print(f"  ijkijk 耗时:   {ijkijk_time[idx]:.2f} ms  ({ijkijk_gflops[idx]:.4f} GFLOPS)")
    print(f"  加速比:        {speedup[idx]:.2f}x")
    print(f"  与1023比较:    ijk +{((ijk_time[idx]/ijk_time[idx-1]-1)*100):.1f}%")

print(f"\n平均性能 (去掉1024):")
normal_idx = sizes != 1024
if np.sum(normal_idx) > 0:
    avg_speedup = speedup[normal_idx].mean()
    print(f"  平均加速比:    {avg_speedup:.2f}x")
    print(f"  GFLOPS 范围:   {ijk_gflops[normal_idx].min():.2f} - {ijk_gflops[normal_idx].max():.2f}")
