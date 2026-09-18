# Stage 7 · 综合（Capstone）

> **先读完这一课再做题**
> 1. `src/spring2024/s24_my_ptr.cpp` —— 从零手写一个 `Pointer<T>`，串起移动语义、RAII、运算符重载
>
> 并且本阶段默认你已经做完 Stage 1 ~ 6。
> 两题：P7.1 手写 `MyUniquePtr`（把前面的模板 / RAII / 移动 / 运算符重载全用上）；
> P7.2 迷你 BufferPool（把前面所有东西加上并发与条件变量，做一个 15-445 Project 2 的最小内存版）。
>
> 第 1 节先补两块知识：**完美转发与变参模板**（P7.1 的 `MyMakeUnique` 要用），
> 以及 **buffer pool 的帧/页表模型**和两个并发陷阱（P7.2 会踩）。

---

## 0. 本阶段题目一览

| 题号 | 主题 | 类型 | 你要写的文件（放 `projects/mysol/stage7/`） | 测评点数 |
| :--: | :-- | :--: | :-- | :--: |
| P7.1 | 手写 `MyUniquePtr` | 主线 | `p7_1_my_unique_ptr.h` | 19 |
| P7.2 | Mini BufferPool | 扩展 | `p7_2_mini_buffer_pool.h` | 16 |

两题都**只写头文件，不要写 `main()`**；P7.2 编译要加 `-pthread`。

---

## 1. 做题之前必须懂的几件事

### 1.1 测评程序怎么用（回顾）

`projects/solutions/stage7/*_test.cpp` 是测评程序，**自带 `main()`**；你只写头文件，不要写 `main()`。
断言宏说明见 `stage1.md` 第 1.1 节。流程：在 `projects/mysol/stage7/` 下写 `.h`，复制测评文件，再编译。

```bash
mkdir -p projects/mysol/stage7
cp projects/solutions/stage7/p7_1_my_unique_ptr_test.cpp    projects/mysol/stage7/
cp projects/solutions/stage7/p7_2_mini_buffer_pool_test.cpp projects/mysol/stage7/
```

### 1.2 完美转发与变参模板（`MyMakeUnique` 要用）

`std::make_unique<T>(args...)` 能在堆上直接构造 `T`，把 `args` 原样转发给 `T` 的构造函数，
中间不做任何拷贝。它靠两样东西实现：

- **变参模板**：`template <typename T, typename... Args>`，`Args` 是"任意多个类型"的包，
  `args` 是"任意多个值"的包。
- **完美转发**：形参写成 `Args&&... args`（转发引用），再通过 `std::forward<Args>(args)...` 传下去。
  `std::forward` 会**保持每个参数原本的值类别**：传进来的是左值就继续当左值传，是右值就当右值传。
  如果用 `std::move` 一律当右值，就可能把调用者还想保留的左值给搬空；直接按值传又会多一次拷贝。

```cpp
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
```

P7.1 会让你写一个等价的 `MyMakeUnique<T>(args...)`。

### 1.3 `explicit operator bool`、`Reset`、`Swap`

- `explicit operator bool() const`：让 `if (p)`、`!p` 可用，但**禁止**隐式转成整数
  （不加 `explicit` 的话 `p + 1` 之类会莫名其妙编译过）。所以测评要求它是 `explicit`。
- `Reset(T* p = nullptr)`：删除当前持有的对象，改为持有 `p`。
  这里**和 `std::unique_ptr::reset` 不同**：本题要求你实现的版本显式做 `if (p == ptr_) return;` 短路，
  这样 `p.Reset(p.Get())` 是安全的（标准库那个会 double free，见 `stage5.md` 1.3）。
- `Release()`：交出裸指针、自己变空，**不删除对象**；调用者接住后负责 `delete`。
- `Swap(MyUniquePtr& other)`：交换两个指针，常用 `std::swap(ptr_, other.ptr_)`。
- 移动构造/赋值必须 `noexcept`；移动赋值要有 self-move 防护；移动后源为 `nullptr`。

### 1.4 BufferPool 的模型：帧、空闲帧表、页表

15-445 的 buffer pool 是"磁盘页在内存里的缓存"。本题是它的**最小内存版**，只保留三样东西：

- **帧（frame）**：一块固定大小的内存，能装一个页。用 `std::vector<Page> frames_` 表示，
  数量固定（`frame_count`），这就是池子的容量。
- **空闲帧表（free list）**：记录哪些帧现在是空的，可以用。初始化时所有帧都在里面。
- **页表（page table）**：记录"某个 `page_id` 现在装在哪个帧里"，即 `unordered_map<int, size_t>`。

三个操作：

- `Pin(page_id)`：如果页表里已经有这页，直接返回它的帧；否则从空闲帧表拿一个帧装进这页，
  记入页表并返回。没有空闲帧时，**等待**别人 `Unpin`。
- `Unpin(page_id)`：这页用完了，把它的帧还回空闲帧表、从页表里删掉，并唤醒等待者。
- `NumPages()` / `FreeFrames()`：观察当前驻留页数和空闲帧数。

