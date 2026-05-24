# 选题3 —— d) & e) 实验报告

## 实验环境

| 项目 | 本机配置 |
|------|----------|
| CPU | AMD Ryzen 7 9700X (Zen 5, 8C/16T, 最高5.58GHz) |
| 内存 | 96GB DDR5 |
| 系统盘 | Samsung 990 PRO 2TB (PCIe 4.0 x4 NVMe) |
| 操作系统 | Ubuntu (Linux 6.8.0-111-generic) |
| 文件系统 | ext4 |

---

## d) SSD 与 HDD 的随机 4K 读写性能对比

### d.1 测试方法

使用 fio (v3.28) 进行随机 4K 读写测试，参数如下：
- 块大小：4KB
- I/O 深度：1（模拟单线程同步访问）和 32（模拟高并发异步访问）
- 引擎：libaio (Linux AIO)
- 直接 I/O：绕过页缓存 (O_DIRECT)
- 测试时长：30秒
- 测试文件大小：1GB
- 测试模式：randread / randwrite

### d.2 测试结果

#### 本机实测数据（Samsung 990 PRO 2TB, NVMe SSD）

| 测试项 | QD=1 | QD=32 |
|--------|------|-------|
| 随机4K读 IOPS | 21,370 | 583,901 |
| 随机4K写 IOPS | 70,232 | 458,704 |
| 随机4K读带宽 | 83.5 MB/s | 2,280.9 MB/s |
| 随机4K写带宽 | 274.3 MB/s | 1,791.8 MB/s |
| 随机4K读延迟(avg) | 45.7 μs | 53.3 μs |
| 随机4K写延迟(avg) | 12.9 μs | 67.8 μs |

#### 延迟百分位分布（实测）

| 百分位 | 随机4K读 QD=1 | 随机4K读 QD=32 | 随机4K写 QD=1 | 随机4K写 QD=32 |
|--------|---------------|----------------|---------------|----------------|
| P50 | 46.3 μs | 51.5 μs | 13.0 μs | 68.1 μs |
| P99 | 48.9 μs | 84.5 μs | 14.1 μs | 81.4 μs |
| P99.9 | 64.3 μs | 122.4 μs | 17.3 μs | 138.2 μs |

#### 对比数据（来源：厂商官方 Datasheet）

| 设备类型 | 随机4K读 IOPS (QD=1) | 随机4K读 IOPS (QD=32) | 随机4K写 IOPS (QD=1) | 随机4K写 IOPS (QD=32) | 随机4K读延迟(avg) | 数据来源 |
|----------|----------------------|------------------------|----------------------|------------------------|-------------------|----------|
| **NVMe SSD (Samsung 990 PRO 2TB)** | 21,370 | 583,901 | 70,232 | 458,704 | 45.7 μs | 本机 fio 实测 |
| SATA SSD (Samsung 860 EVO 1TB) | 10,000 | 98,000 | 42,000 | 90,000 | ~100 μs | Samsung 860 EVO Datasheet Rev1.0 (2018) [1] |
| HDD (Seagate Barracuda ST1000DM003, 7200rpm) | ~79 | ~150 | ~79 | ~150 | ~12,700 μs (12.7ms) | Seagate Datasheet DS1737-1 (2011) [2] |

> 注：
> - 860 EVO 数据来自官方 Datasheet 第5页 "Performance" 表格，测试条件为 IOmeter 1.1.0, Intel Core i5-3550, DDR3 4GB, Win7 x64
> - HDD IOPS 由官方寻道时间推算：平均访问时间 = 寻道时间(8.5ms) + 旋转延迟(60s/7200/2=4.17ms) = 12.67ms → 1000/12.67 ≈ 79 IOPS (QD=1)
> - HDD QD=32 时通过 NCQ (Native Command Queuing) 优化寻道路径，实际约 100-180 IOPS

**性能差距总结：**
- NVMe vs HDD：随机4K读 IOPS 差距约 **270倍 (QD=1) ~ 3,900倍 (QD=32)**
- NVMe vs SATA SSD：约 **2.1倍 (QD=1) ~ 6.0倍 (QD=32)**
- SATA SSD vs HDD：约 **127倍 (QD=1) ~ 650倍 (QD=32)**

### d.3 延迟分布分析

#### NVMe SSD 延迟分布特征（实测）

本机 Samsung 990 PRO 随机4K读延迟分布呈现**极度集中的单峰**特征：
- P1 = 37.6 μs, P50 = 46.3 μs, P95 = 47.4 μs — 95%的请求落在 38-47 μs 的极窄区间内
- P99 = 48.9 μs, P99.9 = 64.3 μs — 尾延迟非常收敛
- 极少数离群点可达 ~218 μs (P99.99)，通常由以下原因导致：
  - 内部 GC（垃圾回收）
  - FTL 映射表缓存未命中
  - 写放大导致的后台数据搬移

