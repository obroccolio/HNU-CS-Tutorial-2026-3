#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define N 1001

static double A[N][N];
static double B[N][N];
static double C[N][N];

static void init_matrices(void)
{
    srand(42);
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            A[i][j] = (double)rand() / RAND_MAX;
            B[i][j] = (double)rand() / RAND_MAX;
        }
}

static void zero_C(void)
{
    memset(C, 0, sizeof(C));
}

static double get_time(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* C[i][j] += A[i][k] * B[k][j] */

static void gemm_ijk(void)
{
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            for (int k = 0; k < N; k++)
                C[i][j] += A[i][k] * B[k][j];
}

static void gemm_ikj(void)
{
    for (int i = 0; i < N; i++)
        for (int k = 0; k < N; k++)
            for (int j = 0; j < N; j++)
                C[i][j] += A[i][k] * B[k][j];
}

static void gemm_jik(void)
{
    for (int j = 0; j < N; j++)
        for (int i = 0; i < N; i++)
            for (int k = 0; k < N; k++)
                C[i][j] += A[i][k] * B[k][j];
}

static void gemm_jki(void)
{
    for (int j = 0; j < N; j++)
        for (int k = 0; k < N; k++)
            for (int i = 0; i < N; i++)
                C[i][j] += A[i][k] * B[k][j];
}

static void gemm_kij(void)
{
    for (int k = 0; k < N; k++)
        for (int i = 0; i < N; i++)
            for (int j = 0; j < N; j++)
                C[i][j] += A[i][k] * B[k][j];
}

static void gemm_kji(void)
{
    for (int k = 0; k < N; k++)
        for (int j = 0; j < N; j++)
            for (int i = 0; i < N; i++)
                C[i][j] += A[i][k] * B[k][j];
}

typedef void (*gemm_func)(void);

int main(void)
{
    const char *names[] = {"ijk", "ikj", "jik", "jki", "kij", "kji"};
    gemm_func funcs[] = {gemm_ijk, gemm_ikj, gemm_jik, gemm_jki, gemm_kij, gemm_kji};
    int num = 6;

    double flops = 2.0 * N * N * N;

    init_matrices();

    printf("Matrix size: %d x %d\n", N, N);
    printf("Total FLOPs per multiply: %.2e\n\n", flops);
    printf("%-8s %12s %12s\n", "Order", "Time(s)", "GFLOPS");
    printf("-------- ------------ ------------\n");

    FILE *csv = fopen("results.csv", "w");
    fprintf(csv, "order,time_s,gflops\n");

    for (int t = 0; t < num; t++) {
        zero_C();

        double t0 = get_time();
        funcs[t]();
        double t1 = get_time();

        double elapsed = t1 - t0;
        double gflops = flops / elapsed / 1e9;

        printf("%-8s %12.3f %12.4f\n", names[t], elapsed, gflops);
        fprintf(csv, "%s,%.6f,%.6f\n", names[t], elapsed, gflops);
    }

    fclose(csv);
    printf("\nResults saved to results.csv\n");
    return 0;
}
