// P4.4 测试点
#include <iostream>
#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "test_util.h"
#include "p4_4_auto_decltype.h"

using namespace a4;

BT_TEST(P4_4, auto_deduces_int_and_double) {
  auto a = 1;
  auto b = 3.2;
  static_assert(std::is_same_v<decltype(a), int>);
  static_assert(std::is_same_v<decltype(b), double>);   // 注意：3.2 是 double，不是 float
  BT_CHECK_EQ(a, 1);
}

BT_TEST(P4_4, auto_deduces_string_from_literal) {
  auto c = std::string("Hello");
  static_assert(std::is_same_v<decltype(c), std::string>);
  BT_CHECK_EQ(c.size(), static_cast<size_t>(5));
}

BT_TEST(P4_4, auto_strips_top_level_const_and_reference) {
  const std::string s = "hi";
  auto a = s;                                            // 顶层 const 被剥掉
  static_assert(std::is_same_v<decltype(a), std::string>);
  BT_CHECK_EQ(a, std::string("hi"));
}

BT_TEST(P4_4, auto_ref_keeps_const) {
  const std::string s = "hi";
  auto &b = s;                                           // 引用到 const
  static_assert(std::is_same_v<decltype(b), const std::string &>);
  const auto &c = s;
  static_assert(std::is_same_v<decltype(c), const std::string &>);
  BT_CHECK_EQ(b, c);
}

BT_TEST(P4_4, auto_ampamp_forwarding_reference) {
  const std::string s = "hi";
  auto &&d = std::move(s);
  static_assert(std::is_same_v<decltype(d), const std::string &&>);
  BT_CHECK_EQ(d, std::string("hi"));
}

BT_TEST(P4_4, auto_copies_but_const_auto_ref_does_not) {
  Big big(7);
  Big::Reset();
  { auto x = big; (void)x; }                             // auto → 拷贝
  BT_CHECK_EQ(Big::copies, 1);

  Big::Reset();
  {
    const auto &y = big;                                 // const auto& → 不拷贝
    BT_CHECK_EQ(Big::copies, 0);
    BT_CHECK_EQ(y.v, 7);
  }
}

BT_TEST(P4_4, range_for_auto_copies_each_element) {
  std::vector<Big> v;
  v.reserve(3);
  v.emplace_back(1);
  v.emplace_back(2);
  v.emplace_back(3);
  Big::Reset();
  for (auto x : v) (void)x;                              // auto → 每个元素拷贝一次
  BT_CHECK_EQ(Big::copies, 3);

  Big::Reset();
  for (const auto &x : v) (void)x;                        // const auto& → 零拷贝
  BT_CHECK_EQ(Big::copies, 0);
}

BT_TEST(P4_4, structured_binding_modifies_map_values) {
  std::map<std::string, int> m{{"a", 1}, {"b", 2}};
  BT_CHECK_EQ(ScaleValues(m, 10), std::string("a10b20"));
  BT_CHECK_EQ(m.at("a"), 10);
}

BT_TEST(P4_4, structured_binding_key_is_const) {
  std::map<std::string, int> m{{"a", 1}};
  for (auto &[k, v] : m) {
    static_assert(std::is_same_v<decltype(k), const std::string>);   // key 带 const
    static_assert(std::is_same_v<decltype(v), int>);
    BT_CHECK(!std::is_const_v<std::remove_reference_t<decltype(v)>>);
  }
}

BT_TEST(P4_4, structured_binding_by_value_copies) {
  Big big(1);
  const std::pair<int, Big> p{1, big};
  Big::Reset();
  auto [a, b] = p;                                       // 按值绑定 → 拷贝
  (void)a; (void)b;
  BT_CHECK(Big::copies >= 1);
}

BT_TEST(P4_4, structured_binding_on_vector_of_pairs) {
  std::vector<std::pair<int, int>> v{{1, 2}, {3, 4}, {5, 6}};
  BT_CHECK_EQ(SumPairs(v), 21);
}

BT_TEST(P4_4, decltype_of_vector_subscript_is_reference) {
  std::vector<int> vec{1, 2, 3};
  static_assert(std::is_same_v<decltype(vec[0]), int &>);
  decltype(vec[0]) ref = vec[0];                          // 是 int& → 可修改
  ref = 99;
  BT_CHECK_EQ(vec[0], 99);
}

BT_TEST(P4_4, decltype_auto_preserves_reference_but_auto_loses_it) {
  std::vector<int> vec{1, 2, 3};
  AtRef(vec, 1) = 77;                                     // decltype(auto) → int&
  BT_CHECK_EQ(vec[1], 77);

  int copy = AtVal(vec, 2);                               // auto → int，是一个副本
  copy = 55;                                              // 改的是副本
  BT_CHECK_EQ(vec[2], 3);                                 // 原 vector 没变
}

BT_TEST(P4_4, decltype_on_id_expression_vs_expression) {
  int x = 5;
  int &rx = x;
  static_assert(std::is_same_v<decltype(x), int>);         // 变量名 → 声明类型
  static_assert(std::is_same_v<decltype(rx), int &>);
  static_assert(std::is_same_v<decltype((x)), int &>);     // 加括号 → 变成表达式 → 左值引用
  BT_CHECK_EQ(x, 5);
}

BT_TEST(P4_4, auto_for_iterators) {
  std::map<std::string, int> m{{"b", 2}, {"a", 1}};
  std::string keys;
  for (auto it = m.begin(); it != m.end(); ++it) keys += it->first;
  BT_CHECK_EQ(keys, std::string("ab"));
}

BT_TEST(P4_4, auto_for_long_template_types) {
  std::vector<std::map<std::string, std::vector<int>>> nested;
  nested.resize(2);
  nested[0]["k"].push_back(1);
  auto copy = nested;                                      // 长类型交给 auto
  BT_CHECK_EQ(copy.size(), static_cast<size_t>(2));
  BT_CHECK_EQ(copy[0].at("k").size(), static_cast<size_t>(1));
}

BT_TEST(P4_4, structured_binding_large_map_1e5) {
  std::map<std::string, int> m;
  const int n = 100000;
  for (int i = 0; i < n; ++i) m["k" + std::to_string(i)] = i;
  long long sum = 0;
  for (const auto &[k, v] : m) { (void)k; sum += v; }
  BT_CHECK_EQ(sum, 1LL * n * (n - 1) / 2);
}

BT_MAIN("Test Suite p4_4_auto_decltype_test.cpp")
