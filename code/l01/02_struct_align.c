/* 02_struct_align.c —— 结构体内存布局、填充（padding）与对齐
 * 编译：gcc -std=c11 -Wall -Wextra -O2 02_struct_align.c -o 02_struct_align
 */
#include <stddef.h>
#include <stdio.h>

struct A { char c; int x; };            /* 1 字节 + 3 填充 + 4 字节 = 8 */
struct B { char c1; char c2; int x; };  /* 2 字节 + 2 填充 + 4 字节 = 8 */
struct C { int x; char c; };            /* 4 字节 + 1 字节 + 3 尾部填充 = 8 */
struct D { char c; double d; int i; };  /* 1 + 7 + 8 + 4 + 4 尾部 = 24 */
struct Node { int value; struct Node *next; };  /* 4 + 4 填充 + 8 = 16 */

int main(void) {
    printf("sizeof(struct A) = %2zu,  offsetof(x) = %zu\n", sizeof(struct A), offsetof(struct A, x));
    printf("sizeof(struct B) = %2zu,  offsetof(x) = %zu\n", sizeof(struct B), offsetof(struct B, x));
    printf("sizeof(struct C) = %2zu,  offsetof(c) = %zu\n", sizeof(struct C), offsetof(struct C, c));
    printf("sizeof(struct D) = %2zu, offsetof(d) = %zu, offsetof(i) = %zu\n",
           sizeof(struct D), offsetof(struct D, d), offsetof(struct D, i));
    printf("sizeof(struct Node) = %zu, offsetof(next) = %zu\n",
           sizeof(struct Node), offsetof(struct Node, next));

    printf("\n结论：\n");
    printf("1) 结构体大小 >= 成员大小之和，差值就是填充字节；\n");
    printf("2) 每个成员相对结构体首地址的偏移必须是其对齐数的整数倍；\n");
    printf("3) 结构体总大小必须是其最大成员对齐数的整数倍（为了数组里每个元素都对齐）。\n");
    printf("4) 把大的成员排在前面、把同类型成员排在一起，常常能省内存。\n");

    return 0;
}
