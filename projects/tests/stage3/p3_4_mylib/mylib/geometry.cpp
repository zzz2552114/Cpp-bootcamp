#include "geometry.h"

namespace {
// 匿名命名空间 = 内部链接：只在本翻译单元可见，不会污染外部符号表。
// （比 C 风格的 static 函数更现代，也更适合放常量/辅助类型。）
int HelperAbs(int x) { return x < 0 ? -x : x; }
constexpr int kMagic = 7;
}  // namespace

namespace mylib {
int Add(int a, int b) { return a + b; }
int Sub(int a, int b) { return HelperAbs(a - b); }   // 用了匿名 namespace 的助手
int Abs(int x) { return HelperAbs(x); }
}  // namespace mylib

namespace other {
int Add(int a, int b) { return a + b + kMagic; }     // 同样能用匿名 namespace 里的常量
}  // namespace other