**范围声明**：本题**不做**磁盘 I/O、**不做**替换策略（LRU/Clock）、**不实现 pin count**。
"等待"只发生在"没有空闲帧"的时候。

### 1.5 两个并发陷阱（P7.2 会专门考）

**陷阱一：等待的谓词里必须包含"这页已经被别人装进来了"。**

如果你在 `wait` 之前只 `find` 一次，就下结论"这页不在池里"：

```
线程 A：find(page 5) → 不在 → wait（等空闲帧）
线程 B：把 page 5 装进池里 → Unpin？不，继续用 → notify
线程 A：醒来 → 分配一个帧 → page_table_[5] = 新帧   ← 覆盖了 B 的记录！
```

被覆盖的那个帧既不在 `free_list_` 里、也不在 `page_table_` 里，**永久丢失**（帧泄漏）。
判断现象是 `NumPages() + FreeFrames() != FrameCount()`。正确做法是把两个条件合并进 `wait` 的谓词：

```cpp
cv_.wait(lk, [&]{ return page_table_.count(page_id) > 0 || !free_list_.empty(); });
auto it = page_table_.find(page_id);
if (it != page_table_.end()) return &frames_[it->second];   // 等待期间被别人装进来了
// 否则一定有空闲帧
```

**陷阱二：要同时读两个量时，必须在同一个临界区里一次读完。**

`NumPages()` 和 `FreeFrames()` 各自加锁。如果你分别调用它们再相加，两次加锁之间状态可能已经变了，
于是"看起来"不满足不变量——那不是数据坏了，是**观测方式**错了。所以本题提供一个
`GetStats()`，在**一次加锁**里同时返回两个数。

**关于返回的 `Page*`**：本简化版**没有 pin count**，所以调用者拿到 `Page*` 后如果别的线程
`Unpin` 了这一页、帧又被新页复用，指针指向的内容就会变。真实 buffer pool 必须有 pin count + latch
来保证"被 pin 的页不会被回收"，这是留给 15-445 Project 2 的功课。测评里有一个测试点专门演示这个局限。

---

## 2. 主线题目

### P7.1 手写 `MyUniquePtr`（template + RAII + move + 运算符重载）

- **类型**：主线
- **考什么**：把 Stage 1 ~ 5 的概念浓缩进一个类模板：RAII、delete 拷贝、noexcept 移动、self-move、
  `release`/`reset`/`swap`、运算符重载、`explicit operator bool`，再加变参模板 + 完美转发（1.2）。
- **你要写**：`projects/mysol/stage7/p7_1_my_unique_ptr.h`。只写头文件。
- **复制过来的测评文件**：`solutions/stage7/p7_1_my_unique_ptr_test.cpp`。测评点数：19。

**测评程序要求 `namespace myptr7` 里提供：**

```cpp
template <typename T>
class MyUniquePtr {
public:
  constexpr MyUniquePtr() noexcept = default;          // 空指针
  explicit MyUniquePtr(T* ptr) noexcept;               // 接管裸指针
  ~MyUniquePtr();                                      // delete 持有的对象

  MyUniquePtr(MyUniquePtr&& other) noexcept;           // 接管 + 源置空
  MyUniquePtr& operator=(MyUniquePtr&& other) noexcept;// self-move 防护 + 释放旧的 + 接管
  MyUniquePtr(const MyUniquePtr&) = delete;
  MyUniquePtr& operator=(const MyUniquePtr&) = delete;

  T&   operator*() const;                              // *p
  T*   operator->() const;                             // p->
  T*   Get() const noexcept;
  explicit operator bool() const noexcept;             // if (p) / !p
  T*   Release() noexcept;                             // 交出裸指针、自己变空
  void Reset(T* p = nullptr) noexcept;                 // 删旧的、接管新的；p == Get() 时短路
  void Swap(MyUniquePtr& other) noexcept;
};

// 等价于 std::make_unique：变参 + 完美转发
template <typename T, typename... Args>
MyUniquePtr<T> MyMakeUnique(Args&&... args);
```

**为什么是这些签名：**

- `explicit operator bool`：见 1.3。`explicit` 防止隐式转整数。
- 移动构造/赋值 `noexcept`：测评有 `static_assert`；而且 `MyUniquePtr` 要能放进 `std::vector`。
- `Reset` 必须对 `p == ptr_` 短路：测评的 `reset_to_same_pointer_is_safe_here` 会传 `p.Get()`，
  若按标准库 `reset` 的语义写就会 double free。见 1.3。
- `MyMakeUnique` 用 `new T(std::forward<Args>(args)...)`，不能引入任何拷贝。见 1.2。
  测评对它计数：`Foo::copies == 0`。
- `Foo` 的生命周期用 `live` 计数（构造 +1、析构 -1），测试结束时必须归零，证明没有泄漏或重复释放。

**要做的事**：实现 `MyUniquePtr<T>` 和 `MyMakeUnique`；`operator*` 返回 `T&`、`operator->` 返回 `T*`。

