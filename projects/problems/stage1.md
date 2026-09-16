# Stage 1 · 引用与移动语义

对应源文件：`1 - References and Move Semantics/references.cpp`、`move_semantics.cpp`、`move_constructors.cpp`。

---
## P1.1 引用游乐场（Reference Playground）

**对应 / 前置**：读完 `references.cpp`。
**目标**：搞清"引用是别名"、`const&`、const 成员函数。

**任务**：实现一个统计小结构。

```cpp
struct Statistics {
    std::vector<int> data;

    void AddValue(int x);          // 修改原对象
    int  Sum() const;              // 只读（⚠️ 大数据会 int 溢出）
    long long SumL() const;        // 只读，64 位，不溢出
    double Average() const;        // 只读；data 为空时返回 0.0
};
```

- `AddValue` 必须能修改 `data`；`Sum`/`Average` 必须标 `const`。
- 写一个自由函数通过 `const Statistics&` 接收它，证明只读引用不会触发拷贝。

**验收**：`{1,2,3,4,5}` → `sum=15, average=3.0`；空对象 → `sum=0, average=0.0`；`const Statistics&` 形参能调用 `Sum()` 但不能调用任何非 const 成员。

**大数据陷阱**：`Sum()` 返回 `int`，对 `1..10^6` 求和（=500000500000）会**有符号溢出（UB）**；`Average()` 内部必须用 `SumL()`，否则“先溢出再平均”的结果是错的。

**自查三问**：
1. `Statistics s` 与 `const Statistics& s` 形参各自拷贝了几次？
2. 为什么"函数不想改对象"要写 `const Statistics&` 而不是 `Statistics` 或 `Statistics&`？
3. const 成员函数内部能不能调用非 const 成员函数？反过来呢？

---
## P1.2 重载解析三兄弟（Overload Resolution）

**对应 / 前置**：读完 `move_semantics.cpp`（它通篇在讲 `T&&` 如何参与重载选择）。
**目标**：这是理解 `std::move` 到底"选中了哪个函数"的关键。

**任务**：写三个同名重载，各自打印被选中者。

```cpp
void Which(int& x);       // 打印 "lvalue ref"
void Which(const int& x); // 打印 "const lvalue ref"
void Which(int&& x);      // 打印 "rvalue ref"
```

在 `main()` 里分别用这些实参调用 `Which(...)`，并**预测**输出，然后运行核对：

- `Which(a)`（`int a=1;`）
- `Which(static_cast<const int&>(a))`
- `Which(10)`（字面量实参）
- `Which(std::move(a))`
- `Which(const_i)`（`const int const_i = 2;`）

**验收**：给出"实参 → 选中哪个重载"的完整对照表，并能解释每条。预期：
`a`→lvalue ref；`const_cast`/const 变量→const lvalue ref；`10`→rvalue ref；`std::move(a)`→rvalue ref。

**自查三问**：
1. `const int&` 为什么既能绑左值又能绑右值？
2. `int&&` 能不能直接绑定 `a`（左值）？这跟 `std::move` 的存在有什么关系？
3. 去掉 `int&&` 重载后，`Which(10)` 会绑定到哪个？这说明了什么？

---
## P1.3 MoveOnlyBuffer（拥有堆内存、只可移动）

**对应 / 前置**：读完 `move_constructors.cpp` 和 `3 - Misc/wrapper_class.cpp` 的 `IntPtrManager`。
**目标**：自己写一个"拥有 `new[]/new` 资源、删除拷贝、只可移动"的类——这是整个 Bootcamp 最重要的一个基础类。

**任务**：实现一个**不可拷贝、只可移动**的动态数组。

```cpp
class Buffer {
public:
    explicit Buffer(size_t n);                 // new int[n]，全部置 0
    ~Buffer();                                 // delete[]，空指针安全

    Buffer(Buffer&& other) noexcept;           // 移动构造：接管 other 的资源
    Buffer& operator=(Buffer&& other) noexcept;// 移动赋值

    Buffer(const Buffer&) = delete;            // 禁拷贝
    Buffer& operator=(const Buffer&) = delete;

    size_t Size() const;
    int& operator[](size_t i);                 // 不做越界检查即可
    const int& operator[](size_t i) const;

private:
    int* data_;
    size_t size_;
};
```

