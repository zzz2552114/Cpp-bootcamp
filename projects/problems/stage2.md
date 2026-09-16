# Stage 2 · C++ 模板

对应源文件：`2 - C++ Templates/templated_functions.cpp`、`templated_classes.cpp`。

---
## P2.1 模板函数（Templated Functions）

**对应 / 前置**：读完 `templated_functions.cpp`。
**目标**：模板 = 编译期按类型生成代码；函数模板的类型推导。

**任务**：实现并测试。

```cpp
template <typename T> T Min(const T& a, const T& b) { return a < b ? a : b; }
template <typename T> T Max(const T& a, const T& b) { return a < b ? b : a; }
// 返回数组长度的非类型模板（用于 C 风格数组）
template <typename T, size_t N> constexpr size_t ArrayLen(const T(&)[N]) { return N; }
```

**验收**：`Min(3,5)==3`、`Min(3.2,1.5)==1.5`、`Min(std::string("abc"),std::string("def"))=="abc"`；`Max` 同理；`ArrayLen([]{int a[7]; return a;}())` 之类能推出 7。回答：为什么 `Min(3, 3.2)` 编译不过？

**自查三问**：
1. `Min(3,5)` 和 `Min(3.2,1.5)` 生成的是同一个函数吗？
2. `const T&` 形参把 `Min("abc","def")` 的 T 推导成什么？会出什么意外？（提示：数组会退化成指针）
3. 什么时候必须显式写 `Min<int>(...)`，什么时候能靠推导？

---
## P2.2 模板类 Stack（Templated Class + 借用 + 移动）

**对应 / 前置**：读完 `templated_classes.cpp`。
**目标**：类模板 + 把 Stage 1 的 const&/move 用进容器。**底层用 `std::vector<T>`，不重复造轮子。**

**任务**：

```cpp
template <typename T>
class Stack {
public:
    void  Push(const T& value);   // 拷贝入栈
    void  Push(T&& value);        // 移动入栈（用 std::move）
    void  Pop();                  // 弹出栈顶
    T&    Top();
    const T& Top() const;
    bool  Empty() const;
    size_t Size() const;
private:
    std::vector<T> data_;
};
```

**验收**：`Stack<int>` 压 10/20/30 → `Top()==30`，`Pop()` 后 `Top()==20`；`Stack<std::string>` 能 `Push("abc")`（走 `std::move` 重载）；空栈 `Pop()`/`Top()` 的行为由你定义但要在注释里说明（建议 `assert`）。

**自查三问**：
1. 为什么 `Push` 要同时提供 `const T&` 和 `T&&` 两个重载？只留 `const T&` 会怎样、只留"按值传参 `Push(T value)`"又会怎样？
2. `Top()` 为什么返回 `T&` 而不是 `T`？为什么还要一个 `const T& Top() const` 版本？
3. `Stack<std::unique_ptr<int>>` 现在能正常用吗？`Push` 的哪条重载让它可移动不可拷贝？

---
## P2.3 模板特化与非类型模板参数（Specialization + Non-type）

**对应 / 前置**：读完 `templated_functions.cpp`（`print_msg<float>` 特化、`add3<true>` 非类型）与 `templated_classes.cpp`（`Bar<int T>`）。
**目标**：特化 = 个别类型给专门实现；非类型参数 = 把值带进类型（编译期计算）。

**任务**：
1. **函数模板特化**：`template<typename T> std::string Describe(const T&)`，通用版返回 `"default"`；对 `double` 特化返回 `"double"`，对 `const char*` 特化返回 `"c-string"`。
2. **非类型模板参数**：`template<size_t N> struct FixedArray { int a[N]; constexpr size_t Size() const { return N; } };`，并用一个 `template<size_t N> constexpr size_t Factorial(){ return N*Factorial<N-1>(); }` + `template<> constexpr size_t Factorial<0>(){return 1;}` 在编译期求 `Factorial<10>`。
3. **[选做] `constexpr if`**：用 `if constexpr (std::is_integral_v<T>)` 写一个 `ToString(T)`，整型走 `std::to_string`，否则走 `std::string("non-integral")`。说明它比 `add3<bool>` 那样的运行时 `if` 好在哪里。

**验收**：`Describe(1)== "default"`、`Describe(1.0)== "double"`、`Describe("hi")== "c-string"`；`Factorial<10>()==3628800`；`static_assert(FixedArray<5>().Size()==5)`。

**自查三问**：
1. `Factorial<10>` 是在运行时还是编译期算完的？怎么证明？（`static_assert`）
2. 特化和重载有什么不同？`Describe(double)` 里作用的是什么机制？
3. `if constexpr` 与普通 `if` 的区别（丢弃分支是否还会被实例化/编译）？

---
## P2.4 模板定义放头文件 + CTAD（Header Files & 类模板实参推导）

**对应 / 前置**：构建过 repo 的 CMake，见过 `.cpp` 单独编译。
**目标**：理解"模板必须对每个翻译单元可见"，所以模板实现要在头文件；认识 C++17 CTAD。

**任务**：建立如下目录结构（多文件项目，这次让 CMake 链接起来）：

```text
minimath/
    min.h        // Min<T> 的声明 + 实现（都写这里）
    min.cpp      // 只放一个"显式实例化"示例 + 普通函数
main.cpp
```

1. 在头文件里定义 `template<typename T> T Min(const T&, const T&);`。回答：为什么**不能**像普通函数那样"声明放 .h，实现放 .cpp"（提示：链接错误 `undefined reference to Min<int>`）。
2. 演示**显式实例化**：在某 TU 写 `template int Min<int>(int, int);` 并说明它能把哪些类型提前实例化。
3. **CTAD**：`std::vector v{1,2,3}; std::pair p{1,2.0};` 推导出什么类型？在你的 Stack 上，`Stack s = Stack{1};` 能推导吗？你的 `Stack` 需要补什么（含参构造函数）才能支持 CTAD？（提示：给一个 `Stack(std::initializer_list<T>)` 或单参构造）

**验收**：`main` 调 `Min(3,5)` 与 `Min(3.2,1.5)` 通过；能给出"模板为什么头文件化"的三句话解释；能打印 `std::vector v{...}` 的元素证明它被正确推导为 `std::vector<int>`。

**自查三问**：
1. 头文件不加 include guard（`#pragma once` 或 `#ifndef`）会出现什么？
2. `using namespace` 能放头文件吗？（结合 `namespaces.cpp` 那课的结论）
3. CTAD 在什么时候会推导失败、需要显式给模板实参？