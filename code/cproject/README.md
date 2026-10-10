# cproject —— C 项目组织与函数库的最小示例

两周的链表代码放进一个真正的 C 项目，长成下面这个样子。目录里每一件东西都有它的用处。

```text
cproject/
  Makefile        顶层构建：先出动态链接库，再出调用它的测试程序
  README.md       结构说明与运行方法
  list.h          链表模块的声明（对外接口）
  list.c          链表模块的实现
  sort.h          排序模块的声明（对外接口）
  sort.c          排序模块的实现
  test/
    Makefile      测试目录自己的构建规则
    run_tests.sh  测试脚本
    test_list.c   测试程序：只包含公开头文件，链接共享库
```

## 构建与运行

```sh
make            # 构建 liblist.so 与 test/run_test
make run        # 运行测试程序
make clean      # 清理产物
test/run_tests.sh   # 一键构建并运行测试
```

`make` 的顺序是刻意的：先把 `list.c`、`sort.c` 编译成位置无关目标文件（`-fPIC`），
链接出动态库 `liblist.so`；再进入 `test/` 编译只包含公开头文件、链接该动态库的测试程序。

其他可用目标：

```sh
make asan       # 用 -fsanitize=address,undefined 单独编译并运行一遍测试
make rebuild    # clean + all
make -C test    # 只构建测试程序（要求顶层 liblist.so 已存在）
```

### 共享库的运行时查找路径（rpath）

动态库方式的经典坑：链接成功但一运行就报
`error while loading shared libraries: liblist.so: cannot open shared object file`。
原因是链接期能找到库（`-L.`），运行期却没人告诉它去哪儿找。

本项目用 **rpath** 解决，写死在可执行文件里，不需要设置 `LD_LIBRARY_PATH`：

```sh
# test/Makefile 里：
gcc -Wall -Wextra -g -std=c11 -I.. test_list.c -o run_test \
    -L.. -llist -Wl,-rpath,'$ORIGIN/..'
```

`$ORIGIN` 表示“可执行文件自身所在目录”，`$ORIGIN/..` 即顶层的 `cproject/`，
正是 `liblist.so` 所在处。注意在 Makefile 里 `$` 要写成 `$$`，并且整段
`'$ORIGIN/..'` 要用单引号保护，否则会被 shell 当成变量展开成空串。

验证方式（在项目根目录）：

```sh
readelf -d test/run_test | grep -E 'RUNPATH|RPATH'   # 应看到 $ORIGIN/..
ldd test/run_test | grep liblist                      # 应解析到 ./liblist.so
./test/run_test                                       # 直接跑，无需 LD_LIBRARY_PATH
```

若不用 rpath，等价做法是运行前导出路径：

```sh
LD_LIBRARY_PATH=. ./test/run_test
```

## 每个文件做什么

| 文件 | 作用 |
| --- | --- |
| `Makefile` | 顶层构建：编 `list.o`/`sort.o` → 链接 `liblist.so` → 进入 `test/` 编测试程序；另含 `run`、`clean`、`asan` 目标 |
| `README.md` | 本文件：结构、运行方法、接口说明与易错点 |
| `list.h` | 链表模块的**对外承诺**：`ListNode`/`List` 类型与全部函数声明、参数/返回值/失败情形注释 |
| `list.c` | 链表模块的**内部实现**：哨兵链表的增删查改、反转、归并、快慢指针、环检测、交点、释放；内部细节用 `static` 隐藏 |
| `sort.h` | 排序模块的对外承诺：3 个数组排序 + 2 个链表排序 + 2 个自检函数 |
| `sort.c` | 排序模块的实现：冒泡/插入/快速（三数取中 + Hoare 划分）、链表插入排序、链表归并排序；只 `#include "list.h"` 用公开接口，绝不包含 `list.c` |
| `test/Makefile` | 测试目录自己的构建规则：`-I` 指向顶层，`-L` 链接顶层动态库，写入 rpath |
| `test/run_tests.sh` | 一键脚本：`make clean → make → 检查产物/rpath → make run → make asan`，逐步回显并汇总，统计 warning |
| `test/test_list.c` | 测试程序：只包含 `list.h`/`sort.h`，断言风格，覆盖边界与退化输入，失败打印期望值与实际值并返回非零 |

## 为什么头文件与源文件成对

一个模块由两个文件组成：头文件写对外承诺（类型定义与函数声明），源文件写内部实现。别的模块只包含头文件，看不到实现细节；改内部实现时，只要承诺不变，其它模块不必重编。`sort.c` 要建链与打印，它包含的是 `list.h`，不是 `list.c`。

