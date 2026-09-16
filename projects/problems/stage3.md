# Stage 3 · 包装类 / 迭代器 / 命名空间

对应源文件：`3 - Misc/wrapper_class.cpp`、`iterator.cpp`、`namespaces.cpp`。

---
## P3.1 RAII 资源句柄 + RVO / copy elision（Resource Manager & 返回值优化）

**对应 / 前置**：读完 `wrapper_class.cpp`。
**目标**：RAII（资源 = new/delete，生命周期绑对象），以及观察"按值返回资源管理对象"时到底发生了几次构造/移动/拷贝。

**任务**：实现一个管理**带 id 的堆资源**的类（id 全局递增，用来打印，避免和 P1.3 的 Buffer 重复造轮子）。

```cpp
class Handle {
public:
    explicit Handle(int id);      // 打印 "acquire <id>"
    ~Handle();                    // 未失效时打印 "release <id>"
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    Handle(Handle&& other) noexcept;        // 打印 "move-ctor <id>"
    Handle& operator=(Handle&& other) noexcept;
    int Id() const { return id_; }
    bool Valid() const { return valid_; }
private:
    int  id_;
    bool valid_;
};
Handle MakeHandle(int id);        // 按值返回
```

1. `auto h = MakeHandle(42);` 观察打印——在 C++17 下，返回临时对象通常是 **guaranteed copy elision**：只 `acquire` 一次、`release` 一次，**没有** move。把 `-fno-elide-constructors` 加上再编译，观察 move 出现。
2. `Handle h2 = std::move(h);` 观察 move-ctor，随后 `h.Valid()==false`。
3. `h2 = MakeHandle(7);` 观察 move-assignment，确认旧资源被释放、不可自 move。

**验收**：能写出"elision on → 零移动；elision off → 一次移动"的打印差异，并解释"RVO 是标准规定的优化、move 是兜底手段"。

**自查三问**：
1. 为什么 C++17 能在"按值返回局部对象"时不调移动构造（对象"直接构造"在调用者栈上）？
2. `return std::move(x);` 在 C++17 里是不是更"快"？会有什么副作用？
3. 移动后 `valid_` 标志的意义：moved-from 对象"可以析构但别再用"——析构安全靠哪一行保证？

---
## P3.2 双向迭代器（修好 `--end()`）+ range-for（Forward/Backward Iterator）

**对应 / 前置**：读完 `iterator.cpp`。
**目标**：在仓库双向链表基础上，实现**能往回走**的迭代器、返回**引用**、并让 `for (auto x : dll)` 能用。

**关键坑（必须先想清楚）**：`End()` 返回的是 `curr_==nullptr` 的"过去末尾"迭代器。如果 `operator--` 写成 `curr_ = curr_->prev_;`，`--End()` 会**解引用 nullptr → 段错误**（见 REVIEW 硬伤 1）。所以 DLL 必须维护 `tail_`，迭代器要携带 `tail_`。

**任务**：改造一个最小双向链表（只需头插 + 遍历）：

```cpp
struct Node { int value_; Node* next_; Node* prev_; ... };

class DLLIterator {
public:
    DLLIterator(Node* curr, Node* tail);   // 携带 tail 以便 End() 回退
    int&        operator*();               // 返回引用，可 *it = 5
    const int&  operator*() const;
    DLLIterator& operator++();             // 前缀
    DLLIterator  operator++(int);          // 后缀：保存旧值
    DLLIterator& operator--();             // curr_==nullptr 时回退到 tail_
    bool operator==(const DLLIterator&) const;
    bool operator!=(const DLLIterator&) const;
private:
    Node* curr_;
    Node* tail_;
};

class DLL {
    // InsertAtHead / Begin() / End() / 析构
    DLLIterator Begin();   // (head_, tail_)
    DLLIterator End();     // (nullptr, tail_)
    // range-for 需要小写成员：begin() / end()（或提供自由 begin/end）
};
```

**验收**：
1. 正向：把 1..6 头插（得到 6 5 4 3 2 1），前缀/后缀 `++` 都遍历一遍。
2. **反向**：注意反向遍历必须**先回退再解引用**（`End()` 的 `curr_` 是 `nullptr`，直接 `*it` 会崩）：
   ```cpp
   for (auto it = dll.End(); it != dll.Begin();) { --it; std::cout << *it << " "; }
   ```
   `auto it = dll.End(); --it;` 后 `*it == 1`（尾元素），**不再崩溃**。
