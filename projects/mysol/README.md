# mysol · 个人解答目录

这里是针对 project 题目自己写的实现。**题目的完整说明在 [`projects/problems/stageN.md`](../problems/stageN.md)**，
每题都写清了文件名、接口要求、测评点和编译命令，请以那里为准。

## 目录约定

```text
projects/mysol/
├── stage1/          # 对应 problems/stage1.md
│   ├── p1_1_statistics.h        # ★ 你写的实现
│   ├── p1_1_statistics_test.cpp # 从 tests/ 复制过来的测评程序
│   └── p1_1 (可执行文件，忽略)
├── stage2/
│   └── ...
└── ...
```

## 三步流程

1. 在 `mysol/stageN/` 下按题面创建 `.h` 文件。**只写头文件，不要写 `main()`**——
   测评程序自带 `main()`，你写了会重复定义。
2. 把对应的测评文件从 `tests/stageN/` 复制到同一个目录。
   原因：测评程序用 `#include "pX_Y_*.h"` 找你的头文件，而双引号包含会优先搜索
   **测评文件自己所在目录**；不复制的话，它会直接包含 `tests/` 里的参考实现，你的代码根本没被编译。
3. 在该目录编译运行：

```bash
# 单文件题（默认）
g++ -std=c++17 p1_1_statistics_test.cpp -I../../tests -o p1_1 && ./p1_1

# 并发题（stage6 全部、P7.2）：加 -pthread
g++ -std=c++17 -pthread p6_1_counter_test.cpp -I../../tests -o p6_1 && ./p6_1

# 多文件题（P2.4 / P3.4）：连你的 .cpp 一起编
g++ -std=c++17 p2_4_minimath_test.cpp minimath/min.cpp -I../../tests -o p2_4 && ./p2_4
```

`-I../../tests` 只是为了让测评文件找到 `test_util.h`，不需要其它参数。

## 预期输出

```text
================ Test Suite p1_1_statistics_test.cpp ================
[  1] PASS  P1_1/empty_sum_is_zero
[  2] PASS  P1_1/empty_average_is_zero_not_nan
...
[ 12] PASS  P1_1/move_does_not_count_as_copy
---------------- Result: 12/12 Passed ----------------
```

看到 `Result: N/N Passed` 就是通过（测评程序输出为英文，避免终端中文乱码）。

## 跑完一个阶段的全部题

```bash
cd projects/mysol/stage1
for f in *_test.cpp; do
  base="${f%.cpp}"
  echo "=== $base ==="
  g++ -std=c++17 "$f" -I../../tests -o "/tmp/$base" && "/tmp/$base"
done
```

（stage6 的脚本里给 `g++` 加 `-pthread`。）

## 写完再看答案

先自己独立完成，再对照 `tests/stageN/pX_Y_*.h`（参考实现，带详细注释）。
每道题末尾有"自查"，做完先自己答一遍，能答上来说明真的懂了。
