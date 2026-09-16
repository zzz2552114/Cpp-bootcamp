// P4.2 测试点
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "test_util.h"
#include "p4_2_set_comparator.h"

using namespace s4;

BT_TEST(P4_2, set_dedups_and_sorts) {
  BT_CHECK_EQ(JoinAscending(SortedUnique({5, 1, 5, 3, 1})), std::string("1 3 5"));
}

BT_TEST(P4_2, set_empty) {
  BT_CHECK_EQ(JoinAscending(SortedUnique({})), std::string(""));
  BT_CHECK(SortedUnique({}).empty());
}

BT_TEST(P4_2, set_single) {
  BT_CHECK_EQ(JoinAscending(SortedUnique({9})), std::string("9"));
}

BT_TEST(P4_2, set_negatives_sorted) {
  BT_CHECK_EQ(JoinAscending(SortedUnique({3, -5, 0, -1, 2})), std::string("-5 -1 0 2 3"));
}

BT_TEST(P4_2, insert_reports_whether_it_was_new) {
  std::set<int> s{1, 2, 3};
  BT_CHECK(s.insert(3).second == false);      // 已存在
  BT_CHECK(s.insert(4).second == true);       // 新插入
  BT_CHECK_EQ(s.size(), static_cast<size_t>(4));
}

BT_TEST(P4_2, find_and_count) {
  std::set<int> s{1, 2, 3};
  BT_CHECK(s.find(2) != s.end());
  BT_CHECK(s.find(99) == s.end());
  BT_CHECK_EQ(s.count(2), static_cast<size_t>(1));
  BT_CHECK_EQ(s.count(99), static_cast<size_t>(0));
}

BT_TEST(P4_2, erase_by_key_and_by_iterator) {
  std::set<int> s{1, 2, 3, 4, 5};
  BT_CHECK_EQ(s.erase(3), static_cast<size_t>(1));   // 按 key
  BT_CHECK_EQ(s.erase(3), static_cast<size_t>(0));   // 再删就是 0
  s.erase(s.begin());                               // 按迭代器：删掉最小的 1
  BT_CHECK_EQ(s.count(1), static_cast<size_t>(0));
  BT_CHECK_EQ(JoinAscending(s), std::string("2 4 5"));
}

BT_TEST(P4_2, erase_range) {
  std::set<int> s{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  s.erase(s.find(9), s.end());                       // 删 9..10
  BT_CHECK_EQ(JoinAscending(s), std::string("1 2 3 4 5 6 7 8"));
}

BT_TEST(P4_2, descending_set_with_greater) {
  std::set<int, std::greater<int>> ds{1, 5, 3};
  std::string got;
  for (int x : ds) { got += std::to_string(x); got += " "; }
  BT_CHECK_EQ(got, std::string("5 3 1 "));
}

BT_TEST(P4_2, pair_natural_order) {
  std::set<std::pair<int, int>> ps{{2, 1}, {1, 9}, {1, 2}, {2, 1}};
  BT_CHECK_EQ(ps.size(), static_cast<size_t>(3));    // (2,1) 重复被去掉
  auto it = ps.begin();
  BT_CHECK(*it == std::make_pair(1, 2));
  ++it;
  BT_CHECK(*it == std::make_pair(1, 9));
  ++it;
  BT_CHECK(*it == std::make_pair(2, 1));
}

BT_TEST(P4_2, custom_comparator_orders_by_sum) {
  std::set<std::pair<int, int>, SumCmp> ss{{5, 1}, {1, 1}, {3, 3}, {0, 2}};
  // 和为 2 的 (0,2) 与 (1,1) 平局 → 按 pair 自然序，所以 (0,2) 在前
  BT_CHECK_EQ(JoinBySumOrder(ss), std::string("(0,2) (1,1) (3,3) (5,1)"));
}

BT_TEST(P4_2, custom_comparator_is_strict_weak_ordering) {
  SumCmp c;
  const std::pair<int, int> a{1, 1};
  BT_CHECK(!c(a, a));                                // irreflexive：a<a 必须为 false
  const std::pair<int, int> b{1, 1};
  BT_CHECK(!c(a, b) && !c(b, a));                    // 等价
  const std::pair<int, int> d{2, 2};
  BT_CHECK(c(a, d) && !c(d, a));                     // 反对称
}

BT_TEST(P4_2, map_word_frequency_and_ordered_keys) {
  auto freq = WordFreq({"b", "a", "c", "a", "b", "a"});
  BT_CHECK_EQ(freq.at("a"), 3);
  BT_CHECK_EQ(freq.at("b"), 2);
  BT_CHECK_EQ(freq.at("c"), 1);
  BT_CHECK_EQ(freq.size(), static_cast<size_t>(3));
  BT_CHECK_EQ(KeysJoined(freq), std::string("abc"));  // map 按 key 有序
}

BT_TEST(P4_2, map_empty_and_missing_key) {
  auto freq = WordFreq({});
  BT_CHECK(freq.empty());
  BT_CHECK(freq.find("x") == freq.end());
}

BT_TEST(P4_2, set_large_1e5) {
  std::vector<int> v;
  v.reserve(100000);
  for (int i = 0; i < 100000; ++i) v.push_back(i % 1000);   // 只有 1000 个不同值
  auto s = SortedUnique(v);
  BT_CHECK_EQ(s.size(), static_cast<size_t>(1000));
  BT_CHECK_EQ(*s.begin(), 0);
  BT_CHECK_EQ(*s.rbegin(), 999);
}

BT_TEST(P4_2, set_is_sorted_after_random_inserts) {
  std::set<int> s;
  for (int x : {42, 7, 99, -1, 0, 13, 7, 42}) s.insert(x);
  std::vector<int> got(s.begin(), s.end());
  for (size_t i = 1; i < got.size(); ++i) BT_CHECK(got[i - 1] < got[i]);
}

BT_MAIN("P4.2 set / map：有序、去重、比较器")
