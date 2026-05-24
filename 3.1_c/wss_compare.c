#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef __linux__
#include <sched.h>
#endif

#define MIN_SIZE   (4ULL * 1024ULL)
#define MAX_SIZE   (1ULL * 1024ULL * 1024ULL * 1024ULL)
#define CACHE_LINE 64ULL
#define REPEATS    5
#define MIN_STEPS  2000000LL
#define MAX_STEPS  10000000LL
#define NS_PER_S   1000000000.0

static inline void *slot_addr(unsigned char *base, size_t i)
{
    return base + i * CACHE_LINE;
}

static uint64_t xorshift64(uint64_t *state)
{
    uint64_t x = *state;

    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;

    return x;
}

static double now_ns(void)
{
    struct timespec t;

    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec * NS_PER_S + (double)t.tv_nsec;
}

static void try_pin_cpu0(void)
{
#ifdef __linux__
    cpu_set_t set;

    CPU_ZERO(&set);
    CPU_SET(0, &set);
    if (sched_setaffinity(0, sizeof(set), &set) != 0) {
        fprintf(stderr, "warning: sched_setaffinity failed: %s\n", strerror(errno));
    }
#endif
}

__attribute__((noinline))
static double measure_sequential(unsigned char *buf, size_t size, long long total)
{
    volatile unsigned long long sum = 0;
    size_t slots = size / CACHE_LINE;
    double t0;
    double t1;

    for (size_t i = 0; i < size; i += CACHE_LINE) {
        sum += buf[i];
    }

    t0 = now_ns();
    for (long long k = 0; k < total; k++) {
        size_t idx = (size_t)k % slots;
        sum += buf[idx * CACHE_LINE];
    }
    t1 = now_ns();

    asm volatile("" : : "r"(sum) : "memory");
    return (t1 - t0) / (double)total;
}

__attribute__((noinline))
static double measure_random(void *start, long long total)
{
    void *p = start;
    double t0;
    double t1;

    for (int i = 0; i < 1000; i++) {
        asm volatile("" : "+r"(p) : : "memory");
        p = *(void **)p;
    }

    t0 = now_ns();
    for (long long k = 0; k < total; k++) {
        asm volatile("" : "+r"(p) : : "memory");
        p = *(void **)p;
    }
    t1 = now_ns();

    asm volatile("" : : "r"(p) : "memory");
    return (t1 - t0) / (double)total;
}

static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a;
    double y = *(const double *)b;

    return (x > y) - (x < y);
}

static void print_size(size_t size)
{
    const char *unit = "B";
    double value = (double)size;

    if (size >= 1024ULL * 1024ULL * 1024ULL) {
        value /= 1024.0 * 1024.0 * 1024.0;
        unit = "GB";
    } else if (size >= 1024ULL * 1024ULL) {
        value /= 1024.0 * 1024.0;
        unit = "MB";
    } else if (size >= 1024ULL) {
        value /= 1024.0;
        unit = "KB";
    }

    printf("%8.0f %-2s", value, unit);
}

int main(void)
{
    uint64_t rng = (uint64_t)time(NULL) ^ (uint64_t)(uintptr_t)&rng;

    try_pin_cpu0();

    printf("%-12s %16s %16s\n", "WorkingSet", "Sequential(ns)", "Random(ns)");
    printf("%-12s %16s %16s\n", "----------", "--------------", "----------");

    for (size_t size = MIN_SIZE; size <= MAX_SIZE; size *= 2) {
        size_t slots = size / CACHE_LINE;
        unsigned char *buf = malloc(size);
        size_t *perm = malloc(slots * sizeof(*perm));
        long long total = (long long)slots * 5LL;
        double seq[REPEATS];
        double rnd[REPEATS];

        if (buf == NULL || perm == NULL) {
            fprintf(stderr, "malloc failed at %zu bytes\n", size);
            free(buf);
            free(perm);
            break;
        }

        if (total < MIN_STEPS) {
            total = MIN_STEPS;
        }
        if (total > MAX_STEPS) {
            total = MAX_STEPS;
        }

        for (size_t i = 0; i < size; i++) {
            buf[i] = (unsigned char)i;
        }

        for (size_t i = 0; i < slots; i++) {
            perm[i] = i;
        }

        for (size_t i = slots - 1; i > 0; i--) {
            size_t j = (size_t)(xorshift64(&rng) % (i + 1));
            size_t tmp = perm[i];
            perm[i] = perm[j];
            perm[j] = tmp;
        }

        for (size_t i = 0; i < slots; i++) {
            size_t cur = perm[i];
            size_t next = perm[(i + 1) % slots];
            *(void **)slot_addr(buf, cur) = slot_addr(buf, next);
        }

        for (int r = 0; r < REPEATS; r++) {
            seq[r] = measure_sequential(buf, size, total);
            rnd[r] = measure_random(slot_addr(buf, 0), total);
        }

        qsort(seq, REPEATS, sizeof(seq[0]), cmp_double);
        qsort(rnd, REPEATS, sizeof(rnd[0]), cmp_double);

        print_size(size);
        printf(" %16.2f %16.2f\n", seq[REPEATS / 2], rnd[REPEATS / 2]);
        fflush(stdout);

        free(perm);
        free(buf);
    }

    return 0;
}
