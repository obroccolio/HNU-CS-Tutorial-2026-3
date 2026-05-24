/*
 * Sequential vs random read-bandwidth benchmark (v2).
 * Changes from v1:
 *   - CPU-core pinning (sched_setaffinity)
 *   - bench_rand: replaced "% n" with "& mask" — integer division (~30 cycles)
 *     was dominating the L1 loop and masking true random-access bandwidth
 *   - RUNS=5 repeated measurements; reports median GB/s
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sched.h>

#define RUNS 5

static uint64_t ns_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static uint64_t xorshift64(uint64_t *state) {
    uint64_t x = *state;
    x ^= x << 13; x ^= x >> 7; x ^= x << 17;
    return *state = x;
}

static double bench_seq(volatile uint64_t *arr, size_t n, uint64_t iters) {
    volatile uint64_t sink = 0;
    for (size_t i = 0; i < n; i++) sink += arr[i];  /* warm-up */
    uint64_t t0 = ns_now();
    for (uint64_t r = 0; r < iters; r++)
        for (size_t i = 0; i < n; i += 8)            /* stride = 1 cacheline */
            sink += arr[i];
    uint64_t t1 = ns_now();
    (void)sink;
    double bytes = (double)iters * (double)(n >> 3) * 64.0;
    return bytes / ((double)(t1 - t0) / 1e9) / 1e9;
}

static double bench_rand(volatile uint64_t *arr, size_t n, uint64_t iters) {
    volatile uint64_t sink = 0;
    uint64_t state = 0xdeadbeefcafe1234ULL;
    /* n is always a power of 2: & mask costs 1 cycle vs % n ~30 cycles */
    size_t mask = n - 1;
    for (size_t i = 0; i < n; i++) sink += arr[i];  /* warm-up */
    uint64_t t0 = ns_now();
    uint64_t total = iters * 1024;
    for (uint64_t i = 0; i < total; i++)
        sink += arr[xorshift64(&state) & mask];
    uint64_t t1 = ns_now();
    (void)sink;
    return (double)total * 8.0 / ((double)(t1 - t0) / 1e9) / 1e9;
}

static void isort(double *a, int n) {
    for (int i = 1; i < n; i++) {
        double k = a[i]; int j = i - 1;
        while (j >= 0 && a[j] > k) { a[j+1] = a[j]; j--; }
        a[j+1] = k;
    }
}

int main(void) {
    cpu_set_t cs;
    CPU_ZERO(&cs); CPU_SET(0, &cs);
    sched_setaffinity(0, sizeof(cs), &cs);

    /* All sizes are powers of 2 (required by & mask in bench_rand) */
    size_t     sizes[]  = { 32UL<<10, 512UL<<10, 16UL<<20, 256UL<<20 };
    const char *labels[] = { "L1(32KB)", "L2(512KB)", "L3(16MB)", "RAM(256MB)" };
    uint64_t   iters[]  = { 2000, 200, 10, 2 };
    int n = 4;

    printf("%-12s  %14s  %14s  %10s\n",
           "Level", "Seq(GB/s)", "Rand(GB/s)", "Ratio");
    printf("%-12s  %14s  %14s  %10s\n",
           "-----", "---------", "----------", "-----");

    for (int s = 0; s < n; s++) {
        size_t elem = sizes[s] / sizeof(uint64_t);
        volatile uint64_t *arr = malloc(sizes[s]);
        if (!arr) { fprintf(stderr, "malloc failed\n"); continue; }
        for (size_t i = 0; i < elem; i++) arr[i] = i;

        double sseq[RUNS], srand[RUNS];
        for (int r = 0; r < RUNS; r++) {
            sseq[r]  = bench_seq(arr, elem, iters[s]);
            srand[r] = bench_rand(arr, elem, iters[s]);
        }
        isort(sseq,  RUNS);
        isort(srand, RUNS);

        double seq = sseq[RUNS/2], rnd = srand[RUNS/2];
        printf("%-12s  %14.2f  %14.2f  %10.1fx\n",
               labels[s], seq, rnd, seq / rnd);
        free((void *)arr);
    }
    return 0;
}