#### HDD 延迟分布特征

HDD 的随机4K读延迟分布呈现**明显长尾**：
- 主峰在 4-8 ms（取决于寻道距离和旋转等待）
- 长尾可延伸至 20-50 ms（跨磁道大范围寻道 + 接近一整圈旋转等待）
- 延迟由两个物理过程叠加：
  - **寻道时间**：磁头移动到目标磁道，平均约 4-8 ms
  - **旋转等待**：等待目标扇区转到磁头下方，7200rpm 平均约 4.17 ms

#### "SSD 是否真的没有寻道时间？"

严格来说，SSD **没有机械寻道时间**，但存在"逻辑寻道"开销：
1. **FTL 地址翻译**：逻辑地址 → 物理 NAND 页地址的映射查找（通常缓存在 DRAM 中，~1-5 μs）
2. **NAND 页读取**：从闪存芯片读出数据（TLC ~50-75 μs, QLC ~100-150 μs）
3. **通道/芯片竞争**：多个请求竞争同一 NAND die 时产生排队延迟

因此 SSD 的"寻道"本质上是电子信号传输和逻辑查表，比 HDD 的机械运动快 **2-3个数量级**，但并非零延迟。

### d.4 延迟分布示意

```
延迟分布对比（示意，基于实测 + HDD典型值）

NVMe SSD 随机4K读 (实测, QD=1):
频率                    ← 95%请求集中在此窄区间
 ▌                     ┌──────┐
 █                     │      │
 █                     │      │
 █▌                    │      │
 ██                    │      │
 ██▌                   │      │         (P99.99 尾延迟)
 ███                   │      │               ·
─┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──→ 延迟
 0  10 20 30 38 47 50 60 80 100  200   μs
            ↑     ↑
           P1    P95

HDD 随机4K读 (典型值):
频率
         ▌
        ██
       ███▌
      █████
     ██████▌                    (长尾: 大范围寻道)
    ████████▌·  ·  ·  ·  ·  ·  ·  ·
─┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──→ 延迟
 0  2  4  6  8  10 12 14 16 18 20  ms
```

---

## e) 不同硬件配置的对比分析

### e.1 本机定位

本机属于**高端桌面/工作站**级别：
- Zen 5 架构 + DDR5 + PCIe 4.0 NVMe
- 代表 2024 年消费级存储性能的上限水平

### e.2 各类硬件配置对比（含虚拟典型数据）

#### 随机4K读性能对比

| 配置 | 设备 | IOPS (QD=1) | IOPS (QD=32) | 平均延迟 | 数据来源 |
|------|------|-------------|--------------|----------|----------|
| **本机 (桌面)** | Samsung 990 PRO 2TB (NVMe, PCIe 4.0) | 21,370 | 583,901 | 45.7 μs | fio 实测 |
| 笔记本 (典型) | WD Black SN770 1TB (NVMe, PCIe 4.0) | — | 740,000 | — | WD 官方产品规格 [3] |
| 服务器 (企业) | Intel Optane P5800X 800GB (PCIe 4.0) | — | 1,500,000 | <6 μs (QD=1) | Intel Product Brief [4] |
| 旧笔记本 | Samsung 860 EVO 1TB (SATA SSD) | 10,000 | 98,000 | ~100 μs | Samsung Datasheet [1] |
| 台式机 (机械) | Seagate Barracuda ST1000DM003 (7200rpm) | ~79 | ~150 | ~12.7 ms | Seagate Datasheet [2] |

> 注：SN770 和 P5800X 的 QD=1 数据官方未单独公布，QD=32 为官方标称峰值条件下的数据。

#### DDR4 vs DDR5 对存储子系统的间接影响

| 项目 | DDR4-3200 | DDR5-5600 |
|------|-----------|-----------|
| 内存带宽 | ~50 GB/s | ~85 GB/s |
| 内存延迟 | ~80 ns | ~90 ns (首字延迟略高) |
| 对 NVMe 性能影响 | 间接：页缓存命中时走内存通道 | 更高带宽利于大文件缓存读取 |
| 对 FTL 映射表影响 | 映射表在 SSD DRAM 中，与主机内存无关 | 同左 |

> DDR5 首字延迟略高于 DDR4（CAS latency 绝对时间更长），但带宽优势在大块顺序访问场景下显著。对随机4K小IO场景，主机内存代际差异影响有限，瓶颈在 NAND 本身。

#### SATA SSD vs NVMe SSD 关键差异

