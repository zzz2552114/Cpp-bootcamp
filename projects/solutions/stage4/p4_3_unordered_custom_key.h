// P4.3 unordered_map：operator[] 陷阱 + 自定义键的 std::hash
#pragma once
#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>

namespace u4 {

// pair 作 key：pair 自带 operator==，只需补一个 hash
struct PairHash {
  size_t operator()(const std::pair<int, int> &p) const {
    const size_t h1 = std::hash<int>{}(p.first);
    const size_t h2 = std::hash<int>{}(p.second);
    return h1 ^ (h2 << 1);              // 简单组合（生产环境可用更均匀的混合）
  }
};
using PairMap = std::unordered_map<std::pair<int, int>, std::string, PairHash>;

// 自定义结构作 key：需要 operator== + std::hash 特化
struct Point {
  int x, y;
  bool operator==(const Point &o) const { return x == o.x && y == o.y; }
};

}  // namespace u4

namespace std {
template <> struct hash<u4::Point> {
  size_t operator()(const u4::Point &p) const noexcept {
    return std::hash<int>{}(p.x) ^ (std::hash<int>{}(p.y) << 1);
  }
};
}  // namespace std

namespace u4 {

inline std::unordered_map<std::string, int> MakeBasicMap() {
  std::unordered_map<std::string, int> m;
  m.insert({"foo", 2});
  m.insert(std::make_pair("jignesh", 445));
  m.insert({{"spam", 1}, {"eggs", 2}});
  return m;
}

}  // namespace u4
