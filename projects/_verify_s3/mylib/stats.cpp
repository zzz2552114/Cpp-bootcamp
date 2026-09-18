#include "stats.h"

namespace mylib {
long long Sum(const std::vector<int> &v) {
  long long s = 0;
  for (int x : v) s += x;                    // 累加到 64 位，不会溢出
  return s;
}

double Average(const std::vector<int> &v) {
  if (v.empty()) return 0.0;
  return static_cast<double>(Sum(v)) / static_cast<double>(v.size());
}
}  // namespace mylib
