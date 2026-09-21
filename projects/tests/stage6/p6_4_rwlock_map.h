// P6.4 读写锁并发 Map：shared_mutex / shared_lock / unique_lock
#pragma once
#include <cstddef>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <unordered_map>

namespace rw6 {

class ConcurrentMap {
public:
  void Put(int key, int value) {
    std::unique_lock<std::shared_mutex> lk(m_);      // 写：独占
    map_[key] = value;
  }

  bool Get(int key, int &value) const {
    std::shared_lock<std::shared_mutex> lk(m_);      // 读：共享
    auto it = map_.find(key);
    if (it == map_.end()) return false;
    value = it->second;
    return true;
  }

  std::optional<int> Get(int key) const {
    int v = 0;
    if (!Get(key, v)) return std::nullopt;
    return v;
  }

  bool Remove(int key) {
    std::unique_lock<std::shared_mutex> lk(m_);
    return map_.erase(key) > 0;
  }

  bool Contains(int key) const {
    std::shared_lock<std::shared_mutex> lk(m_);
    return map_.find(key) != map_.end();
  }

  size_t Size() const {
    std::shared_lock<std::shared_mutex> lk(m_);
    return map_.size();
  }

  void Clear() {
    std::unique_lock<std::shared_mutex> lk(m_);
    map_.clear();
  }

private:
  std::unordered_map<int, int> map_;
  mutable std::shared_mutex m_;    // const 成员里要加锁 → mutable
};

}  // namespace rw6
