# Stage 6 · 并发同步

> **先读完这四课再做题**
> 1. `src/6 - Synch Primitives/mutex.cpp` —— `std::mutex` 的 `lock` / `unlock`
> 2. `src/6 - Synch Primitives/scoped_lock.cpp` —— RAII 风格的 `std::scoped_lock`
> 3. `src/6 - Synch Primitives/condition_variable.cpp` —— 条件变量与带谓词的 `wait`
> 4. `src/6 - Synch Primitives/rwlock.cpp` —— `shared_mutex` / `shared_lock` / `unique_lock`
>
> src 演示了"怎么上锁"。测评还会考几件并发里必须懂的事：**数据竞争是 UB 而不只是算错**、
> 三种锁（`lock_guard`/`unique_lock`/`scoped_lock`）的分工、**多锁死锁**与规避、
> 条件变量的**虚假唤醒/丢失唤醒**与**关闭协议**。第 1 节讲清这些，再做题目。
>
> 另外，本阶段会用到而 src 没讲的还有：`std::thread` 的规矩（不 `join` 就崩）、
> `std::atomic`（计数器的另一条路）、`std::optional`（P6.4 的返回类型）、
> `std::deque`（装不可移动类型的容器）和 `assert`。这些都在第 1.7 ~ 1.11 节。
>
> **阅读约定**：凡是 src 没细讲、但本阶段会用到的知识点，都按同一个格式写：
> **是什么** → **为什么需要它（它解决了什么问题）** → **常见用法** → **什么时候用** → **一个跟本题无关的例子**。
> 凡是一行就能写完、写了就等于把答案送给你的东西，都写成"**必须自己想清楚、自己补上**"。
>
> ⚠️ **命名空间（之前漏写了，必看）**：测评文件里写着 `using namespace counter6;`（P6.1）、
> `bank6`（P6.2）、`bq6`（P6.3）、`rw6`（P6.4）。
> 所以每道题的名字必须放进对应命名空间，否则编译直接报 `'counter6' is not a namespace-name`。
>
> **本阶段所有题编译时都要加 `-pthread`。**

---

## 0. 本阶段题目一览

| 题号 | 主题 | 类型 | 你要写的文件（放 `projects/mysol/stage6/`） | 测评点数 |
| :--: | :-- | :--: | :-- | :--: |
| P6.1 | 线程安全计数器（竞态 → 锁） | 主线 | `p6_1_counter.h` | 13 |
| P6.2 | 银行转账与多锁死锁 | 主线+扩展 | `p6_2_bank.h` | 12 |
| P6.3 | 有界阻塞队列（条件变量+关闭协议） | 主线+扩展 | `p6_3_blocking_queue.h` | 13 |
| P6.4 | 读写锁并发 Map | 主线 | `p6_4_rwlock_map.h` | 13 |

所有题目都**只写头文件，不要写 `main()`**；编译**都要加 `-pthread`**。

---

## 1. 做题之前必须懂的几件事

### 1.1 测评程序怎么用（回顾）

`projects/tests/stage6/*_test.cpp` 是测评程序，**自带 `main()`**；你只写头文件，不要写 `main()`。
断言宏说明见 `stage1.md` 第 1.1 节。流程：在 `projects/mysol/stage6/` 下写 `.h`，复制测评文件，再编译。

```bash
mkdir -p projects/mysol/stage6
cp projects/tests/stage6/p6_1_counter_test.cpp         projects/mysol/stage6/
cp projects/tests/stage6/p6_2_bank_test.cpp            projects/mysol/stage6/
cp projects/tests/stage6/p6_3_blocking_queue_test.cpp  projects/mysol/stage6/
cp projects/tests/stage6/p6_4_rwlock_map_test.cpp      projects/mysol/stage6/
```

### 1.2 数据竞争是未定义行为，不只是"算错"

如果多个线程同时读写同一个非原子变量、又没有任何同步，这就是**数据竞争（data race）**。
在 C++ 内存模型里，数据竞争是**未定义行为**——不只是"结果可能不对"，而是编译器可以做
任何假设、任何优化，程序行为完全不可预测。

`mutex.cpp` 之所以要给 `count += 1` 加锁，就是因为 `count += 1` 实际上是"读→加→写"三步，
两个线程交错执行会丢失更新。这也是 P6.1 要亲手复现的：把读和写拆开并主动 `yield`，
就能稳定地看到最终结果小于期望值。

**是什么**：数据竞争（data race）= 两个线程**同时**访问同一内存位置，**至少一个写**，且没有任何同步。

