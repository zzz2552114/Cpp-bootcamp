// P2.1 模板函数：编译期按类型生成代码 + 类型推导
#pragma once
#include <cstddef>
#include <type_traits>
#include <utility>

// 同一个模板在不同 T 上会生成【不同】的函数
template <typename T> T Min(const T &a, const T &b) { return a < b ? a : b; }
template <typename T> T Max(const T &a, const T &b) { return a < b ? b : a; }

// 编译期可用（constexpr）：能出现在 static_assert 里
template <typename T> constexpr T MinC(const T &a, const T &b) { return a < b ? a : b; }

// 非类型模板参数：把"长度"这个值带进类型里
template <typename T, size_t N> constexpr size_t ArrayLen(const T (&)[N]) { return N; }

// 演示"两个参数的 T 必须推成同一个类型"：用 void_t 做 SFINAE 探测
template <typename A, typename B, typename = void> struct CanDeduceMin : std::false_type {};
template <typename A, typename B>
struct CanDeduceMin<A, B, std::void_t<decltype(Min(std::declval<A>(), std::declval<B>()))>>
    : std::true_type {};
