# Stage 7 · 综合（Capstone）

对应源文件：`spring2024/s24_my_ptr.cpp`（应先读完 `move_semantics.cpp`、`move_constructors.cpp` 再读它）+ 全部前六章。

---
## P7.1 手写 `MyUniquePtr`（串起 template + RAII + move + 运算符重载）

**对应 / 前置**：`s24_my_ptr.cpp` 送你一个简化版 `Pointer<T>`。不要照抄，自己从头写完整版。
**目标**：把 Stage 1~5 全部概念浓缩进一个类。

**任务**：

```cpp
template <typename T>
class MyUniquePtr {
public:
    MyUniquePtr() noexcept;                       // 空指针
    explicit MyUniquePtr(T* ptr) noexcept;        // 接管裸指针
    ~MyUniquePtr();                               // delete 持有的对象

    MyUniquePtr(MyUniquePtr&& other) noexcept;
    MyUniquePtr& operator=(MyUniquePtr&& other) noexcept;
    MyUniquePtr(const MyUniquePtr&) = delete;
    MyUniquePtr& operator=(const MyUniquePtr&) = delete;

    T&  operator*() const;                        // 空则 UB（可 assert）
    T*  operator->() const;
    T*  Get() const;
    explicit operator bool() const;               // if (p) 可用
    T*  Release() noexcept;                       // 交出裸指针、自己变空
    void Reset(T* ptr = nullptr) noexcept;        // 删旧的、接管新的
    void Swap(MyUniquePtr& other) noexcept;
private:
    T* ptr_ = nullptr;
};
```

**必须覆盖**：self-move（`p = std::move(p)` 安全）；移动后源为 `nullptr`；`Release` 返回裸指针后调用者负责删除；`Reset()` 释放旧对象。写一个 `MyMakeUnique<T>(Args&&...)`（用 `new T(std::forward<Args>(args)...)`）体验转发 + 变参模板。

**验收**：
- `MyUniquePtr<Foo> p = MyMakeUnique<Foo>(42); p->Get()==42; (*p).Get()==42;`
- `auto q = std::move(p);` 后 `!p` 且 `q` 拥有对象；
- `p.Reset(new Foo(1)); p.Reset();` 无泄漏（给 Foo 加构造/析构打印计数，main 结束时计数归零）；
- `auto r = q.Release(); delete r;` 后 `q` 为空。

**★ 最容易写错的地方（我实测踩到了）**：

`Pin` 里**不能**只在 `wait` 之前 `find` 一次就下结论"这页不在池里"。因为 `wait` 会释放锁，等待期间别的线程可能已经把这页装进池里；醒来后你再分配一个帧并执行 `page_table_[page_id] = fid`，就会**覆盖**掉原来那条记录——被覆盖的那个帧既不在 `free_list_` 也不在 `page_table_` 里，于是**帧泄漏**（表现为 `NumPages() + FreeFrames() != FrameCount()`，且差值越来越大）。

正确写法是把"这页已经存在"也纳入等待谓词：

```cpp
cv_.wait(lk, [this, page_id] {
  return page_table_.count(page_id) > 0 || !free_list_.empty();   // 有活可干才醒
});
auto it = page_table_.find(page_id);
if (it != page_table_.end()) return &frames_[it->second];         // 等待期间被装进来了
// 否则一定有空闲帧
```

> 另外：想"同时读两个量"（例如同时要 `NumPages()` 和 `FreeFrames()`）必须**在一个临界区里一次取完**；分别调用两个加锁的函数，中间会被别人改状态，你会发现 `NumPages() + FreeFrames() != FrameCount()`——那是**观测方式**错了，不是数据坏了。

**自查三问**：
1. `operator=` 里 self-move 防护为什么不能省？`if (this == &other) return *this;` 与 `pk ptr_ == other.ptr_` 哪个更稳？
2. `MyUniquePtr<T>` 能不能像 `std::unique_ptr<T[]>` 那样支持数组？缺了什么（`delete[]`、`operator[]`）？（不要求实现，但要能说出差异）
3. `explicit operator bool` 的 `explicit` 是为了什么？

---
## P7.2 Mini BufferPool（固定帧 + free list + page table + 条件变量）

**对应 / 前置**：全部完成。这是**改写后的**原 Project 14：删掉空白无根的"满则等待"，换成 15-445 Project 2 的最小内存版骨架。
**范围声明**：**不做**磁盘读写、**不做**替换策略（LRU/Clock）、**不做**真正的 page_id/磁盘页。Page 只是内存里的一个 `struct`。

**任务**：

```cpp
struct Page { int page_id; std::array<int, 8> data; /* 随便塞点数据 */ };

class MiniBufferPool {
public:
    explicit MiniBufferPool(size_t frame_count);   // frame_count >= 1
    ~MiniBufferPool();

    Page* Pin(int page_id);        // 已存在则返回其帧；否则分配空闲帧，无空闲帧则等待
    void  Unpin(int page_id);      // 归还该页使用（本简化版：释放该页）并 notify 等待者
    size_t NumPages() const;       // 当前驻留页面数
private:
    // 固定数的帧 + 空闲帧索引列表
    // 简化：用 unordered_map<page_id, Page> 存驻留页 + 计数上限 + cv 即可（见下）
};
```

**实现约束**（保持简单但语义正确）：
- 用 `std::vector<Page>` 当固定帧 + `std::vector<size_t> free_list`；`std::unordered_map<int,size_t> page_table`（page_id → 帧下标）。
- `Pin` 在 `page_table` 命中时返回帧指针；未命中时若 `free_list` 空则 `cv.wait(lock, [&]{ return !free_list.empty(); })`；取一空闲帧填入。
- `Unpin` 从 `page_table` 移除该页、把帧归还 `free_list`、`cv.notify_one()`。
- 所有共享状态用一把 `std::mutex` + 一个 `std::condition_variable` 保护（`m_` 要 `mutable` 或把 `NumPages` 也加锁）。

**验收**（单线程先行，再并发）：
1. 单线程：`frame_count=3`，依次 `Pin(1,2,3)`，第 4 个 `Pin(4)` 应**阻塞**（可另起线程 `Unpin` 后恢复）；`Unpin(2)` 后 `Pin(4)` 成功；`NumPages()` 正确。
2. 并发：主线程 `Pin` 一批页面，若干工作线程各自 `Pin` 后 `Unpin`，最终 `NumPages()==0`、无死锁、无崩溃；每页数据可独立读写不串扰。
3. 说明 `Pin` 返回的 `Page*` 是否安全（调用者在池锁外使用帧是否可能被别的线程 `Unpin` 抢走？——指出本简化版**没有引用计数**，真实 buffer pool 必须有 pin count，这是留给 15-445 P2 的功课）。

**自查三问**：
1. 为什么 `Unpin` 要在锁内修改 `free_list` 再 `notify_one`？`wait` 的谓词为什么是"空闲帧非空"而不是"我上次见过的状态"？
2. 返回 `Page*` 给调用者、同时池里线程可能回收该帧——这里面 ownership/并发谁负责？真实实现需要什么（pin count / latch）？
3. 把这个池的"等待有空闲帧"与 P6.3 的有界阻塞队列"等待有空位"对照：两者 condition variable 用法有何异同？