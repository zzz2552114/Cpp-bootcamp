// P6.1 测试点（含随机对拍：反复多线程累加，恒等于数学期望）
#include <atomic>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include "test_util.h"
#include "p6_1_counter.h"

using namespace counter6;

namespace {
template <typename F> void RunThreads(int n, F fn) {
  std::vector<std::thread> ts;
  ts.reserve(n);
  for (int i = 0; i < n; ++i) ts.emplace_back(fn);
  for (auto &t : ts) t.join();
}
}  // namespace

BT_TEST(P6_1, unsafe_counter_loses_updates) {
  // 用 yield 版稳定复现：多线程 read-modify-write 一定丢更新
  UnsafeCounter c;
  const int kThreads = 8, kIters = 10000;
  RunThreads(kThreads, [&] { c.Increment(kIters); });
  BT_CHECK(c.value < kThreads * kIters);      // 少于 80000
  std::cout << "        [Reference] unsafe = " << c.value << " / " << kThreads * kIters << "\n";
}

BT_TEST(P6_1, locked_counter_is_exact) {
  LockedCounter c;
  const int kThreads = 8, kIters = 10000;
  RunThreads(kThreads, [&] { c.Increment(kIters); });
  BT_CHECK_EQ(c.value, kThreads * kIters);
}

BT_TEST(P6_1, scoped_lock_counter_is_exact) {
  ScopedCounter c;
  const int kThreads = 8, kIters = 10000;
  RunThreads(kThreads, [&] { c.Increment(kIters); });
  BT_CHECK_EQ(c.value, kThreads * kIters);
}

BT_TEST(P6_1, counter_class_basic) {
  Counter c;
  BT_CHECK_EQ(c.Get(), 0);
  c.Increment();
  BT_CHECK_EQ(c.Get(), 1);
  c.Add(41);
  BT_CHECK_EQ(c.Get(), 42);
}

BT_TEST(P6_1, counter_get_is_const_callable) {
  Counter c;
  c.Add(5);
  const Counter &cc = c;
  BT_CHECK_EQ(cc.Get(), 5);          // const 成员里能加锁（mutable mutex）
}

BT_TEST(P6_1, counter_class_multithreaded) {
  Counter c;
  BT_CHECK_EQ(RunCounter(c, 8, 100000), 800000);
}

BT_TEST(P6_1, counter_reset_is_thread_safe_after_join) {
  Counter c;
  RunCounter(c, 4, 1000);
  BT_CHECK_EQ(c.Get(), 4000);
  c.Reset();
  BT_CHECK_EQ(c.Get(), 0);
}

BT_TEST(P6_1, single_thread_exact) {
  Counter c;
  for (int i = 0; i < 1000; ++i) c.Increment();
  BT_CHECK_EQ(c.Get(), 1000);
}

BT_TEST(P6_1, two_threads_exact) {
  Counter c;
  BT_CHECK_EQ(RunCounter(c, 2, 50000), 100000);
}

BT_TEST(P6_1, many_threads_exact) {
  Counter c;
  BT_CHECK_EQ(RunCounter(c, 16, 20000), 320000);
}

BT_TEST(P6_1, stress_repeat_many_configs) {
  // 对拍：随机线程数 / 迭代数，反复跑，必须每次都精确等于期望值
  std::mt19937 rng(445);
  for (int iter = 0; iter < 40; ++iter) {
    const int nthreads = 1 + static_cast<int>(rng() % 8);
    const int iters = 100 + static_cast<int>(rng() % 20000);
    Counter c;
    BT_CHECK_EQ(RunCounter(c, nthreads, iters), nthreads * iters);
  }
}

BT_TEST(P6_1, stress_high_contention) {
  // 线程多、迭代少 → 争抢最激烈
  for (int rep = 0; rep < 10; ++rep) {
    Counter c;
    BT_CHECK_EQ(RunCounter(c, 16, 500), 8000);
  }
}

BT_TEST(P6_1, interleaved_increment_and_get_never_decreases) {
  Counter c;
  std::atomic<bool> stop{false};
  std::thread reader([&] {
    int last = 0;
    while (!stop.load()) {
      const int now = c.Get();
      if (now < last) { BT_CHECK(false); return; }   // 计数只能单调不减
      last = now;
    }
  });
  std::vector<std::thread> writers;
  for (int t = 0; t < 4; ++t) writers.emplace_back([&] { for (int i = 0; i < 20000; ++i) c.Increment(); });
  for (auto &w : writers) w.join();
  stop.store(true);
  reader.join();
  BT_CHECK_EQ(c.Get(), 80000);
}

BT_MAIN("Test Suite p6_1_counter_test.cpp")
