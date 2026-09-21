// P4.2 set：有序、去重、自定义比较器 / map 词频
#pragma once
#include <functional>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace s4 {

// 升序去重
inline std::set<int> SortedUnique(const std::vector<int> &v) {
  return std::set<int>(v.begin(), v.end());
}

// 用 set 按序输出（去重后的）
inline std::string JoinAscending(const std::set<int> &s) {
  std::string out;
  bool first = true;
  for (int x : s) {
    if (!first) out += " ";
    first = false;
    out += std::to_string(x);
  }
  return out;
}

// 自定义比较器：先按两数之和升序；和相等时按 pair 自然序，保证"严格弱序"
struct SumCmp {
  bool operator()(const std::pair<int, int> &a, const std::pair<int, int> &b) const {
    const int sa = a.first + a.second;
    const int sb = b.first + b.second;
    if (sa != sb) return sa < sb;
    return a < b;
  }
};

inline std::string JoinBySumOrder(const std::set<std::pair<int, int>, SumCmp> &s) {
  std::string out;
  bool first = true;
  for (const auto &p : s) {
    if (!first) out += " ";
    first = false;
    out += "(" + std::to_string(p.first) + "," + std::to_string(p.second) + ")";
  }
  return out;
}

// map 词频（key 有序）
inline std::map<std::string, int> WordFreq(const std::vector<std::string> &words) {
  std::map<std::string, int> freq;
  for (const auto &w : words) ++freq[w];
  return freq;
}

inline std::string KeysJoined(const std::map<std::string, int> &m) {
  std::string out;
  for (const auto &kv : m) out += kv.first;
  return out;
}

}  // namespace s4
