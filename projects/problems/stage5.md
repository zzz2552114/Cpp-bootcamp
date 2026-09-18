# Stage 5 · 智能指针

> **先读完这三课再做题**
> 1. `src/5 - Memory/unique_ptr.cpp` —— 独占所有权、`make_unique`、`std::move` 转移、按引用传参
> 2. `src/5 - Memory/shared_ptr.cpp` —— 共享所有权、`use_count`、拷贝/移动、按值传参
> 3. `src/spring2024/s24_my_ptr.cpp` —— 从一个手写 `Pointer` 讲清"为什么不能拷贝、什么是 `std::move`"
>
> 主线考 `unique_ptr` / `shared_ptr` 的所有权语义。
> 另外两件事测评会用到、src 没展开：`unique_ptr` 的 `release` / `reset` 究竟做了什么
> （以及 `reset(get())` 为什么会 double free），和 `weak_ptr` 如何打破循环引用。放在第 1 节。

---

## 0. 本阶段题目一览

| 题号 | 主题 | 类型 | 你要写的文件（放 `projects/mysol/stage5/`） | 测评点数 |
| :--: | :-- | :--: | :-- | :--: |
| P5.1 | `unique_ptr` 二叉树（ownership tree） | 主线 | `p5_1_binary_tree.h` | 13 |
| P5.2 | 所有权传参：借用 / 观察 / 移交 | 主线+扩展 | `p5_2_ownership.h` | 15 |
| P5.3 | `shared_ptr` 注册表与 `use_count` | 主线 | `p5_3_registry.h` | 12 |
| P5.4 | `weak_ptr` 打破循环引用 | 扩展 | `p5_4_weak_ptr.h` | 10 |

所有题目都**只写头文件，不要写 `main()`**。

---

## 1. 做题之前必须懂的几件事

### 1.1 测评程序怎么用（回顾）

`projects/solutions/stage5/*_test.cpp` 是测评程序，**自带 `main()`**；你只写头文件，不要写 `main()`。
断言宏说明见 `stage1.md` 第 1.1 节。流程：在 `projects/mysol/stage5/` 下写 `.h`，复制测评文件，再编译。

```bash
mkdir -p projects/mysol/stage5
cp projects/solutions/stage5/p5_1_binary_tree_test.cpp  projects/mysol/stage5/
cp projects/solutions/stage5/p5_2_ownership_test.cpp    projects/mysol/stage5/
cp projects/solutions/stage5/p5_3_registry_test.cpp     projects/mysol/stage5/
cp projects/solutions/stage5/p5_4_weak_ptr_test.cpp     projects/mysol/stage5/
```

### 1.2 `unique_ptr`：独占所有权

`std::unique_ptr<T>` 是一个只可移动、不可拷贝的指针包装：它唯一地拥有 `T` 对象，
析构时自动 `delete`。要点（都来自 `unique_ptr.cpp` 和 `s24_my_ptr.cpp`）：

- 用 `std::make_unique<T>(args...)` 创建，比 `unique_ptr<T>(new T(...))` 更安全。
- **不能拷贝**：`std::unique_ptr<T> b = a;` 编译错误（copy 被 `= delete`）。
  想转移所有权必须 `std::unique_ptr<T> b = std::move(a);`，之后 `a` 为空。
- 可以像指针一样用：`if (u)` / `u->member` / `*u`。
- 当成参数传递时，要问自己"这个函数**要不要**拿走所有权"：
  - 只是借用/修改对象 → 传 `std::unique_ptr<T>&` 或 `T*`（不转移所有权）；
  - 要接管所有权 → 按值传 `std::unique_ptr<T>`，调用时 `std::move(up)`。

### 1.3 `release`、`reset` 与 `reset(get())` 的陷阱

`std::unique_ptr` 有两个容易混的成员：

- **`release()`**：交出裸指针、自己变空。**调用者现在负责 `delete`**。它不会释放对象。
- **`reset(p = nullptr)`**：删除当前持有的对象（如果有），然后改为持有 `p`。

标准对 `reset(p)` 的规定顺序是：

1. 记下 `old = get()`；
2. 把内部指针设为 `p`；
3. **如果 `old` 非空，就 `delete old`**。

