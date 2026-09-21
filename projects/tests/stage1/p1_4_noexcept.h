// P1.4 noexcept 为什么不能省略：vector 扩容时的 move_if_noexcept
#pragma once
#include <vector>

// A. 移动构造带 noexcept
struct Counted {
  inline static int copies = 0;
  inline static int moves = 0;
  int v;
  explicit Counted(int x) : v(x) {}
  Counted(const Counted &o) : v(o.v) { ++copies; }
  Counted(Counted &&o) noexcept : v(o.v) { ++moves; }
  static void Reset() { copies = 0; moves = 0; }
};

// B. 移动构造【没有】noexcept（拷贝仍可用）
struct CountedThrowy {
  inline static int copies = 0;
  inline static int moves = 0;
  int v;
  explicit CountedThrowy(int x) : v(x) {}
  CountedThrowy(const CountedThrowy &o) : v(o.v) { ++copies; }
  CountedThrowy(CountedThrowy &&o) : v(o.v) { ++moves; }   // 故意不写 noexcept
  static void Reset() { copies = 0; moves = 0; }
};

// C. 移动构造没 noexcept，但拷贝被删除 → 只能移动
struct MoveOnlyThrowy {
  inline static int moves = 0;
  int v;
  explicit MoveOnlyThrowy(int x) : v(x) {}
  MoveOnlyThrowy(const MoveOnlyThrowy &) = delete;
  MoveOnlyThrowy &operator=(const MoveOnlyThrowy &) = delete;
  MoveOnlyThrowy(MoveOnlyThrowy &&o) : v(o.v) { ++moves; }  // 同样没 noexcept
  static void Reset() { moves = 0; }
};

// 不 reserve，逼 vector 反复扩容，从而观察它对已有元素做了什么
template <typename T> void GrowVector(int n) {
  std::vector<T> v;
  for (int i = 0; i < n; ++i) v.push_back(T(i));
}
