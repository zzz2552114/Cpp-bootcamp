// P6.2 测试点
//
// 想复现死锁（会永久挂起，用 Ctrl-C 或 timeout 结束）：
//   g++ -std=c++17 -pthread -DDEMO_DEADLOCK ... && ./a.out --demo-deadlock
#include <chrono>
#include <deque>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "test_util.h"
#include "p6_2_bank.h"

using namespace bank6;

namespace {
// 注意：std::thread 会【拷贝】可调用对象。如果 lambda 里用 `[t = 0]() mutable { t++ }`
// 想给每个线程一个不同编号，那个编号会被复制 8 份、每个线程都从 0 开始 —— 典型的坑。
// 所以凡是需要"每线程不同编号"的地方，都用下面这个显式传 index 的版本。
template <typename F> void RunThreads(int n, F fn) {
  std::vector<std::thread> ts;
  ts.reserve(n);
  for (int i = 0; i < n; ++i) ts.emplace_back(fn);
  for (auto &t : ts) t.join();
}
template <typename F> void RunThreadsIdx(int n, F fn) {
  std::vector<std::thread> ts;
  ts.reserve(n);
  for (int i = 0; i < n; ++i) ts.emplace_back([&fn, i] { fn(i); });
  for (auto &t : ts) t.join();
}
}  // namespace

#ifdef DEMO_DEADLOCK
namespace {
void DemoDeadlock() {
  std::cerr << "[demo] Two threads lock in opposite order; deadlock expected "
               "(press Ctrl-C to quit)...\n"
            << std::flush;
  Account a(1000), b(1000);
  std::thread t1([&] { for (int i = 0; i < 100; ++i) a.TransferNaive(b, 1); });
  std::thread t2([&] { for (int i = 0; i < 100; ++i) b.TransferNaive(a, 1); });
  t1.join();
  t2.join();
  std::cerr << "[demo] No deadlock happened?!\n";
}
}  // namespace
#endif

BT_TEST(P6_2, basic_transfer) {
  Account a(100), b(50);
  a.Transfer(b, 30);
  BT_CHECK_EQ(a.Balance(), 70);
  BT_CHECK_EQ(b.Balance(), 80);
}

BT_TEST(P6_2, transfer_preserves_total) {
  Account a(100), b(200);
  a.Transfer(b, 30);
  BT_CHECK_EQ(a.Balance() + b.Balance(), 300);
}

BT_TEST(P6_2, insufficient_funds_is_noop) {
  Account a(10), b(0);
  a.Transfer(b, 999);
  BT_CHECK_EQ(a.Balance(), 10);      // 没变
  BT_CHECK_EQ(b.Balance(), 0);
}

BT_TEST(P6_2, exact_balance_transfer_works) {
  Account a(10), b(0);
  a.Transfer(b, 10);
  BT_CHECK_EQ(a.Balance(), 0);
  BT_CHECK_EQ(b.Balance(), 10);
}

BT_TEST(P6_2, self_transfer_is_safe_noop) {
  Account a(100);
  a.Transfer(a, 10);                 // 手动双锁会死锁，scoped_lock 版安全
  BT_CHECK_EQ(a.Balance(), 100);
}

BT_TEST(P6_2, zero_amount_transfer) {
  Account a(100), b(0);
  a.Transfer(b, 0);
  BT_CHECK_EQ(a.Balance(), 100);
  BT_CHECK_EQ(b.Balance(), 0);
}

BT_TEST(P6_2, concurrent_transfers_two_accounts_conserve_total) {
  Account a(10000), b(10000);
  RunThreads(2, [&, i = 0]() mutable {
    if (i++ % 2 == 0) {
      for (int k = 0; k < 50000; ++k) a.Transfer(b, 1);
    } else {
      for (int k = 0; k < 50000; ++k) b.Transfer(a, 1);
    }
  });
  BT_CHECK_EQ(a.Balance() + b.Balance(), 20000);
  BT_CHECK(a.Balance() >= 0);
  BT_CHECK(b.Balance() >= 0);
}

BT_TEST(P6_2, concurrent_transfers_many_accounts_conserve_total) {
  const int kAccounts = 4;
  std::deque<Account> acc;      // Account 内含 mutex → 不可移动，只能用 deque
  for (int i = 0; i < kAccounts; ++i) acc.emplace_back(10000);
  const long long before = TotalBalance(acc);

  RunThreadsIdx(4, [&](int idx) {
    Account &from = acc[idx % kAccounts];
    Account &to = acc[(idx + 1) % kAccounts];
    for (int k = 0; k < 100000; ++k) from.Transfer(to, 1);
  });

  BT_CHECK_EQ(TotalBalance(acc), before);
  for (const auto &a : acc) BT_CHECK(a.Balance() >= 0);
}

BT_TEST(P6_2, concurrent_transfers_with_self_transfers) {
  Account a(5000), b(5000);
  const long long before = a.Balance() + b.Balance();
  RunThreadsIdx(4, [&](int idx) {
    for (int k = 0; k < 20000; ++k) {
      if (idx % 2 == 0) a.Transfer(b, 1);
      else a.Transfer(a, 1);          // 混入自转账
    }
  });
  BT_CHECK_EQ(a.Balance() + b.Balance(), before);
}

BT_TEST(P6_2, random_transfer_pattern_conserves_total) {
  std::mt19937 rng(7);
  const int kAccounts = 5;
  std::deque<Account> acc;
  for (int i = 0; i < kAccounts; ++i) acc.emplace_back(1000);
  const long long before = TotalBalance(acc);

  RunThreadsIdx(8, [&](int seed) {
    std::mt19937 r(seed + 12345);
    for (int k = 0; k < 20000; ++k) {
      int i = static_cast<int>(r() % kAccounts);
      int j = static_cast<int>(r() % kAccounts);
      acc[i].Transfer(acc[j], static_cast<int>(r() % 5));
    }
  });

  BT_CHECK_EQ(TotalBalance(acc), before);
  for (const auto &a : acc) BT_CHECK(a.Balance() >= 0);
}

BT_TEST(P6_2, deposit_is_thread_safe) {
  Account a(0);
  RunThreads(8, [&] { for (int i = 0; i < 20000; ++i) a.Deposit(1); });
  BT_CHECK_EQ(a.Balance(), 160000);
}

BT_TEST(P6_2, stress_repeated_rounds) {
  for (int rep = 0; rep < 15; ++rep) {
    Account a(1000), b(1000);
    RunThreads(4, [&, t = 0]() mutable {
      const int idx = t++;
      for (int k = 0; k < 10000; ++k) {
        if (idx % 2 == 0) a.Transfer(b, 1);
        else b.Transfer(a, 1);
      }
    });
    BT_CHECK_EQ(a.Balance() + b.Balance(), 2000);
  }
}

int main(int argc, char **argv) {
#ifdef DEMO_DEADLOCK
  if (argc > 1 && std::string(argv[1]) == "--demo-deadlock") {
    DemoDeadlock();
    return 0;
  }
#endif
  (void)argc; (void)argv;
  return bt::RunAll("P6.2 Bank Transfer & Multi-lock Deadlock");
}
