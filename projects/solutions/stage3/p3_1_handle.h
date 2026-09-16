// P3.1 RAII 资源句柄 + RVO / copy elision
#pragma once
#include <utility>

class Handle {
public:
  // 统计量：让测试能验证"无泄漏"和"RVO 时零移动"
  inline static int live = 0;      // 当前存活的资源数
  inline static int acquires = 0;
  inline static int releases = 0;
  inline static int moves = 0;

  explicit Handle(int id) : id_(id), valid_(true) {
    ++live;
    ++acquires;
  }

  ~Handle() {
    if (valid_) {                  // moved-from 对象不再释放
      --live;
      ++releases;
    }
  }

  // 一个资源一个 owner → 禁止拷贝
  Handle(const Handle &) = delete;
  Handle &operator=(const Handle &) = delete;

  Handle(Handle &&other) noexcept : id_(other.id_), valid_(true) {
    other.valid_ = false;          // 关键：把源标记为无效
    ++moves;
  }

  Handle &operator=(Handle &&other) noexcept {
    if (this == &other) return *this;         // self-move 防护
    if (valid_) { --live; ++releases; }       // 释放自己旧资源
    id_ = other.id_;
    valid_ = true;
    other.valid_ = false;
    ++moves;
    return *this;
  }

  int Id() const { return id_; }
  bool Valid() const { return valid_; }

  static void ResetStats() { acquires = releases = moves = 0; }

private:
  int id_;
  bool valid_;
};

// 按值返回：C++17 保证 copy elision → 调用处不会发生移动
inline Handle MakeHandle(int id) { return Handle(id); }
