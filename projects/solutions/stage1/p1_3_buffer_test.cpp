// P1.3 测试点：从正常路径到 moved-from / self-move / 大 buffer
#include <iostream>
#include <type_traits>
#include <utility>

#include "test_util.h"
#include "p1_3_buffer.h"

BT_TEST(P1_3, construct_and_size) {
  Buffer b(100);
  BT_CHECK_EQ(b.Size(), static_cast<size_t>(100));
  BT_CHECK(b.Owns());
}

BT_TEST(P1_3, elements_are_zero_initialized) {
  Buffer b(10);
  for (size_t i = 0; i < b.Size(); ++i) BT_CHECK_EQ(b[i], 0);
}

BT_TEST(P1_3, write_then_read) {
  Buffer b(5);
  b[0] = 445;
  b[4] = -7;
  BT_CHECK_EQ(b[0], 445);
  BT_CHECK_EQ(b[4], -7);
}

BT_TEST(P1_3, const_access_and_at) {
  Buffer b(3);
  b[1] = 42;
  const Buffer &cb = b;
  BT_CHECK_EQ(cb[1], 42);
  BT_CHECK_EQ(cb.At(1), 42);
}

BT_TEST(P1_3, at_throws_on_out_of_range) {
  Buffer b(3);
  BT_CHECK_THROWS(b.At(3), std::out_of_range);
  BT_CHECK_THROWS(b.At(999), std::out_of_range);
  const Buffer &cb = b;
  BT_CHECK_THROWS(cb.At(3), std::out_of_range);
}

BT_TEST(P1_3, zero_size_buffer_is_valid) {
  Buffer b(0);
  BT_CHECK_EQ(b.Size(), static_cast<size_t>(0));
  BT_CHECK(b.Owns());                 // 有分配，只是长度为 0
  BT_CHECK_THROWS(b.At(0), std::out_of_range);
}

BT_TEST(P1_3, move_ctor_transfers_ownership) {
  Buffer a(100);
  a[0] = 445;
  Buffer b(std::move(a));
  BT_CHECK_EQ(b.Size(), static_cast<size_t>(100));
  BT_CHECK_EQ(b[0], 445);
  BT_CHECK(b.Owns());
}

BT_TEST(P1_3, move_ctor_empties_source) {
  Buffer a(100);
  Buffer b(std::move(a));
  BT_CHECK(!a.Owns());                // 源不再拥有
  BT_CHECK_EQ(a.Size(), static_cast<size_t>(0));
}

BT_TEST(P1_3, moved_from_object_is_destructible) {
  // 能跑到这里不崩，就说明 moved-from 对象的析构是安全的
  Buffer a(10);
  { Buffer b(std::move(a)); }
  BT_CHECK(!a.Owns());
}

BT_TEST(P1_3, move_assign_releases_old_resource) {
  const int allocs0 = Buffer::allocs, frees0 = Buffer::frees;
  {
    Buffer a(100);
    Buffer b(200);                    // b 自己也有 200 个元素
    b = std::move(a);                 // 必须先把 b 原来的 200 个释放掉
    BT_CHECK_EQ(b.Size(), static_cast<size_t>(100));
    BT_CHECK_EQ(Buffer::allocs - allocs0, 2);
  }
  // 若没释放旧的 200 个元素，frees 会少 1 → 泄漏
  BT_CHECK_EQ(Buffer::frees - frees0, 2);
}

BT_TEST(P1_3, move_assign_empties_source) {
  Buffer a(10);
  a[0] = 1;
  Buffer b(20);
  b = std::move(a);
  BT_CHECK(!a.Owns());
  BT_CHECK_EQ(b[0], 1);
}

BT_TEST(P1_3, self_move_assign_is_safe) {
  Buffer a(64);
  a[0] = 123;
  Buffer &self = a;      // 别名绕开 -Wself-move；我们就是要测自赋值
  a = std::move(self);                   // 原计划漏了这个 case
  BT_CHECK(a.Owns());
  BT_CHECK_EQ(a.Size(), static_cast<size_t>(64));
  BT_CHECK_EQ(a[0], 123);
}

BT_TEST(P1_3, chained_moves) {
  Buffer a(8);
  a[7] = 77;
  Buffer b(std::move(a));
  Buffer c(std::move(b));
  Buffer d(1);
  d = std::move(c);
  BT_CHECK(!a.Owns() && !b.Owns() && !c.Owns());
  BT_CHECK_EQ(d.Size(), static_cast<size_t>(8));
  BT_CHECK_EQ(d[7], 77);
}

BT_TEST(P1_3, no_leak_over_many_moves) {
  const int allocs0 = Buffer::allocs, frees0 = Buffer::frees;
  {
    Buffer cur(1000);
    cur[999] = 1;
    for (int i = 0; i < 1000; ++i) {
      Buffer nxt(1000);
      nxt = std::move(cur);
      cur = std::move(nxt);
    }
    BT_CHECK_EQ(cur[999], 1);
  }
  BT_CHECK_EQ(Buffer::allocs - allocs0, Buffer::frees - frees0);
}

BT_TEST(P1_3, large_buffer_1e7) {
  const size_t n = 10000000;
  Buffer b(n);
  b[0] = 1;
  b[n - 1] = 2;
  BT_CHECK_EQ(b.Size(), n);
  BT_CHECK_EQ(b[0], 1);
  BT_CHECK_EQ(b[n - 1], 2);
}

BT_TEST(P1_3, move_operations_are_noexcept) {
  // 这是 P1.4 结论的直接应用：不标 noexcept，vector 扩容时就不敢移动你
  static_assert(std::is_nothrow_move_constructible_v<Buffer>);
  static_assert(std::is_nothrow_move_assignable_v<Buffer>);
  static_assert(!std::is_copy_constructible_v<Buffer>);
  static_assert(!std::is_copy_assignable_v<Buffer>);
}

BT_TEST(P1_3, buffer_is_usable_inside_std_vector) {
  // 只可移动类型必须能被 vector 移动（依赖上面的 noexcept）
  std::vector<Buffer> v;
  for (int i = 0; i < 20; ++i) v.emplace_back(4);
  v[0][0] = 9;
  BT_CHECK_EQ(v.size(), static_cast<size_t>(20));
  BT_CHECK_EQ(v[0][0], 9);
}

BT_MAIN("P1.3 MoveOnlyBuffer（移动语义 / self-move / 无泄漏）")
