# 使用 Cachegrind 采集 Cache Miss Rate · 完整操作步骤

> 适用场景：**任何 Linux 环境**（裸机、VM、WSL2 都行）。
> 不依赖 PMU 硬件计数器，纯软件模拟 → 100% 可复现。
> 
> 唯一代价：运行速度慢 30-50 倍（N=1001 每个顺序约 1-3 分钟）。

---

## 0. 原理 30 秒

Cachegrind 是 Valgrind 的一个工具，它：
1. 把你的程序每条指令翻译执行
2. 对每次内存访问，模拟一个**两层 cache 模型**（D1 = L1d，LL = L3）
3. 统计出精确的 miss 次数和 miss rate

| cachegrind 层 | 对应真实硬件 | 你输入的参数 |
|---|---|---|
| D1 | L1 Data Cache | `--D1=大小,关联度,行大小` |
| LL | Last Level (L3) | `--LL=大小,关联度,行大小` |

> 注意：**cachegrind 不建模 L2**。对 (c) 题来说 L1 和 L3 已足够定位瓶颈。

---

## 1. 安装 Valgrind

```bash
sudo apt update
sudo apt install valgrind
valgrind --version   # 应输出 valgrind-3.18.x 或更高
```

---

## 2. 编译被测程序

```bash
cd ~/Desktop/share/2.1\(b\)\(c\)
gcc -O2 -o gemm_one gemm_one.c
```

> 不要用 `-g`（加调试信息会让运行更慢）。
> 但如果之后想看逐行 miss 分布（`cg_annotate`），则加 `-g`。

---

## 3. 确认你 CPU 的 Cache 参数

```bash
lscpu | grep -i cache
```

示例输出（AMD Ryzen 7 9700X）：

```
L1d cache:     48 KiB  (12-way)
L1i cache:     32 KiB  (8-way)
L2 cache:      1 MiB   (16-way)
L3 cache:      32 MiB  (16-way)
```

换算成 cachegrind 参数格式 `字节数,关联度,行大小`：

| 层 | 参数 | 计算 |
|---|---|---|
| D1 (L1d) | `--D1=49152,12,64` | 48 × 1024 = 49152 B |
| LL (L3)  | `--LL=33554432,16,64` | 32 × 1024 × 1024 = 33554432 B |

> 如果你的 CPU 不同，改成你自己的值即可。
> cache line 几乎所有现代 x86 都是 **64 B**。

---

## 4. 跑单个顺序（手动体验一次）

```bash
valgrind --tool=cachegrind \
    --D1=49152,12,64 \
    --LL=33554432,16,64 \
    --cachegrind-out-file=cg_ikj.out \
    ./gemm_one ikj
```

### 屏幕输出（stderr）解读

等 1-3 分钟后会看到：

```
==12345== Cachegrind, a cache and branch-prediction profiler
==12345== Copyright (C) 2002-2017, and GNU GPL'd, by Nicholas Nethercote et al.
==12345== Using Valgrind-3.18.1 and LibVEX; rerun with -h for copyright info
==12345== Command: ./gemm_one ikj
==12345== 
[order=ikj]  time=45.8234s  gflops=0.044  trace=330825.660165
==12345== 
==12345== I   refs:      6,025,123,456
==12345== I1  misses:            2,345
==12345== LLi misses:            1,890
==12345== I1  miss rate:          0.00%
==12345== LLi miss rate:          0.00%
==12345== 
==12345== D   refs:      3,012,345,678  (2,008,230,456 rd   + 1,004,115,222 wr)
==12345== D1  misses:      126,234,567  (  125,123,456 rd   +     1,111,111 wr)
==12345== LLd misses:            3,456  (        2,345 rd   +         1,111 wr)
==12345== D1  miss rate:          4.2%  (          6.2%     +           0.1%  )
==12345== LLd miss rate:          0.0%  (          0.0%     +           0.0%  )
==12345== 
==12345== LL refs:         126,236,912  (  125,125,801 rd   +     1,111,111 wr)
==12345== LL misses:             5,346  (        4,235 rd   +         1,111 wr)
==12345== LL miss rate:           0.0%  (          0.0%     +           0.0%  )
```

### 你需要的 4 个关键数字

| 找哪一行 | 取什么 | 含义 |
|---|---|---|
| `D   refs:` | 第一个数字 | 数据访问总次数 |
| `D1  misses:` | 第一个数字 | L1d miss 次数 |
| **`D1  miss rate:`** | **第一个百分比** | **← L1 miss rate（这就是 (c) 题要的）** |
| **`LLd miss rate:`** | **第一个百分比** | **← L3 miss rate** |

> `time=45.82s` 是 cachegrind 模拟下的时间（慢了 ~50×），不是真实时间。
> 真实时间看 `results.csv` 里的数据。

---

## 5. 批量测 6 顺序 · 一键脚本

你上次已经成功跑过的 `bench_cachegrind.sh`，在这里完整列出：

