/*
╔════════════════════════════════════════════════════════════════╗
║                     题目 d) 实验代码（改进版）                   ║
║              转置 B 矩阵的性能影响分析 - 更公平的对比              ║
╚════════════════════════════════════════════════════════════════╝

【编译命令】
    gcc -O3 -march=native -o test_d_v2 test_d_v2.c -lm

【运行命令】
    ./test_d_v2

【关键改进】
    1. 不使用 memset（编译器优化杀手）
    2. 防止内存分配被优化掉
    3. 分别测量转置时间和乘法时间
    4. 更公平的对比
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
    // 直接计算，不初始化（因为结构决定了 C 的值）
    for(int k = 0; k < K; k++) {
        for(int i = 0; i < M; i++) {
            double Aik = A[i*K + k];  // 缓存 A[i][k]
            for(int j = 0; j < N; j++) {
                C[i*N + j] += Aik * B[k*N + j];
            }
        }
    }
}

// ========== 版本 2a：只测量转置时间 ==========
void transpose_B(double *B, double *B_T) {
    for(int k = 0; k < K; k++) {
        for(int j = 0; j < N; j++) {
            B_T[j*K + k] = B[k*N + j];
        }
    }
}

// ========== 版本 2b：用转置后的 B_T 进行乘法 ==========
void matmul_kij_with_Bt(double *A, double *B_T, double *C) {
    for(int k = 0; k < K; k++) {
        for(int i = 0; i < M; i++) {
            double Aik = A[i*K + k];
            for(int j = 0; j < N; j++) {
                C[i*N + j] += Aik * B_T[j*K + k];
            }
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
void init_matrices(double *A, double *B, double *C) {
    for(int i = 0; i < M * K; i++)
        A[i] = (double)(i % 10);
    for(int i = 0; i < K * N; i++)
        B[i] = (double)((i + 7) % 10);
    for(int i = 0; i < M * N; i++)
        C[i] = 0.0;
}

// ========== 验证两个版本的结果是否一致 ==========
int verify_result(double *C1, double *C2) {
    for(int i = 0; i < M * N; i++) {
        // 浮点数比较要用 epsilon
        if(C1[i] != C2[i]) {
            printf("结果不一致！ C1[%d]=%.6f, C2[%d]=%.6f\n", 
                   i, C1[i], i, C2[i]);
            return 0;
        }
    }
    printf("✓ 两个版本结果一致\n");
    return 1;
}

// ========== 主函数 ==========
int main() {
    printf("═════════════════════════════════════════════════\n");
    printf("题目 d) 矩阵转置对性能的影响（改进版对比）\n");
    printf("矩阵规模: %d × %d × %d\n", M, K, N);
    printf("═════════════════════════════════════════════════\n\n");
    
    // 分配内存
    double *A = (double*)malloc(M * K * sizeof(double));
    double *B = (double*)malloc(K * N * sizeof(double));
    double *B_T = (double*)malloc(K * N * sizeof(double));
    double *C1 = (double*)malloc(M * N * sizeof(double));
    double *C2 = (double*)malloc(M * N * sizeof(double));
    
    // 初始化数据
    init_matrices(A, B, C1);
    init_matrices(A, B, C2);
    
    printf("【方案 1】不转置，原始 kij 顺序\n");
    printf("B 的访问：列优先（步长为 %d）\n", N);
    double time1 = measure_time_ms(matmul_kij_original, A, B, C1);
    printf("耗时: %.2f ms\n\n", time1);
    
    printf("【方案 2】转置 B 后进行乘法\n");
    printf("分步分析：\n");
    
    // 第一步：测量转置时间
    printf("  → 第一步：转置 B\n");
    double transpose_time = measure_transpose_time(transpose_B, B, B_T);
    printf("     耗时: %.2f ms\n", transpose_time);
    
    // 第二步：测量用 B_T 进行乘法的时间
    printf("  → 第二步：用 B_T 进行 kij 乘法\n");
    double matmul_time = measure_time_ms(matmul_kij_with_Bt, A, B_T, C2);
    printf("     耗时: %.2f ms\n", matmul_time);
    
    double time2 = transpose_time + matmul_time;
    printf("  → 总耗时: %.2f ms\n\n", time2);
    
    // 验证
    verify_result(C1, C2);
    
    printf("═════════════════════════════════════════════════\n");
    printf("【性能对比结果】\n");
    printf("方案 1（不转置）耗时:  %.2f ms\n", time1);
    printf("方案 2（转置）耗时:    %.2f ms\n", time2);
    printf("  ├─ 转置开销: %.2f ms (%.1f%%)\n", 
           transpose_time, transpose_time/time2*100);
    printf("  └─ 乘法耗时: %.2f ms (%.1f%%)\n", 
           matmul_time, matmul_time/time2*100);
    printf("\n加速比:          %.2f x\n", time1 / time2);
    printf("性能提升:        %.1f %%\n", (1.0 - time2/time1) * 100.0);
    printf("═════════════════════════════════════════════════\n\n");
    
    // 分析
    printf("【分析】\n");
    if (time1 < time2) {
        printf("❌ 转置方案反而更慢。\n");
        printf("原因可能包括:\n");
        printf("  1. Apple Silicon 的缓存结构对列优先访问优化更好\n");
        printf("  2. 编译器（-O3）已自动优化列优先访问（SIMD 向量化）\n");
        printf("  3. 转置引入的额外内存分配破坏了优化\n");
        printf("  4. B_T 是新分配内存，未必在 L1/L2 缓存中\n");
    } else {
        printf("✓ 转置方案更快。\n");
        printf("转置开销 (%.1f%%) 被乘法加速抵消。\n", transpose_time/time2*100);
    }
    printf("═════════════════════════════════════════════════\n");
    
    // 释放内存
    free(A);
    free(B);
    free(B_T);
    free(C1);
    free(C2);
    
    return 0;
}
