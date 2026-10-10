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

## 为什么头文件与源文件成对

一个模块由两个文件组成：头文件写对外承诺（类型定义与函数声明），源文件写内部实现。别的模块只包含头文件，看不到实现细节；改内部实现时，只要承诺不变，其它模块不必重编。`sort.c` 要建链与打印，它包含的是 `list.h`，不是 `list.c`。

## 为什么测试单独放

测试程序是库的使用者，它与库之间只通过公开头文件发生关系。单独放在 `test/` 里，一条命令就能把“这个库现在是对的”这件事复核一遍，也让提交记录里能看清哪一次改动动了实现、哪一次改动动了测试。

## 工具链

编译用 clang，调试用 lldb：

```sh
clang -Wall -Wextra -g -fPIC -I. -c list.c -o list.o
lldb ./test/run_test
```