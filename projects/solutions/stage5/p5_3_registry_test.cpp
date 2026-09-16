// P5.3 测试点：引用计数的精确值 + RemoveUser 后对象仍存活
#include <iostream>
#include <memory>
#include <string>

#include "test_util.h"
#include "p5_3_registry.h"

using namespace reg5;

BT_TEST(P5_3, add_user_stores_one_owner) {
  const int live0 = User::live;
  UserRegistry reg;
  reg.AddUser(1, "alice");
  BT_CHECK_EQ(reg.Count(), static_cast<size_t>(1));
  BT_CHECK_EQ(reg.PeekUseCount(1), 1);        // 只有 map 持有
  BT_CHECK_EQ(User::live, live0 + 1);
}

BT_TEST(P5_3, get_user_returns_a_shared_copy) {
  UserRegistry reg;
  reg.AddUser(1, "alice");
  auto u1 = reg.GetUser(1);
  BT_CHECK(u1 != nullptr);
  BT_CHECK_EQ(u1->Name(), std::string("alice"));
  BT_CHECK_EQ(reg.PeekUseCount(1), 2);        // map + u1
  BT_CHECK_EQ(u1.use_count(), 2);
}

BT_TEST(P5_3, two_gets_make_three_owners) {
  UserRegistry reg;
  reg.AddUser(1, "alice");
  auto u1 = reg.GetUser(1);
  auto u2 = reg.GetUser(1);
  BT_CHECK(u1 == u2);                          // 指向同一个对象
  BT_CHECK_EQ(u1.use_count(), 3);              // map + u1 + u2
  BT_CHECK_EQ(reg.PeekUseCount(1), 3);
}

BT_TEST(P5_3, get_missing_user_returns_null) {
  UserRegistry reg;
  BT_CHECK(reg.GetUser(999) == nullptr);
  BT_CHECK_EQ(reg.PeekUseCount(999), -1);
}

BT_TEST(P5_3, remove_user_does_not_destroy_while_borrowed) {
  const int live0 = User::live;
  UserRegistry reg;
  reg.AddUser(1, "alice");
  auto u1 = reg.GetUser(1);
  auto u2 = reg.GetUser(1);
  BT_CHECK_EQ(u1.use_count(), 3);

  BT_CHECK(reg.RemoveUser(1));
  BT_CHECK_EQ(reg.Count(), static_cast<size_t>(0));
  BT_CHECK_EQ(u1.use_count(), 2);              // 只剩 u1、u2
  BT_CHECK_EQ(u1->Name(), std::string("alice"));  // 对象仍然活着
  BT_CHECK_EQ(User::live, live0 + 1);

  u1.reset();
  BT_CHECK_EQ(u2.use_count(), 1);
  BT_CHECK_EQ(User::live, live0 + 1);          // 还有 u2
  u2.reset();
  BT_CHECK_EQ(User::live, live0);              // 最后一个 owner 走了 → 析构
}

BT_TEST(P5_3, last_owner_triggers_destruction) {
  const int live0 = User::live;
  {
    UserRegistry reg;
    reg.AddUser(7, "bob");
    BT_CHECK_EQ(User::live, live0 + 1);
  }                                            // registry 析构 → map 里的 shared_ptr 释放
  BT_CHECK_EQ(User::live, live0);
}

BT_TEST(P5_3, remove_nonexistent_returns_false) {
  UserRegistry reg;
  BT_CHECK(!reg.RemoveUser(42));
}

BT_TEST(P5_3, overwriting_same_id_releases_old) {
  const int live0 = User::live;
  UserRegistry reg;
  reg.AddUser(1, "old");
  reg.AddUser(1, "new");                       // 同 id 覆盖 → 旧对象析构
  BT_CHECK_EQ(reg.Count(), static_cast<size_t>(1));
  BT_CHECK_EQ(User::live, live0 + 1);
  BT_CHECK_EQ(reg.GetUser(1)->Name(), std::string("new"));
}

BT_TEST(P5_3, move_shared_ptr_does_not_change_count) {
  UserRegistry reg;
  reg.AddUser(1, "alice");
  auto u1 = reg.GetUser(1);
  auto u2 = reg.GetUser(1);
  BT_CHECK_EQ(u1.use_count(), 3);
  auto u3 = std::move(u2);                     // 移动：不增减计数
  BT_CHECK(u2 == nullptr);
  BT_CHECK_EQ(u3.use_count(), 3);
  BT_CHECK_EQ(u1.use_count(), 3);
}

BT_TEST(P5_3, shared_ptr_can_be_passed_by_value) {
  UserRegistry reg;
  reg.AddUser(1, "alice");
  auto u1 = reg.GetUser(1);
  auto by_value = [](std::shared_ptr<User> p) { return p.use_count(); };
  BT_CHECK_EQ(by_value(u1), 3);                // 函数内多一个副本
  BT_CHECK_EQ(u1.use_count(), 2);              // 函数返回后副本销毁
}

BT_TEST(P5_3, many_users_no_leak) {
  const int live0 = User::live;
  {
    UserRegistry reg;
    for (int i = 0; i < 5000; ++i) reg.AddUser(i, "u" + std::to_string(i));
    BT_CHECK_EQ(reg.Count(), static_cast<size_t>(5000));
    BT_CHECK_EQ(User::live, live0 + 5000);
  }
  BT_CHECK_EQ(User::live, live0);
}

BT_TEST(P5_3, many_gets_and_releases_balance) {
  const int live0 = User::live;
  UserRegistry reg;
  reg.AddUser(1, "alice");
  for (int i = 0; i < 10000; ++i) {
    auto p = reg.GetUser(1);
    BT_CHECK_EQ(p.use_count(), 2);
  }
  BT_CHECK_EQ(reg.PeekUseCount(1), 1);
  BT_CHECK_EQ(User::live, live0 + 1);
}

BT_MAIN("P5.3 shared_ptr 注册表")