**为什么是 UB 而不只是"算错"**：C++ 内存模型明确规定：有数据竞争的程序行为是**未定义**的。
这意味着编译器可以假设“不存在竞争”，从而把变量缓存到寄存器、重排读写顺序、
甚至删掉“看起来没用”的写入。所以后果不只是“少加了几次”——它可能是崩溃、死循环、
读到不可能的值，而且**今天能跑对不代表明天能跑对**。并发 bug 往往在换编译器/换优化等级/
换 CPU 后才现形，这正是它最难查的原因。

**常见用法 / 避免方式**：

1. 用 `std::mutex` 保护所有共享可写数据；
2. 把单个变量换成 `std::atomic`（见 1.8）；
3. 让每个线程只碰自己的局部数据，最后再合并（“避免共享”）；
4. 用更高层的同步原语（条件变量、读写锁）。

**什么时候用**：只要两个线程可能碰同一份数据，就必须想一遍这个问题。

**一个跟本题无关的例子**：

```cpp
bool ready = false;        // 一个线程写，另一个线程轮询
// 线程 A: ready = true;
// 线程 B: while (!ready) {}   // 没有 atomic/mutex 时，B 可能永远看不到 true（UB）
```

### 1.3 三种 RAII 锁，各管什么

**是什么**：三种 RAII 锁对象——构造时加锁、析构时自动解锁。

**为什么需要它们**：手动 `m.lock(); ...; m.unlock();` 一旦遇到 `return` / `break` / 抛异常，
`unlock()` 就被跳过，锁永远不释放，其他线程全部卡死。RAII 把“解锁”放进析构函数，
无论怎么离开作用域都会执行。

**常见用法 / 怎么选**：

| 类型 | 特性 | 适用场景 |
| :-- | :-- | :-- |
| `std::lock_guard` | 最简单，不可手动解锁、不可移动 | 单锁简单临界区 |
| `std::scoped_lock`（C++17） | `lock_guard` 升级版，**可一次锁多把**（内部用免死锁算法） | 需要同时锁多把锁 |
| `std::unique_lock` | 可手动 `lock/unlock`、可移动，**可与条件变量配合** | `cv.wait` / 需要延迟加锁 |

**什么时候用**：单锁 → `lock_guard` / `scoped_lock`（前者是后者的退化，选哪个都对）；
多锁 → `scoped_lock`（见 1.4）；条件变量或需要中途解锁 → `unique_lock`。

**一个跟本题无关的例子**：用一个 `scoped_lock` 同时锁住“账户 A 和账户 B”，
保证转账的“扣一边、加另一边”在同一个临界区内完成（P6.2）。

**`mutable` 的本质**：`const` 成员函数里 `this` 是 `const T*`，普通成员都不可改；
但加锁改的是 **mutex 自己的状态**，不是对象的“逻辑值”，所以用 `mutable` 明确告诉编译器
“这个成员在 const 函数里也能改”。一个逻辑上“只读”的操作（如 `Get() const`）仍然需要改 mutex，
就是这个道理。

### 1.4 死锁：四个必要条件与"按同一顺序加锁"

**是什么**：死锁 = 两个或多个线程互相等待对方持有的锁（或资源），谁也无法继续。

**为什么会出现（四个必要条件，破坏任意一个即可预防）**：

1. **互斥**：资源一次只能被一个线程持有（锁天然如此，不好破坏）；
2. **持有并等待**：拿着 A 去等 B；
3. **不可剥夺**：别人不能从你手里抢走 A；
4. **循环等待**：T1 等 T2、T2 等 T1（或更长的环）。

实践中最实用的预防手段就是破坏第 4 条——**统一加锁顺序**：所有地方都按同一个（如内存地址/ID）
顺序拿锁，就不可能出现“持 A 等 B / 持 B 等 A”。

**常见用法**：

- `std::scoped_lock lk(a.m_, b.m_);`：内部使用 `std::lock` 的算法（一次性尝试所有锁，
  失败就全部释放重试），破坏循环等待；
- 按对象地址/ID 排序后再依次加锁；
- `try_lock` + 退避（拿不到就释放已有的重来）；
- 死锁检测/超时（`timed_mutex`）——工程上常用作兜底手段。

**什么时候用**：只要需要同时持有两把及以上锁，就必须用上述手段之一。

**一个跟本题无关的例子**：

```cpp
// 线程 1：lock(A); lock(B);
// 线程 2：lock(B); lock(A);   ← 这两个函数一组合就可能死锁
// 修复：两处都改成 std::scoped_lock lk(A, B);  或都按地址顺序加锁
```

**两个附加的坑**

- **自转账 / 自赋值**：`a.Transfer(a, 10)` 如果对同一个 mutex 连续 `lock()` 两次，
  在非递归 mutex 上是死锁/UB。正确做法是函数开头 `if (this == &other) return;`。
- **“检查”与“修改”必须在同一临界区内**：先检查后加锁，中间别人可能已经把余额改掉，
  会出现“检查时够、扣款时不够”的竞态。

