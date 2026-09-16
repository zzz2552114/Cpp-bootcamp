// P4.5 测试点（含随机对拍：与"线性暴力"实现比对）
#include <iostream>
#include <random>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "test_util.h"
#include "p4_5_log_analyzer.h"

using namespace log5;

namespace {
const std::vector<std::string> kSample{"INFO", "ERROR", "INFO", "WARN", "ERROR", "INFO"};

// ---------- 暴力参考实现（完全独立于 log5::，只做线性扫描） ----------
int BruteCount(const std::vector<std::string> &logs, const std::string &lv) {
  int n = 0;
  for (const auto &l : logs) if (l == lv) ++n;
  return n;
}
std::set<std::string> BruteUnique(const std::vector<std::string> &logs) {
  std::set<std::string> s;
  for (const auto &l : logs) s.insert(l);
  return s;
}
void BruteRemove(std::vector<std::string> &logs, const std::string &lv) {
  std::vector<std::string> keep;
  for (const auto &l : logs) if (l != lv) keep.push_back(l);
  logs.swap(keep);
}
}  // namespace

BT_TEST(P4_5, count_levels_basic) {
  auto c = CountLevels(kSample);
  BT_CHECK_EQ(CountOf(c, "INFO"), 3);
  BT_CHECK_EQ(CountOf(c, "ERROR"), 2);
  BT_CHECK_EQ(CountOf(c, "WARN"), 1);
  BT_CHECK_EQ(c.size(), static_cast<size_t>(3));
}

BT_TEST(P4_5, count_levels_empty) {
  auto c = CountLevels({});
  BT_CHECK(c.empty());
  BT_CHECK_EQ(CountOf(c, "INFO"), 0);
}

BT_TEST(P4_5, count_levels_missing_key_is_zero) {
  auto c = CountLevels(kSample);
  BT_CHECK_EQ(CountOf(c, "DEBUG"), 0);      // 注意：用 find，不能 operator[]
  BT_CHECK_EQ(c.size(), static_cast<size_t>(3));
}

BT_TEST(P4_5, unique_levels_is_sorted) {
  BT_CHECK_EQ(Join(UniqueLevels(kSample)), std::string("ERROR INFO WARN"));
}

BT_TEST(P4_5, unique_levels_empty) {
  BT_CHECK_EQ(Join(UniqueLevels({})), std::string(""));
}

BT_TEST(P4_5, unique_levels_single) {
  BT_CHECK_EQ(Join(UniqueLevels({"A"})), std::string("A"));
}

BT_TEST(P4_5, filter_keeps_order_and_count) {
  auto errs = Filter(kSample, "ERROR");
  BT_CHECK_EQ(errs.size(), static_cast<size_t>(2));
  BT_CHECK_EQ(Join(errs), std::string("ERROR ERROR"));
}

BT_TEST(P4_5, filter_nonexistent_level) {
  BT_CHECK(Filter(kSample, "DEBUG").empty());
}

BT_TEST(P4_5, filter_all) {
  auto all = Filter(kSample, "INFO");
  BT_CHECK_EQ(all.size(), static_cast<size_t>(3));
}

BT_TEST(P4_5, remove_level_keeps_relative_order) {
  auto logs = kSample;
  RemoveLevel(logs, "INFO");
  BT_CHECK_EQ(Join(logs), std::string("ERROR WARN ERROR"));
  BT_CHECK_EQ(logs.size(), static_cast<size_t>(3));
}

BT_TEST(P4_5, remove_nonexistent_level_is_noop) {
  auto logs = kSample;
  RemoveLevel(logs, "DEBUG");
  BT_CHECK_EQ(logs.size(), kSample.size());
  BT_CHECK_EQ(Join(logs), Join(kSample));
}

BT_TEST(P4_5, remove_all_levels_empties) {
  auto logs = kSample;
  for (const auto &lv : UniqueLevels(kSample)) RemoveLevel(logs, lv);
  BT_CHECK(logs.empty());
}

