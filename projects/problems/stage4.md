# Stage 4 · STL 容器

> **先读完这四课再做题**
> 1. `src/4 - Containers/vectors.cpp` —— `vector`、`push_back/emplace_back`、`erase`、`remove_if`、lambda
> 2. `src/4 - Containers/sets.cpp` —— `set` 有序去重、`find/count/erase`
> 3. `src/4 - Containers/unordered_maps.cpp` —— `unordered_map`、`insert/find/count/erase`、迭代
> 4. `src/4 - Containers/auto.cpp` —— `auto`、range-for、`auto&`
>
> src 教了容器怎么用；测评还会考几件**用容器必须懂、但 src 没展开**的事：`vector` 的**迭代器失效**、
> `set` 的**比较器与严格弱序**、`unordered_map` 的**自定义键要配 hash**、
> **lambda 捕获**、以及 **erase-remove 惯用法**。第 1 节把这几个讲清，然后才是题目。

---

## 0. 本阶段题目一览

| 题号 | 主题 | 类型 | 你要写的文件（放 `projects/mysol/stage4/`） | 测评点数 |
| :--: | :-- | :--: | :-- | :--: |
| P4.2 | `set` 有序去重 / 比较器 / `map` 词频 | 主线 | `p4_2_set_comparator.h` | 16 |
| P4.4 | `auto` / `decltype` / 结构化绑定 | 主线 | `p4_4_auto_decltype.h` | 17 |
| P4.5 | Log Analyzer（erase-remove + lambda） | 主线 | `p4_5_log_analyzer.h` | 17 |
| P4.1 | `vector` 迭代器失效 / `reserve` vs `resize` | 扩展 | `p4_1_vector_invalidation.h` | 17 |
| P4.3 | `unordered_map` 陷阱 / 自定义键与 hash | 扩展 | `p4_3_unordered_custom_key.h` | 16 |

所有题目都**只写头文件，不要写 `main()`**（P4.5 的 stdin/stdout 外壳由题目提供）。

---

## 1. 做题之前必须懂的几件事

### 1.1 测评程序怎么用（回顾）

`projects/solutions/stage4/*_test.cpp` 是测评程序，**自带 `main()`**；你只写头文件，不要写 `main()`。
断言宏说明见 `stage1.md` 第 1.1 节。流程：在 `projects/mysol/stage4/` 下写 `.h`，复制测评文件到同目录，再编译。

```bash
mkdir -p projects/mysol/stage4
cp projects/solutions/stage4/p4_1_vector_invalidation_test.cpp   projects/mysol/stage4/
cp projects/solutions/stage4/p4_2_set_comparator_test.cpp        projects/mysol/stage4/
cp projects/solutions/stage4/p4_3_unordered_custom_key_test.cpp  projects/mysol/stage4/
cp projects/solutions/stage4/p4_4_auto_decltype_test.cpp         projects/mysol/stage4/
cp projects/solutions/stage4/p4_5_log_analyzer_test.cpp          projects/mysol/stage4/
cp projects/solutions/stage4/p4_5_log_analyzer_cli.cpp           projects/mysol/stage4/
```

### 1.2 `vector`：`size` 与 `capacity` 是两件事

- `size()`：当前**真正有元素**的个数。
- `capacity()`：当前**已经申请好的内存**能放下多少个元素，`capacity() >= size()` 恒成立。
- `push_back`/`emplace_back` 时如果 `size == capacity`，`vector` 会申请一块更大的内存、
  把旧元素搬过去、释放旧内存，然后才放入新元素。

由此引出两个最容易混淆的接口：

| 调用 | 对 `size` 的影响 | 对 `capacity` 的影响 | 能否访问 `v[i]`（`i < n`） |
| :-- | :-- | :-- | :-- |
| `v.reserve(n)` | 不变 | 至少变成 `n` | **不能**（元素还不存在，是 UB） |
| `v.resize(n)` | 变成 `n` | 至少变成 `n` | 可以（新元素被默认初始化） |