**必须处理的局面**（逐条自查，缺一条就是 bug）：
1. `Buffer b(200); b = std::move(a);` ——不能泄漏 `b` 原来的 200 个元素（先释放再接管）。
2. `Buffer c(std::move(a));` —— `a` 格局：`data_=nullptr, size_=0`；析构 `a` 不能崩。
3. **`b = std::move(b);` self-move** ——不能把自己 delete 掉再接管悬垂指针（见仓库 `IntPtrManager::operator=` 的 `if (ptr_==other.ptr_) return *this;`）。
4. 移动构造/移动赋值要标 `noexcept`（原因在 P1.4 验证）。

**验收**：`a[0]=445; Buffer b(std::move(a));` 后 `b[0]==445` 且 `b.Size()` 正确、`a.Size()==0`；`assert(a.data_ 为空)` 通过析构安全。

**自查三问**：
1. 为什么这里必须 `= delete` 拷贝？不删会发生什么（double free？）
2. 移动赋值里"先释放自己再接管别人"的顺序为什么不能颠倒到"先接管再释放"？
3. `std::move(a)` 这一行本身干了什么？真正把 `a.data_` 搬走的是谁？

---
## P1.4 `noexcept` 为什么不能省略（实验）

**对应 / 前置**：读 `move_constructors.cpp` 时你可能注意到 `Person` 的移动构造**没标** `noexcept`，而 `std::vector` 提供的移动通常标了。本题验证后果。
**目标**：理解 `std::vector`/`unordered_map` 扩容时的 `move_if_noexcept` 规则。

**任务**：写一个带"观测打印"的小类 `Widget`：

```cpp
struct Widget {
    int v;
    explicit Widget(int x);
    Widget(const Widget&);          // 打印 "copy"
    Widget(Widget&&) /* 先不写 noexcept */; // 打印 "move"
};
```

1. 把它 `push_back` 进 `std::vector<Widget>` 4 次（不 `reserve`，逼出一次扩容），观察打印：**移动构造未标 `noexcept` 时，扩容走的是 copy 还是 move？**
2. 给移动构造加上 `noexcept`，重跑，观察变化。
3. 再试：`Widget` 的拷贝构造 `= delete`（只留移动、且移动不 `noexcept`），还能不能 `push_back` 扩容？解释为什么。

**验收**：能自己复现"未标 `noexcept` → 重分配时拷贝；标了 → 移动"，并写出 `std::move_if_noexcept` 的选择规则（move 是 noexcept，或没有可用拷贝构造，才移动）。

**自查三问**：
1. 为什么容器扩容时"宁拷贝不移动"?（提示：异常安全 / 强保证）
2. 15-445 里 `Page`、`Tuple` 的移动构造**必须**标 `noexcept`，现在你能说出原因了吗？
3. 你的 P1.3 `Buffer` 移动操作为什么也该标 `noexcept`？

---
## P1.5 拷贝 vs 移动：一个数字（微基准）

**对应 / 前置**：`move_semantics.cpp` 的 motivation 就是"移动比深拷贝快"。
**目标**：把"更快"变成一个你跑出来的数字。

**任务**：用 `std::chrono::steady_clock` 分别计时：

```cpp
// A) 拷贝：    std::vector<std::string> v2 = v1;         // v1 含 N 个长字符串
// B) 移动：    std::vector<std::string> v3 = std::move(v1_2);
```

N 取足够大（例如 200000，每个字符串 64 字节），打印两段耗时与倍数。

**验收**：移动耗时通常比拷贝小 1~2 个数量级；能说清"为什么移动快"（只搬 3 个指针，不复制堆数据），并注意 `std::string` 的 SSO（短字符串原地存储、几乎无堆，移动优势不明显），所以选**长**字符串。

**自查三问**：
1. `std::vector` 的移动为什么只交换 `begin/end/capacity` 三个指针量级的东西？
2. 移动后源 vector 还能用吗？它的 `size()` 一定是 0 吗（标准保证 vs 实现细节）？
3. 对一个 `int` 做 `std::move` 有意义吗？