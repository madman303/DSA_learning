# =====================================================================
# test/Makefile —— 测试目录自己的构建规则
#
# 测试程序是动态库的使用者：只 include 公开头文件（-I 指到顶层），
# 链接顶层的 liblist.so；并用 -Wl,-rpath,'$ORIGIN/..' 把“运行时到哪里
# 找 liblist.so”写进可执行文件，这样直接 ./run_test 也能跑，不必设置
# LD_LIBRARY_PATH。
# =====================================================================

# 本机没有 clang，默认用 gcc；命令行 make CC=clang 仍可覆盖
CC       := gcc
CFLAGS   ?= -Wall -Wextra -g -std=c11
ROOT     := $(abspath $(CURDIR)/..)
CPPFLAGS += -I$(ROOT)
LDFLAGS  := -L$(ROOT) -llist -Wl,-rpath,'$$ORIGIN/..'

TARGET   := run_test
SRC      := test_list.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC) $(ROOT)/liblist.so
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SRC) -o $@ $(LDFLAGS)

# 直接在本目录 make 而顶层库还没构建时，给出明确提示
$(ROOT)/liblist.so:
	@echo "错误：找不到 $(ROOT)/liblist.so，请先在顶层执行 make" >&2
	@exit 1

clean:
	rm -f $(TARGET) $(TARGET)_asan
