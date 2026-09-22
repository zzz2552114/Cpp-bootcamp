# Stage 2 · C++ 模板

> **先读完这两课再做题**
> 1. `src/2 - C++ Templates/templated_functions.cpp` —— 函数模板、显式模板实参、特化、非类型参数
> 2. `src/2 - C++ Templates/templated_classes.cpp` —— 类模板、多类型参数、类模板特化、非类型参数
>
> 本阶段主线考的是模板的两件基本事：**编译器按类型自动生成代码**、**类型推导与显式特化**。
> 另外几件事 src 只带过或没讲——`constexpr` 编译期计算（本阶段用来算阶乘、数组长度）、
> "模板为什么必须写在头文件里 + 显式实例化 + CTAD"，以及 **命名空间**（P2.4 要用，
> 但 src 直到第 3 课才正式讲）。这三块都放在第 1 节，按照
> "是什么 → 为什么会有它 → 常见用法 → 什么时候用 → 一个跟本题无关的例子" 的格式讲清楚，再做题。
>
> ⚠️ **为什么第 1 节里会突然出现命名空间**：P2.4 要求你把 `Min` / `Add` / `Box` 放进 `minimath` 命名空间，
> 并且声明放 `.h`、普通函数实现放 `.cpp`。这既是"模板必须头文件化"的练习，也是**跨文件使用命名空间**的练习。
> 第 **1.9 节**会把命名空间（尤其是跨 `.h`/`.cpp` 的用法）完整讲一遍，**先看 1.9 再做 P2.4**。

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

和 Stage 1 完全一样：`projects/tests/stage2/*_test.cpp` 是测评程序，**已经自带 `main()`**。
你只写头文件，**不要写 `main()`**。

`projects/tests/test_util.h` 里的宏负责注册和断言（`BT_TEST` / `BT_CHECK` / `BT_CHECK_EQ` /
`BT_CHECK_THROWS` / `BT_MAIN`），完整说明见 `stage1.md` 第 1.1 节。这里只重复操作要点：

1. 在 `projects/mysol/stage2/` 下写你的 `.h`；
2. 把对应的测评文件复制到同一个目录（`#include "..."` 会优先找测评文件所在目录，不复制就会用到参考解答）；
3. 在 `projects/mysol/stage2/` 下编译，用 `-I../../tests` 找到 `test_util.h`。

```bash
# 只做一次：建目录并复制本阶段测评文件
mkdir -p projects/mysol/stage2
cp projects/tests/stage2/p2_1_functions_test.cpp            projects/mysol/stage2/
cp projects/tests/stage2/p2_2_stack_test.cpp                projects/mysol/stage2/
cp projects/tests/stage2/p2_3_specialization_test.cpp       projects/mysol/stage2/
cp projects/tests/stage2/p2_4_minimath/p2_4_minimath_test.cpp projects/mysol/stage2/

# 每道题的编译方式（以 P2.1 为例）
cd projects/mysol/stage2
g++ -std=c++17 p2_1_functions_test.cpp -I../../tests -o p2_1 && ./p2_1
```

> **阅读约定**：从 1.3 节开始，凡是从 src 里没有细讲、但本阶段题目会用到的知识点，都按同一个格式写：
> **是什么** → **为什么需要它（它解决了什么问题）** → **常见用法** → **什么时候用** → **一个跟本题无关的例子**。
> 如果你看完某个知识点仍然不知道"我该在题目里怎么落笔"，那就是这里没写全，直接说。
> 另外，凡是"一行就能写完、写了就等于把答案送给你"的东西，本阶段不会给代码，而是写成
> "**必须自己想清楚、自己补上**"，把要求说清楚、把代码留给你写。

### 1.2 函数模板：不是"一个函数"，是"造函数的图纸"

`template <typename T> T add(T a, T b) { return a + b; }` 本身**没有生成任何代码**。
只有当你调用 `add(3, 5)` 时，编译器才用 `T = int` 把这张图纸"实例化"成一份真正的函数。
调用 `add(2.8, 3.7)` 会实例化出另一份 `T = double` 的版本。所以：

- `add<int>` 和 `add<double>` 是**两个不同的函数**，各有各的地址；
- 类型由实参**推导**：`T` 从两个实参同时推导，两者必须一致。`add(3, 3.2)` 里一个推 `int`、一个推 `double`，
  推导失败 → 编译错误。想通过就显式写 `add<double>(3, 3.2)`，或让参数类型不同（如 `template<typename A, typename B>`）。

**按值传参 vs 按引用传参会影响拷贝次数**：`T add(T a, T b)` 会把两个实参各拷一份；
`T Min(const T& a, const T& b)` 只借别名、不拷贝。P2.1 会用拷贝计数器验证你的形参是 `const T&` 而不是按值。

