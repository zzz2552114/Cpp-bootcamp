// P2.4 测试点：模板头文件化 / 显式实例化 / CTAD
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "test_util.h"
#include "minimath/min.h"

BT_TEST(P2_4, template_usable_across_tu) {
  BT_CHECK_EQ(minimath::Min(3, 5), 3);
  BT_CHECK_EQ(minimath::Min(3.2, 1.5), 1.5);
  BT_CHECK_EQ(minimath::Min(std::string("abc"), std::string("def")), std::string("abc"));
}

BT_TEST(P2_4, non_template_function_from_cpp) {
  BT_CHECK_EQ(minimath::Add(2, 3), 5);
  BT_CHECK_EQ(minimath::Add(-1, 1), 0);
}

BT_TEST(P2_4, box_ctad_deduces_int) {
  minimath::Box b(42);
  static_assert(std::is_same_v<decltype(b), minimath::Box<int>>);
  BT_CHECK_EQ(b.Get(), 42);
}

BT_TEST(P2_4, box_ctad_deduces_string) {
  minimath::Box b(std::string("hi"));
  static_assert(std::is_same_v<decltype(b), minimath::Box<std::string>>);
  BT_CHECK_EQ(b.Get(), std::string("hi"));
}

BT_TEST(P2_4, vector_ctad) {
  std::vector v{1, 2, 3};
  static_assert(std::is_same_v<decltype(v), std::vector<int>>);
  BT_CHECK_EQ(v.size(), static_cast<size_t>(3));
  BT_CHECK_EQ(v[2], 3);
}

BT_TEST(P2_4, pair_ctad) {
  std::pair p{1, 2.0};
  static_assert(std::is_same_v<decltype(p), std::pair<int, double>>);
  BT_CHECK_EQ(p.first, 1);
}

BT_TEST(P2_4, ctad_needs_a_constructor_to_deduce_from) {
  // Box 只有 explicit Box(T)，所以 Box<int> 可以推导；
  // 若类没有任何"能从实参推导 T"的构造函数，就必须显式写 Box<int>。
  minimath::Box<int> explicit_box(7);
  BT_CHECK_EQ(explicit_box.Get(), 7);
}

BT_TEST(P2_4, template_in_header_instantiated_in_this_tu_too) {
  // 这个 TU 自己也实例化了 Min<std::string>，与 min.cpp 里的 Min<int> 不冲突
  BT_CHECK_EQ(minimath::Min(std::string("z"), std::string("a")), std::string("a"));
  BT_CHECK_EQ(minimath::Min(100, 100), 100);
}

BT_MAIN("P2.4 模板头文件化 / 显式实例化 / CTAD")
