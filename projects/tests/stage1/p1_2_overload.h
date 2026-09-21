// P1.2 重载解析三兄弟：f(T&) / f(const T&) / f(T&&)
// 为了让测试能"看到"选中的是哪个重载，这里返回标签字符串（而不是打印）。
#pragma once
#include <string>

inline std::string Which(int &) { return "lvalue ref"; }
inline std::string Which(const int &) { return "const lvalue ref"; }
inline std::string Which(int &&) { return "rvalue ref"; }

// 用于测试：拿一个"const 左值"的辅助函数
inline std::string CallWithConst(int &x) {
  const int &cx = x;      // cx 是 const 左值
  return Which(cx);
}