**顺带把"类模板"的最小常识补齐**（`templated_classes.cpp` 的主题，P2.2/P2.3/P2.4 都要用）：
类模板是"一族类"的图纸，`Stack<int>` 和 `Stack<std::string>` 是**两个完全不同的类**，各有各的成员和静态变量。

- **成员函数定义在类外**时，必须写"两行模板头 + 带 `<T>` 的限定名"：

```cpp
// 声明（类内）
template <typename T>
class Stack {
 public:
  void Push(const T& v);     // 类内只声明
 private:
  std::vector<T> data_;
};

// 定义（类外）：① 重新写模板头；② 类名后面必须写 <T>
template <typename T>
void Stack<T>::Push(const T& v) { data_.push_back(v); }
```

  漏掉 `template <typename T>`，或者把限定名写成 `Stack::Push`（少了 `<T>`），都会编译报错。
  想省事就把实现直接写在类里（类内定义的成员函数隐式 `inline`），**本阶段题目推荐这么写**。
- **类模板的静态成员**在类外定义时同样要带模板头，例如 `template <typename T> int Stack<T>::count_ = 0;`。
  想在头文件里直接给静态成员初值，用 `inline static`（见 stage1 第 1.5 节）。
- 类模板的成员函数**只有被真正调用时才会检查函数体**：写错的地方可能一直不报错，直到你实例化并调用它才爆出来。
- `typename` 和 `class` 在模板参数列表里等价：`template <class T>` 与 `template <typename T>` 一个意思。
  但**在模板内部引用"依赖于模板参数的类型"时**必须用 `typename`，例如
  `typename std::vector<T>::iterator it;`（`T::value_type`、`std::vector<T>::size_type` 这些都要加 `typename`）。

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

**是什么**：`constexpr` 是 C++11 引入的说明符。加在**变量**上，表示"这是一个编译期常量表达式"；
加在**函数**上，表示"当所有实参都是编译期常量时，这次调用可以在编译期算完"——但同一个函数
在运行期用普通实参调用时，就照常当普通函数执行。

**为什么需要它（它解决了什么问题）**：

1. C++ 里有若干位置**语法上必须**是编译期常量，运行期的值根本放不进去：
   数组长度、非类型模板实参（1.4）、`case` 标签、枚举值、`static_assert` 的条件、`std::array` 的长度……
   有了 `constexpr` 函数，就能用一个"函数"去产生这些常量，而不必手写一堆常量或宏。
2. 把计算从运行期搬到编译期：结果直接内联成一个常量，**运行时零开销**，还能让编译器顺便帮你检查。
3. C++11 之前只能靠 `#define` 宏或手写常量。宏没有类型、不遵守作用域、不能调试、容易写错；
   `constexpr` 是"有类型、有作用域、可调试、可参与重载"的替代品。

**常见用法**：

- 定义编译期常量：`constexpr int kMax = 3 + 4;`、`constexpr double kPi = 3.14159;`。
- 编译期小工具函数：`constexpr int Square(int x) { return x * x; }`——既能进 `static_assert(Square(3) == 9)`，
  也能在运行期 `int y = Square(n);` 用。
- 编译期递归/循环产生常量：`Factorial<N>()` 就是本节例子。
- `static constexpr` 静态成员（P2.3 的 `Constant<T>::value`）：常量属于类型、不占对象空间。
- `constexpr` 成员函数：对象本身是字面量类型时，可以在编译期构造/运算。
- 与 `if constexpr`（1.5）配合做编译期分支。

**什么时候用**：

- 某处**必须**要常量（数组长度、模板实参、`static_assert`）而你不想硬编码；
- 想给一个"又当常量、又当普通函数"的小工具（平方、取最小、阶乘、位运算……）；
- 想让模板做编译期计算（元编程）。

**一个跟本题无关的例子**：

```cpp
constexpr int MaxBufLen() { return 128; }

char buf[MaxBufLen()];                 // 数组长度必须是编译期常量
static_assert(MaxBufLen() == 128);     // 编译期检查

constexpr int Square(int x) { return x * x; }
static_assert(Square(9) == 81);

int n = 6;
int area = Square(n);                  // 同一个函数，这里在运行期执行
```

**容易搞混的两点**：

- `constexpr` 变量**本身就带 `const`**（`constexpr int x = 3;` 之后 `x` 不可改）；反过来，
  `const` 变量**不一定**是 `constexpr`——`const int y = std::rand();` 是 `const`，但它的值编译期不知道。
- 调用点能不能在编译期完成，看的是**实参是不是编译期常量**，不是看函数有没有写 `constexpr`。
- 想强制"只能在编译期调用"，C++20 用 `consteval`；想让静态变量常量初始化，C++20 用 `constinit`。
  本仓库统一 C++17，知道有这回事即可。
- **`constexpr` 函数要有定义可见**：它和模板一样，编译期求值需要看到函数体，所以通常也应该写在头文件里。

### 1.4 非类型模板参数：把"值"带进类型

模板参数不一定是一种类型，也可以是一个**编译期常量值**：

