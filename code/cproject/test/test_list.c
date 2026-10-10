/*
 * test_list.c —— list / sort 两个模块的公开接口测试。
 *
 * 规则：本文件只包含公开头文件 list.h 与 sort.h，链接共享库 liblist.so，
 *       绝不包含 list.c / sort.c，也不用库的内部 static 函数。
 * 风格：断言式。每条断言失败就打印“期望值 vs 实际值”，
 *       全部跑完后再汇总；有失败则返回退出码 1，供 make / 脚本判断。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "sort.h"

/* ------------------------------------------------------------------ */
/* 断言框架                                                           */
/* ------------------------------------------------------------------ */

static int g_pass = 0;
static int g_fail = 0;

#define FAILF(...)                                     \
    do {                                               \
        g_fail++;                                      \
        printf("  [FAIL] %s:%d: ", __FILE__, __LINE__); \
        printf(__VA_ARGS__);                           \
        putchar('\n');                                 \
    } while (0)

/* 条件断言 */
#define CHECK_TRUE(expr)                                     \
    do {                                                     \
        if (expr) {                                          \
            g_pass++;                                        \
        } else {                                             \
            FAILF("%s 期望为真，实际为假", #expr);            \
        }                                                    \
    } while (0)

/* 整数断言：第一个参数是期望值，第二个是实际值 */
#define CHECK_EQ_INT(expect, actual)                                     \
    do {                                                                 \
        long long e_ = (long long)(expect);                              \
        long long a_ = (long long)(actual);                              \
        if (e_ == a_) {                                                  \
            g_pass++;                                                    \
        } else {                                                         \
            FAILF("%s 期望 %lld，实际 %lld", #actual, e_, a_);            \
        }                                                                \
    } while (0)

/* 指针断言（比较结点地址，用于环入口/交点/中点） */
#define CHECK_EQ_PTR(expect, actual)                                     \
    do {                                                                 \
        const void *e_ = (const void *)(expect);                         \
        const void *a_ = (const void *)(actual);                         \
        if (e_ == a_) {                                                  \
            g_pass++;                                                    \
        } else {                                                         \
            FAILF("%s 期望 %p，实际 %p", #actual, e_, a_);                \
        }                                                                \
    } while (0)

/* ------------------------------------------------------------------ */
/* 测试辅助                                                           */
/* ------------------------------------------------------------------ */

#define MAXELEM 8192

/* 用 list.h 的公开接口建一条链 */
static List *make_list(const int *a, size_t n)
{
    List *l = list_create();
    if (l == NULL) {
        FAILF("list_create 返回 NULL（内存不足）");
        return NULL;
    }
    if (!list_from_array(l, a, n)) {
        FAILF("list_from_array 失败");
    }
    return l;
}

/* 用 node_create 手工串一条裸链（无哨兵），供环/交点等算法使用 */
static ListNode *make_chain(const int *a, size_t n)
{
    ListNode *head = NULL;
    ListNode *tail = NULL;
    for (size_t i = 0; i < n; i++) {
        ListNode *nd = node_create(a[i]);
        if (nd == NULL) {
            node_free_chain(head);
            FAILF("node_create 失败");
            return NULL;
        }
        if (tail != NULL) {
            tail->next = nd;
        } else {
            head = nd;
        }
        tail = nd;
    }
    return head;
}

/* 校验链表内容与长度；不符时打印期望值与实际链 */
static void check_list(const List *l, const int *want, size_t n, const char *what)
{
    if (l == NULL) {
        FAILF("%s: 链表为 NULL", what);
        return;
    }
    if (list_size(l) != n) {
        FAILF("%s: list_size 期望 %zu，实际 %zu", what, n, list_size(l));
        return;
    }
    int got[MAXELEM];
    if (n > MAXELEM) {
        FAILF("%s: 测试缓冲不足", what);
        return;
    }
    if (!list_to_array(l, got, n)) {
        FAILF("%s: list_to_array 失败", what);
        return;
    }
    for (size_t i = 0; i < n; i++) {
        if (got[i] != want[i]) {
            FAILF("%s: 第 %zu 个元素期望 %d，实际 %d，实际链为 ", what, i, want[i], got[i]);
            node_print_chain(list_first(l));
            putchar('\n');
            return;
        }
    }
    g_pass++;
}

/* 校验数组是否与期望完全一致 */
static void check_array(const int *got, const int *want, size_t n, const char *what)
{
    for (size_t i = 0; i < n; i++) {
        if (got[i] != want[i]) {
            FAILF("%s: a[%zu] 期望 %d，实际 %d", what, i, want[i], got[i]);
            return;
        }
    }
    g_pass++;
}

/* qsort 用作独立参照实现 */
static int cmp_int(const void *x, const void *y)
{
    int a = *(const int *)x;
    int b = *(const int *)y;
    return (a > b) - (a < b);
}

/* 确定性伪随机（不依赖 rand 的实现差异） */
static unsigned g_seed = 20260901u;
static int lcg_next(int mod)
{
    g_seed = g_seed * 1103515245u + 12345u;
    return (int)((g_seed >> 16) % (unsigned)mod);
}

/* ------------------------------------------------------------------ */
/* 1. 生命周期与查询：空链、NULL 安全、长度                           */
/* ------------------------------------------------------------------ */

static void test_lifecycle(void)
{
    puts("-- 生命周期/查询：空链、NULL 参数、长度");

    List *l = list_create();
    CHECK_TRUE(l != NULL);
    CHECK_EQ_INT(0, list_size(l));
    CHECK_TRUE(list_empty(l));
    CHECK_EQ_PTR(NULL, list_first(l));

    list_print(l);                 /* 空链打印 [] (size=0) */
    list_reverse(l);               /* 空链反转不崩 */
    list_reverse_recursive(l);
    CHECK_EQ_INT(0, list_size(l));
    CHECK_TRUE(!list_delete_at(l, 0, NULL));
    CHECK_TRUE(!list_delete_value(l, 1));
    CHECK_EQ_PTR(NULL, list_find(l, 1));
    CHECK_TRUE(!list_insert_at(l, 1, 5)); /* pos > size 应失败 */
    CHECK_TRUE(list_insert_at(l, 0, 5));  /* pos == size == 0 合法 */
    check_list(l, (const int[]){5}, 1, "空链 0 号位插入");
    CHECK_TRUE(list_delete_at(l, 0, NULL));
    list_destroy(l);

    /* NULL 参数的安全行为 */
    CHECK_EQ_INT(0, list_size(NULL));
    CHECK_TRUE(list_empty(NULL));
    CHECK_EQ_PTR(NULL, list_first(NULL));
    CHECK_TRUE(!list_push_back(NULL, 1));
    CHECK_TRUE(!list_push_front(NULL, 1));
    CHECK_TRUE(!list_insert_at(NULL, 0, 1));
    CHECK_TRUE(!list_delete_at(NULL, 0, NULL));
    CHECK_TRUE(!list_delete_value(NULL, 1));
    CHECK_EQ_PTR(NULL, list_find(NULL, 1));
    CHECK_TRUE(!list_from_array(NULL, (const int[]){1}, 1));
    CHECK_TRUE(!list_to_array(NULL, (int[]){0}, 1));
    list_print(NULL);
    list_destroy(NULL);
    list_clear(NULL);
    node_free(NULL);
    node_free_chain(NULL);
    CHECK_TRUE(true); /* 走到这里说明 NULL 输入没有崩溃 */
}

/* ------------------------------------------------------------------ */
/* 2. 构造：尾插、头插、数组转链、导出数组、打印                      */
/* ------------------------------------------------------------------ */

static void test_build(void)
{
    puts("-- 构造：尾插/头插/数组转链/导出数组/打印");

    List *l = list_create();
    CHECK_TRUE(list_push_back(l, 1));
    CHECK_TRUE(list_push_back(l, 2));
    CHECK_TRUE(list_push_back(l, 3));
    check_list(l, (const int[]){1, 2, 3}, 3, "三次尾插");
    CHECK_TRUE(list_push_front(l, 0));
    check_list(l, (const int[]){0, 1, 2, 3}, 4, "再头插一个 0");
    list_print(l);

    CHECK_TRUE(list_from_array(l, (const int[]){9, 8}, 2));
    check_list(l, (const int[]){9, 8}, 2, "数组转链覆盖旧内容");
    CHECK_TRUE(!list_from_array(l, NULL, 1)); /* 非法：a 为 NULL 而 n > 0 */
    check_list(l, (const int[]){9, 8}, 2, "失败调用不改动链表");
    CHECK_TRUE(list_from_array(l, NULL, 0));
    check_list(l, NULL, 0, "n == 0 且 a == NULL 清空链表");

    int buf[8];
    CHECK_TRUE(list_from_array(l, (const int[]){4, 5, 6}, 3));
    CHECK_TRUE(list_to_array(l, buf, 8));
    check_array(buf, (const int[]){4, 5, 6}, 3, "list_to_array 内容");
    CHECK_TRUE(!list_to_array(l, buf, 2)); /* 容量不足应失败 */
    CHECK_TRUE(!list_to_array(l, NULL, 8));

    /* 单结点与两结点的基本性质 */
    List *one = make_list((const int[]){7}, 1);
    CHECK_EQ_INT(1, list_size(one));
    CHECK_TRUE(!list_empty(one));
    CHECK_EQ_PTR(list_first(one), list_middle(list_first(one)));
    CHECK_EQ_PTR(list_first(one), list_kth_from_end(list_first(one), 1));
    CHECK_EQ_PTR(NULL, list_kth_from_end(list_first(one), 2)); /* k 超出长度 */
    CHECK_TRUE(!list_has_cycle(list_first(one)));
    list_reverse(one);
    check_list(one, (const int[]){7}, 1, "单结点迭代反转");
    list_reverse_recursive(one);
    check_list(one, (const int[]){7}, 1, "单结点递归反转");
    int out = -1;
    CHECK_TRUE(list_delete_at(one, 0, &out));
    CHECK_EQ_INT(7, out);
    CHECK_EQ_INT(0, list_size(one));
    list_destroy(one);

    list_destroy(l);
}

/* ------------------------------------------------------------------ */
/* 3. 按位置插入                                                      */
/* ------------------------------------------------------------------ */

static void test_insert_at(void)
{
    puts("-- 按位置插入：头/中/尾/越界");

    List *l = make_list((const int[]){1, 2, 3}, 3);
    CHECK_TRUE(list_insert_at(l, 0, 0));
    check_list(l, (const int[]){0, 1, 2, 3}, 4, "0 号位插入（新头）");
    CHECK_TRUE(list_insert_at(l, 4, 4));
    check_list(l, (const int[]){0, 1, 2, 3, 4}, 5, "size 号位插入（尾插）");
    CHECK_TRUE(list_insert_at(l, 2, 99));
    check_list(l, (const int[]){0, 1, 99, 2, 3, 4}, 6, "中间位置插入");
    CHECK_TRUE(!list_insert_at(l, 7, 7)); /* pos > size */
    check_list(l, (const int[]){0, 1, 99, 2, 3, 4}, 6, "越界插入不改动链表");
    list_destroy(l);
}

/* ------------------------------------------------------------------ */
/* 4. 按位置删除：删头、删尾、越界、空表                              */
/* ------------------------------------------------------------------ */

static void test_delete_at(void)
{
    puts("-- 按位置删除：头/尾/中间/越界/空表");

    List *l = make_list((const int[]){10, 20, 30}, 3);
    int out = -1;

    CHECK_TRUE(list_delete_at(l, 0, &out)); /* 删头 */
    CHECK_EQ_INT(10, out);
    check_list(l, (const int[]){20, 30}, 2, "删头之后");

    CHECK_TRUE(list_delete_at(l, 1, &out)); /* 删尾 */
    CHECK_EQ_INT(30, out);
    check_list(l, (const int[]){20}, 1, "删尾之后");

    CHECK_TRUE(!list_delete_at(l, 1, &out)); /* 越界 */
    CHECK_TRUE(!list_delete_at(l, 5, NULL));
    check_list(l, (const int[]){20}, 1, "越界删除不改动链表");

    CHECK_TRUE(list_delete_at(l, 0, NULL)); /* out_val 允许为 NULL */
    CHECK_EQ_INT(0, list_size(l));
    CHECK_TRUE(!list_delete_at(l, 0, &out)); /* 空表再删应失败 */
    list_destroy(l);

    List *m = make_list((const int[]){1, 2, 3}, 3);
    CHECK_TRUE(list_delete_at(m, 1, &out)); /* 删中间 */
    CHECK_EQ_INT(2, out);
    check_list(m, (const int[]){1, 3}, 2, "删中间之后");
    list_destroy(m);
}

/* ------------------------------------------------------------------ */
/* 5. 按值查找/删除：删头、删中间、删尾、不存在                       */
/* ------------------------------------------------------------------ */

static void test_find_delete_value(void)
{
    puts("-- 按值查找/删除：头/中/尾/不存在");

    List *l = make_list((const int[]){10, 20, 30, 20}, 4);

    CHECK_EQ_PTR(NULL, list_find(l, 99));
    CHECK_TRUE(list_find(l, 20) != NULL);
    CHECK_EQ_INT(20, list_find(l, 20)->val);
    CHECK_EQ_INT(10, list_find(l, 10)->val);       /* 命中头结点 */
    CHECK_EQ_INT(30, list_find(l, 20)->next->val); /* 命中的是第一个 20 */

    CHECK_TRUE(!list_delete_value(l, 99)); /* 不存在 */
    check_list(l, (const int[]){10, 20, 30, 20}, 4, "删除不存在的值不改动链表");

    CHECK_TRUE(list_delete_value(l, 10)); /* 删头 */
    check_list(l, (const int[]){20, 30, 20}, 3, "按值删头");

    CHECK_TRUE(list_delete_value(l, 20)); /* 删中间（只删第一个） */
    check_list(l, (const int[]){30, 20}, 2, "按值删第一个匹配");

    CHECK_TRUE(list_delete_value(l, 20)); /* 删尾 */
    check_list(l, (const int[]){30}, 1, "按值删尾");

    CHECK_TRUE(!list_delete_value(l, 30 - 1));
    CHECK_TRUE(list_delete_value(l, 30));
    CHECK_EQ_INT(0, list_size(l));
    CHECK_TRUE(!list_delete_value(l, 30)); /* 空表再删 */
    list_destroy(l);
}

/* ------------------------------------------------------------------ */
/* 6. 反转：迭代 / 递归 / 反转后再反转                                */
/* ------------------------------------------------------------------ */

static void test_reverse(void)
{
    puts("-- 反转：迭代/递归/反转后再反转/偶数长度/长链");

    int src[] = {1, 2, 3, 4, 5};
    List *l = make_list(src, 5);

    list_reverse(l);
    check_list(l, (const int[]){5, 4, 3, 2, 1}, 5, "迭代反转");
    list_reverse(l);
    check_list(l, (const int[]){1, 2, 3, 4, 5}, 5, "反转后再反转（迭代）");

    list_reverse_recursive(l);
    check_list(l, (const int[]){5, 4, 3, 2, 1}, 5, "递归反转");
    list_reverse_recursive(l);
    check_list(l, (const int[]){1, 2, 3, 4, 5}, 5, "反转后再反转（递归）");

    /* 迭代与递归结果必须一致 */
    list_reverse(l);
    List *r = make_list(src, 5);
    list_reverse_recursive(r);
    size_t n = list_size(l);
    int ga[MAXELEM];
    int gb[MAXELEM];
    CHECK_TRUE(list_to_array(l, ga, n));
    CHECK_TRUE(list_to_array(r, gb, n));
    check_array(ga, gb, n, "迭代与递归反转结果一致");
    list_destroy(l);
    list_destroy(r);

    /* 偶数长度 */
    List *e = make_list((const int[]){1, 2, 3, 4}, 4);
    list_reverse(e);
    check_list(e, (const int[]){4, 3, 2, 1}, 4, "偶数长度迭代反转");
    list_reverse_recursive(e);
    check_list(e, (const int[]){1, 2, 3, 4}, 4, "偶数长度递归反转");
    list_destroy(e);

    /* 较长链，验证 size 不变与首尾 */
    enum { N = 2000 };
    static int big[N];
    for (int i = 0; i < N; i++) {
        big[i] = i;
    }
    List *b = make_list(big, N);
    list_reverse(b);
    CHECK_EQ_INT(N, list_size(b));
    CHECK_EQ_INT(N - 1, list_first(b)->val);
    CHECK_TRUE(sort_array_is_sorted(NULL, 0)); /* 顺带覆盖 NULL 检查 */
    list_reverse_recursive(b);
    CHECK_EQ_INT(0, list_first(b)->val);
    check_list(b, big, N, "长链反转两次回到原序");
    list_destroy(b);
}

/* ------------------------------------------------------------------ */
/* 7. 合并两条有序链表                                                */
/* ------------------------------------------------------------------ */

static void test_merge_sorted(void)
{
    puts("-- 有序合并：常规/含相等/空链/输入不变/别名拒绝");

    List *a = make_list((const int[]){1, 3, 5, 7}, 4);
    List *b = make_list((const int[]){2, 3, 6}, 3);
    List *out = list_create();

    CHECK_TRUE(list_merge_sorted(a, b, out));
    check_list(out, (const int[]){1, 2, 3, 3, 5, 6, 7}, 7, "归并两条有序链");
    check_list(a, (const int[]){1, 3, 5, 7}, 4, "归并后输入 a 不变");
    check_list(b, (const int[]){2, 3, 6}, 3, "归并后输入 b 不变");

    List *empty = list_create();
    CHECK_TRUE(list_merge_sorted(empty, b, out));
    check_list(out, (const int[]){2, 3, 6}, 3, "空链 + 非空链");
    CHECK_TRUE(list_merge_sorted(a, empty, out));
    check_list(out, (const int[]){1, 3, 5, 7}, 4, "非空链 + 空链");
    CHECK_TRUE(list_merge_sorted(empty, empty, out));
    check_list(out, NULL, 0, "两条空链归并");

    /* out 原有内容应被清空后重写 */
    CHECK_TRUE(list_merge_sorted(b, empty, out));
    check_list(out, (const int[]){2, 3, 6}, 3, "归并覆盖 out 旧内容");

    /* 参数与别名 */
    CHECK_TRUE(!list_merge_sorted(NULL, b, out));
    CHECK_TRUE(!list_merge_sorted(a, NULL, out));
    CHECK_TRUE(!list_merge_sorted(a, b, NULL));
    CHECK_TRUE(!list_merge_sorted(a, b, a)); /* 输出与输入别名：拒绝 */
    check_list(a, (const int[]){1, 3, 5, 7}, 4, "别名被拒后 a 不变");

    ptrdiff_t dummy = 0; /* 独立于库的裸链归并演示 */
    (void)dummy;
    ListNode *x = make_chain((const int[]){1, 4, 9}, 3);
    ListNode *y = make_chain((const int[]){2, 5}, 2);
    node_print_chain(x);
    putchar('\n');
    node_print_chain(y);
    putchar('\n');
    node_free_chain(x);
    node_free_chain(y);

    list_destroy(a);
    list_destroy(b);
    list_destroy(out);
    list_destroy(empty);
}

/* ------------------------------------------------------------------ */
/* 8. 快慢指针：中点、倒数第 k 个                                     */
/* ------------------------------------------------------------------ */

static void test_fast_slow(void)
{
    puts("-- 快慢指针：中点/倒数第 k 个/超出长度");

    CHECK_EQ_PTR(NULL, list_middle(NULL));
    CHECK_EQ_PTR(NULL, list_kth_from_end(NULL, 1));

    /* 中点为中间偏右 */
    List *l1 = make_list((const int[]){1}, 1);
    CHECK_EQ_INT(1, list_middle(list_first(l1))->val);
    list_destroy(l1);

    List *l2 = make_list((const int[]){1, 2}, 2);
    CHECK_EQ_INT(2, list_middle(list_first(l2))->val); /* 偶数长度偏右 */
    list_destroy(l2);

    List *l3 = make_list((const int[]){1, 2, 3}, 3);
    CHECK_EQ_INT(2, list_middle(list_first(l3))->val);
    list_destroy(l3);

    List *l4 = make_list((const int[]){1, 2, 3, 4}, 4);
    CHECK_EQ_INT(3, list_middle(list_first(l4))->val);
    list_destroy(l4);

    List *l5 = make_list((const int[]){1, 2, 3, 4, 5}, 5);
    CHECK_EQ_INT(3, list_middle(list_first(l5))->val);

    CHECK_EQ_INT(5, list_kth_from_end(list_first(l5), 1)->val);
    CHECK_EQ_INT(4, list_kth_from_end(list_first(l5), 2)->val);
    CHECK_EQ_INT(1, list_kth_from_end(list_first(l5), 5)->val);
    CHECK_EQ_PTR(NULL, list_kth_from_end(list_first(l5), 6)); /* k 超出长度 */
    CHECK_EQ_PTR(NULL, list_kth_from_end(list_first(l5), 0)); /* k == 0 非法 */
    CHECK_EQ_PTR(NULL, list_kth_from_end(list_first(l5), (size_t)-1));
    list_destroy(l5);
}

/* ------------------------------------------------------------------ */
/* 9. 环：检测与入口                                                  */
/* ------------------------------------------------------------------ */

static void test_cycle(void)
{
    puts("-- 环：无环/尾接中间/自环/入口为头");

    /* 无环：用公开接口建的链 */
    List *l = make_list((const int[]){1, 2, 3, 4}, 4);
    CHECK_TRUE(!list_has_cycle(list_first(l)));
    CHECK_EQ_PTR(NULL, list_cycle_entry(list_first(l)));
    CHECK_TRUE(!list_has_cycle(NULL));
    CHECK_EQ_PTR(NULL, list_cycle_entry(NULL));
    list_destroy(l);

    /* 4 -> 2：尾巴接回中间 */
    ListNode *n1 = node_create(1);
    ListNode *n2 = node_create(2);
    ListNode *n3 = node_create(3);
    ListNode *n4 = node_create(4);
    n1->next = n2;
    n2->next = n3;
    n3->next = n4;
    n4->next = n2;
    CHECK_TRUE(list_has_cycle(n1));
    CHECK_EQ_PTR(n2, list_cycle_entry(n1));
    n4->next = NULL; /* 断环再释放，否则释放会死循环 */
    node_free_chain(n1);

    /* 自环 */
    ListNode *s = node_create(9);
    s->next = s;
    CHECK_TRUE(list_has_cycle(s));
    CHECK_EQ_PTR(s, list_cycle_entry(s));
    s->next = NULL;
    node_free_chain(s);

    /* 整链成环：入口就是头 */
    ListNode *m1 = node_create(1);
    ListNode *m2 = node_create(2);
    ListNode *m3 = node_create(3);
    m1->next = m2;
    m2->next = m3;
    m3->next = m1;
    CHECK_TRUE(list_has_cycle(m1));
    CHECK_EQ_PTR(m1, list_cycle_entry(m1));
    m3->next = NULL;
    node_free_chain(m1);

    /* 用 list.h 建链后手动加环，再断环销毁；size 不受加环影响 */
    List *c = make_list((const int[]){1, 2, 3, 4, 5}, 5);
    ListNode *entry = list_first(c)->next->next; /* 第 3 个结点 */
    ListNode *tail = list_first(c);
    while (tail->next != NULL) {
        tail = tail->next;
    }
    tail->next = entry;
    CHECK_TRUE(list_has_cycle(list_first(c)));
    CHECK_EQ_PTR(entry, list_cycle_entry(list_first(c)));
    CHECK_EQ_INT(5, list_size(c));
    tail->next = NULL; /* 断环 */
    list_destroy(c);

    /* 无环的长链不应误报 */
    static int big[1000];
    for (int i = 0; i < 1000; i++) {
        big[i] = i;
    }
    List *plain = make_list(big, 1000);
    CHECK_TRUE(!list_has_cycle(list_first(plain)));
    CHECK_EQ_PTR(NULL, list_cycle_entry(list_first(plain)));
    list_destroy(plain);
}

/* ------------------------------------------------------------------ */
/* 10. 两条链的交点                                                   */
/* ------------------------------------------------------------------ */

static void test_intersection(void)
{
    puts("-- 交点：Y 形/不相交/空链/同一条链");

    ListNode *c2 = node_create(20);
    ListNode *c1 = node_create(10);
    c1->next = c2;

    ListNode *a2 = node_create(2);
    ListNode *a1 = node_create(1);
    a1->next = a2;
    a2->next = c1; /* a: 1 -> 2 -> 10 -> 20 */

    ListNode *b1 = node_create(7);
    b1->next = c1; /* b: 7 -> 10 -> 20（长度不同） */

    CHECK_EQ_PTR(c1, list_intersection(a1, b1));
    CHECK_EQ_PTR(c1, list_intersection(b1, a1)); /* 交换参数结果一致 */
    CHECK_EQ_PTR(a1, list_intersection(a1, a1)); /* 同一条链：首结点即交点 */

    ListNode *d2 = node_create(200);
    ListNode *d1 = node_create(100);
    d1->next = d2; /* 与 a 不相交 */
    CHECK_EQ_PTR(NULL, list_intersection(a1, d1));

    CHECK_EQ_PTR(NULL, list_intersection(a1, NULL));
    CHECK_EQ_PTR(NULL, list_intersection(NULL, d1));
    CHECK_EQ_PTR(NULL, list_intersection(NULL, NULL));

    /* 交点为尾结点 */
    ListNode *e1 = node_create(5);
    e1->next = c2;
    CHECK_EQ_PTR(c2, list_intersection(e1, b1));

    /* 释放：a1/a2、b1、e1 各自独有，c1/c2 共享只释放一次 */
    node_free(a1);
    node_free(a2);
    node_free(b1);
    node_free(e1);
    node_free_chain(c1);
    node_free_chain(d1);
}

/* ------------------------------------------------------------------ */
/* 11. 数组排序                                                       */
/* ------------------------------------------------------------------ */

typedef void (*array_sort_fn)(int *, size_t);

typedef struct {
    const char *name;
    array_sort_fn fn;
} ArraySort;

static void check_one_array_sort(const ArraySort *s, const char *case_name,
                                 const int *input, size_t n)
{
    int got[MAXELEM];
    int want[MAXELEM];
    if (n > MAXELEM) {
        FAILF("用例过大");
        return;
    }
    if (n > 0) {
        memcpy(got, input, n * sizeof *input);
        memcpy(want, input, n * sizeof *input);
    }
    qsort(want, n, sizeof *want, cmp_int); /* 独立参照实现 */
    s->fn(got, n);

    if (!sort_array_is_sorted(got, n)) {
        FAILF("%s / %s: 结果不是升序", s->name, case_name);
    } else {
        g_pass++;
    }
    check_array(got, want, n, case_name);
}

static void test_array_sorts(void)
{
    puts("-- 数组排序：冒泡/插入/快速（已排序/逆序/重复/边界）");

    static const ArraySort sorts[] = {
        {"冒泡排序", sort_bubble},
        {"插入排序", sort_insertion},
        {"快速排序", sort_quick},
    };
    const size_t nsorts = sizeof sorts / sizeof sorts[0];

    static const int a_mixed[] = {5, 2, 9, 1, 5, 6, -3, 0, 5};
    static const int a_sorted[] = {-5, -1, 0, 3, 3, 8, 12, 100};
    static const int a_reverse[] = {100, 12, 8, 3, 3, 0, -1, -5};
    static const int a_dup[] = {7, 7, 7, 7, 7, 7, 7, 7};
    static const int a_two[] = {2, 1};
    static const int a_one[] = {42};

    for (size_t k = 0; k < nsorts; k++) {
        check_one_array_sort(&sorts[k], "随机混合", a_mixed, 9);
        check_one_array_sort(&sorts[k], "已排序输入", a_sorted, 8);
        check_one_array_sort(&sorts[k], "逆序输入", a_reverse, 8);
        check_one_array_sort(&sorts[k], "全部重复", a_dup, 8);
        check_one_array_sort(&sorts[k], "两元素", a_two, 2);
        check_one_array_sort(&sorts[k], "单元素", a_one, 1);
        check_one_array_sort(&sorts[k], "空数组", NULL, 0);
    }

    /* 大数组：随机（大量重复）、已排序、逆序 */
    enum { BIG = 5000 };
    static int big_rand[BIG];
    static int big_sorted[BIG];
    static int big_rev[BIG];
    for (int i = 0; i < BIG; i++) {
        big_rand[i] = lcg_next(50);       /* 0..49，故意制造重复 */
        big_sorted[i] = i;
        big_rev[i] = BIG - 1 - i;
    }
    for (size_t k = 0; k < nsorts; k++) {
        check_one_array_sort(&sorts[k], "随机 5000（大量重复）", big_rand, BIG);
        check_one_array_sort(&sorts[k], "已排序 5000", big_sorted, BIG);
        check_one_array_sort(&sorts[k], "逆序 5000", big_rev, BIG);
    }

    /* 排序函数对 NULL 的容忍 */
    sort_bubble(NULL, 0);
    sort_insertion(NULL, 5);
    sort_quick(NULL, 5);
    sort_bubble(NULL, 1);
    CHECK_TRUE(true);
}

/* ------------------------------------------------------------------ */
/* 12. 链表排序                                                       */
/* ------------------------------------------------------------------ */

typedef void (*list_sort_fn)(List *);

typedef struct {
    const char *name;
    list_sort_fn fn;
} ListSort;

static void check_one_list_sort(const ListSort *s, const char *case_name,
                                const int *input, size_t n)
{
    if (n > MAXELEM) {
        FAILF("用例过大");
        return;
    }
    int want[MAXELEM];
    if (n > 0) {
        memcpy(want, input, n * sizeof *input);
    }
    qsort(want, n, sizeof *want, cmp_int);

    List *l = make_list(input, n);
    s->fn(l);

    if (!sort_list_is_sorted(l)) {
        FAILF("%s / %s: 结果不是升序", s->name, case_name);
    } else {
        g_pass++;
    }
    CHECK_EQ_INT(n, list_size(l)); /* 排序不得增删结点 */
    check_list(l, want, n, case_name);
    list_destroy(l);
}

static void test_list_sorts(void)
{
    puts("-- 链表排序：链表插入排序/链表归并排序");

    static const ListSort sorts[] = {
        {"链表插入排序", sort_list_insertion},
        {"链表归并排序", sort_list_merge},
    };
    const size_t nsorts = sizeof sorts / sizeof sorts[0];

    static const int a_mixed[] = {5, 2, 9, 1, 5, 6, -3, 0};
    static const int a_sorted[] = {-2, 0, 0, 4, 7, 11, 11};
    static const int a_reverse[] = {9, 8, 7, 6, 5, 4, 3};
    static const int a_dup[] = {3, 3, 3, 3, 3, 3};

    for (size_t k = 0; k < nsorts; k++) {
        check_one_list_sort(&sorts[k], "随机混合", a_mixed, 8);
        check_one_list_sort(&sorts[k], "已排序输入", a_sorted, 7);
        check_one_list_sort(&sorts[k], "逆序输入", a_reverse, 7);
        check_one_list_sort(&sorts[k], "全部重复", a_dup, 6);
        check_one_list_sort(&sorts[k], "两元素", (const int[]){2, 1}, 2);
        check_one_list_sort(&sorts[k], "单元素", (const int[]){42}, 1);
        check_one_list_sort(&sorts[k], "空链", NULL, 0);
    }

    /* 大链：随机、已排序、逆序 */
    enum { BIG = 3000 };
    static int big_rand[BIG];
    static int big_sorted[BIG];
    static int big_rev[BIG];
    for (int i = 0; i < BIG; i++) {
        big_rand[i] = lcg_next(1000);
        big_sorted[i] = i;
        big_rev[i] = BIG - 1 - i;
    }
    for (size_t k = 0; k < nsorts; k++) {
        check_one_list_sort(&sorts[k], "随机 3000", big_rand, BIG);
        check_one_list_sort(&sorts[k], "已排序 3000", big_sorted, BIG);
        check_one_list_sort(&sorts[k], "逆序 3000", big_rev, BIG);
    }

    /* 原地排序：排序前后结点个数一致，且哨兵仍在 */
    List *l = make_list((const int[]){3, 1, 2}, 3);
    ListNode *sentinel_before = l->head;
    sort_list_merge(l);
    CHECK_EQ_PTR(sentinel_before, l->head);
    CHECK_EQ_PTR(NULL, list_first(l)->next->next->next); /* 恰好三个结点 */
    list_destroy(l);

    sort_list_insertion(NULL);
    sort_list_merge(NULL);
    CHECK_TRUE(sort_list_is_sorted(NULL));
    CHECK_TRUE(true);
}

/* ------------------------------------------------------------------ */
/* 13. 内存：反复创建/清空/销毁（交给 ASan 检查泄漏）                  */
/* ------------------------------------------------------------------ */

static void test_memory_churn(void)
{
    puts("-- 内存：反复创建/清空/排序/销毁");

    for (int round = 0; round < 100; round++) {
        List *l = list_create();
        CHECK_TRUE(l != NULL);
        for (int j = 0; j < 20; j++) {
            CHECK_TRUE(list_push_back(l, j % 7));
        }
        CHECK_EQ_INT(20, list_size(l));
        list_clear(l);
        CHECK_EQ_INT(0, list_size(l));
        for (int j = 0; j < 20; j++) {
            CHECK_TRUE(list_push_front(l, (20 - j) % 7));
        }
        list_reverse(l);
        sort_list_merge(l);
        list_destroy(l);
    }
}

/* ------------------------------------------------------------------ */

int main(void)
{
    puts("=== cproject 测试：list 模块 + sort 模块 ===");

    test_lifecycle();
    test_build();
    test_insert_at();
    test_delete_at();
    test_find_delete_value();
    test_reverse();
    test_merge_sorted();
    test_fast_slow();
    test_cycle();
    test_intersection();
    test_array_sorts();
    test_list_sorts();
    test_memory_churn();

    printf("\n=== 结果：通过 %d 项，失败 %d 项 ===\n", g_pass, g_fail);
    if (g_fail == 0) {
        puts("全部测试通过。");
        return 0;
    }
    printf("存在 %d 项失败，退出码 1。\n", g_fail);
    return 1;
}
