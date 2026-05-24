/*
╔════════════════════════════════════════════════════════════════╗
║                        题目 d) 实验代码                         ║
║                  转置 B 矩阵的性能影响分析                      ║
╚════════════════════════════════════════════════════════════════╝

【编译命令】
    gcc -O3 -march=native -o test_d test_d.c -lm

【运行命令】
    ./test_d

【预期耗时】
    约 20~40 秒（取决于 CPU 速度）

【注意事项】
    - 必须使用 -O3 优化，否则看不到真实的 Cache 效应
    - 使用 -march=native 可以启用 CPU 特定的优化
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define M 1001
#define K 1001
#define N 1001

// ========== 版本 1：不转置 B（原始 kij 顺序）==========
void matmul_kij_original(double *A, double *B, double *C) {
    // 初始化 C
    memset(C, 0, M * N * sizeof(double));
    
    for(int k = 0; k < K; k++)
        for(int i = 0; i < M; i++)
            for(int j = 0; j < N; j++)
                C[i*N + j] += A[i*K + k] * B[k*N + j];
                //                          ^^^^^^^^
                //                          列优先访问，步长为 N
}

// ========== 版本 2：转置 B 后进行乘法 ==========
void matmul_kij_with_transpose(double *A, double *B, double *C) {
    // 第一步：转置 B 为 B_T
    double *B_T = (double*)malloc(K * N * sizeof(double));
    
    // 转置（行列互换）
    for(int k = 0; k < K; k++)
        for(int j = 0; j < N; j++)
            B_T[j*K + k] = B[k*N + j];
            //  行优先        列优先
    
    // 第二步：用转置后的 B_T 进行乘法
    memset(C, 0, M * N * sizeof(double));
    
    for(int k = 0; k < K; k++)
        for(int i = 0; i < M; i++)
            for(int j = 0; j < N; j++)
                C[i*N + j] += A[i*K + k] * B_T[j*K + k];
                //                          '^^^^^^^
                //                          现在行优先，步长为 K！
    
    // 第三步：释放临时数组
    free(B_T);
}

// ========== 性能测量函数 ==========
double measure_time_ms(void (*func)(double*, double*, double*), 
                       double *A, double *B, double *C) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    func(A, B, C);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    long seconds = end.tv_sec - start.tv_sec;
    long ns = end.tv_nsec - start.tv_nsec;
    if (start.tv_nsec > end.tv_nsec) {
        --seconds;
        ns += 1000000000;
    }
    
    return (double)seconds * 1000.0 + (double)ns / 1000000.0;
}

// ========== 数据初始化 ==========
void init_matrices(double *A, double *B) {
    for(int i = 0; i < M * K; i++)
        A[i] = (double)(i % 10);  // 填充可重复的值
    for(int i = 0; i < K * N; i++)
        B[i] = (double)((i + 7) % 10);
}

// ========== 验证两个版本的结果是否一致 ==========
int verify_result(double *C1, double *C2) {
    for(int i = 0; i < M * N; i++) {
        if(C1[i] != C2[i]) {
            printf("结果不一致！\n");
            return 0;
        }
    }
    printf("✓ 两个版本结果一致\n");
    return 1;
}

// ========== 主函数 ==========
int main() {
    printf("═════════════════════════════════════════════════\n");
    printf("题目 d) 矩阵转置对性能的影响\n");
    printf("矩阵规模: %d × %d × %d\n", M, K, N);
    printf("═════════════════════════════════════════════════\n\n");
    
    // 分配内存
    double *A = (double*)malloc(M * K * sizeof(double));
    double *B = (double*)malloc(K * N * sizeof(double));
    double *C1 = (double*)malloc(M * N * sizeof(double));
    double *C2 = (double*)malloc(M * N * sizeof(double));
    
    // 初始化数据
    init_matrices(A, B);
    
    // ========== 版本 1：不转置 ==========
    printf("【版本 1】不转置，原始 kij 顺序\n");
    printf("B 的访问：列优先（步长为 %d）\n", N);
    double time1 = measure_time_ms(matmul_kij_original, A, B, C1);
    printf("耗时: %.2f ms\n\n", time1);
    
    // ========== 版本 2：转置 B ==========
    printf("【版本 2】转置 B 后，改进的 kij 顺序\n");
    printf("B_T 的访问：行优先（步长为 %d）\n", K);
    double time2 = measure_time_ms(matmul_kij_with_transpose, A, B, C2);
    printf("耗时: %.2f ms\n\n", time2);
    
    // ========== 验证与对比 ==========
    verify_result(C1, C2);
    
    printf("═════════════════════════════════════════════════\n");
    printf("【性能对比结果】\n");
    printf("原始版本耗时:    %.2f ms\n", time1);
    printf("转置后耗时:      %.2f ms\n", time2);
    printf("加速比:          %.2f x\n", time1 / time2);
    printf("性能提升:        %.1f %%\n", (1.0 - time2/time1) * 100.0);
    printf("═════════════════════════════════════════════════\n");
    
    // 释放内存
    free(A);
    free(B);
    free(C1);
    free(C2);
    
    return 0;
}
