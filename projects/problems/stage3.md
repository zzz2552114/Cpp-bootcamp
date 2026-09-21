# Stage 3 · 包装类 / 迭代器 / 命名空间

> **先读完这三课再做题**
> 1. `src/3 - Misc/wrapper_class.cpp` —— RAII：资源生命周期绑定对象生命周期
> 2. `src/3 - Misc/iterator.cpp` —— 自己实现迭代器（`*`、`++`、`==`、`Begin/End`）
> 3. `src/3 - Misc/namespaces.cpp` —— 命名空间、`using`、嵌套命名空间
>
> 主线考 RAII、迭代器协议、命名空间组织。另外两件事 src 没细讲，但测评要用、而且很值得懂：
> **拷贝消除 / RVO**（P3.1 要观察"按值返回资源管理对象时发生了几次构造"）和
> **Rule of 0/3/5**（P3.3 要把隐式浅拷贝导致的 double free 一步步拆开）。这两块放在第 1 节。

---

## 0. 本阶段题目一览

| 题号 | 主题 | 类型 | 你要写的文件（放 `projects/mysol/stage3/`） | 测评点数 |
| :--: | :-- | :--: | :-- | :--: |
| P3.1 | RAII 资源句柄 + RVO / copy elision | 主线+扩展 | `p3_1_handle.h` | 11 |
| P3.2 | 双向迭代器（修好 `--End()`）+ range-for | 主线 | `p3_2_iterator.h` | 17 |
| P3.4 | 命名空间 mini 库（`.h`/`.cpp`） | 主线 | `mylib/geometry.{h,cpp}` + `mylib/stats.{h,cpp}` | 10 |
| P3.3 | Rule of 0/3/5 | 扩展 | `p3_3_rule_of_three_five_zero.h` | 12 |

所有题目都**只写头文件（P3.4 另写 `.cpp`），不要写 `main()`**。

---

## 1. 做题之前必须懂的几件事

### 1.1 测评程序怎么用（回顾）

`projects/tests/stage3/*_test.cpp` 是测评程序，**自带 `main()`**；你只写头文件，不要写 `main()`。
断言宏的说明见 `stage1.md` 第 1.1 节。操作流程同样是：在 `projects/mysol/stage3/` 下写 `.h`，
把测评文件复制到同一目录，再编译。

```bash
mkdir -p projects/mysol/stage3
cp projects/tests/stage3/p3_1_handle_test.cpp                   projects/mysol/stage3/
cp projects/tests/stage3/p3_2_iterator_test.cpp                 projects/mysol/stage3/
cp projects/tests/stage3/p3_3_rule_of_three_five_zero_test.cpp  projects/mysol/stage3/
cp projects/tests/stage3/p3_4_mylib/p3_4_mylib_test.cpp         projects/mysol/stage3/
```

### 1.2 RAII：资源就是"跟着对象走的东西"

`wrapper_class.cpp` 的 `IntPtrManager` 是典型 RAII（Resource Acquisition Is Initialization，资源获取即初始化）：

- 构造函数里申请资源（`new int`）；
- 析构函数里释放资源（`delete ptr_`）；
- 于是资源的生命周期就等于对象的生命周期——对象在栈上，出作用域自动析构，资源自动释放；
- 因为"一个资源只能有一个 owner"，拷贝构造和拷贝赋值被 `= delete`，只保留移动。

RAII 就是 Stage 1 里 `Buffer` 的同一种思路，只是这次资源是"带 id 的句柄"，更接近真实项目里的文件描述符、
数据库连接、锁。`std::unique_ptr`、`std::lock_guard`、`std::fstream` 全是这个套路。

### 1.3 观察构造/析构/移动：计数器 + `ResetStats`

P3.1 要观察"创建 1000 个句柄"、"按值返回"各发生了多少次 acquire / release / move。
做法和 Stage 1 的 1.5 节一样：用 `inline static` 的静态计数器，再加一个 `ResetStats()` 静态方法，
让测评可以在每个测试点开头把计数清零，只统计这一段里发生的事。

这里新增的一个量是 `live`（当前存活资源数）：构造时 +1、析构时 -1。
如果一段代码结束时 `live` 回到进入前的值，就说明**没有泄漏、也没有重复释放**。

### 1.4 拷贝消除 / RVO：按值返回资源管理对象为什么不会多一次移动

考虑：

```cpp
Handle MakeHandle(int id) { return Handle(id); }
auto h = MakeHandle(42);
```

