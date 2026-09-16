#pragma once   // include guard：防止同一 TU 里重复展开
#include <cstddef>
#include <string>

namespace minimath {

// 模板的定义必须对每个使用它的翻译单元可见 —— 所以实现写在头文件里。
// 如果"声明放 .h、实现放 .cpp"，链接时会 undefined reference to Min<int>。
template <typename T> T Min(const T &a, const T &b) { return a < b ? a : b; }

// 普通函数：声明放头文件、实现放 .cpp 即可（不涉及模板实例化）
int Add(int a, int b);

// 用于演示 CTAD：有构造函数，编译器才能从实参推导出 T
template <typename T> class Box {
public:
  explicit Box(T v) : v_(v) {}
  const T &Get() const { return v_; }

private:
  T v_;
};

}  // namespace minimath