| 维度 | SATA SSD | NVMe SSD |
|------|----------|----------|
| 接口协议 | AHCI (单队列, 深度32) | NVMe (最多64K队列, 每队列64K深度) |
| 接口带宽上限 | ~600 MB/s | PCIe 4.0 x4: ~7,000 MB/s |
| 随机4K读 (QD=1) | ~10,000 IOPS | ~20,000+ IOPS |
| 随机4K读 (QD=32) | ~90,000-100,000 IOPS | ~700,000-1,000,000 IOPS |
| 高队列深度优势 | 受限于单队列，扩展性差 | 多队列并行，线性扩展 |
| 典型使用场景 | 旧平台升级、预算方案 | 主流新装机、高性能需求 |

**核心结论：** NVMe 在高并发（高QD）场景下优势最为显著，这是协议层面（多队列）带来的本质差异。在 QD=1 的单线程场景下，NVMe 相对 SATA SSD 的优势主要体现在协议开销更低（~2倍），而非数量级差距。

### e.3 各配置特点总结

| 类型 | 存储特点 |
|------|----------|
| **笔记本** | 多为单盘 NVMe，受散热限制可能降速；容量通常 512GB-1TB |
| **台式机** | 可多盘组合（NVMe系统盘 + HDD仓储盘），散热好，持续性能稳定 |
| **服务器** | 企业级 SSD（更高耐久度、稳定延迟）、可能配备 Optane 做缓存层、RAID 阵列 |
| **DDR4 平台** | PCIe 3.0 为主，NVMe 带宽上限 ~3.5 GB/s，随机性能差异不大 |
| **DDR5 平台** | PCIe 4.0/5.0，NVMe 带宽上限 7-14 GB/s，CPU 缓存更大 |

---

## 待补充数据

当前报告数据状态：
- **本机 NVMe (990 PRO)** — fio 实测 ✓
- **SATA SSD (860 EVO)** — Samsung 官方 Datasheet Rev 1.0 (2018) ✓
- **HDD (Seagate Barracuda 7200)** — Seagate 官方 Datasheet DS1737-1 (2011)，IOPS 为寻道时间推算 ✓
- **WD SN770** — WD 官方产品规格页标称值 ✓
- **Intel Optane P5800X** — Intel 官方 Product Brief 标称值 ✓

如需进一步完善（可选）：
1. 借一台带 HDD 的机器跑 fio 实测 — 验证理论推算值，获取真实延迟分布直方图
2. 借一台 SATA SSD 的机器跑 fio 实测 — 获取 QD=1 精确延迟百分位数据
3. 其他同学的 NVMe SSD — 不同品牌/容量的横向对比

---

## 参考资料

1. [1] Samsung V-NAND SSD 860 EVO Data Sheet, Revision 1.0, December 2017 — 第5页 Performance 表格：4KB Ran. Read (QD1) = 10,000 IOPS, (QD32) = 98,000 IOPS; 4KB Ran. Write (QD1) = 42,000 IOPS, (QD32) = 90,000 IOPS. 测试条件：IOmeter 1.1.0, CrystalDiskMark v5.0.2, Intel Core i5-3550 @3.3GHz, DDR3 4GB. 来源：download.semiconductor.samsung.com
2. [2] Seagate Barracuda Data Sheet, DS1737-1-1111US, November 2011 — ST1000DM003: 7200 RPM, Seek Average Read <8.5ms, Write <9.5ms, Max Sustained Data Rate 210 MB/s, SATA 6Gb/s NCQ. 来源：seagate.com/files/staticfiles/docs/pdf/datasheet/
3. [3] WD Black SN770 NVMe SSD 官方产品规格 — 随机读取最高 740,000 IOPS, 随机写入最高 800,000 IOPS, 顺序读取 5,150 MB/s, 顺序写入 4,900 MB/s (1TB 型号). 来源：westerndigital.com 产品页
4. [4] Intel Optane SSD P5800X Product Brief — 随机读取 1,500,000 IOPS (QD=128), 随机写入 1,400,000 IOPS (QD=128), 读延迟 <6μs (QD=1), 顺序读取 7,200 MB/s, 顺序写入 5,400 MB/s. 来源：intel.com
5. [5] WD Blue PC Hard Drives Spec Sheet, 2879-771436 — WD60EZRZ: 5400 RPM Class, 持续传输率 175 MB/s, SATA 6Gb/s, 64MB Cache. 来源：products.wdc.com/library/SpecSheet/
6. [6] Samsung 990 PRO 2TB 官方规格 — 随机读取 1,200,000 IOPS, 随机写入 1,550,000 IOPS (QD=32, 线程16), 顺序读取 7,450 MB/s, 顺序写入 6,900 MB/s. 来源：samsung.com/semiconductor
7. [7] NVMe 规范 1.4 — 支持最多 65,535 个 I/O 队列，每队列 65,536 条目
8. [8] 7200rpm HDD 平均旋转延迟计算：60s / 7200rpm / 2 = 4.17ms
9. [9] fio 文档 v3.28 — Flexible I/O Tester, https://fio.readthedocs.io/
