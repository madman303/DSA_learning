/* 04_struct_dynamic.c —— 结构体 + 动态内存：所有权与释放顺序
 * 编译：gcc -std=c11 -Wall -Wextra -O2 04_struct_dynamic.c -o 04_struct_dynamic
 * 检查内存：gcc -std=c11 -Wall -Wextra -g -fsanitize=address,undefined 04_struct_dynamic.c -o 04_asan && ./04_asan
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Student {
    int id;
    char *name;     /* 指向堆上的字符串：结构体"拥有"这块内存 */
    float score;
};

/* 只读打印：用 const 表明"我不会修改你" */
static void print_student(const struct Student *s) {
    printf("学号: %d, 姓名: %s, 分数: %.2f\n", s->id, s->name, s->score);
}

/* 工厂函数：在堆上造一个学生，返回指针；调用者负责 destroy */
static struct Student *student_create(int id, const char *name, float score) {
    struct Student *s = (struct Student *)malloc(sizeof(struct Student));
    if (!s) {
        return NULL;
    }
    s->id = id;
    s->score = score;
    s->name = (char *)malloc(strlen(name) + 1);   /* +1 放结束符 '\0' */
    if (!s->name) {
        free(s);                                  /* 分配失败：先把自己释放掉 */
        return NULL;
    }
    strcpy(s->name, name);
    return s;
}

/* 销毁：必须先释放内部成员，再释放结构体本身 */
static void student_destroy(struct Student *s) {
    if (!s) {
        return;
    }
    free(s->name);      /* 顺序 1：内部成员 */
    s->name = NULL;
    free(s);            /* 顺序 2：结构体本身 */
}

int main(void) {
    struct Student *stu = student_create(202501, "张三", 95.5f);
    if (!stu) {
        fprintf(stderr, "创建学生失败\n");
        return 1;
    }
    print_student(stu);

    /* p->member 是 (*p).member 的语法糖 */
    printf("等价写法：(*stu).id = %d, stu->id = %d\n", (*stu).id, stu->id);
    printf("sizeof(struct Student) = %zu 字节（名字的内容不在其中）\n", sizeof(struct Student));

    student_destroy(stu);
    stu = NULL;         /* 置空，杜绝悬空指针 */

    /* ---- 反面教材（已注释，打开会内存泄漏或崩溃） ----
       free(stu);            // 错误 1：先释放结构体 -> name 那块内存再也找不到，泄漏
       free(stu->name);      // 错误 2：stu 已悬空，读 stu->name 是未定义行为
       print_student(stu);   // 错误 3：释放后继续使用 = use-after-free
       free(stu->name);      // 错误 4：二次释放 double free
    */
    printf("\n正确顺序：free(s->name) 先，free(s) 后，最后把指针置 NULL。\n");
    return 0;
}
