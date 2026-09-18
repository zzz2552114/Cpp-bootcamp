# Stage 1 · 引用、左值/右值 与移动语义

> **先按顺序读完这三课再做题**
> 1. `src/1 - References and Move Semantics/references.cpp` —— 引用是别名
> 2. `src/1 - References and Move Semantics/move_semantics.cpp` —— 左值 / 右值 / `std::move`
> 3. `src/1 - References and Move Semantics/move_constructors.cpp` —— 移动构造 / 移动赋值 / 删除拷贝
>
> 本阶段**主线的考点只有这三课教过的东西**：引用、`const&`、重载解析、移动构造/移动赋值、moved-from 状态。
> 但测评程序要"看见"你的代码有没有真的移动、有没有真的泄漏，这就需要几样 src 没讲的观测手段。
> 所以下面第 1 节把这几个东西从零讲清楚（它们是什么、为什么要用、怎么工作），第 2、3 节才是题目。

---

## 0. 本阶段题目一览

| 题号 | 主题 | 类型 | 你要写的文件（放 `projects/mysol/stage1/`） | 测评点数 |
| :--: | :-- | :--: | :-- | :--: |
| P1.1 | 引用游乐场（引用 / `const&` / const 成员函数） | 主线+扩展 | `p1_1_statistics.h` | 12 |
| P1.2 | 重载解析三兄弟 `f(T&)` / `f(const T&)` / `f(T&&)` | 主线 | `p1_2_overload.h` | 8 |
| P1.3 | `MoveOnlyBuffer`：拥有堆内存、只可移动 | 主线+扩展 | `p1_3_buffer.h` | 17 |
| P1.4 | `noexcept` 为什么不能省（vector 扩容） | 扩展 | `p1_4_noexcept.h` | 9 |
| P1.5 | 拷贝 vs 移动：把"更快"变成一个数字 | 扩展 | `p1_5_bench.h` | 6 |

所有题目都**只写头文件，不要写 `main()`**（`main()` 由测评程序提供）。

---

## 1. 做题之前必须懂的几件事

### 1.1 测评程序是什么？我到底该写什么？

`projects/solutions/stageN/` 下的 `*_test.cpp` 就是**测评程序**，它已经写好了 `main()`。
它做两件事：

1. 调用你的代码；
2. 用一套断言宏检查结果，最后打印 `Result: N/M Passed`。

你**只需要写 `.h` 头文件**（里面放类型和函数），**绝对不要自己写 `main()`**——
否则一个程序里会出现两个 `main`，链接直接失败。

测评程序里常见的宏（定义在 `projects/solutions/test_util.h`，你不用写、也不用改）：

| 宏 | 作用 |
| :-- | :-- |
| `BT_TEST(套件, 名字) { ... }` | 注册一个测试点；花括号里就是检查逻辑 |
| `BT_CHECK(条件)` | 条件为假就判定这个测试点失败 |
| `BT_CHECK_EQ(a, b)` | 断言 `a == b`，失败时会把两边的值打印出来 |
| `BT_CHECK_THROWS(表达式, 异常类型)` | 断言该表达式抛出指定异常 |
| `BT_MAIN("标题")` | 展开成 `main()`，跑完所有测试点并打印统计 |

例如 `p1_1_statistics_test.cpp` 里写着：

```cpp
BT_TEST(P1_1, basic_1_to_5) {
  Statistics s;                                  // 用你的类型
  for (int x : {1, 2, 3, 4, 5}) s.AddValue(x);   // 调你的接口
  BT_CHECK_EQ(s.Sum(), 15);                      // 检查结果
}
BT_MAIN("Test Suite p1_1_statistics_test.cpp")   // 这里才有 main
```

所以你的任务本质是：**让测评程序里出现的每一个名字、每一种用法都能编译、且结果正确**。

**头文件必须和测评文件放在同一个目录**。原因是 `#include "p1_1_statistics.h"` 这种双引号包含，
编译器会**优先在测评文件自己所在的目录**里找头文件；如果你把自己写的头文件放在别处、
而参考解答还躺在 `solutions/stage1/`，编译器就会悄悄用参考解答，你的代码根本没被编译。

