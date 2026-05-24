"""
Generate storage-hierarchy charts as SVG (no external dependencies).
Output: latency_pyramid.svg, bandwidth_chart.svg
"""
import math

# ── Measured data ─────────────────────────────────────────────────────────
latency_data = [
    ("L1 Cache (32 KB)",   4.0),
    ("L2 Cache (512 KB)",  18.6),
    ("L3 Cache (8 MB)",    116.6),
    ("Main Memory (RAM)",  344.0),
    ("SSD Disk (random)",  180_000.0),
]

bw_data = [
    # (label, seq_GB/s, rand_GB/s)  — v2 实测中位数
    ("L1 Cache",  121.23, 1.76),
    ("L2 Cache",   91.88, 2.71),
    ("L3 Cache",    5.83, 1.27),
    ("RAM",         3.60, 0.17),
]

# ── Helpers ───────────────────────────────────────────────────────────────
def fmt_lat(ns):
    if ns < 1000:  return f"{ns:.1f} ns"
    if ns < 1e6:   return f"{ns/1000:.0f} µs"
    return f"{ns/1e6:.1f} ms"

def write_svg(path, content, w, h):
    svg = f'''<?xml version="1.0" encoding="utf-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}"
     viewBox="0 0 {w} {h}" font-family="monospace,Courier New">
{content}
</svg>'''
    with open(path, 'w', encoding='utf-8') as f:
        f.write(svg)
    print(f"Saved {path}")

# ════════════════════════════════════════════════════════════════════════════
# Chart 1 – Latency Pyramid (horizontal log-scale bars)
# ════════════════════════════════════════════════════════════════════════════
W, H = 820, 380
pad_l, pad_r, pad_t, pad_b = 170, 110, 55, 45
plot_w = W - pad_l - pad_r
plot_h = H - pad_t - pad_b

log_min, log_max = 0, 6   # 10^0=1 ns … 10^6=1ms
colors = ['#2196F3', '#4CAF50', '#FF9800', '#E91E63', '#795548']

def x_of(ns):
    return pad_l + (math.log10(max(ns, 1)) - log_min) / (log_max - log_min) * plot_w

elems = []
# Title
elems.append(f'<text x="{W//2}" y="30" text-anchor="middle" font-size="14" font-weight="bold" fill="#222">'
             f'Storage Hierarchy Latency (log scale)</text>')
elems.append(f'<text x="{W//2}" y="47" text-anchor="middle" font-size="11" fill="#555">'
             f'Intel i9-13900HX · Ubuntu 22.04 · VirtualBox VM</text>')

bar_h = 34
gap   = (plot_h - len(latency_data) * bar_h) // (len(latency_data) + 1)

# Gridlines at powers of 10
for exp in range(log_min, log_max + 1):
    xg = x_of(10**exp)
    elems.append(f'<line x1="{xg:.1f}" y1="{pad_t}" x2="{xg:.1f}" y2="{pad_t+plot_h}" '
                 f'stroke="#ccc" stroke-width="1" stroke-dasharray="4,3"/>')
    lbl = {0:'1 ns', 1:'10 ns', 2:'100 ns', 3:'1 µs', 4:'10 µs', 5:'100 µs', 6:'1 ms'}[exp]
    elems.append(f'<text x="{xg:.1f}" y="{pad_t+plot_h+15}" text-anchor="middle" '
                 f'font-size="9" fill="#888">{lbl}</text>')

# Bars
for i, (lbl, ns) in enumerate(latency_data):
    y = pad_t + gap * (i + 1) + bar_h * i
    x1 = x_of(1)
    x2 = x_of(ns)
    bw = max(x2 - x1, 4)
    elems.append(f'<rect x="{x1:.1f}" y="{y}" width="{bw:.1f}" height="{bar_h}" '
                 f'fill="{colors[i]}" rx="3"/>')
    # Level label (left)
    elems.append(f'<text x="{pad_l-8}" y="{y+bar_h//2+4}" text-anchor="end" '
                 f'font-size="11" fill="#333">{lbl}</text>')
    # Value label (right of bar)
    elems.append(f'<text x="{x2+6:.1f}" y="{y+bar_h//2+4}" font-size="11" '
                 f'font-weight="bold" fill="{colors[i]}">{fmt_lat(ns)}</text>')

# X axis
elems.append(f'<line x1="{pad_l}" y1="{pad_t+plot_h}" x2="{pad_l+plot_w}" '
             f'y2="{pad_t+plot_h}" stroke="#999" stroke-width="1.5"/>')
elems.append(f'<text x="{pad_l+plot_w//2}" y="{H-4}" text-anchor="middle" '
             f'font-size="11" fill="#555">Latency (ns, log scale →)</text>')

write_svg('latency_pyramid.svg', '\n'.join(elems), W, H)