BT_TEST(P4_5, remove_is_idempotent) {
  auto logs = kSample;
  RemoveLevel(logs, "ERROR");
  const size_t after_first = logs.size();
  RemoveLevel(logs, "ERROR");
  BT_CHECK_EQ(logs.size(), after_first);
}

BT_TEST(P4_5, lambda_capture_by_ref_vs_value) {
  std::string level = "INFO";
  auto by_ref = [&level](const std::string &l) { return l == level; };
  auto by_val = [level](const std::string &l) { return l == level; };
  level = "ERROR";                          // 改掉外层变量
  BT_CHECK(by_ref("ERROR"));                // 引用捕获：看到新值
  BT_CHECK(!by_ref("INFO"));
  BT_CHECK(by_val("INFO"));                 // 值捕获：保留旧值
  BT_CHECK(!by_val("ERROR"));
}

BT_TEST(P4_5, large_count_1e5) {
  std::vector<std::string> logs;
  logs.reserve(100000);
  const char *lv[] = {"A", "B", "C", "D"};
  for (int i = 0; i < 100000; ++i) logs.emplace_back(lv[i % 4]);
  auto c = CountLevels(logs);
  BT_CHECK_EQ(c.size(), static_cast<size_t>(4));
  BT_CHECK_EQ(CountOf(c, "A"), 25000);
  BT_CHECK_EQ(CountOf(c, "D"), 25000);
  BT_CHECK_EQ(Join(UniqueLevels(logs)), std::string("A B C D"));
}

// ---------------- 对拍：随机操作 vs 暴力参考实现 ----------------
BT_TEST(P4_5, stress_differential_vs_brute_force) {
  std::mt19937 rng(20240645);
  const char *levels[] = {"INFO", "WARN", "ERROR", "DEBUG", "TRACE"};
  const int kLevels = 5;

  for (int iter = 0; iter < 200; ++iter) {
    const int n = static_cast<int>(rng() % 60);
    std::vector<std::string> logs, brute;
    for (int i = 0; i < n; ++i) {
      std::string lv = levels[rng() % kLevels];
      logs.push_back(lv);
      brute.push_back(lv);
    }

    // 随机若干次操作，每步都与暴力实现比对
    const int ops = 1 + static_cast<int>(rng() % 8);
    for (int o = 0; o < ops; ++o) {
      const int kind = static_cast<int>(rng() % 4);
      const std::string lv = levels[rng() % kLevels];

      switch (kind) {
        case 0: {   // COUNT
          auto c = CountLevels(logs);
          BT_CHECK_EQ(CountOf(c, lv), BruteCount(brute, lv));
          break;
        }
        case 1: {   // UNIQUE
          BT_CHECK(Join(UniqueLevels(logs)) == Join(BruteUnique(brute)));
          break;
        }
        case 2: {   // FILTER 条数
          BT_CHECK_EQ(Filter(logs, lv).size(),
                      static_cast<size_t>(BruteCount(brute, lv)));
          break;
        }
        case 3: {   // REMOVE
          RemoveLevel(logs, lv);
          BruteRemove(brute, lv);
          BT_CHECK_EQ(Join(logs), Join(brute));
          break;
        }
      }
    }
    BT_CHECK_EQ(logs.size(), brute.size());
  }
}

BT_TEST(P4_5, stress_random_medium_size) {
  std::mt19937 rng(7);
  const char *levels[] = {"INFO", "WARN", "ERROR"};
  std::vector<std::string> logs, brute;
  for (int i = 0; i < 20000; ++i) {
    std::string lv = levels[rng() % 3];
    logs.push_back(lv);
    brute.push_back(lv);
  }
  for (int k = 0; k < 3; ++k) {
    const std::string lv = levels[k];
    BT_CHECK_EQ(CountOf(CountLevels(logs), lv), BruteCount(brute, lv));
    RemoveLevel(logs, lv);
    BruteRemove(brute, lv);
  }
  BT_CHECK(logs.empty());
  BT_CHECK(brute.empty());
}

BT_MAIN("P4.5 Log Analyzer")
