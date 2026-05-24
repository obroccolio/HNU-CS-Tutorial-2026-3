/*
╔════════════════════════════════════════════════════════════════╗
║                  题目 d) 实验代码（最终版 - 正确对比）            ║
║              使用 ijk 顺序，展示转置 B 的真正威力                ║
╚════════════════════════════════════════════════════════════════╝

【编译命令】
    gcc -O3 -march=native -o test_d_final test_d_final.c -lm

【运行命令】
    ./test_d_final

【关键改变】
    ✓ 改用 ijk 循环顺序（最内层是 k）
    ✓ 这样不转置时，B[k][j] 是列优先访问（最慢）
    ✓ 转置后，B_T[j][k] 变成行优先访问（最快）
    ✓ 现在应该能看到转置的真正威力！

【对比】
    - 原始 kij 顺序：已经是行优先，手动转置反而变列优先 ❌
    - 改进 ijk 顺序：列优先（慢），转置后行优先（快） ✓
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define M 1001
#define K 1001
#define N 1001

// ========== 版本 1：ijk 顺序，不转置 B（列优先访问，慢）==========
void matmul_ijk_original(double *A, double *B, double *C) {
    // C 初始化
    for(int i = 0; i < M * N; i++)
        C[i] = 0.0;
    
    // 最内层是 k（列优先访问 B）
    for(int i = 0; i < M; i++) {
        for(int j = 0; j < N; j++) {
            double sum = 0.0;
            for(int k = 0; k < K; k++) {
                sum += A[i*K + k] * B[k*N + j];  // ← B[k][j]：k 增加时步长为 N，列优先！
            }
            C[i*N + j] = sum;
        }
    }
}

// ========== 版本 2a：转置 B ==========
void transpose_B(double *B, double *B_T) {
    for(int k = 0; k < K; k++) {
        for(int j = 0; j < N; j++) {
            B_T[j*K + k] = B[k*N + j];
        }
    }
}

// ========== 版本 2b：ijk 顺序，用转置后的 B_T（行优先访问，快）==========
void matmul_ijk_with_Bt(double *A, double *B_T, double *C) {
    // C 初始化
    for(int i = 0; i < M * N; i++)
        C[i] = 0.0;
    
    // 最内层还是 k，但现在访问 B_T
    for(int i = 0; i < M; i++) {
        for(int j = 0; j < N; j++) {
            double sum = 0.0;
            for(int k = 0; k < K; k++) {
                sum += A[i*K + k] * B_T[j*K + k];  // ← B_T[j][k]：k 增加时步长为 1，行优先！
            }
            C[i*N + j] = sum;
        }
    }
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

// ========== 性能测量函数（转置专用）==========
double measure_transpose_time(void (*func)(double*, double*), 
                              double *B, double *B_T) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    func(B, B_T);
    
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
        A[i] = (double)(i % 10);
    for(int i = 0; i < K * N; i++)
        B[i] = (double)((i + 7) % 10);
}

// ========== 结果验证 ==========
int verify_result(double *C1, double *C2) {
    for(int i = 0; i < M * N; i++) {
        if(C1[i] != C2[i]) {
            printf("❌ 结果不一致！ C1[%d]=%.6f, C2[%d]=%.6f\n", 
                   i, C1[i], i, C2[i]);
            return 0;
        }
    }
    printf("✓ 两个版本结果一致\n");
    return 1;
}

// ========== 主函数 ==========
int main() {
    printf("═════════════════════════════════════════════════════════════\n");
    printf("题目 d) 矩阵转置对性能的影响（正确版 - ijk 循环顺序）\n");
    printf("矩阵规模: %d × %d × %d\n", M, K, N);
    printf("═════════════════════════════════════════════════════════════\n\n");
    
    // 分配内存
    double *A = (double*)malloc(M * K * sizeof(double));
    double *B = (double*)malloc(K * N * sizeof(double));
    double *B_T = (double*)malloc(K * N * sizeof(double));
    double *C1 = (double*)malloc(M * N * sizeof(double));
    double *C2 = (double*)malloc(M * N * sizeof(double));
    
    // 初始化数据
    init_matrices(A, B);
    
    printf("【方案 1】ijk 顺序，不转置 B\n");
    printf("┌─────────────────────────────────┐\n");
    printf("│ B 的访问模式：列优先             │\n");
    printf("│ 外层 i，中层 j，内层 k          │\n");
    printf("│ 当 k 增加时：B[k*N+j]           │\n");
    printf("│ 步长 = N（列优先，最慢）✗      │\n");
    printf("└─────────────────────────────────┘\n");
    double time1 = measure_time_ms(matmul_ijk_original, A, B, C1);
    printf("耗时: %.2f ms\n\n", time1);
    
    printf("【方案 2】ijk 顺序，转置 B 后\n");
    printf("分步分析：\n");
    
    // 第一步：转置
    printf("  → 第一步：转置 B → B_T\n");
    double transpose_time = measure_transpose_time(transpose_B, B, B_T);
    printf("     耗时: %.2f ms\n", transpose_time);
    
    // 第二步：用转置后的矩阵进行乘法
    printf("  → 第二步：用转置后的 B_T 进行 ijk 乘法\n");
    printf("     ┌─────────────────────────────────┐\n");
    printf("     │ B_T 的访问模式：行优先           │\n");
    printf("     │ 外层 i，中层 j，内层 k          │\n");
    printf("     │ 当 k 增加时：B_T[j*K+k]        │\n");
    printf("     │ 步长 = 1（行优先，最快）✓      │\n");
    printf("     └─────────────────────────────────┘\n");
    double matmul_time = measure_time_ms(matmul_ijk_with_Bt, A, B_T, C2);
    printf("     耗时: %.2f ms\n", matmul_time);
    
    double time2 = transpose_time + matmul_time;
    printf("  → 总耗时: %.2f ms\n\n", time2);
    
    // 验证结果
    verify_result(C1, C2);
    
    printf("═════════════════════════════════════════════════════════════\n");
    printf("【性能对比结果】\n");
    printf("方案 1（不转置）耗时:  %.2f ms\n", time1);
    printf("方案 2（转置）耗时:    %.2f ms\n", time2);
    printf("  ├─ 转置开销: %.2f ms (%.1f%%)\n", 
           transpose_time, transpose_time/time2*100);
    printf("  └─ 乘法耗时: %.2f ms (%.1f%%)\n", 
           matmul_time, matmul_time/time2*100);
    printf("\n加速比:          %.2f x\n", time1 / time2);
    printf("性能提升:        %.1f %%\n", (1.0 - time2/time1) * 100.0);
    printf("═════════════════════════════════════════════════════════════\n\n");
    
    // 详细分析
    printf("【分析】\n");
    if (time1 > time2) {
        printf("✓✓✓ 转置方案获得显著加速！\n");
        printf("这就是'转置大法'的真正威力！\n");
        printf("\n原因：\n");
        printf("  1. 原始方案（ijk 不转置）：\n");
        printf("     - B[k][j] 的 k 在最内层变化\n");
        printf("     - 步长为 N（列优先），L1 Cache Miss 率高\n");
        printf("  2. 优化后方案（ijk + 转置）：\n");
        printf("     - B_T[j][k] 的 k 在最内层变化\n");
        printf("     - 步长为 1（行优先），L1 Cache Miss 率极低\n");
        printf("  3. 转置开销仅 %.1f%%，而计算加速 %.1f%%\n", 
               transpose_time/time2*100, (1.0 - matmul_time/time1)*100);
    } else {
        printf("❌ 转置方案仍然没有加速。\n");
        printf("可能是 Apple Silicon 的硬件预取太强大了。\n");
    }
    printf("═════════════════════════════════════════════════════════════\n");
    
    // 释放内存
    free(A);
    free(B);
    free(B_T);
    free(C1);
    free(C2);
    
    return 0;
}
