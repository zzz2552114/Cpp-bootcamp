# Stage 2 · C++ 模板

> **先读完这两课再做题**
> 1. `src/2 - C++ Templates/templated_functions.cpp` —— 函数模板、显式模板实参、特化、非类型参数
> 2. `src/2 - C++ Templates/templated_classes.cpp` —— 类模板、多类型参数、类模板特化、非类型参数
>
> 本阶段主线考的是模板的两件基本事：**编译器按类型自动生成代码**、**类型推导与显式特化**。
> 另外两件事 src 只带过或没讲——`constexpr` 编译期计算（本阶段用来算阶乘、数组长度）、
> 以及"模板为什么必须写在头文件里 + 显式实例化 + CTAD"，放在第 1 节讲清后再做。

---

## 0. 本阶段题目一览

| 题号 | 主题 | 类型 | 你要写的文件（放 `projects/mysol/stage2/`） | 测评点数 |
| :--: | :-- | :--: | :-- | :--: |
| P2.1 | 模板函数（`Min`/`Max`/`MinC`/`ArrayLen`） | 主线 | `p2_1_functions.h` | 11 |
| P2.2 | 模板类 `Stack<T>`（拷贝/移动双 Push） | 主线 | `p2_2_stack.h` | 12 |
| P2.3 | 特化、非类型参数、`constexpr if` | 主线+扩展 | `p2_3_specialization.h` | 15 |
| P2.4 | 模板头文件化、显式实例化、CTAD | 扩展 | `minimath/min.h` + `minimath/min.cpp` | 8 |

所有题目都**只写头文件，不要写 `main()`**（`main()` 由测评程序提供）。

---

## 1. 做题之前必须懂的几件事

### 1.1 测评程序怎么用（回顾）

和 Stage 1 完全一样：`projects/solutions/stage2/*_test.cpp` 是测评程序，**已经自带 `main()`**。
你只写头文件，**不要写 `main()`**。

`projects/solutions/test_util.h` 里的宏负责注册和断言（`BT_TEST` / `BT_CHECK` / `BT_CHECK_EQ` /
`BT_CHECK_THROWS` / `BT_MAIN`），完整说明见 `stage1.md` 第 1.1 节。这里只重复操作要点：

1. 在 `projects/mysol/stage2/` 下写你的 `.h`；
2. 把对应的测评文件复制到同一个目录（`#include "..."` 会优先找测评文件所在目录，不复制就会用到参考解答）；
3. 在 `projects/mysol/stage2/` 下编译，用 `-I../../solutions` 找到 `test_util.h`。

```bash
# 只做一次：建目录并复制本阶段测评文件
mkdir -p projects/mysol/stage2
cp projects/solutions/stage2/p2_1_functions_test.cpp            projects/mysol/stage2/
cp projects/solutions/stage2/p2_2_stack_test.cpp                projects/mysol/stage2/
cp projects/solutions/stage2/p2_3_specialization_test.cpp       projects/mysol/stage2/
cp projects/solutions/stage2/p2_4_minimath/p2_4_minimath_test.cpp projects/mysol/stage2/

# 每道题的编译方式（以 P2.1 为例）
cd projects/mysol/stage2
g++ -std=c++17 p2_1_functions_test.cpp -I../../solutions -o p2_1 && ./p2_1
```

### 1.2 函数模板：不是"一个函数"，是"造函数的图纸"

`template <typename T> T add(T a, T b) { return a + b; }` 本身**没有生成任何代码**。
只有当你调用 `add(3, 5)` 时，编译器才用 `T = int` 把这张图纸"实例化"成一份真正的函数。
调用 `add(2.8, 3.7)` 会实例化出另一份 `T = double` 的版本。所以：

- `add<int>` 和 `add<double>` 是**两个不同的函数**，各有各的地址；
- 类型由实参**推导**：`T` 从两个实参同时推导，两者必须一致。`add(3, 3.2)` 里一个推 `int`、一个推 `double`，
  推导失败 → 编译错误。想通过就显式写 `add<double>(3, 3.2)`，或让参数类型不同（如 `template<typename A, typename B>`）。

**按值传参 vs 按引用传参会影响拷贝次数**：`T add(T a, T b)` 会把两个实参各拷一份；
`T Min(const T& a, const T& b)` 只借别名、不拷贝。P2.1 会用拷贝计数器验证你的形参是 `const T&` 而不是按值。

### 1.3 `constexpr` 与「编译期计算」

