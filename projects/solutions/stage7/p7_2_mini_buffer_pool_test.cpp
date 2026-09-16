// P7.2 测试点（含与"单线程参考模型"的随机对拍）
#include <atomic>
#include <chrono>
#include <iostream>
#include <random>
#include <set>
#include <thread>
#include <vector>

#include "test_util.h"
#include "p7_2_mini_buffer_pool.h"

using namespace bp7;

BT_TEST(P7_2, fresh_pool_is_empty) {
  MiniBufferPool pool(3);
  BT_CHECK_EQ(pool.NumPages(), static_cast<size_t>(0));
  BT_CHECK_EQ(pool.FreeFrames(), static_cast<size_t>(3));
  BT_CHECK_EQ(pool.FrameCount(), static_cast<size_t>(3));
}

BT_TEST(P7_2, pin_fresh_page_allocates_frame) {
  MiniBufferPool pool(3);
  Page *p = pool.Pin(1);
  BT_CHECK(p != nullptr);
  BT_CHECK_EQ(p->page_id, 1);
  BT_CHECK_EQ(pool.NumPages(), static_cast<size_t>(1));
  BT_CHECK_EQ(pool.FreeFrames(), static_cast<size_t>(2));
  BT_CHECK(pool.HasPage(1));
}

BT_TEST(P7_2, pin_existing_page_returns_same_frame) {
  MiniBufferPool pool(3);
  Page *a = pool.Pin(1);
  a->data[0] = 42;
  Page *b = pool.Pin(1);                  // 命中
  BT_CHECK(a == b);                       // 同一帧
  BT_CHECK_EQ(b->data[0], 42);            // 数据保留
  BT_CHECK_EQ(pool.NumPages(), static_cast<size_t>(1));   // 没有多占帧
  BT_CHECK_EQ(pool.FreeFrames(), static_cast<size_t>(2));
}

BT_TEST(P7_2, fresh_page_data_is_zeroed) {
  MiniBufferPool pool(2);
  Page *a = pool.Pin(1);
  a->data[3] = 7;
  pool.Unpin(1);
  Page *b = pool.Pin(2);                  // 复用刚才的帧
  for (int i = 0; i < 8; ++i) BT_CHECK_EQ(b->data[i], 0);  // 必须被清零
}

BT_TEST(P7_2, data_isolation_between_pages) {
  MiniBufferPool pool(2);
  Page *a = pool.Pin(10);
  Page *b = pool.Pin(11);
  BT_CHECK(a != b);
  a->data[0] = 1;
  b->data[0] = 2;
  BT_CHECK_EQ(pool.Pin(10)->data[0], 1);
  BT_CHECK_EQ(pool.Pin(11)->data[0], 2);
  pool.Unpin(10);
  pool.Unpin(11);
}

BT_TEST(P7_2, unpin_frees_the_frame) {
  MiniBufferPool pool(3);
  pool.Pin(1);
  pool.Pin(2);
  BT_CHECK_EQ(pool.FreeFrames(), static_cast<size_t>(1));
  pool.Unpin(1);
  BT_CHECK_EQ(pool.FreeFrames(), static_cast<size_t>(2));
  BT_CHECK_EQ(pool.NumPages(), static_cast<size_t>(1));
  BT_CHECK(!pool.HasPage(1));
}

BT_TEST(P7_2, unpin_unknown_page_is_noop) {
  MiniBufferPool pool(3);
  pool.Pin(1);
  pool.Unpin(999);                        // 不在池里
  BT_CHECK_EQ(pool.NumPages(), static_cast<size_t>(1));
  BT_CHECK_EQ(pool.FreeFrames(), static_cast<size_t>(2));
}

BT_TEST(P7_2, unpin_twice_is_noop) {
  MiniBufferPool pool(3);
  pool.Pin(1);
  pool.Unpin(1);
  pool.Unpin(1);                          // 第二次不该多还一个帧
  BT_CHECK_EQ(pool.NumPages(), static_cast<size_t>(0));
  BT_CHECK_EQ(pool.FreeFrames(), static_cast<size_t>(3));
}

BT_TEST(P7_2, fill_all_frames) {
  MiniBufferPool pool(4);
  for (int i = 0; i < 4; ++i) pool.Pin(i);
  BT_CHECK_EQ(pool.NumPages(), static_cast<size_t>(4));
  BT_CHECK_EQ(pool.FreeFrames(), static_cast<size_t>(0));
  for (int i = 0; i < 4; ++i) BT_CHECK(pool.HasPage(i));
}

BT_TEST(P7_2, frame_reuse_after_unpin) {
  MiniBufferPool pool(1);
  Page *a = pool.Pin(1);
  a->data[0] = 5;
  pool.Unpin(1);
  Page *b = pool.Pin(2);
  BT_CHECK(b == a);                       // 只有 1 个帧 → 必然复用
  BT_CHECK_EQ(b->page_id, 2);
  BT_CHECK_EQ(b->data[0], 0);
  pool.Unpin(2);
}

BT_TEST(P7_2, pin_blocks_when_full_then_resumes) {
  MiniBufferPool pool(2);
  pool.Pin(1);
  pool.Pin(2);
  BT_CHECK_EQ(pool.FreeFrames(), static_cast<size_t>(0));

  std::atomic<bool> started{false};
  std::atomic<int> got{-1};
  std::thread t([&] {
    started.store(true);
    Page *p = pool.Pin(3);                // 必须等待
    got.store(p->page_id);
  });

  while (!started.load()) std::this_thread::yield();
  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  BT_CHECK_EQ(got.load(), -1);            // 确实被挡住了，没有返回

  pool.Unpin(1);                          // 释放一个帧 → 唤醒
  t.join();
  BT_CHECK_EQ(got.load(), 3);
  BT_CHECK_EQ(pool.NumPages(), static_cast<size_t>(2));
}