这就是 P4.1 要用 `BT_CHECK_THROWS(v.at(0), std::out_of_range)` 而不是 `v[0]` 来验证
"reserve 出来的位置还没有元素"的原因——用 `operator[]` 访问未构造的元素是未定义行为。

### 1.3 迭代器失效：`vector` 的"坑王"

当 `vector` 扩容时，旧内存被释放、元素被搬到新内存。**任何还指向旧内存的迭代器、指针、引用都会失效**
（继续用就是 UB：可能读到垃圾值，也可能崩溃）。这不是理论问题，是 BusTub 里最高发的 bug 类型之一。

关键结论：

- **会失效所有迭代器/指针/引用的操作**：任何导致 `capacity` 变大或元素被搬动的操作
  （越过 capacity 的 `push_back`/`insert`、`reserve` 变大、`resize` 变大、`shrink_to_fit`、赋值等）。
- **只失效"被删元素及其之后"的迭代器**：`erase`、`insert`（在中间插入且没扩容时）。
- **判断边界是 `capacity`，不是 `size`**：只要你事先 `reserve` 够了、后续 `push_back` 总数不超过 capacity，
  中间不会重分配，之前取得的指针/引用就一直有效。

P4.1 会用"数据起始地址有没有变"这个可观测量来证明上面的规则（`v.data()` 就是底层数组的首地址）。

### 1.4 `set` / `map` 的顺序与比较器：严格弱序

`std::set<T>` 是**有序 + 唯一**的容器（`std::map` 就是 key→value 版）。它内部靠比较器维护顺序，
默认是 `std::less<T>`（用 `operator<`）。

用自定义比较器时，比较器必须满足**严格弱序（strict weak ordering）**，口语化地说：

1. `cmp(a, a)` 必须为 `false`（不能出现 `a < a`）；
2. 如果 `cmp(a, b)` 为真，那么 `cmp(b, a)` 必须为假（反对称）；
3. 传递性：`cmp(a,b) && cmp(b,c)` 则 `cmp(a,c)`。

`set` 判断"两个元素相等"的方式不是 `operator==`，而是 **`!cmp(a,b) && !cmp(b,a)`**。
所以如果你把比较器写成 `<=`，`cmp(a,a)` 会为真，`set` 就会认为所有元素都相等、把数据全丢掉。
这就是为什么比较器的 `operator()` 必须写 `a < b` 而不是 `a <= b`，并且要 `const` 成员。

`std::set<int, std::greater<int>>` 直接就是降序集合。对 `std::pair<int,int>` 做 set 时，
`pair` 自带按"先 first 后 second"的字典序比较。

### 1.5 `unordered_map` 的两个坑：`operator[]` 会插入；自定义键要配 hash

坑一：**`m[key]` 在 key 不存在时会默认构造一个 value 并插入**。
`std::unordered_map<std::string,int> m; int x = m["nope"];` 执行完，`m.size()` 变成 1，`x` 是 0。
如果只是想查询，用 `find`（不插入）或 `at`（不存在就抛 `std::out_of_range`）。
C++17 还提供了 `try_emplace`（已存在则不覆盖）和 `insert_or_assign`（已存在则覆盖）。

坑二：`unordered_map<K,V>` 对键 `K` 有两个要求：

- **能判断相等**：`K` 要有 `operator==`（或你提供相等比较器）；
- **能算哈希**：存在可用的 `std::hash<K>`，或者你把它作为模板参数传进去。

标准库只给基础类型和 `std::string` 等配好了 `std::hash`，`std::pair` 和自定义结构没有。
两种解决办法：

- 给容器传一个自定义哈希函数对象（模板的第三个参数），例如
  `std::unordered_map<std::pair<int,int>, V, PairHash>`；
- 或者为你的类型**特化 `std::hash`**：`namespace std { template <> struct hash<Point> { ... }; }`。

哈希函数只要"相等的东西哈希值一定相同、不同的东西尽量分散"即可；允许冲突（冲突只是变慢），
不影响正确性。

### 1.6 `auto` / `decltype` / 结构化绑定

