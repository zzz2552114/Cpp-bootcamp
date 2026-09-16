// P1.5 测试点：移动的"元素级"代价（确定性）+ 真实计时（参考）
#include <chrono>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "test_util.h"
#include "p1_5_bench.h"

using Clock = std::chrono::steady_clock;

BT_TEST(P1_5, move_does_zero_element_copies_but_copy_does_n) {
  const int n = 1000;
  std::vector<Tracked> src;
  src.reserve(n);
  for (int i = 0; i < n; ++i) src.emplace_back(i);

  Tracked::Reset();
  auto copied = src;                      // 拷贝整个 vector
  BT_CHECK_EQ(Tracked::copies, n);        // 每个元素都被拷贝一次
  BT_CHECK_EQ(copied.size(), static_cast<size_t>(n));

  Tracked::Reset();
  auto moved = std::move(src);            // 移动整个 vector：只偷缓冲区
  BT_CHECK_EQ(Tracked::copies, 0);        // 元素一次都没被拷贝
  BT_CHECK_EQ(Tracked::moves, 0);         // 甚至不需要移动元素
  BT_CHECK_EQ(moved.size(), static_cast<size_t>(n));
  BT_CHECK_EQ(copied.size(), static_cast<size_t>(n));
}

BT_TEST(P1_5, moved_content_is_intact) {
  const int n = 500;
  std::vector<Tracked> src;
  src.reserve(n);
  for (int i = 0; i < n; ++i) src.emplace_back(i);
  const long long expect = SumValues(src);

  auto moved = std::move(src);
  BT_CHECK_EQ(SumValues(moved), expect);
}

BT_TEST(P1_5, move_is_cheap_for_strings_too) {
  const size_t n = 100000;
  auto a = MakeLongStrings(n);
  const size_t expect_len = TotalLength(a);

  auto b = std::move(a);                  // 只搬指针
  BT_CHECK_EQ(TotalLength(b), expect_len);
  BT_CHECK_EQ(b.size(), n);
}

BT_TEST(P1_5, timing_copy_vs_move_informational) {
  const size_t n = 200000;
  auto v1 = MakeLongStrings(n);
  auto v1b = v1;

  auto t0 = Clock::now();
  std::vector<std::string> copied = v1;
  auto t1 = Clock::now();
  std::vector<std::string> moved = std::move(v1b);
  auto t2 = Clock::now();

  auto copy_us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
  auto move_us = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

  std::cout << "        [参考] copy=" << copy_us << "us  move=" << move_us
            << "us  ratio=" << (move_us ? copy_us / move_us : 0) << "x\n";

  BT_CHECK_EQ(copied.size(), n);
  BT_CHECK_EQ(moved.size(), n);
  BT_CHECK(copy_us > 0);
  // 只做很宽松的断言，避免机器抖动导致 flaky
  BT_CHECK(copy_us <= move_us * 5 + 100000);
}

BT_TEST(P1_5, stress_many_sizes_content_equal) {
  for (size_t n : {size_t(0), size_t(1), size_t(2), size_t(3), size_t(7), size_t(64),
                   size_t(255), size_t(1000), size_t(4096)}) {
    auto a = MakeLongStrings(n);
    const size_t expect = TotalLength(a);
    auto b = std::move(a);
    BT_CHECK_EQ(b.size(), n);
    BT_CHECK_EQ(TotalLength(b), expect);
  }
}

BT_TEST(P1_5, moved_from_vector_is_reusable) {
  std::vector<std::string> a = MakeLongStrings(10);
  auto b = std::move(a);
  // moved-from 对象仍然活着：可以安全地重新赋值、重新使用
  a = std::vector<std::string>{"hello"};
  BT_CHECK_EQ(a.size(), static_cast<size_t>(1));
  BT_CHECK_EQ(a[0], std::string("hello"));
  BT_CHECK_EQ(b.size(), static_cast<size_t>(10));
}

BT_MAIN("P1.5 拷贝 vs 移动（元素级代价 + 计时）")
