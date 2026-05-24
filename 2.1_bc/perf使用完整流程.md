# 使用 perf 采集 Cache Miss Rate · 完整操作流程

> 适用环境：**Linux 主机或带 PMU 透传的虚拟机**。
> 如果你的 VM 没有 PMU 透传（典型表现：`instructions = 0` 或 `<not supported>`），
> 请走"路径 B：修 VM"或退回到 `如何采集真实Miss_Rate.md` 里的 **cachegrind**。

---

## 0. 先做 30 秒自检：PMU 在你这台机器上能用吗？

```bash
perf stat -e cycles,instructions ls > /dev/null
```

看输出最末两行：

```
       11,234,567      cycles
       18,765,432      instructions     #    1.67  insn per cycle
```

- ✅ **两个数字都不是 0、不是 `<not supported>`** → PMU 可用，跳到第 1 节
- ❌ `instructions = 0` 或 `<not supported>` → **你处于无 PMU 透传的 VM**，先看第 7 节

---

## 1. 安装 perf

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install linux-tools-common linux-tools-generic linux-tools-$(uname -r)
perf --version          # 应输出 perf version 6.x.x
```

> 如果提示 `WARNING: perf not found for kernel ...`：
> ```bash
> sudo apt install linux-tools-$(uname -r) linux-cloud-tools-$(uname -r)
> ```

### 放开权限（每次重启都要做，或写进 sysctl）

```bash
# 临时
echo 1 | sudo tee /proc/sys/kernel/perf_event_paranoid

# 永久（重启后仍生效）
echo 'kernel.perf_event_paranoid = 1' | sudo tee /etc/sysctl.d/local.conf
sudo sysctl --system
```

`perf_event_paranoid` 数值含义：

| 值 | 含义 |
|---|---|
| 3 | 默认，只允许 root |
| 2 | 用户可监控自己的进程，禁用 kernel 事件 |
| **1** | **推荐**，用户可监控自己 + CPU 计数器 |
| 0 | 几乎全开 |
| -1 | 全开（root 也不限） |

---

## 2. 准备被测程序

我已经在 `2.1(b)(c)/` 下给了你 `gemm_one.c`，按命令行参数选循环顺序：

```bash
gcc -O2 -o gemm_one gemm_one.c
# 验证
./gemm_one ikj
# 应该看到类似：[order=ikj] time=0.21s gflops=9.61 trace=...
```

如果想对比不同优化级别：

```bash
gcc -O0 -o gemm_one_O0 gemm_one.c
gcc -O3 -march=native -o gemm_one_O3 gemm_one.c
```

---

## 3. 单次试跑（推荐先测 ikj，看看 perf 跑得通）

```bash
perf stat \
  -e cycles,instructions,L1-dcache-loads,L1-dcache-load-misses,LLC-loads,LLC-load-misses \
  ./gemm_one ikj
```

期望看到的输出格式：

```
[order=ikj]  time=0.215s  gflops=9.34  trace=330825.660165

 Performance counter stats for './gemm_one ikj':

         640.21 msec task-clock                #    1.000 CPUs utilized
      1,560,123,456      cycles                       #    2.437 GHz
      4,012,456,789      instructions             #    2.57  insn per cycle
      1,205,678,901      L1-dcache-loads          #    1.883 G/sec
        150,234,567      L1-dcache-load-misses    #   12.46% of all L1-dcache accesses
         18,234,567      LLC-loads                #   28.484 K/sec
            312,456      LLC-load-misses          #    1.71% of all LL-cache accesses

         0.640 seconds time elapsed
