// P1.1 测试点：由易到难
#include <iostream>
#include <type_traits>

#include "test_util.h"
#include "p1_1_statistics.h"

BT_TEST(P1_1, empty_sum_is_zero) {
  Statistics s;
  BT_CHECK_EQ(s.Sum(), 0);
}

BT_TEST(P1_1, empty_average_is_zero_not_nan) {
  Statistics s;
  BT_CHECK_EQ(s.Average(), 0.0);
}

BT_TEST(P1_1, single_value) {
  Statistics s;
  s.AddValue(7);
  BT_CHECK_EQ(s.Sum(), 7);
  BT_CHECK_EQ(s.Average(), 7.0);
}

BT_TEST(P1_1, basic_1_to_5) {
  Statistics s;
  for (int x : {1, 2, 3, 4, 5}) s.AddValue(x);
  BT_CHECK_EQ(s.Sum(), 15);
  BT_CHECK_EQ(s.Average(), 3.0);
  BT_CHECK_EQ(s.Size(), static_cast<size_t>(5));
}

BT_TEST(P1_1, add_value_mutates_original) {
  Statistics s;
  s.AddValue(1);
  s.AddValue(2);
  BT_CHECK_EQ(s.Size(), static_cast<size_t>(2));   // 真的是原地修改
}

BT_TEST(P1_1, average_is_floating_point_not_truncated) {
  Statistics s;
  s.AddValue(1);
  s.AddValue(2);
  BT_CHECK_EQ(s.Sum(), 3);
  BT_CHECK(s.Average() == 1.5);   // 整数除法会得到 1.0，这里必须是 1.5
}

BT_TEST(P1_1, negative_values) {
  Statistics s;
  for (int x : {-5, -1, -4}) s.AddValue(x);
  BT_CHECK_EQ(s.Sum(), -10);
  BT_CHECK(s.Average() < 0);
}

BT_TEST(P1_1, const_ref_does_not_copy_but_value_does) {
  Statistics s;
  s.AddValue(1);
  Statistics::copies = 0;

  (void)ReportSum(s);                     // const& 借用
  BT_CHECK_EQ(Statistics::copies, 0);     // 一次拷贝都没有

  auto by_value = [](Statistics t) { return t.Sum(); };
  (void)by_value(s);                      // 按值传参
  BT_CHECK(Statistics::copies >= 1);      // 至少拷贝一次
}

BT_TEST(P1_1, const_object_can_call_const_members) {
  const Statistics s = [] {
    Statistics t;
    t.AddValue(3);
    return t;
  }();
  BT_CHECK_EQ(s.Sum(), 3);
  BT_CHECK_EQ(s.Average(), 3.0);
  BT_CHECK_EQ(s.Size(), static_cast<size_t>(1));
  static_assert(std::is_same_v<decltype(s.Sum()), int>);
}

BT_TEST(P1_1, large_data_1e6_uses_64bit_sum) {
  Statistics s;
  s.data.reserve(1000000);
  for (int i = 1; i <= 1000000; ++i) s.data.push_back(i);
  const long long expect = 1000000LL * 1000001LL / 2;   // 500000500000，远超 INT_MAX(2147483647)
  BT_CHECK_EQ(s.SumL(), expect);                        // 64 位求和正确
  BT_CHECK(s.Average() == 500000.5);
  BT_CHECK_EQ(s.Size(), static_cast<size_t>(1000000));
}

BT_TEST(P1_1, sum_int_would_overflow_here_so_use_suml) {
  // 这个测试点专门说明"返回 int 的 Sum() 在大数据下会溢出"。
  // 有符号溢出是 UB，所以我们【不真的去触发它】，而是用类型层面证明风险存在。
  static_assert(sizeof(int) < sizeof(long long), "int is narrower than long long");
  Statistics s;
  s.data.assign(1000000, 1);            // 一百万个 1：Sum() 也安全（=1e6）
  BT_CHECK_EQ(s.Sum(), 1000000);
  BT_CHECK_EQ(s.SumL(), 1000000LL);
  // 若把每个元素换成 i(1..1e6)，SumL()=500000500000 仍正确，而 Sum() 会溢出。
}

BT_TEST(P1_1, move_does_not_count_as_copy) {
  Statistics a;
  a.AddValue(1);
  Statistics::copies = 0;
  Statistics b(std::move(a));   // 移动：不该增加 copies
  BT_CHECK_EQ(Statistics::copies, 0);
  BT_CHECK_EQ(b.Sum(), 1);
}

BT_MAIN("Test Suite p1_1_statistics_test.cpp")
