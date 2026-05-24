#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MIN_SIZE   (4 * 1024ULL)
#define MAX_SIZE   (1ULL * 1024 * 1024 * 1024)
#define CACHE_LINE 64
#define REPEATS    5
#define NS_PER_S   1e9

static inline void *slot_addr(unsigned char *base, size_t i) {
    return base + i * CACHE_LINE;
}

// noinline: prevents the compiler from seeing through this call and
// optimizing the pointer-chase loop away. From main's perspective, the
// memory behind |start| escapes and must be read exactly as written.
__attribute__((noinline))
static double measure_chase(void *start, long long total) {
    struct timespec t0, t1;
    void *p = start;

    // warm-up: walk a few hundred steps to prime TLB / cache
    for (int i = 0; i < 1000; i++) {
        asm volatile("" : "+r"(p) : : "memory");
        p = *(void **)p;
    }

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (long long k = 0; k < total; k++) {
        asm volatile("" : "+r"(p) : : "memory");
        p = *(void **)p;
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);

    // tell the compiler p is "used" — prevents dead-store elimination
    asm volatile("" : : "r"(p) : "memory");

    double elapsed = (t1.tv_sec - t0.tv_sec)
                   + (t1.tv_nsec - t0.tv_nsec) / NS_PER_S;
    return (elapsed * NS_PER_S) / (double)total;
}

int main(void) {
    srand((unsigned)time(NULL));

    printf("%-16s %12s\n", "Working Set", "Avg Access (ns)");
    printf("%-16s %12s\n", "------------", "-------------");

    for (size_t size = MIN_SIZE; size <= MAX_SIZE; size *= 2) {
        if ((size & (size - 1)) != 0) continue;

        size_t num_slots = size / CACHE_LINE;
        if (num_slots < 2) num_slots = 2;

        unsigned char *buf = (unsigned char *)malloc(size);
        if (!buf) { fprintf(stderr, "malloc(%zu) failed\n", size); break; }

        // ---- Fisher-Yates shuffle: build random cyclic permutation ----
        size_t *perm = (size_t *)malloc(num_slots * sizeof(size_t));
        if (!perm) { free(buf); break; }
        for (size_t i = 0; i < num_slots; i++) perm[i] = i;
        for (size_t i = num_slots - 1; i > 0; i--) {
            size_t j = (size_t)rand() % (i + 1);
            size_t t = perm[i]; perm[i] = perm[j]; perm[j] = t;
        }

        // wire the random chain:  perm[i] --> perm[(i+1) % N]
        for (size_t i = 0; i < num_slots; i++) {
            size_t cur = perm[i];
            size_t nxt = perm[(i + 1) % num_slots];
            *(void **)slot_addr(buf, cur) = slot_addr(buf, nxt);
        }
        free(perm);

        // follow count: at least num_slots * 5, bounded [2M .. 10M]
        long long total = (long long)num_slots * 5;
        if (total <  2000000) total =  2000000;
        if (total > 10000000) total = 10000000;

        double times[REPEATS];
        for (int r = 0; r < REPEATS; r++) {
            times[r] = measure_chase(slot_addr(buf, 0), total);
        }

        // median
        for (int i = 0; i < REPEATS - 1; i++)
            for (int j = i + 1; j < REPEATS; j++)
                if (times[i] > times[j]) {
                    double t = times[i]; times[i] = times[j]; times[j] = t;
                }
        double median = times[REPEATS / 2];

        const char *unit = "B";
        double ds = (double)size;
        if      (ds >= 1024*1024*1024) { ds /= 1024*1024*1024; unit = "GB"; }
        else if (ds >= 1024*1024)      { ds /= 1024*1024;      unit = "MB"; }
        else if (ds >= 1024)           { ds /= 1024;           unit = "KB"; }

        printf("%8.0f %-7s %10.2f ns\n", ds, unit, median);
        fflush(stdout);

        free(buf);
    }
    return 0;
}