**`auto` 会剥掉引用和顶层 const**，所以它常常"静默地拷贝"：

```cpp
const std::string s = "hi";
auto        a = s;    // std::string        （拷贝！顶层 const 和引用都没了）
auto&       b = s;    // const std::string& （引用，保留 const）
const auto& c = s;    // const std::string&
auto&&      d = std::move(s);  // const std::string&&（转发引用）
```

**`decltype(expr)` 完全按表达式的类型走**，不剥引用：
`std::vector<int> v; decltype(v[0])` 是 `int&`（因为 `operator[]` 返回引用），
而 `auto x = v[0];` 得到的是 `int`（拷贝）。
`decltype(auto)` 用在返回类型上，表示"按 `decltype` 规则保留引用"。

**结构化绑定**（C++17）用来一次性拆开 pair/tuple/结构体：

```cpp
for (auto& [k, v] : m) { v *= 2; }        // 可改 value
for (const auto& [k, v] : m) { ... }      // 只读，零拷贝
```

注意 `std::map` 的元素类型是 `std::pair<const K, V>`，所以 `k` 的类型带 `const`。
写 `for (auto [k, v] : m)`（不带 `&`）会**整体拷贝**每个 pair，通常不是你要的。

### 1.7 lambda 捕获

`vectors.cpp` 里用过 `[](const Point& p){ return p.GetX() == 37; }`。方括号里决定 lambda 怎么拿到外部变量：

| 写法 | 含义 |
| :-- | :-- |
| `[]` | 不捕获任何外部变量，用到外部变量会编译错误 |
| `[x]` | **按值**捕获 `x`（拷贝一份，lambda 内改的是副本） |
| `[&x]` | **按引用**捕获 `x`（lambda 内改到外面那个变量） |
| `[=]` | 按值捕获所有用到的外部变量 |
| `[&]` | 按引用捕获所有用到的外部变量 |
| `[this]` | 捕获当前对象的 `this`（成员函数里用） |

按值捕获的对象生命周期跟着 lambda 自己；按引用捕获的变量必须在 lambda 被调用时还活着，否则就是悬垂引用。
P4.5 会专门对比 `[&level]` 和 `[level]`。

### 1.8 erase-remove 惯用法

`std::remove_if(begin, end, pred)` **不会真的删除元素**，也不能改变容器大小
（它只拿到一对迭代器，不知道容器本体）。它的实际行为是：把"不该删"的元素往前挪，返回一个迭代器 `new_end`，
指向第一个"应该被删掉"的位置。所以要真正删除，必须再调用容器的 `erase`：

```cpp
v.erase(std::remove_if(v.begin(), v.end(), pred), v.end());
```

`vectors.cpp` 里 `point_vector.erase(std::remove_if(...), point_vector.end())` 就是这一套。
如果只调 `remove_if` 不 `erase`，`v.size()` 不会变，末尾会留下"已经搬走"的无效内容。

---

## 2. 题目

下面按编号列出全部题目（一览表见第 0 节）。建议先做主线题 P4.2 → P4.4 → P4.5，
再做扩展题 P4.1 → P4.3；也可以按编号做。

### P4.1 `vector`：迭代器失效 / `reserve` vs `resize` / 下标

- **类型**：扩展（第 1.2、1.3 节）

- **考什么**：1.2 与 1.3。题目本身不要求你实现容器，而是要求你写一组**测量函数**，
  用它们把"地址什么时候变、capacity 什么时候涨"变成可断言的事实。
- **你要写**：`projects/mysol/stage4/p4_1_vector_invalidation.h`。只写头文件。
- **复制过来的测评文件**：`solutions/stage4/p4_1_vector_invalidation_test.cpp`。测评点数：17。

**测评程序要求 `namespace v4` 里提供：**

