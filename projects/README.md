# Bootcamp 配套 Project 路线图

> 面向：已学 CSAPP 前 8 章、熟悉算法竞赛、会 `struct`/STL/`using namespace std` 的学习者。
> 目标：把你从"会写 C++"带到"能舒服地读懂和修改 15-445/BusTub 的 C++17 代码"。
> 语言：C++17。除个别题（多文件、并发）外，编译只需要 `-std=c++17` 和 `-I`。

---

## 一、目录结构

```text
projects/
├── problems/               # ★ 题目描述：stage1.md ~ stage7.md，先读这个
├── mysol/                  # ★ 你的解答写在这里（每个 stage 一个子目录）
├── solutions/              # 参考实现 + 测评程序（只读，不要改）
│   ├── test_util.h         # 共用测试框架（BT_TEST / BT_CHECK / ...）
│   └── stage1/ ... stage7/
│       ├── pX_Y_*.h            # 参考实现（带详细注释）
│       └── pX_Y_*_test.cpp     # 测评程序（自带 main，用它验证你的代码）
├── README.md               # ← 你现在看的这个
└── REVIEW.md               # 对早期题目设计的评审（历史记录）
```

**题目的完整说明在 `problems/stageN.md`**：每道题都会写清楚"要你设计出什么、测评程序要求哪些接口、
每个接口为什么长这样、测评点在查什么、怎么编译运行"。`problems/` 才是你应该逐字读的文档。

---

## 二、使用方式（每一步都很具体）

对每个阶段：

1. **读 src**：按 `problems/stageN.md` 开头的"先读 src"列表，把对应章节的 `.cpp` 读完。
2. **读题**：打开 `problems/stageN.md`，先看"做题之前必须懂的几件事"，再看每道题。
3. **写代码**：在 `projects/mysol/stageN/` 下创建题目要求的 `.h` 文件。**只写头文件，不要写 `main()`**
   （`main()` 由测评程序提供）。
4. **把测评程序复制到你的目录**：测评程序用 `#include "pX_Y_*.h"` 找你的头文件，而双引号包含
   会优先在测评文件自己所在目录里找。所以必须把测评文件复制到你的 `.h` 旁边，否则会错误地
   包含到 `solutions/` 里的参考实现。
5. **编译运行**，看到 `Result: N/N Passed` 即通过（测评程序输出全部是英文，避免中文乱码）。
6. 卡住时再回看 `solutions/stageN/pX_Y_*.h` 的参考实现和注释；每题末尾的"自查"用来确认你真的懂了。

```bash
# 以 Stage 1 的 P1.1 为例
mkdir -p projects/mysol/stage1
cp projects/solutions/stage1/p1_1_statistics_test.cpp projects/mysol/stage1/
# 在 projects/mysol/stage1/p1_1_statistics.h 里写你的实现，然后：
cd projects/mysol/stage1
g++ -std=c++17 p1_1_statistics_test.cpp -I../../solutions -o p1_1 && ./p1_1
```

`-I../../solutions` 只是为了让测评文件找到 `test_util.h`。除此以外不需要别的参数。

---

## 三、编译命令速查（尽量精简）

所有命令都在 `projects/mysol/stageN/` 下执行。

| 情形 | 题目 | 命令 |
| :-- | :-- | :-- |
| 普通单文件（默认） | stage1、stage2 的 P2.1~P2.3、stage3 的 P3.1~P3.3、stage4、stage5、stage7 | `g++ -std=c++17 pX_Y_<topic>_test.cpp -I../../solutions -o pX_Y && ./pX_Y` |
| 并发（必须加 `-pthread`） | stage6 全部、P7.2 | `g++ -std=c++17 -pthread pX_Y_<topic>_test.cpp -I../../solutions -o pX_Y && ./pX_Y` |
| 多文件（要连你的 `.cpp` 一起编） | P2.4 | `g++ -std=c++17 p2_4_minimath_test.cpp minimath/min.cpp -I../../solutions -o p2_4 && ./p2_4` |
| 多文件（要连你的 `.cpp` 一起编） | P3.4 | `g++ -std=c++17 p3_4_mylib_test.cpp mylib/geometry.cpp mylib/stats.cpp -I../../solutions -o p3_4 && ./p3_4` |
| stdin/stdout 对拍（P4.5 附加） | P4.5 | 先 `g++ -std=c++17 p4_5_log_analyzer_cli.cpp -o cli`，再 `bash ../../solutions/stage4/tests/run_tests.sh ./cli` |