因此本阶段统一这样做（每道题都要先把你自己的实现文件建好，测评文件复制过来）：

```bash
# ① 建工作目录，并把本阶段 5 个测评文件复制进来（只需做一次）
mkdir -p projects/mysol/stage1
cp projects/solutions/stage1/p1_1_statistics_test.cpp \
   projects/solutions/stage1/p1_2_overload_test.cpp \
   projects/solutions/stage1/p1_3_buffer_test.cpp \
   projects/solutions/stage1/p1_4_noexcept_test.cpp \
   projects/solutions/stage1/p1_5_bench_test.cpp \
   projects/mysol/stage1/

# ② 在 projects/mysol/stage1/ 下写好 p1_1_statistics.h 之后：
cd projects/mysol/stage1
g++ -std=c++17 p1_1_statistics_test.cpp -I../../solutions -o p1_1 && ./p1_1
```

`-I../../solutions` 的作用：让测评文件能找到 `test_util.h`。
`-std=c++17` 必不可少（本仓库统一用 C++17）。除此以外不需要别的编译参数。

### 1.2 引用、`const T&` 与 const 成员函数

`references.cpp` 已经演示了引用是别名：`int& b = a;` 之后 `b` 和 `a` 是同一块内存，改 `b` 就是改 `a`。

做题要用到的两条推论：

- **引用的形参不拷贝**。`void f(const Statistics& s)` 传的是别名，函数里读的是调用者那个对象本身，
  所以不会触发拷贝构造、也不会修改调用者（因为 `const`）。
- **`const T&` 能绑定右值**。`const int& x = 10;` 合法，编译器会为 `10` 造一个临时对象并把引用绑上去，
  这也是为什么 `void f(const T&)` 既能接左值又能接临时对象。而 `int& x = 10;` 非法。

**const 成员函数**：成员函数后面写 `const`（如 `int Sum() const`），函数体内的 `this` 类型变成
`const Statistics*`。这意味着：

- 它只能读成员、调用别的 const 成员函数，不能改成员；
- const 对象只能调用 const 成员函数。

`references.cpp` 里 `add_three(int&)` 这类非 const 形参（能改到调用者）与后面 P1.1 里 `AddValue`（非 const，改对象）/ `Sum() const`（只读）的搭配，就是这个规律的体现。

### 1.3 左值 / 右值，以及 `std::move` 到底做了什么

- **左值**：有名字、能取地址的表达式。变量、`*p`、`v[0]`、返回 `T&` 的函数调用。
- **右值**：临时的、马上销毁的表达式。字面量 `10`、`a + b`、返回 `T` 的函数调用、`std::move(x)` 的结果。

`std::move(x)` **不是**一个"搬运函数"。它做的事只有一件：把 `x` 这个表达式**强制转换成右值引用**
（`static_cast<T&&>(x)` 的意思）。转换完它就不管了。真正"把资源搬走"的动作，发生在**移动构造/移动赋值**里。
所以：

- 对一个**没有移动构造**的类型调 `std::move`，什么都不会快，它只是变成了一个可以绑定到 `T&&` 的表达式；
- `std::move` 之后原对象**不会自动清空**，它是否被清空，取决于移动构造/赋值有没有把源置空；
- moved-from 对象**仍然活着**，处于"合法但值未指定"的状态：可以安全析构、可以重新赋值，别依赖它原来的内容。

`move_semantics.cpp` 里 `move_add_three_and_print(std::move(int_array2))` 之后 `int_array2` 被掏空，
而 `add_three_and_print(std::move(int_array3))` 里函数内没有把 `vec` 移给任何左值，所以 `int_array3` 依然可用——
这两段对比的就是"`std::move` 只是 cast，移动与否看函数内部"。

### 1.4 拷贝构造 vs 移动构造：编译器怎么选

给定一个"正在用某个表达式初始化/赋值一个同类型对象"的场景，编译器按实参的值类别选择：

| 实参 | 选中的构造函数 |
| :-- | :-- |
| 左值 `a` | 拷贝构造 `T(const T&)` |
| 右值 `std::move(a)`、临时对象 | 移动构造 `T(T&&)` |

