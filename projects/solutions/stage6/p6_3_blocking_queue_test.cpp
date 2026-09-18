// P6.3 测试点（含大规模"不丢不重"对拍）
#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <numeric>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "test_util.h"
#include "p6_3_blocking_queue.h"

using namespace bq6;

BT_TEST(P6_3, single_thread_push_pop_fifo) {
  BlockingQueue<int> q(8);
  for (int i = 1; i <= 5; ++i) q.Push(i);
  BT_CHECK_EQ(q.Size(), static_cast<size_t>(5));
  for (int i = 1; i <= 5; ++i) BT_CHECK_EQ(q.Pop(), i);   // 先进先出
  BT_CHECK_EQ(q.Size(), static_cast<size_t>(0));
}

BT_TEST(P6_3, pop_on_empty_after_close_throws) {
  BlockingQueue<int> q(4);
  q.Close();
  BT_CHECK_THROWS(q.Pop(), ClosedError);
}

BT_TEST(P6_3, push_after_close_throws) {
  BlockingQueue<int> q(4);
  q.Close();
  BT_CHECK_THROWS(q.Push(1), ClosedError);
}

BT_TEST(P6_3, closed_flag_and_capacity) {
  BlockingQueue<int> q(3);
  BT_CHECK(!q.Closed());
  BT_CHECK_EQ(q.Capacity(), static_cast<size_t>(3));
  q.Close();
  BT_CHECK(q.Closed());
}

BT_TEST(P6_3, drained_after_close_still_pops_remaining) {
  BlockingQueue<int> q(8);
  for (int i = 0; i < 3; ++i) q.Push(i);
  q.Close();
  // 已关闭但还有元素 → 先把剩下的取完
  BT_CHECK_EQ(q.Pop(), 0);
  BT_CHECK_EQ(q.Pop(), 1);
  BT_CHECK_EQ(q.Pop(), 2);
  BT_CHECK_THROWS(q.Pop(), ClosedError);                // 空了才抛
}

BT_TEST(P6_3, push_blocks_until_consumer_frees_space) {
  BlockingQueue<int> q(2);
  q.Push(1);
  q.Push(2);                                            // 队列满

  std::atomic<bool> blocked{false};
  std::thread producer([&] {
    blocked.store(true);
    q.Push(3);                                          // 必须等消费者腾出空间
  });

  while (!blocked.load()) std::this_thread::yield();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  BT_CHECK_EQ(q.Size(), static_cast<size_t>(2));        // 生产者确实被挡住了

  BT_CHECK_EQ(q.Pop(), 1);                              // 腾出一个位置
  producer.join();                                      // 生产者得以继续
  BT_CHECK_EQ(q.Size(), static_cast<size_t>(2));
  BT_CHECK_EQ(q.Pop(), 2);
  BT_CHECK_EQ(q.Pop(), 3);
}

BT_TEST(P6_3, pop_blocks_until_producer_pushes) {
  BlockingQueue<int> q(4);
  std::atomic<int> got{-1};
  std::thread consumer([&] { got.store(q.Pop()); });

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  BT_CHECK_EQ(got.load(), -1);                          // 还没东西可拿
  q.Push(42);
  consumer.join();
  BT_CHECK_EQ(got.load(), 42);
}

