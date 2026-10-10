#!/usr/bin/env python3
"""07_python_timing.py —— Python 侧的计时与 C 的对照

课件作业第 3 题的 Python 版本。注意 Python 的递归深度限制与解释执行开销。
运行：python3 07_python_timing.py
"""
import sys
import time
import timeit

sys.setrecursionlimit(10000)


def fib_rec(n: int) -> int:
    return n if n < 2 else fib_rec(n - 1) + fib_rec(n - 2)


def fib_iter(n: int) -> int:
    a, b = 0, 1
    for _ in range(n):
        a, b = b, a + b
    return a


def bench(fn, n: int, repeat: int = 5) -> tuple[int, float]:
    """取 repeat 次中的最小值：最小值最接近算法的真实代价。"""
    best = float("inf")
    value = 0
    for _ in range(repeat):
        t0 = time.perf_counter()
        value = fn(n)
        best = min(best, time.perf_counter() - t0)
    return value, best


def main() -> None:
    print(f"{'n':<6}{'递归结果':<16}{'递归(秒)':<16}{'迭代结果':<16}{'迭代(秒)':<16}")
    print("-" * 70)
    for n in (20, 25, 30, 32, 34):
        r, rt = bench(fib_rec, n, repeat=3)
        i, it = bench(fib_iter, n, repeat=5)
        print(f"{n:<6}{r:<16}{rt:<16.6f}{i:<16}{it:<16.8f}")

    print("\n用 timeit 做更严格的微观测量：")
    t_empty = min(timeit.repeat("pass", number=1_000_000, repeat=5))
    t_add = min(timeit.repeat("sum(range(100))", number=100_000, repeat=5))
    t_iter34 = min(timeit.repeat("fib_iter(34)", globals=globals(), number=100_000, repeat=5)) / 100_000
    t_rec30 = min(timeit.repeat("fib_rec(30)", globals=globals(), number=20, repeat=5)) / 20
    print(f"  pass                x 1e6 : {t_empty:.6f} 秒")
    print(f"  sum(range(100))     x 1e5 : {t_add:.6f} 秒")
    print(f"  fib_iter(34) 单次          : {t_iter34 * 1e9:.0f} 纳秒")
    print(f"  fib_rec (30) 单次          : {t_rec30 * 1e3:.1f} 毫秒")

    print("""
对照结论（把 C 侧 05_timing_fib 的输出抄在旁边一起看）：
  1. 同一个 fib_rec(30)：C 约 0.9 毫秒，Python 约 59 毫秒 —— 语言实现差约 60 倍常数因子。
  2. 同一个 fib_iter(34)：C 约 7 纳秒，Python 约 490 纳秒 —— 差距约 70 倍，同样是常数因子。
  3. 但在同一门语言内，fib_rec 与 fib_iter 之间是指数 vs 线性的复杂度差距：
     n 每增加 1，递归耗时约乘以 1.618；n 增加 10，耗时约乘以 120。
     C 的 fib_rec(34) 要 6.8 毫秒，而 Python 的 fib_iter(34) 只要 0.5 微秒 ——
     慢语言配上好算法，反而比快语言配坏算法快一万倍。
     这类差距换机器、换编译器都追不回来：先选对算法，再优化实现。
  4. Python 计时用 time.perf_counter()（单调、高精度）；批量比较用 timeit
     （自动多次、取最小值、可关闭 GC 干扰）。永远报告测量次数与取值方式。
""")


if __name__ == "__main__":
    main()