因此**只要类型定义了移动构造**，`T b = std::move(a)` 才真的会走移动。
特别地：一旦你自己写了拷贝构造，编译器就**不再隐式生成移动构造**（这是 `move_constructors.cpp` 之后最容易踩的坑），
所以想保留移动，必须显式写 `T(T&&) noexcept` 或 `= default`。

移动构造/赋值写完，**必须把源对象置于可安全析构的状态**：通常是把它内部的指针/句柄置空。
否则两个对象都以为自己拥有同一份资源，析构时就会二次释放（`double free`）。

### 1.5 怎么"看见"拷贝和移动：静态计数器

测评程序跑的时候你看不到对象内部发生了什么，所以需要一个**能被断言**的观测点。
最常用的办法是给类型加一个**静态数据成员**当计数器。

**什么是静态数据成员**：写在类里、带 `static` 的变量。它**不属于某个对象，而属于整个类**——
不管你创建多少实例，它只有一份，所有实例共用。普通成员是"每个对象一份"，静态成员是"整个类一份"。
正因为只有一份，测试才能在任意时刻读它、检查它。

**为什么这里要写成 `inline static int copies = 0;`**：

- 不加 `inline` 时，静态数据成员通常要在类外的某个 `.cpp` 里再定义一次。
  而我们的代码全在头文件里，头文件会被多个编译单元包含，那个类外定义就会出现多份 → 链接报
  `multiple definition`。
- C++17 起允许在类内直接 `inline static` 定义并初始化静态数据成员，**全程序只有一份**，
  正好适配"实现都写在头文件里"的场景。

**怎么用它**：让"拷贝"这件事发生时把计数器 +1，移动时不加。这样测试就能断言：

- 通过 `const&` 借用时 `copies == 0`（证明没拷贝）；
- 按值传参时 `copies >= 1`（证明发生了拷贝）；
- 移动时 `copies == 0`（证明走的是移动）。

P1.1 要求 `Statistics` 里有一个公开的 `inline static int copies`，P1.4 / P1.5 也各自需要
`copies` / `moves` 计数器，用途完全相同：把"看不见的拷贝/移动"变成"能断言的数字"。

### 1.6 怎么确认"没有内存泄漏"

如果类型自己 `new` 了内存，就要保证每个 `new` 都有对应的 `delete`。最直接的自检方式是**再放一对计数器**：

- 每次 `new[]` 时 `allocs` +1；
- 每次 `delete[]` 时 `frees` +1；
- 测试结束时断言 `allocs - frees == 0`。

泄漏的两种典型成因，正是 P1.3 要练的：

1. **移动赋值**里忘了先释放自己原有的资源 → 旧的那块内存再也无人释放；
2. **移动后没有把源置空** → 源和被移动到的对象都持有同一指针，析构两次 → `double free`。

想更权威地确认，可以用 AddressSanitizer（ASan）：编译时加 `-fsanitize=address`，
运行时会自动报告越界访问、use-after-free、内存泄漏。命令见 P1.3。

### 1.7 编译期检查：`static_assert` 和 `<type_traits>`

测评里有些"检查"是**编译期**完成的，写法是 `static_assert(条件, "信息")`。
它检查的类型性质来自 `<type_traits>`，常用的有：

| 写法 | 含义 |
| :-- | :-- |
| `std::is_nothrow_move_constructible_v<T>` | 移动构造是不是 `noexcept` |
| `std::is_copy_constructible_v<T>` | 类型能不能拷贝构造 |
| `std::is_same_v<A, B>` | `A` 和 `B` 是不是同一个类型 |

如果你看到编译错误是 `static assertion failed`，那说明你的**函数签名**和规格不一致，
典型原因是：漏写了 `noexcept`，或者忘了用 `= delete` 禁止拷贝。

### 1.8 `noexcept` 为什么不能省

`std::vector` 扩容时要把旧元素搬到新内存。标准库用 `move_if_noexcept` 规则决定怎么搬：