直觉上这里有"函数内构造一个临时对象 → 移动/拷贝到调用者的 `h`"，但 C++17 起规则变了：

- `return Handle(id);` 返回的是一个**纯右值（prvalue）**，标准规定它**直接在调用者的存储位置构造**，
  不产生临时对象、也不调用移动构造。这叫**保证的拷贝消除（guaranteed copy elision）**，不是"优化"，
  而是语言语义。
- 如果返回的是**具名局部变量**（`Handle tmp; return tmp;`），这叫 NRVO，**允许**编译器消除，但不保证；
  这时如果没有消除，就会调用移动构造（因为返回局部变量被视为右值）。

所以 P3.1 的 `rvo_makes_zero_moves` 测试点期望：`auto h = MakeHandle(99);` 之后 `moves == 0`、`acquires == 1`。
而显式写 `Handle b(std::move(a));` 会实实在在调用一次移动构造，`moves == 1`。

**顺带一个重要结论**：`return std::move(x);` 在 C++17 里**不会更快**，反而可能更慢——
它把原本可能被 NRVO 消除的具名返回值变成了一个真实的移动。返回值直接用变量名即可。

如果你想亲眼看到"取消消除之后会多出移动"，可以在编译时加 `-fno-elide-constructors`（会关闭 NRVO/部分消除）：

```bash
g++ -std=c++17 -fno-elide-constructors p3_1_handle_test.cpp -I../../tests -o p3_1_noelide && ./p3_1_noelide
```

注意：C++17 里"返回 prvalue"的消除是语言规定，`-fno-elide-constructors` 也关不掉；
它影响的是 NRVO 这类允许被消除的情形，所以上面的命令可能全部通过、也可能在某些编译器上让 NRVO 测试点失败——
把它当作一个实验，不作为通过标准。

### 1.5 迭代器协议：`*` / `++` / `--` / `begin` / `end`

`iterator.cpp` 实现了一个双向链表迭代器。完整要点：

- **`operator*` 决定"取出来是什么"**。src 里返回 `int`（拷贝），所以 `*it = 5` 改不到链表；
  想要能改，必须返回 `int&`，同时提供 `const int& operator*() const` 给 const 场景用。
- **前缀 `++it` 返回 `DLLIterator&`**（改完自己再把自己交出去），**后缀 `it++` 返回旧值的拷贝**。
  后缀的实现一定是"先存下 `*this`，再 `++*this`，最后返回存下的那个"。
  因此前缀通常比后缀高效（后缀必须多一次拷贝），循环里优先用 `++it`。
- **`==` / `!=` 比较的是"指向同一个位置"**，一般是比较内部的节点指针。
- **`Begin()` 是第一个元素，`End()` 是"最后一个元素之后"（past-the-end）**，不是最后一个元素。
  `Begin() == End()` 表示容器为空。

**`--End()` 为什么是个坑**：src 的 `End()` 返回 `curr_ == nullptr`。如果 `operator--` 天真地写
`curr_ = curr_->prev_;`，那么第一次 `--End()` 时 `curr_` 是空指针，解引用直接段错误。
`std::list` 之所以能 `--end()`，是因为它内部有哨兵节点。我们这里没有哨兵，所以
**迭代器必须额外记住尾节点**（构造时带上 `tail_`），`operator--` 里判断"当前是 past-the-end 就回到 `tail_`"。
这正是 P3.2 的核心考点。

**range-for 的原理**：`for (auto& v : dll) { ... }` 会被编译器展开成对 `dll.begin()` / `dll.end()` 的调用。
所以除了大写的 `Begin()/End()`，还要提供小写的成员函数 `begin()` / `end()`（名字必须精确）。
`for (auto& v : dll)` 想改到元素，`operator*` 必须返回引用。

### 1.6 Rule of 0 / 3 / 5

一个类如果管理资源（裸 `new`、文件句柄……），编译器默认生成的**拷贝操作是"逐成员浅拷贝"**，
这会让两个对象持有同一份资源，析构时重复释放。这就引出了三条规则：

- **Rule of 3**：如果你需要手写析构函数，那么你几乎一定也需要手写**拷贝构造**和**拷贝赋值**（都做深拷贝）。
  三个函数要么都不写，要么一起写。
- **Rule of 5**：在 Rule of 3 基础上，如果类型可以移动，再补上**移动构造**和**移动赋值**（都 `noexcept`）。
  一共 5 个特殊成员函数。移动的语义是"偷走资源并把源置空"，成本 O(1) 而不是 O(n)。
