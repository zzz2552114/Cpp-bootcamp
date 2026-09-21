// P4.5 Log Analyzer：vector + set + unordered_map + lambda + erase-remove
#pragma once
#include <algorithm>
#include <iterator>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace log5 {

using Counts = std::unordered_map<std::string, int>;

// 统计每个 level 出现次数
inline Counts CountLevels(const std::vector<std::string> &logs) {
  Counts counts;
  for (const auto &lvl : logs) ++counts[lvl];
  return counts;
}

// 出现过的 level，按字典序升序（set 天生有序去重）
inline std::set<std::string> UniqueLevels(const std::vector<std::string> &logs) {
  return std::set<std::string>(logs.begin(), logs.end());
}

// 筛出某个 level（保持原顺序）
inline std::vector<std::string> Filter(const std::vector<std::string> &logs,
                                       const std::string &level) {
  std::vector<std::string> out;
  std::copy_if(logs.begin(), logs.end(), std::back_inserter(out),
               [&level](const std::string &l) { return l == level; });
  return out;
}

// 就地删除某 level（erase-remove idiom，保持其余元素相对顺序）
inline void RemoveLevel(std::vector<std::string> &logs, const std::string &level) {
  logs.erase(std::remove_if(logs.begin(), logs.end(),
                            [&level](const std::string &l) { return l == level; }),
             logs.end());
}

inline int CountOf(const Counts &c, const std::string &level) {
  auto it = c.find(level);
  return it == c.end() ? 0 : it->second;
}

inline std::string Join(const std::set<std::string> &s) {
  std::string out;
  bool first = true;
  for (const auto &x : s) {
    if (!first) out += " ";
    first = false;
    out += x;
  }
  return out;
}

inline std::string Join(const std::vector<std::string> &v) {
  std::string out;
  for (size_t i = 0; i < v.size(); ++i) {
    if (i) out += " ";
    out += v[i];
  }
  return out;
}

}  // namespace log5