- 移动构造是 `noexcept` → 放心**移动**（快）；
- 移动构造**没有** `noexcept`，但类型还能拷贝 → 为了异常安全，**改走拷贝**（慢）；
- 移动构造没有 `noexcept`、拷贝又被删掉 → 没得选，只能移动。

所以你在 BusTub 里会看到 `Page`、`Tuple` 的移动构造一律标 `noexcept`。P1.4 会让你把这个差异数出来。

---

## 2. 主线题目（贴合 src 三课）

### P1.1 引用游乐场（Reference Playground）

- **考什么**：引用是别名、`const&` 形参不拷贝、const 成员函数、`const&` 能绑右值。
- **你要写**：`projects/mysol/stage1/p1_1_statistics.h`。只写头文件，不要写 `main()`。
- **把哪个测评文件复制过来**：`solutions/stage1/p1_1_statistics_test.cpp`。
- **测评点数**：12。

**测评程序会用到你类型的这些东西，所以规格必须满足：**

```cpp
struct Statistics {
  inline static int copies = 0;          // 观测拷贝次数，见 1.5
  std::vector<int> data;                 // 测评会直接访问 s.data（reserve/assign 造大数据）

  Statistics() = default;
  Statistics(const Statistics&);                    // 拷贝：data 拷过来，copies +1
  Statistics& operator=(const Statistics&);         // 拷贝赋值：同上
  Statistics(Statistics&&) noexcept = default;      // 移动：不增加 copies
  Statistics& operator=(Statistics&&) noexcept = default;

  void AddValue(int x);                  // 改自身 → 非 const 成员函数
  int Sum() const;                       // 只读；返回 int 和
  long long SumL() const;                // 只读；返回 64 位和
  double Average() const;                // 只读；空数据返回 0.0
  size_t Size() const;                   // 只读
  bool Empty() const;                    // 只读
};

inline int ReportSum(const Statistics& s);   // 通过 const& 借用
```

**为什么是这些签名（逐条解释）：**

- `data` 必须**公开且叫 `data`**：测评用 `s.data.reserve(1000000)` 和 `s.data.assign(...)` 直接构造大数据集，
  这是测评的硬性要求，不是风格建议。
- `copies` 必须是 `inline static`：理由见 1.5。测评通过 `Statistics::copies` 读取它。
- **拷贝构造/拷贝赋值必须手写**：只有手写，才能在拷贝时 `++copies`。也正因为你手写了拷贝构造，
  编译器不会再自动生成移动构造，所以**必须显式写出移动构造/移动赋值**（这里 `= default` 即可），
  否则 `Statistics b(std::move(a))` 会退化成拷贝、`copies` 就会不为 0。
- 移动构造/赋值标 `noexcept`：理由见 1.8；测评里有 `static_assert` 会检查。
- `Sum()` 返回 `int`、`SumL()` 返回 `long long`：测评用 `static_assert(std::is_same_v<decltype(s.Sum()), int>)`
  检查返回类型，并用 1..1000000 的数据验证 `SumL()` 不溢出。
- `Average()` 必须先把和转成 `double` 再除，并且**用 `SumL()` 而不是 `Sum()`**：
  否则 1..1000000 的和会先在 `int` 里溢出，平均值就错了；空数据要返回 `0.0`（不是 NaN，也不能除零）。
- `AddValue` 不能是 const：它要修改对象。
- `ReportSum` 接收 `const Statistics&`：这是"借用、不拷贝"的示例；测评会断言调用它之后 `copies` 仍为 0。

**要做的事**：把上面的类型和自由函数实现出来；行为细节以"测评点在查什么"为准：

- 空对象 `Sum()==0`、`Average()==0.0`；
- `AddValue` 真的改到原对象（`Size()` 增加）；
- `Average()` 是浮点结果（`{1,2}` 得 1.5，不是整数除法截断的 1.0）；
- 负数正确；
- `const Statistics s = ...;` 能调用 `Sum/Average/Size`；
- 100 万个 1..1e6 时 `SumL()` 与 `Average()` 正确；
- 移动不增加 `copies`。

**编译运行**：

```bash
cd projects/mysol/stage1
g++ -std=c++17 p1_1_statistics_test.cpp -I../../solutions -o p1_1 && ./p1_1
```