3. `*it = 99` 能真正改到节点值。
4. `for (auto& v : dll) { ... }`（range-for）行得通。

**自查三问**：
1. `++iter` 为什么比 `iter++` 高效？后缀版的那行为什么必须先 `temp = *this`？
2. 为什么 `End()` 需要 tail 信息、普通迭代器（非 past-the-end）的 `--` 又走 `prev_`？
3. `operator*` 返回 `int&` 与返回 `int` 的差别？const 版本存在的意义？

---
## P3.3 Rule of 0/3/5（把隐藏的 double-free 揪出来）

**对应 / 前置**：刚写完 P3.2 的裸链表（手动 `new/delete`）。
**目标**：把"默认生成的拷贝构造/赋值"带来的坑按顺序走一遍，落到 Rule of 0/3/5。

**任务**：在 P3.2 的 `DLL`（纯裸指针、手写析构）上：

1. 先写 `DLL d2 = d1;`（编译通过吗？运行会发生什么？）→ 预期 **double free**（两个对象 `head_` 相同、析构各 delete 一遍）。记录下来。
2. 演示 **Rule of 3**：用手写拷贝实现深拷贝（复制整条链），使 `d2 = d1` 安全。
3. 再演示 **Rule of 5**：补移动构造/移动赋值（用 P1.3 的搬运手法），使 `DLL d3(std::move(d2))` 快速且安全。
4. 最后演示 **Rule of 0**：让 `DLL` 只保留 `std::unique_ptr<Node> head_;`（head 是 owner），并让**每个节点自己拥有它的后继**：`std::unique_ptr<Node> next;`。此时拷贝自动被禁止、析构自动级联、移动自动可用——一行特殊成员函数都不用写。

   > ⚠️ **易错点**：`head_` 用 `unique_ptr`、而 `next_` 仍写裸 `Node*` 是**不够的**——那样析构只会 `delete` 头节点，后面整条链全部泄漏（本仓库的测试点 `rule_of_0_destruction_is_cascading` 就是专门抓这个的）。Rule of 0 的前提是**所有权链条本身是完整的**。

**验收**：四步各自能编译/运行，能写出"为什么 `unique_ptr` 成员让 `DLL` 自动变 move-only"；顺带解释：链表中"谁拥有节点"($= \text{head}_$，owning) 与"谁指向节点"($\text{next}_/\text{prev}_$，non-owning view) 的区别——这正是 15-445 到处是 raw pointer 的正当理由。

**自查三问**：
1. 默认拷贝构造把 `head_` 怎么样了？为什么导致 double free？
2. Rule of 0 的前提是什么？（成员自己把拷贝/移动语义"弄对了"）
3. 什么时候你**故意**写 `Node*` 而不是 `unique_ptr<Node>` 当成员？

---
## P3.4 命名空间 mini 库（Namespace + .h/.cpp + include guard + 匿名命名空间）

**对应 / 前置**：读完 `namespaces.cpp`。
**目标**：命名空间组织代码、`.h/.cpp` 分工、include guard、把实现藏进匿名命名空间。

**任务**：建立多文件项目：

```text
mylib/
    geometry.h      // #pragma once；namespace mylib { int Add(int,int); }
    geometry.cpp    // include 后实现；内部助手函数放匿名 namespace
    stats.h
    stats.cpp       // namespace mylib { double Average(const std::vector<int>&); }
main.cpp
```

1. `geometry.cpp` 里放一个只在内部使用的 `static int helper(...)` 或匿名命名空间函数，说明它为何不外泄符号。
2. `main.cpp` 同时调用 `mylib::Add` 与 `mylib::Average`，**不写 `using namespace mylib;`**（保持 `mylib::` 前缀，符合 BusTub 风格）。
3. 故意去掉 `.h` 的 include guard，回答会发生什么（多重包含时的重定义）。

**验收**：CMake 三条 add_library/executable + target_link_libraries 能编出可执行文件；`Add(2,3)==5`、`Average({1,2,3})==2.0`；打印观察匿名命名空间内函数未参与外部链接的说明语。

**自查三问**：
1. `#ifndef`/`#pragma once` 的作用与各自取舍。
2. 匿名命名空间 vs `static` 函数：哪个更现代、为什么？
3. 为什么**不要再**写全局 `using namespace std;`（结合 `A::foo`/`B::foo` 那课的冲突）？