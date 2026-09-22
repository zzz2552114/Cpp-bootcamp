# Stage 3 · 包装类 / 迭代器 / 命名空间

> **先读完这三课再做题**
> 1. `src/3 - Misc/wrapper_class.cpp` —— RAII：资源生命周期绑定对象生命周期
> 2. `src/3 - Misc/iterator.cpp` —— 自己实现迭代器（`*`、`++`、`==`、`Begin/End`）
> 3. `src/3 - Misc/namespaces.cpp` —— 命名空间、`using`、嵌套命名空间
>
> 主线考 RAII、迭代器协议、命名空间组织。另外几件事 src 没细讲，但测评要用、而且很值得懂：
> **拷贝消除 / RVO**（P3.1 要观察"按值返回资源管理对象时发生了几次构造"）、
> **Rule of 0/3/5**（P3.3 要把隐式浅拷贝导致的 double free 一步步拆开）、
> 以及 **跨文件使用命名空间**（src 的 `namespaces.cpp` 只讲了单文件里的用法，而 P3.4 要求你写一个
> 多文件小库）。这几块都放在第 1 节。
>
> **阅读约定**：从 1.3 节开始，凡是从 src 里没有细讲、但本阶段题目会用到的知识点，都按同一个格式写：
> **是什么** → **为什么需要它（它解决了什么问题）** → **常见用法** → **什么时候用** → **一个跟本题无关的例子**。
> 凡是一行就能写完、写了就等于把答案送给你的东西，都写成"**必须自己想清楚、自己补上**"，
> 把要求说清、把代码留给你写。

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

**是什么**：`inline static` 的静态数据成员 + 静态 `ResetStats()`。带 `static` 的成员属于**整个类**
（所有实例共用一份，见 stage1 第 1.5 节），所以测试可以在任意时刻读它、比较它。

**为什么需要它（它解决了什么问题）**：测试没法直接看见对象内部发生了什么——
“移动了没有”“拷贝了几次”“有没有泄漏”都是隐藏行为。加一个全类共享、可随时读的计数器，
就把这些隐藏行为变成了**可断言、可打印的数字**。`live`（进入前后差值）是验证"无泄漏、无重复释放"
最直接的手段。

**常见用法**：

```cpp
class Counted {
 public:
  inline static int ctor = 0;
  inline static int copies = 0;
  static void ResetStats() { ctor = copies = 0; }
  // 拷贝构造里 ++copies，构造里 ++ctor ……
};
```

**什么时候用**：只要题目要求"证明某操作发生/没发生几次"就用它：
stage1 的 P1.1/P1.4/P1.5、本阶段 P3.1、stage4 的 P4.4、stage5 的 P5.2/P5.3/P5.4、stage7 的 P7.1 都在用。

**为什么要 `inline`**：实现都在头文件里，头文件被多个编译单元包含时，
类外定义的静态成员会出现多份 → 链接报 `multiple definition`；
C++17 的 `inline static` 类内定义保证了"全程序只有一份"（详见 stage1 第 1.5 节）。

**一个跟本题无关的例子**：数一个简单类被默认构造了多少次，用来验证 `std::vector` 在你预期的时候
才重新构造元素（而不是每次 `push_back` 都构造额外副本）。

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

**是什么**：拷贝消除（copy elision）指编译器/语言规则让"返回值"**直接构造在目标位置**，
中间不产生临时对象、也不调用拷贝/移动构造。

- **保证的拷贝消除（C++17 起）**：用 prvalue（纯右值，如 `Handle(id)`、`T(args)`）初始化对象时，
  对象**就在目标位置就地构造**。这是语言语义，不是优化，`-O0` 下也成立。
- **NRVO / RVO（允许但非保证）**：返回**具名局部变量**（`Handle tmp; ...; return tmp;`）时，
  编译器**允许**把它直接构造在调用者位置；没消除的话会调用一次移动构造（因为返回局部变量被当作右值）。

**为什么需要它（它解决了什么问题）**：