通过标准：`---------------- Result: 12/12 Passed ----------------`。

**自查**：① 为什么 `ReportSum(const Statistics&)` 不会让 `copies` 增加，而按值传参会？② 为什么 `Average()` 要用 `SumL()`？③ const 对象为什么调不了 `AddValue`？

---

### P1.2 重载解析三兄弟（Overload Resolution）

- **考什么**：同一组实参，值类别（左值 / const 左值 / 右值）不同，会选中不同的重载。
  这是理解 `std::move` 为什么有用的关键。
- **你要写**：`projects/mysol/stage1/p1_2_overload.h`。只写头文件。
- **把哪个测评文件复制过来**：`solutions/stage1/p1_2_overload_test.cpp`。
- **测评点数**：8。

**测评程序要求你提供这 4 个函数：**

```cpp
inline std::string Which(int& x);         // 实参是非 const 左值 → 返回 "lvalue ref"
inline std::string Which(const int& x);   // 实参是 const 左值   → 返回 "const lvalue ref"
inline std::string Which(int&& x);        // 实参是右值         → 返回 "rvalue ref"

// 内部把一个非 const 左值看成 const 左值，再调用 Which
inline std::string CallWithConst(int& x);
```

**为什么返回字符串而不是打印**：测评要断言"选中了哪一个"，返回标签字符串才能被 `BT_CHECK_EQ` 比较。

**要做的事**：写三个 `Which` 重载，各自返回对应标签。`CallWithConst(int& x)` 里先建立
`const int& cx = x;`（把左值"降级"成 const 左值），再 `return Which(cx);`。

**测评点在查什么**：非 const 变量选 `int&`；const 变量和 `CallWithConst` 选 `const int&`；
字面量 `10`、`1+2`、`std::move(a)` 选 `int&&`；`std::move(a)` 之后 `a` 本身仍是左值；
`const int c; Which(std::move(c))` 得到的是 `const int&&`，它绑不到 `int&&`，只能退到 `const int&`。

**编译运行**：

```bash
cd projects/mysol/stage1
g++ -std=c++17 p1_2_overload_test.cpp -I../../solutions -o p1_2 && ./p1_2
```

通过标准：`Result: 8/8 Passed`。

**自查**：① `Which(std::move(a))` 之后 `a` 还是左值吗？② 为什么 `Which(std::move(c))`（`c` 是 const）不走 `int&&`？③ 若删掉 `Which(int&)`，`Which(a)` 会选谁？

---

### P1.3 MoveOnlyBuffer（拥有堆内存、只可移动）

- **考什么**：移动构造、移动赋值、self-move、moved-from 的析构安全、只可移动类型的用途。
- **你要写**：`projects/mysol/stage1/p1_3_buffer.h`。只写头文件。
- **把哪个测评文件复制过来**：`solutions/stage1/p1_3_buffer_test.cpp`。
- **测评点数**：17。

**测评程序要求你的类提供这些接口：**

```cpp
class Buffer {
public:
  inline static int allocs = 0;   // 每次申请内存 +1（观测用，见 1.6）
  inline static int frees  = 0;   // 每次释放内存 +1

  explicit Buffer(size_t n);      // 申请长度为 n 的动态数组，元素初始化为 0
  ~Buffer();                      // 释放

  Buffer(const Buffer&) = delete;
  Buffer& operator=(const Buffer&) = delete;

  Buffer(Buffer&& other) noexcept;
  Buffer& operator=(Buffer&& other) noexcept;

  size_t Size() const noexcept;
  bool   Owns() const noexcept;             // 当前是否持有内存
  int&       operator[](size_t i);          // 不做越界检查
  const int& operator[](size_t i) const;
  int&       At(size_t i);                  // 越界抛 std::out_of_range
  const int& At(size_t i) const;
};
```

**为什么是这些签名：**

- `explicit Buffer(size_t n)`：`explicit` 防止 `Buffer b = 10;` 这种把整数悄悄当长度用的隐式转换。
  元素要**初始化为 0**（测评会逐个检查），所以申请时要记得做值初始化。