### 1.5 条件变量：带谓词的 `wait`、以及"关闭协议"

**是什么**：条件变量（`std::condition_variable`）让一个线程“挂起等待”另一个线程通知；
它总是和一把 mutex 配合，用来等待“某个谓词（条件）成立”。

**为什么需要它**：如果只靠轮询（`while (q.empty()) {}`），线程会白烧 CPU，还可能拿不到最新状态。
条件变量让等待者真正睡眠，状态变化时由别人 `notify` 叫醒。

**为什么必须用带谓词的 `wait`**：`cv.wait(lk, pred)` 等价于
`while (!pred()) cv.wait(lk);`，它额外解决了两件事：

- **虚假唤醒**：操作系统允许在没有 `notify` 的情况下无故唤醒等待者；带谓词就会重新检查，
  不成立就继续睡（自写 `if` 做不到这一点）；
- **丢失唤醒**：如果线程在“检查条件”和“进入 wait”之间，另一个线程已经 `notify` 过了，
  这个通知就丢了。带谓词的 `wait` 在挂起前先查一次谓词，避免漏掉已经发生的变化。

**常见用法**：

```cpp
std::unique_lock<std::mutex> lk(m);
cv.wait(lk, [&]{ return cond; });   // 醒来后 cond 一定为 true
// ... 修改共享状态 ...
lk.unlock();                        // 常见做法：先解锁再 notify
cv.notify_one();                    // 或 cv.notify_all();
```

**什么时候用**：生产者-消费者、线程池、任何“等到条件成立再继续”的场景。

**一个跟本题无关的例子**：一个下载线程等“缓冲区里有数据可读”，一个读取线程等“缓冲区里有空位可写”，
两边各用一个条件变量（队列非空 / 队列非满）。

**`notify_one` 还是 `notify_all`**：只有一个等待条件时 `notify_one` 够用；
存在**多种不同条件**（如“队列非空”和“队列非满”）时，只唤醒一个可能叫错人，
本题的 P6.3 用 `notify_all` 最稳。

**关闭协议（P6.3 的核心）**：一个阻塞队列的消费者会在队列空时永久等待，
必须有一套“什么时候退出”的约定。本题采用：`Close()` 置 `closed_ = true` 并 `notify_all()`；
`Push` 在已关闭时抛异常；`Pop` 在“已关闭且队列空”时抛异常。
谓词里要**同时考虑 `closed_` 和数据条件**（如 `closed_ || !q_.empty()`），
否则关闭时正在等待的线程不会被唤醒。这个套路（bool 关闭位 + 谓词里带上它）在真实系统里到处都是。

### 1.6 读写锁：读共享、写独占

**是什么**：一种“读共享、写独占”的锁。`std::shared_mutex`（C++17）+ `std::shared_lock`（读）/`std::unique_lock`（写）。

**为什么需要它**：普通 `mutex` 连多个读者之间也互斥；而“读”本身不改变数据，
多个读者同时读是安全的。读多写少的场景（配置表、缓存、路由表）用读写锁能大幅提高并行度。

**常见用法**：

```cpp
mutable std::shared_mutex m_;
// 读
{ std::shared_lock lk(m_); return map_.at(k); }
// 写
{ std::unique_lock lk(m_); map_[k] = v; }
```

**什么时候用**：读操作远多于写操作，且临界区有一定长度（否则锁开销反而变成瓶颈）。

**一个跟本题无关的例子**：一份几分钟才更新一次的配置表，被上百个线程频繁读取——
读锁能让这些读取真正并行。

**三个注意**：

- **不能把读锁升级成写锁**：需要写时必须先释放读锁、再重新申请写锁（中间状态可能被别人改掉，
  要重新检查）。
- 读写锁的公平性由实现决定，写得频繁时读者可能饿死。
- 在 `const` 成员里加共享锁，`m_` 同样要 `mutable`。

---

### 1.7 `std::thread` 的最少必要知识

**是什么**：`std::thread` 是一个线程对象；构造时启动一个新线程，执行你给的可调用对象。

**为什么需要它**：并发执行的入口。src 的 `mutex.cpp` 已经演示了 `std::thread t(f); t.join();`。

**常见用法 / 必须记住的规则**：

```cpp
std::thread t(worker, 42, std::string("x"));   // 参数默认按值拷贝进线程
t.join();                                       // 等它结束（阻塞当前线程）
```

- **参数默认按值拷贝**：想传引用要用 `std::ref(x)`；否则你在主线程改的 `x`，子线程看不到，
  或者更糟——子线程拿到了一个已经被销毁的局部变量的引用。
- **`join()` 或 `detach()` 二选一，且必须显式做**。`std::thread` 析构时如果还 `joinable()`，
  会直接 `std::terminate()`（进程崩溃）。