注意第 3 步**不比较 `old` 和 `p`**。所以 `up.reset(up.get())` 会先记住 `old`（就是当前对象），
把内部指针设成同一个值，然后 `delete old`——对象被删了，可 `up` 手里还捏着指向它的悬垂指针，
之后 `up` 析构时再删一次 → `double free`。
P5.2 有一个测试点会给你一个**安全写法**：先把所有权 `release()` 出来（此时内部变空、`old` 为 `nullptr`），
再 `reset()` 回去。

### 1.4 `shared_ptr`：共享所有权与引用计数

`std::shared_ptr<T>` 允许多个指针共享同一个对象。它内部有一个**控制块**记录引用计数：

- `std::make_shared<T>(...)` 创建，计数为 1；
- **拷贝**一个 `shared_ptr`（拷贝构造/拷贝赋值/按值传参/按值返回）→ 计数 +1；
- 销毁一个 `shared_ptr`，或对它 `reset()` → 计数 -1；
- **移动到另一个 `shared_ptr` 不改变计数**（只是把控制块句柄换个主人，源变空）；
- 计数降到 0 时，对象被析构。

`use_count()` 返回当前计数。注意它主要用于调试：多线程下读到的值天生有竞态，不要拿它做逻辑判断。
另外"对象还活着"和"对象还在某个注册表里"是两回事——一个对象被移出 map 后，
只要还有别的 `shared_ptr` 指着它，它就仍然存活。P5.3 专门考这一点。

### 1.5 `weak_ptr`：不增加计数的"观察者"

`std::weak_ptr<T>` 也指向 `shared_ptr` 管理的对象，但**不增加强引用计数**，因此不会阻止对象析构。
它解决的核心问题是**循环引用**：

```cpp
struct Node { std::shared_ptr<Node> next; };   // A→B→A 成环
```

如果两个对象互相用 `shared_ptr` 指着对方，那么即使外部引用都消失了，它俩的计数也各至少为 1
（对方还指着我），永远不会析构 → **内存泄漏**。把环里**任意一条边**改成 `weak_ptr`，环就断了：
强引用只朝一个方向，另一端只是"观察"，对象就能正常析构。

`weak_ptr` 的用法：

| 操作 | 含义 |
| :-- | :-- |
| `w.expired()` | 对象是否已被析构（强引用数是否为 0） |
| `w.lock()` | 尝试"升级"成 `shared_ptr`：对象还在就返回一个增加计数的 `shared_ptr`，否则返回空 |
| `w.use_count()` | 观察到的强引用数 |
| `w.reset()` | 放弃观察，不影响强引用 |

因为 `lock()` 返回的是 `shared_ptr`，访问对象前先 `if (auto sp = w.lock()) { sp->...; }`，
这样在 `sp` 存活期间对象一定不会消失。

### 1.6 用计数器观察"对象有没有被正确释放"

P5 的测评用 `inline static int live`（见 `stage1.md` 1.5）数"当前存活对象数"：
构造 +1、析构 -1。测试结束时 `live` 回到初始值，就说明所有权处理正确、没有泄漏也没有重复释放。
`weak_ptr` 那一题的计数器是命名空间里的 `g_live`，因为要同时观察好几个类型。

---

## 2. 主线题目（贴合 src 三课）

### P5.1 UniquePtr 二叉树（ownership tree）

- **考什么**：用 `unique_ptr` 表达"谁拥有谁"；树是最自然的 ownership 链。
- **你要写**：`projects/mysol/stage5/p5_1_binary_tree.h`。只写头文件。
- **复制过来的测评文件**：`solutions/stage5/p5_1_binary_tree_test.cpp`。测评点数：13。

**测评程序要求 `namespace tree5` 里提供：**

```cpp
struct Node {
  int value;
  std::unique_ptr<Node> left;    // 节点拥有自己的左右孩子
  std::unique_ptr<Node> right;
  explicit Node(int v);
};

class BinaryTree {
public:
  BinaryTree() = default;
  BinaryTree(BinaryTree&&) noexcept = default;             // 可移动
  BinaryTree& operator=(BinaryTree&&) noexcept = default;

  void Insert(int x);              // 二叉搜索树；重复值放右子树
  bool Contains(int x) const;
  int  Height() const;             // 空树 = 0
  int  Size() const;
  std::vector<int> InOrder() const;// 中序遍历（有序，含重复值）
  bool Empty() const;
  const Node* Root() const;
};
```

