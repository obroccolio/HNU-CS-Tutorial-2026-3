/*
 * gemm_one.c —— 按命令行参数选定循环顺序运行一次 GEMM
 *
 * 用途：配合 perf stat / cachegrind 采集单一顺序的硬件计数器
 *      （避免一次跑 6 种全部混在一起）
 *
 * 编译: gcc -O2 -o gemm_one gemm_one.c
 *      （或 -O0 / -O3 -march=native，按需对照）
 *
 * 用法: ./gemm_one ikj
 *      perf stat -r 3 -e cycles,instructions,L1-dcache-loads,\
 *          L1-dcache-load-misses,LLC-loads,LLC-load-misses ./gemm_one ikj
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define N 1001

static double A[N][N];
static double B[N][N];
static double C[N][N];

/* 用宏统一展开 6 种顺序，避免重复代码。
 * o1/o2/o3 是循环嵌套顺序里的"外/中/内"变量；
 * e1/e2/e3 固定为 i/j/k —— 矩阵下标永远是 C[i][j] += A[i][k]*B[k][j]。
 */
#define LOOP(o1, o2, o3) \
    for (int o1 = 0; o1 < N; o1++) \
        for (int o2 = 0; o2 < N; o2++) \
            for (int o3 = 0; o3 < N; o3++) \
                C[i][j] += A[i][k] * B[k][j];

static void init(void) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = i * 0.001 + j * 0.0001;
            B[i][j] = j * 0.001 - i * 0.0001;
            C[i][j] = 0.0;
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr,
            "usage: %s {ijk|ikj|jik|jki|kij|kji}\n", argv[0]);
        return 1;
    }
    init();

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    const char *o = argv[1];
    if      (!strcmp(o, "ijk")) { LOOP(i, j, k) }
    else if (!strcmp(o, "ikj")) { LOOP(i, k, j) }
    else if (!strcmp(o, "jik")) { LOOP(j, i, k) }
    else if (!strcmp(o, "jki")) { LOOP(j, k, i) }
    else if (!strcmp(o, "kij")) { LOOP(k, i, j) }
    else if (!strcmp(o, "kji")) { LOOP(k, j, i) }
    else { fprintf(stderr, "unknown order: %s\n", o); return 1; }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double sec = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
    double gflops = 2.0 * N * N * N / sec / 1e9;

    /* 防止整段循环被优化掉 */
    double s = 0.0;
    for (int i = 0; i < N; i++) s += C[i][i];

    fprintf(stderr, "[order=%s]  time=%.4fs  gflops=%.3f  trace=%.6f\n",
            o, sec, gflops, s);
    return 0;
}