- **Rule of 0**：更好的做法是——**让成员变量自己把所有权管好**，你一个特殊成员函数都不用写。
  比如让唯一 owner 是一个 `std::unique_ptr`，`unique_ptr` 自己删拷贝、自己实现移动，
  于是外层类自动变成"不可拷贝、可移动、析构自动释放"。

P3.3 会让你把同一件事用 4 种写法各实现一遍（隐式浅拷贝的 `NaiveList`、`List3`、`List5`、`List0`），
用分配/释放计数器比较它们的行为差异。

**Rule of 0 里最容易错的地方**：只把"头指针"换成 `unique_ptr` 是**不够的**。链表要能自动级联释放，
必须让**每个节点自己拥有它的后继**（成员写成 `std::unique_ptr<N> next;`）。
如果 `head_` 是 `unique_ptr` 而 `next` 还是裸指针，析构时只删头节点，后面整条链全泄漏。
P3.3 有一个 `rule_of_0_destruction_is_cascading` 测试点专门抓这个。

### 1.7 命名空间、include guard、匿名命名空间

- **命名空间**给名字划分作用域，避免冲突：`mylib::Add` 和 `other::Add` 是两个不同的函数。
- **include guard**（`#pragma once` 或 `#ifndef/#define/#endif`）保证头文件在同一个编译单元里
  被重复包含时只展开一次，否则类/函数会被重复定义。
- **匿名命名空间**（`namespace { ... }`）里的函数/常量只有**内部链接**，只在本 `.cpp` 可见，
  不会污染外部符号表。它比 C 风格的 `static` 函数更现代，也可以用来放常量。
- **不要在头文件里写 `using namespace std;`**：头文件会被别人包含，等于把整个 `std` 塞进别人的作用域，
  容易造成名字冲突。这也是 BusTub 的风格：全程写 `mylib::Add` 这样的完整前缀。

---

## 2. 主线题目（贴合 src 三课）

### P3.1 RAII 资源句柄 + RVO / copy elision

- **考什么**：RAII、只可移动的资源管理类、self-move、moved-from 的析构安全，以及观察拷贝消除（1.4）。
- **你要写**：`projects/mysol/stage3/p3_1_handle.h`。只写头文件。
- **复制过来的测评文件**：`tests/stage3/p3_1_handle_test.cpp`。测评点数：11。

**测评程序会用到的接口（名字必须一致）：**

```cpp
class Handle {
public:
  explicit Handle(int id);          // 记录 id
  ~Handle();                        // 只有仍然有效时才释放资源、更新计数
  int  Id() const;
  bool Valid() const;               // moved-from 之后为 false
  static void ResetStats();         // 清空统计
};
inline Handle MakeHandle(int id);   // 按值返回一个 Handle
```

下面这些要你自己补（每一条都对应一个观测点）：

- 4 个公开静态计数器 `live` / `acquires` / `releases` / `moves`：含义与声明方式见 1.3；
  构造、析构、移动分别该动哪几个计数器，自己想清楚。
- **拷贝构造、拷贝赋值必须禁止**（一个资源只能有一个拥有者）。
- **移动构造、移动赋值必须提供且标 `noexcept`**；移动赋值里要有 self-move 防护。

**为什么是这些签名：**

- 四个静态计数器 + `ResetStats()` 是测评的观测点（见 1.3）。`live` 用来验证不泄漏/不重复释放；
  `moves` 用来验证 RVO（1.4）；`acquires/releases` 用来验证 acquire 和 release 一一对应。
- **`Valid()` 很关键**：移动之后源对象不应再释放资源，否则同一个资源会被释放两次。
  析构里要“只有还有效时才更新计数”。测评的 `moved_from_object_destructs_safely` 就在查这个。
- 移动操作必须 `noexcept`：测评有 `static_assert(std::is_nothrow_move_constructible_v<Handle>)`。
- 移动赋值里要有 self-move 防护（`this == &other` 直接返回）：否则会先释放自己的资源再接管自己，
  对自己已经释放的资源再操作。测评有 `self_move_assign_is_safe`。
- `MakeHandle` 按值返回，且实现要能触发 C++17 的保证消除（返回 prvalue 时零移动）。见 1.4。

**要做的事**：实现 `Handle` 与 `MakeHandle`，让 acquire/release 配对、moved-from 安全、移动计数正确。

