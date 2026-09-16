// P6.1 线程安全计数器：竞态 → lock/unlock → scoped_lock
#pragma once
#include <mutex>
#include <thread>
#include <vector>

namespace counter6 {

// ---- 反面教材：无锁。把 read / write 拆开并 yield，用于稳定复现丢失更新 ----
struct UnsafeCounter {
  int value = 0;
  void Increment(int iters) {
    for (int i = 0; i < iters; ++i) {
      int tmp = value;              // read
      std::this_thread::yield();    // 放大竞态窗口（否则编译器可能合成一条 RMW 指令）
      value = tmp + 1;              // write
    }
  }
};

// ---- 手写 lock / unlock ----
struct LockedCounter {
  int value = 0;
  std::mutex m;
  void Increment(int iters) {
    for (int i = 0; i < iters; ++i) {
      m.lock();
      value += 1;
      m.unlock();
    }
  }
};

// ---- RAII：scoped_lock ----
struct ScopedCounter {
  int value = 0;
  std::mutex m;
  void Increment(int iters) {
    for (int i = 0; i < iters; ++i) {
      std::scoped_lock lk(m);       // 构造加锁、析构解锁；异常也安全
      value += 1;
    }
  }
};

// ---- 正式封装：const 成员里加锁 → mutex 必须 mutable ----
class Counter {
public:
  void Increment() {
    std::scoped_lock lk(m_);
    ++value_;
  }
  void Add(int n) {
    std::scoped_lock lk(m_);
    value_ += n;
  }
  int Get() const {
    std::scoped_lock lk(m_);        // 改的是锁的状态，不是对象逻辑状态 → m_ 要 mutable
    return value_;
  }
  void Reset() {
    std::scoped_lock lk(m_);
    value_ = 0;
  }

private:
  int value_ = 0;
  mutable std::mutex m_;
};

// 用 nthreads 个线程各跑 iters 次，返回最终计数
inline int RunCounter(Counter &c, int nthreads, int iters) {
  std::vector<std::thread> ts;
  ts.reserve(nthreads);
  for (int t = 0; t < nthreads; ++t) {
    ts.emplace_back([&c, iters] {
      for (int i = 0; i < iters; ++i) c.Increment();
    });
  }
  for (auto &th : ts) th.join();
  return c.Get();
}

}  // namespace counter6