**为什么是这些签名：**

- 孩子必须是 `std::unique_ptr<Node>`：这才构成 ownership 链，析构时自动级联释放整棵树，
  不需要任何手写 `delete`。如果改用裸 `Node*` 拥有孩子，就得自己写析构，本题的考点就没了。
- `Insert(int)`：按 BST 规则比较——`x < 当前值` 进左子树，否则进右子树（所以重复值全在右边）。
  测评的 `duplicates_go_to_right_subtree` 会检查插入三个 5 之后树高为 3、中序为 `5 5 5`。
- `Height()` 空树返回 0，否则 `1 + max(左高, 右高)`。
- `Contains` 必须是 const 成员，并且不能修改树。
- `InOrder()` 返回 `std::vector<int>`，中序序列必须有序（含重复）。
- 因为成员是 `unique_ptr`，**拷贝自动被删除、移动自动可用**；测评用 `static_assert` 检查
  `!is_copy_constructible_v<BinaryTree>` 和 `is_move_constructible_v<BinaryTree>`。

**要做的事**：实现 `Node` 和 `BinaryTree`。递归时可以用 `std::unique_ptr<Node>&` 引用表示"某个槽位"，
插入就是给这个槽位赋值。**不要**手写 `delete`，也不要用裸指针拥有孩子。

**测评点在查什么**：空树；单节点；指定例子的高度/大小/中序；任意插入顺序后中序有序；
重复值走右子树；升序插入退化成链（高度 = 元素数）；完全平衡树高度；`Contains` 是 const 且不改变树；
负数与 0；不可拷贝可移动（移动后源为空）；10 万节点无需手写 delete；与 `std::multiset` 随机对拍；
反复插入同一个值。

**编译运行**：

```bash
cd projects/mysol/stage5
g++ -std=c++17 p5_1_binary_tree_test.cpp -I../../solutions -o p5_1 && ./p5_1
```

通过标准：`Result: 13/13 Passed`。

**自查**：① "谁拥有这个节点"是怎么一层层传下去的？② 为什么 `Contains` 标 const 之后递归要传 `const` 引用？③ 拷贝一棵树为什么默认不可行？

---

### P5.2 所有权传入函数：三种姿势（borrow / observe / take）

- **考什么**：区分"借用对象"、"非拥有观察"、"移交所有权"三种传参方式；
  `release` / `reset` 的语义与 `reset(get())` 陷阱（1.3）。
- **你要写**：`projects/mysol/stage5/p5_2_ownership.h`。只写头文件。
- **复制过来的测评文件**：`solutions/stage5/p5_2_ownership_test.cpp`。测评点数：15。

**测评程序要求 `namespace own5` 里提供：**

```cpp
struct Widget {
  inline static int live = 0;      // 当前存活对象数
  int v;
  explicit Widget(int x);          // live++
  ~Widget();                       // live--
};

void Borrow(std::unique_ptr<Widget>& up);   // 只修改对象，不动所有权：up->v += 1
int  Observe(const Widget* raw);            // 非拥有裸指针：raw ? raw->v : -1；绝不 delete
std::unique_ptr<Widget> Take(std::unique_ptr<Widget> up);  // 按值收下所有权，再把它移出去返回
std::unique_ptr<Widget> Make(int v);        // 返回一个新对象的所有权

void ResetKeepingSamePointer(std::unique_ptr<Widget>& up); // 安全地"重置到同一个指针"
int  TakeAndDestroy(std::unique_ptr<Widget> up);            // 按值收下，返回 v（函数结束时销毁）
```

**为什么是这些签名：**

- `Borrow(std::unique_ptr<Widget>&)`：用引用表示"我只借用你的指针，不改它指向谁"；
  函数里 `up->v += 1` 修改对象，但调用者的 `up` 仍持有同一个对象。测评检查 `up` 非空、
  地址不变、`v` 从 10 变成 11。
- `Observe(const Widget*)`：**非拥有**裸指针，函数绝不能 `delete` 它；传 `nullptr` 时返回 -1。
  这是"我只看看、不参与生命周期"的接口。