**测评点在查什么**：构造后有效；出作用域 `live` 回落；拷贝被禁；移动转移并让源失效；
moved-from 析构安全；移动赋值释放旧资源；self-move 安全；`MakeHandle` 零移动且只 acquire 一次；
显式移动计 1 次；1000 个句柄无泄漏；链式移动后只剩一个存活。

**编译运行**：

```bash
cd projects/mysol/stage3
g++ -std=c++17 p3_1_handle_test.cpp -I../../tests -o p3_1 && ./p3_1
```

通过标准：`Result: 11/11 Passed`。

**自查**：① 为什么 C++17 里 `auto h = MakeHandle(42);` 可以零移动？② `return std::move(x);` 会有什么副作用？③ 移动后 `Valid()` 为 false 的意义是什么？

---

### P3.2 双向迭代器（修好 `--End()`）+ range-for

- **考什么**：迭代器协议、`*`/`++`/`--` 的前后缀语义、past-the-end 的处理、range-for 的接口要求。
- **你要写**：`projects/mysol/stage3/p3_2_iterator.h`。只写头文件。
- **复制过来的测评文件**：`tests/stage3/p3_2_iterator_test.cpp`。测评点数：17。

**测评程序会用到的接口（名字必须一致）：**

（先自己设计一个节点类型 `Node`：至少要能存一个 `int` 值以及前驱/后继指针，成员名由你决定。）

```cpp
class DLLIterator {
public:
  DLLIterator(Node* curr, Node* tail);   // 除了当前节点，还要带上尾节点（为了 --End()）
  int&       operator*();                // 返回引用，*it 可读可写
  const int& operator*() const;
  DLLIterator& operator++();             // 前缀
  DLLIterator  operator++(int);          // 后缀
  DLLIterator& operator--();             // 前缀；curr_ 为 nullptr 时回到 tail_
  DLLIterator  operator--(int);          // 后缀
  bool operator==(const DLLIterator& o) const;
  bool operator!=(const DLLIterator& o) const;
  Node* Raw() const;                     // 测评用来观察"当前是不是 nullptr"
};

class DLL {
public:
  void InsertAtHead(int v);              // 头插
  DLLIterator Begin();                   // 第一个元素
  DLLIterator End();                     // past-the-end
  DLLIterator begin();                   // 给 range-for 用（小写）
  DLLIterator end();
  size_t Size() const;
  bool   Empty() const;
  int    Front() const;                  // 头节点值
  int    Back() const;                   // 尾节点值
};
```

**为什么是这些签名：**

- 构造函数要接收 `tail_`：见 1.5，`--End()` 需要它来"从 past-the-end 回到尾节点"。
  如果你更愿意用哨兵节点实现，也可以，但必须满足同样的外部行为。
- `operator*` 返回引用：测评有 `*it = 99;` 并检查 `dll.Front() == 99`，返回拷贝会失败。
  const 版本给 `const DLLIterator` 用。
- 前缀 `operator++` / `operator--` 返回 `DLLIterator&`，测评会用 `auto& ref = ++it; &ref == &it;` 检查；
  后缀返回旧值，测评用 `auto old = it++; *old` 检查。
- `Raw()` 是测评专用的观察口，返回当前节点指针（`End()` 时为 `nullptr`），照着提供即可。
- `DLL` 里 `tail_` 必须维护（头插第一个节点时它既是头也是尾）；`Front()/Back()` 分别返回头/尾节点的值。
- `begin()/end()` 是小写成员函数，range-for 依赖它们。见 1.5。
- `DLL` 持有裸 `new` 出来的节点，析构要遍历删除。它**不应该可拷贝**（默认浅拷贝会 double free，
  这正是 P3.3 的主题）；测评不会拷贝它，所以你删掉拷贝/移动即可。

**要做的事**：实现双向链表和它的双向迭代器。注意头插顺序：依次 `InsertAtHead(n)` … `InsertAtHead(1)`
之后，从头到尾是 `1, 2, ..., n`。

**测评点在查什么**：空链表 `Begin()==End()`；单元素；正向遍历顺序；前缀/后缀产生同样序列；
后缀返回旧值、前缀返回引用；`*it` 可改；const 解引用只读；`--End()` 回到尾节点（不崩）；
单元素 `--End()`；完整反向遍历；中间 `--`；后缀 `--`；range-for 可读可改；
10 万元素正反向和一致；两个独立迭代器互不干扰。

