#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import matplotlib.pyplot as plt
import matplotlib.patches as patches
import numpy as np
from matplotlib import rcParams

# 设置中文字体
rcParams['font.sans-serif'] = ['Arial Unicode MS', 'SimHei', 'DejaVu Sans']
rcParams['axes.unicode_minus'] = False

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 10))

# ==================== 左图：1024 冲突情况 ====================
ax1.set_xlim(-0.5, 2.5)
ax1.set_ylim(-0.5, 9)
ax1.set_aspect('equal')
ax1.axis('off')

# 绘制 Cache 组（以组 0 为例）
ax1.text(0.5, 8.5, 'L2 Cache 组 0（8个缓存行位置）', fontsize=14, fontweight='bold', ha='center')
ax1.text(0.5, 8, '每行 64B，共 512 组', fontsize=11, color='gray', ha='center')

# 绘制 8 个缓存行位置
y_start = 7.5
for i in range(8):
    # 先绘制空位置
    rect = patches.Rectangle((0.1, y_start - i*0.8), 0.8, 0.7, 
                             linewidth=2, edgecolor='lightgray', facecolor='white')
    ax1.add_patch(rect)
    ax1.text(0.15, y_start - i*0.8 + 0.35, f'位置{i}', fontsize=9, va='center')

# 显示矩阵行映射（1024 规模）
ax1.text(1.5, 8.5, '矩阵 A 行映射到组 0', fontsize=12, fontweight='bold', ha='center', color='#FF6B6B')
ax1.text(1.5, 8, '(1024×8=8192=2^13)', fontsize=10, color='gray', ha='center')

# 映射信息
mappings_1024 = [
    ('A[0]', 0, '#FFE5E5', True),
    ('A[1]', 1, '#E5F5E5', False),
    ('A[2]', 2, '#E5F0FF', False),
    ('A[3]', 3, '#F5E5FF', False),
    ('A[4]', 0, '#FFE5E5', True),  # 冲突！
    ('A[5]', 1, '#E5F5E5', False),
    ('...', None, 'white', False),
    ('A[8]', 0, '#FFE5E5', True),  # 冲突！
]

y_pos = 7.5
for idx, (label, pos, color, conflict) in enumerate(mappings_1024):
    if pos is not None:
        ax1.text(1.3, y_pos - idx*0.8 + 0.35, f'{label}', fontsize=10, fontweight='bold' if conflict else 'normal')
        ax1.text(1.7, y_pos - idx*0.8 + 0.35, f'→ 位置{pos}', fontsize=9, 
                color='red' if conflict else 'black', fontweight='bold' if conflict else 'normal')
        
        # 加冲突标记
        if conflict:
            ax1.text(2.2, y_pos - idx*0.8 + 0.35, '🔴 冲突', fontsize=11, color='red', fontweight='bold')
    else:
        ax1.text(1.3, y_pos - idx*0.8 + 0.35, f'{label}', fontsize=10, color='gray')

# 添加说明
ax1.text(0.5, 0.5, '问题：每隔 4 行就映射回位置 0\n多行竞争 8 个位置 → 频繁驱逐', 
        fontsize=11, ha='center', bbox=dict(boxstyle='round', facecolor='#FFE5E5', alpha=0.7))

# ==================== 右图：1024 vs 1025 对比 ====================
ax2.set_xlim(-0.5, 3)
ax2.set_ylim(-0.5, 9)
ax2.axis('off')

# 标题
ax2.text(0.75, 8.7, '1024 vs 1025 冲突对比', fontsize=14, fontweight='bold', ha='center')

# 左侧：1024（冲突）
ax2.text(0.3, 8.2, '1024×1024 (冲突！)', fontsize=11, fontweight='bold', 
        bbox=dict(boxstyle='round', facecolor='#FFE5E5', alpha=0.8), ha='center')

collision_data_1024 = [
    'A[0]: 组 0',
    'A[1]: 组 128',
    'A[2]: 组 256',
    'A[3]: 组 384',
    'A[4]: 组 0 ⚠️',
    'A[5]: 组 128 ⚠️',
    '...',
    '规律性强 → 冲突集中',
]

for i, text in enumerate(collision_data_1024):
    color = '#FFE5E5' if '⚠️' in text else 'white'
    weight = 'bold' if '⚠️' in text else 'normal'
    ax2.text(0.3, 7.8 - i*0.5, text, fontsize=9, ha='center', fontweight=weight,
            bbox=dict(boxstyle='round,pad=0.3', facecolor=color, alpha=0.6))

# 右侧：1025（无冲突）
ax2.text(1.7, 8.2, '1025×1025 (无冲突)', fontsize=11, fontweight='bold',
        bbox=dict(boxstyle='round', facecolor='#E5F5E5', alpha=0.8), ha='center')

collision_data_1025 = [
    'A[0]: 组 0',
    'A[1]: 组 128',
    'A[2]: 组 256',
    'A[3]: 组 385',
    'A[4]: 组 1',
    'A[5]: 组 129',
    '...',
    '规律性弱 → 冲突消失',
]

