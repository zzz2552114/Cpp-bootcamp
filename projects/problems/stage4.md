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
>
> **阅读约定**：从 1.3 节开始，凡是从 src 里没有细讲、但本阶段题目会用到的知识点，都按同一个格式写：
> **是什么** → **为什么需要它（它解决了什么问题）** → **常见用法** → **什么时候用** → **一个跟本题无关的例子**。
> 凡是一行就能写完、写了就等于把答案送给你的东西，都写成"**必须自己想清楚、自己补上**"，
> 把要求说清、把代码留给你写。另外，**每道题都明确写了名字该放在哪个命名空间里**，
> 测评文件里写了 `using namespace XXX;`，命名空间不存在就直接编译失败，别漏。

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

`projects/tests/stage4/*_test.cpp` 是测评程序，**自带 `main()`**；你只写头文件，不要写 `main()`。
断言宏说明见 `stage1.md` 第 1.1 节。流程：在 `projects/mysol/stage4/` 下写 `.h`，复制测评文件到同目录，再编译。

```bash
mkdir -p projects/mysol/stage4
cp projects/tests/stage4/p4_1_vector_invalidation_test.cpp   projects/mysol/stage4/
cp projects/tests/stage4/p4_2_set_comparator_test.cpp        projects/mysol/stage4/
cp projects/tests/stage4/p4_3_unordered_custom_key_test.cpp  projects/mysol/stage4/
cp projects/tests/stage4/p4_4_auto_decltype_test.cpp         projects/mysol/stage4/
cp projects/tests/stage4/p4_5_log_analyzer_test.cpp          projects/mysol/stage4/
cp projects/tests/stage4/p4_5_log_analyzer_cli.cpp           projects/mysol/stage4/
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

**是什么**：迭代器 / 指针 / 引用“失效”，指原来指向容器内某个元素的那个观察者不再指向有效元素。
对 `vector` 而言，扩容会在新内存重新放置元素、释放旧内存，所有指向旧内存的迭代器/指针/引用都变成
悬垂（dangling），继续使用就是未定义行为。

**为什么会出现（细致）**：`vector` 的元素在内存里必须**连续存放**。当 `size` 超过 `capacity` 时，
无法原地扩充（后面那块内存可能已经被别人占了），只能：申请一块更大的内存 → 把元素搬过去 →
释放旧内存。元素被搬走后，旧地址当然失效。

**常见用法 / 规律（记住这四条）**：

| 操作 | 失效范围 |
| :-- | :-- |
| 越过 capacity 的 `push_back`/`insert`、`reserve`/`resize` 变大、`shrink_to_fit`、赋值、`swap` | **全部**迭代器/指针/引用 |
| 中间 `insert`（未扩容）、`erase` | **被删/被插元素及其之后**的 |
| 未越 capacity 的 `push_back`/`emplace_back` | 不失效 |
| `clear` | 全部（但 capacity 不变，底层内存还在） |

- **判断安不安全，看 `capacity` 而不是 `size`**：预先 `reserve` 够、后续 `push_back` 总数不超过 capacity，
  中间就不会重分配，之前取得的指针/引用一直有效。
- 要边遍历边删时，用 `erase` 返回的下一个迭代器，或者直接用 1.8 的 erase-remove 惯用法。

**什么时候用**：任何“先存下一个指针 / 迭代器 / 引用，之后再访问”的代码都要想一遍这个问题。
这是 BusTub 里最高发的 bug 类型之一。

**一个跟本题无关的例子**：

```cpp
std::vector<int> v{1, 2, 3};
int& first = v[0];
v.push_back(4);        // 可能扩容 → first 悬垂
// first = 5;          // 若真的扩容了，这行就是 UB
```

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

**是什么**：比较器（comparator）是告诉 `set`/`map` “怎么排序、怎么判等”的可调用对象
（函数对象、函数指针、lambda）。默认是 `std::less<T>`（用 `operator<`）。
`set` 判断两个元素相等靠的是 `!cmp(a,b) && !cmp(b,a)`，**不是 `operator==`**。

**为什么需要它**：有序容器靠“保持顺序”才能做到 O(log n) 查找/插入；
但“顺序”不一定就是 `operator<`——可能要降序、按两数之和、按坐标距离……所以顺序必须可配置。

**常见用法**：

- 降序：`std::set<int, std::greater<int>> s;`；
- 自定义函数对象：`struct SumCmp { bool operator()(const std::pair<int,int>&, const std::pair<int,int>&) const; };`
  （P4.2 会让你写这个）；
- lambda 也能做比较器，但需要显式写出它的类型（`decltype(lambda)`）或 C++20 的新写法；
- `std::map<K,V,Cmp>` 同理，比较器只作用于 key。

**什么时候用**：需要一个“有序 + 唯一”的集合/映射，而默认顺序不是你要的顺序时。

**一个跟本题无关的例子**（按字符串长度排序，长度相同再按字典序——保证严格弱序）：

```cpp
struct ByLen {
  bool operator()(const std::string& a, const std::string& b) const {
    if (a.size() != b.size()) return a.size() < b.size();
    return a < b;                 // 同长度时用字典序打平，避免“两个不同串被判相等”
  }
};
std::set<std::string, ByLen> s{"bb", "a", "ccc"};   // a, bb, ccc
```

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

**是什么**：`unordered_map` 是哈希表（桶数组 + 碰撞处理），平均 O(1) 完成插入/查找/删除，
但**遍历顺序是未指定的**（不同实现、不同插入顺序都可能不同）。

**为什么需要它**：只要“查得快”，不在乎顺序时，它通常比 `map`（红黑树，O(log n)）更快；
词频统计、缓存、去重这类场景是它的主场。

**常见用法**：

| 需求 | 推荐 |
| :-- | :-- |
| 查存不存在、取已有值 | `find`（不插入）/ `at`（不存在抛 `std::out_of_range`） |
| 不存在则插入、已存在则保持 | `insert` / `try_emplace`（C++17） |
| 已存在则覆盖、不存在则插入 | `insert_or_assign`（C++17） |
| 计数 | `++m[key]`（会插入，确认存在时才用） |
| 遍历 | `for (const auto& [k, v] : m)` |

- **自定义键**要提供 `operator==` 和哈希：把哈希当第三个模板参数传（`PairHash`），
  或者为类型特化 `std::hash`（`std::hash<Point>`）；两条路 P4.3 都练。

**什么时候用**：频繁查找/计数、且不需要按 key 有序；需要有序遍历用 `map`，需要去重有序用 `set`。

**一个跟本题无关的例子**：

```cpp
std::unordered_map<std::string, int> freq;
for (const std::string& w : words) ++freq[w];      // 计数
if (auto it = freq.find("the"); it != freq.end()) std::cout << it->second;
```

**两个额外的坑**：`insert` / `operator[]` 可能触发 **rehash**，使**迭代器**失效
（但指向元素的指针/引用不受影响，因为节点本身不搬家）；`erase` 只失效被删元素的迭代器。

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

**是什么**：

- `auto`：让编译器从初始化表达式推导类型；
- `decltype(expr)`：给出表达式**确切**的类型（包括引用与 const）；
- 结构化绑定（C++17）：一次性把一个 pair/tuple/聚合的成员拆成几个名字。

**为什么需要它们**：C++ 的类型经常又长又难写（迭代器类型、嵌套模板），手写既痛苦又容易写错；
`auto` 让代码短且类型自动跟随。代价是 `auto` **会剥掉引用和顶层 `const`**，于是可能**静默拷贝**。
`decltype` 则是“不剥”的那个，用来保留引用。结构化绑定让 `for (auto& [k, v] : map)` 这种遍历变得干净。

**常见用法 / 规则（背下这张表）**：

| 写法 | 推出来的类型 | 会不会拷贝 |
| :-- | :-- | :-- |
| `auto a = s;`（s 是 `const std::string&`） | `std::string` | **会** |
| `auto& b = s;` | `const std::string&` | 不会 |
| `const auto& c = s;` | `const std::string&` | 不会（也能绑右值） |
| `auto&& d = std::move(s);` | `const std::string&&` | 不会 |
| `decltype(v[0])`（`vector<int>`） | `int&` | 不会 |
| `decltype(x)`（x 是变量 `int`） | `int` | 不会 |
| `decltype((x))`（带括号） | `int&` | 不会（带括号就当成“表达式”看值类别） |
| 返回类型写 `decltype(auto)` | 按 `decltype` 规则保留引用 | 视情况 |
| 结构化绑定 `auto [k,v]` | `first`/`second` 的“去引用”类型 | **会** |
| 结构化绑定 `auto& [k,v]` | 引用，可改 | 不会 |
| 结构化绑定 `const auto& [k,v]` | 只读引用 | 不会 |

**什么时候用**：迭代器与长模板类型 → `auto`；只读遍历 → `const auto&`；要改元素 → `auto&`；
需要函数返回引用 → `decltype(auto)`；遍历 `map` → `const auto& [k, v]`。

**一个跟本题无关的例子**：

```cpp
std::unordered_map<std::string, std::vector<int>> index;
for (const auto& [word, positions] : index) {   // 零拷贝（相对于 pair 而言）
  std::cout << word << " appears " << positions.size() << " times\n";
}
```

**注意**：`std::map` 的元素类型是 `std::pair<const K, V>`，所以在结构化绑定里 `k` 的类型自带 `const`；
`auto [k, v]`（无 `&`）会把整个 pair 拷贝一份。

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

**是什么**：lambda 是“就地定义的匿名函数对象”。方括号 `[]` 是**捕获列表**，决定它怎么拿到外部变量；
圆括号是参数；箭头是返回类型（可省）。

**为什么需要它**：算法（`sort`/`find_if`/`remove_if`）、回调、线程函数都需要“传一段行为进去”。
用独立函数或手写函数对象太啰嗦，而且拿不到局部变量；lambda 能就地写、还能捕获上下文。

**常见用法**：

| 写法 | 含义 |
| :-- | :-- |
| `[]` | 不捕获；用到外部变量就编译错误 |
| `[x]` | 按值捕获 `x`（拷贝一份，lambda 内改的是副本） |
| `[&x]` | 按引用捕获 `x`（能改到外面那个变量） |
| `[=]` / `[&]` | 按值/引用捕获所有用到的外部变量（范围太大，慎用） |
| `[this]` | 捕获当前对象的 `this`（成员函数里用） |
| `[x = std::move(v)]` | 初始化捕获（C++14）：把 `v` 移进 lambda |
| `[=]() mutable { ... }` | 允许修改按值捕获的副本 |
| `[](auto x) { ... }` | 泛型 lambda（C++14） |

- lambda 的类型是编译器生成的**唯一匿名类型**；要把它存起来通常用 `auto` 或 `std::function`
  （后者有类型擦除开销，能 `auto` 就 `auto`）。
- 想显式写返回类型：`[](int x) -> long { return x; }`。

**什么时候用**：给算法传谓词/比较器、写回调、写线程函数、写“延迟执行”的闭包。

**一个跟本题无关的例子**：

```cpp
std::vector<int> v{3, 1, 4, 1, 5};
int threshold = 2;
int cnt = std::count_if(v.begin(), v.end(), [threshold](int x) { return x > threshold; });
std::sort(v.begin(), v.end(), [](int a, int b) { return a > b; });   // 降序
```

**两个必须记住的陷阱**：不要把捕获了**局部引用**的 lambda 存起来/返回出去（悬垂）；
捕获 `[this]` 的 lambda 被存到对象死了之后再调用，也是悬垂。

### 1.8 erase-remove 惯用法

`std::remove_if(begin, end, pred)` **不会真的删除元素**，也不能改变容器大小
（它只拿到一对迭代器，不知道容器本体）。它的实际行为是：把"不该删"的元素往前挪，返回一个迭代器 `new_end`，
指向第一个"应该被删掉"的位置。所以要真正删除，必须再调用容器的 `erase`：

```cpp
v.erase(std::remove_if(v.begin(), v.end(), pred), v.end());
```

`vectors.cpp` 里 `point_vector.erase(std::remove_if(...), point_vector.end())` 就是这一套。
如果只调 `remove_if` 不 `erase`，`v.size()` 不会变，末尾会留下"已经搬走"的无效内容。

**是什么**：erase-remove 惯用法 = `std::remove` / `std::remove_if` 把“该保留”的元素往前挪、
返回新的逻辑结尾，再用容器的 `erase` 真正收缩 size。

**为什么需要它**：`std::remove_if` 只拿到一对迭代器，**根本不知道容器本体**，所以它没有能力改 `size`。
它是算法库（`<algorithm>`）里的通用算法，不是容器成员。所以必须由容器自己 `erase`。

**常见用法**：

```cpp
// 按谓词原地删除
v.erase(std::remove_if(v.begin(), v.end(), pred), v.end());

