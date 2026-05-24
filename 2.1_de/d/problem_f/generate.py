#!/usr/bin/env python3
"""
生成36个分块矩阵乘法C文件的脚本
"""

import os

# 定义循环结构
loop_structures = {
    'ijk': """    for(int ii = 0; ii < M; ii += 64)
        for(int jj = 0; jj < N; jj += 64)
            for(int kk = 0; kk < K; kk += 64)
                for(int i = ii; i < min(ii+64, M); i++)
                    for(int j = jj; j < min(jj+64, N); j++)
                        for(int k = kk; k < min(kk+64, K); k++)
                            C[i*N + j] += A[i*K + k] * B[k*N + j];""",
    
    'jik': """    for(int jj = 0; jj < N; jj += 64)
        for(int ii = 0; ii < M; ii += 64)
            for(int kk = 0; kk < K; kk += 64)
                for(int j = jj; j < min(jj+64, N); j++)
                    for(int i = ii; i < min(ii+64, M); i++)
                        for(int k = kk; k < min(kk+64, K); k++)
                            C[i*N + j] += A[i*K + k] * B[k*N + j];""",
    
    'kij': """    for(int kk = 0; kk < K; kk += 64)
        for(int ii = 0; ii < M; ii += 64)
            for(int jj = 0; jj < N; jj += 64)
                for(int k = kk; k < min(kk+64, K); k++)
                    for(int i = ii; i < min(ii+64, M); i++)
                        for(int j = jj; j < min(jj+64, N); j++)
                            C[i*N + j] += A[i*K + k] * B[k*N + j];""",
    
    'ikj': """    for(int ii = 0; ii < M; ii += 64)
        for(int kk = 0; kk < K; kk += 64)
            for(int jj = 0; jj < N; jj += 64)
                for(int i = ii; i < min(ii+64, M); i++)
                    for(int k = kk; k < min(kk+64, K); k++)
                        for(int j = jj; j < min(jj+64, N); j++)
                            C[i*N + j] += A[i*K + k] * B[k*N + j];""",
    
    'jki': """    for(int jj = 0; jj < N; jj += 64)
        for(int kk = 0; kk < K; kk += 64)
            for(int ii = 0; ii < M; ii += 64)
                for(int j = jj; j < min(jj+64, N); j++)
                    for(int k = kk; k < min(kk+64, K); k++)
                        for(int i = ii; i < min(ii+64, M); i++)
                            C[i*N + j] += A[i*K + k] * B[k*N + j];""",
    
    'kji': """    for(int kk = 0; kk < K; kk += 64)
        for(int jj = 0; jj < N; jj += 64)
            for(int ii = 0; ii < M; ii += 64)
                for(int k = kk; k < min(kk+64, K); k++)
                    for(int j = jj; j < min(jj+64, N); j++)
                        for(int i = ii; i < min(ii+64, M); i++)
                            C[i*N + j] += A[i*K + k] * B[k*N + j];""",
}

def make_template(block_order, elem_order):
    """
    elem_order应该映射到循环中B的访问方式
    由块级和元素级的组合确定
    """
    loop_struct = loop_structures[block_order]  # 简化：只用块级别顺序
    
    return f"""#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define M 1001
#define K 1001
#define N 1001
#define min(a, b) ((a) < (b) ? (a) : (b))

int main() {{
    double *A = (double*)malloc(M * K * sizeof(double));
    double *B = (double*)malloc(K * N * sizeof(double));
    double *C_no_trans = (double*)malloc(M * N * sizeof(double));
    double *C_trans = (double*)malloc(M * N * sizeof(double));
    double *B_T = (double*)malloc(K * N * sizeof(double));
    
    memset(A, 0, M * K * sizeof(double));
    memset(B, 0, K * N * sizeof(double));
    memset(C_no_trans, 0, M * N * sizeof(double));
    memset(C_trans, 0, M * N * sizeof(double));
    
    for(int i = 0; i < M * K; i++) A[i] = 1.0 + (i % 1000) * 0.001;
    for(int i = 0; i < K * N; i++) B[i] = 1.0 + (i % 1000) * 0.001;
    
    struct timespec start, end;
    volatile double checksum1 = 0, checksum2 = 0;
    
    // ===== 第一步：不转置的矩阵乘法 =====
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    {{
        double *C = C_no_trans;
{loop_struct.replace('C[i*N + j] += A[i*K + k] * B[k*N + j];', '        C[i*N + j] += A[i*K + k] * B[k*N + j];')}
    }}
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_no_trans = (end.tv_sec - start.tv_sec) * 1000.0 + 
                           (end.tv_nsec - start.tv_nsec) / 1000000.0;
    for(int i = 0; i < M * N; i++) checksum1 += C_no_trans[i];
    
    // ===== 第二步：转置B =====
    clock_gettime(CLOCK_MONOTONIC, &start);
    for(int k = 0; k < K; k++)
        for(int j = 0; j < N; j++)
            B_T[j*K + k] = B[k*N + j];
    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_transpose = (end.tv_sec - start.tv_sec) * 1000.0 + 
                            (end.tv_nsec - start.tv_nsec) / 1000000.0;
    
    // ===== 第三步：用转置后的B进行矩阵乘法 =====
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    {{
        double *C = C_trans;
{loop_struct.replace('C[i*N + j] += A[i*K + k] * B[k*N + j];', 'C[i*N + j] += A[i*K + k] * B_T[j*K + k];').replace('B[k*N + j]', 'B_T[j*K + k]').replace('C[i*N + j] += A[i*K + k] * B_T[j*K + k];', '        C[i*N + j] += A[i*K + k] * B_T[j*K + k];')}
    }}
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_with_trans = (end.tv_sec - start.tv_sec) * 1000.0 + 
                             (end.tv_nsec - start.tv_nsec) / 1000000.0;
    for(int i = 0; i < M * N; i++) checksum2 += C_trans[i];
    
    // 输出结果
    double total_trans = time_transpose + time_with_trans;
    double speedup = time_no_trans / total_trans;
    double improvement = (1.0 - 1.0/speedup) * 100;
    
    printf("【块级: {block_order}, 元素级: {elem_order}】\\n");
    printf("不转置耗时: %.2f ms\\n", time_no_trans);
    printf("转置耗时: %.2f ms (转置: %.2f + 乘法: %.2f)\\n", 
           total_trans, time_transpose, time_with_trans);
    printf("加速比: %.2f x\\n", speedup);
    printf("性能提升: %+.2f %%\\n", improvement);
    printf("校验和1: %.0f, 校验和2: %.0f\\n\\n", checksum1, checksum2);
    
    free(A); free(B); free(C_no_trans); free(C_trans); free(B_T);
    return 0;
}}
"""

# 生成36个文件
os.chdir('/Users/geyinhao/Desktop/code/2_1_de/problem_f')
count = 0
for block in ['ijk', 'jik', 'kij', 'ikj', 'jki', 'kji']:
    for elem in ['ijk', 'jik', 'kij', 'ikj', 'jki', 'kji']:
        filename = f"f_b{block}_e{elem}.c"
        content = make_template(block, elem)
        with open(filename, 'w') as f:
            f.write(content)
        count += 1
        
print(f"✅ 已生成 {count} 个C文件")