```cpp
template <typename T, size_t N> constexpr size_t ArrayLen(const T (&)[N]) { return N; }
template <int T> struct Constant { static constexpr int value = T; };
template <size_t N> struct FixedArray { int a[N]; constexpr size_t Size() const { return N; } };
```


***这里看不懂的可以去看 [引用数组与constexpr](https://github.com/zzz2552114/Notes/blob/main/cmu15-445/Cpp-bootcamp/P2-%E5%BC%95%E7%94%A8%E6%95%B0%E7%BB%84%E4%B8%8Econstexpr.md)***


***以及同目录下的 P2-对ArrayLen模板函数的解释.md***



调用 `ArrayLen(arr)` 时，`N` 由数组类型 `T[N]` 直接推导出来（**这就是为什么形参要写成数组引用
`const T(&)[N]`**：写成 `const T*` 的话数组会退化成指针，`N` 就丢了）。
`FixedArray<5>` 和 `FixedArray<6>` 是**两个不同的类型**，`a` 的大小在编译期就定下来了。

**是什么**：模板参数不一定是一种类型，也可以是一个**编译期常量值**（整型、枚举、指针、左值引用、
C++17 起还可以是 `auto`）。这种参数叫**非类型模板参数**（non-type template parameter，NTTP）。
`template <typename T, size_t N>` 里的 `N` 就是它。

**为什么需要它（它解决了什么问题）**：

- 它把"长度/容量/索引"这种**值**提升到"类型"层面。`FixedArray<5>` 与 `FixedArray<6>` 是两个不同的类型，
  长度成为类型的一部分，编译器可以在编译期检查越界、可以做特化。
- **零运行期存储、零运行期开销**：长度不需要作为对象成员存一份，编译器直接把 `N` 代进去。
  对比一下"运行时长度"的 `std::vector`：它必须把 size/capacity 存在对象里。
- 让"策略"可被类型系统区分：`RingBuffer<1024>` 与 `RingBuffer<2048>` 不会互相赋值。

**常见用法**：

- 固定大小容器：`std::array<T, N>`、`std::bitset<N>`、自己写的 `FixedArray<N>`；
- 编译期索引访问：`std::get<I>(std::tuple)`、`std::get<I>(std::array)`；
- 编译期常量包装：本阶段的 `Constant<T>::value`；
- 策略/配置参数：`template <int Policy> struct Allocator;`；
- 配合 `if constexpr` / 特化做编译期分支。

**什么时候用**：某个值在编译期就已知、并且你希望它成为类型的一部分时。
如果长度要到运行期才知道（从输入读、从容器算），那就不能用它，应该用 `std::vector` 这类运行期容器。

**一个跟本题无关的例子**：

```cpp
// 编译期把整数取整到 2 的幂（常用于对齐）
template <size_t N>
struct RoundUpPow2 { static constexpr size_t value = RoundUpPow2<(N + 1) / 2>::value * 2; };
template <> struct RoundUpPow2<1> { static constexpr size_t value = 1; };

static_assert(RoundUpPow2<5>::value == 8);
static_assert(RoundUpPow2<9>::value == 16);
```

**两个坑**：

- **能传什么**：C++17 里允许整型、枚举、指针、左值引用、`nullptr_t`、`auto`；**不允许浮点数**
  （`template <double D>` 在 C++17 是非法的，C++20 起才允许浮点与部分类类型）。
- **数组会退化**：非类型参数写成 `N` 时靠的是形参是"数组引用"`const T(&)[N]`。如果你把形参写成
  `const T*` 或 `const T[]`，数组退化成指针，`N` 就没法从实参推出来了——这正是 P2.1 里
  `ArrayLen` 必须写成数组引用的原因。

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

**是什么**：`if constexpr`（C++17）是"条件分支在**编译期**决定"的 `if`。条件必须是编译期常量表达式。
条件为真的那一支会被保留，**为假的那一支根本不会被实例化**（连编译都不编译）。

**为什么需要它（它解决了什么问题）**：

- 普通 `if` 的两个分支**都必须能通过编译**，即使某个分支运行时永远不会走。写泛型代码时会卡死：
  比如 `if (std::is_integral_v<T>) return std::to_string(v);` 对于 `T = std::string`，
  那个 `std::to_string(v)` 分支照样要编译，于是报错。
- 在 C++17 之前，同样的需求要用 SFINAE、`std::enable_if`、tag dispatch、重载等"偏方"，
  代码又长又难读。`if constexpr` 把它们统一成"看起来像普通 if"的写法。
- 它还能用来写递归模板的**终止分支**：`if constexpr (N == 0) { ... } else { ... }`，
  避免为每个 N 都写一个特化。

**常见用法**：

- 按类型分流：`if constexpr (std::is_integral_v<T>) { ... } else if constexpr (std::is_floating_point_v<T>) { ... } else { ... }`；
- 按类型能力分流：`if constexpr (std::is_same_v<T, std::string>) ...`；
- 编译期递归终止：`template <size_t N> void Print() { if constexpr (N > 0) { ...; Print<N-1>(); } }`；
- 与 `std::is_*` 系列 traits（`<type_traits>`）配合，是泛型库的标准写法。

**什么时候用**：模板里"不同类型的处理方式不同、且某些处理方式对该类型根本不合法"时。
如果两个分支对所有类型都合法，用普通 `if` 就行，别为了炫技改 `if constexpr`。

**一个跟本题无关的例子**：

```cpp
template <typename T>
std::string Tag(const T&) {
  if constexpr (std::is_pointer_v<T>) {
    return "pointer";
  } else if constexpr (std::is_array_v<T>) {
    return "array";          // 指针没有 extent，数组有；两个分支互不干扰
  } else {
    return "value";
  }
}
```

**注意**：`if constexpr` 只影响**模板实例化**；在非模板函数里写它，为假的分支仍然要求能编译
（因为没有"实例化"这回事）。所以它基本只出现在模板里。

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


***这里去看 [const指针](https://github.com/zzz2552114/Notes/tree/main/Cpp)***

**是什么**：特化 = 对"某一组具体的模板实参"单独给一份实现，而不再用通用版。
函数模板的**全特化**写法是 `template <>`，后面接具体的函数签名。

**为什么需要它（它解决了什么问题）**：

- 通用实现对某个类型**算不对或效率太差**。最典型的例子：`std::hash<MyType>`、
  `std::numeric_limits<T>`、`std::formatter<T>`——标准库都靠特化让用户/库作者补上"这个类型怎么办"。
- 想让某个类型走一条**完全不同的代码路径**（比如日志里 `const char*` 按字符串打印，
  而其它类型按数值打印）。
- 编译期"查表"：`Factorial<0>`、`RoundUpPow2<1>` 这类终止条件本质就是特化。

**常见用法**：

- 函数模板全特化（本节例子）：`template <> std::string Describe<double>(const double&);`；
- **类模板全特化**：`template <> struct Printer<float> { ... };`（P2.3 会写）；
- **类模板偏特化**（partial specialization）：只固定一部分参数、或对指针/引用等结构做特化，
  例如 `template <typename T> struct Printer<T*> { ... };`、`template <typename T, typename U> struct Pair<T, U*> { ... };`。
  ⚠️ **函数模板不能偏特化**，只能用重载来达到类似效果（这是语言规则）。
- 为类型特化 `std::hash`（stage4 还会遇到）、`std::swap`、`std::numeric_limits` 等。

**什么时候用**：某个（或某类）具体类型需要和通用版不一样的行为时。
**不要**用特化去做"本来一个重载就能解决"的事——函数重载通常比特化更直观、重载决议规则也更好预期。

**一个跟本题无关的例子**：

```cpp
template <typename T> struct Limits { static constexpr T max() { return T(); } };
template <> struct Limits<int>    { static constexpr int max() { return 2147483647; } };
template <> struct Limits<size_t> { static constexpr size_t max() { return static_cast<size_t>(-1); } };

static_assert(Limits<int>::max() > 0);
static_assert(Limits<size_t>::max() > 0);
```

**三个必须知道的坑**：

1. **特化必须写在"第一次使用它"之前**，而且要和主模板在**同一个命名空间**里。
   写在别的命名空间（或写在用之后）编译器不会报"找不到"，而是直接用通用版，非常隐蔽。
2. **形参类型要精确对上**：`const T&` 在 `T = const char*` 时形参类型是 `const char* const&`
   （指针本身也 const 了），特化括号里写错就对不上。
3. **字符串字面量不是 `const char*`**：`Describe("hi")` 推导出的是 `char[3]`（数组），
   命中不了 `const char*` 特化。要命中就存成 `const char*` 变量，或显式写 `Describe<const char*>("hi")`。


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
1. template int Min<int>(...) 是在干什么？
这行是显式实例化定义：

```cpp
template int Min<int>(const int&, const int&);
```
逐词拆开：

`template`：我要显式实例化一个模板。

`int`：实例化出来的函数返回类型是 `int`。

`Min<int>`：函数模板是 `Min`，模板实参是 `int`，也就是让 `T = int`。

`(const int&, const int&)`：实例化出来的函数参数类型。

分号：这是一个定义，编译器要在这里生成 `Min<int>` 的代码。

它的意思就是：

***编译器，请你现在、在这个编译单元里，用主模板 Min 的定义，为 T = int 生成一份真正的函数代码。***

生成的函数大概等价于：

```cpp
int Min<int>(const int& a, const int& b) {
    return a < b ? a : b;
}
```
所以它放在 .cpp 里，别的 .cpp 调用 Min(3, 5) 时，就可以链接到这一份代码。

注意：template 后面没有 <>。因为这不是特化，只是显式实例化。
这样别的编译单元就能链接到这一份。

P2.4 会同时演示普通 `.h/.cpp` 分工和显式实例化。

**是什么**：C++ 的编译模型是"每个 `.cpp`（翻译单元）独立编译成 `.o`，再由链接器拼起来"。
模板的特殊性在于：编译器遇到 `Min(3, 5)` 时必须**当场看到 `Min` 的定义**，才能生成 `Min<int>`。
"模板定义放头文件"和"显式实例化"都是围绕这个约束的应对手段。

**为什么需要它（它解决了什么问题）**：

- 把模板定义放进 `.cpp`，别的翻译单元只看到声明、看不到定义，无法实例化，链接时就会
  `undefined reference to '...Min<int>(...)'`。所以默认做法是**定义跟着声明一起放头文件**。
- 但头文件化也有代价：每个包含它的 `.cpp` 都可能各生成一份 `Min<int>` 的代码，编译变慢、体积变大。
  如果你只想支持固定几种类型，可以把定义藏进 `.cpp`，再用**显式实例化**提前生成那几份。
- 对应的还有 **`extern template`**（C++11）：在头文件里写
  `extern template int Min<int>(const int&, const int&);`，
  意思是"这个特化别在我这个翻译单元里隐式实例化，链接时去别处拿"，用来抑制重复实例化、加快编译。

**常见用法**：

| 做法 | 头文件 | `.cpp` | 适合场景 |
| :-- | :-- | :-- | :-- |
| 定义全放头文件（**默认**） | 声明 + 定义 | 无 | 泛型库；支持任意类型 |
| 声明在 `.h`、定义在 `.cpp` + 显式实例化 | 只有声明（+ 可选 `extern template`） | 定义 + `template ...` 显式实例化 | 只支持固定几种类型，想减少重复实例化 / 隐藏实现 |

**什么时候用**：绝大多数情况——**定义放头文件**。只有当你明确要"封闭类型集合"或想缩短编译时间时，
才用"`.cpp` + 显式实例化"。P2.4 让你两种都体验一下。

**一个跟本题无关的例子**：

```cpp
// counter.h
#pragma once
#include <cstddef>
template <typename T> T Sum3(const T& a, const T& b, const T& c) { return a + b + c; }

// 如果只想支持 int/long 两种，就在 counter.cpp 里：
//   #include "counter.h"
//   template int  Sum3<int>(const int&, const int&, const int&);
//   template long Sum3<long>(const long&, const long&, const long&);
// 别的 .cpp 调 Sum3<double> 就会链接失败——这正是"封闭类型集合"的效果。
```

**额外一条头文件纪律**：头文件里**非模板、非成员函数**必须写 `inline`（或在 `.cpp` 里定义），
否则多处包含会让链接器看到多份定义；类内定义的成员函数和模板本身不受此限。

### 1.8 CTAD（类模板实参推导，C++17）

C++17 之前，用类模板必须写全 `std::vector<int> v{1,2,3};`。C++17 起，如果构造函数能从实参推出模板参数，
就可以省略：

```cpp
std::vector v{1, 2, 3};      // 推出 std::vector<int>
std::pair   p{1, 2.0};       // 推出 std::pair<int, double>
```

对你自己的类模板也一样：`Box b(42);` 能推出 `Box<int>`，前提是类里有一个**能由实参推导出 `T` 的构造函数**
（比如 `explicit Box(T v)`）。如果没有任何这样的构造函数，就必须显式写 `Box<int> b(...)`。

**是什么**：CTAD（Class Template Argument Deduction，类模板实参推导，C++17）= 创建类模板对象时，
让编译器从构造函数实参推出模板参数，不用手写 `<...>`。

**为什么需要它（它解决了什么问题）**：C++17 之前 `std::vector<int> v{1,2,3};`、`std::pair<int,double> p{1,2.0};`
必须写全模板实参，又啰嗦、又容易和不一致的类型打架。CTAD 让"能从实参看出来"的情况省掉 `<...>`，
减少冗余、也让代码在类型变化时不用同步改两处。

**常见用法**：

- 标准库：`std::vector v{1,2,3};`、`std::pair p{1,2.0};`、`std::optional o{5};`、
  `std::lock_guard lk(m);`、`std::unique_lock lk(m);`；
- 自己的类：只要有一个构造函数能从实参推出全部模板参数（如 `explicit Box(T v)`），`Box b(42);` 就成立；
- 推导指引（deduction guide）：类模板可以额外写 `Box(T) -> Box<T>;` 告诉编译器怎么推，
  甚至在构造函数推不出来时也能用（本阶段用不到，但要知道 CTAD 不只是"构造函数自动推"）。

**什么时候用**：类型能从实参一眼看出、且不写 `<...>` 不会引起歧义时。
如果实参推不出唯一类型，或者你想明确指定（例如 `Box<int> b(7)`），就老老实实写全。

**一个跟本题无关的例子**：

```cpp
template <typename K, typename V>
struct Entry {
  K key;
  V value;
  Entry(K k, V v) : key(k), value(v) {}
};

Entry e1{1, std::string("one")};          // CTAD → Entry<int, std::string>
Entry e2{std::string("one"), 1};         // CTAD → Entry<std::string, int>
// Entry e3{1, 1};                       // 也可以，推成 Entry<int,int>（想区分类型就显式写）
```

**三个细节**：

- CTAD 只在 C++17 起可用（`-std=c++17` 已满足）。
- 构造函数是 `explicit` **不影响** CTAD：`Box b(42);` 这种"直接初始化"照样能推出 `Box<int>`；
  `explicit` 只是禁止 `Box b = 42;` 这类隐式转换。
- 如果类**没有任何**能从实参推出 `T` 的构造函数（例如只有默认构造、或者构造函数参数是 `std::initializer_list` 以外的复杂形式），
  就必须显式写 `Box<int>`。P2.4 有一个测试点专门验证这一点。

### 1.9 命名空间：为什么要有它，以及**跨文件**怎么用

> 这一节是 P2.4 的前置知识。src 的 `src/3 - Misc/namespaces.cpp` 讲的是单文件里的命名空间、
> 嵌套命名空间和 `using`；**跨 `.h`/`.cpp` 使用命名空间的规则它没讲**，这里补齐。

**是什么**：命名空间（namespace）是给"名字"划分的一块作用域/前缀。
写 `namespace minimath { int Add(int, int); }`，这个函数的**完整名字**就叫 `minimath::Add`。
`::` 叫作用域解析运算符。没有放进任何命名空间的名字属于**全局命名空间**（可以用 `::Add` 明确指它）。

**为什么需要它（它解决了什么问题）**

1. **命名冲突**：你的库和别人的库都有 `Add`、`Init`、`Node`，如果都在全局命名空间，链接/编译就撞车。
   放进不同命名空间后 `mylib::Add` 与 `other::Add` 是两个不同的标识符，可以共存——P3.4 会专门验证。
   C++ 标准库把所有东西放进 `std` 正是这个原因（所以是 `std::cout` 而不是 `cout`）。
2. **组织代码**：把逻辑相关的一组类型/函数打包成一个"库"，名字本身就说明了归属（`bustub::BufferPool`）。
3. **跨文件拼装同一个库**：同一个命名空间可以在**多个文件里被反复打开**（reopen），里面的名字会累积到一起。
   这就是"声明放 `.h`、实现放 `.cpp`"能协同工作的基础。
4. **控制可见性**：匿名命名空间（`namespace { ... }`）和 `static` 让名字只在本编译单元可见（见 stage3 第 1.7 节）。

**常见用法（重点：跨文件）**

1) **头文件里声明**，声明包在命名空间里（头文件一定要有 include guard）：

```cpp
// mylib/math.h
#pragma once
namespace mylib {
int Add(int a, int b);        // 这里只是声明
}  // namespace mylib
```

2) **实现文件里"重新打开"同一个命名空间**（推荐写法，可读性最好）：

