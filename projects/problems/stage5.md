# Stage 5 · 智能指针

对应源文件：`5 - Memory/unique_ptr.cpp`、`shared_ptr.cpp`、`spring2024/s24_my_ptr.cpp`。

---
## P5.1 UniquePtr 二叉树（ownership tree）

**对应 / 前置**：读完 `unique_ptr.cpp`。
**目标**：`unique_ptr` 表达"谁是 owner"，树就是自然的 ownership 链。

**任务**：

```cpp
struct Node {
    int value;
    std::unique_ptr<Node> left;   // Node 拥有自己的左右孩子
    std::unique_ptr<Node> right;
    explicit Node(int v);
};

class BinaryTree {
public:
    void  Insert(int x);          // 建 BST（可重复值：放右子树）
    bool  Contains(int x) const;
    int   Height() const;         // 空树 0
    int   Size() const;
    void  PrintInOrder() const;
private:
    std::unique_ptr<Node> root_;
};
```

- **不允许**手写 `delete`，也不允许成员用裸 `Node*` 拥有孩子。插入用递归 `unique_ptr<Node>&` 定位（或迭代 + `unique_ptr<Node>*` 指针）。
- 说明为什么 `Node` 里的孩子用 `unique_ptr`，而 parent 若要用的话只能是 **non-owning** `Node*`。

**验收**：插入 `5 3 7 1 4 6 8` → `Contains(4)==true`、`Contains(9)==false`、`Height()==3`、`Size()==7`、中序 `1 3 4 5 6 7 8`；析构无泄漏（可用 valgrind/ASan 佐证，非必需）。

**自查三问**：
1. "谁拥有这个 Node"的链是怎么传递的？root 拥有谁，谁拥有叶子？
2. 为什么 `Contains` 标 `const` 后，递归需要 `const Node*`（或 `const unique_ptr<Node>&`）？`unique_ptr` 本身能不能拷贝传进去？
3. 拷贝一棵树为什么默认不可行？要拷贝你需要写什么？

---
## P5.2 所有权传入函数：三种姿势（borrow / non-owning / take）

**对应 / 前置**：`s24_my_ptr.cpp` 的 `not_take_ownership` / `take_ownership`，以及 `unique_ptr.cpp` 的 `SetXTo445`。
**目标**：把一个 `unique_ptr` 传给函数时，明确区分"借用"和"移交所有权"。

**任务**：定义：

```cpp
void Borrow(std::unique_ptr<Widget>& up);          // 只改对象，不改所有权
void Observe(const Widget* raw);                    // 非拥有裸指针：绝不 delete
std::unique_ptr<Widget> Make();                      // 返回新所有权
void Take(std::unique_ptr<Widget> up);              // 按值收下，接管所有权
```

在 `main()` 里演示并回答：
1. `Borrow(up)` 之后 `up` 是否还有效？
2. `Observe(up.get())` 之后 `up` 是否还有效？为什么函数内**不能** `delete raw`？
3. `Take(std::move(up))` 之后 `up` 是空吗？为什么普通 `Take(up)` 编译不过（`= delete` 拷贝）？
4. `auto w = Make();` 中间有几次移动/构造？（参考 P3.1 的 elision 结论）
5. `up.release()` 与 `up.reset(new Widget)` 与 `up.reset()` 各自语义。

**验收**：打印（或断言）每一步之后 `up` 的 `()!=nullptr` 状态，形成一张"传参前后的所有权表"。

**自查三问**：
1. `void Take(std::unique_ptr<Widget> up)` 为什么用"按值"而不是 `&&`？按值能同时接受左值（先拷贝——但此类型不可拷贝，故实际只能移动）和右值吗？
2. `raw` 裸指针与"非拥有"的含义：谁负责删它？
3. `release()` 泄漏资源的场景是什么？谁该接住它返回的裸指针？

---
## P5.3 SharedPtr 注册表（共享所有权 + use_count）

**对应 / 前置**：读完 `shared_ptr.cpp`。
**目标**：多个 `shared_ptr` 共享同一对象；引用计数随副本增减；对象在最后一个 owner 释放时才析构。

**任务**：

```cpp
class User { std::string name_; public: explicit User(std::string n); const std::string& Name() const; };
class UserRegistry {
public:
    void AddUser(int id, std::string name);          // make_shared + 存入 map
    std::shared_ptr<User> GetUser(int id);            // 找不到返回空 shared_ptr
    void RemoveUser(int id);                          // 从 map 移除
    size_t Count() const;                             // map 大小
private:
    std::unordered_map<int, std::shared_ptr<User>> users_;
};
```

验收（把 `use_count` 的精确数字打出来）：
1. `AddUser(1,"alice")` 后 map 内该对象计数为 1。
2. `auto u1 = GetUser(1); auto u2 = GetUser(1);` → 此时计数为 **3**（map + u1 + u2）。解释为什么"按值返回"会让计数 +1。
3. `RemoveUser(1)` → map 里没了、`Count()==0`，但 `u1`、`u2` 还活着 → 对象**尚未析构**、`use_count()==2`、`u1->Name()` 仍可用。
4. `u1.reset(); u2.reset();` → 计数归零，对象析构（给 User 加析构打印证明）。

**自查三问**：
1. `shared_ptr` 拷贝构造/赋值如何修改控制块计数？`std::move` 一个 `shared_ptr` 会使计数改变吗（对比仓库 `shared_ptr.cpp` 里 `s6=std::move(s5)` 的注释）？
2. "对象活着"和"在 registry 里"是两件事——为什么 `RemoveUser` 后 `u1` 还能用？
3. 多线程里频繁读 `use_count()` 可靠吗？（提示：它本身就是竞态窗口；仅用于调试）

---
## P5.4 weak_ptr 打破循环引用 [选做]

**对应 / 前置**：`shared_ptr.cpp` + 笔记里 `weak_ptr` 的对比表。
**目标**：`shared_ptr` 成环 → 永远不释放；用 `weak_ptr` 剪断一条边。

**任务**：构造一个成环结构：

```cpp
struct Node {
    std::shared_ptr<Node> next;   // 先全部用 shared_ptr
    std::string name;
    ~Node();                       // 打印 "~Node name" 证明是否析构
};
auto a = std::make_shared<Node>(); a->next = b; b->next = a; // 环
```

1. 观察：main 结束时 `~Node` **未**打印（环让引用计数永远 ≥1）→ 泄漏。
2. 把环的**某一条边**改成 `std::weak_ptr<Node>`，`~Node` 应正常打印。
3. 用 `weak_ptr` 访问时先 `if (auto sp = wp.lock())`，说明为什么 `lock()` 前要判空、`lock()` 返回什么。

**验收**：能复现"有 weak 边 → 正常析构，无 → 沉默泄漏"，并写出"环 + 纯 shared_ptr 永远不释放"的原因。

**自查三问**：
1. `weak_ptr` 为什么不增加引用计数？它指向控制块还是对象？
2. `wp.lock()` 返回什么？对象已析构时返回什么？
3. 什么场景你会选 `weak_ptr`（缓存？观察者？打破环？）