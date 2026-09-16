// P5.3 SharedPtr 注册表：共享所有权 + use_count
#pragma once
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace reg5 {

struct User {
  inline static int live = 0;
  std::string name_;
  explicit User(std::string n) : name_(std::move(n)) { ++live; }
  ~User() { --live; }
  const std::string &Name() const { return name_; }
};

class UserRegistry {
public:
  void AddUser(int id, std::string name) {
    users_[id] = std::make_shared<User>(std::move(name));
  }

  // 注意：按值返回 → 引用计数 +1
  std::shared_ptr<User> GetUser(int id) {
    auto it = users_.find(id);
    if (it == users_.end()) return nullptr;
    return it->second;
  }

  // 只读版本，便于 const 对象查询
  std::shared_ptr<User> GetUser(int id) const {
    auto it = users_.find(id);
    if (it == users_.end()) return nullptr;
    return it->second;
  }

  // 不增加引用计数的"偷看"（用于测试观察内部计数）
  long PeekUseCount(int id) const {
    auto it = users_.find(id);
    return it == users_.end() ? -1 : it->second.use_count();
  }

  bool RemoveUser(int id) { return users_.erase(id) > 0; }
  size_t Count() const { return users_.size(); }
  bool Empty() const { return users_.empty(); }

private:
  std::unordered_map<int, std::shared_ptr<User>> users_;
};

}  // namespace reg5