1. 让"按值返回大对象"不再是一场性能灾难：没有它，每次返回都要拷一遍；
2. C++17 起，"返回一个不可拷贝也不可移动的类型"也能编译（只要返回的是 prvalue）——
   这把"返回工厂"写法从"必须可移动"里解放了出来；
3. 它是"返回值优化"从"编译器自由"变成"语言保证"的一步。

**常见用法**：工厂函数 `Handle MakeHandle(int id) { return Handle(id); }`；
运算符 `Vec operator+(const Vec&, const Vec&) { return Vec(...); }`；
任何"函数内部造一个对象、返回给调用者"的写法。

**什么时候在意它**：

- 你在写"按值返回资源管理对象"的代码时（P3.1）；
- 你在用计数器验证"到底发生了几次移动"时（本节 1.3）；
- **绝大多数时候你什么都不用做**。尤其是**不要画蛇添足写 `return std::move(x);`**：
  那会把本来可能被 NRVO 消除的具名返回变成一次真实移动，只会更慢、不会更快。

**一个跟本题无关的例子**：

```cpp
std::vector<int> MakeZeros(int n) { return std::vector<int>(n, 0); }   // prvalue → 就地构造
std::vector<int> v = MakeZeros(5);   // 只构造一次 vector
```

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

**补充：`iterator_traits` 与迭代器分类（标准库怎么"认识"你的迭代器）**

**是什么**：`std::iterator_traits<It>` 是一个模板，把迭代器的"五件套"暴露成类型：
迭代器分类 `iterator_category`（`input` / `forward` / `bidirectional` / `random_access`）、
元素类型 `value_type`、距离类型 `difference_type`（一般是 `std::ptrdiff_t`）、`pointer`、`reference`。

**为什么需要它**：标准算法（`std::distance`、`std::advance`、`std::copy`、`std::sort`……）需要根据
迭代器能力选择不同实现（能 `it + n` 就随机跳，只能 `++` 就一步步走），它们统一通过 `iterator_traits` 查询。
没有它，算法就不知道你的迭代器能不能 `--`、能不能随机访问。

**常见用法**：自己写的迭代器可以让 `std::iterator_traits` 认出它。两种做法：
在类里写嵌套 typedef（老写法），或者特化 `std::iterator_traits`（新写法，参考实现用的这个）：

```cpp
namespace std {
template <> struct iterator_traits<DLLIterator> {
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  using pointer = int*;
  using reference = int&;
};
}  // namespace std
```

**什么时候用**：想让自己的迭代器能被标准算法/`std::distance` 等使用，或想在自定义容器上支持
更多算法时。range-for 本身只要求 `begin()/end()`、`!=`、`++`、`*`，**不强制** traits；
P3.2 的测评也只用手写循环和 range-for，所以 traits 是**可选加分项**——
但 `tests/stage3/p3_2_iterator.h` 的参考实现给了，值得知道有这回事。

**一个跟本题无关的例子**：给一个"只能前进"的迭代器标上 `forward_iterator_tag` 后，
`std::distance(it, end)` 就知道自己只能一步步走，而不会去尝试 `end - it`。

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

**补充：三条规则到底在说什么（什么/为什么/怎么选/何时用）**

**是什么**：关于"管理资源的类该写哪些特殊成员函数"的三条经验规则。特殊成员函数指：析构函数、
拷贝构造、拷贝赋值、移动构造、移动赋值（共 5 个）。“0/3/5”就是“写 0 个 / 写 3 个 / 写 5 个”。

**为什么会出现这些规则**：编译器默认为你生成的拷贝是"逐成员拷贝"（浅拷贝），
对管理资源的类，这会直接导致两个对象持有同一份资源 → 析构时 double free / 泄漏。
但手写全部 5 个又冗长、容易写漏（尤其是漏了 noexcept 或 self-move 防护）。
三条规则就是"什么时候必须手写、什么时候根本不用写"的判据：

