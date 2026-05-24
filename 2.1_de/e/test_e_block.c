#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<string.h>

/**
 * 问题 e：ijkijk 分块矩阵乘法
 * 
 * 6层嵌套循环：块级 ijk 顺序 + 元素级 ijk 顺序
 * 块大小固定为 64x64
 * 
 * 目标：对比直接 ijk 和分块 ijkijk 的性能差异
 */

#define BLOCK_SIZE 64

typedef struct {
    int size;
    double time_ms;
    double gflops;
} PerformanceData;

PerformanceData test_block_ijkijk(int n) {
    PerformanceData result = {n, 0, 0};
    
    // 分配内存
    double *A = (double *)malloc((long)n * n * sizeof(double));
    double *B = (double *)malloc((long)n * n * sizeof(double));
    double *C = (double *)calloc((long)n * n, sizeof(double));
    
    if (!A || !B || !C) {
        fprintf(stderr, "Memory allocation failed for size %d\n", n);
        return result;
    }
    
    // 初始化矩阵
    for (long i = 0; i < (long)n * n; i++) {
        A[i] = 1.0 + (i % 1000) * 0.001;
        B[i] = 1.0 + (i % 1000) * 0.001;
    }
    
    struct timespec start, end;
    
    // 执行 ijkijk 分块矩阵乘法
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // 块级 ijk 循环
    for (int ii = 0; ii < n; ii += BLOCK_SIZE) {
        for (int jj = 0; jj < n; jj += BLOCK_SIZE) {
            for (int kk = 0; kk < n; kk += BLOCK_SIZE) {
                // 元素级 ijk 循环
                int ii_end = (ii + BLOCK_SIZE < n) ? ii + BLOCK_SIZE : n;
                int jj_end = (jj + BLOCK_SIZE < n) ? jj + BLOCK_SIZE : n;
                int kk_end = (kk + BLOCK_SIZE < n) ? kk + BLOCK_SIZE : n;
                
                for (int i = ii; i < ii_end; i++) {
                    for (int j = jj; j < jj_end; j++) {
                        for (int k = kk; k < kk_end; k++) {
                            C[i * n + j] += A[i * n + k] * B[k * n + j];
                        }
                    }
                }
            }
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double time_seconds = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    result.time_ms = time_seconds * 1000;
    
    // 计算 GFLOPS: 2 * n^3 / (time in seconds)
    long flops = 2LL * n * n * n;
    result.gflops = flops / (time_seconds * 1e9);
    
    // 验证结果（简单检查）
    if (C[0] < 0) {
        printf("Dummy: %lf\n", C[0]);
    }
    
    free(A);
    free(B);
    free(C);
    
    return result;
}

int main() {
    printf("=====================================================\n");
    printf("问题 e：ijkijk 分块矩阵乘法性能测试 (块大小:%d)\n", BLOCK_SIZE);
    printf("=====================================================\n\n");
    
    // 测试规模：1001 开始，重点观察 1024 附近
    int sizes[] = {
        1001, 1010, 1020, 1023, 1024, 1025, 1030, 1040, 1050, 1060, 1100, 1200, 1300, 1400, 1500
    };
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    
    printf("%-10s | %-12s | %-10s\n", "规模", "时间(ms)", "GFLOPS");
    printf("%-10s | %-12s | %-10s\n", "---", "---", "---");
    
    for (int i = 0; i < num_sizes; i++) {
        PerformanceData result = test_block_ijkijk(sizes[i]);
        printf("%-10d | %-12.2f | %-10.4f\n", result.size, result.time_ms, result.gflops);
        fflush(stdout);
    }
    
    printf("\n=====================================================\n");
    printf("测试完成\n");
    printf("=====================================================\n");
    
    return 0;
}
