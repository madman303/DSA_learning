/* 06_optimization_trap.c —— 计时的头号陷阱：编译器把你"要计时"的代码删了
 *
 * 编译并对比（推荐用 make O0，它会一次跑出两个版本的对照）：
 *   gcc -std=c11 -O0 06_optimization_trap.c -o 06_O0 && ./06_O0
 *   gcc -std=c11 -O2 06_optimization_trap.c -o 06_O2 && ./06_O2
 *
 * 预期现象：
 *   -O0：unused 与 used 耗时接近；
 *   -O2：unused 耗时≈0（整个循环被删除），used 仍有明显耗时。
 * 这说明：如果被测代码的结果没有任何"可观测"的用处，编译器有权不算它。
 */
#include <stdio.h>
#include <time.h>

#define N 200000000LL

/* 线性同余递推：非线性，编译器无法把它化简成等比/等差公式 */
static unsigned long long lcg(unsigned long long x) {
    return x * 6364136223846793005ULL + 1442695040888963407ULL;
}

static volatile long long g_sink = 0;   /* volatile：禁止编译器优化掉这次写入 */

/* 结果没人用 -> 整个循环是可删除的死代码 */
static void unused_loop(void) {
    unsigned long long x = 1;
    for (long long i = 0; i < N; i++) {
        x = lcg(x);
    }
    /* x 从未被读取 */
}

/* 结果写入 volatile -> 循环必须真的执行 */
static void used_loop(void) {
    unsigned long long x = 1;
    for (long long i = 0; i < N; i++) {
        x = lcg(x);
    }
    g_sink = (long long)x;
}

static double elapsed(void (*f)(void)) {
    clock_t t0 = clock();
    f();
    clock_t t1 = clock();
    return (double)(t1 - t0) / (double)CLOCKS_PER_SEC;
}

int main(void) {
#if defined(__OPTIMIZE__)
    printf("[本程序以优化模式编译：__OPTIMIZE__ = %d]\n", __OPTIMIZE__);
#else
    printf("[本程序以 -O0 编译：没有定义 __OPTIMIZE__]\n");
#endif
    printf("N = %lld\n", (long long)N);
    printf("unused_loop : %.6f 秒\n", elapsed(unused_loop));
    printf("used_loop   : %.6f 秒\n", elapsed(used_loop));
    printf("g_sink      = %lld\n", g_sink);
    printf("\n若 unused_loop 明显更快，说明编译器把整个循环删掉了，\n"
           "此时你测到的不是算法速度，而是【编译器不在场】的假象。\n"
           "对策：让结果被使用（volatile / 打印 / 返回值汇合），\n"
           "      或者降低优化级别做对照，并且永远报告你的编译选项。\n");
    return 0;
}