- `detach()` 让线程脱离管理、独立运行；除非你非常确定不需要等待，否则用 `join()`。
- 当前线程工具：`std::this_thread::get_id()`、`std::this_thread::yield()`（让出时间片）、
  `std::this_thread::sleep_for(std::chrono::milliseconds(10))`。
- **异常不会跨线程传播**：子线程抛出的异常会导致 `std::terminate`。
- 线程里访问的所有共享可写数据，都要按 1.2 ~ 1.6 的规矩同步。
- （C++20 的 `std::jthread` 会在析构时自动 `join`，还带停止令牌；本仓库用 C++17，用 `std::thread`。）

**什么时候用**：需要真正并行执行时。P6.1 的 `RunCounter` 就是“起 n 个线程各跑 iters 次，然后 join”。

**一个跟本题无关的例子**：

```cpp
long long total = 0;
std::mutex m;
auto work = [&](int lo, int hi) {
  long long local = 0;
  for (int i = lo; i < hi; ++i) local += i;
  std::scoped_lock lk(m);       // 只在合并时加锁，减少竞争
  total += local;
};
std::thread a(work, 0, 500), b(work, 500, 1000);
a.join(); b.join();
```

### 1.8 `std::atomic`：不用 mutex 的计数器做法

**是什么**：原子类型（`std::atomic<int>` 等）。对它的读、写、自增等操作是**不可分割**的，
不会产生数据竞争。

**为什么需要它**：对一个简单的计数器，加一把 `mutex` 太重（每次自增都要进内核/原子指令）。
`atomic` 直接用 CPU 的原子指令，又快又对。

**常见用法**：

```cpp
std::atomic<int> c{0};
c.fetch_add(1);              // 原子 +1
++c;                         // 等价
int v = c.load();            // 原子读
c.store(0);                  // 原子写
// 复合逻辑（检查+修改）仍然需要 mutex 或 compare_exchange
```

**什么时候用**：单个变量的计数 / 标志开关 / 进度报告。
**什么时候不能用**：需要“多个变量一起保持一致”（比如队列的 size 和内容）时，
atomic 不够，必须用 mutex。

**一个跟本题无关的例子**：多个工作线程各自处理任务，用一个 `std::atomic<long> done{0}` 统计完成数，
主线程轮询它（实际会配合条件变量）。

**注意**：默认内存序是 `seq_cst`（最严格、最好理解）；`memory_order_relaxed` 等高级内存序
在你真的懂之前不要碰。

### 1.9 `std::optional`（P6.4 的返回类型）

**是什么**：`std::optional<T>` 表示“**可能有**一个 `T`，也可能什么都没有”（空状态）。

**为什么需要它**：函数“查不到”时，过去常用特殊值当哨兵（返回 `-1`、`nullptr`、`INT_MIN`），
但这要求特殊值与合法值不重叠，很容易出错。`optional` 让“有没有值”成为类型的一部分，
调用方必须显式处理“空”的情况。

**常见用法**：

```cpp
std::optional<int> Find(int k) const {
  auto it = map_.find(k);
  if (it == map_.end()) return std::nullopt;   // 空
  return it->second;                           // 有值
}

std::optional<int> o = Find(1);
if (o) { int v = *o; }                    // operator bool + 解引用
int v = o.value_or(-1);                   // 空时用默认值
o.value();                                // 空时抛 std::bad_optional_access
o.reset();                                // 变空
```

**什么时候用**：函数“可能找不到 / 可能没有结果”，而且这不算异常情况。
P6.4 的 `Get(int) const` 就是典型：找到返回 `optional<int>`，找不到返回 `nullopt`。

**一个跟本题无关的例子**：读配置项——配置里可能有也可能没有 `timeout`，
用 `optional<int> GetTimeout()` 比“返回 -1 表示没配”清楚得多。

**注意**：`optional<T&>` 在 C++17 是非法的（要表达“可选的引用”得用 `T*` 或 `reference_wrapper`）；
表达“失败原因”不要用 `optional`（那应该用异常，或 C++23 的 `expected`）。

### 1.10 `std::deque` 与“不可移动类型”的容器

**是什么**：`std::deque`（double-ended queue）是双端队列，内部是**分段连续**存储，
两头都能 O(1) 插入/删除。

**为什么需要它**：`std::vector` 扩容时要**移动已有元素**；如果元素类型不可移动
（比如 `Account` 里含 `std::mutex`，mutex 不可拷贝也不可移动），`vector` 的 `reserve` / 扩容
就编译不过。`deque` 不移动已有元素，所以能装这类类型。

**常见用法**：

```cpp
std::deque<Account> accts;
accts.emplace_back(100);      // 就地构造
accts.emplace_back(200);
for (const auto& a : accts) ...
```

**什么时候用**：需要在两端频繁插入；或元素**不可拷贝/不可移动**。