- 拷贝用 `= delete`：一个资源只能有一个拥有者；允许拷贝就会有两个对象持有同一块内存，析构两次即 `double free`。
  这也是 `wrapper_class.cpp` 里 `IntPtrManager` 的做法。
- 移动构造/赋值必须 `noexcept`：见 1.8；测评有 `static_assert` 检查，而且 `vector<Buffer>` 的测试依赖它。
- 移动赋值里必须有 **self-move 防护**（`this == &other` 时直接返回）：否则 `b = std::move(b)` 会先释放自己的内存，
  再把自己那个已经被释放的指针赋给自己，析构时二次释放。测评专门有一个 `self_move_assign_is_safe` 测试点。
- `Owns()`：区分"长度为 0 但已申请"和"资源已被移动走"。`Buffer b(0)` 时 `Owns()` 应为真。
- `operator[]` 不检查、`At` 检查并抛 `std::out_of_range`：这是 STL 的惯例（`vector[]` vs `vector::at`），
  测评两个版本（const 与非 const）都会用。

**要做的事**：实现上面全部接口。内存申请/释放时维护 `allocs`/`frees`，
保证移动赋值**先释放自己的旧内存**、移动后**把源置空**。

**测评点在查什么**：构造后长度、元素全 0、读写、const 访问与 `At` 抛异常、长度为 0 仍算持有；
移动构造后资源转移且源被掏空；moved-from 对象能安全析构；移动赋值释放旧资源（用 `allocs/frees` 差值验证）；
self-move 安全；链式移动；大量移动后 `allocs == frees`（无泄漏）；1e7 大数组；移动操作 `noexcept`；能放进 `std::vector`。

**编译运行**：

```bash
cd projects/mysol/stage1
g++ -std=c++17 p1_3_buffer_test.cpp -I../../solutions -o p1_3 && ./p1_3
```

（可选）想让它替你检查越界和泄漏，可以用 AddressSanitizer 再跑一遍：

```bash
g++ -std=c++17 -fsanitize=address,undefined -g p1_3_buffer_test.cpp -I../../solutions -o p1_3_asan && ./p1_3_asan
```

通过标准：`Result: 17/17 Passed`。

**自查**：① 移动赋值若不先释放旧内存，哪个测试点会失败、失败信息长什么样？② 为什么移动操作必须 `noexcept`？③ `Buffer b(0)` 的 `Owns()` 为什么是 true？

---

## 3. 扩展题目（对应第 1.5 ~ 1.8 节）

### P1.4 `noexcept` 为什么不能省略（实验）

- **考什么**：`vector` 扩容时的 `move_if_noexcept` 规则（1.8），以及 `noexcept` 如何影响类型在容器里的行为。
- **你要写**：`projects/mysol/stage1/p1_4_noexcept.h`。只写头文件。
- **把哪个测评文件复制过来**：`solutions/stage1/p1_4_noexcept_test.cpp`。
- **测评点数**：9。

**测评程序要求你提供 3 个类型和 1 个函数模板：**

```cpp
struct Counted {                       // 移动构造【带】noexcept
  inline static int copies = 0;
  inline static int moves  = 0;
  int v;
  explicit Counted(int x);
  Counted(const Counted& o);           // ++copies
  Counted(Counted&& o) noexcept;       // ++moves
  static void Reset();                 // 两个计数器都清零
};

struct CountedThrowy {                 // 移动构造【不写】noexcept，但拷贝还在
  // 同上：copies / moves / Reset / explicit 构造 / 拷贝构造 / 移动构造
};

struct MoveOnlyThrowy {                // 移动构造不写 noexcept，且拷贝被删除
  inline static int moves = 0;
  int v;
  explicit MoveOnlyThrowy(int x);
  MoveOnlyThrowy(const MoveOnlyThrowy&) = delete;
  MoveOnlyThrowy& operator=(const MoveOnlyThrowy&) = delete;
  MoveOnlyThrowy(MoveOnlyThrowy&& o);  // 故意不写 noexcept
  static void Reset();
};

// 建 vector<T>，依次 push_back T(0)..T(n-1)，【不要】reserve，逼它反复扩容
template <typename T> void GrowVector(int n);
```

