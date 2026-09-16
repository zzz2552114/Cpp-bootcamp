// P7.1 手写 MyUniquePtr：template + RAII + move + 运算符重载
#pragma once
#include <cstddef>
#include <utility>

namespace myptr7 {

template <typename T>
class MyUniquePtr {
public:
  constexpr MyUniquePtr() noexcept = default;
  explicit MyUniquePtr(T *ptr) noexcept : ptr_(ptr) {}

  ~MyUniquePtr() { delete ptr_; }

  // 移动构造：接管指针并把源置空
  MyUniquePtr(MyUniquePtr &&other) noexcept : ptr_(other.ptr_) { other.ptr_ = nullptr; }

  // 移动赋值：先释放自己的旧资源，再接管
  MyUniquePtr &operator=(MyUniquePtr &&other) noexcept {
    if (this == &other) return *this;        // self-move 防护
    Reset(other.Release());
    return *this;
  }

  // 独占所有权 → 禁止拷贝
  MyUniquePtr(const MyUniquePtr &) = delete;
  MyUniquePtr &operator=(const MyUniquePtr &) = delete;

  T &operator*() const { return *ptr_; }
  T *operator->() const { return ptr_; }
  T *Get() const noexcept { return ptr_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }

  // 交出裸指针，自己变空（之后由调用者负责 delete）
  T *Release() noexcept {
    T *p = ptr_;
    ptr_ = nullptr;
    return p;
  }

  // 删除旧的、接管新的。
  // ★ 这里比 std::unique_ptr::reset 更安全：显式做了 `p == ptr_` 的短路。
  //   标准库的 reset 语义是"先记下 old，设 ptr=p，再无条件 delete old"，
  //   所以对标准库写 up.reset(up.get()) 会 double free（详见 P5.2）。
  void Reset(T *p = nullptr) noexcept {
    if (p == ptr_) return;                   // 同一指针：什么都不做
    delete ptr_;
    ptr_ = p;
  }

  void Swap(MyUniquePtr &other) noexcept { std::swap(ptr_, other.ptr_); }

private:
  T *ptr_ = nullptr;
};

// 变参 + 完美转发（等价于 std::make_unique）
template <typename T, typename... Args>
MyUniquePtr<T> MyMakeUnique(Args &&...args) {
  return MyUniquePtr<T>(new T(std::forward<Args>(args)...));
}

}  // namespace myptr7
