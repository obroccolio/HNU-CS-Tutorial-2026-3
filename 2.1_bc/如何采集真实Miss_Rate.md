# 如何采集真实的 Cache Miss Rate —— 选题 2.1(c) 实测指南

> 给 (c) 问题的"差距来自哪一层 cache"提供 **真硬件计数器** 证据，
> 而不是停留在理论估算（12.5% / 100% 那种）。

---

## 0. 思路

Cache miss rate 不是 CPU 直接给你算好的指标，需要做两步：

1. 让 CPU 的 **PMU (Performance Monitoring Unit)** 在运行时统计：
   - "L1 dcache load 次数" 和 "L1 dcache miss 次数"
   - "LLC（最后一级缓存）load 次数" 和 "miss 次数"
   - 周期数 / 指令数（用来算 CPI）
2. 用 `miss / load` 自己算出 miss rate。

下面给出 **4 条采集路径**，按推荐度排序。**(a) 报告用的就是 Linux + Ryzen 7 9700X**，所以路径 A 最自然。

---

## 路径 A：Linux `perf stat`（强推荐）

> 适合：(a) 已使用的 Linux 环境（Ubuntu 6.8 + Ryzen 7 9700X）。
> 优点：内核原生支持，零额外依赖，输出直接可读。

### A.1 安装

```bash
# Ubuntu / Debian
sudo apt install linux-tools-common linux-tools-generic linux-tools-$(uname -r)

# 验证
perf --version
```

如果第一次跑出现 `perf_event_paranoid` 权限错误：

```bash
echo 1 | sudo tee /proc/sys/kernel/perf_event_paranoid
# 永久：把上面值写到 /etc/sysctl.d/local.conf
```

### A.2 让 gemm 程序"按 order 跑一种"

为了让 perf 计数器只覆盖一种循环顺序，建议把 `gemm_loop_order.c` 改成**按命令行参数选 order**。
本目录下 `gemm_one.c` 给出了改造版（见文末附录），编译：

```bash
gcc -O2 -o gemm_one gemm_one.c
# 或 -O3 -march=native
```

### A.3 一键采集脚本（推荐）

把下面保存为 `bench_miss.sh`：

```bash
#!/usr/bin/env bash
set -e
ORDERS=(ijk ikj jik jki kij kji)
EVENTS="cycles,instructions,L1-dcache-loads,L1-dcache-load-misses,LLC-loads,LLC-load-misses"

# 让结果可复现：repeat 3 次取均值
for o in "${ORDERS[@]}"; do
  echo "=========== $o ==========="
  perf stat -r 3 -e $EVENTS ./gemm_one $o 2>&1 | tee perf_${o}.txt
done
```

跑完会得到 `perf_ikj.txt` ... `perf_kji.txt` 共 6 个原始日志。

### A.4 perf 输出怎么读

典型输出（节选）：

```
 Performance counter stats for './gemm_one ikj' (3 runs):

      1,500,000,000      cycles                                                        ( +-  0.10% )
      4,000,000,000      instructions              #    2.66  insn per cycle           ( +-  0.05% )
      1,200,000,000      L1-dcache-loads                                               ( +-  0.20% )
        150,000,000      L1-dcache-load-misses     #   12.50% of all L1-dcache hits   ( +-  0.30% )
         15,000,000      LLC-loads                                                     ( +-  0.40% )
            300,000      LLC-load-misses           #    2.00% of all LL-cache hits    ( +-  0.50% )

       0.398 +- 0.002 seconds time elapsed
```

读法：
- 第 4 行末：`12.50% of all L1-dcache hits` → **L1 miss rate**
- 第 6 行末：`2.00% of all LL-cache hits` → **LLC miss rate**
- `insn per cycle` = IPC，倒数是 CPI（**(c) 瓶颈说明的核心指标**）

把 6 种顺序的数字汇总成一张表，就是 (c) 的硬证据。

### A.5 进阶：L2 miss

L2 计数在不同 CPU 上事件名不一样：

```bash
# AMD Zen 系列（包括 Ryzen 7 9700X / Zen 5）：用裸事件
perf list | grep -i l2_cache    # 列出可用事件
# 常见：l2_cache_accesses_from_dc_misses, l2_cache_misses_from_dc_misses

perf stat -r 3 \
  -e l2_cache_accesses_from_dc_misses \
  -e l2_cache_misses_from_dc_misses \
  ./gemm_one ikj
```

```bash
# Intel：
perf stat -e mem_load_retired.l2_hit,mem_load_retired.l2_miss ./gemm_one ikj
```

---

## 路径 B：WSL2（Windows 上跑 Linux perf）

> 适合：本地是 Windows 的同学，不想搭虚拟机。

```powershell
# Windows PowerShell 管理员
wsl --install -d Ubuntu-22.04
```