| 规则 | 含义 | 什么时候选它 |
| :-- | :-- | :-- |
| **Rule of 3** | 需要手写析构，就几乎一定需要手写拷贝构造 + 拷贝赋值（都做深拷贝） | 成员里有裸拥有的资源，而且要求类型**可拷贝** |
| **Rule of 5** | 在 Rule of 3 基础上再加移动构造 + 移动赋值（都 `noexcept`，移动赋值做 self-move 防护） | 资源转移比深拷贝便宜，想支持 `std::move` |
| **Rule of 0** | 一个特殊成员函数都不写，让**成员自己**管好所有权 | 能用 `unique_ptr` / 容器 / `string` / `shared_ptr` 表达所有权时（**首选**） |

**常见用法**：

- Rule of 3：`List3` 里手写 `~List3`、`List3(const List3&)`、`operator=(const List3&)`；
- Rule of 5：再加 `List5(List5&&) noexcept`、`operator=(List5&&) noexcept`；
- Rule of 0：成员写成 `std::unique_ptr<Node>`，于是拷贝自动被删、移动自动可用、析构自动级联；
- 深拷贝的两种实现思路：递归拷贝、或用"尾指针 `N**`"迭代地把新节点接到末尾（参考实现用的后者）。

**什么时候用哪条**：能 Rule of 0 就 Rule of 0（少写代码、少出错）；
资源必须共享所有权就用 `shared_ptr`（还是 Rule of 0）；
只有当你必须手写资源生命周期、且类型确实要被拷贝/移动时才退到 Rule of 3 / 5。

**一个跟本题无关的例子**（管理一个 `FILE*`）：

```cpp
class File {                       // Rule of 3：需要可拷贝
 public:
  explicit File(const char* path) : fp_(std::fopen(path, "r")) {}
  ~File() { if (fp_) std::fclose(fp_); }
  File(const File&) = delete;      // 文件句柄无法简单共享，禁止拷贝
  File& operator=(const File&) = delete;
  File(File&& o) noexcept : fp_(o.fp_) { o.fp_ = nullptr; }   // Rule of 5 的移动
 private:
  std::FILE* fp_ = nullptr;
};
// 若改成 std::unique_ptr<std::FILE, decltype(&std::fclose)> fp_; → 就变成 Rule of 0。
```

### 1.7 命名空间、include guard、匿名命名空间

这一节的三个东西都是"多文件组织代码"的基本工具，P3.4 会把它们一次用全。

#### 1.7.1 命名空间（namespace）

**是什么**：给名字划一块作用域 / 前缀。写 `namespace mylib { int Add(int, int); }`，
这个函数的**完整名字**就叫 `mylib::Add`。`::` 是作用域解析运算符；
没放进任何命名空间的名字在**全局命名空间**（可用 `::Add` 明确指它）。

**为什么需要它（它解决了什么问题）**

1. **避免命名冲突**。C++ 程序是由很多库拼起来的，`Add`/`Init`/`Node`/`Size` 这种名字到处都有人用。
   放进不同命名空间后 `mylib::Add` 与 `other::Add` 是两个不同的标识符，可以共存——P3.4 会专门验证。
   C++ 标准库把所有东西放进 `std` 正是这个原因（所以是 `std::cout` 而不是 `cout`）。
2. **组织代码**：把逻辑相关的一组类型/函数打包成一个"库"，名字自带归属（比如 `bustub::BufferPool`）。
3. **跨文件拼装同一个库**：同一个命名空间可以在多个文件里**反复打开**，里面的名字累积到一起——
   这就是"声明放 `.h`、实现放 `.cpp`"能协同工作的基础。
4. **控制可见性**：匿名命名空间 / `static` 可以让名字只在本编译单元可见（见 1.7.3）。

**常见用法（含跨文件）**

1) **头文件里声明**，声明包在命名空间里，并且头文件必须有 include guard：

```cpp
// mylib/geometry.h
#pragma once
namespace mylib {
int Add(int a, int b);        // 只是声明
}  // namespace mylib
```

2) **实现文件里"重新打开"同一个命名空间**（推荐写法，可读性最好）：

```cpp
// mylib/geometry.cpp
#include "geometry.h"
namespace mylib {             // 重新打开，不是新建；名字加进同一个 mylib
int Add(int a, int b) { return a + b; }
}  // namespace mylib
```

