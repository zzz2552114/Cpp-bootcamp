#!/usr/bin/env bash
# P4.5 洛谷式测试：对每个 caseNN_*.in 跑 CLI，与 caseNN_*.ans 逐字节 diff
#
# 用法（可在任意目录执行）:
#   ./run_tests.sh                          # 用 solutions/build/ 下的 CLI
#   ./run_tests.sh /path/to/your_cli        # 显式指定 CLI
set -u

HERE="$(cd "$(dirname "$0")" && pwd)"

# 默认 CLI 相对【脚本自身位置】解析（solutions/build/...），与调用者的 CWD 无关
CLI="${1:-$HERE/../../build/p4_5_log_analyzer_cli}"
if [ ! -x "$CLI" ]; then
  echo "找不到 CLI: $CLI"
  echo "请先构建：  cd \"$HERE/../..\" && make all"
  echo "（或 cmake -S . -B build && cmake --build build）"
  exit 127
fi

pass=0
fail=0
for inp in "$HERE"/case*.in; do
  name=$(basename "$inp" .in)
  ans="$HERE/$name.ans"
  got=$(mktemp)
  if ! "$CLI" < "$inp" > "$got" 2>/dev/null; then
    echo "[RE] $name  (运行失败)"
    fail=$((fail + 1))
    rm -f "$got"
    continue
  fi
  if diff -q "$ans" "$got" >/dev/null 2>&1; then
    echo "[AC] $name"
    pass=$((pass + 1))
  else
    echo "[WA] $name"
    echo "  --- 期望(前5行) ---"; head -5 "$ans" | sed 's/^/  /'
    echo "  --- 实际(前5行) ---"; head -5 "$got" | sed 's/^/  /'
    fail=$((fail + 1))
  fi
  rm -f "$got"
done

echo "---------------- AC $pass / $((pass + fail)) ----------------"
[ "$fail" -eq 0 ]
