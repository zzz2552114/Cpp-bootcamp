# 参考解答与测试点

## 结构

```text
solutions/
├── test_util.h              # 共用迷你测试框架（约 70 行）
├── Makefile                 # make -j8 / make check / make demo
├── CMakeLists.txt           # 等价的 CMake 构建（ctest 集成）
├── stage1/ ... stage7/
│   ├── pX_Y_<topic>.h       # ★ 参考实现（带详细注释）—— 读这个
│   ├── pX_Y_<topic>_test.cpp# ★ 分级测试点 —— 跑这个
│   └── (个别题目含多文件工程目录)
└── stage4/tests/            # P4.5 的洛谷式 stdin/stdout 测试点
    ├── case*.in / case*.ans
    ├── gen_cases.py         # 用【独立 Python 朴素实现】生成 .ans
    └── run_tests.sh         # 批量 diff
```

命名约定：**目标名 == 测试源文件去掉 `.cpp`**。

## 怎么跑

```bash
cd solutions
make -j8           # 只编译
make check         # 编译 + 运行全部（29 个套件，含 P4.5 洛谷式对拍）
make demo          # 额外编译会死锁的演示程序

# 单题
g++ -std=c++17 -pthread -I. -O2 -o /tmp/t stage1/p1_3_buffer_test.cpp && /tmp/t

# CMake 路线
cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure
```

`make check` 的典型输出：

```text
  [OK]   p1_3_buffer_test                     结果: 17/17
  [OK]   p3_2_iterator_test                   结果: 17/17
  [OK]   p7_2_mini_buffer_pool_test           结果: 16/16
  [OK]   p4_5_luogu_cases                     ---------------- AC 14 / 14 ----------------
================================================================
  测试套件: 29 个   失败: 0 个
```

## 测试点写法的约定

`test_util.h` 提供：

| 宏 | 作用 |
|---|---|
| `BT_TEST(suite, name)` | 注册一个测试点，显示名为 `suite/name` |
| `BT_CHECK(cond)` | 断言条件，失败时打印条件文本与行号 |
| `BT_CHECK_EQ(a, b)` | 断言相等，失败时打印**左右两边的实际值** |
| `BT_CHECK_NE(a, b)` | 断言不等 |
| `BT_CHECK_THROWS(expr, ExType)` | 断言抛出指定类型异常（没抛/抛错类型都算失败） |
| `BT_MAIN("标题")` | 生成 `main()`，跑完打印 `结果: N/M 通过`，全绿返回 0 |

每个套件的测试点**由易到难、由宽到边**排列，大致顺序是：
正常路径 → 空/边界 → moved-from / 自赋值 → 资源归还（无泄漏）→ 类型层面 `static_assert` → 大规模（1e5~1e7）→ 随机对拍。

## 需要特殊参数/宏的题目

| 题目 | 命令 | 预期现象 |
|---|---|---|
| P3.3 | `./build/p3_3_rule_of_three_five_zero_test --demo-double-free` | `free(): double free detected` → abort |
| P5.2 | `./build/p5_2_ownership_test --demo-reset-self` | 同上（`reset(get())` 的后果） |
| P6.2 | `make demo && timeout 5 ./build/p6_2_bank_deadlock --demo-deadlock` | 挂起（exit 124 = 真死锁） |
| P3.1 | 加 `-fno-elide-constructors` 重新编译 | 出现额外的 `move-ctor`（平时是 0 次移动） |
| P1.4 | 见 `stage1/p1_4_noexcept_test.cpp` | `Counted`（noexcept）扩容零拷贝；`CountedThrowy` 扩容走拷贝 |

## 验证状态

- `make check`：**29 个套件全绿**（381 个测试点，含 14 个洛谷式用例）
- `g++ -fsanitize=address,undefined` + LSan：**28 个可编译套件全部干净**（无泄漏、无 UB、无越界）
- 唯一的例外是 P5.4 里**故意演示**的 `shared_ptr` 成环泄漏，测试里已手工断环回收
