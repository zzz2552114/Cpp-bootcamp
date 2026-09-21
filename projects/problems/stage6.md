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

### 1.3 三种 RAII 锁，各管什么

手动 `m.lock(); ...; m.unlock();` 的问题是：如果临界区中途 `return`、`break` 或抛异常，
`unlock()` 就被跳过了，锁永远不释放 → 其它线程全部卡死。RAII 锁把"解锁"放进析构函数解决这个问题。

| 类型 | 何时用 |
| :-- | :-- |
| `std::lock_guard` | 最简单的"构造加锁、析构解锁"；不可手动解锁、不可移动 |
| `std::scoped_lock` | C++17 的 `lock_guard` 升级版，**可以一次锁多把**（内部用避免死锁的算法） |
| `std::unique_lock` | 最灵活：可手动 `lock/unlock`、可移动，**与条件变量配合时必须用它** |

另外：**`const` 成员函数里要加锁时，`mutex` 成员必须声明为 `mutable`**。
因为 `const` 成员函数里 `this` 是 `const T*`，普通成员都不可改；但加锁改的是 mutex 自己的状态，
不是对象的"逻辑值"，所以用 `mutable` 明确告诉编译器"这个成员在 const 函数里也能改"。

### 1.4 死锁：四个必要条件与"按同一顺序加锁"

死锁的经典成因是**两个线程以相反顺序获取多把锁**：线程 1 先拿 A 再等 B，线程 2 先拿 B 再等 A，
互相等对方手里的锁，永远解不开。要避免它，最实用的规则是**统一加锁顺序**。

`std::scoped_lock lk(a.m_, b.m_)` 之所以安全，是因为它在内部采用"一次性尝试获取所有锁、
失败就全部释放重来"的算法（`std::lock`），不会出现"各持一把、互相等"的局面。它不是魔法，
理解成"我来帮你按一个不会死锁的策略同时拿多把锁"即可。

**两个附加的坑**：

- **自转账/自赋值**：`a.Transfer(a, 10)` 如果实现里对同一个 mutex 连续 `lock()` 两次，
  在非递归 mutex 上是死锁/UB。正确做法是函数开头 `if (this == &other) return;`。
- **"检查余额"和"扣款"必须在同一个临界区内**：如果先检查后加锁，中间别的线程可能已经把余额改掉了，
  就会出现"检查时够、扣款时不够"的竞态。

### 1.5 条件变量：带谓词的 `wait`、以及"关闭协议"

条件变量用来让线程"等到某个条件成立再继续"。`condition_variable.cpp` 里的标准写法是：

```cpp
std::unique_lock lk(m);
cv.wait(lk, []{ return count == 2; });   // 带谓词的等待
```

为什么必须用**带谓词**的版本：

- **虚假唤醒（spurious wakeup）**：操作系统允许条件变量在没有 `notify` 的情况下"无故"醒来。
  如果 `wait` 后面直接往下走，就会在条件不成立时继续执行。带谓词的 `wait` 内部是一个循环：
  醒来就重新检查谓词，不成立就继续睡。
- **丢失唤醒（lost wakeup）**：如果线程在检查条件和进入 `wait` 之间，另一个线程已经 `notify` 过了，
  这个通知就丢了。带谓词的 `wait` 在挂起前会先查一次谓词，避免丢失。

条件变量**必须配 `std::unique_lock`**（不是 `lock_guard`/`scoped_lock`），因为 `wait` 需要在挂起时
原子地"解锁并睡眠"、醒来时重新加锁，这要求锁对象可以被 `wait` 内部反复解锁/加锁。

`notify_one` 只叫醒一个等待者，`notify_all` 叫醒全部。存在**多个不同条件**（比如"队列非空"和"队列非满"）
时，如果只唤醒一个，可能叫醒的不是真正需要醒的那个；P6.3 用 `notify_all` 最稳。
持锁期间调用 `notify` 不会错，但被唤醒的线程会立刻又阻塞在锁上，所以常见做法是**先解锁再 notify**。

**关闭协议（P6.3 的核心）**：一个阻塞队列的消费者会在队列空时永久等待，必须有一套"什么时候退出"的约定。
本题采用：`Close()` 置 `closed_ = true` 并 `notify_all()`；`Push` 在已关闭时抛异常；
`Pop` 在"已关闭且队列空"时抛异常。谓词里要同时考虑 `closed_` 和数据条件，
否则关闭时正在等待的线程不会被唤醒。

### 1.6 读写锁：读共享、写独占

读多写少的场景用 `std::shared_mutex`（C++17 提供）：

- 读操作用 `std::shared_lock lk(m);` —— 多个读者可以**同时**持有共享锁；
- 写操作用 `std::unique_lock lk(m);` —— 写锁与所有读锁、其它写锁互斥。

同样地，如果读操作是 `const` 成员函数，`m_` 要声明为 `mutable`。
读写锁通常**不支持**"把读锁升级成写锁"，需要写的时候必须释放读锁再重新申请写锁。

---

## 2. 主线题目（贴合 src 四课）

### P6.1 线程安全计数器（竞态 → `lock`/`unlock` → `scoped_lock`）

- **类型**：主线（src 的 mutex.cpp / scoped_lock.cpp）
- **考什么**：亲手复现数据竞争，然后分别用手动锁和 RAII 锁修正；`const` 成员里的 `mutable mutex`。
- **你要写**：`projects/mysol/stage6/p6_1_counter.h`。只写头文件。
- **复制过来的测评文件**：`tests/stage6/p6_1_counter_test.cpp`。测评点数：13。

**测评程序会用到的接口（名字必须一致）：**

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

- `LockedCounter` / `ScopedCounter` 各自需要一把 `std::mutex` 成员；**`UnsafeCounter` 不要加锁**——
  它就是用来稳定复现竞态的，加了锁反而测不出丢失更新。
- `Counter` 需要一个 `int` 计数成员和一把互斥锁成员。注意 `Get()` 是 `const` 成员却要加锁，
  所以锁成员必须能在 `const` 函数里被修改（该加哪个关键字见 1.3）。

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
