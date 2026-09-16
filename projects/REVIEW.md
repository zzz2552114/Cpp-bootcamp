# 对原 Project 计划的评审

> 评审对象：你贴出的那一整套 Project 路线（P1 Reference Playground ～ P14 Mini Buffer Pool，外加最终 MyUniquePtr）。
> 评审标准：**题目是否语意正确、能否编译/跑出目标结果、是否仍在 C++17/Bootcamp 范围内、是否真正覆盖了 Bootcamp 讲授并在 15-445 会用到的概念**。
> 结论先行：这套计划的**方向、顺序、难度控制都很好**，但存在 **4 处硬伤**（其中 2 处会导致 crash/永久卡死），且**漏掉了若干 15-445 必用但计划没讲的概念**，另有 1 个大项目的题意不闭合。下面逐条说。

---

## 一、必须修复的硬伤（不修复会崩溃 / 卡死 / 题意不完整）

### 硬伤 1：Project 5 的 `--iter`（从 `End()` 往回走）会段错误 ⚠️ 最严重

原计划要求：

```cpp
DLLIterator iter = dll.End();
--iter;
cout << *iter;
```

但仓库里 `End()` 返回的是 `DLLIterator(nullptr)`。如果照惯性写：

```cpp
DLLIterator& operator--() { curr_ = curr_->prev_; return *this; }
```

那么第一次 `--iter` 时 `curr_ == nullptr`，`curr_->prev_` 就是**解引用空指针**，我实测直接 **segfault（退出码 139）**。

**为什么 `std::list` 的 `--end()` 能工作而这里不能？** 因为 `std::list` 用了一个**哨兵节点（sentinel）**把首尾连成一个环，`end()` 是哨兵本身，`--end()` 自然回到尾节点。仓库的链表是"`head_ + nullptr` 结尾"的裸实现，没有可供回退的 `tail` 信息。

**修法**（二选一，我在新题里采用第一种）：
1. DLL 维护 `tail_`，迭代器持有 `(curr_, tail_)`，`operator--` 里"若 `curr_==nullptr` 则 `curr_=tail_`，否则 `curr_=curr_->prev_`"。
2. 引入哨兵节点（更接近 `std::list`，但要改 `InsertAtHead`/析构等）。

这正好把一个"看似加减号"的任务升级成"真正理解迭代器的 last/past-the-end 语义"，不是负担而是收获。

### 硬伤 2：Project 2 `MoveOnlyBuffer` 缺 self-move-assignment 防护

计划的接口里 `operator=` 没要求处理 `b = std::move(b)`。但仓库的 `IntPtrManager::operator=` 第一行就是：

```cpp
if (ptr_ == other.ptr_) return *this;
```

如果照计划的"释放自己→接管对方"的顺序写，`b = std::move(b)` 时 `ptr_ == other.ptr_`，会先 `delete ptr_` 再 `ptr_ = other.ptr_`（此时 `other.ptr_` 已经是被 delete 的悬垂指针），结果是把一个**已释放的指针**重新赋给自己，析构时二次 delete → UB。这必须作为一个显式检查点出现在题目里。

### 硬伤 3：Project 12 阻塞队列"测试"不闭合，会永久卡死或无法验证

原计划说"4 producer 4 consumer，每个 producer 生产 10000 个整数，检查总共消费 40000、不丢不重、不卡死"，但**没说消费者何时停、停止信号怎么发**。消费者在 `Pop()` 上 `wait`，队列空时会永久阻塞。你必须给一种终止协议：

- 方案 A：`Close()` 成员函数 + `cv.notify_all()`，`Pop()` 在"已关闭且队列空"时抛出或返回哨兵值。
- 方案 B：main 知道总数，每个消费者固定 pop N 个后退出。

另外"校验不丢不重"需要**额外的同步**（所有消费者往同一个地方登记已消费值），计划完全没写。新题里采用：producers 生产互不重复的 `0..39999`，消费者用一把 mutex 保护的 `vector<bool> seen` 登记，最后断言 40000 个全为 true。

### 硬伤 4：Project 11 银行转账"制造死锁"不可靠且危险

`m_.lock(); other.m_.lock();` 的死锁依赖精确的交错时序：两个线程必须刚好各持一把锁再互相要对方的锁。写不好可能**一次都不死锁**（你误以为没问题），也可能真死锁后程序**永久挂起**只能 Ctrl-C。而且这个版本还藏着一个自转账坑：`a.Transfer(a, 10)` 会对同一个 `std::mutex` 连续 `lock()` 两次 → UB/死锁。