**测评点在查什么**：默认构造为空；接管指针后 `*p`/`p->` 可用、析构自动删除；`operator*` 返回引用；
`MyMakeUnique` 无拷贝（含多参构造如 `std::string(5,'x')`）；移动构造转移且源为空；
移动赋值释放旧资源；self-move 安全；拷贝被删且移动 `noexcept`；`Reset(new)` / `Reset()`；
`Reset(同指针)` 安全；`Release` 后调用者负责删；`Swap`（有值/空）；放进 `vector`；
链式移动；5000 次 `Reset` 无泄漏；5000 次移动无泄漏；生命周期与 `std::unique_ptr` 一致。

**编译运行**：

```bash
cd projects/mysol/stage7
g++ -std=c++17 p7_1_my_unique_ptr_test.cpp -I../../solutions -o p7_1 && ./p7_1
```

通过标准：`Result: 19/19 Passed`。

**自查**：① `operator=` 里的 self-move 防护为什么不能省？② `MyUniquePtr<T>` 缺什么才能支持数组（对比 `unique_ptr<T[]>`）？③ `explicit operator bool` 的 `explicit` 为了什么？

---

## 3. 扩展题目

### P7.2 Mini BufferPool（固定帧 + free list + page table + 条件变量）

- **类型**：扩展（第 1.4、1.5 节）

- **考什么**：把前面的 RAII / 容器 / 并发 / 条件变量全用上；学会"等待谓词必须覆盖所有导致状态变化的路径"
  和"复合读取必须在一个临界区里完成"（1.5 的两个陷阱）。
- **你要写**：`projects/mysol/stage7/p7_2_mini_buffer_pool.h`。只写头文件。
- **复制过来的测评文件**：`solutions/stage7/p7_2_mini_buffer_pool_test.cpp`。测评点数：16。

**测评程序要求 `namespace bp7` 里提供：**

```cpp
struct Page {
  int page_id = -1;
  std::array<int, 8> data{};       // 随便塞点 payload，测评会读写 data[i]
};

class MiniBufferPool {
public:
  explicit MiniBufferPool(size_t frame_count);   // frame_count >= 1
  Page*  Pin(int page_id);        // 命中返回该帧；否则占一个空闲帧；没有空闲帧就等待
  void   Unpin(int page_id);      // 回收该页的帧并唤醒等待者；页不在池里则无害返回
  size_t NumPages() const;        // 当前驻留页数
  size_t FreeFrames() const;      // 当前空闲帧数
  size_t FrameCount() const;      // 总帧数
  bool   HasPage(int page_id) const;

  struct Stats { size_t pages; size_t free_frames; };
  Stats  GetStats() const;        // 在一次加锁里同时取两个量（见 1.5 陷阱二）
};
```

**为什么是这些签名：**

- `Page` 要有公开的 `page_id` 和 `data`（`int[8]`），测评直接读写。新装入的页要把 `data` **清零**。
- `Pin` 必须处理"页已在池里 → 返回同一个帧"，并且 `wait` 谓词要同时包含
  "页已存在" 和 "有空闲帧" 两个条件（1.5 陷阱一）。
- `Unpin` 对不存在的页要无害返回；同一页 `Unpin` 两次不应该多还一个帧。
- `NumPages()` / `FreeFrames()` 各自加锁。
- `GetStats()` 在**一次加锁**里返回 `{page_table_.size(), free_list_.size()}`，
  这样 `pages + free_frames == FrameCount()` 这个不变量在任何时刻都成立（1.5 陷阱二）。
- `FrameCount()` 是固定的 `frame_count`，不需要加锁（构造后不变）。
- 共享状态用一把 `std::mutex` + 一个 `std::condition_variable` 保护；`NumPages/FreeFrames/GetStats/HasPage`
  是 const 成员，所以 mutex 要 `mutable`。

**要做的事**：实现 `Page` 和 `MiniBufferPool`。内部用固定长度的帧数组 + 空闲帧列表 + `page_id → 帧下标`
的映射；等待用条件变量。

**测评点在查什么**：新池为空；`Pin` 新页分配帧；`Pin` 已存在页返回同一帧且数据保留；
新页数据清零；不同页数据互不干扰；`Unpin` 归还帧；`Unpin` 未知页无害；`Unpin` 两次无害；
装满所有帧；仅 1 帧时复用；满时 `Pin` 阻塞、`Unpin` 后恢复；
单线程随机 Pin/Unpin 与参考模型逐步对拍；8 个 worker 并发用完归还后帧数为 0；
监视线程确认 `pages + free == FrameCount` 恒成立；单帧高竞争；以及"没有 pin count 时
持有 `Page*` 不安全"这条局限的演示。

**编译运行**：

```bash
cd projects/mysol/stage7
g++ -std=c++17 -pthread p7_2_mini_buffer_pool_test.cpp -I../../solutions -o p7_2 && ./p7_2
```

通过标准：`Result: 16/16 Passed`。

**自查**：① `Unpin` 为什么要在锁内改 `free_list_` 再 `notify`？`wait` 的谓词为什么不能只写"空闲帧非空"？② 返回 `Page*` 给调用者、同时别的线程可能回收该帧——谁该负责？真实实现需要什么？③ 这个池的"等待空闲帧"和 P6.3 阻塞队列的"等待空位"有什么异同？