```cpp
struct CapSize {
  size_t cap;      // capacity
  size_t size;     // size
  bool operator==(const CapSize& o) const;
};

CapSize PushNTimes(size_t n);                  // 不 reserve，push_back n 次，返回最终 {capacity, size}
CapSize ReserveThenPush(size_t reserve_n, size_t push_n);  // 先 reserve(reserve_n) 再 push push_n 次

// 返回底层数组首地址，转成整数便于比较（避免比较悬垂指针的语义争议）
uintptr_t DataAddr(const std::vector<int>& v);

// 先 reserve(cap)，记下地址，再 push fill 次（fill <= cap），返回地址是否始终不变
bool AddressStableWithinCapacity(size_t cap, size_t fill);

// 不 reserve，push exceed 次（会越过初始 capacity），返回地址是否发生了变化
bool AddressChangesPastCapacity(size_t exceed);
```

**为什么这样设计**：`vector` 的迭代器失效是"内部行为"，你没法直接断言"迭代器失效了"，
但可以断言"底层数组地址变了"。`DataAddr` 就是把 `v.data()` 转成整数做比较；
`AddressStableWithinCapacity` / `AddressChangesPastCapacity` 分别在验证 1.3 的两条结论。
`CapSize` 用来把"capacity 和 size"一次返回，测评再分别读 `.cap` / `.size`。

**要做的事**：实现这 5 个测量函数。注意它们的**返回值语义**（各函数的注释就是规格）。
额外要求：`CapSize` 的成员名字必须是 `cap` 和 `size`（测评直接访问）。

**测评点在查什么**：新 vector 为空；`reserve` 只改 capacity 不改 size；`reserve` 出来的位置
用 `at` 访问会抛异常；`resize` 改 size 且新元素为 0、可写；`resize` 变小会截断；`clear` 保留 capacity；
预留容量内地址不变；越过 capacity 地址改变；`reserve` 后 push 不动地址；capacity 恒 >= size；
5000 次 push 的重分配次数远小于 5000（摊还增长）；迭代器算术与 `erase`/`insert`；100 万规模的 capacity/size。

**编译运行**：

```bash
cd projects/mysol/stage4
g++ -std=c++17 p4_1_vector_invalidation_test.cpp -I../../solutions -o p4_1 && ./p4_1
```

通过标准：`Result: 17/17 Passed`。

**自查**：① `v.reserve(100)` 之后 `v[50] = 1` 合法吗？`v.resize(100)` 之后呢？② 哪些操作会让所有迭代器失效？③ 判断旧指针是否安全，看的是 size 还是 capacity？

---

### P4.2 `set`：有序、去重、自定义比较器；`map` 词频

- **类型**：主线（第 1.4 节补充了比较器/严格弱序）
- **考什么**：`set` 的有序去重与三种 `erase` 形式、降序集合、`pair` 的自然序、
  自定义比较器与严格弱序（1.4）、`map` 词频统计与有序遍历。
- **你要写**：`projects/mysol/stage4/p4_2_set_comparator.h`。只写头文件。
- **复制过来的测评文件**：`solutions/stage4/p4_2_set_comparator_test.cpp`。测评点数：16。

**测评程序要求 `namespace s4` 里提供：**

```cpp
std::set<int> SortedUnique(const std::vector<int>& v);
std::string   JoinAscending(const std::set<int>& s);          // 用空格连接，如 "1 3 5"

struct SumCmp {   // 先按两数之和升序；和相等时按 pair 自然序（保证唯一顺序）
  bool operator()(const std::pair<int,int>& a, const std::pair<int,int>& b) const;
};
std::string JoinBySumOrder(const std::set<std::pair<int,int>, SumCmp>& s);  // 如 "(0,2) (1,1)"

std::map<std::string,int> WordFreq(const std::vector<std::string>& words);
std::string KeysJoined(const std::map<std::string,int>& m);    // 按 key 顺序拼接
```

**为什么这样设计**：

- `SumCmp` 用来演示自定义比较器。它必须满足严格弱序：先比 `a.first+a.second`，
  相等时再比 `a < b`（`pair` 自带的字典序）——如果只比和，那么"和相等但内容不同"的两个 pair
  会被 `set` 当成同一个元素而不能共存。测评的 `custom_comparator_orders_by_sum` 期望
  `(0,2)` 排在 `(1,1)` 前面（和都是 2，再按字典序）。