3) 或者用**限定名定义**（完全等价，少一层缩进）：

```cpp
// mylib/geometry.cpp
#include "geometry.h"
int mylib::Add(int a, int b) { return a + b; }
```

   这里有三条**铁律**，跨文件时每条都会实际咬人：
   - **在哪个命名空间声明，就必须在同一个命名空间里定义**。`namespace mylib { int Add(int,int); }`
     配一个全局的 `int Add(int,int) { ... }` 会变成两个不同函数，链接时报 `undefined reference to mylib::Add`。
   - **同一个命名空间可以分散在多个文件、多个位置**，编译器会拼在一起：`a.h` 里开 `namespace lib {}`、
     `b.cpp` 里再开 `namespace lib {}`，是同一个 `lib`。
   - **不能"跳级"定义**。若声明在 `namespace A { namespace B { int f(); } }`，
     定义必须是 `namespace A { namespace B { int f() {...} } }` 或 `int A::B::f() {...}`，
     不能只开 `namespace A {}` 就定义 `B::f`。

4) **调用时用限定名**：`mylib::Add(2, 3)`。在 `mylib` 内部可以直接写 `Add(2, 3)`（不用前缀）。

5) **`using` 的两种形式**：
   - `using namespace mylib;`：把**整个命名空间**引入当前作用域。方便，但很容易引入意料之外的名字冲突。
   - `using mylib::Add;`：只引入**一个名字**。安全得多，通常写在函数作用域里。
   - `using mm = mylib;` 是**命名空间别名**，给长名字起短名（之后写 `mm::Add`）。

6) **嵌套命名空间**：`namespace A { namespace B { ... } }`；C++17 起可简写为 `namespace A::B { ... }`。

7) **`main` 不进命名空间**：`main` 必须在全局作用域。

8) **`inline namespace`（C++17）**：`inline namespace v2 { ... }` 里的名字可以被外层命名空间直接看到
   （库做版本兼容常用）。知道有这回事即可，本阶段用不到。

**什么时候用**：写库 / 多文件项目时**默认就用**；名字可能和别处冲突时；想把一组相关工具打包时。
单文件小练习里不是必须，但 P3.4 明确要求用。

**一个跟本题无关的例子（一个跨两个文件的 `shapes` 小库）**：

```cpp
// shapes/shapes.h
#pragma once
namespace shapes {
struct Point { double x, y; };
double Distance(const Point& a, const Point& b);
}  // namespace shapes
```

```cpp
// shapes/shapes.cpp
#include "shapes.h"
#include <cmath>
namespace shapes {
double Distance(const Point& a, const Point& b) {
  const double dx = a.x - b.x, dy = a.y - b.y;
  return std::sqrt(dx * dx + dy * dy);
}
}  // namespace shapes
```

```cpp
// main.cpp
#include "shapes/shapes.h"
int main() {
  shapes::Point a{0, 0}, b{3, 4};
  return shapes::Distance(a, b) == 5.0 ? 0 : 1;
}
```

编译：`g++ -std=c++17 main.cpp shapes/shapes.cpp -o demo`。
对照 P3.4：`shapes` 就是 `mylib` / `other`，`Distance` 就是 `Add` / `Sub` / `Abs`。

#### 1.7.2 include guard（头文件保护）

**是什么**：头文件开头的一小段"只展开一次"的防护。两种等价写法：

- `#pragma once`：非标准，但所有主流编译器都支持，最省事；
- `#ifndef MYLIB_GEOMETRY_H_` / `#define MYLIB_GEOMETRY_H_` / `#endif`：标准、可移植，需要保证宏名唯一。

**为什么需要它**：`#include` 本质是**文本替换**。同一个头文件如果被同一个编译单元包含两次
（直接重复包含，或 a.h 和 b.h 都包含它），里面的**类定义、函数定义、变量定义**就会展开两遍，
编译器报 `redefinition of ...` / `multiple definition`。

**常见用法**：

```cpp
// mylib/geometry.h
#pragma once          // 放在文件最前面
namespace mylib { ... }
```

