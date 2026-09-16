// P7.1 测试点
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "test_util.h"
#include "p7_1_my_unique_ptr.h"

using namespace myptr7;

namespace {
struct Foo {
  inline static int live = 0;
  inline static int copies = 0;
  int v;
  explicit Foo(int x) : v(x) { ++live; }
  Foo(const Foo &o) : v(o.v) { ++live; ++copies; }
  ~Foo() { --live; }
  int Get() const { return v; }
  static void Reset() { live = 0; copies = 0; }
};
}  // namespace

BT_TEST(P7_1, default_constructed_is_null) {
  MyUniquePtr<Foo> p;
  BT_CHECK(p.Get() == nullptr);
  BT_CHECK(!p);
  BT_CHECK(!static_cast<bool>(p));
}

BT_TEST(P7_1, explicit_ctor_owns_pointer) {
  Foo::Reset();
  {
    MyUniquePtr<Foo> p(new Foo(42));
    BT_CHECK(p.Get() != nullptr);
    BT_CHECK(static_cast<bool>(p));
    BT_CHECK_EQ(p->Get(), 42);            // operator->
    BT_CHECK_EQ((*p).Get(), 42);          // operator*
    BT_CHECK_EQ(Foo::live, 1);
  }
  BT_CHECK_EQ(Foo::live, 0);              // 析构自动 delete
}

BT_TEST(P7_1, operator_star_returns_reference) {
  MyUniquePtr<Foo> p(new Foo(1));
  (*p).v = 99;
  BT_CHECK_EQ(p->Get(), 99);
}

BT_TEST(P7_1, make_unique_variadic) {
  Foo::Reset();
  {
    auto p = MyMakeUnique<Foo>(7);
    BT_CHECK_EQ(p->Get(), 7);
    BT_CHECK_EQ(Foo::live, 1);
    BT_CHECK_EQ(Foo::copies, 0);          // 直接构造，没有拷贝
  }
  BT_CHECK_EQ(Foo::live, 0);
}

BT_TEST(P7_1, make_unique_with_string) {
  auto s = MyMakeUnique<std::string>(5, 'x');
  BT_CHECK_EQ(*s, std::string("xxxxx"));
}

BT_TEST(P7_1, move_ctor_transfers_and_empties_source) {
  Foo::Reset();
  {
    auto p = MyMakeUnique<Foo>(5);
    auto q = std::move(p);
    BT_CHECK(!p);
    BT_CHECK(q && q->Get() == 5);
    BT_CHECK_EQ(Foo::live, 1);            // 全程只有一个对象
    BT_CHECK_EQ(Foo::copies, 0);
  }
  BT_CHECK_EQ(Foo::live, 0);
}

BT_TEST(P7_1, move_assign_releases_old) {
  Foo::Reset();
  {
    auto a = MyMakeUnique<Foo>(1);
    auto b = MyMakeUnique<Foo>(2);
    b = std::move(a);
    BT_CHECK(!a);
    BT_CHECK_EQ(b->Get(), 1);
    BT_CHECK_EQ(Foo::live, 1);            // b 原来那个已经被删掉
  }
  BT_CHECK_EQ(Foo::live, 0);
}

BT_TEST(P7_1, self_move_assign_is_safe) {
  auto p = MyMakeUnique<Foo>(3);
  MyUniquePtr<Foo> &self = p;
  p = std::move(self);
  BT_CHECK(p && p->Get() == 3);
}

BT_TEST(P7_1, copy_is_deleted) {
  static_assert(!std::is_copy_constructible_v<MyUniquePtr<Foo>>);
  static_assert(!std::is_copy_assignable_v<MyUniquePtr<Foo>>);
  static_assert(std::is_nothrow_move_constructible_v<MyUniquePtr<Foo>>);
  static_assert(std::is_nothrow_move_assignable_v<MyUniquePtr<Foo>>);
  BT_CHECK(true);
}

BT_TEST(P7_1, reset_releases_old_and_takes_new) {
  Foo::Reset();
  {
    auto p = MyMakeUnique<Foo>(1);
    p.Reset(new Foo(2));
    BT_CHECK_EQ(p->Get(), 2);
    BT_CHECK_EQ(Foo::live, 1);
    p.Reset();                            // 释放，变空
    BT_CHECK(!p);
    BT_CHECK_EQ(Foo::live, 0);
  }
  BT_CHECK_EQ(Foo::live, 0);
}

