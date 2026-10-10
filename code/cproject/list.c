/*
 * list.c —— 单链表模块的内部实现。
 *
 * 对外只通过 list.h 承诺；本文件里的 static 函数都是内部细节，
 * 其它模块看不到，改掉它们不影响任何使用方。
 */
#include "list.h"

#include <stdio.h>
#include <stdlib.h>

/* ------------------------------------------------------------------ */
/* 内部细节（static）：只在本文件可见                                   */
/* ------------------------------------------------------------------ */

/* 返回裸链长度（用于求交点时对齐起点）。 */
static size_t chain_length(const ListNode *head)
{
    size_t n = 0;
    for (const ListNode *p = head; p != NULL; p = p->next) {
        n++;
    }
    return n;
}

/* 递归反转一条裸链，返回反转后的新头。 */
static ListNode *reverse_rec(ListNode *node)
{
    if (node == NULL || node->next == NULL) {
        return node; /* 空链或单结点：反转后就是自己 */
    }
    ListNode *new_head = reverse_rec(node->next);
    node->next->next = node; /* 让后继回头指向自己 */
    node->next = NULL;       /* 自己暂时作为新链尾 */
    return new_head;
}

/* ------------------------------------------------------------------ */
/* 结点级操作                                                         */
/* ------------------------------------------------------------------ */

ListNode *node_create(int val)
{
    ListNode *n = (ListNode *)malloc(sizeof *n);
    if (n == NULL) {
        return NULL; /* 分配失败：调用方负责处理 */
    }
    n->val = val;
    n->next = NULL;
    return n;
}

void node_free(ListNode *n)
{
    free(n);
}

void node_free_chain(ListNode *head)
{
    while (head != NULL) {
        ListNode *next = head->next; /* 先记住后继，再释放当前 */
        free(head);
        head = next;
    }
}

void node_print_chain(const ListNode *head)
{
    putchar('[');
    for (const ListNode *p = head; p != NULL; p = p->next) {
        printf("%d", p->val);
        if (p->next != NULL) {
            printf(" -> ");
        }
    }
    putchar(']');
}

/* ------------------------------------------------------------------ */
/* 生命周期                                                           */
/* ------------------------------------------------------------------ */

List *list_create(void)
{
    List *l = (List *)malloc(sizeof *l);
    if (l == NULL) {
        return NULL;
    }
    l->head = node_create(0); /* 哨兵：值无意义，只占位 */
    if (l->head == NULL) {
        free(l);
        return NULL;
    }
    l->size = 0;
    return l;
}

void list_destroy(List *l)
{
    if (l == NULL) {
        return;
    }
    node_free_chain(l->head); /* 哨兵与所有真实结点一起释放 */
    free(l);
}

void list_clear(List *l)
{
    if (l == NULL) {
        return;
    }
    node_free_chain(l->head->next);
    l->head->next = NULL;
    l->size = 0;
}

/* ------------------------------------------------------------------ */
/* 查询                                                               */
/* ------------------------------------------------------------------ */

size_t list_size(const List *l)
{
    return (l == NULL) ? 0 : l->size;
}

bool list_empty(const List *l)
{
    return list_size(l) == 0;
}

ListNode *list_first(const List *l)
{
    if (l == NULL || l->head == NULL) {
        return NULL;
    }
    return l->head->next;
}

