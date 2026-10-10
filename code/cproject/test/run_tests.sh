#!/usr/bin/env bash
# =====================================================================
# run_tests.sh —— 一键构建并运行 cproject 的全部测试
#
#   依次执行：make clean -> make -> make run -> make asan
#   每一步都完整打印输出；构建输出里出现 warning 会被单独点名。
#   全部成功退出码 0，任何一步失败退出码 1（失败原因和退出码都打印出来）。
#
#   用法： bash test/run_tests.sh      （在任意目录下都可以这样调用）
# =====================================================================
set -u
set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT" || { echo "无法进入项目目录 $ROOT"; exit 2; }

LOG="$(mktemp -t cproject_tests.XXXXXX.log)"
FAILED=0
WARN_COUNT=0

cleanup() { rm -f "$LOG"; }
trap cleanup EXIT

step() {
    echo
    echo "=============================================================="
    echo "== $*"
    echo "=============================================================="
}

# 运行一步：完整回显输出，统计 warning，记录退出码
run_step() {
    local title="$1"
    shift
    step "$title"
    local st=0
    "$@" >"$LOG" 2>&1 || st=$?
    cat "$LOG"

    local w
    w="$(grep -c 'warning:' "$LOG" || true)"
    if [ "${w:-0}" -gt 0 ]; then
        echo ">>> [注意] 上述输出中出现 $w 条 warning（-Wall -Wextra 下应为 0）"
        WARN_COUNT=$((WARN_COUNT + w))
    fi

    if [ "$st" -eq 0 ]; then
        echo ">>> [通过] $title"
    else
        echo ">>> [失败] $title（退出码 $st）"
        FAILED=1
    fi
}

echo "项目根目录: $ROOT"
echo "编译器: $(gcc --version | head -n 1)"
echo "工作目录: $(pwd)"

# 1) 清理
run_step "make clean" make clean

# 2) 构建动态库 + 测试程序
run_step "make（构建 liblist.so 与 test/run_test）" make

# 3) 检查产物确实生成
step "检查构建产物"
for f in liblist.so test/run_test; do
    if [ -f "$f" ]; then
        echo "[通过] 产物存在: $f"
    else
        echo "[失败] 缺少产物: $f"
        FAILED=1
    fi
done

# 4) 共享库运行时查找路径（rpath）——直接运行，不设置 LD_LIBRARY_PATH
step "检查共享库查找路径（rpath = \$ORIGIN/..）"
if readelf -d test/run_test 2>/dev/null | grep -E 'RUNPATH|RPATH'; then
    :
else
    echo "[提示] 未读到 RUNPATH/RPATH 信息（readelf 不可用或未设置）"
fi

# 5) 运行测试
run_step "make run（运行 test/run_test）" make run

# 6) AddressSanitizer + UBSan 再跑一遍（内存泄漏、越界、未定义行为）
run_step "make asan（-fsanitize=address,undefined）" make asan

# 7) 汇总
echo
echo "=============================================================="
if [ "$FAILED" -eq 0 ] && [ "$WARN_COUNT" -eq 0 ]; then
    echo "== 汇总：全部步骤通过，0 条编译警告，测试与 ASan/UBSan 均无问题"
    echo "=============================================================="
    exit 0
fi
echo "== 汇总：失败标记=$FAILED，编译警告数=$WARN_COUNT"
echo "=============================================================="
exit 1
