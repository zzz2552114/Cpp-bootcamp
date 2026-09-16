// P1.5 拷贝 vs 移动：可确定地度量"元素级"代价，另外给出真实计时
#pragma once
#include <cstddef>
#include <string>
#include <vector>

// 元素级拷贝/移动计数器：让"移动不复制元素"这件事可被确定性断言
struct Tracked {
  inline static int copies = 0;
  inline static int moves = 0;
  int v;
  explicit Tracked(int x) : v(x) {}
  Tracked(const Tracked &o) : v(o.v) { ++copies; }
  Tracked(Tracked &&o) noexcept : v(o.v) { ++moves; }
  static void Reset() { copies = 0; moves = 0; }
};

// 生成 n 个 64 字节的长字符串（绕过 SSO，确保有堆分配）
inline std::vector<std::string> MakeLongStrings(size_t n) {
  return std::vector<std::string>(n, std::string(64, 'x'));
}

inline size_t TotalLength(const std::vector<std::string> &v) {
  size_t s = 0;
  for (const auto &x : v) s += x.size();
  return s;
}

// 仅用于打印参考，不作为断言依据
inline long long SumValues(const std::vector<Tracked> &v) {
  long long s = 0;
  for (const auto &x : v) s += x.v;
  return s;
}