// 按值删除
v.erase(std::remove(v.begin(), v.end(), 42), v.end());

// 过滤到新容器（不修改原容器）
std::vector<T> out;
std::copy_if(v.begin(), v.end(), std::back_inserter(out), pred);   // 需要 <iterator>
```

- `std::unique` 也是同一类算法（先去重，但它只去掉**相邻**重复，通常要先排序）。
- `std::back_inserter(out)` 是一个“插入迭代器”：对它赋值就等于 `out.push_back(...)`。

**什么时候用**：原地删除满足条件的元素、且要保持其余元素的相对顺序时（P4.5 的 `RemoveLevel` 就是）。

**一个跟本题无关的例子**：

```cpp
std::vector<int> v{1, 2, 3, 4, 5, 6};
v.erase(std::remove_if(v.begin(), v.end(), [](int x) { return x % 2 == 0; }), v.end());
// v == {1, 3, 5}
```

**一个常见的错**：只写 `std::remove_if(...)` 不接 `erase`。编译能过，但 `size()` 没变，
末尾几个位置里放着“被搬值后剩下的旧内容”，之后遍历就会看到不该看到的元素。

---

## 2. 题目

下面按编号列出全部题目（一览表见第 0 节）。建议先做主线题 P4.2 → P4.4 → P4.5，
再做扩展题 P4.1 → P4.3；也可以按编号做。

### P4.1 `vector`：迭代器失效 / `reserve` vs `resize` / 下标

- **类型**：扩展（第 1.2、1.3 节）

- **考什么**：1.2 与 1.3。题目本身不要求你实现容器，而是要求你写一组**测量函数**，
  用它们把"地址什么时候变、capacity 什么时候涨"变成可断言的事实。
- **你要写**：`projects/mysol/stage4/p4_1_vector_invalidation.h`。只写头文件。
- **复制过来的测评文件**：`tests/stage4/p4_1_vector_invalidation_test.cpp`。测评点数：17。

**测评程序要求 `namespace v4` 里提供：**

> ⚠️ **命名空间**：测评文件里写着 `using namespace v4;`，所以下面这些名字**必须**放在 `namespace v4` 里。
> 如果你直接写在全局，编译会报 `'v4' is not a namespace-name`。

```cpp
struct CapSize {
  size_t cap;      // capacity
  size_t size;     // size
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

**下面这些你必须自己想清楚、自己补上（只给要求，不给能直接复制的代码）：**

- `PushNTimes(n)` 要用一个**不 reserve** 的 `vector`，push n 次，返回最终的 `{capacity, size}`；
- `ReserveThenPush` 要先 `reserve(reserve_n)` 再 push `push_n` 次；
- `DataAddr` 把 `v.data()` 转成 `uintptr_t`（用 `reinterpret_cast`），目的是避免“比较悬垂指针”的语义争议；
- `AddressStableWithinCapacity` / `AddressChangesPastCapacity` 要分别在“不超 capacity”与“越过 capacity”
  两种情况下比较**地址是否变化**，返回 `bool`。（想一想：后者还必须保证开始的 `exceed` 确实超过初始 capacity。）
- 五个函数都放在 `namespace v4` 里（见上方警告）。

**要做的事**：实现这 5 个测量函数。注意它们的**返回值语义**（各函数的注释就是规格）。
额外要求：`CapSize` 的成员名字必须是 `cap` 和 `size`（测评直接访问）。

**测评点在查什么**：新 vector 为空；`reserve` 只改 capacity 不改 size；`reserve` 出来的位置
用 `at` 访问会抛异常；`resize` 改 size 且新元素为 0、可写；`resize` 变小会截断；`clear` 保留 capacity；
预留容量内地址不变；越过 capacity 地址改变；`reserve` 后 push 不动地址；capacity 恒 >= size；
5000 次 push 的重分配次数远小于 5000（摊还增长）；迭代器算术与 `erase`/`insert`；100 万规模的 capacity/size。

**编译运行**：

```bash
cd projects/mysol/stage4
g++ -std=c++17 p4_1_vector_invalidation_test.cpp -I../../tests -o p4_1 && ./p4_1
```

通过标准：`Result: 17/17 Passed`。

**自查**：① `v.reserve(100)` 之后 `v[50] = 1` 合法吗？`v.resize(100)` 之后呢？② 哪些操作会让所有迭代器失效？③ 判断旧指针是否安全，看的是 size 还是 capacity？

---

### P4.2 `set`：有序、去重、自定义比较器；`map` 词频

- **类型**：主线（第 1.4 节补充了比较器/严格弱序）
- **考什么**：`set` 的有序去重与三种 `erase` 形式、降序集合、`pair` 的自然序、
  自定义比较器与严格弱序（1.4）、`map` 词频统计与有序遍历。
- **你要写**：`projects/mysol/stage4/p4_2_set_comparator.h`。只写头文件。
- **复制过来的测评文件**：`tests/stage4/p4_2_set_comparator_test.cpp`。测评点数：16。

**测评程序要求 `namespace s4` 里提供：**

> ⚠️ **命名空间**：测评文件里写着 `using namespace s4;`，所以下面这些名字**必须**放在 `namespace s4` 里。

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

**下面这些你必须自己想清楚、自己补上（只给要求，不给能直接复制的代码）：**

- `SortedUnique`：用 `set` 的区间构造函数（两个迭代器）最省事；
- `JoinAscending` / `JoinBySumOrder` / `KeysJoined`：注意**第一个元素前不加空格**，
  `JoinBySumOrder` 的格式是 `(a,b)`；遍历顺序就是 `set`/`map` 的顺序，不要额外排序；
- `SumCmp`：先比 `a.first + a.second`，相等时再比 `a < b`（`pair` 自带的字典序）——
  不能只比和，否则“和相等但内容不同”的两个 pair 会被 `set` 当成同一个元素；
- `operator()` 必须是 **`const` 成员函数**，且返回“严格弱序”的结果（用 `<`，不能用 `<=`）。见 1.4。

**要做的事**：实现以上 6 个函数/类型。`SortedUnique` 可以直接用 `set` 的区间构造；
`JoinAscending`/`JoinBySumOrder`/`KeysJoined` 按注释的格式拼接字符串。

**测评点在查什么**：去重升序（含空集、单元素、负数）；`insert` 的返回值表示是否新插入；
`find`/`count`；`erase(key)` 与 `erase(iterator)` 与 `erase(range)`；`std::greater<int>` 降序；
`pair` 自然序；自定义比较器按和排序；比较器满足严格弱序（`a<a` 为 false 等）；
`map` 词频与有序 key；10 万个值去重；随机插入后仍有序。

**编译运行**：

```bash
cd projects/mysol/stage4
g++ -std=c++17 p4_2_set_comparator_test.cpp -I../../tests -o p4_2 && ./p4_2
```

通过标准：`Result: 16/16 Passed`。

**自查**：① `insert` 返回什么，怎么用它判断是否插入成功？② 比较器的 `operator()` 为什么必须 `const`？③ 为什么比较器写 `<=` 会让 `set` 丢掉数据？

---

### P4.3 `unordered_map`：`operator[]` 陷阱 + 自定义键的 `std::hash`

- **类型**：扩展（第 1.5 节）
- **考什么**：1.5。`operator[]` 会默认插入；`find`/`at`/`try_emplace`/`insert_or_assign` 的区别；
  给 `pair` 当 key 写哈希函数；给自定义结构当 key 特化 `std::hash`。
- **你要写**：`projects/mysol/stage4/p4_3_unordered_custom_key.h`。只写头文件。
- **复制过来的测评文件**：`tests/stage4/p4_3_unordered_custom_key_test.cpp`。测评点数：16。

**测评程序要求你提供：**

> ⚠️ **命名空间**：`PairHash` / `PairMap` / `Point` / `MakeBasicMap` 必须放在 `namespace u4` 里
> （测评文件写着 `using namespace u4;`）；而 `std::hash<u4::Point>` 的特化必须写在 `namespace std` 里。

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

**下面这些你必须自己想清楚、自己补上（只给要求，不给能直接复制的代码）：**

- `Point::operator==`：逐成员比较（`x == o.x && y == o.y`）；`unordered_map` 靠它判断键相等。
- `PairHash::operator()`：把两个 `int` 的哈希组合起来，必须是 `const`，返回 `size_t`。
- `std::hash<u4::Point>` 特化：写在 `namespace std` 里，`operator()` 应该是 `const noexcept`。
- `MakeBasicMap`：正好四个键值对 `foo=2, jignesh=445, spam=1, eggs=2`（值不能写错，测评会断言）。
- 所有这些名字都放在 `namespace u4` 里，除了 `std::hash` 特化放在 `namespace std`。

**要做的事**：实现 `PairHash`、`Point` 的 `operator==`、`std::hash<Point>` 特化、`MakeBasicMap`。

**测评点在查什么**：`m["nope"]` 会插入默认值并使 `size()` 变成 1；`find` 不插入；`at` 抛
`std::out_of_range` 且不插入；`at` 取已有键；`insert` 遇到重复键保留旧值；按 key / 按迭代器 `erase`；
`try_emplace` 不覆盖、`insert_or_assign` 覆盖；`pair` 键（含负数）能 find/insert；
`Point` 键与 `unordered_set<Point>`；大量冲突键仍全部可查；10 万键往返；遍历每个 key 恰好一次；
`m[key] += ...` 依赖默认构造。

**编译运行**：

```bash
cd projects/mysol/stage4
g++ -std=c++17 p4_3_unordered_custom_key_test.cpp -I../../tests -o p4_3 && ./p4_3
```

通过标准：`Result: 16/16 Passed`。

**自查**：① `unordered_map<K,V>` 对 `K` 有哪两个要求？② `m[key]` 为什么要求 `V` 可默认构造？③ 哈希冲突会破坏正确性吗？

---

### P4.4 `auto` / `decltype` / 结构化绑定

- **类型**：主线（src 的 auto.cpp + 第 1.6 节的 decltype）
- **考什么**：1.6。`auto` 剥引用与顶层 const、`const auto&` 借用、结构化绑定、`decltype` 与
  `decltype(auto)` 保留引用。
- **你要写**：`projects/mysol/stage4/p4_4_auto_decltype.h`。只写头文件。
- **复制过来的测评文件**：`tests/stage4/p4_4_auto_decltype_test.cpp`。测评点数：17。

**测评程序会用到的接口（名字必须一致）：**

> ⚠️ **命名空间**：下面这些名字（包括 `Big`、`AtRef`、`AtVal`、`ScaleValues`、`SumPairs`）
> **必须全部放在 `namespace a4` 里**。测评文件里写着 `using namespace a4;`，
> 如果你定义在全局命名空间，`a4` 这个命名空间根本不存在，编译会直接失败。

```cpp
struct Big {                       // 带拷贝计数的类型，用来证明 auto 会静默拷贝
  int v;                           // 要有一个 int 成员
  explicit Big(int x);
  Big();                           // 默认构造（测试会默认构造/拷它）
  static void Reset();             // 把 copies 计数器清零
};

template <typename C> decltype(auto) AtRef(C& c, size_t i);  // 返回 c[i] 原样（vector 上是 int&）
template <typename C> auto AtVal(C& c, size_t i);            // 剥掉引用，返回拷贝

std::string ScaleValues(std::map<std::string,int>& m, int factor);  // 用 auto& [k,v] 原地放大 value
int SumPairs(const std::vector<std::pair<int,int>>& v);      // 用结构化绑定求和
```

`Big` 还需要一个**公开的静态计数器 `copies`**（每拷贝/拷贝赋值 +1，声明方式见 1.5），
以及相应的拷贝构造和拷贝赋值——这两处都要让 `copies` 自增。

**下面这些你必须自己想清楚、自己补上（只给要求，不给能直接复制的代码）：**

- **`Big` 的内部成员**：一个 `int`（名字就叫 `v`，测评会读 `y.v`）；还有一个 `inline static int copies` 计数器。
- **`Big` 的拷贝控制**：拷贝构造、拷贝赋值都要让 `copies` 自增（默认构造、移动不增）。
  注意：手写了拷贝构造后，编译器不会再自动生成移动构造，但本题不需要移动。
- **`AtRef` 为什么用 `decltype(auto)`**：`c[i]` 在 `vector` 上是 `int&`，`decltype(auto)` 会把这个引用原样带出去；
  写成 `auto` 就变成了值（拷贝）。两个版本都要写。
- **`ScaleValues` 的遍历**：必须用 `for (auto& [k, v] : m)` 才能原地改 value；
  用 `for (auto [k, v] : m)` 只会改副本。返回值是把每个 `key + to_string(value)` 拼接起来。
- **`SumPairs`**：用 `const auto& [a, b]` 遍历，累加 `a + b`。

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
g++ -std=c++17 p4_4_auto_decltype_test.cpp -I../../tests -o p4_4 && ./p4_4
```

通过标准：`Result: 17/17 Passed`。

**自查**：① `for (auto x : v)` / `for (auto& x : v)` / `for (const auto& x : v)` 各自适合什么场景？② `for (auto [k,v] : m)` 会发生什么？③ `decltype(auto)` 和 `auto` 的核心差别？

---

### P4.5 Log Analyzer（`erase-remove` + lambda + 综合容器）

- **类型**：主线（src 的 vectors.cpp + 第 1.7、1.8 节）
- **考什么**：把 `vector` / `set` / `unordered_map` / lambda 捕获 / erase-remove 串成一个完整小工具。
- **你要写**：`projects/mysol/stage4/p4_5_log_analyzer.h`。只写头文件，不要写 `main()`。
- **复制过来的测评文件**：
  - `tests/stage4/p4_5_log_analyzer_test.cpp`（分级测试点，17 个）
  - `tests/stage4/p4_5_log_analyzer_cli.cpp`（**这是测评配套的命令行外壳，不用你写**：
    它读 stdin、调用你头文件里的函数、把结果打到 stdout。你只要把测评文件复制过来、
    和你自己的头文件一起编译即可）

**测评程序要求 `namespace log5` 里提供：**

> ⚠️ **命名空间**：测评文件里写着 `using namespace log5;`，所以要写的所有名字都放进 `namespace log5`。

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

**下面这些你必须自己想清楚、自己补上（只给要求，不给能直接复制的代码）：**

- `CountLevels`：用 `unordered_map` 计数（`++counts[lvl]`）；
- `UniqueLevels`：用 `set` 的区间构造实现“有序去重”；
- `Filter`：返回**新** `vector`，保持原相对顺序，不要修改输入；
- `RemoveLevel`：**必须**用 `erase(remove_if(...), logs.end())`（见 1.8），且 lambda 捕获 `level`；
- `CountOf`：必须用 `find` 而不是 `operator[]`，否则查询不存在的 level 会静静往表里插一个键
  （测评会检查 `c.size()` 没变）；
- `Join` 是两个重载（`set<string>` 和 `vector<string>`），空格连接，第一个前面不加空格；
- 所有名字都放在 `namespace log5` 里。

**要做的事**：实现以上 7 个函数。

**测评点在查什么**：计数（基本/空/不存在键为 0）；有序去重（基本/空/单元素）；
过滤保持顺序与条数、过滤不存在的 level；删除保持相对顺序、删除不存在的 level 是空操作、
删掉所有 level 后为空、重复删除幂等；lambda 捕获对比；10 万条计数；
与"线性暴力实现"随机对拍 200 轮；中等规模随机对拍。

**编译运行（分级测试点）**：

```bash
cd projects/mysol/stage4
g++ -std=c++17 p4_5_log_analyzer_test.cpp -I../../tests -o p4_5 && ./p4_5
```

通过标准：`Result: 17/17 Passed`。

**洛谷式 stdin/stdout 对拍**（一共 14 个用例，`.ans` 由独立的 Python 朴素实现生成，是外部校验）：

```bash
cd projects/mysol/stage4
g++ -std=c++17 p4_5_log_analyzer_cli.cpp -o cli
bash ../../tests/stage4/tests/run_tests.sh ./cli
```

通过标准：最后一行 `---------------- AC 14 / 14 ----------------`。

**自查**：① 为什么必须 `erase(remove_if(...), end())` 而不能只调 `remove_if`？② `[&level]` 和 `[level]` 有什么区别？③ 遍历 `unordered_map` 用结构化绑定拿到的 key 为什么是 `const`？