每道题的具体命令在 `problems/stageN.md` 里都有，可以直接使用。

---

## 四、三种测评形态

| 形态 | 覆盖 | 位置 |
| :-- | :-- | :-- |
| ① 分级单元测试点 | 全部 28 个分级套件 | `solutions/stageN/pX_Y_*_test.cpp` |
| ② stdin/stdout 对拍 | P4.5 | `solutions/stage4/tests/`（`case*.in` / `case*.ans` / `run_tests.sh`） |
| ③ 随机对拍（对独立参考实现） | P1.5 / P4.5 / P5.1 / P6.1 / P7.2 | 各 `_test.cpp` 里的 `stress_*` 测试点 |

P4.5 的 `.ans` 由**独立的 Python 朴素实现**生成（`solutions/stage4/tests/gen_cases.py`），
所以 `run_tests.sh` 的 diff 是真正的外部校验。

---

## 五、需要特殊编译参数/运行参数的题目

| 题目 | 命令 | 预期现象 |
| :-- | :-- | :-- |
| P3.1 | 加 `-fno-elide-constructors` 重新编译 | 观察拷贝消除被关闭后的构造/移动差异（实验，非通过标准） |
| P3.3 | `./p3_3 --demo-double-free` | `free(): double free detected` → abort |
| P5.2 | `./p5_2 --demo-reset-self` | 同上（`reset(get())` 的后果） |
| P6.2 | `g++ -std=c++17 -pthread -DDEMO_DEADLOCK ...`，再 `timeout 5 ./out --demo-deadlock` | 挂起（退出码 124 = 真死锁） |
| P1.3 / P1.4 / P1.5 | 可选 `-fsanitize=address` | 检查越界、泄漏、UB |

---

## 六、阶段总览

| 阶段 | 主题 | 对应 Bootcamp 源文件 | 题目 |
| :--: | :-- | :-- | :--: |
| 1 | 引用与移动语义 | references / move_semantics / move_constructors | P1.1 ~ P1.5 |
| 2 | 模板 | templated_functions / templated_classes | P2.1 ~ P2.4 |
| 3 | 包装类/迭代器/命名空间 | wrapper_class / iterator / namespaces | P3.1 ~ P3.4 |
| 4 | STL 容器 | vectors / sets / unordered_maps / auto | P4.1 ~ P4.5 |
| 5 | 智能指针 | unique_ptr / shared_ptr / s24_my_ptr | P5.1 ~ P5.4 |
| 6 | 并发同步 | mutex / scoped_lock / condition_variable / rwlock | P6.1 ~ P6.4 |
| 7 | 综合 | s24_my_ptr + 全部 | P7.1 ~ P7.2 |

每题固定给出四样东西：**目标 / 要写的文件 / 测评程序要求的接口（含为什么）/ 测评点与编译命令**。

---

## 七、几条贯穿全部题目的重点

- `std::move(x)` 只是把 `x` 转成右值的一次 cast，**本身不移动任何东西**；真正的搬运发生在移动构造/移动赋值里。
- **moved-from 对象仍然活着**，处于 "valid but unspecified" 状态：可以安全析构、可以重新赋值，但不要依赖它原来的内容。
- 锁也是一种资源 → 用 RAII 管理（`scoped_lock`/`unique_lock`）；多锁用 `std::scoped_lock` 避免死锁。
- `auto` **会剥掉引用和顶层 const**：`const std::string& s = ...; auto x = s;` 得到的是 `std::string`（拷贝）。想借用用 `const auto&`。
- 成员函数加 `const` 后，其内 `this` 类型为 `const T*`；要在 `const` 成员里加锁，mutex 必须 `mutable`。
- 容器/智能指针/锁这些"管理资源"的类型，优先让**成员自己**管好所有权（Rule of 0），少手写 `delete`。