`list.c` 里 `chain_length`、`reverse_rec` 这类辅助函数都带 `static`：它们不是承诺，
只是实现手法，改名字、改写法都不影响任何使用方。`sort.c` 里的 `partition_hoare`、
`split_half`、`merge_nodes` 同理。

## 为什么测试单独放

测试程序是库的使用者，它与库之间只通过公开头文件发生关系。单独放在 `test/` 里，一条命令就能把“这个库现在是对的”这件事复核一遍，也让提交记录里能看清哪一次改动动了实现、哪一次改动动了测试。

## 数据表示：带头结点（哨兵）的单链表

```c
typedef struct ListNode { int val; struct ListNode *next; } ListNode;
typedef struct { ListNode *head; size_t size; } List;
```

`List.head` 是**哨兵结点**：它自己不放有效数据，`head->next` 才是第一个真实结点。

- 好处：插入/删除的代码统一。没有哨兵时，“在表头插入”要改头指针，“在别处插入”
  不用改，两套逻辑；有哨兵后一律是“改前驱的 `next`”，空表也不用特判。
- `size` 字段让求长度变成 O(1)，代价是每次增删都要同步维护它。
- 释放整条链：`list_destroy` 会连同哨兵一起释放；`list_clear` 只释放真实结点、保留哨兵。
- **纯算法函数**（`list_middle`、`list_kth_from_end`、`list_has_cycle`、
  `list_cycle_entry`、`list_intersection`、`node_free_chain` 等）的参数 `head` 指的是
  **第一个真实结点**，也就是 `list_first(l)`，与哨兵无关；它们既可以作用在库建的链上，
  也可以作用在 `node_create` 手工串的裸链上。

## 接口清单（`list.h`）

| 分类 | 函数 |
| --- | --- |
| 结点 | `node_create` `node_free` `node_free_chain` `node_print_chain` |
| 生命周期 | `list_create` `list_destroy` `list_clear` |
| 查询 | `list_size` `list_empty` `list_first` `list_to_array` |
| 构造 | `list_push_back` `list_push_front` `list_from_array` |
| 打印 | `list_print` |
| 按位置 | `list_insert_at` `list_delete_at` |
| 按值 | `list_find` `list_delete_value` |
| 反转 | `list_reverse`（迭代） `list_reverse_recursive`（递归） |
| 合并 | `list_merge_sorted`（拷贝值新建结点，输入不被改动） |
| 快慢指针/双指针 | `list_middle` `list_kth_from_end` `list_has_cycle` `list_cycle_entry` `list_intersection` |

约定与失败情形：

- 位置从 0 开始，合法范围 `[0, size]`（插入）与 `[0, size-1]`（删除）；越界返回 `false`，不改动链表。
- `k` 从 1 开始；`k == 0` 或 `k > 链长` 时 `list_kth_from_end` 返回 `NULL`。
- 绝大多数函数对 `NULL` 句柄是安全的（返回 `false`/`NULL`/0），但 `list_destroy` 遇到有环的链会死循环。
- `list_merge_sorted(a, b, out)` 拒绝 `out` 与 `a`/`b` 别名（返回 `false`），且 **不共享结点**：
  它按值新建结点，所以 `a`、`b` 保持原样，三条链各自负责自己的内存。
- `list_reverse_recursive` 递归深度 O(n)，`sort_quick` 只递归较短一侧（栈深 O(log n)）；前者别用在超长链上。

## 接口清单（`sort.h`）

| 分类 | 函数 | 复杂度 / 稳定性 |
| --- | --- | --- |
| 数组 | `sort_bubble` | O(n²) / O(1) / 稳定 |
| 数组 | `sort_insertion` | O(n²)，近乎有序接近 O(n) / O(1) / 稳定 |
| 数组 | `sort_quick` | 平均 O(n log n)，三数取中 + Hoare 划分 / O(log n) 栈 / 不稳定 |
| 链表 | `sort_list_insertion` | O(n²) / O(1) 额外空间 / 稳定，原地改指针不建新结点 |
| 链表 | `sort_list_merge` | O(n log n) / O(log n) 递归栈 / 稳定，不需要额外数组 |
| 自检 | `sort_array_is_sorted` `sort_list_is_sorted` | O(n) |

`sort_quick` 用三数取中选基准，且 Hoare 划分在“全部元素相等”时仍能对半划分，
因此对**已排序、逆序、大量重复**三类容易退化的输入都能保持 O(n log n)。