进入 WSL 后基本沿用路径 A。**注意：**
- WSL2 的 perf 必须使用 **WSL2 自带内核**（默认即是）；
- 部分 PMU 事件可能不可用（被 hypervisor 屏蔽）。`L1-dcache-load-misses` 和 `LLC-load-misses` 通常可用，足够 (c) 题分析。

---

## 路径 C：Valgrind / Cachegrind（无需硬件 PMU）

> 适合：跑在 VM、容器或没有 PMU 权限的机器；**软件模拟 cache，结果稳定可复现，但比真实硬件慢约 10–50×**。

```bash
sudo apt install valgrind

# 模拟一个典型 L1 + LL 配置
valgrind --tool=cachegrind \
  --I1=32768,8,64 --D1=49152,12,64 --LL=33554432,16,64 \
  ./gemm_one ikj
```

参数含义：`size,assoc,line`，应当对照真实机器 `lscpu | grep -i cache` 设置。

输出 `cachegrind.out.<pid>` + 屏幕上汇总：

```
==12345== D refs:        1,200,000,000  (..)
==12345== D1  misses:      150,000,000  (..)
==12345== LLd misses:        2,000,000  (..)
==12345== D1  miss rate:        12.5%
==12345== LLd miss rate:         0.2%
```

直接得到 D1（L1d）和 LLd（LLC）miss rate，**适合做对照验证**：硬件 perf 和 cachegrind 数字接近时说明结论稳健。

---

## 路径 D：AMD uProf（Windows 原生，针对 Ryzen）

> 适合：(a) 那台 Ryzen 7 9700X 在 Windows 系统下跑。

1. 下载 [AMD uProf](https://www.amd.com/en/developer/uprof.html)（免费）
2. New CPU Profile → Cache Analysis → 选 `gemm_one.exe ikj`
3. 跑完看 "L1 DC Misses / Accesses"、"L2 DC Misses"、"LLC Misses"

uProf 会直接给柱状图和热点函数，但导出原始日志稍麻烦，做截图放进 PPT 也够用。

Intel CPU 的对应工具是 **Intel VTune Profiler**（同样免费）。

---

## 推荐操作顺序（最少工作量出最完整数据）

按你们组的 (a) 环境（Linux + Ryzen 7 9700X），建议：

1. **必做** —— 路径 A：拿 6 组 `perf_*.txt`，做出 6×3 表（L1 miss rate / LLC miss rate / IPC）。
2. **二选一交叉验证**：
   - 路径 C cachegrind 跑 `ikj` 和 `jki` 各一次，对比 perf 数值。
   - 或路径 D uProf 跑这两个并截图。
3. **(c) PPT 上展示**：
   - 一张柱状图：6 顺序的 L1 miss rate
   - 一张柱状图：最快/最慢的 L1 / L2 / LLC 三条
   - 一句话结论：差距主要来自 L1（87.5% pp 差），L2/LLC 由于预取失效继续放大

---

## 附录：`gemm_one.c`（按参数选循环顺序）

```c
// 编译: gcc -O2 -o gemm_one gemm_one.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define N 1001
static double A[N][N], B[N][N], C[N][N];

#define LOOP(o1,o2,o3,e1,e2,e3) \
    for (int o1=0;o1<N;o1++) for (int o2=0;o2<N;o2++) for (int o3=0;o3<N;o3++) \
        C[e1][e2] += A[e1][e3] * B[e3][e2];

int main(int argc, char** argv){
    if(argc<2){fprintf(stderr,"usage: %s {ijk|ikj|jik|jki|kij|kji}\n",argv[0]);return 1;}
    // 初始化（写一遍触发缺页，避免计入测时）
    for (int i=0;i<N;i++) for (int j=0;j<N;j++){A[i][j]=i*0.001+j*0.0001; B[i][j]=j*0.001-i*0.0001; C[i][j]=0;}

    const char* o = argv[1];
    if(!strcmp(o,"ijk")) { LOOP(i,j,k,i,j,k) }
    else if(!strcmp(o,"ikj")) { LOOP(i,k,j,i,j,k) }
    else if(!strcmp(o,"jik")) { LOOP(j,i,k,i,j,k) }
    else if(!strcmp(o,"jki")) { LOOP(j,k,i,i,j,k) }
    else if(!strcmp(o,"kij")) { LOOP(k,i,j,i,j,k) }
    else if(!strcmp(o,"kji")) { LOOP(k,j,i,i,j,k) }
    else { fprintf(stderr,"unknown order: %s\n",o); return 1; }

    // 防止编译器优化掉
    double s=0; for (int i=0;i<N;i++) s+=C[i][i];
    fprintf(stderr,"trace=%.6f\n",s);
    return 0;
}
```

把这一份和 `bench_miss.sh` 跑一遍就有 (c) 全部硬证据，**总耗时 < 5 分钟**。
