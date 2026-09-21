// P3.2 双向迭代器：支持 --End()、返回引用、配合 range-for
#pragma once
#include <cstddef>
#include <iterator>

struct Node {
  int value_;
  Node *next_;
  Node *prev_;
  explicit Node(int v) : value_(v), next_(nullptr), prev_(nullptr) {}
};

class DLLIterator {
public:
  // 除了"当前节点"，还要带上 tail_：
  // 因为 End() 的 curr_ 是 nullptr，往回走时只能靠 tail_ 才知道尾在哪。
  DLLIterator(Node *curr, Node *tail) : curr_(curr), tail_(tail) {}

  // 返回引用（不是拷贝），这样 *it = 5 才能真正改到节点
  int &operator*() { return curr_->value_; }
  const int &operator*() const { return curr_->value_; }

  DLLIterator &operator++() {                 // 前缀 ++it
    curr_ = curr_->next_;
    return *this;
  }

  DLLIterator operator++(int) {               // 后缀 it++：必须留下旧值
    DLLIterator temp = *this;
    ++*this;
    return temp;
  }

  DLLIterator &operator--() {                 // 前缀 --it
    // 关键修复：从 End() 回退时 curr_ == nullptr，必须回到 tail_，
    // 否则 curr_->prev_ 就是解引用空指针 → 段错误
    curr_ = (curr_ == nullptr) ? tail_ : curr_->prev_;
    return *this;
  }

  DLLIterator operator--(int) {               // 后缀 it--
    DLLIterator tmp = *this;
    --*this;
    return tmp;
  }

  bool operator==(const DLLIterator &o) const { return curr_ == o.curr_; }
  bool operator!=(const DLLIterator &o) const { return curr_ != o.curr_; }

  // 便于测试观察内部状态
  Node *Raw() const { return curr_; }

private:
  Node *curr_;
  Node *tail_;
};

class DLL {
public:
  DLL() = default;
  ~DLL() { Clear(); }

  // 裸指针成员 + 手写析构 → 默认浅拷贝会 double free，这里直接禁用
  // （为什么、以及如何实现深拷贝，见 P3.3）
  DLL(const DLL &) = delete;
  DLL &operator=(const DLL &) = delete;
  DLL(DLL &&) = delete;
  DLL &operator=(DLL &&) = delete;

  void InsertAtHead(int v) {
    Node *n = new Node(v);
    n->next_ = head_;
    if (head_) {
      head_->prev_ = n;
    } else {
      tail_ = n;                              // 第一个节点同时也是尾节点
    }
    head_ = n;
    ++size_;
  }

  DLLIterator Begin() { return DLLIterator(head_, tail_); }
  DLLIterator End() { return DLLIterator(nullptr, tail_); }

  // range-for 需要的是小写 begin()/end()
  DLLIterator begin() { return Begin(); }
  DLLIterator end() { return End(); }

  size_t Size() const { return size_; }
  bool Empty() const { return size_ == 0; }
  int Front() const { return head_->value_; }
  int Back() const { return tail_->value_; }

private:
  void Clear() {
    Node *cur = head_;
    while (cur) {
      Node *nxt = cur->next_;
      delete cur;
      cur = nxt;
    }
    head_ = tail_ = nullptr;
    size_ = 0;
  }

  Node *head_ = nullptr;
  Node *tail_ = nullptr;                      // 新增：支持 --End()
  size_t size_ = 0;
};

// 让 std::distance 之类也能用（可选，标准库 traits）
namespace std {
template <> struct iterator_traits<DLLIterator> {
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  using pointer = int *;
  using reference = int &;
};
}  // namespace std