BT_TEST(P7_2, stress_differential_vs_reference_model) {
  // 单线程随机 Pin/Unpin，与一个"朴素集合模型"逐步比对
  std::mt19937 rng(445);
  const size_t kFrames = 8;
  MiniBufferPool pool(kFrames);

  std::set<int> resident;                 // 参考模型：当前驻留页
  size_t free_frames = kFrames;

  for (int step = 0; step < 20000; ++step) {
    const int pid = static_cast<int>(rng() % 50);
    if (rng() % 3 == 0) {                 // Unpin
      pool.Unpin(pid);
      if (resident.erase(pid)) ++free_frames;
    } else {                              // Pin（有空闲帧时才真的去 Pin）
      if (resident.count(pid) == 0) {
        if (free_frames == 0) continue;   // 否则会阻塞，测试里跳过
        resident.insert(pid);
        --free_frames;
      }
      Page *p = pool.Pin(pid);
      BT_CHECK_EQ(p->page_id, pid);
    }
    BT_CHECK_EQ(pool.NumPages(), resident.size());
    BT_CHECK_EQ(pool.FreeFrames(), free_frames);
  }

  // 收尾：全部释放
  for (int pid : std::vector<int>(resident.begin(), resident.end())) pool.Unpin(pid);
  BT_CHECK_EQ(pool.NumPages(), static_cast<size_t>(0));
  BT_CHECK_EQ(pool.FreeFrames(), kFrames);
}

BT_TEST(P7_2, stress_concurrent_workers_drain_pool) {
  const size_t kFrames = 4;
  MiniBufferPool pool(kFrames);
  constexpr int kWorkers = 8;
  std::atomic<int> ok{0};

  std::vector<std::thread> ts;
  for (int w = 0; w < kWorkers; ++w) {
    ts.emplace_back([&pool, &ok, w] {
      for (int i = 0; i < 200; ++i) {
        const int pid = 1000 + w;         // 每个 worker 固定一个页
        Page *p = pool.Pin(pid);
        p->data[i % 8] = i;               // 写点东西
        if (p->page_id == pid) ok.fetch_add(1);
        pool.Unpin(pid);
      }
    });
  }
  for (auto &t : ts) t.join();

  BT_CHECK_EQ(ok.load(), kWorkers * 200);
  BT_CHECK_EQ(pool.NumPages(), static_cast<size_t>(0));    // 全部归还
  BT_CHECK_EQ(pool.FreeFrames(), kFrames);                 // 没有丢帧
}

BT_TEST(P7_2, stress_concurrent_invariant_pages_plus_free) {
  MiniBufferPool pool(6);
  constexpr int kWorkers = 10;
  std::atomic<bool> stop{false};
  std::atomic<int> bad{0};

  // 监视线程：不变量 "驻留页数 + 空闲帧数 == 总帧数" 必须恒成立。
  // 注意必须用 GetStats() 一次取完两个量；分两次读会在两次加锁之间被改状态。
  std::thread monitor([&] {
    while (!stop.load()) {
      const auto st = pool.GetStats();
      if (st.pages + st.free_frames != pool.FrameCount()) bad.fetch_add(1);
    }
  });

  std::vector<std::thread> ts;
  for (int w = 0; w < kWorkers; ++w) {
    ts.emplace_back([&pool, w] {
      for (int i = 0; i < 400; ++i) {
        const int pid = 2000 + (w * 7 + i) % 20;
        Page *p = pool.Pin(pid);
        p->data[0] += 1;
        pool.Unpin(pid);
      }
    });
  }
  for (auto &t : ts) t.join();
  stop.store(true);
  monitor.join();

  BT_CHECK_EQ(bad.load(), 0);
  BT_CHECK_EQ(pool.NumPages(), static_cast<size_t>(0));
  BT_CHECK_EQ(pool.FreeFrames(), pool.FrameCount());
}

BT_TEST(P7_2, stress_single_frame_heavy_contention) {
  // 只有 1 个帧 → 所有线程都在抢，最能暴露 cv 用法错误
  MiniBufferPool pool(1);
  constexpr int kWorkers = 8;
  std::atomic<int> count{0};
  std::vector<std::thread> ts;
  for (int w = 0; w < kWorkers; ++w) {
    ts.emplace_back([&] {
      for (int i = 0; i < 50; ++i) {
        Page *p = pool.Pin(7);
        BT_CHECK_EQ(p->page_id, 7);
        pool.Unpin(7);
        count.fetch_add(1);
      }
    });
  }
  for (auto &t : ts) t.join();
  BT_CHECK_EQ(count.load(), kWorkers * 50);
  BT_CHECK_EQ(pool.FreeFrames(), static_cast<size_t>(1));
}

BT_TEST(P7_2, stale_pointer_caveat_is_real) {
  // 说明本简化版的已知局限：没有 pin count，所以"持有一个 Page*"并不安全。
  MiniBufferPool pool(1);
  Page *held = pool.Pin(1);
  held->data[0] = 111;
  pool.Unpin(1);                          // 帧被回收
  Page *reused = pool.Pin(2);             // 同一个帧被 2 号页复用
  BT_CHECK(held == reused);               // 同一个地址！
  BT_CHECK_EQ(reused->page_id, 2);        // held 现在看到的是 2 号页
  BT_CHECK_EQ(held->data[0], 0);          // 1 号页的数据已经被清掉
  // 真实 buffer pool 需要用 pin count 保证"被 pin 的页不被回收"
  pool.Unpin(2);
}

BT_MAIN("P7.2 Mini BufferPool")
