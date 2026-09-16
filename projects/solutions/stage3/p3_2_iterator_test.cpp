// P3.2 测试点：正向/反向遍历、--End()、引用解引用、range-for
#include <iostream>
#include <vector>

#include "test_util.h"
#include "p3_2_iterator.h"

namespace {
// DLL 带手写析构 + 裸指针，所以既不可拷贝也不可移动（细节见 P3.3），
// 这里就用"就地填充"的辅助函数。
// 头插 n..1 → head 到 tail 的顺序是 1 2 ... n
void Fill(DLL &d, int n) {
  for (int i = n; i >= 1; --i) d.InsertAtHead(i);
}
}  // namespace

BT_TEST(P3_2, empty_list_begin_equals_end) {
  DLL d;
  BT_CHECK(d.Empty());
  BT_CHECK(d.Begin() == d.End());
  BT_CHECK(!(d.Begin() != d.End()));
}

BT_TEST(P3_2, single_element) {
  DLL d;
  Fill(d, 1);
  BT_CHECK_EQ(d.Size(), static_cast<size_t>(1));
  BT_CHECK_EQ(d.Front(), 1);
  BT_CHECK_EQ(d.Back(), 1);
  BT_CHECK(*d.Begin() == 1);
}

BT_TEST(P3_2, forward_iteration_order) {
  DLL d;
  Fill(d, 6);
  std::vector<int> got;
  for (DLLIterator it = d.Begin(); it != d.End(); ++it) got.push_back(*it);
  BT_CHECK_EQ(got.size(), static_cast<size_t>(6));
  for (int i = 0; i < 6; ++i) BT_CHECK_EQ(got[i], i + 1);
}

BT_TEST(P3_2, prefix_and_postfix_give_same_sequence) {
  DLL d;
  Fill(d, 5);
  std::vector<int> a, b;
  for (DLLIterator it = d.Begin(); it != d.End(); ++it) a.push_back(*it);
  for (DLLIterator it = d.Begin(); it != d.End(); it++) b.push_back(*it);
  BT_CHECK(a == b);
}

BT_TEST(P3_2, postfix_returns_old_value) {
  DLL d;
  Fill(d, 3);
  auto it = d.Begin();
  auto old = it++;                 // 后缀返回旧迭代器
  BT_CHECK_EQ(*old, 1);
  BT_CHECK_EQ(*it, 2);
}

BT_TEST(P3_2, prefix_increment_returns_reference) {
  DLL d;
  Fill(d, 3);
  auto it = d.Begin();
  auto &ref = ++it;                // 前缀返回引用
  BT_CHECK(&ref == &it);
  BT_CHECK_EQ(*it, 2);
}

BT_TEST(P3_2, operator_star_returns_reference_so_can_modify) {
  DLL d;
  Fill(d, 3);
  auto it = d.Begin();
  *it = 99;
  BT_CHECK_EQ(d.Front(), 99);
  BT_CHECK_EQ(*d.Begin(), 99);
}

BT_TEST(P3_2, const_deref_is_readonly_view) {
  DLL d;
  Fill(d, 3);
  const DLLIterator cit = d.Begin();
  BT_CHECK_EQ(*cit, 1);
}

BT_TEST(P3_2, decrement_from_end_goes_to_tail) {
  // 这是原计划里会段错误的地方
  DLL d;
  Fill(d, 6);
  auto it = d.End();
  --it;
  BT_CHECK_EQ(*it, 6);             // 尾元素
  BT_CHECK(it != d.End());         // 已经不是 past-the-end 了
  BT_CHECK(it.Raw() != nullptr);   // 拿到的是真实节点，而不是 nullptr
}

BT_TEST(P3_2, decrement_from_end_on_single_element) {
  DLL d;
  Fill(d, 1);
  auto it = d.End();
  --it;
  BT_CHECK_EQ(*it, 1);
}

BT_TEST(P3_2, reverse_full_traversal) {
  DLL d;
  Fill(d, 6);
  std::vector<int> got;
  for (auto it = d.End(); it != d.Begin();) {
    --it;                          // 先回退再解引用
    got.push_back(*it);
  }
  BT_CHECK_EQ(got.size(), static_cast<size_t>(6));
  for (int i = 0; i < 6; ++i) BT_CHECK_EQ(got[i], 6 - i);
}

BT_TEST(P3_2, decrement_in_the_middle) {
  DLL d;
  Fill(d, 5);
  auto it = d.Begin();
  ++it; ++it; ++it;                // 指向 4
  BT_CHECK_EQ(*it, 4);
  --it;
  BT_CHECK_EQ(*it, 3);
}

BT_TEST(P3_2, postfix_decrement_returns_old_value) {
  DLL d;
  Fill(d, 5);
  auto it = d.Begin();
  ++it; ++it;                      // 指向 3
  auto old = it--;
  BT_CHECK_EQ(*old, 3);
  BT_CHECK_EQ(*it, 2);
}

BT_TEST(P3_2, range_for_works) {
  DLL d;
  Fill(d, 5);
  std::vector<int> got;
  for (auto v : d) got.push_back(v);
  BT_CHECK_EQ(got.size(), static_cast<size_t>(5));
  for (int i = 0; i < 5; ++i) BT_CHECK_EQ(got[i], i + 1);
}

BT_TEST(P3_2, range_for_can_modify_through_reference) {
  DLL d;
  Fill(d, 5);
  for (auto &v : d) v += 100;
  std::vector<int> got;
  for (auto v : d) got.push_back(v);
  for (int i = 0; i < 5; ++i) BT_CHECK_EQ(got[i], i + 1 + 100);
}

BT_TEST(P3_2, large_list_1e5_forward_and_backward) {
  DLL d;
  const int n = 100000;
  Fill(d, n);
  BT_CHECK_EQ(d.Size(), static_cast<size_t>(n));
  long long fwd = 0;
  for (auto it = d.Begin(); it != d.End(); ++it) fwd += *it;
  BT_CHECK_EQ(fwd, 1LL * n * (n + 1) / 2);

  long long bwd = 0;
  for (auto it = d.End(); it != d.Begin();) { --it; bwd += *it; }
  BT_CHECK_EQ(bwd, fwd);
}

BT_TEST(P3_2, independent_iterators_do_not_interfere) {
  DLL d;
  Fill(d, 4);
  auto a = d.Begin();
  auto b = d.Begin();
  ++a; ++a;
  ++b;
  BT_CHECK_EQ(*a, 3);
  BT_CHECK_EQ(*b, 2);
}

BT_MAIN("P3.2 双向迭代器（--End / range-for / 引用解引用）")
