/* 01_pointer_basics.c —— 指针的基本事实：地址、解引用、指针算术、多级指针、const
 * 编译：gcc -std=c11 -Wall -Wextra -O2 01_pointer_basics.c -o 01_pointer_basics
 * 运行：./01_pointer_basics
 * 说明：本程序打印的都是"真实观测值"，地址每次运行都不同，但差值恒定。
 */
#include <stddef.h>
#include <stdio.h>

int main(void) {
    /* ---- 1. 指针的双重性：它既是一个值（地址），又是对另一对象的引用 ---- */
    int x = 10;
    int *p = &x;                      /* &x：取 x 的地址，类型是 int * */

    printf("x = %d\n", x);
    printf("&x = %p   (x 的地址)\n", (void *)&x);
    printf("p  = %p   (p 里存的值，就是 x 的地址)\n", (void *)p);
    printf("*p = %d   (通过 p 间接访问 x)\n", *p);

    /* ---- 2. *p 作左值 / 作右值 ---- */
    *p = 20;                          /* *p 作左值：写入 x 所在的内存位置 */
    printf("执行 *p = 20 后，x = %d\n", x);

    int y = *p + 1;                   /* *p 作右值：读出 x 的值参与运算 */
    printf("int y = *p + 1;  y = %d\n", y);

    /* ---- 3. 指针自身的大小 vs 所指类型的大小 ---- */
    printf("sizeof(int)   = %zu 字节\n", sizeof(int));
    printf("sizeof(char)  = %zu 字节\n", sizeof(char));
    printf("sizeof(double)= %zu 字节\n", sizeof(double));
    printf("sizeof(int *) = %zu 字节（64 位平台上与所指类型无关）\n", sizeof(int *));
    printf("sizeof(p)     = %zu 字节\n", sizeof p);

    /* ---- 4. 指针算术的步长 = sizeof(所指类型) ---- */
    int arr[5] = {10, 20, 30, 40, 50};
    int *q = arr;                     /* 数组名退化为指向首元素的指针 */
    printf("\narr       = %p\n", (void *)arr);
    printf("arr + 1   = %p\n", (void *)(arr + 1));
    printf("字节差    = %td  (等于 sizeof(int))\n",
           (char *)(arr + 1) - (char *)arr);

    for (int i = 0; i < 5; i++) {
        printf("*(q + %d) = %-3d   q[%d] = %-3d   等价\n", i, *(q + i), i, q[i]);
    }

    /* ---- 5. 两个指针相减 = 元素个数，类型 ptrdiff_t ---- */
    int *end = arr + 5;               /* "末端后一位"：合法，但不可解引用 */
    ptrdiff_t len = end - arr;
    printf("end - arr = %td 个元素\n", len);

    /* ---- 6. 多级指针 ---- */
    int **pp = &p;                    /* pp 指向 p，p 指向 x */
    **pp = 30;                        /* 两次解引用回到 x */
    printf("\n通过 int **pp = &p; **pp = 30; 修改后  x = %d\n", x);

    /* ---- 7. const 与指针的三种组合（读法：从右往左） ---- */
    const int *pc = &x;               /* 所指对象只读，指针本身可改指向 */
    int *const cp = &x;               /* 指针本身固定，所指对象可改 */
    const int *const cpc = &x;        /* 两者都不可改 */
    printf("*pc = %d, *cp = %d, *cpc = %d\n", *pc, *cp, *cpc);
    /* *pc = 1;   // 编译错误：assignment of read-only location
       cp = &y;   // 编译错误：assignment of read-only variable
       cpc = &y;  // 编译错误
    */
    pc = &y;                          /* 合法：改的是指针的指向 */
    printf("pc 改指向 y 后 *pc = %d\n", *pc);

    return 0;
}