`constexpr` 加在函数上，表示"如果实参都是编译期常量，这个函数就能在编译期算完"。
编译期算完的结果可以放在 `static_assert` 里，也可以用来定义数组大小等**必须是常量**的地方。

```cpp
constexpr int k = 3 + 4;         // 编译期就有值
static_assert(k == 7);           // 编译期检查，不通过就不给编译
```

这与 Stage 1 的 `static_assert`（1.7 节）配合：P2.1 要求 `MinC` 能在 `static_assert` 里用，
P2.3 要求 `Factorial<10>()` 在编译期算完并用于 `static_assert`。

**编译期递归**（P2.3 会用到）：模板可以自己调用自己，配合一个"终止特化"实现循环。

```cpp
template <size_t N> constexpr size_t Factorial() { return N * Factorial<N - 1>(); }
template <>        constexpr size_t Factorial<0>() { return 1; }   // 终止条件：N = 0
```

`Factorial<3>()` 会被展开成 `3 * Factorial<2>()` → `3 * 2 * Factorial<1>()` → … → `3*2*1*1`，
整个展开在编译期完成。

### 1.4 非类型模板参数：把"值"带进类型

模板参数不一定是一种类型，也可以是一个**编译期常量值**：

```cpp
template <typename T, size_t N> constexpr size_t ArrayLen(const T (&)[N]) { return N; }
template <int T> struct Constant { static constexpr int value = T; };
template <size_t N> struct FixedArray { int a[N]; constexpr size_t Size() const { return N; } };
```

调用 `ArrayLen(arr)` 时，`N` 由数组类型 `T[N]` 直接推导出来（**这就是为什么形参要写成数组引用
`const T(&)[N]`**：写成 `const T*` 的话数组会退化成指针，`N` 就丢了）。
`FixedArray<5>` 和 `FixedArray<6>` 是**两个不同的类型**，`a` 的大小在编译期就定下来了。

### 1.5 `if constexpr`（C++17）：把分支选择搬到编译期

普通 `if` 的两个分支**都要能编译**；`if constexpr` 里条件为假的分支**根本不会被实例化**，
所以即使它里面的代码对这个类型不合法，也没关系。

```cpp
template <typename T> std::string ToString(const T& v) {
  if constexpr (std::is_integral_v<T>) {
    return std::to_string(v);          // 只有整型才会实例化这一段
  } else {
    return "non-numeric";              // 其它类型走这里；上面那段不参与编译
  }
}
```

这解决了"用一个模板处理多种类型、每种类型用不同代码"的问题，比运行时 `if` 更安全：
运行时 `if` 的另一个分支可能引用该类型没有的操作（比如给 `std::string` 调 `std::to_string`），从而编译失败。

### 1.6 函数模板的全特化

`templated_functions.cpp` 里 `print_msg<float>` 就是全特化：给某个具体类型单独写一份实现。

```cpp
template <typename T> std::string Describe(const T&);              // 通用版
template <> std::string Describe<double>(const double&);           // 只对 double
template <> std::string Describe<const char*>(const char* const&); // 只对 const char*
```

两个容易踩的点：

- **特化必须能对上形参类型**。通用版形参是 `const T&`，当 `T = const char*` 时形参类型是
  `const char* const&`，所以特化的括号里要写对。
- **字符串字面量推导出的不是 `const char*`**。`Describe("hi")` 里 `T` 推导成 `char[3]`（数组！），
  不会命中 `const char*` 特化，会走通用版。想命中得先存成 `const char*` 变量，或显式写 `Describe<const char*>("hi")`。

### 1.7 模板为什么必须写在头文件里；显式实例化

普通函数可以"声明放 `.h`、实现放 `.cpp`"：编译器为每个 `.cpp` 生成目标文件，链接器再把调用和实现接起来。

模板不行。编译器看到 `Min(3, 5)` 时，**必须当场看到 `Min` 的函数体**，才能为 `T = int` 生成代码。
如果函数体在另一个 `.cpp` 里，这个 `.cpp` 只知道自己被谁调用、不知道 `Min` 长什么样，就无法生成
`Min<int>`；链接时就会报 `undefined reference to 'int Min<int>(...)'`。

所以模板的**实现**要跟着**声明**一起放在头文件里，被每个用到它的编译单元看到。

**显式实例化**是例外手段：如果你确实想把模板实现藏在 `.cpp` 里、只对固定的几种类型提供服务，
可以在 `.cpp` 里写

```cpp
template int Min<int>(const int&, const int&);   // 提前生成 Min<int> 这一份代码
```

这样别的编译单元就能链接到这一份。P2.4 会同时演示普通 `.h/.cpp` 分工和显式实例化。

