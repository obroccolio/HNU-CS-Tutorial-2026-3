# 术语表（讲义外可能陌生的概念）

## Cache / 局部性

- Cache line：Cache 以“行”为单位搬运数据（常见 64B）。顺序访问能把一行里的多个元素都用上
- 空间局部性（Spatial locality）：访问 `x` 后，很快访问 `x` 附近的地址（顺序遍历数组）
- 时间局部性（Temporal locality）：访问 `x` 后，很快再次访问 `x`（重复使用同一小块数据）
- Stride（步长访问）：每次访问地址差固定且很大（如跨行访问 2D 数组），空间局部性差
- Miss rate：未命中次数 / 访问次数。分层统计：L1 miss、L2 miss、LLC(L3) miss
- 冲突不命中（Conflict miss）：即使容量够，也因组相联/映射导致冲突把数据挤出去（常见于“整齐尺寸”）
- 硬件预取（Hardware prefetcher）：CPU 预测“接下来会用到哪些 cache line”，提前加载；对顺序/固定步长更有效

## 性能计数 / 瓶颈分析

- CPI：Cycles Per Instruction，越高通常表示等待数据或分支等导致流水线停顿
- Stalled cycles：周期在等待（常见为等待内存）而不能退休指令
- 带宽瓶颈（Bandwidth bound）：算术强度低、数据搬运跟不上，性能被内存/LLC 带宽限制
- 计算瓶颈（Compute bound）：数据能及时到位，性能主要受 FMA/ALU 吞吐限制

## 矩阵乘法常用优化词汇

- 转置（Transpose）：把列访问变成行访问（尤其对 `B[k][j]` 这类“按列取”很关键）
- 分块/分片（Blocking/Tiling）：把大矩阵拆成小块，让工作集落入 L1/L2，提高复用
- 循环交换（Loop interchange）：改变循环嵌套顺序以改变访存模式（本题核心）
- 循环展开（Unrolling）：减少循环开销、提升指令级并行（ILP），但会增大寄存器压力
