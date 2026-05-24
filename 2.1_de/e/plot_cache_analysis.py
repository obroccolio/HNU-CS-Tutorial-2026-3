import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np
from matplotlib import rcParams

# 设置中文字体
rcParams['font.sans-serif'] = ['Heiti SC', 'Songti SC', 'SimHei', 'DejaVu Sans']
rcParams['axes.unicode_minus'] = False

# 实验数据
sizes = [1000, 1001, 1002, 1023, 1024, 1025, 1500, 1600, 2000, 2047, 2048, 2049, 2050, 3000]
time_no_transpose = [114.69, 127.24, 110.66, 218.71, 189.27, 235.27, 379.34, 452.42, 893.53, 1843.07, 1615.72, 1903.91, 1543.84, 3011.86]
time_with_transpose = [268.70, 274.60, 273.10, 296.61, 791.95, 300.05, 953.39, 1140.91, 2217.39, 2515.83, 7562.30, 2601.99, 2473.84, 7477.48]
speedup = [t1/t2 for t1, t2 in zip(time_no_transpose, time_with_transpose)]

# 创建图表
fig, axes = plt.subplots(2, 2, figsize=(14, 10))
fig.suptitle('问题E：矩阵大小对缓存性能的影响', fontsize=16, fontweight='bold')

# ========== 图1：加速比曲线 ==========
ax1 = axes[0, 0]
ax1.plot(sizes, speedup, 'o-', linewidth=2, markersize=6, color='#2E86AB', label='加速比')
ax1.axvline(x=1024, color='red', linestyle='--', linewidth=2, alpha=0.7, label='2^10 (1024)')
ax1.axvline(x=2048, color='red', linestyle='--', linewidth=2, alpha=0.7, label='2^11 (2048)')
ax1.axhline(y=1.0, color='gray', linestyle=':', linewidth=1, alpha=0.5)

# 标注关键点
ax1.plot(1024, speedup[sizes.index(1024)], 'r*', markersize=20)
ax1.plot(2048, speedup[sizes.index(2048)], 'r*', markersize=20)
ax1.text(1024, 0.25, '0.24倍\n(崩溃!)', ha='center', color='red', fontweight='bold')
ax1.text(2048, 0.22, '0.21倍\n(崩溃!)', ha='center', color='red', fontweight='bold')

ax1.set_xlabel('矩阵大小 (N)', fontsize=11)
ax1.set_ylabel('加速比 (转置 / 无转置)', fontsize=11)
ax1.set_title('加速比曲线 - 2的幂次处性能崩溃', fontsize=12, fontweight='bold')
ax1.grid(True, alpha=0.3)
ax1.legend()
ax1.set_ylim([0, 1.0])

# ========== 图2：时间对比（对数尺度） ==========
ax2 = axes[0, 1]
ax2.semilogy(sizes, time_no_transpose, 'o-', linewidth=2, markersize=6, label='无转置', color='#06A77D')
ax2.semilogy(sizes, time_with_transpose, 's-', linewidth=2, markersize=6, label='有转置', color='#D62839')
ax2.axvline(x=1024, color='red', linestyle='--', linewidth=1.5, alpha=0.5)
ax2.axvline(x=2048, color='red', linestyle='--', linewidth=1.5, alpha=0.5)

ax2.set_xlabel('矩阵大小 (N)', fontsize=11)
ax2.set_ylabel('运行时间 (ms，对数尺度)', fontsize=11)
ax2.set_title('运行时间对比 (对数尺度)', fontsize=12, fontweight='bold')
ax2.grid(True, alpha=0.3, which='both')
ax2.legend()

# ========== 图3：放大的1024附近区域 ==========
ax3 = axes[1, 0]
zoom_indices = [2, 3, 4, 5]  # 1002, 1023, 1024, 1025
zoom_sizes = [sizes[i] for i in zoom_indices]
zoom_speedup = [speedup[i] for i in zoom_indices]
colors = ['green' if s != 1024 else 'red' for s in zoom_sizes]

bars = ax3.bar(range(len(zoom_sizes)), zoom_speedup, color=colors, alpha=0.7, edgecolor='black', linewidth=2)
ax3.set_xticks(range(len(zoom_sizes)))
ax3.set_xticklabels(zoom_sizes)
ax3.set_ylabel('加速比', fontsize=11)
ax3.set_title('放大：1024处性能崩溃', fontsize=12, fontweight='bold')
ax3.set_ylim([0, 1.0])
ax3.axhline(y=0.74, color='green', linestyle='--', alpha=0.5, label='正常范围')