bool list_to_array(const List *l, int *out, size_t cap)
{
    if (l == NULL || out == NULL || cap < l->size) {
        return false;
    }
    size_t i = 0;
    for (const ListNode *p = l->head->next; p != NULL; p = p->next) {
        out[i] = p->val;
        i++;
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* 构造                                                               */
/* ------------------------------------------------------------------ */

bool list_push_back(List *l, int val)
{
    if (l == NULL) {
        return false;
    }
    ListNode *n = node_create(val);
    if (n == NULL) {
        return false;
    }
    ListNode *tail = l->head;
    while (tail->next != NULL) { /* 走到最后一个结点 */
        tail = tail->next;
    }
    tail->next = n;
    l->size++;
    return true;
}

bool list_push_front(List *l, int val)
{
    if (l == NULL) {
        return false;
    }
    ListNode *n = node_create(val);
    if (n == NULL) {
        return false;
    }
    n->next = l->head->next; /* 插在哨兵之后 */
    l->head->next = n;
    l->size++;
    return true;
}

bool list_from_array(List *l, const int *a, size_t n)
{
    if (l == NULL || (a == NULL && n > 0)) {
        return false;
    }
    /* 先建临时链，全部成功后再一次性替换，失败时 l 保持不变。 */
    ListNode *head = NULL;
    ListNode *tail = NULL;
    for (size_t i = 0; i < n; i++) {
        ListNode *nd = node_create(a[i]);
        if (nd == NULL) {
            node_free_chain(head);
            return false;
        }
        if (tail != NULL) {
            tail->next = nd;
        } else {
            head = nd;
        }
        tail = nd;
    }
    node_free_chain(l->head->next);
    l->head->next = head;
    l->size = n;
    return true;
}

/* ------------------------------------------------------------------ */
/* 打印                                                               */
/* ------------------------------------------------------------------ */

void list_print(const List *l)
{
    if (l == NULL) {
        printf("[NULL]\n");
        return;
    }
    putchar('[');
    for (const ListNode *p = l->head->next; p != NULL; p = p->next) {
        printf("%d", p->val);
        if (p->next != NULL) {
            printf(" -> ");
        }
    }
    printf("] (size=%zu)\n", l->size);
}

/* ------------------------------------------------------------------ */
/* 按位置插入/删除                                                    */
/* ------------------------------------------------------------------ */

bool list_insert_at(List *l, size_t pos, int val)
{
    if (l == NULL || pos > l->size) {
        return false;
    }
    ListNode *prev = l->head; /* 从哨兵出发，走 pos 步到前驱 */
    for (size_t i = 0; i < pos; i++) {
        prev = prev->next;
    }
    ListNode *n = node_create(val);
    if (n == NULL) {
        return false;
    }
    n->next = prev->next;
    prev->next = n;
    l->size++;
    return true;
}

bool list_delete_at(List *l, size_t pos, int *out_val)
{
    if (l == NULL || pos >= l->size) {
        return false;
    }
    ListNode *prev = l->head;
    for (size_t i = 0; i < pos; i++) {
        prev = prev->next;
    }
    ListNode *victim = prev->next;
    prev->next = victim->next;
    if (out_val != NULL) {
        *out_val = victim->val;
    }
    free(victim);
    l->size--;
    return true;
}

/* ------------------------------------------------------------------ */
/* 按值查找/删除                                                      */
/* ------------------------------------------------------------------ */

ListNode *list_find(const List *l, int val)
{
    if (l == NULL) {
        return NULL;
    }
    for (ListNode *p = l->head->next; p != NULL; p = p->next) {
        if (p->val == val) {
            return p;
        }
    }
    return NULL;
}

bool list_delete_value(List *l, int val)
{
    if (l == NULL) {
        return false;
    }
    ListNode *prev = l->head;
    while (prev->next != NULL && prev->next->val != val) {
        prev = prev->next;
    }
    if (prev->next == NULL) {
        return false; /* 走到底仍未找到 */
    }
    ListNode *victim = prev->next;
    prev->next = victim->next;
    free(victim);
    l->size--;
    return true;
}

/* ------------------------------------------------------------------ */
/* 反转                                                               */
/* ------------------------------------------------------------------ */

void list_reverse(List *l)
{
    if (l == NULL) {
        return;
    }
    ListNode *prev = NULL;
    ListNode *cur = l->head->next;
    while (cur != NULL) {
        ListNode *next = cur->next; /* 先存后继 */
        cur->next = prev;           /* 掉头 */
        prev = cur;                 /* 前移 */
        cur = next;
    }
    l->head->next = prev; /* 哨兵指向新的第一个真实结点 */
}

void list_reverse_recursive(List *l)
{
    if (l == NULL) {
        return;
    }
    l->head->next = reverse_rec(l->head->next);
}

/* ------------------------------------------------------------------ */
/* 合并                                                               */
/* ------------------------------------------------------------------ */

bool list_merge_sorted(const List *a, const List *b, List *out)
{
    if (a == NULL || b == NULL || out == NULL) {
        return false;
    }
    if (out == a || out == b) {
        return false; /* 不支持输出与输入别名 */
    }
    list_clear(out);

    const ListNode *pa = a->head->next;
    const ListNode *pb = b->head->next;
    ListNode *tail = out->head;
    while (pa != NULL && pb != NULL) {
        const ListNode *pick;
        if (pa->val <= pb->val) { /* <= 保证相等时先取 a，结果稳定 */
            pick = pa;
            pa = pa->next;
        } else {
            pick = pb;
            pb = pb->next;
        }
        ListNode *nd = node_create(pick->val);
        if (nd == NULL) {
            list_clear(out); /* 失败不留下半条链 */
            return false;
        }
        tail->next = nd;
        tail = nd;
        out->size++;
    }
    for (const ListNode *rest = (pa != NULL) ? pa : pb; rest != NULL; rest = rest->next) {
        ListNode *nd = node_create(rest->val);
        if (nd == NULL) {
            list_clear(out);
            return false;
        }
        tail->next = nd;
        tail = nd;
        out->size++;
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* 快慢指针 / 双指针算法                                              */
/* ------------------------------------------------------------------ */

ListNode *list_middle(ListNode *head)
{
    ListNode *slow = head;
    ListNode *fast = head;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;       /* 慢指针每次一步 */
        fast = fast->next->next; /* 快指针每次两步 */
    }
    return slow; /* 偶数个结点时停在中间偏右的那个 */
}

ListNode *list_kth_from_end(ListNode *head, size_t k)
{
    if (k == 0) {
        return NULL;
    }
    ListNode *fast = head;
    for (size_t i = 0; i < k; i++) { /* 快指针先走 k 步 */
        if (fast == NULL) {
            return NULL; /* 链长不足 k */
        }
        fast = fast->next;
    }
    ListNode *slow = head;
    while (fast != NULL) { /* 快慢同步，快指针到头时慢指针即倒数第 k 个 */
        fast = fast->next;
        slow = slow->next;
    }
    return slow;
}

bool list_has_cycle(const ListNode *head)
{
    const ListNode *slow = head;
    const ListNode *fast = head;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) {
            return true; /* 有环必然相遇 */
        }
    }
    return false;
}

ListNode *list_cycle_entry(ListNode *head)
{
    ListNode *slow = head;
    ListNode *fast = head;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) {
            /* 相遇后：一个指针回到起点，两指针同速前进，再次相遇处即入口 */
            ListNode *p = head;
            while (p != slow) {
                p = p->next;
                slow = slow->next;
            }
            return p;
        }
    }
    return NULL; /* 无环 */
}

ListNode *list_intersection(ListNode *a, ListNode *b)
{
    size_t la = chain_length(a);
    size_t lb = chain_length(b);
    ListNode *x = a;
    ListNode *y = b;
    while (la > lb) { /* 长链先走长度差，使剩余长度相同 */
        x = x->next;
        la--;
    }
    while (lb > la) {
        y = y->next;
        lb--;
    }
    while (x != NULL && y != NULL && x != y) { /* 同步走，找第一个公共结点 */
        x = x->next;
        y = y->next;
    }
    return x; /* 走到 NULL 表示不相交，返回 NULL */
}