**什么时候用**：**每一个 `.h` / `.hpp` 都要有**；`.cpp` 不需要（它只被编译一次）。
P3.4 的测评故意把 `geometry.h` 包含了两次，就靠这个保证能编译。

**一个跟本题无关的例子**：如果你在自己项目里同时 `#include "a.h"` 和 `#include "b.h"`，
而两者都包含了同一个 `common.h`，没有 include guard 就会在类定义处
报 `redefinition of 'struct Common'`。

#### 1.7.3 匿名命名空间

**是什么**：`namespace { ... }`，没有名字的命名空间。里面的名字只有**内部链接**，
只在本翻译单元（本 `.cpp`）可见。

**为什么需要它**：不想把"只给自己用的"助手函数 / 常量 / 类型暴露出去，以免污染外部符号表、
与别的文件同名冲突、或让别人依赖不该依赖的内部细节。C 时代用 `static` 函数/变量达到同样效果；
匿名 namespace 更现代，而且还能放类型和模板。

**常见用法**：

```cpp
// mylib/stats.cpp
#include "stats.h"
namespace {
constexpr int kPageSize = 4096;                            // 只在本文件可见
long long SumRaw(const std::vector<int>& v) {              // 内部助手
  long long s = 0;
  for (int x : v) s += x;
  return s;
}
}  // namespace

namespace mylib {
long long Sum(const std::vector<int>& v) { return SumRaw(v); }
}  // namespace mylib
```

**什么时候用**：`.cpp` 里只给自己用的工具函数 / 常量 / 类型。
⚠️ **不要写在头文件里**：头文件会被每个包含它的翻译单元各展开一份，等于每个 TU 都有自己的副本，
反而容易出 ODR / 重复定义问题（想写"头文件里的内部工具"应该用 1.7.2 的 include guard 思维，
或者让它成为 `inline` / 模板 / 类成员）。

**和 `static` 函数的区别**：两者都提供内部链接。差别是匿名 namespace 能放**类型、模板、常量**，
而 `static` 只能修饰函数和变量。现代 C++ 推荐匿名 namespace。

**一个跟本题无关的例子**：一个日志模块 `.cpp` 里有个 `FormatTimestamp()` 只在内部用，
放进匿名 namespace 后就不会与另一个模块里同名的 `FormatTimestamp()` 冲突。

#### 1.7.4 头文件里不要写 `using namespace`

**不要**在头文件里写 `using namespace std;` 或 `using namespace mylib;`：头文件会被别人包含，
等于把整个命名空间塞进所有使用者的作用域，非常容易造成名字冲突，而且很难排查。
这正是 BusTub 的风格：头文件全程写 `mylib::Add` / `std::vector` 这种完整前缀，
只有 `.cpp` / 测试文件里为了可读性才可能写 `using namespace ...;`（测评文件就是这么写的）。

---

## 2. 主线题目（贴合 src 三课）

### P3.1 RAII 资源句柄 + RVO / copy elision

- **考什么**：RAII、只可移动的资源管理类、self-move、moved-from 的析构安全，以及观察拷贝消除（1.4）。
- **你要写**：`projects/mysol/stage3/p3_1_handle.h`。只写头文件。
- **复制过来的测评文件**：`tests/stage3/p3_1_handle_test.cpp`。测评点数：11。

**命名空间**：这些名字直接放在**全局命名空间**（测评文件里没有任何 `using namespace`），
所以不要把 `Handle` / `MakeHandle` 再套一层 namespace。

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

下面这些你必须自己想清楚、自己补上（只给要求，不给能直接复制的代码）：

- **内部成员**：`Handle` 至少要存两样东西——① 资源的 id；② 一个"这个对象现在还持不持有资源"的标志
  （moved-from 之后要变成不持有的状态）。它们的类型和名字由你定。
- 4 个公开静态计数器 `live` / `acquires` / `releases` / `moves`：含义与声明方式见 1.3；
  构造、析构、移动分别该动哪几个计数器，自己想清楚（注意：被移动后的源对象析构时**不能**再减 `live`）。