- `Take(std::unique_ptr<Widget> up)` **按值**接收：调用者必须 `std::move(up)` 才能传进来
  （因为 `unique_ptr` 不可拷贝），函数结束后由被调用者负责释放。测评检查移动后源为空。
- `Make` 返回新所有权，用 `std::make_unique<Widget>(v)`。
- `ResetKeepingSamePointer` 演示 1.3 的安全写法：`Widget* owned = up.release(); up.reset(owned);`。
  注意**不要**写成 `up.reset(up.get())`，那会 double free。
- `TakeAndDestroy` 按值收下，返回 `up->v`，函数返回时 `up` 析构、对象释放。
- `Widget::live` 计数器用来验证每个环节都没有泄漏。

**要做的事**：实现以上 6 个函数。核心是"让所有权关系在函数签名上一眼可见"。

**测评点在查什么**：借用不改所有权；观察不改所有权；观察 nullptr；`std::move` 转移后源为空；
按值接收的被调用者负责释放；`unique_ptr` 不可拷贝可移动；`Make` 产生独立对象；
`release` 后调用者负责删；`reset(new ...)` 释放旧的、接管新的；`ResetKeepingSamePointer` 后仍存活；
`reset()` 释放并变空；`reset` 到不同指针会删旧的；`operator bool`；2000 次移交无泄漏；
最后 `live == 0`。

**编译运行**：

```bash
cd projects/mysol/stage5
g++ -std=c++17 p5_2_ownership_test.cpp -I../../solutions -o p5_2 && ./p5_2
```

（可选）亲眼看 `reset(get())` 的后果——这个参数会真的触发 double free：

```bash
./p5_2 --demo-reset-self     # 预期 free(): double free detected → abort
```

通过标准：不带参数运行时 `Result: 15/15 Passed`。

**自查**：① `Take` 为什么按值接收而不是 `&&`？② 裸指针 `raw` 由谁负责删？③ 为什么 `up.reset(up.get())` 会 double free？安全写法是什么？

---

### P5.3 SharedPtr 注册表（共享所有权 + `use_count`）

- **考什么**：`shared_ptr` 的引用计数如何随拷贝/移动/销毁变化；"对象活着"与"在注册表里"是两回事。
- **你要写**：`projects/mysol/stage5/p5_3_registry.h`。只写头文件。
- **复制过来的测评文件**：`solutions/stage5/p5_3_registry_test.cpp`。测评点数：12。

**测评程序要求 `namespace reg5` 里提供：**

```cpp
struct User {
  inline static int live = 0;
  std::string name_;
  explicit User(std::string n);            // live++
  ~User();                                 // live--
  const std::string& Name() const;
};

class UserRegistry {
public:
  void AddUser(int id, std::string name);          // make_shared + 存进 map
  std::shared_ptr<User> GetUser(int id);           // 找不到返回空
  std::shared_ptr<User> GetUser(int id) const;     // const 版本
  long PeekUseCount(int id) const;                 // 返回 map 内该对象的 use_count；不存在返回 -1
  bool RemoveUser(int id);                         // 删掉返回 true，不存在返回 false
  size_t Count() const;                            // map 大小
  bool Empty() const;
};
```

**为什么是这些签名：**

- `AddUser` 用 `std::make_shared<User>` 创建，并存进内部的 `unordered_map<int, std::shared_ptr<User>>`；
  同一 id 再次 `AddUser` 会覆盖旧值（旧对象若没有别的引用就被析构）。
- `GetUser` **按值返回** `shared_ptr`，所以每调用一次引用计数 +1；找不到返回 `nullptr`。
  测评精确检查：刚 `AddUser` 后 `PeekUseCount == 1`（只有 map 持有）；`GetUser` 一次后为 2；
  取两个副本后为 3。见 1.4。
- `PeekUseCount` 是给测评观察内部计数用的：它**只读 map 里那个 `shared_ptr`**，不会额外增加计数。
- `RemoveUser` 只是从 map 里删掉；如果外面还有人持有（`u1`、`u2`），对象**不会**析构——
  测评会在 `RemoveUser` 后检查 `u1->Name()` 仍可用、`live` 没变。这是本题最重要的考点。
- `User::live` 计数器验证"最后一个 owner 消失时才析构"。

**要做的事**：实现 `User` 和 `UserRegistry`。

