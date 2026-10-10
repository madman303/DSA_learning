/*
 * list.h —— 单链表模块的对外承诺（public interface）。
 *
 * 本文件只描述“别人能用什么”，不暴露任何实现细节：
 *   - 类型 ListNode / List 的定义；
 *   - 每个函数的参数、返回值、失败情形（见各函数上方注释）。
 * 使用方只 #include "list.h"，实现放在 list.c，改实现不必改本文件。
 *
 * 【头结点（哨兵）约定】
 *   本模块采用“带头结点”的单链表：
 *     List.head 是一个哨兵结点，本身不存有效数据，head->next 才是第一个真实结点。
 *   好处：空表与非空表的插入/删除代码统一，不必再单独判断“是否要改头指针”。
 *   注意：所有“纯算法”函数（中点/倒数第 k/环/交点/裸链释放）的参数 head
 *   指的是“第一个真实结点”，即 list_first(l)（可以为 NULL），与哨兵无关。
 */
#ifndef LIST_H
#define LIST_H

#include <stdbool.h> /* bool / true / false */
#include <stddef.h>  /* size_t / NULL */

/* ------------------------------------------------------------------ */
/* 类型                                                               */
/* ------------------------------------------------------------------ */

/* 单链表结点：数据域 val + 指针域 next（NULL 表示链尾）。 */
typedef struct ListNode {
    int val;
    struct ListNode *next;
} ListNode;

/*
 * 带头结点的单链表句柄。
 *   head : 哨兵结点，永不为 NULL（list_create 成功时）；
 *   size : 有效结点个数，使求长度成为 O(1)，不必每次遍历。
 * 由 List 拥有的结点统一由 list_destroy / list_clear 释放，不要手工 free。
 */
typedef struct {
    ListNode *head; /* 哨兵 */
    size_t size;    /* 有效结点个数 */
} List;

/* ------------------------------------------------------------------ */
/* 结点级操作（不依赖 List，供算法与测试构造裸链使用）                 */
/* ------------------------------------------------------------------ */

/* 分配并初始化一个结点（val=值, next=NULL）；失败返回 NULL。 */
ListNode *node_create(int val);

/* 释放单个结点（不递归释放后继）；node 可为 NULL。 */
void node_free(ListNode *n);

/* 释放一条以 head 开头的无环裸链（含 head）；head 可为 NULL。
 * 若链上有环，必须先把环断开，否则会死循环。 */
void node_free_chain(ListNode *head);

/* 打印一条以 head 开头的裸链，格式 [1 -> 2 -> 3]；head 为 NULL 时打印 []。 */
void node_print_chain(const ListNode *head);

/* ------------------------------------------------------------------ */
/* 生命周期                                                           */
/* ------------------------------------------------------------------ */

/* 创建空链表（含哨兵）；失败返回 NULL。调用方负责 list_destroy。 */
List *list_create(void);

/* 释放整条链：先释放所有真实结点，再释放哨兵与句柄本身；l 可为 NULL。
 * 链上有环时行为未定义（会死循环），请先断环。 */
void list_destroy(List *l);

/* 清空所有真实结点，保留哨兵，size 归零；l 可为 NULL。 */
void list_clear(List *l);

/* ------------------------------------------------------------------ */
/* 查询                                                               */
/* ------------------------------------------------------------------ */

/* 返回有效结点个数；l 为 NULL 时返回 0。O(1)。 */
size_t list_size(const List *l);

/* 空表返回 true；l 为 NULL 视为空表。 */
bool list_empty(const List *l);

/* 返回第一个真实结点（空表返回 NULL）；不转移所有权。 */
ListNode *list_first(const List *l);

/* 把链表中所有值按顺序写入 out（容量 cap）。
 * 成功返回 true；l/out 为 NULL 或 cap < list_size(l) 时返回 false 且不越界写。 */
bool list_to_array(const List *l, int *out, size_t cap);

/* ------------------------------------------------------------------ */
/* 构造                                                               */
/* ------------------------------------------------------------------ */

/* 尾插一个值为 val 的新结点；成功 true，l 为 NULL 或分配失败为 false。O(n)。 */
bool list_push_back(List *l, int val);