**为什么这样设计**：三个类型的唯一区别就是"移动构造有没有 `noexcept`、拷贝还在不在"。
把它们放进同一个 `GrowVector` 里跑，就能看出扩容时标准库到底选了移动还是拷贝：
`Counted` 全程移动、`CountedThrowy` 被迫拷贝、`MoveOnlyThrowy` 因为不能拷贝只能移动。
`moves`/`copies` 计数器（见 1.5）就是证据。

**要做的事**：实现这三个类型（构造时把 `v` 存下来；拷贝/移动时各自给计数器 +1）和 `GrowVector`。

**测评点在查什么**：三种类型是否 `is_nothrow_move_constructible`；`Counted` 扩容零拷贝；
`CountedThrowy` 扩容出现拷贝；先 `reserve` 再 push 时拷贝为 0、移动恰好等于元素数；
`MoveOnlyThrowy` 仍能编译运行；大数组零拷贝；扩容后元素值正确。

**编译运行**：

```bash
cd projects/mysol/stage1
g++ -std=c++17 p1_4_noexcept_test.cpp -I../../solutions -o p1_4 && ./p1_4
```

通过标准：`Result: 9/9 Passed`。

**自查**：① `CountedThrowy` 为什么既有拷贝又有移动？② 先 `reserve` 后 `push_back` 为什么 `copies == 0` 且 `moves == 元素数`？③ 把 P1.3 的 `Buffer` 移动构造的 `noexcept` 去掉，`vector<Buffer>` 会怎样？

---

### P1.5 拷贝 vs 移动：把"更快"变成一个数字

- **考什么**：`std::vector` 的拷贝是"逐个元素拷贝"，而移动是"只偷缓冲区"——把性能直觉变成可断言的计数。
- **你要写**：`projects/mysol/stage1/p1_5_bench.h`。只写头文件。
- **把哪个测评文件复制过来**：`solutions/stage1/p1_5_bench_test.cpp`。
- **测评点数**：6。

**测评程序要求你提供：**

```cpp
struct Tracked {
  inline static int copies = 0;
  inline static int moves  = 0;
  int v;
  explicit Tracked(int x);
  Tracked(const Tracked& o);          // ++copies
  Tracked(Tracked&& o) noexcept;      // ++moves
  static void Reset();
};

// 返回 n 个长度为 64、内容全是 'x' 的字符串
inline std::vector<std::string> MakeLongStrings(size_t n);

// 累加 vector 中所有字符串的长度
inline size_t TotalLength(const std::vector<std::string>& v);

// 累加 vector 中所有 Tracked 的 v
inline long long SumValues(const std::vector<Tracked>& v);
```

**为什么长度取 64**：`std::string` 对很短的字符串会直接存在对象内部（SSO，小字符串优化），
不产生堆分配，拷贝和移动的差别就看不出。长度 64 超过 SSO 阈值，字符串内容在堆上，
拷贝要真的复制 64 字节，移动只需偷指针——差异才明显。

**要做的事**：实现 `Tracked`（拷贝/移动各自计数）和三个自由函数。
`MakeLongStrings` 可以返回一个"填充了 n 个同样长字符串"的 vector。

**测评点在查什么**：拷贝整个 `vector<Tracked>` 时 `copies == n`（每个元素一次）；
`std::move` 整个 vector 时 `copies == 0 && moves == 0`（只偷缓冲区，连元素都不用移动）；
移动后内容完整；对长字符串同样成立；不同规模下都成立；moved-from 的 vector 可以重新赋值使用。
另有一个计时测试点只打印参考数据，不参与严格的性能判定。

**编译运行**：

```bash
cd projects/mysol/stage1
g++ -std=c++17 p1_5_bench_test.cpp -I../../solutions -o p1_5 && ./p1_5
```

通过标准：`Result: 6/6 Passed`。

**自查**：① 为什么 `vector` 的移动不调用任何元素的移动构造？② 为什么用 64 个字符而不是 3 个？③ moved-from 的 vector 为什么还能用？
