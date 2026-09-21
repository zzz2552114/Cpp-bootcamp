#include "min.h"

namespace minimath {

int Add(int a, int b) { return a + b; }

// 显式实例化：在本 TU 里提前把 Min<int> 生成出来。
// 用途：当你想把模板实现藏进 .cpp、只对固定几种类型供外部使用时。
template int Min<int>(const int &, const int &);

}  // namespace minimath