/* 头插（插在哨兵之后，成为第一个真实结点）；成功 true，失败 false。O(1)。 */
bool list_push_front(List *l, int val);

/* 用数组 a 的前 n 个元素重建链表（先清空原有内容，按数组顺序尾插）。
 * 成功 true；l 为 NULL、a 为 NULL 且 n > 0、或中途分配失败返回 false。
 * 失败时链表保持调用前的原状（先建临时链，成功后再整体挂上）。 */
bool list_from_array(List *l, const int *a, size_t n);

/* ------------------------------------------------------------------ */
/* 打印                                                               */
/* ------------------------------------------------------------------ */

/* 打印链表，格式 [1 -> 2 -> 3] (size=3)；空表打印 [] (size=0)。l 为 NULL 打印 [NULL]。 */
void list_print(const List *l);

/* ------------------------------------------------------------------ */
/* 按位置插入/删除（位置从 0 开始，范围 [0, size]）                    */
/* ------------------------------------------------------------------ */

/* 在第 pos 个位置前插入 val；pos == size 表示尾插。
 * 成功 true；l 为 NULL、pos > size 或分配失败返回 false。O(n)。 */
bool list_insert_at(List *l, size_t pos, int val);

/* 删除第 pos 个结点（pos 范围 [0, size-1]）。
 * 成功 true；被删值经 out_val 返回（out_val 可为 NULL 表示不关心）。
 * l 为 NULL、空表或 pos >= size 时返回 false。O(n)。 */
bool list_delete_at(List *l, size_t pos, int *out_val);

/* ------------------------------------------------------------------ */
/* 按值查找/删除                                                      */
/* ------------------------------------------------------------------ */

/* 返回第一个值为 val 的结点（NULL 表示没找到）；不转移所有权。O(n)。 */
ListNode *list_find(const List *l, int val);

/* 删除第一个值为 val 的结点；删除成功 true，未找到或 l 为 NULL 返回 false。O(n)。 */
bool list_delete_value(List *l, int val);

/* ------------------------------------------------------------------ */
/* 反转                                                               */
/* ------------------------------------------------------------------ */

/* 就地反转整条链（迭代，三指针），O(n) 时间、O(1) 空间；size 不变。 */
void list_reverse(List *l);

/* 就地反转整条链（递归）。
 * 注意：递归深度为 O(n)，超长链（例如十万级以上）可能爆栈，教学演示用。 */
void list_reverse_recursive(List *l);

/* ------------------------------------------------------------------ */
/* 合并                                                               */
/* ------------------------------------------------------------------ */

/* 把两条升序链表 a、b 归并成一条升序链表写入 out（out 原有内容先被清空）。
 * out 不得与 a、b 相同（别名时返回 false）。
 * 采用“拷贝值新建结点”的策略：a、b 不被修改，三条链各自拥有自己的结点。
 * 成功 true；参数为 NULL、发生别名或分配失败返回 false（失败时 out 为空表）。 */
bool list_merge_sorted(const List *a, const List *b, List *out);

/* ------------------------------------------------------------------ */
/* 快慢指针 / 双指针算法（参数是第一个真实结点，可为 NULL）            */
/* ------------------------------------------------------------------ */

/* 返回中间结点：偶数个结点时返回“中间偏右”的那个（如 4 个结点返回第 3 个）。
 * 空链返回 NULL。要求无环，O(n)。 */
ListNode *list_middle(ListNode *head);

/* 返回倒数第 k 个结点（k 从 1 开始）。
 * k == 0 或 k > 链长时返回 NULL；空链返回 NULL。要求无环，O(n)。 */
ListNode *list_kth_from_end(ListNode *head, size_t k);

/* 判断是否有环（Floyd 判圈，快指针每次走两步）；空链返回 false。O(n)。 */
bool list_has_cycle(const ListNode *head);

/* 返回环的入口结点；无环返回 NULL。空链返回 NULL。O(n)。 */
ListNode *list_cycle_entry(ListNode *head);

/* 返回两条无环链的第一个公共结点（按结点地址比较，即“Y 形”交点）；
 * 不相交、任一为空链时返回 NULL。O(n + m)。 */
ListNode *list_intersection(ListNode *a, ListNode *b);

#endif /* LIST_H */
