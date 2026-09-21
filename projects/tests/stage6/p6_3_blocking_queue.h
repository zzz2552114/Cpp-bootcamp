// P6.3 有界阻塞队列：条件变量 + 关闭协议
#pragma once
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <utility>

namespace bq6 {

class ClosedError : public std::runtime_error {
public:
  ClosedError() : std::runtime_error("BlockingQueue is closed") {}
};

template <typename T>
class BlockingQueue {
public:
  explicit BlockingQueue(size_t capacity) : capacity_(capacity) {
    assert(capacity >= 1);
  }

  // 队列满则等待；已关闭则抛出
  void Push(T value) {
    std::unique_lock<std::mutex> lk(m_);
    // 带谓词的 wait：既防虚假唤醒，也防"通知丢失"
    cv_not_full_.wait(lk, [this] { return closed_ || q_.size() < capacity_; });
    if (closed_) throw ClosedError();
    q_.push(std::move(value));
    lk.unlock();                       // 先解锁再 notify（避免被唤醒者立刻又阻塞）
    cv_not_empty_.notify_one();
  }

  // 队列空则等待；已关闭且空则抛出
  T Pop() {
    std::unique_lock<std::mutex> lk(m_);
    cv_not_empty_.wait(lk, [this] { return closed_ || !q_.empty(); });
    if (q_.empty()) throw ClosedError();     // 到这里只有可能是"已关闭且空"
    T v = std::move(q_.front());
    q_.pop();
    lk.unlock();
    cv_not_full_.notify_one();
    return v;
  }

  // 关闭：唤醒所有等待者，让它们看到 closed_ 并退出
  void Close() {
    {
      std::lock_guard<std::mutex> lk(m_);
      closed_ = true;
    }
    cv_not_empty_.notify_all();
    cv_not_full_.notify_all();
  }

  bool Closed() const {
    std::lock_guard<std::mutex> lk(m_);
    return closed_;
  }
  size_t Size() const {
    std::lock_guard<std::mutex> lk(m_);
    return q_.size();
  }
  size_t Capacity() const { return capacity_; }

private:
  std::queue<T> q_;
  size_t capacity_;
  bool closed_ = false;
  mutable std::mutex m_;
  std::condition_variable cv_not_empty_;
  std::condition_variable cv_not_full_;
};

}  // namespace bq6
