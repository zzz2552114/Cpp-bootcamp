# Stage 4 · STL 容器

对应源文件：`4 - Containers/vectors.cpp`、`sets.cpp`、`unordered_maps.cpp`、`auto.cpp`。

---
## P4.1 vector：迭代器失效 / reserve vs resize / 下标

**对应 / 前置**：读完 `vectors.cpp`。
**目标**：`std::vector` 扩容导致**迭代器/指针/引用失效**——BusTub bug 高发区。

**任务**：

1. 写代码复现迭代器失效：拿到 `auto it = v.begin();` 后连续 `push_back` 到越过 capacity，再解引用旧 `it`（**UB，运行结果随机**；用 `-fsanitize=address` 或观察垃圾值/崩溃）。解释为什么：`realloc` 把元素搬到新内存，旧迭代器还指着旧地址。
2. 对比 `reserve(n)`（只预留容量、`size()` 不变、`operator[]` 访问预留段仍是 UB）与 `resize(n)`（真把 size 改到 n、并默认初始化）。用 `capacity()/size()` 打印证明。
3. 用 `reserve` 重写 1：先 `reserve` 再取迭代器再 push 若干次，只要总 size 不超 capacity，迭代器仍然有效。讲清"失效的边界是 capacity，不是 size"。

**验收**：能打印出"push 前后 capacity/size"，并用注释标出哪一步之后旧迭代器不再安全。

**自查三问**：
1. `v.reserve(100);` 之后 `v[50] = 1;` 合法吗？`v.resize(100);` 之后呢？
2. 哪些操作会失效 vector 的所有迭代器？哪些只失效末尾之后的？
3. `push_back` 在"已越 capacity"和"未越 capacity"两种情况下，对旧迭代器的安全性有何不同？

---
## P4.2 set：有序、去重、自定义比较器

**对应 / 前置**：读完 `sets.cpp`。
**目标**：`std::set` 是**有序 + 唯一**；`std::map` 是它的 key→value 版；比较器换排序规则。

**任务**：
1. 用 `std::set<int>` 收集一堆含重复的数，按序遍历打印（应升序去重）。`find/count/erase` 各演练一次（含 `erase(iterator)` 与 `erase(key)` 两种）。
2. **降序 set**：`std::set<int, std::greater<int>>`，遍历应降序。
3. 把 `std::set<std::pair<int,int>>` 按"先 first 后 second"自然序排序；再写一个按"两个数之和"排序的自定义比较器 `struct Cmp { bool operator()(const P&a,const P&b) const; }`，说明为什么比较器必须是**严格弱序**（`a<a` 必须 false）。
4. 顺带用 `std::map<std::string,int>` 做一次词频统计，比较与 `unordered_map` 的遍历序差异。

**验收**：`{5,1,5,3}` → 升序 `1 3 5`；降序 set → `5 3 1`；pair 自定义比较器按和排序输出正确且元素唯一。

**自查三问**：
1. `std::set` 的 `insert` 返回什么？怎么用它判断"是否插入成功"？
2. 比较器的 `operator()` 为什么要 `const`？写成非常量成员会发生什么？
3. 什么时候用 `set`/`map`（序有用），什么时候用 `unordered_*`（只求 O(1) 查）？

---
## P4.3 unordered_map：自定义键必须特化 `std::hash` + `operator[]` 陷阱

**对应 / 前置**：读完 `unordered_maps.cpp`。
**目标**：把 `std::pair`/自定义结构当 key 需要 hash + 相等；`operator[]` 会默认插入。

**任务**：
1. 观察 `operator[]` 陷阱：对一个**空的** `std::unordered_map<std::string,int> m` 执行 `int x = m["nope"];`，打印 `m.size()` 与 `x`——key 被**默认插入**了吗？再对比 `m.find("nope")==m.end()` 与 `m.at("nope")`（会 throw）。给出"查 vs 插入"该用哪种的正确姿势。
2. `std::pair<int,int>` 作 key：写一个 `struct PairHash { size_t operator()(const std::pair<int,int>&) const; }`（合并两个 hash，例如 `h1 ^ (h2<<1)`），并给相等比较，构造 `std::unordered_map<std::pair<int,int>,int, PairHash>`。或直接用 C++17 官方写法，把 `std::hash<int>` 组合起来。
3. **自定义结构 key**：`struct Point{int x,y; bool operator==(const Point&) const;};` + 特化 `std::hash<Point>`，放进 `unordered_map<Point,int>`。
4. `try_emplace` / `insert_or_assign`（C++17）各自语义，各演示一次。

