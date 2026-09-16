// P3.3 测试点
//
// 想看真正的 double free 崩溃（可选，会以非 0 退出）：
//   ./p3_3_rule_of_three_five_zero_test --demo-double-free
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

#include "test_util.h"
#include "p3_3_rule_of_three_five_zero.h"

using namespace r3;

namespace {
void DemoDoubleFree() {
  std::cout << "[demo] 浅拷贝两个 NaiveList（共享同一批节点），准备析构...\n" << std::flush;
  NaiveList a;
  a.Push(1);
  a.Push(2);
  NaiveList b = a;          // 浅拷贝 → b.head == a.head
  std::cout << "[demo] a.head=" << static_cast<const void *>(a.head)
            << "  b.head=" << static_cast<const void *>(b.head) << "\n" << std::flush;
  // 两个对象析构时各 delete 一遍同一批节点 → double free
}
}  // namespace

BT_TEST(P3_3, naive_shallow_copy_shares_the_same_nodes) {
  ResetCounters();
  NaiveList a;
  a.Push(1);
  a.Push(2);
  NaiveList b = a;                    // 编译器生成的浅拷贝
  BT_CHECK(a.head == b.head);         // 两个"所有者"其实是同一块内存
  BT_CHECK_EQ(b.head->v, 2);
  // 为了不真的 double free：手工把 a 的指针摘掉，只让 b 去释放
  a.head = nullptr;
  BT_CHECK_EQ(g_allocs, 2);
  BT_CHECK_EQ(g_frees, 0);
  // b 离开作用域后释放 2 个节点
}

BT_TEST(P3_3, naive_shallow_copy_would_double_free_if_not_rescued) {
  // 这是对上面那个测试点的补充说明：如果不做抢救，析构会重复释放。
  // 我们用一个"计数式"的假想场景来证明危险存在，而不真的触发 UB。
  ResetCounters();
  {
    NaiveList a;
    a.Push(1);
    {
      NaiveList b = a;
      a.head = nullptr;               // 抢救
      (void)b;
    }
  }
  BT_CHECK_EQ(g_allocs, g_frees);     // 抢救之后刚好平衡
}

BT_TEST(P3_3, rule_of_3_deep_copy_is_independent) {
  ResetCounters();
  List3 a;
  a.Push(1);
  a.Push(2);
  List3 b = a;                        // 深拷贝
  BT_CHECK(b.Head() != a.Head());     // 完全不同的内存
  BT_CHECK_EQ(b.Head()->v, 2);
  BT_CHECK_EQ(g_allocs, 4);           // 4 个节点：a 两个 + b 两个
}

BT_TEST(P3_3, rule_of_3_copy_assign_is_independent) {
  ResetCounters();
  List3 a;
  a.Push(7);
  List3 c;
  c.Push(1);
  c.Push(2);
  c = a;                              // 拷贝赋值：先清空再深拷贝
  BT_CHECK(c.Head() != a.Head());
  BT_CHECK_EQ(c.Head()->v, 7);
  BT_CHECK_EQ(c.Head()->next, nullptr);
}

BT_TEST(P3_3, rule_of_3_both_destruct_without_double_free) {
  ResetCounters();
  {
    List3 a;
    a.Push(1);
    a.Push(2);
    List3 b = a;
    List3 c;
    c = a;
    BT_CHECK_EQ(g_allocs - g_frees, 6);   // 6 个节点都还活着
  }
  BT_CHECK_EQ(g_allocs, g_frees);         // 全部释放，无重复
}

BT_TEST(P3_3, rule_of_5_move_transfers_and_empties_source) {
  ResetCounters();
  List5 a;
  a.Push(1);
  a.Push(2);
  List5 b = std::move(a);                // 移动：只偷指针，不新建节点
  BT_CHECK(a.Head() == nullptr);
  BT_CHECK(b.Head() != nullptr);
  BT_CHECK_EQ(g_allocs, 2);              // 没有新分配
  BT_CHECK_EQ(b.Head()->v, 2);

  List5 c;
  c.Push(9);
  c = std::move(b);                      // 移动赋值：c 旧节点被释放
  BT_CHECK(b.Head() == nullptr);
  BT_CHECK_EQ(c.Head()->v, 2);
}

BT_TEST(P3_3, rule_of_5_self_move_is_safe) {
  ResetCounters();
  List5 a;
  a.Push(1);
  a.Push(2);
  List5 &self = a;      // 别名绕开 -Wself-move；我们就是要测自赋值
  a = std::move(self);
  BT_CHECK(a.Head() != nullptr);
  BT_CHECK_EQ(a.Head()->v, 2);
}

BT_TEST(P3_3, rule_of_5_no_leak) {
  ResetCounters();
  {
    List5 a;
    for (int i = 0; i < 100; ++i) a.Push(i);
    List5 b = std::move(a);
    List5 c;
    c = std::move(b);
    BT_CHECK_EQ(c.Head()->v, 99);
  }
  BT_CHECK_EQ(g_allocs, g_frees);
}

BT_TEST(P3_3, rule_of_5_move_ops_are_noexcept) {
  static_assert(std::is_nothrow_move_constructible_v<List5>);
  static_assert(std::is_nothrow_move_assignable_v<List5>);
  static_assert(std::is_copy_constructible_v<List5>);
  BT_CHECK(true);
}

BT_TEST(P3_3, rule_of_0_is_automatically_move_only) {
  static_assert(!std::is_copy_constructible_v<List0>);
  static_assert(!std::is_copy_assignable_v<List0>);
  static_assert(std::is_move_constructible_v<List0>);
  static_assert(std::is_move_assignable_v<List0>);
  BT_CHECK(true);
}

BT_TEST(P3_3, rule_of_0_works_and_needs_no_special_members) {
  ResetCounters();
  {
    List0 a;
    for (int i = 0; i < 50; ++i) a.Push(i);
    BT_CHECK_EQ(a.Head(), 49);
    BT_CHECK_EQ(a.Size(), static_cast<size_t>(50));

    List0 b = std::move(a);              // 移动自动可用
    BT_CHECK(a.Empty());
    BT_CHECK_EQ(b.Head(), 49);
  }
  BT_CHECK_EQ(g_allocs, g_frees);        // 级联释放，无泄漏
}

BT_TEST(P3_3, rule_of_0_destruction_is_cascading) {
  ResetCounters();
  {
    List0 a;
    for (int i = 0; i < 1000; ++i) a.Push(i);
    BT_CHECK_EQ(a.Size(), static_cast<size_t>(1000));
    BT_CHECK_EQ(g_allocs - g_frees, 1000);
  }
  BT_CHECK_EQ(g_allocs, g_frees);        // unique_ptr 链式析构把 1000 个节点全清掉
}

int main(int argc, char **argv) {
  if (argc > 1 && std::string(argv[1]) == "--demo-double-free") {
    DemoDoubleFree();
    return 0;
  }
  return bt::RunAll("P3.3 Rule of 0/3/5");
}
