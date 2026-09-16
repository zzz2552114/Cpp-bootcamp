#pragma once          // include guard：同一个 TU 里重复 include 也安全
namespace mylib {
int Add(int a, int b);
int Sub(int a, int b);      // 内部用匿名命名空间的助手实现
int Abs(int x);
}  // namespace mylib

// 另一个命名空间里放"同名函数"，证明命名空间能消除冲突
namespace other {
int Add(int a, int b);      // 与 mylib::Add 同名但不同标识符
}  // namespace other
