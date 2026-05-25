#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BLOCK_SIZE 64

void ijk(double *A, double *B, double *C, int n) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            for (int k = 0; k < n; k++)
                C[i*n+j] += A[i*n+k] * B[k*n+j];
}

void ijkijk(double *A, double *B, double *C, int n) {
    for (int i = 0; i < n; i += BLOCK_SIZE)
        for (int j = 0; j < n; j += BLOCK_SIZE)
            for (int k = 0; k < n; k += BLOCK_SIZE)
                for (int ii = i; ii < i+BLOCK_SIZE && ii < n; ii++)
                    for (int jj = j; jj < j+BLOCK_SIZE && jj < n; jj++)
                        for (int kk = k; kk < k+BLOCK_SIZE && kk < n; kk++)
                            C[ii*n+jj] += A[ii*n+kk] * B[kk*n+jj];
}

double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

int main() {
    int sizes[100];
    int num_sizes = 0;
    
    // 1001 到 1050 步长为 1
    for (int n = 1001; n <= 1050; n++) {
        sizes[num_sizes++] = n;
    }
    
    // 1060 到 1090 步长为 10
    for (int n = 1060; n <= 1090; n += 10) {
        sizes[num_sizes++] = n;
    }
    
    // 1100 到 1500 步长为 100
    for (int n = 1100; n <= 1500; n += 100) {
        sizes[num_sizes++] = n;
    }

    printf("%-12s %-16s %-16s %-16s %-16s %-10s\n",
           "矩阵规模", "ijk耗时(ms)", "ijk GFLOPS", "ijkijk耗时(ms)", "ijkijk GFLOPS", "加速比");

    for (int s = 0; s < num_sizes; s++) {
        int n = sizes[s];
        double *A = (double*)malloc(n*n*sizeof(double));
        double *B = (double*)malloc(n*n*sizeof(double));
        double *C = (double*)calloc(n*n, sizeof(double));

        for (int i = 0; i < n*n; i++) {
            A[i] = 1.0 + (i % 1000) * 0.001;
            B[i] = 1.0 + (i % 1000) * 0.001;
        }

        double ops = 2.0 * n * n * n;

        double t1 = get_time_ms();
        ijk(A, B, C, n);
        double t2 = get_time_ms();
        double ijk_time = t2 - t1;
        double ijk_gflops = ops / (ijk_time * 1e6);

        for (int i = 0; i < n*n; i++) C[i] = 0;

        double t3 = get_time_ms();
        ijkijk(A, B, C, n);
        double t4 = get_time_ms();
        double blk_time = t4 - t3;
        double blk_gflops = ops / (blk_time * 1e6);

        double speedup = ijk_time / blk_time;

        printf("%-12d %-16.2f %-16.4f %-16.2f %-16.4f %-10.2fx\n",
               n, ijk_time, ijk_gflops, blk_time, blk_gflops, speedup);

        free(A); free(B); free(C);
    }
    return 0;
}