**一个跟本题无关的例子**：滑动窗口最大值、任务调度队列、需要频繁 `push_front` 的场景。

**注意**：deque 内存不连续，随机访问比 vector 稍慢，遍历也不如 vector 缓存友好。
P6.2 的 `TotalBalance` 写成**模板**就是为了能接 `std::deque<Account>`（测评用的就是它）。

### 1.11 `assert` 与 `NDEBUG`

**是什么**：`assert(cond)`（`<cassert>`）在 `cond` 为假时打印文件名/行号并 `abort()`。

**为什么需要它**：用它表达“这里**必须**成立”的内部不变量（程序员错误），
能在 bug 刚出现的地方立刻停住，而不是拖到后面变成一个莫名其妙的结果。

**常见用法**：`assert(capacity >= 1);`、`assert(!q_.empty());`、`assert(idx < size_);`。

**什么时候用**：检查“不可能发生”的情况 / 前置条件。
**什么时候不要用**：处理用户输入、可恢复的错误——那些应该抛异常或返回错误码
（测评程序里对空栈 `Pop` 就是用异常，而不是 `assert`）。

**一个跟本题无关的例子**：二分查找前 `assert(std::is_sorted(v.begin(), v.end()));`。

**注意**：定义了宏 `NDEBUG` 时，`assert` 会被整个去掉（发布构建常常这么做）。
所以**不要把有副作用的表达式放进 `assert`**（例如 `assert(pop() == 3);`），
否则 Release 下这段逻辑会消失。P6.3 的参考实现就用了 `assert(capacity >= 1)`
（可写可不写，写了在 Debug 下能更早暴露误用）。

---

## 2. 主线题目（贴合 src 四课）

### P6.1 线程安全计数器（竞态 → `lock`/`unlock` → `scoped_lock`）

- **类型**：主线（src 的 mutex.cpp / scoped_lock.cpp）
- **考什么**：亲手复现数据竞争，然后分别用手动锁和 RAII 锁修正；`const` 成员里的 `mutable mutex`。
- **你要写**：`projects/mysol/stage6/p6_1_counter.h`。只写头文件。
- **复制过来的测评文件**：`tests/stage6/p6_1_counter_test.cpp`。测评点数：13。

**测评程序会用到的接口（名字必须一致）：**

> ⚠️ **命名空间**：测评文件里写着 `using namespace counter6;`，所以 `UnsafeCounter` / `LockedCounter` /
> `ScopedCounter` / `Counter` / `RunCounter` 必须全部放在 `namespace counter6` 里。

```cpp
struct UnsafeCounter {              // 反面教材：无锁
  int value;                        // 公开：测评直接读它
  void Increment(int iters);        // 每次：读 value → yield → 写回 value+1
};

struct LockedCounter {              // 手写 lock / unlock
  int value;                        // 公开
  void Increment(int iters);
};

struct ScopedCounter {              // RAII：scoped_lock
  int value;                        // 公开
  void Increment(int iters);
};

class Counter {                     // 正式封装
public:
  void Increment();                 // ++value_
  void Add(int n);                  // value_ += n
  int  Get() const;                 // 加锁后读
  void Reset();                     // value_ = 0
};

// 起 nthreads 个线程各调 iters 次 Increment()，join 后返回 Get()
inline int RunCounter(Counter& c, int nthreads, int iters);
```

下面这些内部成员由你自己设计：

- `LockedCounter` / `ScopedCounter` 各自需要一把 `std::mutex` 成员（名字自定）；
  **`UnsafeCounter` 不要加锁**——它就是用来稳定复现竞态的，加了锁反而测不出丢失更新。
- `Counter` 需要一个 `int` 计数成员和一把互斥锁成员（名字自定）。
  注意 `Get()` 是 `const` 成员却要加锁，所以锁成员必须能在 `const` 函数里被修改
  （该加哪个关键字见 1.3）。
- `UnsafeCounter` 的 `Increment` 必须**故意把读和写拆开**并调用 `std::this_thread::yield()`，
  否则编译器可能把它合成一条读-改-写指令，反而不容易复现丢失更新（见 1.2）。
- `LockedCounter` 用手动 `lock()` / `unlock()`；`ScopedCounter` 用 `std::scoped_lock`（见 1.3）。
- `RunCounter` 用 `std::vector<std::thread>` 起 `nthreads` 个线程、每个跑 `iters` 次、然后全部 `join`，
  最后返回 `c.Get()`（`std::thread` 的规矩见 1.7）。

**为什么是这些签名：**

- `UnsafeCounter::Increment` 必须**故意把读和写拆开**并调用 `std::this_thread::yield()`。
  如果直接写 `value += 1`，编译器可能把它合成一条读-改-写指令，多线程下反而不容易复现丢失更新，
  测试就会变得不稳定。拆开 + yield 才能稳定复现。测评断言 `value < 线程数 * 次数`。
