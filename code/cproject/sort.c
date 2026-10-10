/*
 * sort.c —— 排序模块的内部实现。
 *
 * 只包含公开头文件 list.h / sort.h：需要建链、遍历、写回时一律调用 list.h
 * 承诺的函数，绝不 #include "list.c"，也不碰 List 的内部表示。
 */
#include "sort.h"

#include <stddef.h>

/* ------------------------------------------------------------------ */
/* 内部细节（static）                                                 */
/* ------------------------------------------------------------------ */

static void swap_int(int *x, int *y)
{
    int t = *x;
    *x = *y;
    *y = t;
}

/* Hoare 划分：三数取中把中位数换到 a[lo] 当基准，返回划分点 p，
 * 使 [lo, p] 的值 <= [p+1, hi] 的值。基准放在端点可当“哨兵”，
 * 两个扫描指针不会越界。 */
static ptrdiff_t partition_hoare(int *a, ptrdiff_t lo, ptrdiff_t hi)
{
    ptrdiff_t mid = lo + (hi - lo) / 2;
    /* 把 lo / mid / hi 三个位置排成 a[lo] <= a[mid] <= a[hi] */
    if (a[mid] < a[lo]) {
        swap_int(&a[mid], &a[lo]);
    }
    if (a[hi] < a[lo]) {
        swap_int(&a[hi], &a[lo]);
    }
    if (a[hi] < a[mid]) {
        swap_int(&a[hi], &a[mid]);
    }
    /* 再把中位数 a[mid] 换到 a[lo] 作为基准 */
    swap_int(&a[mid], &a[lo]);
    int pivot = a[lo];

    ptrdiff_t i = lo - 1;
    ptrdiff_t j = hi + 1;
    for (;;) {
        do {
            i++;
        } while (a[i] < pivot);
        do {
            j--;
        } while (a[j] > pivot);
        if (i >= j) {
            return j;
        }
        swap_int(&a[i], &a[j]);
    }
}

/* 递归快排：只对较短一侧递归，较长一侧用循环处理，栈深 O(log n)。 */
static void quick_sort_range(int *a, ptrdiff_t lo, ptrdiff_t hi)
{
    while (lo < hi) {
        ptrdiff_t p = partition_hoare(a, lo, hi);
        if (p - lo < hi - p) {
            quick_sort_range(a, lo, p);
            lo = p + 1;
        } else {
            quick_sort_range(a, p + 1, hi);
            hi = p;
        }
    }
}

/* 把裸链切成两半，返回后半部分的头；前半部分以 next=NULL 结尾。 */
static ListNode *split_half(ListNode *head)
{
    ListNode *slow = head;
    ListNode *fast = head->next; /* 让前半在偶数长度时略短，保证正确切分 */
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
    }
    ListNode *second = slow->next;
    slow->next = NULL;
    return second;
}

/* 归并两条升序裸链，返回新头（只改指针，不新建结点）。 */
static ListNode *merge_nodes(ListNode *a, ListNode *b)
{
    ListNode dummy; /* 栈上的哨兵，省去“结果链为空”的特判 */
    dummy.val = 0;
    dummy.next = NULL;
    ListNode *tail = &dummy;
    while (a != NULL && b != NULL) {
        if (a->val <= b->val) { /* <= 保证稳定 */
            tail->next = a;
            a = a->next;
        } else {
            tail->next = b;
            b = b->next;
        }
        tail = tail->next;
    }
    tail->next = (a != NULL) ? a : b; /* 接上剩余部分 */
    return dummy.next;
}

/* 归并排序裸链。 */
static ListNode *merge_sort_nodes(ListNode *head)
{
    if (head == NULL || head->next == NULL) {
        return head; /* 0 或 1 个结点天然有序 */
    }
    ListNode *second = split_half(head);
    return merge_nodes(merge_sort_nodes(head), merge_sort_nodes(second));
}

/* ------------------------------------------------------------------ */
/* 数组排序                                                           */
/* ------------------------------------------------------------------ */

void sort_bubble(int *a, size_t n)
{
    if (a == NULL || n < 2) {
        return;
    }
    for (size_t end = n; end > 1; end--) {
        bool swapped = false;
        for (size_t i = 0; i + 1 < end; i++) {
            if (a[i] > a[i + 1]) {
                swap_int(&a[i], &a[i + 1]);
                swapped = true;
            }
        }
        if (!swapped) {
            return; /* 本趟无交换：已经有序 */
        }
    }
}

void sort_insertion(int *a, size_t n)
{
    if (a == NULL || n < 2) {
        return;
    }
    for (size_t i = 1; i < n; i++) {
        int key = a[i];
        size_t j = i;
        while (j > 0 && a[j - 1] > key) { /* 比 key 大的元素整体后移一格 */
            a[j] = a[j - 1];
            j--;
        }
        a[j] = key;
    }
}

void sort_quick(int *a, size_t n)
{
    if (a == NULL || n < 2) {
        return;
    }
    quick_sort_range(a, 0, (ptrdiff_t)n - 1);
}

/* ------------------------------------------------------------------ */
/* 链表排序                                                           */
/* ------------------------------------------------------------------ */

void sort_list_insertion(List *l)
{
    if (l == NULL) {
        return;
    }
    ListNode *sorted = NULL;          /* 已排好序的裸链（借用原结点） */
    ListNode *cur = l->head->next;    /* 待处理部分 */
    while (cur != NULL) {
        ListNode *next = cur->next;   /* 先摘下 cur */
        if (sorted == NULL || sorted->val > cur->val) {
            cur->next = sorted;       /* 插到最前面 */
            sorted = cur;
        } else {
            ListNode *p = sorted;
            while (p->next != NULL && p->next->val <= cur->val) {
                p = p->next;          /* 找到第一个比 cur 大的位置 */
            }
            cur->next = p->next;
            p->next = cur;
        }
        cur = next;
    }
    l->head->next = sorted; /* 挂回哨兵之后，size 不变 */
}

void sort_list_merge(List *l)
{
    if (l == NULL) {
        return;
    }
    l->head->next = merge_sort_nodes(l->head->next);
}

/* ------------------------------------------------------------------ */
/* 检查                                                               */
/* ------------------------------------------------------------------ */

bool sort_array_is_sorted(const int *a, size_t n)
{
    if (a == NULL || n < 2) {
        return true;
    }
    for (size_t i = 1; i < n; i++) {
        if (a[i - 1] > a[i]) {
            return false;
        }
    }
    return true;
}

bool sort_list_is_sorted(const List *l)
{
    if (l == NULL || l->size < 2) {
        return true;
    }
    for (const ListNode *p = l->head->next; p != NULL && p->next != NULL; p = p->next) {
        if (p->val > p->next->val) {
            return false;
        }
    }
    return true;
}