```

**关键 4 个数 ↓**

| 取值 | 来源 | 用途 |
|---|---|---|
| `12.46%` | L1-dcache-load-misses 行末 | **L1 miss rate** |
| `1.71%`  | LLC-load-misses 行末 | **LLC (L3) miss rate** |
| `2.57`   | instructions 行末 `insn per cycle` | **IPC**（CPI = 1/IPC） |
| `0.640 s`| 最末行 | 总耗时（与 results.csv 校对） |

---

## 4. 批量测 6 顺序 · 用我给的脚本

```bash
chmod +x bench_miss.sh
./bench_miss.sh
```

脚本干的事：

1. 对 ijk / ikj / jik / jki / kij / kji 各跑 `perf stat -r 3`（取 3 次均值，自带 stddev）
2. 每个产生 `perf_<order>.txt`
3. 汇总到 `summary.csv`，最后用 `column -s, -t` 打印对齐表

预期 `summary.csv` 长这样（数字是示例）：

```
order  cycles        instructions  ipc    l1_loads      l1_misses    l1_miss_rate  llc_loads   llc_misses  llc_miss_rate  time_s
ijk    3.65e9        4.0e9         1.10   1.20e9        3.56e8       0.2967        1.5e7       6.5e5       0.0433         1.500
ikj    1.45e9        4.0e9         2.76   1.20e9        5.04e7       0.0420        1.0e7       1.2e5       0.0120         0.454
jik    3.91e9        4.0e9         1.02   1.20e9        3.55e8       0.2960        1.6e7       7.0e5       0.0438         1.501
jki    8.20e9        4.0e9         0.49   1.20e9        7.98e8       0.6650        9.5e7       3.8e7       0.4000         2.267
kij    1.50e9        4.0e9         2.67   1.20e9        5.05e7       0.0421        1.0e7       1.3e5       0.0130         0.499
kji    8.05e9        4.0e9         0.50   1.20e9        7.96e8       0.6633        9.4e7       3.7e7       0.3936         2.180
```

> 上面是裸机预期数值。VM 里只要 PMU 能用，趋势完全一致。

---

## 5. perf 输出怎么读（逐行解释）

```
 Performance counter stats for './gemm_one ikj' (3 runs):
                                                       └─ -r 3 表示取 3 次均值
```

| 行 | 字段含义 |
|---|---|
| `cycles` | CPU 时钟周期数；`@ 2.437 GHz` 是平均频率 |
| `instructions` | 退役指令数；右边 `insn per cycle` 是 IPC |
| `L1-dcache-loads` | L1 数据缓存读次数 |
| `L1-dcache-load-misses` | L1 读未命中次数；**右边的百分比就是 L1 miss rate** |
| `LLC-loads` | 最后一级缓存（在 9700X 上 = L3）读次数 |
| `LLC-load-misses` | L3 读未命中；**右边百分比 = L3 miss rate**；这部分会真的去内存取数 |
| `( +- 0.30% )` | 多次运行的相对标准差 |

> 注：perf 里 "L1-dcache-load-misses" 是**读**未命中。
> 如果想看读 + 写，事件名换成 `L1-dcache-prefetches` / `mem_load_retired.*`，但本题用 `load` 已足够。

---

## 6. 进阶 · 加测 L2 / 区分 stride 受害者

cachegrind 不建模 L2，perf 可以补这一块。AMD Zen / Intel 事件名不一样：

### 看你的 CPU 支持哪些 L2 事件

```bash
perf list cache | grep -i l2
```

### AMD Ryzen 7 9700X（Zen 5）

```bash
perf stat -r 3 \
  -e l2_cache_accesses_from_dc_misses \
  -e l2_cache_misses_from_dc_misses \
  ./gemm_one ikj
```

输出末行会有：

```
miss / access = X.XX%   # 这就是 L2 miss rate
```

> 注：`l2_cache_*` 这类事件名在某些内核下要写成裸 raw 编码 `r0064` 之类，
> `perf list` 会给出当前可用的精确名。

### Intel CPU（任何 Core 系列）

```bash
perf stat -r 3 \
  -e mem_load_retired.l2_hit \
  -e mem_load_retired.l2_miss \
  ./gemm_one ikj
```

### 看哪条指令贡献了最多 L1 miss（热点定位）

```bash
perf record -e L1-dcache-load-misses ./gemm_one jki
perf report          # 按 q 退出
```

会看到类似：

```
  92.3%  gemm_one  gemm_one  [.] main      ← jki 内层
   3.1%  gemm_one  gemm_one  [.] init
