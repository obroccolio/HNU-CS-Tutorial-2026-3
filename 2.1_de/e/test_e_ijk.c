#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<string.h>

/**
 * 问题 e：标准 ijk 直接矩阵乘法
 * 
 * 测试不同矩阵规模对性能的影响
 * 重点观察 1024 及周围规模是否出现性能跳变（缓存冲突）
 */

typedef struct {
    int size;
    double time_ms;
    double gflops;
} PerformanceData;

PerformanceData test_ijk(int n) {
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
    
    // 执行 ijk 顺序的矩阵乘法
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            double sum = 0.0;
            for (int k = 0; k < n; k++) {
                sum += A[i * n + k] * B[k * n + j];
            }
            C[i * n + j] = sum;
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
        // 防止编译器优化掉计算
        printf("Dummy: %lf\n", C[0]);
    }
    
    free(A);
    free(B);
    free(C);
    
    return result;
}

int main() {
    printf("=====================================================\n");
    printf("问题 e：ijk 直接矩阵乘法性能测试\n");
    printf("=====================================================\n\n");
    
    // 测试规模：1001 开始，重点观察 1024 附近
    int sizes[] = {
        1001, 1010, 1020, 1023, 1024, 1025, 1030, 1040, 1050, 1060, 1100, 1200, 1300, 1400, 1500
    };
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    
    printf("%-10s | %-12s | %-10s\n", "规模", "时间(ms)", "GFLOPS");
    printf("%-10s | %-12s | %-10s\n", "---", "---", "---");
    
    for (int i = 0; i < num_sizes; i++) {
        PerformanceData result = test_ijk(sizes[i]);
        printf("%-10d | %-12.2f | %-10.4f\n", result.size, result.time_ms, result.gflops);
        fflush(stdout);
    }
    
    printf("\n=====================================================\n");
    printf("测试完成\n");
    printf("=====================================================\n");
    
    return 0;
}