新题改为**可复现**的演示：
1. 在拿锁之间加一个同步屏障（或短 `sleep`），保证两个线程同时停在各持一把锁的位置 → 稳定复现死锁。
2. 单独测 `a.Transfer(a, x)`（自转账），并显式加 `if (this == &other) return;` 防护。
3. 改用 `std::scoped_lock lock(m_, other.m_);`（内部用 `std::lock` 的避免死锁算法 + 多锁 RAII），再讲清楚它不是"魔法"，而是"统一按序尝试获取多个 mutex"。

---

## 二、覆盖缺失（都属于 Bootcamp 正文 / 15-445 必用，但原计划没讲）

按重要性排序：

| # | 缺失概念 | 为什么重要 | 补进哪里 |
|---|---|---|---|
| 1 | **重载解析：`f(T&)` / `f(const T&)` / `f(T&&)` 三兄弟** | 这是理解 `std::move` 为什么有用的**唯一关键**。`move_semantics.cpp` 通篇就在讲"右值引用怎么参与重载选择"，但计划没让读者自己写一组。 | 新增 P1.2 |
| 2 | **`noexcept` 为什么不能省** | `std::vector` 扩容时会用 `move_if_noexcept`：移动构造是 `noexcept` 就移动，否则**拷贝**。这是 BusTub 里 `Page/Tuple` 移动必须标 `noexcept` 的直接原因。计划只在签名里写了 noexcept，从不问为什么。 | 新增 P1.4 |
| 3 | **`auto` 剥离引用与 const；结构化绑定 `auto& [k,v]`** | 计划 P7-D 只挂了个名，没有真任务。`auto` 把 `const T&` 剥成 `T` 是最常见的隐藏拷贝 bug；`for (auto& [k,v] : map)` 是 C++17 遍历 map 的标配写法。 | 把 P7-D 做成 **P4.4 真任务** |
| 4 | **迭代器返回 `int&`（当前仓库返回 `int` 按值），且适配 range-for** | 仓库 `int operator*()` 返回拷贝，无法 `*iter = 5`；总线代码里迭代器几乎都是返回引用，且用 `begin()/end()`（小写）配合 range-for。计划两个都没提。 | P3.2 |
| 5 | **`unordered_map` 自定义键要特化 `std::hash`** | 15-445 里 `unordered_map<PageId, ...>`、把 `std::pair` 当 key 都要自己写 hash。计划 P7/P9 用 map 却从没点这层纸。 | P4.3 |
| 6 | **`operator[]` 会默认插入元素 vs `at()`/`find()`** | `map[key]` 在 key 不存在时**默认构造并插入**——这是最容易踩的坑。 | P4.3 / P4.5 |
| 7 | **lambda 捕获语义 `[]` / `[=]` / `[&]` / `[this]`** | `vectors.cpp` 和 `condition_variable.cpp` 都用了 lambda（条件变量的 `[&]{...}` 尤其常见），计划只说"用 lambda"从不教捕获。 | P4.5、P6.3 |
| 8 | **`weak_ptr` / 循环引用** | 仓库笔记明确给了 `weak_ptr` 对比表，15-445 的图/缓存结构常遇到。计划完全没提。 | 新增 P5.4（可作选做） |
| 9 | **Rule of 0/3/5（一并给学生）** | 仓库 `DLL` 有一个隐式浅拷贝会 double-free，正好是一课。计划零散讲了 move 但没成体系。 | 新增 P3.3 |
| 10 | **拷贝 vs 移动的"性能"实感** | 仓库 motivation 就是性能，计划全是正确性题，没一道性能题。 | 新增 P1.5 |
| 11 | **`vector` 迭代器失效 / `reserve` 与 `size` 的区别** | BusTub bug 高发区（扩容后旧迭代器失效）。 | P4.1 |
| 12 | **模板定义为何要放头文件（编译单元模型）** | 计划 P6 只留了"namespace + .h/.cpp"，没点"模板必须可见于每个 TU"。 | P2.4 |

---

## 三、过于超纲 / 题意不闭合（应砍掉或重写）

### 1. Project 14 "Mini Buffer Pool" 原规格超纲且自相矛盾

原计划：

```cpp
Page* GetPage(int page_id);            // 返回裸指针
// 内部 unordered_map<int, unique_ptr<Page>>, 满时 cv 等待空闲 slot
```

问题：
- 页面被 `unique_ptr` 独占放在 map 里、`GetPage` 又返回裸 `Page*`，**ownership 说不清**；
- "buffer 满时等待空闲 slot"——但没有 **frame 数量、pin/unpin、free list**，什么叫"满"？谁来释放 slot？模型是空的，写下去必然卡住。