- `SumCmp::operator()` 必须是 `const` 成员且返回严格弱序结果，否则 `set` 无法工作（1.4）。
- `WordFreq` 用 `std::map` 而不是 `unordered_map`，就是为了让 key 有序，测评据此检查遍历顺序。

**要做的事**：实现以上 6 个函数/类型。`SortedUnique` 可以直接用 `set` 的区间构造；
`JoinAscending`/`JoinBySumOrder`/`KeysJoined` 按注释的格式拼接字符串。

**测评点在查什么**：去重升序（含空集、单元素、负数）；`insert` 的返回值表示是否新插入；
`find`/`count`；`erase(key)` 与 `erase(iterator)` 与 `erase(range)`；`std::greater<int>` 降序；
`pair` 自然序；自定义比较器按和排序；比较器满足严格弱序（`a<a` 为 false 等）；
`map` 词频与有序 key；10 万个值去重；随机插入后仍有序。

**编译运行**：

```bash
cd projects/mysol/stage4
g++ -std=c++17 p4_2_set_comparator_test.cpp -I../../solutions -o p4_2 && ./p4_2
```

通过标准：`Result: 16/16 Passed`。

**自查**：① `insert` 返回什么，怎么用它判断是否插入成功？② 比较器的 `operator()` 为什么必须 `const`？③ 为什么比较器写 `<=` 会让 `set` 丢掉数据？

---

### P4.3 `unordered_map`：`operator[]` 陷阱 + 自定义键的 `std::hash`

- **类型**：扩展（第 1.5 节）
- **考什么**：1.5。`operator[]` 会默认插入；`find`/`at`/`try_emplace`/`insert_or_assign` 的区别；
  给 `pair` 当 key 写哈希函数；给自定义结构当 key 特化 `std::hash`。
- **你要写**：`projects/mysol/stage4/p4_3_unordered_custom_key.h`。只写头文件。
- **复制过来的测评文件**：`solutions/stage4/p4_3_unordered_custom_key_test.cpp`。测评点数：16。

**测评程序要求你提供：**

```cpp
namespace u4 {
  struct PairHash {                       // 给 std::pair<int,int> 用的哈希
    size_t operator()(const std::pair<int,int>& p) const;
  };
  using PairMap = std::unordered_map<std::pair<int,int>, std::string, PairHash>;

  struct Point {
    int x, y;
    bool operator==(const Point& o) const;
  };

  std::unordered_map<std::string,int> MakeBasicMap();   // 含 foo/jignesh/spam/eggs 四个键
}

// 为 Point 特化 std::hash（放在 namespace std 里）
namespace std {
  template <> struct hash<u4::Point> {
    size_t operator()(const u4::Point& p) const noexcept;
  };
}
```

**为什么这样设计**：

- `PairHash` 演示"不特化 `std::hash`，而是把哈希当容器的第三个模板参数传进去"。
  写法是把两个 `int` 的哈希组合起来（例如 `h1 ^ (h2 << 1)`）。
- `Point` + `std::hash<Point>` 演示另一种做法：为自定义类型特化标准库的哈希模板。
  注意 `Point` 还需要 `operator==`，这是 `unordered_map` 判断键相等的依据。见 1.5。
- `MakeBasicMap` 是测评检查 `find/at/insert/erase/count/try_emplace` 行为的数据源，
  里面必须正好有 `foo=2, jignesh=445, spam=1, eggs=2` 这 4 个键值对。

**要做的事**：实现 `PairHash`、`Point` 的 `operator==`、`std::hash<Point>` 特化、`MakeBasicMap`。

