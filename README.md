[![License](https://img.shields.io/badge/license-Apache%202.0-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)

## 现代C++ bootcamp 
> ***Fork form https://github.com/ascendho/cpp-bootcamp***

本仓库在 CMU 15-445 bootcamp的教学内容基础上，**额外配套了一整套带测评与讲解的实战 Project**。适合自学，也可作为从 C 过渡到现代 C++ 的开发者的参考

> **配套笔记**：本仓库不内置笔记，学习过程中的个人笔记见
> <a href="https://github.com/zzz2552114/Notes/tree/main/Cpp"> 我的笔记仓库-Cpp </a>。

---

## 一、训练营正文

### 索引

| 序号 |             章节              |                             代码                             |
| :--: | :---------------------------: | :----------------------------------------------------------: |
|  1   | References and Move Semantics | <a href="1 - References and Move Semantics/references.cpp">references.cpp</a> |
|      |                               | <a href="1 - References and Move Semantics/move_semantics.cpp">move_semantics.cpp</a> |
|      |                               | <a href="1 - References and Move Semantics/move_constructors.cpp">move_constructors.cpp</a> |
|  2   |         C++ Templates         | <a href="2 - C++ Templates/templated_functions.cpp">templated_functions.cpp</a> |
|      |                               | <a href="2 - C++ Templates/templated_classes.cpp">templated_classes.cpp</a> |
|  3   |             Misc              |  <a href="3 - Misc/wrapper_class.cpp">wrapper_class.cpp</a>  |
|      |                               |       <a href="3 - Misc/iterator.cpp">iterator.cpp</a>       |
|      |                               |     <a href="3 - Misc/namespaces.cpp">namespaces.cpp</a>     |
|  4   |          Containers           |     <a href="4 - Containers/vectors.cpp">vectors.cpp</a>     |
|      |                               |        <a href="4 - Containers/sets.cpp">sets.cpp</a>        |
|      |                               | <a href="4 - Containers/unordered_maps.cpp">unordered_maps.cpp</a> |
|      |                               |        <a href="4 - Containers/auto.cpp">auto.cpp</a>        |
|  5   |            Memory             |    <a href="5 - Memory/unique_ptr.cpp">unique_ptr.cpp</a>    |
|      |                               |    <a href="5 - Memory/shared_ptr.cpp">shared_ptr.cpp</a>    |
|  6   |       Synch Primitives        |    <a href="6 - Synch Primitives/mutex.cpp">mutex.cpp</a>    |
|      |                               | <a href="6 - Synch Primitives/scoped_lock.cpp">scoped_lock.cpp</a> |
|      |                               | <a href="6 - Synch Primitives/condition_variable.cpp">condition_variable.cpp</a> |
|      |                               |   <a href="6 - Synch Primitives/rwlock.cpp">rwlock.cpp</a>   |
|  -   |          spring2024           |    <a href="spring2024/s24_my_ptr.cpp">s24_my_ptr.cpp</a>    |

### 构建方式

该训练营包含若干 C++ 代码文件，位于各章节目录下，建议深入研读。每个代码文件均可编译为与其同名的可执行文件。***我删除了 cmake 统一编译，因为这里的 cmake 根本不是什么组合成一个，而是帮你批量单独编译***

```bash
g++ -std=c++20 -O2 <file> -o <destination>
```

---

## 二、配套 Project（本仓库特有）

在读完每一章的训练营源文件后，可以进入 `projects/` 完成对应阶段的编程题。覆盖 **7 个阶段、共 29 道题**，每道题都配有**分级测试点**与**带详细注释的参考解答**。

```text
projects/
├── README.md            # 路线图与使用方式
├── REVIEW.md            # 对题目设计的评审（含实测踩坑记录）
├── problems/            # 题目：stage1.md ~ stage7.md
└── solutions/           # 参考解答 + 测试点
    ├── test_util.h               # 测试框架
    ├── Makefile / CMakeLists.txt # 两套等价构建
    ├── stage1/ ... stage7/       # pX_Y_<topic>.h（答案）+ pX_Y_<topic>_test.cpp（测试）
    └── stage4/tests/             # P4.5 多测试点测试
```

### 阶段总览

### 阶段总览

| 阶段 | 主题 | 对应 Bootcamp 源文件 | 题目 |
| :--: | :--: | :--: | :--: |
| 1 | 引用与移动语义 | references / move_semantics / move_constructors | P1.1 ~ P1.5 |
| 2 | 模板 | templated_functions / templated_classes | P2.1 ~ P2.4 |
| 3 | 包装类/迭代器/命名空间 | wrapper_class / iterator / namespaces | P3.1 ~ P3.4 |
| 4 | STL 容器 | vectors / sets / unordered_maps / auto | P4.1 ~ P4.5 |
| 5 | 智能指针 | unique_ptr / shared_ptr / s24_my_ptr | P5.1 ~ P5.4 |
| 6 | 并发同步 | mutex / scoped_lock / condition_variable / rwlock | P6.1 ~ P6.4 |
| 7 | 综合 | s24_my_ptr + 全部 | P7.1 ~ P7.2 |

### 使用方式

1. 顺序读 Bootcamp 源文件，可参考我的笔记 。
2. 读完对应文件后，打开 `projects/problems/stageN.md` 做该阶段项目。
3. 再对照 `projects/solutions/stageN/`：`pX_Y_*.h` 是带详细注释的参考实现，`pX_Y_*_test.cpp` 是分级测试点，逐条打印 `PASS/FAIL` 并给出 `结果: N/M 通过`。
4. 遇到卡住的点，回到 repo 对应 `.cpp` 逐行重看。

### 构建与测试

`solutions/` 下每个 Project 都是 **`实现.h`（答案）+ `_test.cpp`（测试）**，共用框架 `test_util.h`。

```bash
# 方式 A：Makefile（推荐，无需 cmake）
cd projects/solutions
make -j8        # 编译全部测试
make check      # 编译 + 运行全部测试（含 P4.5 的洛谷式对拍）
make demo       # 额外编译"会死锁"的演示程序 p6_2_bank_deadlock


# 方式 B：单独编译某一题
g++ -std=c++17 -pthread -I. -O2 -o /tmp/t stage6/p6_3_blocking_queue_test.cpp && /tmp/t
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

### 三种测试形态

| 形态 | 覆盖 | 位置 |
| :--: | :--: | :-- |
| ① 分级单元测试点 | 全部 29 个套件 | `stageN/pX_Y_*_test.cpp` |
| ② stdin/stdout 对拍 | P4.5 | `stage4/tests/`（`case*.in` / `case*.ans` / `run_tests.sh`） |
| ③ 随机对拍（vs 独立参考实现） | P1.5 / P4.5 / P5.1 / P6.1 / P7.2 | 各 `_test.cpp` 里的 `stress_*` 测试点 |

P4.5 的 `.ans` 由**独立的 Python 朴素实现**生成（`stage4/tests/gen_cases.py`），因此 `run_tests.sh` 的 diff 是真正的外部校验。

### 需要特殊编译宏/参数的题目

| 题目 | 命令 | 预期现象 |
| :--: | :-- | :-- |
| P3.1 | 加 `-fno-elide-constructors` 重新编译 | 出现额外的 move-ctor（平时 0 次移动） |
| P3.3 | `./build/p3_3_rule_of_three_five_zero_test --demo-double-free` | `free(): double free detected` → abort |
| P5.2 | `./build/p5_2_ownership_test --demo-reset-self` | 同上（`reset(get())` 的后果） |
| P6.2 | `make demo && timeout 5 ./build/p6_2_bank_deadlock --demo-deadlock` | 挂起（exit 124 = 真死锁） |



---


## 四、许可证

本项目基于 <a href="https://github.com/cmu-db/15445-bootcamp">15445-bootcamp</a>（CMU）翻译和扩展。原始源代码版权归属 <a href="https://github.com/cmu-db/15445-bootcamp/graphs/contributors">**原作者**</a> 所有。全部内容遵循 <a href="LICENSE">Apache License 2.0</a> 协议。