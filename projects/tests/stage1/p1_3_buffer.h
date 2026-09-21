// P1.3 MoveOnlyBuffer：拥有堆内存、删除拷贝、只可移动
#pragma once
#include <cstddef>
#include <stdexcept>

class Buffer {
public:
  // 分配/释放计数：让测试能验证"没有泄漏、没有重复释放"
  inline static int allocs = 0;
  inline static int frees = 0;

  explicit Buffer(size_t n) : size_(n) {
    data_ = new int[n]();      // () 进行值初始化 → 全部为 0
    ++allocs;
  }

  ~Buffer() { Release(); }     // delete[] nullptr 是安全的空操作

  // 移动构造：仅仅"偷"走对方的指针，不做任何元素拷贝
  Buffer(Buffer &&other) noexcept : data_(other.data_), size_(other.size_) {
    other.data_ = nullptr;     // 关键：置空源对象
    other.size_ = 0;
  }

  // 移动赋值：先释放自己的旧资源，再接管对方的
  Buffer &operator=(Buffer &&other) noexcept {
    if (this == &other) return *this;   // self-move 防护（原计划漏掉，会导致悬垂指针）
    Release();                          // 1. 释放旧资源（否则泄漏）
    data_ = other.data_;                // 2. 接管
    size_ = other.size_;
    other.data_ = nullptr;              // 3. 掏空对方
    other.size_ = 0;
    return *this;
  }

  // 一个资源只能有一个 owner → 禁止拷贝
  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;

  size_t Size() const noexcept { return size_; }
  bool Owns() const noexcept { return data_ != nullptr; }   // 区分"零长 buffer"和"被移动走的 buffer"

  int &operator[](size_t i) { return data_[i]; }              // 不检查（像 std::vector）
  const int &operator[](size_t i) const { return data_[i]; }

  int &At(size_t i) {                                          // 检查越界，便于测试
    if (i >= size_) throw std::out_of_range("Buffer::At");
    return data_[i];
  }
  const int &At(size_t i) const {
    if (i >= size_) throw std::out_of_range("Buffer::At");
    return data_[i];
  }

private:
  void Release() noexcept {
    if (data_) {
      delete[] data_;
      ++frees;
      data_ = nullptr;
    }
  }

  int *data_ = nullptr;
  size_t size_ = 0;
};