```cpp
// mylib/math.cpp
#include "math.h"
namespace mylib {             // 重新打开，不是新建；名字加进同一个 mylib
int Add(int a, int b) { return a + b; }
}  // namespace mylib
```

3) 或者用**限定名定义**（完全等价，少一层缩进）：

```cpp
// mylib/math.cpp
#include "math.h"
int mylib::Add(int a, int b) { return a + b; }
```

   这里有三条**铁律**，跨文件时每条都会实际咬人：
   - **在哪个命名空间声明，就必须在同一个命名空间里定义**。`namespace mylib { int Add(int,int); }`
     配 `int Add(int,int) { ... }`（全局）会变成两个不同函数：链接时报 `undefined reference to mylib::Add`。
   - **命名空间可以分散在多个文件、多个位置**，编译器会把它们拼在一起：`a.h` 里开 `namespace lib {}`、
     `b.cpp` 里再开 `namespace lib {}`，是同一个 `lib`。
   - **不能"跳级"定义**。若声明在 `namespace A { namespace B { int f(); } }`，
     定义必须是 `namespace A { namespace B { int f() {...} } }` 或 `int A::B::f() {...}`，
     不能只开 `namespace A {}` 就定义 `B::f`。

4) **调用时用限定名**：`mylib::Add(2, 3)`。在 `mylib` 内部可以直接写 `Add(2, 3)`（无需前缀）。