### 1.8 CTAD（类模板实参推导，C++17）

C++17 之前，用类模板必须写全 `std::vector<int> v{1,2,3};`。C++17 起，如果构造函数能从实参推出模板参数，
就可以省略：

```cpp
std::vector v{1, 2, 3};      // 推出 std::vector<int>
std::pair   p{1, 2.0};       // 推出 std::pair<int, double>
```

对你自己的类模板也一样：`Box b(42);` 能推出 `Box<int>`，前提是类里有一个**能由实参推导出 `T` 的构造函数**
（比如 `explicit Box(T v)`）。如果没有任何这样的构造函数，就必须显式写 `Box<int> b(...)`。

---

## 2. 主线题目（贴合 src 两课）

### P2.1 模板函数（Templated Functions）

- **考什么**：函数模板的实例化、类型推导、`const T&` 形参、非类型模板参数。
- **你要写**：`projects/mysol/stage2/p2_1_functions.h`。只写头文件。
- **复制过来的测评文件**：`solutions/stage2/p2_1_functions_test.cpp`。测评点数：11。

**测评程序要求你提供 4 个函数模板：**

```cpp
template <typename T> T Min(const T& a, const T& b);   // 返回较小者
template <typename T> T Max(const T& a, const T& b);   // 返回较大者
template <typename T> constexpr T MinC(const T& a, const T& b);   // 编译期可用的 Min
template <typename T, size_t N> constexpr size_t ArrayLen(const T (&)[N]);  // 返回 N
```

**为什么是这些签名：**

- `const T&` 形参：测评用一个带拷贝计数的类型测过——`Min(a, b)` 全程只应发生 1 次拷贝
  （那是"按值返回 `T`"产生的），如果形参写成 `T`（按值），就会变成 3 次。见 1.2。
- `MinC` 必须 `constexpr`：测评在 `static_assert(MinC(3, 5) == 3)` 里用它，不是 `constexpr` 就编译不过。见 1.3。
- `ArrayLen` 的形参必须是**数组引用** `const T(&)[N]`：这样 `N` 才能从实参推导出来。见 1.4。
- `Min` / `Max` 要能用于 `int`、`double`、`std::string` 以及任何定义了 `operator<` 的自定义类型
  （测评用了 `Money`、`Tracked`）。这不需要你特判，只要用 `a < b` 比较即可。

**要做的事**：实现这 4 个模板。注意 `Min(3, 3.2)` 会因为两个实参推出不同的 `T` 而编译失败——这是正确行为，
测评用一套测评侧的探测工具确认"它确实不能被推导"，你不需要为此写任何东西。

**测评点在查什么**：`int`/`double`/`string`/自定义类型的比较；`Min<int>` 与 `Min<double>` 是不同函数；
`Min(3, 3.2)` 推导失败而 `Min<double>(3, 3.2)` 成立；`MinC` 能进 `static_assert`；
`ArrayLen` 对 `int[7]`/`double[13]`/`char[1]` 都返回正确长度；`Min` 的形参不是按值。

**编译运行**：

```bash
cd projects/mysol/stage2
g++ -std=c++17 p2_1_functions_test.cpp -I../../solutions -o p2_1 && ./p2_1
```

通过标准：`Result: 11/11 Passed`。

**自查**：① `Min(3,5)` 与 `Min(3.2,1.5)` 是同一个函数吗？② `Min("abc","def")` 会把 `T` 推成什么？③ 什么时候必须写 `Min<int>(...)`？

---

### P2.2 模板类 Stack（Templated Class + 借用 + 移动）

- **考什么**：类模板、把 Stage 1 的 `const&` / `T&&` / `std::move` 用到容器接口上、const 成员函数重载。
- **你要写**：`projects/mysol/stage2/p2_2_stack.h`。只写头文件。
- **复制过来的测评文件**：`solutions/stage2/p2_2_stack_test.cpp`。测评点数：12。

**测评程序要求 `Stack<T>` 提供：**

```cpp
template <typename T>
class Stack {
public:
  void Push(const T& value);   // 左值：拷贝入栈
  void Push(T&& value);        // 右值：移动入栈
  void Pop();                  // 空栈时抛 std::out_of_range
  T&       Top();              // 空栈时抛 std::out_of_range
  const T& Top() const;        // const 版本
  bool   Empty() const;
  size_t Size() const;
};
```

**为什么是这些签名：**