# ════════════════════════════════════════════════════════════════════════════
# Chart 2 – Bandwidth Comparison (grouped bars, log scale)
# ════════════════════════════════════════════════════════════════════════════
W2, H2 = 740, 380
pad_l2, pad_r2, pad_t2, pad_b2 = 60, 30, 55, 60
plot_w2 = W2 - pad_l2 - pad_r2
plot_h2 = H2 - pad_t2 - pad_b2

log_min2, log_max2 = -1, 3   # 0.1 … 1000 GB/s

def y_of(gbps):
    return pad_t2 + plot_h2 - (math.log10(max(gbps, 0.01)) - log_min2) / (log_max2 - log_min2) * plot_h2

elems2 = []
elems2.append(f'<text x="{W2//2}" y="30" text-anchor="middle" font-size="14" font-weight="bold" fill="#222">'
              f'Sequential vs Random Read Bandwidth</text>')
elems2.append(f'<text x="{W2//2}" y="47" text-anchor="middle" font-size="11" fill="#555">'
              f'Intel i9-13900HX · Ubuntu 22.04 · VirtualBox VM</text>')

n = len(bw_data)
group_w = plot_w2 // n
bar_w2  = group_w // 3

# Gridlines
for exp in range(log_min2, log_max2 + 1):
    yg = y_of(10**exp)
    lbl = {-1:'0.1', 0:'1', 1:'10', 2:'100', 3:'1000'}[exp]
    elems2.append(f'<line x1="{pad_l2}" y1="{yg:.1f}" x2="{pad_l2+plot_w2}" y2="{yg:.1f}" '
                  f'stroke="#ddd" stroke-width="1" stroke-dasharray="4,3"/>')
    elems2.append(f'<text x="{pad_l2-5}" y="{yg+4:.1f}" text-anchor="end" '
                  f'font-size="9" fill="#888">{lbl}</text>')

# Y axis label
elems2.append(f'<text x="12" y="{pad_t2+plot_h2//2}" text-anchor="middle" '
              f'font-size="11" fill="#555" transform="rotate(-90,12,{pad_t2+plot_h2//2})">'
              f'GB/s (log scale)</text>')

seq_color  = '#2196F3'
rand_color = '#E91E63'

for i, (lbl, seq, rnd) in enumerate(bw_data):
    cx = pad_l2 + group_w * i + group_w // 2
    # Seq bar
    x_seq = cx - bar_w2 - 2
    y_seq = y_of(seq)
    h_seq = pad_t2 + plot_h2 - y_seq
    elems2.append(f'<rect x="{x_seq}" y="{y_seq:.1f}" width="{bar_w2}" height="{h_seq:.1f}" '
                  f'fill="{seq_color}" rx="2"/>')
    elems2.append(f'<text x="{x_seq+bar_w2//2}" y="{y_seq-4:.1f}" text-anchor="middle" '
                  f'font-size="9" fill="{seq_color}" font-weight="bold">{seq:.1f}</text>')
    # Rand bar
    x_rnd = cx + 2
    y_rnd = y_of(rnd)
    h_rnd = pad_t2 + plot_h2 - y_rnd
    elems2.append(f'<rect x="{x_rnd}" y="{y_rnd:.1f}" width="{bar_w2}" height="{h_rnd:.1f}" '
                  f'fill="{rand_color}" rx="2"/>')
    elems2.append(f'<text x="{x_rnd+bar_w2//2}" y="{y_rnd-4:.1f}" text-anchor="middle" '
                  f'font-size="9" fill="{rand_color}" font-weight="bold">{rnd:.2f}</text>')
    # Ratio label above
    ratio = seq / rnd
    top_y = min(y_seq, y_rnd) - 18
    elems2.append(f'<text x="{cx}" y="{top_y:.1f}" text-anchor="middle" '
                  f'font-size="10" fill="#333" font-weight="bold">{ratio:.0f}×</text>')
    # X label
    elems2.append(f'<text x="{cx}" y="{pad_t2+plot_h2+16}" text-anchor="middle" '
                  f'font-size="11" fill="#333">{lbl}</text>')

# X axis
elems2.append(f'<line x1="{pad_l2}" y1="{pad_t2+plot_h2}" x2="{pad_l2+plot_w2}" '
              f'y2="{pad_t2+plot_h2}" stroke="#999" stroke-width="1.5"/>')

# Legend
lx = W2 - pad_r2 - 130
ly = pad_t2 + 10
elems2.append(f'<rect x="{lx}" y="{ly}" width="14" height="14" fill="{seq_color}" rx="2"/>')
elems2.append(f'<text x="{lx+18}" y="{ly+11}" font-size="11" fill="#333">Sequential</text>')
elems2.append(f'<rect x="{lx}" y="{ly+20}" width="14" height="14" fill="{rand_color}" rx="2"/>')
elems2.append(f'<text x="{lx+18}" y="{ly+31}" font-size="11" fill="#333">Random</text>')

write_svg('bandwidth_chart.svg', '\n'.join(elems2), W2, H2)