5) **`using` 的两种形式**：
   - `using namespace mylib;`：把**整个命名空间**引入当前作用域。方便，但很容易引入意料之外的名字冲突；
     **绝对不要写在头文件里**（头文件会被别人包含，等于把 `mylib` 塞进所有使用者的作用域）。
   - `using mylib::Add;`：只引入**一个名字**。安全得多，通常写在函数作用域里，或写在你确实要用的地方。
   - `using mm = mylib;` 是**命名空间别名**，给长名字起短名（`mm::Add`）。

6) **嵌套命名空间**：`namespace A { namespace B { ... } }`；C++17 起可以简写成 `namespace A::B { ... }`。

7) **全局作用域**：`::Add` 明确表示"全局命名空间里的 `Add`"，用来在局部名字遮蔽时把全局那个捞出来。

8) **`main` 不进命名空间**：`main` 必须在全局作用域。

9) **`inline namespace`（C++17）**：`inline namespace v2 { ... }` 里的名字可以被外层命名空间直接看到
   （库版本兼容常用）。知道有这回事即可，本阶段用不到。

**什么时候用**：

- 写库 / 多文件项目时——**默认就要用**，把库的所有符号包进一个命名空间；
- 名字可能和别处冲突时；
- 想把一组相关工具打包成一个"逻辑模块"时。