**测评点在查什么**：`m["nope"]` 会插入默认值并使 `size()` 变成 1；`find` 不插入；`at` 抛
`std::out_of_range` 且不插入；`at` 取已有键；`insert` 遇到重复键保留旧值；按 key / 按迭代器 `erase`；
`try_emplace` 不覆盖、`insert_or_assign` 覆盖；`pair` 键（含负数）能 find/insert；
`Point` 键与 `unordered_set<Point>`；大量冲突键仍全部可查；10 万键往返；遍历每个 key 恰好一次；
`m[key] += ...` 依赖默认构造。

**编译运行**：

```bash
cd projects/mysol/stage4
g++ -std=c++17 p4_3_unordered_custom_key_test.cpp -I../../solutions -o p4_3 && ./p4_3
```

通过标准：`Result: 16/16 Passed`。

**自查**：① `unordered_map<K,V>` 对 `K` 有哪两个要求？② `m[key]` 为什么要求 `V` 可默认构造？③ 哈希冲突会破坏正确性吗？

---

### P4.4 `auto` / `decltype` / 结构化绑定

- **类型**：主线（src 的 auto.cpp + 第 1.6 节的 decltype）
- **考什么**：1.6。`auto` 剥引用与顶层 const、`const auto&` 借用、结构化绑定、`decltype` 与
  `decltype(auto)` 保留引用。
- **你要写**：`projects/mysol/stage4/p4_4_auto_decltype.h`。只写头文件。
- **复制过来的测评文件**：`solutions/stage4/p4_4_auto_decltype_test.cpp`。测评点数：17。

**测评程序要求 `namespace a4` 里提供：**

```cpp
struct Big {                       // 带拷贝计数的类型，用来证明 auto 会静默拷贝
  inline static int copies = 0;
  int v = 0;
  Big() = default;
  explicit Big(int x);
  Big(const Big& o);               // ++copies
  Big& operator=(const Big& o);    // ++copies
  static void Reset();
};

template <typename C> decltype(auto) AtRef(C& c, size_t i);  // 返回 c[i] 原样（vector 上是 int&）
template <typename C> auto AtVal(C& c, size_t i);            // 剥掉引用，返回拷贝

std::string ScaleValues(std::map<std::string,int>& m, int factor);  // 用 auto& [k,v] 原地放大 value
int SumPairs(const std::vector<std::pair<int,int>>& v);      // 用结构化绑定求和
```

**为什么这样设计**：

- `Big` 的拷贝计数（见 stage1 1.5）让"`auto x = big;` 到底拷没拷"变成可断言的数字。
- `AtRef` 返回 `decltype(auto)`，`AtVal` 返回 `auto`，两者的差别正好演示 1.6：
  测评对 `AtRef(vec,1) = 77;` 后检查 `vec[1] == 77`（说明返回了引用），
  而对 `int copy = AtVal(vec,2); copy = 55;` 后检查 `vec[2]` 没变（说明只是副本）。
- `ScaleValues` 用 `for (auto& [k,v] : m) v *= factor;`，测评检查修改生效、返回的拼接字符串正确；
  `SumPairs` 用 `const auto& [a,b]`。

**要做的事**：实现以上内容。

**测评点在查什么**：`auto` 对 `int`/`double`/`std::string` 的推导；`auto` 剥顶层 const；
`auto&`/`const auto&`/`auto&&` 的类型；`auto` 拷贝而 `const auto&` 不拷贝；range-for 用 `auto` 会逐个拷贝、
用 `const auto&` 零拷贝；结构化绑定改 map value；map 的 key 带 `const`；按值绑定会拷贝；
vector<pair> 结构化绑定求和；`decltype(vec[0])` 是 `int&`；`decltype(auto)` 与 `auto` 的差别；
`decltype(x)` 与 `decltype((x))` 的差别；用 `auto` 写迭代器与长模板类型；10 万 map 的结构化绑定。

**编译运行**：

```bash
cd projects/mysol/stage4
g++ -std=c++17 p4_4_auto_decltype_test.cpp -I../../solutions -o p4_4 && ./p4_4
```

通过标准：`Result: 17/17 Passed`。

**自查**：① `for (auto x : v)` / `for (auto& x : v)` / `for (const auto& x : v)` 各自适合什么场景？② `for (auto [k,v] : m)` 会发生什么？③ `decltype(auto)` 和 `auto` 的核心差别？