- `Push` 要有两个重载。只留 `const T&`，那么 `Push(Tracked(2))` 这类临时对象会被拷贝而不是移动；
  只留按值的 `Push(T value)`，又无法区分"调用者还想要这个左值"（应拷贝）和"调用者不要了"（可移动）。
  两个重载 + `std::move` 才能让规则清晰。见 1.2 与 Stage 1 的 1.4。
- `Top()` 返回 `T&` 而不是 `T`：返回引用才能 `s.Top() = 99;` 改到真实元素，返回 `T` 只是改副本。
  同时必须有 `const T& Top() const`，const 对象才能读栈顶。
- 空栈的 `Pop()` / `Top()` 行为必须明确。这里**约定抛 `std::out_of_range`**，测评会用
  `BT_CHECK_THROWS(..., std::out_of_range)` 检查。
- 内部用什么容器由你决定；`std::vector<T>` 最省事（别重复造轮子）。测评会关注：
  `Stack<std::unique_ptr<int>>` 能不能用（这要求 `Push(T&&)` 真的移动，而不是拷贝）。

**要做的事**：实现 `Stack<T>`，让左值走拷贝、右值/`std::move` 走移动；空栈操作抛 `std::out_of_range`。

**测评点在查什么**：新建为空、push/top/pop 的 LIFO 顺序、`Top()` 可修改、const 版本只读、
空栈抛异常、左值 push 计数为 1 次拷贝、临时对象与 `std::move` 零拷贝、`unique_ptr` 可用、
`std::string` 语义、10 万元素、交替操作后 size 正确。

**编译运行**：

```bash
cd projects/mysol/stage2
g++ -std=c++17 p2_2_stack_test.cpp -I../../solutions -o p2_2 && ./p2_2
```

通过标准：`Result: 12/12 Passed`。

**自查**：① 为什么 `Push` 要两个重载？只留按值传参会怎样？② `Top()` 为什么返回 `T&`，为什么还要 const 版本？③ `Stack<std::unique_ptr<int>>` 靠哪条重载工作？

---

### P2.3 模板特化、非类型参数与 `constexpr if`

- **考什么**：函数模板全特化、类模板特化、非类型模板参数、编译期递归、`constexpr if`。
- **你要写**：`projects/mysol/stage2/p2_3_specialization.h`。只写头文件。
- **复制过来的测评文件**：`solutions/stage2/p2_3_specialization_test.cpp`。测评点数：15。

**测评程序要求你提供：**

```cpp
// ① 函数模板 + 特化
template <typename T> std::string Describe(const T&);              // 通用版返回 "default"
template <> std::string Describe<double>(const double&);          // 返回 "double"
template <> std::string Describe<const char*>(const char* const&); // 返回 "c-string"

// ② 类模板 + 特化
template <typename T> struct Printer { static std::string Name(); };       // "generic"
template <> struct Printer<float>       { static std::string Name(); };    // "float"
template <> struct Printer<std::string> { static std::string Name(); };    // "string"

// ③ 非类型模板参数
template <size_t N> struct FixedArray {
  int a[N];                                  // 测评会直接读写 a
  constexpr size_t Size() const;             // 返回 N
};
template <int T> struct Constant { static constexpr int value = T; };

// ④ 编译期递归
template <size_t N> constexpr size_t Factorial();   // N * Factorial<N-1>()
template <>         constexpr size_t Factorial<0>(); // 1

// ⑤ constexpr if
template <typename T> std::string ToString(const T&);
// 整型/浮点 → std::to_string(v)；其它 → "non-numeric"
```

**为什么是这些签名（以及各自的坑）：**

- 三个 `Describe` 特化：`double` 特化要返回 `"double"`；`const char*` 特化要返回 `"c-string"`。
  注意 `Describe(1.0f)`（`float`）**不会**命中 `double` 特化，会走通用版；`Describe("hi")` 里 `T` 是 `char[3]`，
  也不会命中 `const char*` 特化。见 1.6。
- `Printer<float>` / `Printer<std::string>` 是类模板特化示例，`Name()` 是静态成员函数，直接 `Printer<int>::Name()` 调用。
- `FixedArray<N>::a` 必须是**公开的、名为 `a` 的 `int` 数组**，`Size()` 必须 `constexpr`（测评在 `static_assert` 里用）。
- `Constant<T>::value` 必须是 `static constexpr`，测评用 `Constant<150>::value`、`Constant<-3>::value` 检查负数也支持。
- `Factorial` 靠"递归 + `Factorial<0>` 终止特化"在编译期算完，见 1.3。
- `ToString` 用 `if constexpr` 分流；对 `std::string` 要能编译并返回 `"non-numeric"`，这只有在用 `if constexpr`
  （假分支不实例化）时才成立。见 1.5。

