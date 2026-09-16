// P4.4 auto / decltype / 结构化绑定
#pragma once
#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace a4 {

// 带"拷贝计数"的类型：用来证明 auto 会静默拷贝、而 const auto& 不会
struct Big {
  inline static int copies = 0;
  int v = 0;
  Big() = default;
  explicit Big(int x) : v(x) {}
  Big(const Big &o) : v(o.v) { ++copies; }
  Big &operator=(const Big &o) { v = o.v; ++copies; return *this; }
  static void Reset() { copies = 0; }
};

// decltype(auto)：完全按表达式的类型来（vector::operator[] 返回 int& → 这里也是 int&）
template <typename C> decltype(auto) AtRef(C &c, size_t i) { return c[i]; }

// auto：按值返回，引用被剥掉 → 返回 int（拷贝）
template <typename C> auto AtVal(C &c, size_t i) { return c[i]; }

// 结构化绑定：auto& 能改到 map 的 value
inline std::string ScaleValues(std::map<std::string, int> &m, int factor) {
  for (auto &[k, v] : m) v *= factor;              // k 类型是 const std::string
  std::string out;
  for (const auto &[k, v] : m) out += k + std::to_string(v);
  return out;
}

// 结构化绑定遍历 pair 的 vector
inline int SumPairs(const std::vector<std::pair<int, int>> &v) {
  int s = 0;
  for (const auto &[a, b] : v) s += a + b;
  return s;
}

}  // namespace a4
