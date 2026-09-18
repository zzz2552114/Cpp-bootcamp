// test_util.h —— Bootcamp 精简版测试框架
//
// 设计目标：把每个 Project 拆成若干"分级测试点"，可运行、可计分、失败能看到原因。
// 用法：
//   #include "test_util.h"
//   BT_TEST(名字) { BT_CHECK(...); BT_CHECK_EQ(a, b); BT_CHECK_THROWS(expr, ExType); }
//   BT_MAIN("Test Suite test_util.h")
//
// 编译：g++ -std=c++17 -pthread -I <solutions目录> xxx_test.cpp -o xxx && ./xxx
#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace bt {

// 测试失败时抛出，由 RunAll 捕获并报告。
struct Failure {
  std::string msg;
};

struct TestCase {
  std::string name;
  std::function<void()> fn;
};

inline std::vector<TestCase> &Registry() {
  static std::vector<TestCase> r;
  return r;
}

// 全局注册器：BT_TEST 会在静态初始化期把自己塞进 Registry。
struct Registrar {
  Registrar(const std::string &name, std::function<void()> fn) {
    Registry().push_back({name, std::move(fn)});
  }
};

inline int RunAll(const std::string &suite) {
  int passed = 0, failed = 0, idx = 0;
  std::cout << "================ " << suite << " ================\n";
  for (auto &tc : Registry()) {
    ++idx;
    std::ostringstream label;
    label << "[" << (idx < 10 ? "  " : (idx < 100 ? " " : "")) << idx << "] ";
    try {
      tc.fn();
      std::cout << label.str() << "PASS  " << tc.name << "\n";
      ++passed;
    } catch (const Failure &f) {
      std::cout << label.str() << "FAIL  " << tc.name << "\n"
                << "        " << f.msg << "\n";
      ++failed;
    } catch (const std::exception &e) {
      std::cout << label.str() << "FAIL  " << tc.name << "\n"
                << "        Unexpected exception: " << e.what() << "\n";
      ++failed;
    } catch (...) {
      std::cout << label.str() << "FAIL  " << tc.name << "\n"
                << "        Unexpected unknown exception\n";
      ++failed;
    }
  }
  int total = passed + failed;
  std::cout << "---------------- Result: " << passed << "/" << total << " Passed";
  if (failed) std::cout << "  (" << failed << " Failed)";
  std::cout << " ----------------\n";
  return failed == 0 ? 0 : 1;
}

// BT_CHECK_EQ 的实参打印（要求可流式输出 + 可 ==）
template <typename A, typename B>
inline void CheckEqImpl(const A &a, const B &b, const char *sa, const char *sb, int line) {
  if (!(a == b)) {
    std::ostringstream oss;
    oss << "CHECK_EQ Failed: " << sa << " == " << sb << "  [Left=" << a << ", Right=" << b
        << "]  (Line " << line << ")";
    throw Failure{oss.str()};
  }
}

template <typename A, typename B>
inline void CheckNeImpl(const A &a, const B &b, const char *sa, const char *sb, int line) {
  if (a == b) {
    std::ostringstream oss;
    oss << "CHECK_NE Failed: " << sa << " != " << sb << "  [Both are " << a << "]  (Line " << line << ")";
    throw Failure{oss.str()};
  }
}

// 期待抛出 ExType；抛了别的类型或没抛都算失败。
template <typename ExType, typename Fn>
inline void CheckThrowsImpl(Fn fn, const char *expr, int line) {
  try {
    fn();
  } catch (const ExType &) {
    return;  // 正确
  } catch (const std::exception &e) {
    std::ostringstream oss;
    oss << "CHECK_THROWS Failed: " << expr << " threw wrong exception (what=" << e.what()
        << ")  (Line " << line << ")";
    throw Failure{oss.str()};
  } catch (...) {
    std::ostringstream oss;
    oss << "CHECK_THROWS Failed: " << expr << " threw non-standard exception  (Line " << line << ")";
    throw Failure{oss.str()};
  }
  std::ostringstream oss;
  oss << "CHECK_THROWS Failed: " << expr << " did not throw any exception  (Line " << line << ")";
  throw Failure{oss.str()};
}

}  // namespace bt

// BT_TEST(suite, name) —— suite 形如 P1_1，name 是测试点名。
// 拼接成合法标识符 suite##_##name，显示名则是 "P1_1/name"。
#define BT_TEST(suite, name)                                              \
  static void suite##_##name();                                           \
  static ::bt::Registrar bt_reg_##suite##_##name(#suite "/" #name,        \
                                                 suite##_##name);         \
  static void suite##_##name()

#define BT_CHECK(cond)                                                       \
  do {                                                                       \
    if (!(cond)) {                                                           \
      std::ostringstream _bt_oss;                                            \
      _bt_oss << "CHECK Failed: " #cond "  (Line " << __LINE__ << ")";       \
      throw ::bt::Failure{_bt_oss.str()};                                    \
    }                                                                        \
  } while (0)

#define BT_CHECK_EQ(a, b) ::bt::CheckEqImpl((a), (b), #a, #b, __LINE__)
#define BT_CHECK_NE(a, b) ::bt::CheckNeImpl((a), (b), #a, #b, __LINE__)
#define BT_CHECK_THROWS(expr, ExType) \
  ::bt::CheckThrowsImpl<ExType>([&] { (void)(expr); }, #expr, __LINE__)

#define BT_MAIN(suite)  \
  int main() { return ::bt::RunAll(suite); }
