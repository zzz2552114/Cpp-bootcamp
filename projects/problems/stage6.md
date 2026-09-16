# Stage 6 · 并发同步

对应源文件：`6 - Synch Primitives/mutex.cpp`、`scoped_lock.cpp`、`condition_variable.cpp`、`rwlock.cpp`。
编译统一加 `-pthread`。

---
## P6.1 线程安全计数器（竞态 → lock/unlock → scoped_lock）

**对应 / 前置**：读完 `mutex.cpp` 与 `scoped_lock.cpp`。
**目标**：亲手看到竞态、体会"锁是资源，用 RAII 管理"。

**任务**：实现 `Counter`：

```cpp
class Counter {
public:
    void Increment();          // 线程安全
    int  Get() const;          // 线程安全（锁要 mutable）
private:
    int value_ = 0;
    mutable std::mutex m_;
};
```

三个版本各写一遍（或一个程序里分三段）：
1. **无锁**：`int tmp = value_; std::this_thread::yield(); value_ = tmp + 1;`（把 read 与 write 拆开 + yield，**稳定**复现丢失；直接用 `value_ += 1` 的紧循环在本机反而可能"碰巧"总是 800000——这本身就是"数据竞争不可靠"的教训，见 REVIEW）。
2. **`m_.lock(); ... m_.unlock();`**：正确，但若临界区内抛异常/提前 return，会不会漏解锁？
3. **`std::scoped_lock lk(m_);`**：RAII，作用域结束自动释放。

**验收**：8 线程 × 每线程 10000（版本 1）最终结果**小于** 80000 且每次不同（用 `yield` 版稳定复现）；版本 2/3 恒等于 80000；在版本 2 里故意演示"解锁前 return/抛出"的隐患。

**自查三问**：
1. `Get() const` 里要加锁，为什么 mutex 必须是 `mutable`？
2. 数据竞争是 UB 还是"只是结果算错"？（提示：C++ 内存模型里它是 UB）
3. `std::scoped_lock`、`std::lock_guard`、`std::unique_lock` 三者差异？（结合 P6.3 用 unique_lock 配条件变量）

**[选做] 对比**：把 `value_` 换成 `std::atomic<int>`，用 `fetch_add(1)` 一行完成计数，观察同样正确但锁开销不同。注意 atomic 不在本次主线，仅作对照阅读。

---
## P6.2 银行转账与多锁死锁（scoped_lock 多锁）

**对应 / 前置**：读完 `scoped_lock.cpp`（它支持多把锁）。
**目标**：两个账户互相转账，多锁若顺序不一致 → 死锁；`std::scoped_lock(m_, other.m_)` 规避。

**任务**：

```cpp
class Account {
public:
    explicit Account(int money);
    void Transfer(Account& other, int money);  // 先从本账户扣，再加到 other
    int  Balance() const;                       // 线程安全
private:
    int balance_ = 0;
    mutable std::mutex m_;
};
```

1. **制造死锁（可复现）**：写一个版本 `m_.lock(); 小睡/屏障; other.m_.lock();`。两个线程 `A->B` 与 `B->A` 同时到达、各持一把锁再要对方 → 稳定死锁（卡住不退出）。用"两道锁之间 sleep(10ms)"即可确定性触发。
2. **自转账**：`a.Transfer(a, 10)` 在手动双锁版本会死锁/UB。正确版本先 `if (this == &other) return;`。
3. **正确版**：`std::scoped_lock lock(m_, other.m_);`（内部 `std::lock` 的避免死锁算法，"统一按序尝试获取多把锁"），再校验 `balance_ >= money` 才转账（扣款必须与检查在同一临界区内）。

**验收**：版本 1 复现卡死（跑之前想清楚会卡在哪，可加超时说明）；版本 3：4 线程各转账 10 万次后两个账户总额守恒（`a.balance+b.balance == 初始和`），且自转账安全。

**自查三问**：
1. 死锁四个必要条件里，"不同线程按相反顺序加锁"对应哪一条？
2. `std::scoped_lock lock(a.m_, b.m_)` 为什么不会被相反顺序卡死？它内部大致在做什么？
3. 为什么"检查余额"和"扣款"必须在同一把锁覆盖下，不能先检查后加锁？

---
## P6.3 有界阻塞队列（条件变量 + 关闭协议）

