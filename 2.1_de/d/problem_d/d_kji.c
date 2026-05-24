/*
题目d) - kji循环顺序 转置前后对比
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define M 1001
#define K 1001
#define N 1001

int main() {
    double *A = malloc(M * K * sizeof(double));
    double *B = malloc(K * N * sizeof(double));
    double *B_T = malloc(K * N * sizeof(double));
    double *C1 = malloc(M * N * sizeof(double));
    double *C2 = malloc(M * N * sizeof(double));
    
    for(int i = 0; i < M * K; i++) A[i] = 1.0 + (i % 10) * 0.1;
    for(int i = 0; i < K * N; i++) B[i] = 2.0 + (i % 10) * 0.1;
    
    struct timespec t1, t2;
    
    memset(C1, 0, M * N * sizeof(double));
    clock_gettime(CLOCK_MONOTONIC, &t1);
    for(int k = 0; k < K; k++) {
        for(int j = 0; j < N; j++) {
            for(int i = 0; i < M; i++) {
                C1[i*N + j] += A[i*K + k] * B[k*N + j];
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &t2);
    double time_no_trans = (t2.tv_sec - t1.tv_sec) * 1000.0 + 
                           (t2.tv_nsec - t1.tv_nsec) / 1000000.0;
    
    clock_gettime(CLOCK_MONOTONIC, &t1);
    for(int k = 0; k < K; k++) {
        for(int j = 0; j < N; j++) {
            B_T[j*K + k] = B[k*N + j];
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &t2);
    double time_transpose = (t2.tv_sec - t1.tv_sec) * 1000.0 + 
                            (t2.tv_nsec - t1.tv_nsec) / 1000000.0;
    
    memset(C2, 0, M * N * sizeof(double));
    clock_gettime(CLOCK_MONOTONIC, &t1);
    for(int k = 0; k < K; k++) {
        for(int j = 0; j < N; j++) {
            for(int i = 0; i < M; i++) {
                C2[i*N + j] += A[i*K + k] * B_T[j*K + k];
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &t2);
    double time_matmul_trans = (t2.tv_sec - t1.tv_sec) * 1000.0 + 
                               (t2.tv_nsec - t1.tv_nsec) / 1000000.0;
    
    double time_with_trans = time_transpose + time_matmul_trans;
    double speedup = time_no_trans / time_with_trans;
    double improvement = (1.0 - time_with_trans / time_no_trans) * 100.0;
    
    double checksum1 = 0, checksum2 = 0;
    for(int i = 0; i < M * N; i++) {
        checksum1 += C1[i];
        checksum2 += C2[i];
    }
    
    printf("【kji循环顺序】\n");
    printf("不转置耗时:     %.2f ms\n", time_no_trans);
    printf("转置耗时:       %.2f ms\n", time_with_trans);
    printf("  ├─ 转置开销:   %.2f ms\n", time_transpose);
    printf("  └─ 乘法耗时:   %.2f ms\n", time_matmul_trans);
    printf("加速比:        %.3f x\n", speedup);
    printf("性能提升:      %.2f %%\n", improvement);
    printf("校验和1: %.0f, 校验和2: %.0f\n\n", checksum1, checksum2);
    
    free(A); free(B); free(B_T); free(C1); free(C2);
    return 0;
}