这题直接照抄会让人写出一个"既不是 buffer pool 也没有并发语义"的东西。**修法**：改成 15-445 Project 2 的最小骨架、且严格在内存内（不做 eviction 算法、不碰磁盘、不实现 LRU/Clock）：

```cpp
固定 N 个 frame + unordered_map<page_id, frame_id> + 空闲 frame 列表
Pin(page_id)  -> Page*   （空闲 frame 不足时在 cv 上等待）
Unpin(page_id)          （归还 frame 并 notify）
```

这样"有界缓冲 + 条件变量 + ownership"三个点都练到，规模约 150 行，仍属最后一题可承受范围。

### 2. `std::atomic` 一律降级为"选做脚注"

原计划的 P10 让我建议"第四版用 atomic"，但 atomic 不在 Bootcamp 六章范围内、也非 CSAPP 前 8 章必讲。凡涉及计数/标志，**主线全用 mutex + RAII 锁**；仅在 P6.1 末尾给一段"对比：`std::atomic<int>` 一行搞定计数"的选做阅读，不设硬性要求。

### 3. 不引入任何 C++20/23 特性

这是原计划已经做对的（"暂时就 C++17 足够"）。本次所有题与解答**只用 C++17**，不引入 `std::erase_if`、`std::barrier`（C++20）等。构建统一 `-std=c++17 -pthread`。

---

## 四、可优化的地方（让计划更可执行）

1. **给每个项目配"自查三问"**（把原计划末尾的"为什么 noexcept/const/const&/T&/std::move"落实到每题末尾），读者做完对着答，而不是等我看答案。原计划的三遍法很好，我把它固化成每题的固定格式。
2. **全部解答改为自测程序**：每个解答 `main()` 里用 `assert` 自验，跑通即打印 `All tests passed`。这样"对应的答案/结果"就是你运行后的可复现输出，而不必我逐条贴"输入 → 输出"。
3. **提供顶层 CMake**：一次 `cmake && make` 编译 27 个小题，省得手动 `g++` 敲路径。
4. **P2 与 P4 有 90% 重复**（都是"拥有堆内存、删拷贝、可移动"）。把两者**合并/差异化**：P2 专攻移动语义与 self-move，P4 换成"带**观测计数**的资源 + 制造函数"来观察 **RVO / copy elision / noexcept 移动**，并换一种资源（句柄 ID）避免同一份代码写两遍。
5. **P3 模板补上特殊化与非类型模板参数**：仓库 `templated_functions/classes` 明确演示了 `print_msg<float>` 特化与 `template<bool>`/`template<int>`，原计划只让写 `Min`、`Stack`，这块丢了。补成"编译期数组 + 特化打印"两小问。
6. **P1 太薄**：`references.cpp` 只有 9 行，原 P1 只练了"传引用改对象"。补 **`const&` 能绑定右值/左值**这一条（这是 `for(const auto&)` 能遍历临时对象、函数 `const T&` 形参能接右值的原因）。

---

## 五、按上述评审重排后的阶段（与 `problems/`、`solutions/` 对应）

| 阶段 | 源文件（Bootcamp） | 项目 | 相对原计划的动作 |
|---|---|---|---|
| Stage 1 引用与移动 | references / move_semantics / move_constructors | P1.1 引用游乐场；P1.2 重载解析；P1.3 MoveOnlyBuffer；P1.4 noexcept；P1.5 copy-vs-move 基准 | P1 强化、P2 补 self-move、新增 1.2/1.4/1.5 |
| Stage 2 模板 | templated_functions / templated_classes | P2.1 模板函数；P2.2 模板 Stack；P2.3 特化+非类型参数；P2.4 头文件分离+CTAD | P3 拆分补齐特化 |
| Stage 3 包装类/迭代器/命名空间 | wrapper_class / iterator / namespaces | P3.1 RAII+RVO；P3.2 双向迭代器+range-for；P3.3 Rule of 3/5；P3.4 命名空间库 | P4 差异化、P5 修 crash、新增 3.3 |
| Stage 4 容器 | vectors / sets / unordered_maps / auto | P4.1 vector 失效；P4.2 set 比较器；P4.3 自定义 hash；P4.4 auto/结构化绑定；P4.5 Log Analyzer | P7 拆为 5 个真子题 |
| Stage 5 智能指针 | unique_ptr / shared_ptr / s24 | P5.1 UniquePtr 二叉树；P5.2 所有权传参；P5.3 SharedPtr 注册表；P5.4 weak_ptr 环 | P8/P9 强化、新增 5.2/5.4 |
| Stage 6 并发 | mutex / scoped_lock / condition_variable / rwlock | P6.1 安全计数器；P6.2 转账死锁；P6.3 阻塞队列；P6.4 读写锁 map | P10/P11/P12/P13 修复硬伤 3、4 |
| Stage 7 综合 | s24_my_ptr / 全部 | P7.1 MyUniquePtr；P7.2 Mini BufferPool（重写） | 终极题保留、P14 重写 |