---

### P4.5 Log Analyzer（`erase-remove` + lambda + 综合容器）

- **类型**：主线（src 的 vectors.cpp + 第 1.7、1.8 节）
- **考什么**：把 `vector` / `set` / `unordered_map` / lambda 捕获 / erase-remove 串成一个完整小工具。
- **你要写**：`projects/mysol/stage4/p4_5_log_analyzer.h`。只写头文件，不要写 `main()`。
- **复制过来的测评文件**：
  - `solutions/stage4/p4_5_log_analyzer_test.cpp`（分级测试点，17 个）
  - `solutions/stage4/p4_5_log_analyzer_cli.cpp`（**这是测评配套的命令行外壳，不用你写**：
    它读 stdin、调用你头文件里的函数、把结果打到 stdout。你只要把测评文件复制过来、
    和你自己的头文件一起编译即可）

**测评程序要求 `namespace log5` 里提供：**

```cpp
using Counts = std::unordered_map<std::string, int>;

Counts CountLevels(const std::vector<std::string>& logs);          // 统计每个 level 出现次数
std::set<std::string> UniqueLevels(const std::vector<std::string>& logs);  // 有序去重
std::vector<std::string> Filter(const std::vector<std::string>& logs,
                                const std::string& level);          // 保留原顺序
void RemoveLevel(std::vector<std::string>& logs, const std::string& level); // 原地删除，保持其余顺序

int CountOf(const Counts& c, const std::string& level);            // 不存在返回 0（用 find，别用 []）
std::string Join(const std::set<std::string>& s);                  // 空格连接
std::string Join(const std::vector<std::string>& v);               // 空格连接
```

**为什么这样设计**：

- `RemoveLevel` **必须**用 `erase(std::remove_if(..., [&level](const std::string& l){ return l == level; }), logs.end())`
  实现（1.8）。测评会验证删除后其余元素的相对顺序不变、重复调用是幂等的。
- `CountOf` 必须用 `find` 而不是 `operator[]`：否则查询一个不存在的 level 会悄悄往计数表里插入键，
  测评的 `count_levels_missing_key_is_zero` 会检查 `c.size()` 没变。见 1.5。
- `Filter` 用 lambda（可以像参考实现那样按引用捕获 `level`，也可以按值，行为一样）。
- 测评里有一个 `lambda_capture_by_ref_vs_value` 测试点直接对比 `[&level]` 和 `[level]`（1.7），
  它是在测评文件里自己构造 lambda 的，不需要你额外提供什么，理解即可。

**要做的事**：实现以上 7 个函数。

**测评点在查什么**：计数（基本/空/不存在键为 0）；有序去重（基本/空/单元素）；
过滤保持顺序与条数、过滤不存在的 level；删除保持相对顺序、删除不存在的 level 是空操作、
删掉所有 level 后为空、重复删除幂等；lambda 捕获对比；10 万条计数；
与"线性暴力实现"随机对拍 200 轮；中等规模随机对拍。

**编译运行（分级测试点）**：

```bash
cd projects/mysol/stage4
g++ -std=c++17 p4_5_log_analyzer_test.cpp -I../../solutions -o p4_5 && ./p4_5
```

通过标准：`Result: 17/17 Passed`。

**洛谷式 stdin/stdout 对拍**（一共 14 个用例，`.ans` 由独立的 Python 朴素实现生成，是外部校验）：

```bash
cd projects/mysol/stage4
g++ -std=c++17 p4_5_log_analyzer_cli.cpp -o cli
bash ../../solutions/stage4/tests/run_tests.sh ./cli
```

通过标准：最后一行 `---------------- AC 14 / 14 ----------------`。

**自查**：① 为什么必须 `erase(remove_if(...), end())` 而不能只调 `remove_if`？② `[&level]` 和 `[level]` 有什么区别？③ 遍历 `unordered_map` 用结构化绑定拿到的 key 为什么是 `const`？
