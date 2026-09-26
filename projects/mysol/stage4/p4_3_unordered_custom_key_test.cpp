// P4.3 测试点
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "test_util.h"
#include "p4_3_unordered_custom_key.h"

using namespace u4;

BT_TEST(P4_3, operator_bracket_INSERTs_a_default_value) {
  std::unordered_map<std::string, int> m;
  BT_CHECK_EQ(m.size(), static_cast<size_t>(0));
  int x = m["nope"];                    // ⚠️ 不存在 → 默认构造 int(0) 并插入
  BT_CHECK_EQ(x, 0);
  BT_CHECK_EQ(m.size(), static_cast<size_t>(1));
  BT_CHECK_EQ(m.count("nope"), static_cast<size_t>(1));
}

BT_TEST(P4_3, find_does_not_insert) {
  std::unordered_map<std::string, int> m;
  BT_CHECK(m.find("nope") == m.end());
  BT_CHECK_EQ(m.size(), static_cast<size_t>(0));      // 什么都没变
}

BT_TEST(P4_3, at_throws_instead_of_inserting) {
  std::unordered_map<std::string, int> m;
  BT_CHECK_THROWS(m.at("nope"), std::out_of_range);
  BT_CHECK_EQ(m.size(), static_cast<size_t>(0));
}

BT_TEST(P4_3, at_on_existing_key) {
  auto m = MakeBasicMap();
  BT_CHECK_EQ(m.at("foo"), 2);
  BT_CHECK_EQ(m.at("jignesh"), 445);
}

BT_TEST(P4_3, insert_variants_and_duplicate_key_keeps_first) {
  auto m = MakeBasicMap();
  BT_CHECK_EQ(m.size(), static_cast<size_t>(4));
  auto r = m.insert({"foo", 999});
  BT_CHECK(r.second == false);                         // 已存在，不覆盖
  BT_CHECK_EQ(m.at("foo"), 2);
}

BT_TEST(P4_3, erase_by_key_and_iterator) {
  auto m = MakeBasicMap();
  BT_CHECK_EQ(m.erase("eggs"), static_cast<size_t>(1));
  BT_CHECK_EQ(m.erase("eggs"), static_cast<size_t>(0));
  BT_CHECK_EQ(m.count("eggs"), static_cast<size_t>(0));
  m.erase(m.find("spam"));
  BT_CHECK_EQ(m.count("spam"), static_cast<size_t>(0));
  BT_CHECK_EQ(m.size(), static_cast<size_t>(2));
}

BT_TEST(P4_3, try_emplace_does_not_overwrite) {
  std::unordered_map<std::string, std::string> cfg{{"k", "v1"}};
  auto r = cfg.try_emplace("k", "v2");
  BT_CHECK(!r.second);
  BT_CHECK_EQ(cfg.at("k"), std::string("v1"));
  cfg.try_emplace("k2", "v3");
  BT_CHECK_EQ(cfg.at("k2"), std::string("v3"));
}

BT_TEST(P4_3, insert_or_assign_overwrites) {
  std::unordered_map<std::string, std::string> cfg{{"k", "v1"}};
  cfg.insert_or_assign("k", "v2");
  BT_CHECK_EQ(cfg.at("k"), std::string("v2"));
}

BT_TEST(P4_3, pair_key_needs_custom_hash) {
  PairMap grid;
  grid.insert({{1, 2}, "a"});
  grid[{3, 4}] = "b";
  BT_CHECK_EQ(grid.size(), static_cast<size_t>(2));
  BT_CHECK_EQ(grid.at({1, 2}), std::string("a"));
  BT_CHECK_EQ(grid.at({3, 4}), std::string("b"));
  BT_CHECK(grid.find({9, 9}) == grid.end());
}

BT_TEST(P4_3, pair_key_negative_components) {
  PairMap grid;
  grid[{-1, -2}] = "neg";
  grid[{1, 2}] = "pos";
  BT_CHECK_EQ(grid.size(), static_cast<size_t>(2));
  BT_CHECK_EQ(grid.at({-1, -2}), std::string("neg"));
  BT_CHECK_EQ(grid.at({1, 2}), std::string("pos"));
}

BT_TEST(P4_3, custom_struct_key_with_hash_specialization) {
  std::unordered_map<Point, int> pm;
  pm[Point{1, 2}] = 10;
  pm[Point{1, 2}] = 11;                          // 同一个 key → 覆盖
  BT_CHECK_EQ(pm.size(), static_cast<size_t>(1));
  BT_CHECK_EQ(pm.at(Point{1, 2}), 11);
  BT_CHECK(pm.find(Point{2, 1}) == pm.end());     // 不同 key
}

BT_TEST(P4_3, custom_struct_in_unordered_set) {
  std::unordered_set<Point> ps;
  ps.insert(Point{1, 1});
  ps.insert(Point{1, 1});
  ps.insert(Point{2, 2});
  BT_CHECK_EQ(ps.size(), static_cast<size_t>(2));
  BT_CHECK_EQ(ps.count(Point{1, 1}), static_cast<size_t>(1));
}

BT_TEST(P4_3, many_colliding_keys_still_all_findable) {
  // 故意用会碰撞的 hash（取模很小）来验证：冲突只是变慢，不影响正确性
  struct ModHash {
    size_t operator()(int x) const { return static_cast<size_t>(x % 4); }
  };
  std::unordered_map<int, int, ModHash> m;
  for (int i = 0; i < 1000; ++i) m[i] = i * 3;
  BT_CHECK_EQ(m.size(), static_cast<size_t>(1000));
  for (int i = 0; i < 1000; ++i) BT_CHECK_EQ(m.at(i), i * 3);
}

BT_TEST(P4_3, large_map_1e5_roundtrip) {
  std::unordered_map<int, int> m;
  const int n = 100000;
  for (int i = 0; i < n; ++i) m[i] = i * 2;
  BT_CHECK_EQ(m.size(), static_cast<size_t>(n));
  long long sum = 0;
  for (int i = 0; i < n; ++i) sum += m.at(i);
  BT_CHECK_EQ(sum, 1LL * n * (n - 1));
}

BT_TEST(P4_3, iterating_visits_every_key_exactly_once) {
  auto m = MakeBasicMap();
  std::unordered_set<std::string> seen;
  for (const auto &kv : m) {
    BT_CHECK(seen.insert(kv.first).second);       // 每个 key 只出现一次
  }
  BT_CHECK_EQ(seen.size(), m.size());
}

BT_TEST(P4_3, operator_bracket_default_requires_default_constructible) {
  // 这里顺带说明：m[key] 要求 V 可默认构造。
  // std::string 可默认构造 → 可以；若 V 是"无默认构造"的类型就必须用 find/at/emplace。
  std::unordered_map<std::string, std::string> m;
  m["k"] += "abc";                              // 默认构造 "" 再 +=
  BT_CHECK_EQ(m.at("k"), std::string("abc"));
}

BT_MAIN("Test Suite p4_3_unordered_custom_key_test.cpp")
