# =====================================================================
# cproject 顶层 Makefile
#
#   先编译成位置无关目标文件（-fPIC），再链接出动态库 liblist.so；
#   最后进入 test/ 编译只包含公开头文件、链接该动态库的测试程序。
#
#   make        构建 liblist.so 与 test/run_test
#   make run    先构建再运行测试程序
#   make clean  清理全部产物
#   make asan   用 -fsanitize=address,undefined 单独编译并运行一遍测试
# =====================================================================

# 本机没有 clang，默认用 gcc；命令行 make CC=clang 仍可覆盖
CC       := gcc
CFLAGS   ?= -Wall -Wextra -g -std=c11
# 动态库必须用位置无关代码
PICFLAGS := -fPIC
CPPFLAGS += -I.
LDFLAGS  ?=

LIB      := liblist.so
OBJS     := list.o sort.o
TESTDIR  := test
TESTBIN  := $(TESTDIR)/run_test

# ASan + UBSan：内存错误与未定义行为都让程序直接失败（-fno-sanitize-recover=all）
ASANFLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all
ASANBIN   := build/asan/run_test_asan

.PHONY: all lib test run clean asan rebuild

all: lib test

# ---- 1. 动态链接库 ---------------------------------------------------
lib: $(LIB)

$(LIB): $(OBJS)
	$(CC) -shared -o $@ $(OBJS) $(LDFLAGS)

list.o: list.c list.h
	$(CC) $(CFLAGS) $(PICFLAGS) $(CPPFLAGS) -c $< -o $@

sort.o: sort.c sort.h list.h
	$(CC) $(CFLAGS) $(PICFLAGS) $(CPPFLAGS) -c $< -o $@

# ---- 2. 测试程序（必须在库之后） -------------------------------------
test: $(LIB)
	$(MAKE) -C $(TESTDIR)

# ---- 3. 运行测试 -----------------------------------------------------
run: test
	@echo "== 运行 $(TESTBIN) =="
	@$(TESTBIN)

# ---- 4. 清理 ---------------------------------------------------------
clean:
	rm -f $(OBJS) $(LIB)
	$(MAKE) -C $(TESTDIR) clean
	rm -rf build

# ---- 5. 内存/未定义行为自检（把源码与测试一起编译，便于 ASan 定位） ----
asan:
	@mkdir -p $(dir $(ASANBIN))
	$(CC) $(CFLAGS) $(CPPFLAGS) $(ASANFLAGS) list.c sort.c test/test_list.c -o $(ASANBIN)
	@echo "== 运行 AddressSanitizer/UBSan 版本：$(ASANBIN) =="
	$(ASANBIN)

rebuild: clean all
