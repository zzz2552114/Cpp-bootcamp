// P5.1 测试点（含与 std::multiset 的随机对拍）
#include <iostream>
#include <random>
#include <set>
#include <vector>

#include "test_util.h"
#include "p5_1_binary_tree.h"

using namespace tree5;

BT_TEST(P5_1, empty_tree) {
  BinaryTree t;
  BT_CHECK(t.Empty());
  BT_CHECK_EQ(t.Size(), 0);
  BT_CHECK_EQ(t.Height(), 0);
  BT_CHECK(!t.Contains(1));
  BT_CHECK(t.InOrder().empty());
}

BT_TEST(P5_1, single_node) {
  BinaryTree t;
  t.Insert(5);
  BT_CHECK(!t.Empty());
  BT_CHECK_EQ(t.Size(), 1);
  BT_CHECK_EQ(t.Height(), 1);
  BT_CHECK(t.Contains(5));
  BT_CHECK(!t.Contains(4));
}

BT_TEST(P5_1, spec_example) {
  BinaryTree t;
  for (int x : {5, 3, 7, 1, 4, 6, 8}) t.Insert(x);
  BT_CHECK(t.Contains(4));
  BT_CHECK(!t.Contains(9));
  BT_CHECK_EQ(t.Height(), 3);
  BT_CHECK_EQ(t.Size(), 7);
  std::vector<int> expect{1, 3, 4, 5, 6, 7, 8};
  BT_CHECK(t.InOrder() == expect);
}

BT_TEST(P5_1, inorder_is_sorted_for_any_insert_order) {
  BinaryTree t;
  for (int x : {42, 7, 99, -1, 0, 13, 7, 42, 55, -20}) t.Insert(x);
  auto in = t.InOrder();
  for (size_t i = 1; i < in.size(); ++i) BT_CHECK(in[i - 1] <= in[i]);
}

BT_TEST(P5_1, duplicates_go_to_right_subtree) {
  BinaryTree t;
  t.Insert(5);
  t.Insert(5);
  t.Insert(5);
  BT_CHECK_EQ(t.Size(), 3);
  BT_CHECK(t.Contains(5));
  BT_CHECK_EQ(t.Height(), 3);            // 退化成一条右链
  std::vector<int> expect{5, 5, 5};
  BT_CHECK(t.InOrder() == expect);
}

BT_TEST(P5_1, sorted_insert_is_a_degenerate_chain) {
  BinaryTree t;
  const int n = 500;                     // 故意小一点，避免递归过深
  for (int i = 1; i <= n; ++i) t.Insert(i);
  BT_CHECK_EQ(t.Size(), n);
  BT_CHECK_EQ(t.Height(), n);            // 升序插入 → 高度 = 元素个数
}

BT_TEST(P5_1, perfectly_balanced_height) {
  // 插入顺序使树完全平衡：中位数先插
  auto insert_balanced = [](BinaryTree &t, int lo, int hi, auto &self) -> void {
    if (lo > hi) return;
    const int mid = lo + (hi - lo) / 2;
    t.Insert(mid);
    self(t, lo, mid - 1, self);
    self(t, mid + 1, hi, self);
  };
  BinaryTree t;
  insert_balanced(t, 1, 7, insert_balanced);
  BT_CHECK_EQ(t.Size(), 7);
  BT_CHECK_EQ(t.Height(), 3);            // 7 个节点完全平衡 = 3 层
}

BT_TEST(P5_1, contains_is_const_and_does_not_modify) {
  BinaryTree t;
  for (int x : {5, 3, 7}) t.Insert(x);
  const BinaryTree &ct = t;
  BT_CHECK(ct.Contains(3));
  BT_CHECK(!ct.Contains(4));
  BT_CHECK_EQ(ct.Size(), 3);
  BT_CHECK_EQ(ct.InOrder().size(), static_cast<size_t>(3));
}

BT_TEST(P5_1, negatives_and_zero) {
  BinaryTree t;
  for (int x : {0, -5, 5, -10, -1}) t.Insert(x);
  std::vector<int> expect{-10, -5, -1, 0, 5};
  BT_CHECK(t.InOrder() == expect);
  BT_CHECK(t.Contains(-10));
  BT_CHECK(!t.Contains(-11));
}

BT_TEST(P5_1, move_is_allowed_but_copy_is_not) {
  static_assert(!std::is_copy_constructible_v<BinaryTree>);
  static_assert(std::is_move_constructible_v<BinaryTree>);
  BinaryTree a;
  a.Insert(1);
  a.Insert(2);
  BinaryTree b = std::move(a);
  BT_CHECK_EQ(b.Size(), 2);
  BT_CHECK(a.Empty());                    // 移动后源为空
  BT_CHECK_EQ(b.Height(), 2);
}

BT_TEST(P5_1, no_manual_delete_needed_1e5_nodes) {
  BinaryTree t;
  const int n = 100000;
  std::mt19937 rng(1);
  for (int i = 0; i < n; ++i) t.Insert(static_cast<int>(rng() % 1000000));
  BT_CHECK_EQ(t.Size(), n);
  // 析构时整棵树自动级联释放，不需要任何手写 delete
}

BT_TEST(P5_1, stress_differential_vs_multiset) {
  std::mt19937 rng(20240645);
  for (int iter = 0; iter < 60; ++iter) {
    BinaryTree t;
    std::multiset<int> ref;
    const int n = 1 + static_cast<int>(rng() % 200);
    for (int i = 0; i < n; ++i) {
      const int x = static_cast<int>(rng() % 500) - 250;
      t.Insert(x);
      ref.insert(x);
    }
    // Size 与 multiset 一致
    BT_CHECK_EQ(t.Size(), static_cast<int>(ref.size()));
    // InOrder 恰好等于 multiset 的有序序列（含重复）→ 非常强的等价性检查
    std::vector<int> mine = t.InOrder();
    std::vector<int> theirs(ref.begin(), ref.end());
    BT_CHECK(mine == theirs);
    // Contains 与 count>0 一致
    for (int k = 0; k < 40; ++k) {
      const int q = static_cast<int>(rng() % 600) - 300;
      BT_CHECK_EQ(t.Contains(q), ref.count(q) > 0);
    }
    // 高度必须落在合理范围内
    if (!ref.empty()) {
      BT_CHECK(t.Height() >= 1);
      BT_CHECK(t.Height() <= static_cast<int>(ref.size()));
    }
  }
}

BT_TEST(P5_1, stress_reinsert_same_values_keeps_size) {
  BinaryTree t;
  for (int i = 0; i < 1000; ++i) t.Insert(7);
  BT_CHECK_EQ(t.Size(), 1000);
  BT_CHECK_EQ(t.Height(), 1000);
}

BT_MAIN("Test Suite p5_1_binary_tree_test.cpp")
