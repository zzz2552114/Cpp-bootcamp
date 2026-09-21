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

在读完每一章的训练营源文件后，可以进入 `projects/` 完成对应阶段的编程题。覆盖 **7 个阶段、共 28 道题**，每道题都配有**分级测试点**与**带详细注释的参考解答**。

```text
projects/
├── README.md            # 路线图与使用方式（含编译命令速查）
├── REVIEW.md            # 对早期题目设计的评审（历史记录）
├── problems/            # ★ 题目：stage1.md ~ stage7.md（逐字读这个）
├── mysol/               # ★ 你的解答写在这里
└── tests/           # 参考解答 + 测评程序（只读）
    ├── test_util.h               # 测评框架（BT_TEST / BT_CHECK / BT_MAIN）
    ├── stage1/ ... stage7/       # pX_Y_<topic>.h（答案）+ pX_Y_<topic>_test.cpp（测评）
    └── stage4/tests/             # P4.5 的 stdin/stdout 对拍用例
```

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

1. 顺序读 Bootcamp 源文件，可参考我的笔记。
2. 读完对应文件后，打开 `projects/problems/stageN.md`。每道题都写清了：**要你设计出什么、测评程序
   要求哪些接口、每个接口为什么长这样、测评点在查什么、怎么编译运行**。
3. 在 `projects/mysol/stageN/` 下写你的 `.h`（只写头文件，**不要写 `main()`**），
   把对应的测评文件从 `projects/tests/stageN/` 复制到同一目录，然后编译运行。
4. 遇到卡住的点，回到 repo 对应 `.cpp` 逐行重看，或对照 `tests/stageN/pX_Y_*.h` 的参考实现。

### 构建与测试

每个 Project 要求你实现一个头文件（例如 `p1_1_statistics.h`），测评程序（例如
`p1_1_statistics_test.cpp`）自带 `main()`。把测评文件复制到你的头文件旁边，用**尽可能精简的命令**编译：

```bash
# 在 projects/mysol/stage1/ 下（单文件题）
g++ -std=c++17 p1_1_statistics_test.cpp -I../../tests -o p1_1 && ./p1_1
```

- 并发题（stage6 全部、P7.2）加 `-pthread`；
- 多文件题（P2.4、P3.4）把你的 `.cpp` 一起写进命令；
- 完整的命令速查表见 [`projects/README.md`](projects/README.md)。

典型输出（测评程序输出为英文，避免终端中文乱码）：

```text
================ Test Suite p1_3_buffer_test.cpp ================
[  1] PASS  P1_3/construct_and_size
[  2] PASS  P1_3/elements_are_zero_initialized
...
---------------- Result: 17/17 Passed ----------------
```

### 三种测试形态

| 形态 | 覆盖 | 位置 |
| :--: | :--: | :-- |
| ① 分级单元测试点 | 全部 28 个分级套件 | `stageN/pX_Y_*_test.cpp` |
| ② stdin/stdout 对拍 | P4.5 | `stage4/tests/`（`case*.in` / `case*.ans` / `run_tests.sh`） |
| ③ 随机对拍（vs 独立参考实现） | P1.5 / P4.5 / P5.1 / P6.1 / P7.2 | 各 `_test.cpp` 里的 `stress_*` 测试点 |

P4.5 的 `.ans` 由**独立的 Python 朴素实现**生成（`stage4/tests/gen_cases.py`），因此 `run_tests.sh` 的 diff 是真正的外部校验。

### 需要特殊编译宏/参数的题目

| 题目 | 命令 | 预期现象 |
| :--: | :-- | :-- |
| P3.1 | 加 `-fno-elide-constructors` 重新编译 | 观察拷贝消除被关闭后的构造/移动差异（实验，非通过标准） |
| P3.3 | `./p3_3 --demo-double-free` | `free(): double free detected` → abort |
| P5.2 | `./p5_2 --demo-reset-self` | 同上（`reset(get())` 的后果） |
| P6.2 | `g++ -std=c++17 -pthread -DDEMO_DEADLOCK ...`，再 `timeout 5 ./out --demo-deadlock` | 挂起（exit 124 = 真死锁） |



---


## 四、许可证

本项目基于 <a href="https://github.com/cmu-db/15445-bootcamp">15445-bootcamp</a>（CMU）翻译和扩展。原始源代码版权归属 <a href="https://github.com/cmu-db/15445-bootcamp/graphs/contributors">**原作者**</a> 所有。全部内容遵循 <a href="LICENSE">Apache License 2.0</a> 协议。