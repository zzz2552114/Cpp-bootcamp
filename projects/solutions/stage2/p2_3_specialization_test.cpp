// P2.3 测试点
#include <iostream>
#include <string>

#include "test_util.h"
#include "p2_3_specialization.h"

BT_TEST(P2_3, describe_default_for_int) {
  BT_CHECK_EQ(Describe(1), std::string("default"));
  BT_CHECK_EQ(Describe(short(1)), std::string("default"));
}

BT_TEST(P2_3, describe_double_specialization) {
  BT_CHECK_EQ(Describe(1.0), std::string("double"));
}

BT_TEST(P2_3, describe_float_is_not_double) {
  // float 不会命中 double 的特化 → 走通用版本
  BT_CHECK_EQ(Describe(1.0f), std::string("default"));
}

BT_TEST(P2_3, describe_cstring_specialization) {
  const char *p = "hi";
  BT_CHECK_EQ(Describe(p), std::string("c-string"));
}

BT_TEST(P2_3, describe_string_literal_deduces_array_not_pointer) {
  // 重点：字符串字面量的类型是 const char[N]，T 推导为 char[N]，
  // 因此【不会】命中 const char* 的特化，而是走通用版本。
  BT_CHECK_EQ(Describe("hi"), std::string("default"));
  // 想要命中特化，要么先存成指针（见上一个测试点），要么显式指定模板实参：
  BT_CHECK_EQ(Describe<const char *>("hi"), std::string("c-string"));
}

BT_TEST(P2_3, describe_std_string_is_default_not_cstring) {
  BT_CHECK_EQ(Describe(std::string("hi")), std::string("default"));
}

BT_TEST(P2_3, class_specialization_generic_vs_float) {
  BT_CHECK_EQ(Printer<int>::Name(), std::string("generic"));
  BT_CHECK_EQ(Printer<double>::Name(), std::string("generic"));
  BT_CHECK_EQ(Printer<float>::Name(), std::string("float"));
  BT_CHECK_EQ(Printer<std::string>::Name(), std::string("string"));
}

BT_TEST(P2_3, factorial_is_compile_time) {
  static_assert(Factorial<0>() == 1);
  static_assert(Factorial<1>() == 1);
  static_assert(Factorial<5>() == 120);
  static_assert(Factorial<10>() == 3628800);
  constexpr size_t f12 = Factorial<12>();
  static_assert(f12 == 479001600);
  BT_CHECK_EQ(Factorial<10>(), static_cast<size_t>(3628800));
}

BT_TEST(P2_3, fixed_array_size_is_a_compile_time_constant) {
  FixedArray<5> a;
  static_assert(a.Size() == 5);
  BT_CHECK_EQ(a.Size(), static_cast<size_t>(5));
  for (int i = 0; i < 5; ++i) BT_CHECK_EQ(a.a[i], 0);   // 默认全 0
}

BT_TEST(P2_3, non_type_param_can_be_large) {
  FixedArray<1024> a;
  a.a[1023] = 7;
  BT_CHECK_EQ(a.Size(), static_cast<size_t>(1024));
  BT_CHECK_EQ(a.a[1023], 7);
}

BT_TEST(P2_3, constant_non_type_param_int) {
  static_assert(Constant<150>::value == 150);
  static_assert(Constant<-3>::value == -3);
  BT_CHECK_EQ(Constant<150>::value, 150);
}

BT_TEST(P2_3, constexpr_if_integral_branch) {
  BT_CHECK_EQ(ToString(42), std::string("42"));
  BT_CHECK_EQ(ToString(-7), std::string("-7"));
}

BT_TEST(P2_3, constexpr_if_floating_branch) {
  BT_CHECK_EQ(ToString(1.5), std::string("1.500000"));
}

BT_TEST(P2_3, constexpr_if_non_numeric_branch) {
  BT_CHECK_EQ(ToString(std::string("hi")), std::string("non-numeric"));
  struct S {};
  BT_CHECK_EQ(ToString(S{}), std::string("non-numeric"));
}

BT_TEST(P2_3, constexpr_if_discards_branch_so_it_still_compiles) {
  // 即使 T 没有 std::to_string 支持（这里是 std::string），
  // 被 if constexpr 丢弃的分支也不会被实例化，所以能编译通过。
  BT_CHECK_EQ(ToString(std::string("x")), std::string("non-numeric"));
}

BT_MAIN("P2.3 特化 / 非类型参数 / constexpr if")
