// P2.2 模板类 Stack：const& / && 双 Push 重载 + const 版 Top
#pragma once
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

template <typename T>
class Stack {
public:
  // 左值：拷贝入栈（源对象保持不变）
  void Push(const T &value) { data_.push_back(value); }

  // 右值：移动入栈（"吃掉"临时对象）
  void Push(T &&value) { data_.push_back(std::move(value)); }

  void Pop() {
    if (Empty()) throw std::out_of_range("Stack::Pop on empty");
    data_.pop_back();
  }

  T &Top() {
    if (Empty()) throw std::out_of_range("Stack::Top on empty");
    return data_.back();
  }

  const T &Top() const {
    if (Empty()) throw std::out_of_range("Stack::Top on empty");
    return data_.back();
  }

  bool Empty() const { return data_.empty(); }
  size_t Size() const { return data_.size(); }

private:
  std::vector<T> data_;
};
