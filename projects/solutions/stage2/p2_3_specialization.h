// P2.3 模板特化 + 非类型模板参数 + constexpr if
#pragma once
#include <cstddef>
#include <string>
#include <type_traits>

// ---------- 1. 函数模板特化 ----------
template <typename T> std::string Describe(const T &) { return "default"; }
template <> std::string Describe<double>(const double &) { return "double"; }
// 显式特化时形参类型要写成"T = const char* 时的形参"，即 const char* const&
template <> std::string Describe<const char *>(const char *const &) { return "c-string"; }

// ---------- 2. 类模板特化 ----------
template <typename T> struct Printer {
  static std::string Name() { return "generic"; }
};
template <> struct Printer<float> {
  static std::string Name() { return "float"; }
};
template <> struct Printer<std::string> {
  static std::string Name() { return "string"; }
};

// ---------- 3. 非类型模板参数 ----------
template <size_t N> struct FixedArray {
  int a[N] = {};                              // 默认全 0
  constexpr size_t Size() const { return N; }
};
template <int T> struct Constant {
  static constexpr int value = T;
};

// 编译期递归：Factorial<N>() 在编译期算完
template <size_t N> constexpr size_t Factorial() { return N * Factorial<N - 1>(); }
template <> constexpr size_t Factorial<0>() { return 1; }

// ---------- 4. constexpr if：把分支选择搬到编译期 ----------
template <typename T> std::string ToString(const T &v) {
  if constexpr (std::is_integral_v<T>) {
    return std::to_string(v);
  } else if constexpr (std::is_floating_point_v<T>) {
    return std::to_string(v);
  } else {
    return "non-numeric";                     // 这个分支对整型/浮点根本不会被实例化
  }
}