```bash
#!/usr/bin/env bash
set -e
ORDERS=(ijk ikj jik jki kij kji)

echo "order,d1_refs,d1_misses,d1_miss_rate,ll_refs,ll_misses,ll_miss_rate" > cg_summary.csv

for o in "${ORDERS[@]}"; do
  echo "===========  $o  ==========="
  log="cachegrind_${o}.txt"
  
  valgrind --tool=cachegrind \
      --D1=49152,12,64 \
      --LL=33554432,16,64 \
      --cachegrind-out-file=cg_${o}.out \
      ./gemm_one "$o" 2> "$log"

  # 提取数值
  d1_refs=$(awk '/D   refs:/{gsub(/,/,"",$4); print $4; exit}' "$log")
  d1_miss=$(awk '/D1  misses:/{gsub(/,/,"",$4); print $4; exit}' "$log")
  d1_rate=$(awk '/D1  miss rate:/{print $4; exit}' "$log")
  ll_refs=$(awk '/LL refs:/{gsub(/,/,"",$3); print $3; exit}' "$log")
  ll_miss=$(awk '/LL misses:/{gsub(/,/,"",$3); print $3; exit}' "$log")
  ll_rate=$(awk '/LL miss rate:/{print $4; exit}' "$log")

  echo "  D1 miss rate = $d1_rate    LL miss rate = $ll_rate"
  echo "$o,$d1_refs,$d1_miss,$d1_rate,$ll_refs,$ll_miss,$ll_rate" >> cg_summary.csv
done

echo
echo "=========== 最终结果 (cg_summary.csv) ==========="
column -s, -t < cg_summary.csv
```

### 执行

```bash
chmod +x bench_cachegrind.sh
./bench_cachegrind.sh
```

### 总耗时

- N=1001，6 个顺序，每个 1-3 分钟
- **总计约 10-15 分钟**
- 如果时间紧，只跑最快和最慢：

```bash
# 只测 ikj 和 jki（2 分钟搞定）
for o in ikj jki; do
  valgrind --tool=cachegrind --D1=49152,12,64 --LL=33554432,16,64 \
      --cachegrind-out-file=cg_${o}.out ./gemm_one "$o" 2> cachegrind_${o}.txt
  echo "=== $o ==="; tail -15 cachegrind_${o}.txt
done
```

---

## 6. 你上次的实测结果（已验证正确）

```
order  d1_miss_rate  ll_miss_rate  档位
ikj    4.2%          0.0%          ★★★ 最佳
kij    4.2%          0.0%          ★★★ 最佳
ijk    56.0%         0.0%          ★★☆ 中等
jik    56.0%         0.0%          ★★☆ 中等
jki    66.5%         0.0%          ★☆☆ 最差
kji    66.5%         0.0%          ★☆☆ 最差
```

### (c) 题答案

- **Δ L1 miss rate = 66.5% − 4.2% = 62.3 pp**
- **Δ L3 miss rate ≈ 0 pp**（24 MB < 32 MB，全放得下）
- **瓶颈 = L1（+ L2，cachegrind 未建模）**

---

## 7. 进阶 · 逐行热点定位（`cg_annotate`）

如果老师问你"具体哪条语句 miss 最多"：

```bash
# 先用 -g 重编译
gcc -O2 -g -o gemm_one gemm_one.c

# 跑 jki（miss 最多，对比明显）
valgrind --tool=cachegrind --D1=49152,12,64 --LL=33554432,16,64 \
    --cachegrind-out-file=cg_jki_annotate.out ./gemm_one jki

# 生成逐行报告
cg_annotate cg_jki_annotate.out gemm_one.c
```

输出示例（截取）：

```
         Dr       D1mr      DLmr
          .          .         .   for (int o1 = 0; o1 < N; o1++)
          .          .         .       for (int o2 = 0; o2 < N; o2++)
          .          .         .           for (int o3 = 0; o3 < N; o3++)
2,006,012,001  668,997,333    0             C[i][j] += A[i][k] * B[k][j];
```

| 列 | 含义 |
|---|---|
| `Dr` | 数据读次数 |
| `D1mr` | L1d 读 miss 次数 |
| `DLmr` | LL 读 miss 次数 |

你可以看到：**所有 miss 集中在内层那条乘加语句**。这就是铁证。

---

## 8. 常见问题

| 问题 | 解决 |
|---|---|
| `valgrind: command not found` | `sudo apt install valgrind` |
| 跑了 10 分钟还没结束 | 正常，N=1001 的 jki 在 cachegrind 下可能要 3 分钟 |
| `D1 miss rate` 全是 0% | 检查是否真的跑了 gemm（不是空程序退出） |
| miss rate 和 perf 不一样 | 正常。cachegrind 是确定性模拟（无噪声），perf 是硬件采样（有 OS 干扰）。两者差 5-10% 很正常 |
| `--D1` 参数报 `too small` | 确保字节数 ≥ 关联度 × 行大小，如 49152 ≥ 12×64 = 768 ✓ |
| 想看 L2 | cachegrind 不建模 L2。想看 L2 必须用 perf（见 perf 流程文档） |

---

## 9. 一页纸速查（答辩时贴在手边）

```bash
# 安装
sudo apt install valgrind

# 编译
gcc -O2 -o gemm_one gemm_one.c

# 测单个
valgrind --tool=cachegrind --D1=49152,12,64 --LL=33554432,16,64 \
    --cachegrind-out-file=cg_ikj.out ./gemm_one ikj 2>&1 | tail -15

# 测全部 6 个
./bench_cachegrind.sh

# 看结果
column -s, -t < cg_summary.csv

# 逐行定位（需 -g 编译）
cg_annotate cg_jki_annotate.out gemm_one.c | head -50
```

**核心结论：ikj 4.2% vs jki 66.5% → Δ 62.3 pp → 瓶颈在 L1。**