for i, text in enumerate(collision_data_1025):
    ax2.text(1.7, 7.8 - i*0.5, text, fontsize=9, ha='center',
            bbox=dict(boxstyle='round,pad=0.3', facecolor='#E5F5E5', alpha=0.6))

# 中间箭头对比
ax2.annotate('', xy=(1.4, 4), xytext=(0.6, 4),
            arrowprops=dict(arrowstyle='<->', color='red', lw=3))
ax2.text(1.0, 4.3, '每隔 4 行冲突', fontsize=10, ha='center', 
        bbox=dict(boxstyle='round', facecolor='#FFE5E5', alpha=0.8), color='red', fontweight='bold')

# 底部总结
summary_text = '''
关键数学关系：
• 1024 = 2^10 → 每行 2^13 字节
  → 512 组中每隔 4 行冲突 (2^13 / 2^11 = 4)
  
• 1025 ≠ 2^n → 每行 8200 字节
  → 地址映射分散，冲突极少
  
结论：2^n 规模是缓存冲突的"完美风暴"
'''

ax2.text(1.0, 1.5, summary_text, fontsize=10, ha='center', va='center',
        bbox=dict(boxstyle='round,pad=1', facecolor='lightyellow', alpha=0.9, linewidth=2),
        family='monospace')

plt.suptitle('L2 Cache 组冲突示意图：1024 的"完美灾难"vs 1025 的分散映射', 
            fontsize=15, fontweight='bold', y=0.98)

plt.tight_layout()
plt.savefig('cache_conflict_diagram.png', dpi=300, bbox_inches='tight', facecolor='white')
print("✓ 已保存: cache_conflict_diagram.png")
plt.close()

# ==================== 另一个图：Cache 结构示意 ====================
fig, ax = plt.subplots(figsize=(14, 10))

ax.set_xlim(0, 10)
ax.set_ylim(0, 12)
ax.axis('off')

# 标题
ax.text(5, 11.5, 'L2 Cache 结构与地址映射关系', fontsize=16, fontweight='bold', ha='center')

# 上半部分：Cache 结构
ax.text(5, 10.5, 'L2 Cache = 256 KB = 512 组 × 8 路 × 64B行', fontsize=12, 
       bbox=dict(boxstyle='round', facecolor='#2F81F7', alpha=0.8), ha='center', color='white', fontweight='bold')

# 绘制 Cache 结构示意（简化）
y_cache = 9.5
# 显示几个组
for group in range(5):
    # 组号
    ax.text(0.5, y_cache - group*1.5, f'组{group}', fontsize=10, fontweight='bold', ha='center')
    
    # 8 个缓存行
    for way in range(8):
        x = 1.5 + way * 0.8
        rect = patches.Rectangle((x - 0.3, y_cache - group*1.5 - 0.3), 0.6, 0.6,
                                 linewidth=1, edgecolor='#2F81F7', facecolor='#E5F0FF', alpha=0.7)
        ax.add_patch(rect)
        if way == 0:
            ax.text(x, y_cache - group*1.5, '1', fontsize=8, ha='center', va='center')
    
    ax.text(8.5, y_cache - group*1.5, '← 8-way', fontsize=9, ha='left', va='center')

ax.text(9.5, y_cache - 5*1.5 - 0.5, '...', fontsize=12, ha='center')

# 中间：地址映射公式
ax.text(5, 6.5, '地址 → 组索引的映射规则', fontsize=13, fontweight='bold', 
       bbox=dict(boxstyle='round', facecolor='#238636', alpha=0.8), ha='center', color='white')

formula_text = '''
物理地址的第 6～14 位决定映射到哪个组
组索引 = (物理地址 >> 6) & 0x1FF

示例（1024×1024 矩阵）：
  A[0][0] 物理地址 0x0
    → 组索引 = (0x0 >> 6) & 0x1FF = 0

  A[1][0] 物理地址 0x2000
    → 组索引 = (0x2000 >> 6) & 0x1FF = 0x80

  A[4][0] 物理地址 0x8000
    → 组索引 = (0x8000 >> 6) & 0x1FF = 0x00 ⚠️ 冲突！
'''

ax.text(5, 4.5, formula_text, fontsize=10, ha='center', va='top', family='monospace',
       bbox=dict(boxstyle='round,pad=1', facecolor='lightyellow', alpha=0.9, linewidth=2))

# 下部分：关联度说明
ax.text(5, 1.0, '关联度（Associativity）= 8-way\n每个组有 8 个位置可以存放数据，\n' +
                '当超过 8 行映射到同一组时就开始冲突',
       fontsize=11, ha='center', va='top',
       bbox=dict(boxstyle='round,pad=0.8', facecolor='#FFE5E5', alpha=0.8, linewidth=2),
       fontweight='bold')

plt.tight_layout()
plt.savefig('cache_structure_detail.png', dpi=300, bbox_inches='tight', facecolor='white')
print("✓ 已保存: cache_structure_detail.png")
plt.close()

print("\n✅ 两张图都已生成完成！")
print("\n生成的文件：")
print("  1. cache_conflict_diagram.png - 1024 vs 1025 冲突对比示意图")
print("  2. cache_structure_detail.png - Cache 结构与地址映射详解")