BT_TEST(P7_1, reset_to_same_pointer_is_safe_here) {
  // 我们的实现显式做了 p == ptr_ 短路，所以这样写是安全的。
  // ⚠️ 但 std::unique_ptr::reset 没有这个检查 → 对标准库写 reset(get()) 会 double free。
  Foo::Reset();
  {
    auto p = MyMakeUnique<Foo>(7);
    Foo *raw = p.Get();
    p.Reset(raw);
    BT_CHECK(p.Get() == raw);
    BT_CHECK_EQ(p->Get(), 7);
    BT_CHECK_EQ(Foo::live, 1);
  }
  BT_CHECK_EQ(Foo::live, 0);
}

BT_TEST(P7_1, release_gives_up_ownership) {
  Foo::Reset();
  {
    auto p = MyMakeUnique<Foo>(8);
    Foo *raw = p.Release();
    BT_CHECK(!p);
    BT_CHECK(raw != nullptr);
    BT_CHECK_EQ(raw->Get(), 8);
    BT_CHECK_EQ(Foo::live, 1);            // 对象还在，只是没人管了
    delete raw;
  }
  BT_CHECK_EQ(Foo::live, 0);
}

BT_TEST(P7_1, swap) {
  auto a = MyMakeUnique<Foo>(1);
  auto b = MyMakeUnique<Foo>(2);
  a.Swap(b);
  BT_CHECK_EQ(a->Get(), 2);
  BT_CHECK_EQ(b->Get(), 1);
}

BT_TEST(P7_1, swap_with_null) {
  auto a = MyMakeUnique<Foo>(1);
  MyUniquePtr<Foo> b;
  a.Swap(b);
  BT_CHECK(!a);
  BT_CHECK(b && b->Get() == 1);
}

BT_TEST(P7_1, works_inside_vector) {
  Foo::Reset();
  {
    std::vector<MyUniquePtr<Foo>> v;
    for (int i = 0; i < 100; ++i) v.push_back(MyMakeUnique<Foo>(i));
    v[50].Reset(new Foo(9999));
    BT_CHECK_EQ(v.size(), static_cast<size_t>(100));
    BT_CHECK_EQ(v[50]->Get(), 9999);
    BT_CHECK_EQ(v[0]->Get(), 0);
    BT_CHECK_EQ(Foo::live, 100);
  }
  BT_CHECK_EQ(Foo::live, 0);
}

BT_TEST(P7_1, chained_moves) {
  Foo::Reset();
  {
    auto a = MyMakeUnique<Foo>(1);
    auto b = std::move(a);
    auto c = std::move(b);
    auto d = std::move(c);
    BT_CHECK(!a && !b && !c);
    BT_CHECK_EQ(d->Get(), 1);
    BT_CHECK_EQ(Foo::live, 1);
  }
  BT_CHECK_EQ(Foo::live, 0);
}

BT_TEST(P7_1, no_leak_over_many_resets) {
  Foo::Reset();
  {
    auto p = MyMakeUnique<Foo>(0);
    for (int i = 1; i < 5000; ++i) p.Reset(new Foo(i));   // 每次都必须删掉旧的
    BT_CHECK_EQ(p->Get(), 4999);
    BT_CHECK_EQ(Foo::live, 1);
  }
  BT_CHECK_EQ(Foo::live, 0);
}

BT_TEST(P7_1, no_leak_over_many_moves) {
  Foo::Reset();
  {
    auto cur = MyMakeUnique<Foo>(0);
    for (int i = 1; i < 5000; ++i) {
      auto next = MyMakeUnique<Foo>(i);
      next = std::move(cur);              // cur 转移给 next，旧的 next 被释放
      cur = std::move(next);
    }
    BT_CHECK_EQ(Foo::live, 1);
  }
  BT_CHECK_EQ(Foo::live, 0);
}

BT_TEST(P7_1, lifetime_matches_std_unique_ptr_in_simple_use) {
  Foo::Reset();
  {
    auto mine = MyMakeUnique<Foo>(11);
    BT_CHECK_EQ(mine->Get(), 11);
    BT_CHECK_EQ(Foo::live, 1);
  }
  BT_CHECK_EQ(Foo::live, 0);
}

BT_MAIN("P7.1 手写 MyUniquePtr")