单文件小练习里不是必须，但**本题 P2.4 明确要求**使用，因为它演示的是"模板 + 普通函数 + 类模板
如何在同一个命名空间下跨文件协作"，这也正是 BusTub / 15-445 里所有代码的组织方式。

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
把它和 P2.4 的结构对一下：`shapes` 就是 `minimath`，`Distance` 就是 `Add`，`Point` 就是 `Box`。

**跨文件使用 `namespace` 的三句话总结**：

1. 声明与定义必须在**同一个**命名空间里；
2. 头文件里**不要**写 `using namespace ...;`（调用方自己决定要不要 `using`）；
3. 每个 `.cpp` 都 `#include` 自己的头文件，让"声明与定义对不上"这类错误在编译/链接期立刻暴露。

---

## 2. 主线题目（贴合 src 两课）

### P2.1 模板函数（Templated Functions）

- **考什么**：函数模板的实例化、类型推导、`const T&` 形参、非类型模板参数。
- **你要写**：`projects/mysol/stage2/p2_1_functions.h`。只写头文件。
- **复制过来的测评文件**：`tests/stage2/p2_1_functions_test.cpp`。测评点数：11。

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
g++ -std=c++17 p2_1_functions_test.cpp -I../../tests -o p2_1 && ./p2_1
```

通过标准：`Result: 11/11 Passed`。

**自查**：① `Min(3,5)` 与 `Min(3.2,1.5)` 是同一个函数吗？② `Min("abc","def")` 会把 `T` 推成什么？③ 什么时候必须写 `Min<int>(...)`？

---

### P2.2 模板类 Stack（Templated Class + 借用 + 移动）

- **考什么**：类模板、把 Stage 1 的 `const&` / `T&&` / `std::move` 用到容器接口上、const 成员函数重载。
- **你要写**：`projects/mysol/stage2/p2_2_stack.h`。只写头文件。
- **复制过来的测评文件**：`tests/stage2/p2_2_stack_test.cpp`。测评点数：12。

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
g++ -std=c++17 p2_2_stack_test.cpp -I../../tests -o p2_2 && ./p2_2
```