BT_TEST(P6_3, close_wakes_up_blocked_consumer) {
  BlockingQueue<int> q(4);
  std::atomic<bool> threw{false};
  std::thread consumer([&] {
    try {
      q.Pop();
    } catch (const ClosedError &) {
      threw.store(true);
    }
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  q.Close();                                            // 必须唤醒等待中的消费者
  consumer.join();
  BT_CHECK(threw.load());
}

BT_TEST(P6_3, close_wakes_up_blocked_producer) {
  BlockingQueue<int> q(1);
  q.Push(1);                                            // 满了
  std::atomic<bool> threw{false};
  std::thread producer([&] {
    try {
      q.Push(2);
    } catch (const ClosedError &) {
      threw.store(true);
    }
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  q.Close();
  producer.join();
  BT_CHECK(threw.load());
}

BT_TEST(P6_3, move_only_payload) {
  BlockingQueue<std::unique_ptr<int>> q(4);
  q.Push(std::make_unique<int>(7));
  auto p = q.Pop();
  BT_CHECK(p != nullptr);
  BT_CHECK_EQ(*p, 7);
}

BT_TEST(P6_3, stress_multi_producer_consumer_no_loss_no_dup) {
  constexpr int kProducers = 4;
  constexpr int kConsumers = 4;
  constexpr int kPerProducer = 10000;
  constexpr int kTotal = kProducers * kPerProducer;

  BlockingQueue<int> q(64);
  std::mutex seen_mu;
  std::vector<bool> seen(kTotal, false);
  std::atomic<int> consumed{0};

  std::vector<std::thread> producers;
  for (int p = 0; p < kProducers; ++p) {
    producers.emplace_back([&q, p] {
      for (int j = 0; j < kPerProducer; ++j) q.Push(p * kPerProducer + j);   // 全局唯一值
    });
  }
  std::vector<std::thread> consumers;
  for (int c = 0; c < kConsumers; ++c) {
    consumers.emplace_back([&] {
      try {
        while (true) {
          const int v = q.Pop();
          {
            std::lock_guard<std::mutex> lk(seen_mu);
            if (v < 0 || v >= kTotal || seen[v]) { BT_CHECK(false); return; }  // 非法/重复
            seen[v] = true;
          }
          consumed.fetch_add(1);
        }
      } catch (const ClosedError &) {
      }
    });
  }

  for (auto &t : producers) t.join();
  q.Close();
  for (auto &t : consumers) t.join();

  BT_CHECK_EQ(consumed.load(), kTotal);                  // 不丢
  for (int i = 0; i < kTotal; ++i) BT_CHECK(seen[i]);    // 每个值恰好被消费一次
  BT_CHECK_EQ(q.Size(), static_cast<size_t>(0));
  BT_CHECK(q.Closed());
}

BT_TEST(P6_3, stress_small_capacity_heavy_contention) {
  // 容量只有 1 → 生产者/消费者必须频繁互相唤醒，最能暴露条件变量写错
  constexpr int kItems = 20000;
  BlockingQueue<int> q(1);
  std::atomic<long long> sum{0};
  std::atomic<int> count{0};

  std::thread producer([&] {
    for (int i = 0; i < kItems; ++i) q.Push(i);
    q.Close();
  });
  std::vector<std::thread> consumers;
  for (int c = 0; c < 4; ++c) {
    consumers.emplace_back([&] {
      try {
        while (true) {
          const int v = q.Pop();
          sum.fetch_add(v);
          count.fetch_add(1);
        }
      } catch (const ClosedError &) {
      }
    });
  }
  producer.join();
  for (auto &t : consumers) t.join();

  BT_CHECK_EQ(count.load(), kItems);
  BT_CHECK_EQ(sum.load(), 1LL * kItems * (kItems - 1) / 2);
}

BT_TEST(P6_3, stress_repeated_rounds) {
  for (int rep = 0; rep < 12; ++rep) {
    BlockingQueue<int> q(4);
    std::atomic<long long> sum{0};
    std::atomic<int> count{0};
    constexpr int kItems = 3000;

    std::thread p1([&] { for (int i = 0; i < kItems; ++i) q.Push(i); });
    std::thread p2([&] { for (int i = 0; i < kItems; ++i) q.Push(-i); });
    std::vector<std::thread> cs;
    for (int c = 0; c < 3; ++c) {
      cs.emplace_back([&] {
        try {
          while (true) {
            sum.fetch_add(q.Pop());
            count.fetch_add(1);
          }
        } catch (const ClosedError &) {
        }
      });
    }
    p1.join();
    p2.join();
    q.Close();
    for (auto &t : cs) t.join();

    BT_CHECK_EQ(count.load(), 2 * kItems);
    BT_CHECK_EQ(sum.load(), 0LL);                        // i + (-i) 抵消
  }
}

BT_MAIN("Test Suite p6_3_blocking_queue_test.cpp")