- **拷贝构造、拷贝赋值必须禁止**（一个资源只能有一个拥有者）。
- **移动构造、移动赋值必须提供且标 `noexcept`**；移动赋值里要有 self-move 防护。
- 析构函数里要先判断"是否仍然有效"，有效才更新 `live` / `releases`。

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

**命名空间**：这些名字直接放在**全局命名空间**（测评文件里没有任何 `using namespace`），
所以不要把 `Node` / `DLLIterator` / `DLL` 再套一层 namespace。

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

**下面这些你必须自己想清楚、自己补上（只给要求，不给能直接复制的代码）：**

- **`Node` 的成员**：至少一个 `int` 值、一个前驱指针、一个后继指针；具体类型和名字由你定。
- **`DLLIterator` 的成员**：除了“当前节点指针”，还必须记住**尾节点**（构造函数第二个参数就是它）。
  想清楚：为什么只靠当前节点无法实现 `--End()`？（见 1.5）
- **`DLL` 的成员**：头指针、尾指针、元素个数。想清楚：头插**第一个**节点时尾指针该怎么处理；
  头/尾指针在后面所有操作里要保持什么不变量。
- **`operator--()` 的边界**：`End()` 的当前指针是空，前缀 `--` 里必须先判断“当前是不是空”再决定
  是回到尾节点还是走 `prev`——直接解引用空指针会段错误。
- **后缀 `++`/`--`**：先存一份 `*this`，再改 `*this`，最后返回存下的旧值。
- **析构**：从头遍历到尾逐个 `delete`；并禁用拷贝/移动。
- （可选加分）为 `DLLIterator` 补 `std::iterator_traits` 特化，让它能被标准算法使用（见 1.5）。

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

**下面这些你必须自己想清楚、自己补上（只给要求，不给能直接复制的代码）：**

- **每个 `.h` / `.cpp` 的职责**：`.h` 里只写**声明**（函数原型），`.cpp` 里写**定义**（函数体）。
- **命名空间必须前后一致**：头文件里声明在 `mylib`，`.cpp` 里的定义就必须在 `mylib`（重新打开或限定名），
  否则链接时报 `undefined reference to mylib::...`。见 1.7.1 的三条铁律。
- **include guard**：每个 `.h` 都要有（`#pragma once` 或宏）；想清楚“为什么不加会编译失败”。
- **匿名命名空间**：`Sub` / `Abs` 要复用同一个内部助手（比如取绝对值），这个助手放在
  `.cpp` 里的匿名命名空间里；想清楚它为什么不能被 `#include` 到外面去。
- **`Average` 的除法**：必须先把和转成浮点再除，否则整数除法截断。
- `other::Add` 的结果要与 `mylib::Add(2,3)` 不同，具体怎么造由你定（但不能是常数写死到测试里）。

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

**下面这些你必须自己想清楚、自己补上（只给要求，不给能直接复制的代码）：**

- **`List3`/`List5` 的深拷贝怎么写**：遍历源链，为每个节点 `new` 一个新节点，再接成一条新的链；
  注意新链的尾节点后继必须是空指针。
- **`List5` 的移动赋值**：先做 self-move 防护，再释放自己的旧链，最后偷对方的头指针并把源置空。
- **`List0` 的级联析构前提**：`head_` 用 `std::unique_ptr` **不够**，必须让**每个节点**用 `unique_ptr` 拥有它的后继；
  否则只删头节点，后面整条链全泄漏。见 1.6 的警告。
- **节点计数器**：每个节点在构造时 `++g_allocs`、析构时 `++g_frees`；`ResetCounters()` 把两个都清零。
- **`NaiveList`**：**故意**不写拷贝控制，让它浅拷贝——这是反面教材，不是让你修好它。

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

**自查**：① 默认拷贝构造把 `head` 怎么样了，为什么 double free？② Rule of 0 生效的前提是什么？③ 什么时候你会**故意**用裸 `Node*` 而不是 `unique_ptr<Node>`？④ `List0::Head()` 里的 `Head()` 和 `List3::Head()` 返回类型有什么不同，为什么？
