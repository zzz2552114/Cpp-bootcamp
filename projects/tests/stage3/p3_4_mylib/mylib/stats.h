#pragma once
#include <vector>
namespace mylib {
double Average(const std::vector<int> &v);
long long Sum(const std::vector<int> &v);    // 64 位，避免溢出
}  // namespace mylib