**编译运行**：

```bash
cd projects/mysol/stage3
g++ -std=c++17 p3_2_iterator_test.cpp -I../../tests -o p3_2 && ./p3_2
```

通过标准：`Result: 17/17 Passed`。

**自查**：① `++it` 为什么比 `it++` 高效？后缀为什么必须先存 `*this`？② 为什么 `End()` 需要尾节点信息？③ `operator*` 返回 `int&` 和返回 `int` 有什么区别？

---

### P3.4 命名空间 mini 库（Namespace + `.h`/`.cpp` + include guard + 匿名命名空间）

- **考什么**：命名空间组织、头文件/实现文件分工、include guard、匿名命名空间的内部链接。
- **你要写的目录结构**（放在 `projects/mysol/stage3/` 下）：
  - `mylib/geometry.h`、`mylib/geometry.cpp`
  - `mylib/stats.h`、`mylib/stats.cpp`
- **复制过来的测评文件**：`tests/stage3/p3_4_mylib/p3_4_mylib_test.cpp`（放到 `mysol/stage3/`，
  它会 `#include "mylib/geometry.h"` 两次，用来验证 include guard）。测评点数：10。

**测评程序要求：**

```cpp
// mylib/geometry.h
namespace mylib {
  int Add(int a, int b);
  int Sub(int a, int b);   // 内部用匿名命名空间里的助手实现
  int Abs(int x);
}
namespace other {
  int Add(int a, int b);   // 与 mylib::Add 同名、不同命名空间
}

// mylib/stats.h
#include <vector>
namespace mylib {
  double    Average(const std::vector<int>& v);   // 空 vector 返回 0.0
  long long Sum(const std::vector<int>& v);       // 64 位累加，防溢出
}
```

**为什么是这些接口（测评会核对的具体行为）：**

- `mylib::Add(2,3) == 5`；`mylib::Abs(-9) == 9`；`mylib::Sub(2,5) == 3`（即 `Sub` 返回两数之差的绝对值）。
- `other::Add(2,3)` 要返回与 `mylib::Add` **不同**的结果（测评断言两者不相等），
  参考实现是"两数之和再加一个匿名命名空间里的常量 7"，你可以自行设计，只要满足：
  名字在 `other` 命名空间、结果与 `mylib::Add(2,3)` 不同。
- `mylib::Average({1,2,3}) == 2.0`、空 vector 返回 `0.0`、`{1,2}` 得 `1.5`（不能整数截断）、负数正确。
- `mylib::Sum` 要能处理 100 万个 `1e9`（和是 `1e15`，超过 `int` 范围），所以必须用 `long long` 累加。
- `.h` 要有 include guard（`#pragma once` 或宏），因为测评故意重复 include 了 `geometry.h`。
- `geometry.cpp` 里要有一个只在内部使用的助手（匿名命名空间或 `static`），被 `Sub`/`Abs` 调用。
- 测评文件里**没有** `using namespace mylib;`，全程用完整前缀，这是 BusTub 风格。

**要做的事**：建立上述 4 个文件，实现全部函数，并正确划分声明/实现。

**测评点在查什么**：`mylib::Add`；两个命名空间的同名函数不冲突；匿名命名空间助手被用到但没导出；
`Average` 基本/空/浮点/负数；`Sum` 的 64 位不溢出；头文件可重复包含；全程完整前缀可用。

**编译运行**（注意要把你的两个 `.cpp` 一起编译）：

```bash
cd projects/mysol/stage3
g++ -std=c++17 p3_4_mylib_test.cpp mylib/geometry.cpp mylib/stats.cpp -I../../tests -o p3_4 && ./p3_4
```

通过标准：`Result: 10/10 Passed`。

**自查**：① include guard 防的是什么？② 匿名命名空间的函数和 `static` 函数有什么区别？③ 为什么头文件里不要写 `using namespace std;`？

---

## 3. 扩展题目

### P3.3 Rule of 0/3/5（把隐藏的 double free 揪出来）

- **考什么**：隐式浅拷贝的危险、深拷贝、移动语义、以及用 `unique_ptr` 成员实现 Rule of 0（见 1.6）。
- **你要写**：`projects/mysol/stage3/p3_3_rule_of_three_five_zero.h`。只写头文件。
- **复制过来的测评文件**：`tests/stage3/p3_3_rule_of_three_five_zero_test.cpp`。测评点数：12。