## 测试覆盖

`test/run_tests.sh` 会依次跑：

1. `make clean`、`make`（构建动态库与测试程序），并统计编译 warning（应为 0）；
2. 检查 `liblist.so`、`test/run_test` 是否生成，并打印 `RUNPATH`（rpath）；
3. `make run`：运行 `test/run_test`；
4. `make asan`：用 `gcc -fsanitize=address,undefined` 把 `list.c`、`sort.c`、
   `test/test_list.c` 一起重新编译再跑一遍，检查内存泄漏、越界与未定义行为。

手动复现 ASan 那一遍：

```sh
gcc -Wall -Wextra -g -std=c11 -I. -fsanitize=address,undefined \
    -fno-omit-frame-pointer -fno-sanitize-recover=all \
    list.c sort.c test/test_list.c -o /tmp/run_test_asan
/tmp/run_test_asan
```

`test_list.c` 用断言风格，失败时打印期望值与实际值（例如
`第 3 个元素期望 5，实际 7，实际链为 [1 -> 2 -> 7]`），全部跑完再汇总，
有失败则返回退出码 1。覆盖的边界包括：

- 空链、单结点、两结点；`NULL` 句柄的各种调用；
- `list_insert_at` 头/中/尾/越界；`list_delete_at` 删头/删尾/删中间/越界/空表；
- `list_find` / `list_delete_value` 命中头、中间、尾、不存在；
- 迭代与递归反转，以及“反转后再反转”必须回到原序（两者结果互相比对，长链 2000 个）；
- 有序合并（含相等元素、空链、输入不被改动、别名被拒）；
- 中点（偶数长度偏右）、倒数第 k 个（`k` 超出长度、`k == 0`、`k` 为极大值）；
- 有环/无环、尾巴接中间、自环、整链成环（入口为头），加环后再断环销毁；
- 两条链的交点：Y 形（长度不同）、交点为尾、不相交、空链、同一条链；
- 数组三种排序：随机、已排序、逆序、全部重复、两元素、单元素、空数组，以及 5000 元素的
  随机（大量重复）/已排序/逆序；用 `qsort` 作独立参照比对结果，并校验确实升序；
- 链表两种排序：同样的退化输入 + 3000 元素规模，并校验排序前后结点个数不变、哨兵不变；
- 反复创建/清空/排序/销毁 100 轮，交给 ASan 检查无泄漏。

## 工具链

编译用 clang，调试用 lldb：

```sh
clang -Wall -Wextra -g -fPIC -I. -c list.c -o list.o
lldb ./test/run_test
```

本机没有 clang/lldb，用 **gcc + gdb** 完全等价（本项目 Makefile 默认就是 gcc）：

```sh
gcc -Wall -Wextra -g -fPIC -I. -c list.c -o list.o     # 编译（clang 的等价命令）
gcc -Wall -Wextra -g -std=c11 -I. list.c sort.c test/test_list.c -o /tmp/all_in_one  # 不用库、一把编
gdb ./test/run_test                                    # 调试（lldb 的等价命令）
gdb --args ./test/run_test                             # 需要参数时
```

`gcc` 与 `clang` 的常用开关基本同名：`-Wall -Wextra -g -fPIC -I. -c`、`-shared`、
`-Wl,-rpath,...`、`-fsanitize=address,undefined` 都是两边通用的。

## 学生最该注意的三点

1. **哨兵不是数据。** `l->head` 是哨兵，`l->head->next` 才是第一个元素；
   遍历、打印、求长度都要从 `head->next` 开始，否则会打印出一个莫名其妙的 0。
   同时记住 `size` 必须与链的实际长度同步，`push/insert/delete/clear` 里改没改全。
2. **谁分配谁释放，并且“先存后继再 free”。** 删除结点时要先 `victim->next` 存好，
   再 `free(victim)`，否则就是 use-after-free；`list_destroy` 释放的是整条链（含哨兵），
   而 `node_free_chain` 只管裸链，两者不要混用；有环的链必须先断环再释放。
3. **头文件是对外承诺，源文件是内部实现。** `test_list.c` 只 `#include "list.h"`/`"sort.h"`，
   链接 `liblist.so`；`sort.c` 需要建链打印时也只包含 `list.h`，绝不 `#include "list.c"`。
   一旦养成“实现细节不外泄”的习惯，模块才可能被单独替换和测试；这也是 `make asan`
   能独立重建一遍的原因。
