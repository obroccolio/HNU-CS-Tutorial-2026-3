/*
╔════════════════════════════════════════════════════════════════╗
║                        题目 e) 实验代码                         ║
║              矩阵规模对性能的影响（Cache 冲突陷阱）              ║
╚════════════════════════════════════════════════════════════════╝

【编译命令】
    gcc -O3 -march=native -o test_e test_e.c -lm

【运行命令】
    ./test_e

【预期耗时】
    约 2~5 分钟（因为要测试多个规模，每个规模测 3 遍）

【注意事项】
    - 必须使用 -O3 优化，否则看不到真实的 Cache 效应
    - 使用 -march=native 可以启用 CPU 特定的优化
    - 耐心等待，不要中断！
    - 如果想快速测试，可以减少 trials 值（当前为 3）
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// 矩阵乘法的最优顺序：kij
void matmul_kij(double *A, double *B, double *C, int M, int K, int N) {
    memset(C, 0, M * N * sizeof(double));
    
    for(int k = 0; k < K; k++)
        for(int i = 0; i < M; i++)
            for(int j = 0; j < N; j++)
                C[i*N + j] += A[i*K + k] * B[k*N + j];
}

// 性能测量
double measure_time_ms(int size) {
    // 分配矩阵
    double *A = (double*)malloc(size * size * sizeof(double));
    double *B = (double*)malloc(size * size * sizeof(double));
    double *C = (double*)malloc(size * size * sizeof(double));
    
    // 初始化（随意填充）
    for(int i = 0; i < size * size; i++) {
        A[i] = (double)(i % 10);
        B[i] = (double)((i + 7) % 10);
    }
    
    // 计时
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    matmul_kij(A, B, C, size, size, size);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    long seconds = end.tv_sec - start.tv_sec;
    long ns = end.tv_nsec - start.tv_nsec;
    if (start.tv_nsec > end.tv_nsec) {
        --seconds;
        ns += 1000000000;
    }
    
    double elapsed = (double)seconds * 1000.0 + (double)ns / 1000000.0;
    
    // 释放
    free(A);
    free(B);
    free(C);
    
    return elapsed;
}

// 计算 GFLOPS（每秒十亿浮点操作数）
double calculate_gflops(int size, double time_ms) {
    double flops = 2.0 * size * size * size;  // 矩阵乘法的运算量
    double gflops = flops / (time_ms / 1000.0) / 1e9;
    return gflops;
}

int main() {
    printf("═══════════════════════════════════════════════════════════\n");
    printf("题目 e) 矩阵规模对性能的影响（Cache 冲突陷阱）\n");
    printf("═══════════════════════════════════════════════════════════\n\n");
    
    printf("%-8s | %-12s | %-12s | %-12s\n", "规模", "耗时(ms)", "GFLOPS", "备注");
    printf("─────────┼──────────────┼──────────────┼──────────────\n");
    
    // 测试不同规模（关键规模附近）
    int sizes[] = {
        990, 995, 1000, 1001,        // 原始规模附近
        1020, 1022, 1023, 1024,      // 2^10 附近（冲突陷阱）
        1025, 1026, 1030, 1032,      // 1024 之后
        2040, 2044, 2048, 2050       // 2^11 附近
    };
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    
    for(int idx = 0; idx < num_sizes; idx++) {
        int size = sizes[idx];
        
        // 运行多次求平均（消除波动）
        double total_time = 0;
        int trials = 3;
        for(int t = 0; t < trials; t++) {
            total_time += measure_time_ms(size);
        }
        double avg_time = total_time / trials;
        double gflops = calculate_gflops(size, avg_time);
        
        // 标注特殊规模
        const char *note = "";
        if(size == 1001) note = "← 原始";
        else if(size == 1024) note = "← 2^10（陷阱）";
        else if(size == 2048) note = "← 2^11（陷阱）";
        
        printf("%-8d | %12.2f | %12.2f | %s\n", size, avg_time, gflops, note);
    }
    
    printf("═══════════════════════════════════════════════════════════\n\n");
    
    // 详细分析
    printf("【详细分析】\n");
    printf("1. 1001 规模（非 2^n）：\n");
    printf("   - 每行 1001 × 8 = 8008 字节（不规则）\n");
    printf("   - Address 映射"随机"，Cache 冲突少\n");
    printf("   - 性能较好\n\n");
    
    printf("2. 1024 规模（2^10）：\n");
    printf("   - 每行 1024 × 8 = 8192 = 2^13 字节（规则）\n");
    printf("   - Address 映射规律，易产生 Cache 冲突\n");
    printf("   - 性能下降 20~40%%\n\n");
    
    printf("3. 2048 规模（2^11）：\n");
    printf("   - 冲突更严重（更大的步长）\n");
    printf("   - 性能继续下降\n\n");
    
    printf("【为什么会这样？】\n");
    printf("Cache 采用"组关联"结构，多个地址映射到同一 Cache 组。\n");
    printf("2^n 规模的矩阵地址映射规律，多行数据同时争用同一 Cache 组，\n");
    printf("导致频繁的冲突不命中。而非 2^n 规模则相对"混乱"，冲突少。\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    return 0;
}
