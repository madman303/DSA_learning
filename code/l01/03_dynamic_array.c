/* 03_dynamic_array.c —— 动态数组：malloc / realloc / free 与指数扩容的均摊分析
 * 编译：gcc -std=c11 -Wall -Wextra -O2 03_dynamic_array.c -o 03_dynamic_array
 *
 * 这是本课程第一个真正的"数据结构"：它把"数组 + 长度 + 容量"三件事
 * 封装成一个类型，并提供 push_back 操作。链表、栈、队列、树都沿用它。
 */
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int *data;      /* 指向堆上的连续内存块 */
    int size;       /* 当前元素个数 */
    int capacity;   /* 当前内存块能容纳的元素个数 */
} DynamicArray;

/* 统计信息，仅用于教学演示 */
static long g_realloc_calls = 0;
static long g_copied_elements = 0;

/* 保证容量至少为 need；成功返回 1，失败返回 0（失败时旧内存原样保留） */
static int ensure_capacity(DynamicArray *da, int need) {
    if (need <= da->capacity) {
        return 1;
    }
    int newcap = da->capacity ? da->capacity : 2;
    while (newcap < need) {
        newcap *= 2;                       /* 指数增长：容量翻倍 */
    }
    int *tmp = (int *)realloc(da->data, (size_t)newcap * sizeof(int));
    if (!tmp) {
        return 0;                          /* 关键：不要写 da->data = realloc(...) */
    }
    g_realloc_calls++;
    g_copied_elements += da->size;         /* realloc 需要把旧数据搬到新块 */
    da->data = tmp;
    da->capacity = newcap;
    return 1;
}

static int push_back(DynamicArray *da, int v) {
    if (!ensure_capacity(da, da->size + 1)) {
        return 0;
    }
    da->data[da->size++] = v;
    return 1;
}

static int get(const DynamicArray *da, int i, int *out) {
    if (i < 0 || i >= da->size) {
        return 0;                          /* 越界：由封装层拦住，而不是让程序崩 */
    }
    *out = da->data[i];
    return 1;
}

static void da_destroy(DynamicArray *da) {
    free(da->data);                        /* 只释放动态分配的起始地址 */
    da->data = NULL;
    da->size = 0;
    da->capacity = 0;
}

int main(void) {
    /* ---- 演示 1：观察容量如何翻倍 ---- */
    printf("=== 容量增长轨迹 ===\n");
    DynamicArray da = {0};
    for (int i = 1; i <= 20; i++) {
        int old_cap = da.capacity;
        if (!push_back(&da, i * 10)) {
            fprintf(stderr, "push_back 失败\n");
            return 1;
        }
        if (da.capacity != old_cap) {
            printf("size = %2d 时扩容：capacity %d -> %d\n", da.size, old_cap, da.capacity);
        }
    }
    printf("最终 size = %d, capacity = %d\n", da.size, da.capacity);

    int v = 0;
    printf("随机访问 da[7] = %d（O(1)）\n", get(&da, 7, &v) ? v : -1);
    printf("越界访问 da[999] 被拦截：%s\n", get(&da, 999, &v) ? "未拦截" : "已拦截");

    da_destroy(&da);

    /* ---- 演示 2：均摊代价 O(1) ---- */
    printf("\n=== 均摊代价实测（n = 1000000）===\n");
    g_realloc_calls = 0;
    g_copied_elements = 0;
    const int n = 1000000;
    DynamicArray big = {0};
    for (int i = 0; i < n; i++) {
        if (!push_back(&big, i)) {
            fprintf(stderr, "push_back 失败\n");
            return 1;
        }
    }
    printf("n = %d 次 push_back\n", n);
    printf("实际 realloc 次数      = %ld   (约 log2(n) = 20 量级)\n", g_realloc_calls);
    printf("累计搬运元素总数        = %ld   (约 2n)\n", g_copied_elements);
    printf("平均每次 push_back 搬运 = %.3f 个元素 -> 均摊 O(1)\n",
           (double)g_copied_elements / n);
    printf("最终 capacity = %d\n", big.capacity);
    da_destroy(&big);

    printf("\n结论：若每次只扩容 1 个元素，累计搬运是 O(n^2)；\n"
           "      按倍数扩容，几何级数求和给出 O(n)，均摊到每次操作是 O(1)。\n");
    return 0;
}
