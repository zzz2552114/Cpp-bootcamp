// P1.4 测试点：noexcept 决定扩容时是 move 还是 copy
#include <iostream>
#include <type_traits>
#include <utility>

#include "test_util.h"
#include "p1_4_noexcept.h"

BT_TEST(P1_4, noexcept_move_is_detected) {
  static_assert(std::is_nothrow_move_constructible_v<Counted>);
  static_assert(!std::is_nothrow_move_constructible_v<CountedThrowy>);
  static_assert(!std::is_nothrow_move_constructible_v<MoveOnlyThrowy>);
  BT_CHECK(true);
}

BT_TEST(P1_4, noexcept_move_never_copies_on_realloc) {
  Counted::Reset();
  GrowVector<Counted>(64);
  BT_CHECK_EQ(Counted::copies, 0);        // 一次拷贝都没有
  BT_CHECK(Counted::moves > 0);           // 全靠移动
}

BT_TEST(P1_4, throwy_move_forces_copy_on_realloc) {
  CountedThrowy::Reset();
  GrowVector<CountedThrowy>(64);
  BT_CHECK(CountedThrowy::copies > 0);    // 扩容时"宁拷贝不移动"
  BT_CHECK(CountedThrowy::moves > 0);     // 临时对象入容器仍是移动
}

BT_TEST(P1_4, copies_only_come_from_reallocation_not_from_push) {
  // 先 reserve 足够容量，再 push：没有任何 realloc，就不该出现拷贝
  CountedThrowy::Reset();
  std::vector<CountedThrowy> v;
  v.reserve(64);
  for (int i = 0; i < 64; ++i) v.push_back(CountedThrowy(i));
  BT_CHECK_EQ(CountedThrowy::copies, 0);
  BT_CHECK_EQ(CountedThrowy::moves, 64);
}

BT_TEST(P1_4, no_copy_available_so_move_is_used_anyway) {
  MoveOnlyThrowy::Reset();
  GrowVector<MoveOnlyThrowy>(64);         // 能编译并能跑，说明退化为移动
  BT_CHECK(MoveOnlyThrowy::moves > 0);
}

BT_TEST(P1_4, move_only_type_works_in_vector) {
  std::vector<MoveOnlyThrowy> v;
  v.reserve(16);
  for (int i = 0; i < 16; ++i) v.emplace_back(i);
  BT_CHECK_EQ(v.size(), static_cast<size_t>(16));
  BT_CHECK_EQ(v[15].v, 15);
}

BT_TEST(P1_4, large_realloc_still_zero_copies_with_noexcept) {
  Counted::Reset();
  GrowVector<Counted>(2048);
  BT_CHECK_EQ(Counted::copies, 0);
}

BT_TEST(P1_4, values_survive_reallocation) {
  std::vector<Counted> v;
  for (int i = 0; i < 100; ++i) v.push_back(Counted(i * 3));
  for (int i = 0; i < 100; ++i) BT_CHECK_EQ(v[i].v, i * 3);
}

BT_TEST(P1_4, why_it_matters_vector_of_moveonly_uses_move) {
  // 这就是 Buffer（P1.3）为什么必须把移动构造标 noexcept：
  // 否则把它放进 vector 就会退化成"不允许"（拷贝被删）或"必须移动"的不确定状态。
  std::vector<Counted> v;
  v.emplace_back(1);
  v.emplace_back(2);                     // 触发扩容
  BT_CHECK_EQ(v[0].v, 1);
  BT_CHECK_EQ(v[1].v, 2);
}

BT_MAIN("Test Suite p1_4_noexcept_test.cpp")