- `LockedCounter` / `ScopedCounter` 分别用手动锁和 `std::scoped_lock`，功能相同，用来对比写法。
- `Counter::Get()` 是 const 成员却要加锁，所以 `m_` 必须是 `mutable`（1.3）。
- `RunCounter` 把"起线程、join、返回结果"封装起来，测评反复调用它做随机对拍。

**要做的事**：实现 4 个类型和 `RunCounter`。

**测评点在查什么**：无锁版会丢更新（结果 < 期望）；手动锁版与 `scoped_lock` 版恒定精确；
`Counter` 的基本操作；const 对象能调 `Get()`；8 线程 × 10 万精确；`Reset`；单/双/16 线程精确；
随机线程数多次对拍；高竞争；一个读线程 + 4 个写线程，读到的计数单调不减、最终精确。

**编译运行**（注意 `-pthread`）：

```bash
cd projects/mysol/stage6
g++ -std=c++17 -pthread p6_1_counter_test.cpp -I../../tests -o p6_1 && ./p6_1
```

通过标准：`Result: 13/13 Passed`。

**自查**：① `Get() const` 里要加锁，为什么 `m_` 必须 `mutable`？② 数据竞争是 UB 还是只是算错？③ `scoped_lock` / `lock_guard` / `unique_lock` 各适合什么场景？

---

### P6.2 银行转账与多锁死锁（`scoped_lock` 多锁）

- **类型**：主线（src 的 scoped_lock.cpp）＋ 可选死锁演示（第 1.4 节）
- **考什么**：多把锁同时获取、死锁的成因与规避、自转账、余额检查与扣款在同一临界区。
- **你要写**：`projects/mysol/stage6/p6_2_bank.h`。只写头文件。
- **复制过来的测评文件**：`tests/stage6/p6_2_bank_test.cpp`。测评点数：12。

**测评程序会用到的接口（名字必须一致）：**

> ⚠️ **命名空间**：测评文件里写着 `using namespace bank6;`，所以 `Account` 和 `TotalBalance`
> 必须放在 `namespace bank6` 里。

```cpp
class Account {
public:
  explicit Account(int money);
  void Transfer(Account& other, int money);   // 先从本账户扣，再加到 other
  int  Balance() const;                       // 线程安全
  void Deposit(int money);                    // 线程安全
};

// 求和；用模板是为了能接收 std::deque<Account>（Account 含 mutex，不可移动）
template <typename Container>
long long TotalBalance(const Container& accts);
```

另外：

- `Account` 内部需要一个余额成员和一把互斥锁成员；`Balance()` 是 `const` 成员却要加锁（关键字见 1.3）。
- 还需要一个**只在定义了 `DEMO_DEADLOCK` 时才编译**的 `TransferNaive(Account&, int)`：
  先锁自己、睡一小会儿、再锁对方。它只是可选演示用的反面教材，默认编译不需要它。

**为什么是这些签名：**

- `Transfer` 必须用 `std::scoped_lock lk(m_, other.m_)` 一次性拿两把锁（1.4）。并且：
  - 开头判断 `this == &other` 直接 `return`（自转账）；
  - "余额不足就返回"和"扣款/加款"必须在**同一个临界区**内（1.4）。
- `Balance()` 是 const 成员、要加锁，所以 `m_` 必须 `mutable`。
- `TransferNaive` 放在 `#ifdef DEMO_DEADLOCK` 里：它是反面教材，只有做可选演示时才需要编译。
  它先锁自己、睡一会儿、再锁对方，两个线程反向转账时就能稳定死锁。
- `TotalBalance` 用模板而不是 `const std::vector<Account>&`：因为 `Account` 里含 `std::mutex`，
  它是不可拷贝、不可移动的，放进 `vector` 后 `reserve`/扩容需要移动元素，会编译失败；
  测评改用 `std::deque<Account>`（deque 不需要移动已有元素）。模板让它能接任意容器。

**下面这些你必须自己想清楚、自己补上（只给要求，不给能直接复制的代码）：**

- **`Account` 的成员**：一个 `int`（余额）和一把 `mutable std::mutex`；构造函数把初始余额存下。
- **`Transfer`**：开头先 `if (this == &other) return;`（自转账防护）；
  然后 `std::scoped_lock lk(m_, other.m_)` 一次性拿两把锁（见 1.4）；
  “余额不足就返回”与“扣款/加款”必须在**同一个临界区内**完成。
- **`Balance` / `Deposit`**：各自用 `scoped_lock` 保护对余额的读写。
- **`TotalBalance` 写成函数模板**：参数是 `const Container&`，循环里 `s += a.Balance()`。
  为什么不能用 `const std::vector<Account>&`？因为 `Account` 含 `mutex`，不可拷贝也不可移动，
  `vector` 扩容会编译失败；测评改用 `std::deque<Account>`（见 1.10）。