每个项目的统一格式见 `README.md`，题目在 `problems/stageN.md`，可编译解答在 `solutions/stageN/`。

---

## 六、实现阶段新发现的坑（补充进计划）

以下是我在**编写并运行解答**时实测发现、值得加进对应题目检查清单的坑：

| # | 坑 | 实测结论 | 加进哪题 |
|---|---|---|---|
| A | **`up.reset(up.get())` 会 double free** | 现行标准对 `reset(p)` 的措辞是"记下 `old=get()` → 设 `get()=p` → 若 `old` 非空就 `delete old`"，**不比较 `old` 与 `p`**（LWG 806 早已移除带 `p==get()` 短路的老措辞）。libstdc++ 实现与此一致。所以 `Reset` 的实现与调用都要小心。 | P7.1（`Reset` 语义）、P5.2（新增测试点 + 可选崩溃演示） |
| B | **`unique_ptr<Node> head_` + 裸 `next` 不构成 ownership 链** | 只把 head 设为 `unique_ptr`、`next` 仍写裸指针时，析构**只**删头节点，其余全泄漏。Rule of 0 要"级联析构"必须让**每个节点拥有其后继**（`unique_ptr<Node> next`）。 | P3.3（原计划的描述有误，已修正） |
| C | **`Sum()` 返回 `int` 在大数据下溢出** | 1..10⁶ 求和 = 5×10¹¹ 远超 `INT_MAX`，属于有符号溢出（UB）。要么改用 64 位，要么把数据规模限制在安全范围。 | P1.1（已补 `SumL()` 与专门测试点） |
| D | **`std::vector` 重分配会让"移动"多算一次** | 在 `Stack::Push` 里数移动次数时，如果没预留容量，`push_back` 触发的重分配也会移动已有元素，导致计数"多 1"。断言要写"拷贝为 0"而不是"移动恰为 1"。 | P2.2（已改为 `copies==0 && moves>=1`） |
| E | **`push_back` 与"元素级代价"的关系** | `auto v2 = std::move(v1)` 对 vector 是**元素级零拷贝、零移动**（只偷缓冲区），而 `auto v2 = v1` 是 n 次元素拷贝。这比"计时"更适合作断言。 | P1.5（已改为元素级计数为主、计时仅作参考） |
| F | **模板无法识别字符串字面量的特化** | `Describe("hi")` 里 `T` 推导为 `char[3]`，**不会**命中 `const char*` 的特化，而是走通用版本；必须先存成 `const char*` 变量或显式写 `Describe<const char*>`。 | P2.3（已作为专门测试点） |
| G | **条件变量里"先检查后等待"会丢状态** | `Pin` 若在 `wait` 之前只 `find` 一次，等待期间该页可能已被别人装入，醒来后覆盖 `page_table_` 条目 → **帧泄漏**（`NumPages()+FreeFrames()` 恒小于 `FrameCount()`）。检查条件必须并入 `wait` 的谓词，或写在谓词后的循环里。 | P7.2（原计划的 P14 简化后正好暴露此坑） |
| H | **复合读取必须在一个临界区里完成** | 分别调用 `NumPages()` 和 `FreeFrames()` 再相加，会因两次加锁之间状态变化而"看起来不满足不变量"。要么提供 `GetStats()` 一次取完，要么持锁后再读。 | P7.2（已加 `GetStats()` + 专门监视测试点） |
| I | **`std::thread` 会拷贝可调用对象** | 用 `[id = 0]() mutable { id++ }` 想让每个线程拿到不同编号是**错的**——每个线程拿到的是同一份初值。必须显式把 index 传进线程函数。 | P6.x（并发题的测试写法陷阱） |
| J | **含 `std::mutex` 的类型不可移动 → 不能放进 `std::vector` 并 `reserve`** | `vector::reserve`/扩容需要移动元素；`Account` 含 `mutex` 会被隐式 delete 掉移动构造，编译报 `result type must be constructible from input type`。改用 `std::deque`（或 `unique_ptr` 容器）。 | P6.2 |