**测评点在查什么**：`AddUser` 后计数为 1；`GetUser` 后为 2、两个副本为 3；找不到返回空；
`RemoveUser` 后对象仍被外部 `shared_ptr` 持有（计数为 2、`live` 不变），逐个 `reset` 后计数归零并析构；
注册表析构释放对象；删除不存在的 id 返回 false；同 id 覆盖会释放旧对象；
`std::move` 一个 `shared_ptr` 不改变计数；按值传参函数内计数 +1、返回后恢复；5000 个用户无泄漏；
反复 `GetUser` 计数始终平衡。

**编译运行**：

```bash
cd projects/mysol/stage5
g++ -std=c++17 p5_3_registry_test.cpp -I../../solutions -o p5_3 && ./p5_3
```

通过标准：`Result: 12/12 Passed`。

**自查**：① `shared_ptr` 拷贝和 `std::move` 分别对计数做什么？② 为什么 `RemoveUser` 之后 `u1` 还能用？③ `use_count` 能用来做并发判断吗？

---

## 3. 扩展题目

### P5.4 `weak_ptr` 打破循环引用

- **考什么**：1.5。`shared_ptr` 成环导致泄漏；用 `weak_ptr` 剪断一条边；`lock()` 的安全访问。
- **你要写**：`projects/mysol/stage5/p5_4_weak_ptr.h`。只写头文件。
- **复制过来的测评文件**：`solutions/stage5/p5_4_weak_ptr_test.cpp`。测评点数：10。

**测评程序要求 `namespace weak5` 里提供：**

```cpp
inline int g_live = 0;                 // 当前存活对象数（几个类型共用）
inline void ResetLive();               // g_live = 0

struct BadNode {                       // 全用 shared_ptr → 互相持有，成环
  std::string name;
  std::shared_ptr<BadNode> next;
  explicit BadNode(std::string n);
  ~BadNode();
};

struct GoodNode {                      // 一条边用 weak_ptr → 不成环
  std::string name;
  std::shared_ptr<GoodNode> next;      // 强引用：拥有后继
  std::weak_ptr<GoodNode> prev;        // 弱引用：指回前任，不增加计数
  explicit GoodNode(std::string n);
  ~GoodNode();
};

struct Child;                          // 前置声明（Parent 里要用到 shared_ptr<Child>）
struct Parent {
  std::string name;
  std::shared_ptr<Child> child;        // 父拥有子
  explicit Parent(std::string n);
  ~Parent();
};
struct Child {
  std::string name;
  std::weak_ptr<Parent> parent;        // 子弱引用父（典型父子结构）
  explicit Child(std::string n);
  ~Child();
};
```

**为什么这样设计：**

- `BadNode` 是反面教材：两个对象用 `shared_ptr` 互指，外部引用消失后计数仍 >= 1，永远不析构。
  测评在外部引用释放后检查 `g_live == 2`（泄漏），然后手工断掉一条边确认能级联析构。
- `GoodNode` 把反方向的边改成 `weak_ptr`，外部引用消失后两个对象都能正常析构。
- `Parent`/`Child` 是最常见的实际用法：父拥有子（强），子观察父（弱）。
  测评检查 `child->parent.lock()` 能拿到父、`parent` 先析构后 `child->parent.expired()` 为真。

**要做的事**：实现这 4 个类型，构造函数 `++g_live`、析构函数 `--g_live`。

**测评点在查什么**：纯 `shared_ptr` 成环会泄漏（`g_live` 不降），断一条边后级联析构；
`weak_ptr` 打破环后正常析构；`weak_ptr` 不增加 `use_count`；`lock()` 返回 `shared_ptr` 且期间计数 +1；
对象析构后 `expired()` 为真、`lock()` 返回空；空 `weak_ptr`；`weak_ptr::reset` 不影响强引用；
父子的 lock 访问与过期；2000 次建环清理后 `g_live == 0`。

**编译运行**：

```bash
cd projects/mysol/stage5
g++ -std=c++17 p5_4_weak_ptr_test.cpp -I../../solutions -o p5_4 && ./p5_4
```

通过标准：`Result: 10/10 Passed`。

**自查**：① `weak_ptr` 为什么不增加引用计数？② `lock()` 返回什么，对象已析构时返回什么？③ 除了打破环，`weak_ptr` 还适合什么场景（缓存 / 观察者）？
