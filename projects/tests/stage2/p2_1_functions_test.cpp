// P2.1 测试点
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

#include "test_util.h"
#include "p2_1_functions.h"

namespace {
struct Tracked {        // 统计拷贝次数，用来验证"形参是 const& 而不是按值"
  static int copies;
  int v;
  explicit Tracked(int x) : v(x) {}
  Tracked(const Tracked &o) : v(o.v) { ++copies; }
  bool operator<(const Tracked &o) const { return v < o.v; }
  static void Reset() { copies = 0; }
};
int Tracked::copies = 0;
}  // namespace

// 测评侧的类型探测工具：只关心"Min(a,b) 这个调用能不能成立"，与你的实现无关。
// 原理属于 SFINAE，不是本阶段的考点（详见 stage2.md 的说明）。
template <typename A, typename B, typename = void> struct CanDeduceMin : std::false_type {};
template <typename A, typename B>
struct CanDeduceMin<A, B, std::void_t<decltype(Min(std::declval<A>(), std::declval<B>()))>>
    : std::true_type {};

struct Money {          // 自定义类型：只要提供 operator<，模板就能用
  int cents;
  bool operator<(const Money &o) const { return cents < o.cents; }
  bool operator==(const Money &o) const { return cents == o.cents; }
};
inline std::ostream &operator<<(std::ostream &os, const Money &m) { return os << m.cents; }

BT_TEST(P2_1, min_int) {
  BT_CHECK_EQ(Min(3, 5), 3);
  BT_CHECK_EQ(Min(5, 3), 3);
  BT_CHECK_EQ(Min(-1, -9), -9);
}

BT_TEST(P2_1, min_double) {
  BT_CHECK_EQ(Min(3.2, 1.5), 1.5);
  BT_CHECK_EQ(Min(1.5, 3.2), 1.5);
}

BT_TEST(P2_1, min_string) {
  BT_CHECK_EQ(Min(std::string("abc"), std::string("def")), std::string("abc"));
  BT_CHECK_EQ(Min(std::string("zzz"), std::string("aaa")), std::string("aaa"));
}

BT_TEST(P2_1, max_mirror_of_min) {
  BT_CHECK_EQ(Max(3, 5), 5);
  BT_CHECK_EQ(Max(3.2, 1.5), 3.2);
  BT_CHECK_EQ(Max(std::string("abc"), std::string("def")), std::string("def"));
}

BT_TEST(P2_1, min_with_custom_type) {
  BT_CHECK(Min(Money{100}, Money{250}) == Money{100});
}

BT_TEST(P2_1, same_template_different_instantiations) {
  // Min<int> 与 Min<double> 是两个不同的函数
  static_assert(!std::is_same_v<decltype(Min(1, 2)), decltype(Min(1.0, 2.0))>);
  BT_CHECK(true);
}

BT_TEST(P2_1, arg_deduction_requires_consistent_T) {
  static_assert(CanDeduceMin<int, int>::value);
  static_assert(CanDeduceMin<double, double>::value);
  static_assert(!CanDeduceMin<int, double>::value);   // Min(3, 3.2) 推导失败
  BT_CHECK(true);
}

BT_TEST(P2_1, explicit_template_arg_resolves_mixed_types) {
  BT_CHECK_EQ(Min<double>(3, 3.2), 3.0);
  BT_CHECK_EQ(Max<double>(3, 3.2), 3.2);
}

BT_TEST(P2_1, constexpr_is_usable_in_static_assert) {
  static_assert(MinC(3, 5) == 3);
  static_assert(MinC(3.2, 1.5) == 1.5);
  constexpr int k = MinC(10, 20);
  static_assert(k == 10);
  BT_CHECK(true);
}

BT_TEST(P2_1, array_len_non_type_param) {
  int a[7] = {};
  double b[13] = {};
  char c[1] = {};
  static_assert(ArrayLen(a) == 7);
  static_assert(ArrayLen(b) == 13);
  static_assert(ArrayLen(c) == 1);
  BT_CHECK_EQ(ArrayLen(a), static_cast<size_t>(7));
}

BT_TEST(P2_1, params_are_const_ref_not_by_value) {
  Tracked::Reset();
  Tracked a(1), b(2);
  Tracked m = Min(a, b);
  // 形参 const T& → 传参 0 次拷贝；返回类型是 T（按值）→ 1 次拷贝。
  // 若形参是按值传递，这里会是 3 次（两个实参 + 一次返回）。
  BT_CHECK_EQ(Tracked::copies, 1);
  BT_CHECK_EQ(m.v, 1);
  BT_CHECK_EQ(a.v, 1);                 // 源对象未被改动
  BT_CHECK_EQ(b.v, 2);
}

BT_MAIN("Test Suite p2_1_functions_test.cpp")
