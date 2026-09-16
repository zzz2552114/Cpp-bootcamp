# Bootcamp 配套 Project 路线图

> 面向：已学 CSAPP 前 8 章、熟悉算法竞赛、会 `struct`/STL/`using namespace std` 的学习者。
> 目标：把你从"会写 C++"带到"能舒服地读懂和修改 15-445/BusTub 的 C++17 代码"。
> 语言：C++17（不引入 C++20/23）。构建统一 `-std=c++17 -pthread`。

## 使用方式

1. 顺序读 Bootcamp 源文件（中文版），**不读 notes**（你用自己的笔记）。
2. 读完对应文件后，打开 `problems/stageN.md` 做该阶段项目。
3. **先自己写，不看 `solutions/`**。每题末尾有"自查三问"，做完先自答。
4. 再对照 `solutions/stageN/`：`pX_Y_*.h` 是**带详细注释的参考实现**，`pX_Y_*_test.cpp` 是**分级测试点**。
   每跑一题会逐条打印 `PASS/FAIL` 并给出 `结果: N/M 通过`；全部跑通：`cd solutions && make check`。
5. 遇到卡住的点，回到 repo 对应 `.cpp` 逐行重看；不建议跳过自查直接抄答案。

## 构建与测试

`solutions/` 下每个 Project 都是 **`实现.h`（可读的答案）+ `_test.cpp`（分级测试点）**。
测试点会逐条打印 `PASS/FAIL`，最后给出 `结果: N/M 通过`。共用框架是一个 70 行的
`test_util.h`（`BT_TEST` / `BT_CHECK` / `BT_CHECK_EQ` / `BT_CHECK_THROWS` / `BT_MAIN`）。

### 方式 A：Makefile（推荐，无需 cmake）

```bash
cd projects/solutions
make -j8        # 编译全部测试
make check      # 编译 + 运行全部测试（含 P4.5 的洛谷式对拍）
make demo       # 额外编译"会死锁"的演示程序（p6_2_bank_deadlock）
```

### 方式 B：CMake

```bash
cd projects/solutions
cmake -S . -B build && cmake --build build -j
ctest --test-dir build --output-on-failure
```

### 方式 C：单独编译某一题

```bash
cd projects/solutions
g++ -std=c++17 -pthread -I. -O2 -o /tmp/t stage6/p6_3_blocking_queue_test.cpp && /tmp/t
```

### 三种测试形态

| 形态 | 覆盖 | 位置 |
|---|---|---|
| ① 分级单元测试点 | 全部 29 个套件 | `stageN/pX_Y_*_test.cpp` |
| ② 洛谷式 stdin/stdout 对拍 | P4.5 | `stage4/tests/`（`case*.in` / `case*.ans` / `run_tests.sh`） |
| ③ 随机对拍（vs 独立参考实现） | P1.5 / P4.5 / P5.1 / P6.1 / P7.2 | 各 `_test.cpp` 里的 `stress_*` 测试点 |

`.ans` 文件是用**独立的 Python 朴素实现**生成（`stage4/tests/gen_cases.py`），不是用 C++ 程序自己算的，
所以 `run_tests.sh` 的 diff 是真正的外部校验。

### 需要特殊编译宏/参数的题目

| 题目 | 用法 | 说明 |
|---|---|---|
| P3.3 | `./p3_3_rule_of_three_five_zero_test --demo-double-free` | 复现浅拷贝 double free（预期 abort） |
| P5.2 | `./p5_2_ownership_test --demo-reset-self` | 复现 `up.reset(up.get())` 的 double free |
| P6.2 | `make demo && timeout 5 ./build/p6_2_bank_deadlock --demo-deadlock` | 复现多锁死锁（预期挂起） |
| P3.1 | `g++ -fno-elide-constructors ...` | 关掉 copy elision，观察凭空多出的 move |

## 阶段总览

| 阶段 | 主题 | 对应 Bootcamp 源文件 | 项目（problems 编号） | 关键新概念 |
|---|---|---|---|---|
| 1 | 引用与移动语义 | references.cpp / move_semantics.cpp / move_constructors.cpp | P1.1 ~ P1.5 | 引用、`const&`、重载解析、`std::move`、移动构造/赋值、self-move、`noexcept`、copy elision |
| 2 | 模板 | templated_functions.cpp / templated_classes.cpp | P2.1 ~ P2.4 | 模板函数/类、特化、非类型参数、`constexpr if`、头文件分离、CTAD |
| 3 | 包装类/迭代器/命名空间 | wrapper_class.cpp / iterator.cpp / namespaces.cpp | P3.1 ~ P3.4 | RAII、RVO、双向迭代器（含 `--end()`）、range-for、Rule of 0/3/5、`.h/.cpp`、include guard |
| 4 | STL 容器 | vectors.cpp / sets.cpp / unordered_maps.cpp / auto.cpp | P4.1 ~ P4.5 | 迭代器失效、`reserve`/`resize`、比较器、自定义 `std::hash`、`operator[]` 陷阱、`auto`/`decltype`/结构化绑定、erase-remove |
| 5 | 智能指针 | unique_ptr.cpp / shared_ptr.cpp / spring2024/s24_my_ptr.cpp | P5.1 ~ P5.4 | `unique_ptr` ownership tree、所有权传参、`shared_ptr` 引用计数、`weak_ptr` 环 |
| 6 | 并发同步 | mutex.cpp / scoped_lock.cpp / condition_variable.cpp / rwlock.cpp | P6.1 ~ P6.4 | 竞态、RAII 锁、多锁死锁与规避、条件变量（谓词/关闭协议）、读写锁 |
| 7 | 综合 | s24_my_ptr.cpp + 全部 | P7.1 ~ P7.2 | 手写 `unique_ptr`、Mini BufferPool |

## 每题的固定格式

- **对应源文件 / 前置**：做完什么再开始。
- **任务**：要实现的接口。（该接口是"最小完整规格"，你可以在不改变语义的前提下微调。）
- **验收**：`main()` 里断言什么、预期输出。
- **自查三问**：做完必须能回答的 3 个问题（对应 repo 里的"为什么"）。

## 重点声明（贯穿全部题）

- `std::move(x)` 只是把 `x` 转成 xvalue 的一次 cast，**本身不移动任何东西**；真正的搬运发生在移动构造/移动赋值里。
- **moved-from 对象仍然活着**，处于 "valid but unspecified" 状态：可以安全析构、可以重新赋值，但不要依赖它原来的内容。
- 锁也是一种资源 → 用 RAII 管理；多锁用 `std::scoped_lock` 避免死锁。
- `auto` **会剥离引用和顶层 const**：`const std::string& s = ...; auto x = s;` 得到的是 `std::string`（拷贝）。想借用用 `const auto&`。
- 成员函数加 `const` 后，其内 `this` 类型为 `const T*`；要在 `const` 成员里加锁，mutex 必须 `mutable`。

## 关于难度与分工

- 每题保持在"1 个核心概念 + 少量顺带练习"的粒度，不引入与当下概念无关的复杂度。
- 涉及多文件（P2.4、P3.4）的题会明确目录结构；其余都是单文件自包含。
- 标有 **[选做]** 的可以放到最后，不影响主线。