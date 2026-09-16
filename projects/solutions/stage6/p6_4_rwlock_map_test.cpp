// P6.4 测试点
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "test_util.h"
#include "p6_4_rwlock_map.h"

using namespace rw6;

namespace {
template <typename F> void RunThreads(int n, F fn) {
  std::vector<std::thread> ts;
  ts.reserve(n);
  for (int i = 0; i < n; ++i) ts.emplace_back(fn);
  for (auto &t : ts) t.join();
}
// std::thread 会拷贝 lambda；`[id = 0]() mutable { id++ }` 那种"每线程自增编号"是错的
// （每个线程拿到的都是 0）。需要唯一编号时必须显式传入 index。
template <typename F> void RunThreadsIdx(int n, F fn) {
  std::vector<std::thread> ts;
  ts.reserve(n);
  for (int i = 0; i < n; ++i) ts.emplace_back([&fn, i] { fn(i); });
  for (auto &t : ts) t.join();
}
}  // namespace

BT_TEST(P6_4, single_thread_put_get_remove) {
  ConcurrentMap m;
  BT_CHECK_EQ(m.Size(), static_cast<size_t>(0));
  m.Put(1, 100);
  int v = 0;
  BT_CHECK(m.Get(1, v));
  BT_CHECK_EQ(v, 100);
  BT_CHECK(m.Contains(1));
  BT_CHECK(!m.Contains(2));
  BT_CHECK(m.Remove(1));
  BT_CHECK(!m.Remove(1));                    // 再删返回 false
  BT_CHECK_EQ(m.Size(), static_cast<size_t>(0));
}

BT_TEST(P6_4, get_on_missing_key) {
  ConcurrentMap m;
  int v = -1;
  BT_CHECK(!m.Get(42, v));
  BT_CHECK_EQ(v, -1);                        // 没被改
  BT_CHECK(!m.Get(42).has_value());
}

BT_TEST(P6_4, get_overload_returns_optional) {
  ConcurrentMap m;
  m.Put(5, 55);
  BT_CHECK_EQ(m.Get(5).value(), 55);
  BT_CHECK(!m.Get(6).has_value());
}

BT_TEST(P6_4, put_overwrites) {
  ConcurrentMap m;
  m.Put(1, 1);
  m.Put(1, 2);
  BT_CHECK_EQ(m.Size(), static_cast<size_t>(1));
  BT_CHECK_EQ(m.Get(1).value(), 2);
}

BT_TEST(P6_4, clear) {
  ConcurrentMap m;
  for (int i = 0; i < 100; ++i) m.Put(i, i);
  BT_CHECK_EQ(m.Size(), static_cast<size_t>(100));
  m.Clear();
  BT_CHECK_EQ(m.Size(), static_cast<size_t>(0));
}

BT_TEST(P6_4, get_is_const_callable) {
  ConcurrentMap m;
  m.Put(1, 1);
  const ConcurrentMap &cm = m;
  BT_CHECK_EQ(cm.Get(1).value(), 1);
  BT_CHECK_EQ(cm.Size(), static_cast<size_t>(1));
}

BT_TEST(P6_4, concurrent_readers_see_consistent_values) {
  ConcurrentMap m;
  constexpr int kKeys = 200;
  for (int k = 0; k < kKeys; ++k) m.Put(k, k * 2);     // 不变量：value == key*2

  std::atomic<int> bad{0};
  RunThreadsIdx(8, [&](int id) {
    for (int i = 0; i < 2000; ++i) {
      const int key = (id * 31 + i) % kKeys;
      const int v = m.Get(key).value();
      if (v != key * 2) bad.fetch_add(1);               // 读到的快照必须自洽
    }
  });
  BT_CHECK_EQ(bad.load(), 0);
}

BT_TEST(P6_4, readers_run_in_parallel) {
  ConcurrentMap m;
  for (int k = 0; k < 100; ++k) m.Put(k, k);

  std::atomic<int> active{0};
  std::atomic<int> max_active{0};
  RunThreads(8, [&] {
    for (int i = 0; i < 200; ++i) {
      const int cur = active.fetch_add(1) + 1;
      int prev = max_active.load();
      while (cur > prev && !max_active.compare_exchange_weak(prev, cur)) {
      }
      (void)m.Get(i % 100);
      std::this_thread::sleep_for(std::chrono::milliseconds(1));   // 拉长重叠窗口
      active.fetch_sub(1);
    }
  });
  std::cout << "        [参考] 观测到的最大并发读者数 = " << max_active.load() << "\n";
  BT_CHECK_EQ(active.load(), 0);
  BT_CHECK(max_active.load() >= 2);          // 读锁之间确实共享
}

