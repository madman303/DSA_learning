# L01 配套代码：指针、动态内存、结构体、计时
#
#   make          用 -O2 构建全部程序
#   make run      依次运行全部程序（含对比实验）
#   make asan     用 AddressSanitizer 重新构建并检查内存问题
#   make O0       构建 06 的未优化版本，用于对比编译器优化
#   make clean    清理产物
#
# 工具链：本机使用 gcc（课件示例里提到 clang/lldb，命令一一对应：
#         gcc <-> clang, gdb <-> lldb）

CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -O2
SRCS    := 01_pointer_basics.c 02_struct_align.c 03_dynamic_array.c \
           04_struct_dynamic.c 05_timing_fib.c 06_optimization_trap.c
BINS    := $(SRCS:.c=)

.PHONY: all run asan O0 clean

all: $(BINS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

run: all
	@echo "=================== 01 指针基础 ==================="
	@./01_pointer_basics
	@echo
	@echo "=================== 02 结构体对齐 ================="
	@./02_struct_align
	@echo
	@echo "=================== 03 动态数组 ==================="
	@./03_dynamic_array
	@echo
	@echo "=================== 04 结构体+动态内存 ============"
	@./04_struct_dynamic
	@echo
	@echo "=================== 05 Fibonacci 计时 ============="
	@./05_timing_fib
	@echo
	@echo "=================== 06 优化陷阱 (-O2) ============="
	@./06_optimization_trap
	@echo
	@echo "=================== 07 Python 计时 ================"
	@python3 07_python_timing.py

asan:
	$(CC) -std=c11 -Wall -Wextra -g -fsanitize=address,undefined 04_struct_dynamic.c -o 04_asan
	./04_asan
	$(CC) -std=c11 -Wall -Wextra -g -fsanitize=address,undefined 03_dynamic_array.c -o 03_asan
	./03_asan > /dev/null && echo "03 通过 AddressSanitizer 检查（无泄漏、无越界）"

O0:
	$(CC) -std=c11 -O0 06_optimization_trap.c -o 06_O0
	$(CC) -std=c11 -O2 06_optimization_trap.c -o 06_O2
	@echo "--- -O0 ---"; ./06_O0
	@echo "--- -O2 ---"; ./06_O2

clean:
	rm -f $(BINS) 03_asan 04_asan 06_O0 06_O2
