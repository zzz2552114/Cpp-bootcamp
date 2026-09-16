// P6.2 银行转账：多锁死锁与 scoped_lock 规避
#pragma once
#include <mutex>
#include <vector>
#ifdef DEMO_DEADLOCK
#include <thread>
#endif

namespace bank6 {

class Account {
public:
  explicit Account(int money) : balance_(money) {}

  // 正确版：一次按序获取两把锁（scoped_lock 内部用避免死锁的算法）
  void Transfer(Account &other, int money) {
    if (this == &other) return;        // ★ 自转账：否则同一把 mutex 会被 lock 两次
    std::scoped_lock lk(m_, other.m_);
    if (balance_ < money) return;      // 检查余额与扣款必须在同一临界区
    balance_ -= money;
    other.balance_ += money;
  }

  int Balance() const {
    std::scoped_lock lk(m_);
    return balance_;
  }

  void Deposit(int money) {
    std::scoped_lock lk(m_);
    balance_ += money;
  }

#ifdef DEMO_DEADLOCK
  // 反面教材：先锁自己 → 停顿 → 再锁对方。两个线程相反方向时稳定死锁。
  void TransferNaive(Account &other, int money) {
    m_.lock();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    other.m_.lock();                   // ← 卡在这里
    balance_ -= money;
    other.balance_ += money;
    other.m_.unlock();
    m_.unlock();
  }
#endif

private:
  int balance_;
  mutable std::mutex m_;
};

// 用模板接收任意容器（std::vector<Account> 不可用：Account 内含 mutex，不可移动/拷贝）
template <typename Container>
inline long long TotalBalance(const Container &accts) {
  long long s = 0;
  for (const auto &a : accts) s += a.Balance();
  return s;
}

}  // namespace bank6
