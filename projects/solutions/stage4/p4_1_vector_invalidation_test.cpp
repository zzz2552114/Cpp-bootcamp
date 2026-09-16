// P4.1 测试点
#include <iostream>
#include <vector>

#include "test_util.h"
#include "p4_1_vector_invalidation.h"

using namespace v4;

BT_TEST(P4_1, new_vector_is_empty) {
  std::vector<int> v;
  BT_CHECK_EQ(v.size(), static_cast<size_t>(0));
  BT_CHECK(v.empty());
  BT_CHECK_EQ(v.capacity(), static_cast<size_t>(0));
}

BT_TEST(P4_1, reserve_changes_capacity_but_not_size) {
  std::vector<int> v;
  v.reserve(100);
  BT_CHECK(v.capacity() >= 100);
  BT_CHECK_EQ(v.size(), static_cast<size_t>(0));   // reserve 不改 size！
}

BT_TEST(P4_1, accessing_reserved_but_unconstructed_slot_is_ub) {
  // 这里用 at() 来"安全地"证明元素还不存在（operator[] 会是 UB）
  std::vector<int> v;
  v.reserve(10);
  BT_CHECK_THROWS(v.at(0), std::out_of_range);
}

BT_TEST(P4_1, resize_changes_size_and_zero_initializes) {
  std::vector<int> v;
  v.resize(100);
  BT_CHECK_EQ(v.size(), static_cast<size_t>(100));
  BT_CHECK(v.capacity() >= 100);
  for (size_t i = 0; i < 100; ++i) BT_CHECK_EQ(v[i], 0);
  v[50] = 1;                                       // resize 之后就可以写了
  BT_CHECK_EQ(v[50], 1);
}

BT_TEST(P4_1, resize_down_truncates) {
  std::vector<int> v{1, 2, 3, 4, 5};
  v.resize(2);
  BT_CHECK_EQ(v.size(), static_cast<size_t>(2));
  BT_CHECK_EQ(v[1], 2);
  BT_CHECK(v.capacity() >= 5);                     // 容量通常不缩
}

BT_TEST(P4_1, clear_keeps_capacity) {
  std::vector<int> v{1, 2, 3};
  const size_t cap = v.capacity();
  v.clear();
  BT_CHECK_EQ(v.size(), static_cast<size_t>(0));
  BT_CHECK_EQ(v.capacity(), cap);
}

BT_TEST(P4_1, address_stable_when_within_capacity) {
  BT_CHECK(AddressStableWithinCapacity(1000, 1000));   // 全程不越界 → 地址不变
  BT_CHECK(AddressStableWithinCapacity(100, 100));
  BT_CHECK(AddressStableWithinCapacity(1, 1));
}

BT_TEST(P4_1, address_changes_when_past_capacity) {
  BT_CHECK(AddressChangesPastCapacity(100));          // 超过 capacity → 重分配
  BT_CHECK(AddressChangesPastCapacity(10000));
}

BT_TEST(P4_1, reserved_capacity_is_never_exceeded_by_push) {
  for (size_t n : {size_t(1), size_t(2), size_t(8), size_t(64), size_t(1000)}) {
    std::vector<int> v;
    v.reserve(n);
    const uintptr_t base = DataAddr(v);
    for (size_t i = 0; i < n; ++i) v.push_back(1);
    BT_CHECK_EQ(DataAddr(v), base);              // 一次都没搬
    BT_CHECK_EQ(v.size(), n);
  }
}

BT_TEST(P4_1, capacity_always_at_least_size) {
  std::vector<int> v;
  for (int i = 0; i < 5000; ++i) {
    v.push_back(i);
    BT_CHECK(v.capacity() >= v.size());
  }
}

BT_TEST(P4_1, growth_is_amortized_not_linear_realloc) {
  // 5000 次 push 的重分配次数应远小于 5000（通常 ~log2）
  std::vector<int> v;
  size_t reallocs = 0;
  size_t last_cap = v.capacity();
  for (int i = 0; i < 5000; ++i) {
    v.push_back(i);
    if (v.capacity() != last_cap) { ++reallocs; last_cap = v.capacity(); }
  }
  BT_CHECK(reallocs < 40);
}

BT_TEST(P4_1, iterator_arithmetic_and_range) {
  std::vector<int> v{10, 20, 30, 40, 50};
  auto it = v.begin();
  BT_CHECK_EQ(*(it + 2), 30);
  BT_CHECK_EQ(*(v.end() - 1), 50);
  BT_CHECK_EQ(v.end() - v.begin(), 5);
  v.erase(v.begin() + 2);                        // 删掉 30
  BT_CHECK_EQ(v.size(), static_cast<size_t>(4));
  BT_CHECK_EQ(v[2], 40);
}

BT_TEST(P4_1, erase_range_shrinks) {
  std::vector<int> v{0, 1, 2, 3, 4, 5, 6};
  v.erase(v.begin() + 1, v.end());               // 只留第一个
  BT_CHECK_EQ(v.size(), static_cast<size_t>(1));
  BT_CHECK_EQ(v[0], 0);
}

BT_TEST(P4_1, insert_returns_iterator_to_new_element) {
  std::vector<int> v{1, 3, 4};
  auto it = v.insert(v.begin() + 1, 2);          // 插到中间
  BT_CHECK_EQ(*it, 2);
  std::vector<int> expect{1, 2, 3, 4};
  BT_CHECK(v == expect);
}

BT_TEST(P4_1, reserve_enough_then_erase_is_safe) {
  std::vector<int> v;
  v.reserve(1000);
  for (int i = 0; i < 1000; ++i) v.push_back(i);
  const uintptr_t base = DataAddr(v);
  for (int i = 0; i < 500; ++i) v.pop_back();    // 只缩不扩 → 不该重分配
  BT_CHECK_EQ(DataAddr(v), base);
  BT_CHECK_EQ(v.size(), static_cast<size_t>(500));
}

BT_TEST(P4_1, large_push_1e6_capacity_and_size) {
  const size_t n = 1000000;
  auto cs = PushNTimes(n);
  BT_CHECK_EQ(cs.size, n);
  BT_CHECK(cs.cap >= n);
  BT_CHECK(cs.cap < 2 * n + 100);               // 容量不会离谱地爆炸
}

BT_TEST(P4_1, large_reserve_then_push_no_realloc) {
  const size_t n = 1000000;
  std::vector<int> v;
  v.reserve(n);
  const uintptr_t base = DataAddr(v);
  for (size_t i = 0; i < n; ++i) v.push_back(static_cast<int>(i & 0xffff));
  BT_CHECK_EQ(DataAddr(v), base);
  BT_CHECK_EQ(v.size(), n);
}

BT_MAIN("P4.1 vector：迭代器失效 / reserve vs resize")
