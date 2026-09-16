// P3.4 测试点：命名空间组织 / .h-.cpp 分工 / 匿名命名空间 / 避免 using namespace
#include <iostream>
#include <vector>

#include "test_util.h"
#include "mylib/geometry.h"
#include "mylib/geometry.h"   // 故意重复 include：靠 #pragma once 保证安全
#include "mylib/stats.h"

BT_TEST(P3_4, add_in_namespace) {
  BT_CHECK_EQ(mylib::Add(2, 3), 5);
  BT_CHECK_EQ(mylib::Add(-2, 2), 0);
}

BT_TEST(P3_4, same_name_in_two_namespaces_do_not_conflict) {
  BT_CHECK_EQ(mylib::Add(2, 3), 5);
  BT_CHECK_EQ(other::Add(2, 3), 12);     // 5 + kMagic(7)
  BT_CHECK_NE(mylib::Add(2, 3), other::Add(2, 3));
}

BT_TEST(P3_4, anonymous_namespace_helper_is_used_but_not_exported) {
  BT_CHECK_EQ(mylib::Sub(2, 5), 3);      // HelperAbs(2-5) = 3
  BT_CHECK_EQ(mylib::Sub(5, 2), 3);
  BT_CHECK_EQ(mylib::Abs(-9), 9);
  BT_CHECK_EQ(mylib::Abs(9), 9);
}

BT_TEST(P3_4, average_basic) {
  BT_CHECK_EQ(mylib::Average({1, 2, 3}), 2.0);
}

BT_TEST(P3_4, average_empty_is_zero) {
  BT_CHECK_EQ(mylib::Average({}), 0.0);
}

BT_TEST(P3_4, average_is_floating_point) {
  BT_CHECK(mylib::Average({1, 2}) == 1.5);   // 不是整数截断的 1.0
}

BT_TEST(P3_4, average_negative) {
  BT_CHECK(mylib::Average({-1, -2, -3}) == -2.0);
}

BT_TEST(P3_4, sum_does_not_overflow_64bit) {
  std::vector<int> v(1000000, 1000000000);   // 1e6 * 1e9 = 1e15，远小于 2^63
  BT_CHECK_EQ(mylib::Sum(v), 1000000000000000LL);
  BT_CHECK(mylib::Average(v) == 1000000000.0);
}

BT_TEST(P3_4, header_can_be_included_multiple_times) {
  // 上面重复 include 了 geometry.h；能编译通过就说明 include guard 生效。
  BT_CHECK_EQ(mylib::Add(1, 1), 2);
}

BT_TEST(P3_4, fully_qualified_names_used_no_global_using) {
  // 本文件里没有 using namespace mylib; 全程用 mylib:: 前缀 —— 与 BusTub 风格一致
  BT_CHECK_EQ(mylib::Add(mylib::Abs(-1), mylib::Abs(-2)), 3);
}

BT_MAIN("P3.4 命名空间 mini 库（.h/.cpp + 匿名命名空间）")