- **`TransferNaive`**：整体包在 `#ifdef DEMO_DEADLOCK` / `#endif` 里；不加 `-DDEMO_DEADLOCK`
  时它不能参与编译。

**要做的事**：实现 `Account` 和 `TotalBalance`；默认编译不带 `DEMO_DEADLOCK`。

**测评点在查什么**：基本转账；转账前后总额守恒；余额不足是空操作；刚好等于余额可以转；
自转账安全且无变化；转 0；两个账户并发反向转账后总额守恒、余额非负；
多个账户并发转账（用 `deque`）；混入自转账；随机转账模式；`Deposit` 线程安全；多轮压力测试。

**编译运行**：

```bash
cd projects/mysol/stage6
g++ -std=c++17 -pthread p6_2_bank_test.cpp -I../../tests -o p6_2 && ./p6_2
```

通过标准：`Result: 12/12 Passed`。

（可选）亲眼看看死锁——**它会永久挂起，必须用 `timeout` 或 Ctrl-C 结束**：

```bash
g++ -std=c++17 -pthread -DDEMO_DEADLOCK p6_2_bank_test.cpp -I../../tests -o p6_2_dl
timeout 5 ./p6_2_dl --demo-deadlock     # 退出码 124 表示真的死锁挂起了
```

**自查**：① "两个线程以相反顺序加锁"对应死锁的哪个必要条件？② `scoped_lock` 为什么不会被相反顺序卡死？③ 为什么"检查余额"和"扣款"要在同一把锁下？

---

### P6.3 有界阻塞队列（条件变量 + 关闭协议）

- **类型**：主线（src 的 condition_variable.cpp）＋ 关闭协议（第 1.5 节）
- **考什么**：带谓词的 `wait`、`notify`、消费者如何安全退出（1.5）。
- **你要写**：`projects/mysol/stage6/p6_3_blocking_queue.h`。只写头文件。
- **复制过来的测评文件**：`tests/stage6/p6_3_blocking_queue_test.cpp`。测评点数：13。

**测评程序要求 `namespace bq6` 里提供：**

```cpp
class ClosedError : public std::runtime_error {
public:
  ClosedError();                   // runtime_error("BlockingQueue is closed")
};

template <typename T>
class BlockingQueue {
public:
  explicit BlockingQueue(size_t capacity);  // capacity >= 1
  void Push(T value);              // 满则等待；已关闭则抛 ClosedError
  T    Pop();                      // 空则等待；已关闭且空则抛 ClosedError
  void Close();                    // 置 closed 并唤醒所有等待者
  bool Closed() const;
  size_t Size() const;
  size_t Capacity() const;
};
```

**为什么是这些签名：**

- `Push` / `Pop` 必须用 `cv.wait(lk, 谓词)` 的**带谓词**版本，谓词里**同时判断 `closed_` 和数据条件**
  （例如 `closed_ || q_.size() < capacity_`）。理由见 1.5：防虚假唤醒、防丢失唤醒，以及关闭时能醒来。
- `Push`/`Pop` 里的锁必须是 `std::unique_lock<std::mutex>`，因为 `wait` 要反复解锁/加锁。
- `Close()` 先置位 `closed_` 再 `notify_all()`（两个条件变量都要通知），之后 `Push` 抛 `ClosedError`；
  `Pop` 在还有元素时仍把元素取完，空了才抛。
- `Closed()/Size()/Capacity()` 给测评和调试用；`Size()` 是加锁读取。
- `capacity >= 1`；容量为 1 的队列是压力测试的重点。

**下面这些你必须自己想清楚、自己补上（只给要求，不给能直接复制的代码）：**

- **内部成员**：一个存 `T` 的队列（`std::queue<T>` 最省事）、`capacity_`、
  一个 `closed_` 标志、一个 `mutable std::mutex`（因为 `Closed()`/`Size()` 是 const 成员还要加锁）。
- **两个条件变量**：一个表示“非空”（给 `Pop` 等）、一个表示“非满”（给 `Push` 等）；
  名字自定。用一个也行，但要用 `notify_all`。
- **`Push` / `Pop` 的谓词**：必须把 `closed_` 一起写进去，例如 `closed_ || q_.size() < capacity_`
  与 `closed_ || !q_.empty()`；醒来后再判断“到底是被唤醒还是被关闭”，是关闭就抛 `ClosedError`。
- **锁的类型**：必须是 `std::unique_lock<std::mutex>`（不能用 `lock_guard`/`scoped_lock`），
  因为 `wait` 要反复解锁/加锁（见 1.5）。