**要做的事**：实现以上 5 组东西。

**测评点在查什么**：`Describe` 对 `int/short` 为 `default`、`double` 为 `double`、`float` 为 `default`、
`const char*` 变量为 `c-string`、字符串字面量与 `std::string` 为 `default`；类模板特化；
`Factorial<0/1/5/10/12>` 编译期正确；`FixedArray<N>` 的大小、默认全 0、N 可很大；`Constant<-3>`；
`ToString` 的整型/浮点/非数值分支。

**编译运行**：

```bash
cd projects/mysol/stage2
g++ -std=c++17 p2_3_specialization_test.cpp -I../../solutions -o p2_3 && ./p2_3
```

通过标准：`Result: 15/15 Passed`。

**自查**：① `Factorial<10>` 是运行期算的还是编译期算的？怎么证明？② `Describe(double)` 用的是"重载"还是"特化"？③ 用普通 `if` 而不是 `if constexpr`，`ToString(std::string)` 还能编译吗？为什么？

---

## 3. 扩展题目

### P2.4 模板定义放头文件 + 显式实例化 + CTAD

- **考什么**：模板的编译单元模型（1.7）、显式实例化、C++17 类模板实参推导（1.8）。
- **你要写**：一个多文件小项目，全部放在 `projects/mysol/stage2/` 下：
  - `minimath/min.h`（`minimath` 命名空间里的**模板声明 + 模板实现**）
  - `minimath/min.cpp`（普通函数的实现，并演示显式实例化）
- **复制过来的测评文件**：`solutions/stage2/p2_4_minimath/p2_4_minimath_test.cpp`（放到 `projects/mysol/stage2/` 下，
  它会 `#include "minimath/min.h"`，所以你的头文件必须正好在 `mysol/stage2/minimath/min.h`）。测评点数：8。

**测评程序要求 `minimath` 命名空间里提供：**

```cpp
// min.h（声明 + 模板实现都在这里）
namespace minimath {
  template <typename T> T Min(const T& a, const T& b);   // 模板：实现必须在头文件里
  int Add(int a, int b);                                 // 普通函数：声明在 .h
  template <typename T> class Box {
  public:
    explicit Box(T v);
    const T& Get() const;
  };
}

// min.cpp
namespace minimath {
  int Add(int a, int b) { return a + b; }                        // 普通函数实现
  template int Min<int>(const int&, const int&);                 // 显式实例化演示
}
```

**为什么这样分工：**

- `Min` 是模板，**实现必须和声明一起放 `min.h`**；一旦挪到 `.cpp`，编译 `p2_4_minimath_test.cpp` 的编译单元
  看不到函数体，就会 `undefined reference to minimath::Min<int>(...)`。见 1.7。
- `Add` 是普通函数，可以按传统方式"声明在 `.h`、实现在 `.cpp`"；测评会调用 `minimath::Add(2,3)`，
  所以编译时要**把 `min.cpp` 一起编进去**（命令见下）。
- `Box<T>` 有一个 `explicit Box(T)` 构造函数，测评用 `minimath::Box b(42);` 检查 CTAD 是否推导出 `Box<int>`，
  用 `minimath::Box b(std::string("hi"));` 检查 `Box<std::string>`。所以构造函数必须能从实参推出 `T`。见 1.8。

**要做的事**：按上面结构建好 `minimath/min.h` 与 `minimath/min.cpp`，实现 `Min`、`Add`、`Box`；
在 `.cpp` 里写一句 `template int Min<int>(const int&, const int&);` 作为显式实例化。

**测评点在查什么**：跨编译单元能用 `Min<int>` 和 `Min<std::string>`；`Add` 从 `.cpp` 链接成功；
`Box` 的 CTAD 推出 `int` 与 `std::string`；`std::vector v{1,2,3}` 推出 `vector<int>`；
`std::pair p{1,2.0}` 推出 `pair<int,double>`；`Box<int>` 显式写法也能用。

**编译运行**（注意要把你的 `min.cpp` 一起编译）：

```bash
cd projects/mysol/stage2
g++ -std=c++17 p2_4_minimath_test.cpp minimath/min.cpp -I../../solutions -o p2_4 && ./p2_4
```

通过标准：`Result: 8/8 Passed`。

**自查**：① 为什么 `Min` 不能把实现放 `.cpp`？（链接时会发生什么）② 显式实例化 `template int Min<int>(...)` 有什么用？③ `Box b(42);` 能推导的前提是什么？
