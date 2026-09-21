// P4.1 vector：迭代器失效 / reserve vs resize
#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace v4 {

struct CapSize {
  size_t cap;
  size_t size;
  bool operator==(const CapSize &o) const { return cap == o.cap && size == o.size; }
};
inline std::ostream &operator<<(std::ostream &os, const CapSize &c) {
  return os << "{cap=" << c.cap << ",size=" << c.size << "}";
}

inline CapSize PushNTimes(size_t n) {
  std::vector<int> v;
  for (size_t i = 0; i < n; ++i) v.push_back(static_cast<int>(i));
  return {v.capacity(), v.size()};
}

inline CapSize ReserveThenPush(size_t reserve_n, size_t push_n) {
  std::vector<int> v;
  v.reserve(reserve_n);
  for (size_t i = 0; i < push_n; ++i) v.push_back(static_cast<int>(i));
  return {v.capacity(), v.size()};
}

// 把地址当整数比，避免"比较悬垂指针"的语义争议
inline uintptr_t DataAddr(const std::vector<int> &v) {
  return reinterpret_cast<uintptr_t>(v.data());
}

// 先 reserve(cap)，取一次地址，再 push 到 fill（不超过 cap），返回地址是否不变
inline bool AddressStableWithinCapacity(size_t cap, size_t fill) {
  std::vector<int> v;
  v.reserve(cap);
  const uintptr_t before = DataAddr(v);
  for (size_t i = 0; i < fill; ++i) v.push_back(static_cast<int>(i));
  return DataAddr(v) == before;
}

// 不 reserve，push 到超过初始 capacity，返回地址是否变了（发生了重分配）
inline bool AddressChangesPastCapacity(size_t exceed) {
  std::vector<int> v;
  const uintptr_t before = DataAddr(v);
  for (size_t i = 0; i < exceed; ++i) v.push_back(static_cast<int>(i));
  return DataAddr(v) != before;
}

}  // namespace v4