通过标准：`Result: 12/12 Passed`。

**自查**：① 为什么 `Push` 要两个重载？只留按值传参会怎样？② `Top()` 为什么返回 `T&`，为什么还要 const 版本？③ `Stack<std::unique_ptr<int>>` 靠哪条重载工作？

---

### P2.3 模板特化、非类型参数与 `constexpr if`

- **考什么**：函数模板全特化、类模板特化、非类型模板参数、编译期递归、`constexpr if`。
- **你要写**：`projects/mysol/stage2/p2_3_specialization.h`。只写头文件。
- **复制过来的测评文件**：`tests/stage2/p2_3_specialization_test.cpp`。测评点数：15。

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
template <int T> struct Constant;   // 需要一个静态常量成员 value，它的值就是 T（该用哪个关键字见 1.4）

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
g++ -std=c++17 p2_3_specialization_test.cpp -I../../tests -o p2_3 && ./p2_3
```

通过标准：`Result: 15/15 Passed`。

**自查**：① `Factorial<10>` 是运行期算的还是编译期算的？怎么证明？② `Describe(double)` 用的是"重载"还是"特化"？③ 用普通 `if` 而不是 `if constexpr`，`ToString(std::string)` 还能编译吗？为什么？

---

## 3. 扩展题目

### P2.4 模板定义放头文件 + 显式实例化 + CTAD

- **考什么**：模板的编译单元模型（1.7）、显式实例化、C++17 类模板实参推导（1.8），
  以及**跨文件使用命名空间**（1.9）。
- **你要写**：一个多文件小项目，全部放在 `projects/mysol/stage2/` 下：
  - `minimath/min.h`（`minimath` 命名空间里的**模板声明 + 模板实现**）
  - `minimath/min.cpp`（普通函数的实现，并演示显式实例化）
- **复制过来的测评文件**：`tests/stage2/p2_4_minimath/p2_4_minimath_test.cpp`（放到 `projects/mysol/stage2/` 下，
  它会 `#include "minimath/min.h"`，所以你的头文件必须正好在 `mysol/stage2/minimath/min.h`）。测评点数：8。