BT_TEST(P6_4, writers_are_exclusive_final_state_correct) {
  ConcurrentMap m;
  constexpr int kWriters = 4;
  constexpr int kPerWriter = 2000;
  // 每个 writer 写自己独立的 key 空间 → 最终状态可精确校验
  RunThreadsIdx(kWriters, [&](int id) {
    for (int i = 0; i < kPerWriter; ++i) m.Put(id * 100000 + i, i);
  });
  BT_CHECK_EQ(m.Size(), static_cast<size_t>(kWriters * kPerWriter));
  for (int w = 0; w < kWriters; ++w) {
    for (int i = 0; i < kPerWriter; ++i) {
      BT_CHECK_EQ(m.Get(w * 100000 + i).value(), i);
    }
  }
}

BT_TEST(P6_4, mixed_readers_and_writers_no_corruption) {
  ConcurrentMap m;
  constexpr int kStable = 500;
  for (int k = 0; k < kStable; ++k) m.Put(k, k * 3);    // 读者只碰这段稳定区间

  std::atomic<int> bad{0};
  std::vector<std::thread> ts;
  for (int r = 0; r < 8; ++r) {
    ts.emplace_back([&, r] {
      for (int i = 0; i < 3000; ++i) {
        const int key = (r * 17 + i) % kStable;
        if (m.Get(key).value() != key * 3) bad.fetch_add(1);
      }
    });
  }
  for (int w = 0; w < 2; ++w) {
    ts.emplace_back([&, w] {
      for (int i = 0; i < 5000; ++i) m.Put(100000 + w * 10000 + i, i);   // 写另一段键空间
    });
  }
  for (auto &t : ts) t.join();

  BT_CHECK_EQ(bad.load(), 0);
  BT_CHECK_EQ(m.Size(), static_cast<size_t>(kStable + 10000));
  for (int k = 0; k < kStable; ++k) BT_CHECK_EQ(m.Get(k).value(), k * 3);
}

BT_TEST(P6_4, concurrent_insert_then_size) {
  ConcurrentMap m;
  RunThreadsIdx(8, [&](int id) {
    for (int i = 0; i < 5000; ++i) m.Put(id * 100000 + i, 1);
  });
  BT_CHECK_EQ(m.Size(), static_cast<size_t>(40000));
}

BT_TEST(P6_4, concurrent_remove_bijection) {
  ConcurrentMap m;
  constexpr int kN = 20000;
  for (int i = 0; i < kN; ++i) m.Put(i, i);
  std::atomic<int> removed{0};
  RunThreadsIdx(8, [&](int id) {
    for (int i = id; i < kN; i += 8) {
      if (m.Remove(i)) removed.fetch_add(1);
    }
  });
  BT_CHECK_EQ(removed.load(), kN);          // 每个 key 恰好被一个线程删掉
  BT_CHECK_EQ(m.Size(), static_cast<size_t>(0));
}

BT_TEST(P6_4, stress_repeated_rounds) {
  for (int rep = 0; rep < 15; ++rep) {
    ConcurrentMap m;
    for (int k = 0; k < 50; ++k) m.Put(k, k);
    std::atomic<int> bad{0};
    std::vector<std::thread> ts;
    for (int r = 0; r < 4; ++r) {
      ts.emplace_back([&, r] {
        for (int i = 0; i < 500; ++i) {
          const int key = (r * 7 + i) % 50;
          if (m.Get(key).value() != key) bad.fetch_add(1);
        }
      });
    }
    ts.emplace_back([&] { for (int i = 0; i < 500; ++i) m.Put(1000 + i, i); });
    for (auto &t : ts) t.join();
    BT_CHECK_EQ(bad.load(), 0);
    BT_CHECK_EQ(m.Size(), static_cast<size_t>(550));
  }
}

BT_MAIN("P6.4 读写锁并发 Map")
