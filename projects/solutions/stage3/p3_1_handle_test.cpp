// P3.1 测试点
#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>

#include "test_util.h"
#include "p3_1_handle.h"

BT_TEST(P3_1, construct_is_valid) {
  Handle h(42);
  BT_CHECK(h.Valid());
  BT_CHECK_EQ(h.Id(), 42);
}

BT_TEST(P3_1, destructor_releases_resource) {
  const int live0 = Handle::live;
  {
    Handle h(1);
    BT_CHECK_EQ(Handle::live, live0 + 1);
  }
  BT_CHECK_EQ(Handle::live, live0);          // 出了作用域自动释放
}

BT_TEST(P3_1, no_copy_allowed) {
  static_assert(!std::is_copy_constructible_v<Handle>);
  static_assert(!std::is_copy_assignable_v<Handle>);
  static_assert(std::is_nothrow_move_constructible_v<Handle>);
  BT_CHECK(true);
}

BT_TEST(P3_1, move_ctor_transfers_and_invalidates_source) {
  Handle a(7);
  Handle b(std::move(a));
  BT_CHECK(b.Valid());
  BT_CHECK_EQ(b.Id(), 7);
  BT_CHECK(!a.Valid());                      // moved-from
}

BT_TEST(P3_1, moved_from_object_destructs_safely) {
  const int live0 = Handle::live;
  {
    Handle a(1);
    { Handle b(std::move(a)); }              // b 先析构
    BT_CHECK(!a.Valid());
  }
  BT_CHECK_EQ(Handle::live, live0);          // 没有重复 release
}

BT_TEST(P3_1, move_assign_releases_old_resource) {
  const int live0 = Handle::live;
  {
    Handle a(1);
    Handle b(2);
    b = std::move(a);                        // b 的旧资源必须被释放
    BT_CHECK_EQ(b.Id(), 1);
    BT_CHECK(!a.Valid());
    BT_CHECK_EQ(Handle::live, live0 + 1);    // 只剩一个
  }
  BT_CHECK_EQ(Handle::live, live0);
}

BT_TEST(P3_1, self_move_assign_is_safe) {
  Handle a(5);
  Handle &self = a;      // 别名绕开 -Wself-move；我们就是要测自赋值
  a = std::move(self);
  BT_CHECK(a.Valid());
  BT_CHECK_EQ(a.Id(), 5);
}

BT_TEST(P3_1, rvo_makes_zero_moves) {
  // C++17 保证：return 一个 prvalue 时对象直接构造在调用者位置
  Handle::ResetStats();
  {
    auto h = MakeHandle(99);
    BT_CHECK_EQ(h.Id(), 99);
    BT_CHECK_EQ(Handle::moves, 0);           // 一次移动都没有
    BT_CHECK_EQ(Handle::acquires, 1);
  }
  BT_CHECK_EQ(Handle::releases, 1);
  BT_CHECK_EQ(Handle::live, 0);
}

BT_TEST(P3_1, explicit_move_does_count_as_move) {
  Handle::ResetStats();
  {
    Handle a(1);
    Handle b(std::move(a));
    BT_CHECK_EQ(Handle::moves, 1);
  }
  BT_CHECK_EQ(Handle::live, 0);
}

BT_TEST(P3_1, many_handles_no_leak) {
  const int live0 = Handle::live;
  const int acq0 = Handle::acquires, rel0 = Handle::releases;
  {
    std::vector<Handle> v;
    for (int i = 0; i < 1000; ++i) v.push_back(MakeHandle(i));
    BT_CHECK_EQ(v.size(), static_cast<size_t>(1000));
    BT_CHECK_EQ(v[999].Id(), 999);
    BT_CHECK_EQ(Handle::live, live0 + 1000);
  }
  BT_CHECK_EQ(Handle::live, live0);
  BT_CHECK_EQ(Handle::acquires - acq0, 1000);
  BT_CHECK_EQ(Handle::releases - rel0, 1000);
}

BT_TEST(P3_1, chained_moves_leave_one_live) {
  const int live0 = Handle::live;
  {
    Handle a(1);
    Handle b(std::move(a));
    Handle c(std::move(b));
    Handle d(std::move(c));
    BT_CHECK(!a.Valid() && !b.Valid() && !c.Valid());
    BT_CHECK(d.Valid());
    BT_CHECK_EQ(Handle::live, live0 + 1);
  }
  BT_CHECK_EQ(Handle::live, live0);
}

BT_MAIN("Test Suite p3_1_handle_test.cpp")