```

`perf annotate` 还能下钻到汇编级，定位是哪条 `mov` 在 miss —— 答辩加分项。

---

## 7. VM 里 PMU 不工作怎么办

这是你之前遇到的情况：`cycles` 有数字，但 `instructions = 0`、`LLC-* = <not supported>`。

### 7.1 判断你用的是哪种 VM

```bash
sudo dmidecode -s system-product-name
# 或：
systemd-detect-virt
```

可能输出：`vmware`、`virtualbox`、`kvm`、`qemu` 等。

### 7.2 按 VM 类型处理

| VM | 能否开 PMU 透传 | 方法 |
|---|---|---|
| **VirtualBox** | ❌ 不支持 | 放弃 perf，走 cachegrind |
| **VMware Workstation/Player** | ✅ 支持 | 见 7.3 |
| **QEMU/KVM** | ✅ 支持 | 启动参数加 `-cpu host,pmu=on` |
| **WSL2** | ⚠️ 部分支持 | 通常能拿到 cycles/instructions/L1，LLC 看运气 |
| **Hyper-V** | ❌ 默认关闭 | 需要修改 host policy，复杂 |

### 7.3 VMware：打开 PMU 虚拟化

1. **完全关闭虚拟机**（不是挂起）
2. VMware → 虚拟机设置 → **处理器**
3. 勾选 **"虚拟化 CPU 性能计数器"**（英文：`Virtualize CPU performance counters`）
4. **同一面板**还要确认 "虚拟化 Intel VT-x/EPT 或 AMD-V/RVI" 也是勾上的
5. 启动虚拟机
6. 回到 0 节再跑一次自检

如果 GUI 里没有这个选项（旧版 Player），手动编辑 `.vmx` 文件加一行：

```
vpmc.enable = "TRUE"
```

### 7.4 QEMU/KVM：加 `pmu=on`

虚拟机 XML 里：

```xml
<cpu mode='host-passthrough' check='none'>
  <feature policy='require' name='pmu'/>
</cpu>
```

或命令行：

```bash
qemu-system-x86_64 -cpu host,pmu=on ...
```

### 7.5 如果以上都做不到 → 不要硬撑

直接用 cachegrind（`bench_cachegrind.sh`），数据一样可靠，**也是答辩里完全可接受的"真实测量"**（很多论文也是用 cachegrind 而不是 perf）。

---

## 8. 常见报错速查

| 报错 | 原因 | 解决 |
|---|---|---|
| `Permission denied` | `perf_event_paranoid` 太高 | `echo 1 \| sudo tee /proc/sys/kernel/perf_event_paranoid` |
| `event syntax error: 'L1-dcache-...'` | 事件名拼写或内核版本太老 | `perf list \| grep -i dcache` 查可用名 |
| `<not supported>` | VM 没透传该事件 | 看第 7 节 |
| 所有数都 `0` 但没报错 | PMU 多路复用失败 / VM 限制 | 减少 `-e` 事件个数到 ≤ 4，或换 cachegrind |
| `WARNING: ... not synthesized` | 缺 debuginfo，影响 `perf report` 不影响 `perf stat` | 装 `linux-image-$(uname -r)-dbgsym`，可选 |
| `WARNING: Kernel address maps...` | 想看 kernel 符号，`paranoid` 要 ≤ 1 | 同上 |

---

## 9. 完整流程一页纸（粘进答辩备忘）

```bash
# === 准备 ===
sudo apt install linux-tools-$(uname -r) valgrind
echo 1 | sudo tee /proc/sys/kernel/perf_event_paranoid
gcc -O2 -o gemm_one gemm_one.c

# === 验证 PMU 可用 ===
perf stat -e cycles,instructions ls > /dev/null
# instructions 不是 0 才能继续

# === 主测：6 顺序 ===
chmod +x bench_miss.sh
./bench_miss.sh
# 输出 summary.csv 直接用

# === 补 L2（按 CPU 选）===
perf list cache | grep -i l2
perf stat -r 3 -e l2_cache_accesses_from_dc_misses,l2_cache_misses_from_dc_misses \
  ./gemm_one ikj

# === 兜底（PMU 不可用时）===
./bench_cachegrind.sh
```

跑完把 `summary.csv` 或 `cg_summary.csv` 贴回来即可定稿 PPT。
