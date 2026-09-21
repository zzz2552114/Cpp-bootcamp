// P5.2 所有权传入函数：借用 / 非拥有 / 移交
#pragma once
#include <memory>

namespace own5 {

struct Widget {
  inline static int live = 0;
  int v;
  explicit Widget(int x) : v(x) { ++live; }
  ~Widget() { --live; }
};

// (1) 借用：只改对象，不动所有权
inline void Borrow(std::unique_ptr<Widget> &up) {
  if (up) up->v += 1;
}

// (2) 非拥有裸指针：绝不 delete
inline int Observe(const Widget *raw) { return raw ? raw->v : -1; }

// (3) 交出所有权
inline std::unique_ptr<Widget> Take(std::unique_ptr<Widget> up) { return up; }

// (4) 返回新所有权
inline std::unique_ptr<Widget> Make(int v) { return std::make_unique<Widget>(v); }

// (5) 安全地"重置到同一个指针"的写法。
//     ⚠️ 千万不要写 up.reset(up.get())！
//     标准里 reset(p) 的语义是：先记下 old = get()，把 get() 设为 p，
//     然后"若 old 非空就 delete old"——它【不比较 old 和 p】。
//     所以 reset(get()) 会把还握在手里的对象删掉，随后析构再删一次 → double free。
//     （历史上 LWG 806 曾经讨论过带 `p == get()` 短路的老措辞，但现行措辞已移除。）
//     安全写法：先把所有权交出来（release），此时 old 为 nullptr，再 reset。
inline void ResetKeepingSamePointer(std::unique_ptr<Widget> &up) {
  Widget *owned = up.release();       // up 变空；owns 仍在 owned 里
  up.reset(owned);                    // old 是 nullptr → 不会误删
}

// 按值收下并销毁（示范"移交后由被调用者负责"）
inline int TakeAndDestroy(std::unique_ptr<Widget> up) {
  return up ? up->v : -1;
}

}  // namespace own5