**对应 / 前置**：读完 `condition_variable.cpp`。
**目标**：条件变量正确用法（带谓词的 `wait`）、`notify_one/all`、以及**如何让消费者安全退出**（原计划的漏点，见 REVIEW 硬伤 3）。

**任务**：

```cpp
template <typename T>
class BlockingQueue {
public:
    explicit BlockingQueue(size_t capacity);   // capacity >= 1
    void Push(T value);                        // 满则等待；已关闭则抛出
    T Pop();                                   // 空则等待；已关闭且空则抛出
    void Close();                              // 置 closed，notify_all，之后不可再 Push
    bool Closed() const;
    size_t Size() const;                       // 调试用
private:
    std::queue<T> q_;
    size_t capacity_;
    bool closed_ = false;
    std::mutex m_;
    std::condition_variable cv_not_empty_;
    std::condition_variable cv_not_full_;
};
```

**要求**：
- `Push`/`Pop` 必须用 `cv.wait(lk, 谓词)` **带谓词**版本（防止虚假唤醒/丢失唤醒），谓词里要同时判断 `closed_`。
- `Close()` 要在**不再持有锁时**或先置位再 `notify_all`（两种都要能解释）。之后 `Push` 抛 `std::runtime_error("closed")`，`Pop` 在"已关闭且空"时抛同样异常。

**验收**（多生产者/多消费者 + 无丢无重）：
4 个生产者各 `Push` 10000 个**互不重复**的整数（值域 0..39999），4 个消费者并发 `Pop`。多消费者共用一个由另一把 mutex 保护的 `std::vector<bool> seen(40000)` 登记"我消费到了哪些值"。主线程 join 全部生产者后调用 `Close()`，消费者 `Pop` 直到抛异常退出。最后断言：
- 消费总数 == 40000；
- `seen` 40000 位**全为 true**（既不丢也不重）；
- `Close()` 之后确实能退出、程序不挂死。

**自查三问**：
1. `cv.wait(lk, pred)` 为什么比 `cv.wait(lk)` + 手动 `if` 安全？虚假唤醒/丢失唤醒各自是什么？
2. 为什么条件变量必须和 `std::unique_lock`（而非 `lock_guard`/`scoped_lock`）配合？`wait` 时锁发生了什么（原子地解锁并挂起）？
3. `notify_one` vs `notify_all` 何时选哪个？持锁时 notify 有什么隐患？

---
## P6.4 读写锁并发 Map（shared_mutex / shared_lock / unique_lock）

**对应 / 前置**：读完 `rwlock.cpp`。
**目标**：读多写少场景，用 `std::shared_mutex`：读共享、写独占。

**任务**：

```cpp
class ConcurrentMap {
public:
    void Put(int key, int value);            // 写：独占锁
    bool Get(int key, int& value) const;     // 读：共享锁
    void Remove(int key);                    // 写：独占锁
    size_t Size() const;
private:
    std::unordered_map<int,int> map_;
    mutable std::shared_mutex m_;
};
```

- `Get` 用 `std::shared_lock lk(m_)`；`Put/Remove` 用 `std::unique_lock lk(m_)`。为什么 `m_` 仍要 `mutable`？
- **考察读者并行度**：在测试里维护 `std::atomic<int> active_readers`, `max_readers`。n 个读者在 `Get` 里 `++active_readers`、更新 `max_readers`、`sleep(5ms)`、`--active_readers`。启动 8 读者 + 2 写者。运行后：`max_readers` 通常 ≥2（验证共享读），且所有读到的值自洽、写者写完后最终状态正确。
- 说明：`std::shared_mutex` 是 C++17 提供（C++14 只有 `std::shared_timed_mutex`），本次就用 C++17 版。

**验收**：Put 一组键值后：并发 8 读得到一致快照；2 写者各加若干次后 Size 正确、无数据损坏；打印 `max_readers` 观察共享读；能说明"写锁与所有读锁互斥、读锁之间可共享"。

**自查三问**：
1. `shared_lock`（读）与 `unique_lock`（写）在 `shared_mutex` 上分别什么语义？
2. 为什么不能让升级（读锁直接变写锁）？读写锁一般靠什么避免死锁/活锁？
3. `mutable` 在本类的确切作用是什么？