**测评程序要求 `minimath` 命名空间里提供这些名字：**

```cpp
// min.h（模板的声明和实现都要在这个头文件里）
namespace minimath {
  template <typename T> T Min(const T& a, const T& b);   // 模板
  int Add(int a, int b);                                 // 普通函数：声明放这里
  template <typename T> class Box {
  public:
    explicit Box(T v);          // 必须能从实参推导出 T，CTAD 才成立
    const T& Get() const;
  };
}
```

下面这些事也必须在你的代码里做出来（具体怎么写由你决定）：

- 在 `min.cpp` 里给出 `Add` 的实现；
- 在 `min.cpp` 里再写一行**显式实例化**，把 `Min<int>` 提前生成出来；
- 所有名字（`Min`、`Add`、`Box`）都必须在 `minimath` 命名空间里**声明并定义**。

**下面这些你必须自己想清楚、自己补上（这里只给要求，不给能直接复制的代码）：**

- `Box<T>` 内部必须有一个**私有数据成员**，把构造函数收到的那个 `T` 值**存下来**；`Get()` 返回的
  就是它的 `const` 引用。这个成员的类型显然就是模板参数 `T`，名字由你定（叫什么无所谓，关键是
  `Get()` 返回的必须是它）。**不要**让构造函数只是收下参数就丢掉——那样 `Get()` 根本没东西可返回。
- `Box<T>` 的构造函数必须真正把实参存进上面那个成员（初始化列表或函数体里赋值都行）。
- `Min` 与 `Box` 的**定义**都写在 `min.h` 里（模板定义必须让每个用到它的翻译单元看见，见 1.7）；
  `Add` 的**定义**写在 `min.cpp` 里。
- `min.cpp` 里 `Add` 的定义必须放在 `namespace minimath { ... }` 里（或者写成
  `int minimath::Add(int a, int b) { ... }`）。如果写成全局的 `int Add(int, int) { ... }`，
  那是**另一个函数**，测评调用 `minimath::Add(2, 3)` 时会链接失败。见 1.9 的三条铁律。
- `min.cpp` 里那句显式实例化也要作用在 `minimath::Min<int>` 上（写在 `namespace minimath {}` 内，
  或写全限定名）。

**为什么这样分工：**

- `Min` 是模板，**实现必须和声明一起放 `min.h`**；一旦挪到 `.cpp`，编译 `p2_4_minimath_test.cpp` 的编译单元
  看不到函数体，就会 `undefined reference to minimath::Min<int>(...)`。见 1.7。
- `Add` 是普通函数，可以按传统方式"声明在 `.h`、实现在 `.cpp`"；测评会调用 `minimath::Add(2,3)`，
  所以编译时要**把 `min.cpp` 一起编进去**（命令见下）。
- **命名空间必须一致**：头文件里的声明在 `minimath` 里，`.cpp` 里的定义也必须落在 `minimath` 里，
  这靠"重新打开命名空间"或"限定名定义"实现。见 1.9。
- `Box<T>` 有一个 `explicit Box(T)` 构造函数，测评用 `minimath::Box b(42);` 检查 CTAD 是否推导出 `Box<int>`，
  用 `minimath::Box b(std::string("hi"));` 检查 `Box<std::string>`。所以构造函数必须能从实参推出 `T`。见 1.8。

**要做的事**：按上面结构建好 `minimath/min.h` 与 `minimath/min.cpp`（注意 1.9 讲过的跨文件命名空间规则），
实现 `Min`、`Add`、`Box`；在 `.cpp` 里写一句显式实例化，把 `minimath::Min<int>` 提前生成出来。

**测评点在查什么**：跨编译单元能用 `Min<int>` 和 `Min<std::string>`；`Add` 从 `.cpp` 链接成功；
`Box` 的 CTAD 推出 `int` 与 `std::string`；`std::vector v{1,2,3}` 推出 `vector<int>`；
`std::pair p{1,2.0}` 推出 `pair<int,double>`；`Box<int>` 显式写法也能用。

**编译运行**（注意要把你的 `min.cpp` 一起编译）：

```bash
cd projects/mysol/stage2
g++ -std=c++17 p2_4_minimath_test.cpp minimath/min.cpp -I../../tests -o p2_4 && ./p2_4
```

通过标准：`Result: 8/8 Passed`。

**自查**：① 为什么 `Min` 不能把实现放 `.cpp`？（链接时会发生什么）② 显式实例化 `template int Min<int>(...)` 有什么用？③ `Box b(42);` 能推导的前提是什么？④ `Box` 里那个私有成员是什么类型？如果没有它，`Get()` 还能返回什么？⑤ 如果把 `min.cpp` 里的 `Add` 定义写在 `namespace minimath` **外面**，会发生什么错误？
