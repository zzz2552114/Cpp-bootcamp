// P3.3 Rule of 0/3/5
#pragma once
#include <memory>
#include <utility>

namespace r3 {

// 全局的节点分配计数：用来验证"无泄漏、无重复释放"
inline int g_allocs = 0;
inline int g_frees = 0;
inline void ResetCounters() { g_allocs = g_frees = 0; }

// ---------- 反面教材：裸指针 + 编译器隐式生成的浅拷贝 ----------
struct NaiveList {
  struct N {
    int v;
    N *next;
    explicit N(int x) : v(x), next(nullptr) { ++g_allocs; }
    ~N() { ++g_frees; }
  };

  N *head = nullptr;

  NaiveList() = default;
  ~NaiveList() { Clear(); }
  // ⚠️ 这里【故意】不写拷贝构造/拷贝赋值 → 编译器生成"逐成员拷贝"（浅拷贝）。
  //    NaiveList b = a; 会让 a.head == b.head，析构时各 delete 一次 → double free。

  void Push(int v) { N *n = new N(v); n->next = head; head = n; }

  void Clear() {
    while (head) { N *n = head->next; delete head; head = n; }
  }
};

// ---------- Rule of 3：手写析构 + 拷贝构造 + 拷贝赋值 ----------
class List3 {
public:
  struct N {
    int v;
    N *next;
    explicit N(int x) : v(x), next(nullptr) { ++g_allocs; }
    ~N() { ++g_frees; }
  };

  List3() = default;
  ~List3() { Clear(); }

  List3(const List3 &o) { CopyFrom(o); }                 // 深拷贝
  List3 &operator=(const List3 &o) {                     // 先清空，再深拷贝
    if (this != &o) { Clear(); CopyFrom(o); }
    return *this;
  }

  void Push(int v) { N *n = new N(v); n->next = head_; head_ = n; }
  const N *Head() const { return head_; }

private:
  void Clear() {
    while (head_) { N *n = head_->next; delete head_; head_ = n; }
  }
  void CopyFrom(const List3 &o) {
    head_ = nullptr;
    N **tail = &head_;
    for (N *p = o.head_; p; p = p->next) {
      *tail = new N(p->v);          // 逐节点新建 → 两块内存互不相干
      tail = &(*tail)->next;
    }
  }
  N *head_ = nullptr;
};

// ---------- Rule of 5：Rule of 3 + 移动构造 + 移动赋值 ----------
class List5 {
public:
  struct N {
    int v;
    N *next;
    explicit N(int x) : v(x), next(nullptr) { ++g_allocs; }
    ~N() { ++g_frees; }
  };

  List5() = default;
  ~List5() { Clear(); }

  List5(const List5 &o) { CopyFrom(o); }
  List5 &operator=(const List5 &o) {
    if (this != &o) { Clear(); CopyFrom(o); }
    return *this;
  }

  List5(List5 &&o) noexcept : head_(o.head_) { o.head_ = nullptr; }   // 偷指针
  List5 &operator=(List5 &&o) noexcept {
    if (this != &o) { Clear(); head_ = o.head_; o.head_ = nullptr; }
    return *this;
  }

  void Push(int v) { N *n = new N(v); n->next = head_; head_ = n; }
  N *Head() const { return head_; }

private:
  void Clear() {
    while (head_) { N *n = head_->next; delete head_; head_ = n; }
  }
  void CopyFrom(const List5 &o) {
    head_ = nullptr;
    N **tail = &head_;
    for (N *p = o.head_; p; p = p->next) {
      *tail = new N(p->v);
      tail = &(*tail)->next;
    }
  }
  N *head_ = nullptr;
};

// ---------- Rule of 0：让成员自己把所有权/拷贝/移动都弄对 ----------
class List0 {
public:
  struct N {
    int v;
    // ★ 关键：链表要"自动级联析构"，光让 head_ 是 unique_ptr 是不够的，
    //   必须让【每个节点自己拥有它的后继】。这样整条链就是一条 ownership 链。
    //   如果 next 写成裸 N*，那么 head_ 析构时只会 delete 头节点，其余全部泄漏。
    std::unique_ptr<N> next;
    explicit N(int x) : v(x) { ++g_allocs; }
    ~N() { ++g_frees; }
  };

  void Push(int v) {
    auto n = std::make_unique<N>(v);
    n->next = std::move(head_);     // 新节点接管旧链
    head_ = std::move(n);
  }

  int Head() const { return head_ ? head_->v : -1; }
  bool Empty() const { return head_ == nullptr; }
  size_t Size() const {
    size_t n = 0;
    for (const N *p = head_.get(); p; p = p->next.get()) ++n;
    return n;
  }

private:
  // 唯一 owner：拷贝自动 =delete，析构自动级联，移动自动可用。
  // 一行特殊成员函数都不用写 —— 这就是 Rule of 0。
  // 注意：级联析构是递归的，链表过长可能爆栈（真实项目里会改成迭代式销毁）。
  std::unique_ptr<N> head_;
};

}  // namespace r3
