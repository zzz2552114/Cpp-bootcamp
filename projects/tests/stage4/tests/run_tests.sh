#!/usr/bin/env bash
# P4.5 洛谷式测试：对每个 caseNN_*.in 跑 CLI，与 caseNN_*.ans 逐字节 diff
#
# 用法（可在任意目录执行）:
#   ./run_tests.sh                          # 用当前目录下的 CLI
#   ./run_tests.sh /path/to/your_cli        # 显式指定 CLI
set -u

HERE="$(cd "$(dirname "$0")" && pwd)"

# 默认 CLI 相对【调用者的当前目录】解析；推荐直接传一个显式路径，例如 ./cli
CLI="${1:-./p4_5_log_analyzer_cli}"
if [ ! -x "$CLI" ]; then
  echo "CLI not found or not executable: $CLI"
  echo "Build it first, then pass its path as the first argument, e.g.:"
  echo "  ./run_tests.sh ./p4_5_log_analyzer_cli"
  exit 127
fi

pass=0
fail=0
for inp in "$HERE"/case*.in; do
  name=$(basename "$inp" .in)
  ans="$HERE/$name.ans"
  got=$(mktemp)
  if ! "$CLI" < "$inp" > "$got" 2>/dev/null; then
    echo "[RE] $name  (runtime error)"
    fail=$((fail + 1))
    rm -f "$got"
    continue
  fi
  if diff -q "$ans" "$got" >/dev/null 2>&1; then
    echo "[AC] $name"
    pass=$((pass + 1))
  else
    echo "[WA] $name"
    echo "  --- expected (first 5 lines) ---"; head -5 "$ans" | sed 's/^/  /'
    echo "  --- actual   (first 5 lines) ---"; head -5 "$got" | sed 's/^/  /'
    fail=$((fail + 1))
  fi
  rm -f "$got"
done

echo "---------------- AC $pass / $((pass + fail)) ----------------"
[ "$fail" -eq 0 ]
