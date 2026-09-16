// P2.2 测试点
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "test_util.h"
#include "p2_2_stack.h"

namespace {
struct Tracked {
  static int copies;
  static int moves;
  int v;
  explicit Tracked(int x) : v(x) {}
  Tracked(const Tracked &o) : v(o.v) { ++copies; }
  Tracked(Tracked &&o) noexcept : v(o.v) { ++moves; }
  static void Reset() { copies = 0; moves = 0; }
};
int Tracked::copies = 0;
int Tracked::moves = 0;
}  // namespace

BT_TEST(P2_2, new_stack_is_empty) {
  Stack<int> s;
  BT_CHECK(s.Empty());
  BT_CHECK_EQ(s.Size(), static_cast<size_t>(0));
}

BT_TEST(P2_2, push_and_top) {
  Stack<int> s;
  s.Push(10);
  s.Push(20);
  s.Push(30);
  BT_CHECK_EQ(s.Size(), static_cast<size_t>(3));
  BT_CHECK_EQ(s.Top(), 30);
}

BT_TEST(P2_2, pop_is_lifo) {
  Stack<int> s;
  for (int i = 1; i <= 5; ++i) s.Push(i);
  int expect = 5;
  while (!s.Empty()) {
    BT_CHECK_EQ(s.Top(), expect);
    s.Pop();
    --expect;
  }
  BT_CHECK_EQ(expect, 0);
}

BT_TEST(P2_2, top_returns_reference_so_can_modify) {
  Stack<int> s;
  s.Push(1);
  s.Top() = 99;
  BT_CHECK_EQ(s.Top(), 99);
}

BT_TEST(P2_2, const_top_is_readonly_view) {
  Stack<int> s;
  s.Push(1);
  s.Push(2);
  const Stack<int> &cs = s;
  BT_CHECK_EQ(cs.Top(), 2);
  BT_CHECK_EQ(cs.Size(), static_cast<size_t>(2));
}

BT_TEST(P2_2, pop_on_empty_throws) {
  Stack<int> s;
  BT_CHECK_THROWS(s.Pop(), std::out_of_range);
}

BT_TEST(P2_2, top_on_empty_throws) {
  Stack<int> s;
  BT_CHECK_THROWS(s.Top(), std::out_of_range);
  const Stack<int> &cs = s;
  BT_CHECK_THROWS(cs.Top(), std::out_of_range);
}

BT_TEST(P2_2, lvalue_push_copies_rvalue_push_moves) {
  Tracked::Reset();
  Stack<Tracked> s;
  Tracked a(1);
  s.Push(a);                    // 左值 → 拷贝
  BT_CHECK_EQ(Tracked::copies, 1);

  Tracked::Reset();
  s.Push(Tracked(2));           // 临时对象 → 移动
  // 注意：这里 vector 会扩容（capacity 1→2），已有元素也会被"移动"一次，
  // 所以 moves 可能是 2。关键的确定性结论是【一次拷贝都没有】。
  BT_CHECK(Tracked::moves >= 1);
  BT_CHECK_EQ(Tracked::copies, 0);

  Tracked::Reset();
  s.Push(std::move(a));         // 显式移动左值
  BT_CHECK(Tracked::moves >= 1);
  BT_CHECK_EQ(Tracked::copies, 0);    // 同样：绝不拷贝
  BT_CHECK_EQ(s.Size(), static_cast<size_t>(3));
}

BT_TEST(P2_2, move_only_type_works) {
  Stack<std::unique_ptr<int>> s;
  s.Push(std::make_unique<int>(7));
  BT_CHECK_EQ(*s.Top(), 7);
  s.Pop();
  BT_CHECK(s.Empty());
}

BT_TEST(P2_2, string_semantics) {
  Stack<std::string> s;
  std::string owned = "abc";
  s.Push(owned);                       // 拷贝
  BT_CHECK_EQ(owned, std::string("abc"));
  BT_CHECK_EQ(s.Top(), std::string("abc"));
  s.Push(std::string("def"));          // 移动
  BT_CHECK_EQ(s.Top(), std::string("def"));
}

BT_TEST(P2_2, large_stack_1e5) {
  Stack<int> s;
  const int n = 100000;
  for (int i = 0; i < n; ++i) s.Push(i);
  BT_CHECK_EQ(s.Size(), static_cast<size_t>(n));
  BT_CHECK_EQ(s.Top(), n - 1);
  long long sum = 0;
  while (!s.Empty()) { sum += s.Top(); s.Pop(); }
  BT_CHECK_EQ(sum, 1LL * n * (n - 1) / 2);
}

BT_TEST(P2_2, interleaved_ops_keep_size_consistent) {
  Stack<int> s;
  for (int i = 0; i < 100; ++i) {
    s.Push(i);
    s.Push(i * 2);
    s.Pop();
    BT_CHECK_EQ(s.Size(), static_cast<size_t>(i + 1));
  }
  BT_CHECK_EQ(s.Size(), static_cast<size_t>(100));
}

BT_MAIN("P2.2 模板类 Stack")
