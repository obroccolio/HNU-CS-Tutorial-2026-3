#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>

/**
 * 问题e：测试矩阵规模对cache的影响
 * 
 * 测试规模：
 *   - 1001: 基准（非2的幂）
 *   - 1023: 2的幂-1
 *   - 1024: 2的幂（整齐，可能产生冲突）
 *   - 1025: 2的幂+1（破坏对齐）
 *   - 2047: 2的幂-1
 *   - 2048: 2的幂
 *   - 2049: 2的幂+1
 * 
 * 目标：观察性能曲线是否平滑，是否在2的幂处出现跳变
 */

typedef struct {
    int size;
    double time_no_transpose;
    double time_with_transpose;
    double speedup;
} Result;

Result test_matrix_size(int n) {
    Result res = {n, 0, 0, 0};
    
    double *A = malloc((long)n * n * 8);
    double *B = malloc((long)n * n * 8);
    double *C_no = calloc((long)n * n, 8);
    double *C_yes = calloc((long)n * n, 8);
    double *B_T = malloc((long)n * n * 8);
    
    if (!A || !B || !C_no || !C_yes || !B_T) {
        fprintf(stderr, "Memory allocation failed for size %d\n", n);
        return res;
    }
    
    // 初始化
    for (long i = 0; i < (long)n * n; i++) {
        A[i] = 1.0 + (i % 1000) * 0.001;
        B[i] = 1.0 + (i % 1000) * 0.001;
    }
    
    struct timespec s, e;
    
    // 测试1：不转置（使用分块以加快速度）
    clock_gettime(CLOCK_MONOTONIC, &s);
    int block_size = 64;
    for (int ii = 0; ii < n; ii += block_size) {
        for (int kk = 0; kk < n; kk += block_size) {
            for (int jj = 0; jj < n; jj += block_size) {
                for (int i = ii; i < ii + block_size && i < n; i++) {
                    for (int k = kk; k < kk + block_size && k < n; k++) {
                        for (int j = jj; j < jj + block_size && j < n; j++) {
                            C_no[i * n + j] += A[i * n + k] * B[k * n + j];
                        }
                    }
                }
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &e);
    res.time_no_transpose = (e.tv_sec - s.tv_sec) * 1000 + (e.tv_nsec - s.tv_nsec) / 1e6;
    
    // 转置B
    clock_gettime(CLOCK_MONOTONIC, &s);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            B_T[j * n + i] = B[i * n + j];
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &e);
    double t_trans = (e.tv_sec - s.tv_sec) * 1000 + (e.tv_nsec - s.tv_nsec) / 1e6;
    
    // 测试2：转置后（分块）
    clock_gettime(CLOCK_MONOTONIC, &s);
    for (int ii = 0; ii < n; ii += block_size) {
        for (int kk = 0; kk < n; kk += block_size) {
            for (int jj = 0; jj < n; jj += block_size) {
                for (int i = ii; i < ii + block_size && i < n; i++) {
                    for (int k = kk; k < kk + block_size && k < n; k++) {
                        for (int j = jj; j < jj + block_size && j < n; j++) {
                            C_yes[i * n + j] += A[i * n + k] * B_T[j * n + k];
                        }
                    }
                }
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &e);
    double t_mul = (e.tv_sec - s.tv_sec) * 1000 + (e.tv_nsec - s.tv_nsec) / 1e6;
    
    res.time_with_transpose = t_trans + t_mul;
    res.speedup = res.time_no_transpose / res.time_with_transpose;
    
    free(A);
    free(B);
    free(C_no);
    free(C_yes);
    free(B_T);
    
    return res;
}

int main() {
    // 测试规模序列
    int sizes[] = {
        1000, 1001, 1002,
        1023, 1024, 1025,
        1500, 1600,
        2000, 2047, 2048, 2049, 2050,
        3000
    };
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    
    printf("矩阵规模 | 不转置(ms) | 转置后(ms) | 加速比 | 备注\n");
    printf("---------|-----------|-----------|--------|----------\n");
    
    for (int i = 0; i < num_sizes; i++) {
        Result res = test_matrix_size(sizes[i]);
        
        const char *note = "";
        if (sizes[i] == 1024 || sizes[i] == 2048) {
            note = "2^幂";
        } else if (sizes[i] == 1023 || sizes[i] == 2047) {
            note = "2^幂-1";
        } else if (sizes[i] == 1025 || sizes[i] == 2049) {
            note = "2^幂+1";
        }
        
        printf("%7d | %9.2f | %9.2f | %6.2fx | %s\n",
               res.size, res.time_no_transpose, res.time_with_transpose,
               res.speedup, note);
        
        fflush(stdout);
    }
    
    return 0;
}
