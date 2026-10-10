/* 05_timing_fib.c —— 计时：Fibonacci 递归 vs 迭代
 * 编译：gcc -std=c11 -Wall -Wextra -O2 05_timing_fib.c -o 05_timing_fib
 *
 * 课件作业第 3 题的 C 语言解答：比较同一算法的递归实现与迭代实现。
 * 计时用两种时钟：
 *   clock()           -> CPU 时间（不含等待、调度）
 *   clock_gettime()   -> 墙钟时间（真实流逝的时间）
 */
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <time.h>

/* ---------- 两种实现 ---------- */
static long long fib_rec(int n) {
    if (n < 2) {
        return n;
    }
    return fib_rec(n - 1) + fib_rec(n - 2);   /* 指数级：重复计算同一子问题 */
}

static long long fib_iter(int n) {
    long long a = 0, b = 1;
    for (int i = 0; i < n; i++) {
        long long next = a + b;
        a = b;
        b = next;
    }
    return a;
}

/* ---------- 计时工具 ---------- */
static double cpu_sec(clock_t t0, clock_t t1) {
    return (double)(t1 - t0) / (double)CLOCKS_PER_SEC;
}

static double wall_sec(struct timespec t0, struct timespec t1) {
    return (double)(t1.tv_sec - t0.tv_sec) + (double)(t1.tv_nsec - t0.tv_nsec) / 1e9;
}

int main(void) {
    printf("CLOCKS_PER_SEC = %ld\n\n", (long)CLOCKS_PER_SEC);

    printf("%-8s %-14s %-14s %-14s %-14s\n",
           "n", "rec结果", "rec CPU(s)", "iter结果", "iter CPU(s)");
    printf("--------------------------------------------------------------------------\n");

    for (int n = 20; n <= 40; n += 5) {
        clock_t c0 = clock();
        struct timespec w0;
        clock_gettime(CLOCK_MONOTONIC, &w0);
        long long r = fib_rec(n);
        clock_t c1 = clock();
        struct timespec w1;
        clock_gettime(CLOCK_MONOTONIC, &w1);
        double rec_cpu = cpu_sec(c0, c1), rec_wall = wall_sec(w0, w1);

        c0 = clock();
        clock_gettime(CLOCK_MONOTONIC, &w0);
        long long it = fib_iter(n);
        c1 = clock();
        clock_gettime(CLOCK_MONOTONIC, &w1);
        double it_cpu = cpu_sec(c0, c1), it_wall = wall_sec(w0, w1);

        printf("n=%-6d F=%-12lld %-14.6f F=%-12lld %-14.6f\n",
               n, r, rec_cpu, it, it_cpu);
        (void)rec_wall;
        (void)it_wall;
    }

    clock_t t0 = clock();
    long long f40 = fib_rec(40);
    clock_t t1 = clock();
    clock_t u0 = clock();
    long long i40 = fib_iter(40);
    clock_t u1 = clock();
    printf("\n单独对比 n = 40：\n");
    printf("  递归 F(40) = %lld, CPU 时间 = %.3f 秒\n", f40, cpu_sec(t0, t1));
    printf("  迭代 F(40) = %lld, CPU 时间 = %.8f 秒\n", i40, cpu_sec(u0, u1));

    /* ---- 精确测量：重复多次，求"单次调用"的代价 ---- */
    const int reps_iter = 1000000;
    long long acc = 0;
    clock_t b0 = clock();
    for (int i = 0; i < reps_iter; i++) {
        acc += fib_iter(34);
    }
    clock_t b1 = clock();
    double total_iter = cpu_sec(b0, b1);
    printf("\nfib_iter(34) 重复 %d 次：总 %.6f 秒 -> 单次 %.1f 纳秒\n",
           reps_iter, total_iter, total_iter / reps_iter * 1e9);

    const int reps_rec = 100;
    long long acc2 = 0;
    clock_t d0 = clock();
    for (int i = 0; i < reps_rec; i++) {
        acc2 += fib_rec(34);
    }
    clock_t d1 = clock();
    double total_rec = cpu_sec(d0, d1);
    printf("fib_rec (34) 重复 %d 次：总 %.6f 秒 -> 单次 %.3f 毫秒\n",
           reps_rec, total_rec, total_rec / reps_rec * 1e3);
    printf("（校验和 acc = %lld, acc2 = %lld）\n", acc, acc2);

    printf("\n结论：\n");
    printf("  迭代版本 O(n)，递归版本 O(phi^n)，phi = (1+sqrt(5))/2 = 1.618...\n");
    printf("  递归的重复子问题使工作量随 n 指数爆炸：n 每加 1，耗时约乘以 1.618。\n");
    printf("  这不是\"机器慢\"，而是算法复杂度不同 —— 这正是本课程要教的核心。\n");
    return 0;
}
