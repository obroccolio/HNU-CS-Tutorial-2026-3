/*
 * Pointer-chasing latency benchmark (v2).
 * Changes from v1:
 *   - CPU-core pinning (sched_setaffinity) to reduce vCPU migration jitter
 *   - RUNS=5 repeated measurements; reports median + [min, max] spread
 *   - Removed hardcoded "cycles@3.9GHz" column (P-core boost varies 2.2–5.4 GHz)
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sched.h>

#define ITERATIONS 50000000UL
#define RUNS       5           /* odd number → clean median */

static uint64_t ns_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

/* Fisher-Yates shuffle: every access depends on previous result,
   defeating the hardware prefetcher. Fixed seed → reproducible layout. */
static void build_chase(size_t *arr, size_t n) {
    srand(42);
    for (size_t i = 0; i < n; i++) arr[i] = i;
    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)rand() % (i + 1);
        size_t tmp = arr[i]; arr[i] = arr[j]; arr[j] = tmp;
    }
}

static void isort(double *a, int n) {
    for (int i = 1; i < n; i++) {
        double k = a[i]; int j = i - 1;
        while (j >= 0 && a[j] > k) { a[j+1] = a[j]; j--; }
        a[j+1] = k;
    }
}

int main(void) {
    /* Pin to vCPU 0 to reduce OS-scheduler migration jitter inside the VM */
    cpu_set_t cs;
    CPU_ZERO(&cs); CPU_SET(0, &cs);
    sched_setaffinity(0, sizeof(cs), &cs);

    size_t sizes[] = {
        8UL<<10, 32UL<<10, 128UL<<10, 512UL<<10,
        2UL<<20, 8UL<<20,  32UL<<20,  128UL<<20, 256UL<<20
    };
    /* i9-13900HX: L1d=48KB/core, L2=2MB/core, L3=36MB shared */
    const char *labels[] = {
        "8KB(L1)", "32KB(L1)", "128KB(L2)", "512KB(L2)",
        "2MB(L3)", "8MB(L3)",  "32MB(L3)",  "128MB(RAM)", "256MB(RAM)"
    };
    int n = sizeof(sizes) / sizeof(sizes[0]);

    printf("%-16s  %10s  %10s  %10s\n",
           "WorkingSet", "median_ns", "min_ns", "max_ns");
    printf("%-16s  %10s  %10s  %10s\n",
           "----------", "---------", "------", "------");

    for (int s = 0; s < n; s++) {
        size_t elem = sizes[s] / sizeof(size_t);
        size_t *arr = malloc(sizes[s]);
        if (!arr) { fprintf(stderr, "malloc failed\n"); continue; }

        build_chase(arr, elem);

        double samples[RUNS];
        for (int r = 0; r < RUNS; r++) {
            /* Warm-up: one full pass through the chain */
            volatile size_t idx = 0;
            for (size_t i = 0; i < elem; i++) idx = arr[idx];

            idx = 0;
            uint64_t t0 = ns_now();
            for (size_t i = 0; i < ITERATIONS; i++) idx = arr[idx];
            uint64_t t1 = ns_now();
            (void)idx;
            samples[r] = (double)(t1 - t0) / (double)ITERATIONS;
        }

        isort(samples, RUNS);
        printf("%-16s  %10.2f  %10.2f  %10.2f\n",
               labels[s], samples[RUNS/2], samples[0], samples[RUNS-1]);
        free(arr);
    }
    return 0;
}