- **`Close()`**：在锁内置 `closed_ = true`，解锁后 `notify_all()` 两个条件变量。
- **移动语义**：`Pop` 里对 `front()` 用 `std::move`，这样只可移动的 `unique_ptr` 也能入队出队。
- 所有名字都放进 `namespace bq6`。

**要做的事**：实现 `ClosedError` 和 `BlockingQueue<T>`。

**测评点在查什么**：单线程 FIFO；关闭后 `Pop` 抛异常；关闭后 `Push` 抛异常；`Closed`/`Capacity`；
关闭后仍能把剩余元素取完；满时 `Push` 阻塞直到消费者腾位置；空时 `Pop` 阻塞直到生产者放入；
`Close` 能唤醒阻塞的消费者和生产者；能装只可移动的 `unique_ptr`；
4 生产者 × 4 消费者 × 1 万个互不重复的值——不丢不重；容量 1 的高竞争；多轮压力。

**编译运行**：

```bash
cd projects/mysol/stage6
g++ -std=c++17 -pthread p6_3_blocking_queue_test.cpp -I../../tests -o p6_3 && ./p6_3
```

通过标准：`Result: 13/13 Passed`。

**自查**：① `cv.wait(lk, pred)` 比 `cv.wait(lk)` + 手动 `if` 安全在哪？② 为什么条件变量必须配 `unique_lock`？③ `notify_one` 和 `notify_all` 怎么选？

---

### P6.4 读写锁并发 Map（`shared_mutex` / `shared_lock` / `unique_lock`）

- **类型**：主线（src 的 rwlock.cpp）
- **考什么**：读共享、写独占；`const` 成员里的 `mutable shared_mutex`；并发读写不破坏不变量。
- **你要写**：`projects/mysol/stage6/p6_4_rwlock_map.h`。只写头文件。
- **复制过来的测评文件**：`tests/stage6/p6_4_rwlock_map_test.cpp`。测评点数：13。

**测评程序要求 `namespace rw6` 里提供：**

```cpp
class ConcurrentMap {
public:
  void Put(int key, int value);              // 写：unique_lock
  bool Get(int key, int& value) const;       // 读：shared_lock；找到返回 true
  std::optional<int> Get(int key) const;     // 读：找到返回 optional，否则 nullopt
  bool Remove(int key);                      // 写：删掉返回 true
  bool Contains(int key) const;              // 读
  size_t Size() const;                       // 读
  void Clear();                              // 写
};
```

**为什么是这些签名：**

- `Get(int, int&) const` 和 `Get(int) const`（返回 `std::optional<int>`）都要用
  `std::shared_lock`；`Put/Remove/Clear` 用 `std::unique_lock`。见 1.6。
- `Get` 是 const 成员却要加共享锁，所以 `m_` 必须是 `mutable std::shared_mutex`。
- `Get(int, int&)` 在 key 不存在时**不修改** `value` 并返回 `false`；`Get(int)` 返回 `nullopt`。
  测评会检查不存在时传入的 `value` 没被改。
- 底层容器用 `std::unordered_map<int,int>` 即可。

**下面这些你必须自己想清楚、自己补上（只给要求，不给能直接复制的代码）：**

- **内部成员**：一个 `std::unordered_map<int,int>`，以及一把 `mutable std::shared_mutex`
  （因为 const 成员要加共享锁，见 1.6）。
- **读 / 写的锁类型**：`Get(...)` / `Contains` / `Size` 用 `std::shared_lock<std::shared_mutex>`；
  `Put` / `Remove` / `Clear` 用 `std::unique_lock<std::shared_mutex>`。
- **`Get(int)` 重载**：可以先调 `Get(key, v)` 拿到 bool，再决定返回 `v` 还是 `std::nullopt`
  （`std::optional` 的用法见 1.9）。
- **`Get(key, value)` 不能改 `value`**：只有找到时才写 `value = it->second`。
- 所有名字都放进 `namespace rw6`。

**要做的事**：实现 `ConcurrentMap`。

**测评点在查什么**：单线程 put/get/remove/contains/size；缺失键；`optional` 重载；覆盖写；
`Clear`；const 对象可读；8 个读者并发读到的值自洽；8 读者并行（观测到最大并发读者数 >= 2）；
4 个写者各写独立键空间后最终状态精确；读写混合不损坏；并发插入后 size 正确；
8 个线程并发删除 2 万个键，每个恰好删一次；多轮压力。

**编译运行**：

```bash
cd projects/mysol/stage6
g++ -std=c++17 -pthread p6_4_rwlock_map_test.cpp -I../../tests -o p6_4 && ./p6_4
```

通过标准：`Result: 13/13 Passed`。

**自查**：① `shared_lock` 和 `unique_lock` 在 `shared_mutex` 上分别是什么语义？② 为什么读锁不能直接升级成写锁？③ `mutable` 在这里的作用是什么？