# 添加数值标签
for i, (bar, val) in enumerate(zip(bars, zoom_speedup)):
    ax3.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.02, 
             f'{val:.2f}倍', ha='center', fontsize=10, fontweight='bold')

ax3.text(2, 0.3, '缓存\n碰撞!', ha='center', fontsize=12, color='red', fontweight='bold',
         bbox=dict(boxstyle='round', facecolor='yellow', alpha=0.3))
ax3.legend()
ax3.grid(True, axis='y', alpha=0.3)

# ========== 图4：Cache组冲突示意图 ==========
ax4 = axes[1, 1]
ax4.axis('off')

# 绘制cache组
cache_groups = 8  # 简化为8个组示意
y_pos = 0.8

# 标题
ax4.text(0.5, 0.95, '缓存组关联碰撞示意图', 
         ha='center', fontsize=12, fontweight='bold', transform=ax4.transAxes)

# 1001矩阵的分布
ax4.text(0.05, y_pos, '1001x1001（优秀）：', fontsize=10, fontweight='bold', transform=ax4.transAxes)
y_pos -= 0.08
row_height = 0.04
for i in range(cache_groups):
    # 绘制组
    rect = mpatches.Rectangle((0.1 + i*0.08, y_pos), 0.06, row_height, 
                               linewidth=1, edgecolor='black', facecolor='lightblue', transform=ax4.transAxes)
    ax4.add_patch(rect)
    ax4.text(0.13 + i*0.08, y_pos + row_height/2, str(i), ha='center', va='center', 
             fontsize=8, transform=ax4.transAxes)

# 在几个组中添加数据块
data_positions_1001 = [0, 2, 4, 6, 1, 3, 5, 7]  # 分散分布
for pos in data_positions_1001[:4]:
    ax4.text(0.13 + pos*0.08, y_pos + row_height/2, '●', ha='center', va='center', 
             fontsize=14, color='green', transform=ax4.transAxes)

ax4.text(0.05, y_pos - 0.06, '行分布均匀 → 低碰撞', 
         fontsize=9, color='green', transform=ax4.transAxes)

# 1024矩阵的分布
y_pos -= 0.15
ax4.text(0.05, y_pos, '1024x1024（糟糕 - 缓存碰撞）：', fontsize=10, fontweight='bold', transform=ax4.transAxes)
y_pos -= 0.08
for i in range(cache_groups):
    rect = mpatches.Rectangle((0.1 + i*0.08, y_pos), 0.06, row_height, 
                               linewidth=1, edgecolor='black', facecolor='lightyellow', transform=ax4.transAxes)
    ax4.add_patch(rect)
    ax4.text(0.13 + i*0.08, y_pos + row_height/2, str(i), ha='center', va='center', 
             fontsize=8, transform=ax4.transAxes)

# 在某些组中添加很多数据块（冲突）
collision_positions = [0, 2, 4, 6]  # 规则分布导致冲突
for pos in collision_positions:
    for k in range(3):  # 多个行映射到同一组
        ax4.text(0.13 + pos*0.08, y_pos + row_height/2 - k*0.015, '●', ha='center', va='center', 
                 fontsize=12, color='red', transform=ax4.transAxes, alpha=0.7)

ax4.text(0.05, y_pos - 0.08, '行与2的幂对齐 → 集中碰撞 → 缓存颠簸!', 
         fontsize=9, color='red', transform=ax4.transAxes, fontweight='bold')

plt.tight_layout()
plt.savefig('/Users/geyinhao/Desktop/2_1_de/e/cache_performance_analysis.png', dpi=300, bbox_inches='tight')
print("图表已保存到：cache_performance_analysis.png")
plt.show()

# 同时生成一个详细的数据表
print("\n" + "="*80)
print("性能数据详细表")
print("="*80)
print(f"{'大小':<8} {'无转置(ms)':<15} {'有转置(ms)':<15} {'加速比':<10} {'备注':<20}")
print("-"*80)
for size, no_t, with_t, sp in zip(sizes, time_no_transpose, time_with_transpose, speedup):
    notes = ""
    if size == 1024 or size == 2048:
        notes = f"2^{size.bit_length()-1}（崩溃!）"
    elif size in [1023, 2047]:
        notes = f"2^{size.bit_length()}-1"
    elif size in [1025, 2049]:
        notes = f"2^{size.bit_length()-1}+1"
    
    print(f"{size:<8} {no_t:<15.2f} {with_t:<15.2f} {sp:<10.2f} {notes:<20}")