**验收**：pair key 与 Point key 的 map 都能正确 find/insert/遍历；能答出 `m["k"]` 与 `m.at("k")` 在 key 缺失时的差异。

**自查三问**：
1. `unordered_map<K,V>` 需要 K 满足哪两样？`std::hash<K>` 与 `K::operator==` 各自的角色？
2. `m[key]` 为什么要求 `V` 可默认构造？
3. hash 冲突时 `unordered_map` 怎么处理？（铅笔图：桶 + 链）

---
## P4.4 auto / decltype / 结构化绑定（类型推导的坑）

**对应 / 前置**：读完 `auto.cpp`。
**目标**：`auto` 剥引用与 const；`const auto&` 借用；结构化绑定遍历 map/pair。

**任务**：
1. **推导对照**：`const std::string s="hi"; auto a=s; auto& b=s; const auto& c=s; auto&& d=std::move(s);` 逐个用 `static_assert(std::is_same_v<...>)` 断言四者的真实类型。
2. **展示 auto 会拷贝**：一个带"拷贝构造打印"的 `struct Big{ Big(); Big(const Big&); }`，`auto x = big;`（打印 copy）vs `const auto& y = big;`（无 copy）。
3. **结构化绑定**：`std::map<std::string,int> m; for (auto& [k,v] : m) {...}` 修改 v 并验证生效；`for (const auto& [k,v] : m)` 只读。注意 `auto&` 绑到 `std::pair<const std::string,int>` 上，key 类型里带 `const`。
4. **decltype**：`std::vector<int> v; decltype(v[0]) x = v[0];` 的 x 类型是什么（`int&`？）；`decltype(auto)` 保留引用；对比 `auto` 会怎样。

**验收**：所有 `static_assert` 通过；能填一张表（`auto`/`auto&`/`const auto&`/`auto&&`/`decltype(auto)` 各自保留什么、剥掉什么）。

**自查三问**：
1. `for (auto x : v)` 与 `for (auto& x : v)` 与 `for (const auto& x : v)` 何时选哪个？（copy/修改/只读）
2. `for (auto [k,v] : m)`（不带 &）会发生什么？值拷贝的后果？
3. `decltype(auto)` 与 `auto` 的核心差别是什么？

---
## P4.5 Log Analyzer（erase-remove + lambda 捕获 + 综合容器）

**对应 / 前置**：读完 `vectors.cpp`（它的 `remove_if` 是关键词）。
**目标**：把 vector/set/unordered_map/lambda/erase-remove 串成一个完整小工具。

**任务**：给定 `std::vector<std::string> logs`，每个元素形如 `"INFO" / "ERROR" / "WARN" ...`：

```cpp
std::unordered_map<std::string,int> CountLevels(const std::vector<std::string>& logs);
std::set<std::string> UniqueLevels(const std::vector<std::string>& logs);   // 有序去重
std::vector<std::string> Filter(const std::vector<std::string>& logs, const std::string& level);
// 删除某 level 的所有元素（原地，用 erase-remove idiom）
void RemoveLevel(std::vector<std::string>& logs, const std::string& level);
```

1. `RemoveLevel` 内必须用 `std::remove_if(..., [&level](const std::string& l){ return l==level; })` 配合 `erase`。
2. 解释 lambda 的 `[&level]`：为什么能捕获外层 `level`？换成 `[level]` 呢？`[]` 呢（会编译错）？
3. 用 `for (const auto& [level, cnt] : CountLevels(logs))` … 打印（体会 P4.4 的结构化绑定）。

**验收**：对
`{"INFO","ERROR","INFO","WARN","ERROR","INFO"}`
→ `CountLevels` = {INFO:3, ERROR:2, WARN:1}；`UniqueLevels` 有序输出 `ERROR INFO WARN`；`Filter(logs,"ERROR")` = 2 条；`RemoveLevel(logs,"INFO")` 后 `logs` 里无 INFO 且顺序保持 ERROR WARN ERROR。

**自查三问**：
1. 为什么 `erase(remove_if(...), end())` 而不是直接 `remove_if`？`remove_if` 返回什么？
2. `[&level]` 与 `[level]` 捕获的对象生命周期各依赖什么？
3. 遍历 `unordered_map` 用结构化绑定拿到的是 `pair<const K,V>`——为什么 key 是 `const`？