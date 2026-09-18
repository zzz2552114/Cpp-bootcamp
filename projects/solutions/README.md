# 参考解答与测评程序

> 这个目录是**只读对照**用的：`pX_Y_*.h` 是参考答案，`pX_Y_*_test.cpp` 是测评程序。
> 你要写自己的实现到 `projects/mysol/stageN/`，**不要把答案抄进这里**。
> 题目的完整说明在 `projects/problems/stageN.md`。

## 目录结构

```text
solutions/
├── test_util.h               # 共用迷你测试框架（BT_TEST / BT_CHECK / BT_MAIN）
├── stage1/ ... stage7/
│   ├── pX_Y_<topic>.h         # 参考实现（带详细注释）
│   ├── pX_Y_<topic>_test.cpp  # 测评程序，自带 main()，逐条打印 PASS/FAIL
│   └── (个别题目含多文件工程目录，如 P2.4 / P3.4)
└── stage4/tests/             # P4.5 的 stdin/stdout 对拍用例
    ├── case*.in / case*.ans
    ├── gen_cases.py          # 用【独立 Python 朴素实现】生成 .ans
    └── run_tests.sh          # 批量 diff
```

命名约定：**可执行文件名一般 == 测评源文件去掉 `.cpp`**。

## 测评程序怎么跑

给**参考实现**跑，或者给**你自己的实现**跑，方法一样：把 `_test.cpp` 放到你的 `.h` 旁边再编译。

```bash
# 例：在 solutions/ 目录下直接验证参考实现（单文件题）
cd projects/solutions
g++ -std=c++17 stage1/p1_1_statistics_test.cpp -I. -o p1_1 && ./p1_1
```

```text
================ Test Suite p1_1_statistics_test.cpp ================
[  1] PASS  P1_1/empty_sum_is_zero
[  2] PASS  P1_1/empty_average_is_zero_not_nan
...
[ 12] PASS  P1_1/move_does_not_count_as_copy
---------------- Result: 12/12 Passed ----------------
```

测评程序的所有输出都是英文（`PASS` / `FAIL` / `Result: N/M Passed`），避免终端中文乱码；
代码里的注释仍保持中文。

少数题目的编译方式不同（多文件、并发）：

```bash
# 并发题：加 -pthread
g++ -std=c++17 -pthread stage6/p6_1_counter_test.cpp -I. -o p6_1 && ./p6_1

# 多文件题：连 .cpp 一起编
g++ -std=c++17 -I. stage2/p2_4_minimath/p2_4_minimath_test.cpp stage2/p2_4_minimath/minimath/min.cpp -o p2_4 && ./p2_4
g++ -std=c++17 -I. stage3/p3_4_mylib/p3_4_mylib_test.cpp stage3/p3_4_mylib/mylib/geometry.cpp stage3/p3_4_mylib/mylib/stats.cpp -o p3_4 && ./p3_4

# P4.5 洛谷式对拍
g++ -std=c++17 -I. stage4/p4_5_log_analyzer_cli.cpp -o cli
bash stage4/tests/run_tests.sh ./cli
```

## `test_util.h` 宏速查

| 宏 | 作用 |
| :-- | :-- |
| `BT_TEST(suite, name)` | 注册一个测试点，显示名为 `suite/name` |
| `BT_CHECK(cond)` | 断言条件，失败时打印条件文本与行号 |
| `BT_CHECK_EQ(a, b)` | 断言相等，失败时打印**左右两边的实际值** |
| `BT_CHECK_NE(a, b)` | 断言不等 |
| `BT_CHECK_THROWS(expr, ExType)` | 断言抛出指定类型异常（没抛/抛错类型都算失败） |
| `BT_MAIN("标题")` | 生成 `main()`，跑完打印 `Result: N/M Passed`，全绿返回 0 |

每个套件的测试点由易到难排列，大致顺序是：
正常路径 → 空/边界 → moved-from / 自赋值 → 资源归还（无泄漏）→ 类型层面 `static_assert` →
大规模（1e5~1e7）→ 随机对拍。

## 三种测评形态

| 形态 | 覆盖 | 位置 |
| :-- | :-- | :-- |
| ① 分级单元测试点 | 全部 28 个分级套件 | `stageN/pX_Y_*_test.cpp` |
| ② stdin/stdout 对拍 | P4.5 | `stage4/tests/` |
| ③ 随机对拍（对独立参考实现） | P1.5 / P4.5 / P5.1 / P6.1 / P7.2 | 各 `_test.cpp` 里的 `stress_*` 测试点 |

## 需要特殊参数/宏的题目

| 题目 | 命令 | 预期现象 |
| :-- | :-- | :-- |
| P3.3 | `g++ -std=c++17 stage3/p3_3_rule_of_three_five_zero_test.cpp -I. -o p3_3 && ./p3_3 --demo-double-free` | `free(): double free detected` → abort |
| P5.2 | `g++ -std=c++17 -pthread stage5/p5_2_ownership_test.cpp -I. -o p5_2 && ./p5_2 --demo-reset-self` | 同上（`reset(get())` 的后果） |
| P6.2 | `g++ -std=c++17 -pthread -DDEMO_DEADLOCK stage6/p6_2_bank_test.cpp -I. -o p6_2 && timeout 5 ./p6_2 --demo-deadlock` | 挂起（退出码 124 = 真死锁） |
| P3.1 | 加 `-fno-elide-constructors` 重新编译 | 观察拷贝消除被关闭后的差异 |
| P1.4 | 直接跑 `p1_4_noexcept_test.cpp` | `Counted`（noexcept）扩容零拷贝；`CountedThrowy` 扩容走拷贝 |

## 验证状态

- 28 个分级套件全绿（367 个测试点），P4.5 的 14 个洛谷式用例全部 AC。
- `-fsanitize=address,undefined` + LSan：除 P5.4 故意演示的 `shared_ptr` 成环泄漏外全部干净。