> 这个头文件是"同一件事的 4 种写法对照"，所以内容比别的题多。测评程序用 `using namespace r3;`
> 引入你定义的名字，并且会直接访问下面列出的成员，名字必须一致。

**测评程序会用到 `namespace r3` 里的这些名字（成员名必须一致）：**

- 计数器：`g_allocs`、`g_frees`（两个全局变量，每次 `new` 节点 / 析构节点各 +1）和 `ResetCounters()`。
  它们该怎么声明见 1.3。
- `NaiveList`（反面教材）：
  - 一个**公开的成员 `head`**（头指针）——测评会直接读写它；
  - `Push(int)`：头插一个新节点；节点要有一个**公开的 `int` 成员，名字必须是 `v`**（测评读 `b.head->v`）；
  - **故意不写拷贝构造/拷贝赋值**，让编译器生成浅拷贝；析构要遍历删除多少个节点就删多少个。
- `List3`（Rule of 3）：
  - `Push(int)`；
  - `const N* Head() const`：返回头节点指针——节点要有**公开的 `int v` 和 `next`**（测评读 `Head()->v`、`Head()->next`）；
  - **手写析构 + 拷贝构造 + 拷贝赋值**：拷贝时逐节点新建（深拷贝），赋值要注意自赋值。
- `List5`（Rule of 5）：
  - 在 `List3` 的基础上再**手写移动构造和移动赋值**（标 `noexcept`，赋值里做 self-move 防护）；
  - `N* Head() const`（非 const 版本）。
- `List0`（Rule of 0）：
  - **用智能指针让成员自己管好所有权**：链表只用一个智能指针作为唯一 owner，并且**每个节点用智能指针拥有它的后继**；
  - `Push(int)`、`int Head() const`（空链表返回 -1）、`bool Empty() const`、`size_t Size() const`；
  - **一个特殊成员函数都不要写**。

**为什么这样设计（每一版在演示什么）：**

- `NaiveList` 故意不写任何拷贝控制。`NaiveList b = a;` 会逐成员拷贝 `head`，于是 `a.head == b.head`，
  两个对象都以为自己拥有这条链，析构时各删一遍 → `double free`。测评为了不真的崩，
  在检查完 `a.head == b.head` 后手工把 `a.head = nullptr` "抢救"了一下。
- `List3` 手写深拷贝：拷贝时为新对象重新 `new` 出一条独立的链。测评通过 `b.Head() != a.Head()`
  和"分配了 4 个节点"确认确实是深拷贝。
- `List5` 加上移动：移动只偷头指针并把源置空，成本 O(1)；移动赋值要先释放自己的旧链、再做 self-move 防护。
  测评有 `static_assert` 检查移动是 `noexcept`、拷贝仍可用。
- `List0` 是 Rule of 0：唯一 owner 是 `std::unique_ptr<N> head_`，并且**每个节点用 `unique_ptr` 拥有后继**。
  这样拷贝自动被删除、移动自动可用、析构自动级联。见 1.6 的警告。
  测评通过 `g_allocs == g_frees` 确认 1000 个节点全被释放。
- 所有节点的构造 `++g_allocs`、析构 `++g_frees`，测评靠这两个计数器判断有没有泄漏/重复释放。

**要做的事**：实现 `r3` 命名空间里的全部内容，重点是把 4 种写法的行为差异做出来。

**测评点在查什么**：浅拷贝共享节点；浅拷贝若不抢救会 double free（用计数式场景佐证）；
Rule of 3 深拷贝独立、拷贝赋值独立、两个对象析构不重复释放；Rule of 5 移动转移且源为空、self-move 安全、
无泄漏、移动 `noexcept`；Rule of 0 自动 move-only、不用写特殊成员函数即可用、1000 节点级联析构。

**编译运行**：

```bash
cd projects/mysol/stage3
g++ -std=c++17 p3_3_rule_of_three_five_zero_test.cpp -I../../tests -o p3_3 && ./p3_3
```

（可选）亲眼看看 double free 长什么样——这个参数会让 `NaiveList` 真的浅拷贝并析构：

```bash
./p3_3 --demo-double-free     # 预期输出 free(): double free detected → 进程 abort
```

通过标准：不带参数运行时 `Result: 12/12 Passed`。

**自查**：① 默认拷贝构造把 `head` 怎么样了，为什么 double free？② Rule of 0 生效的前提是什么？③ 什么时候你会**故意**用裸 `Node*` 而不是 `unique_ptr<Node>`？
