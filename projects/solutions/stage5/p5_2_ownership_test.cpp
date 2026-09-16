// P5.2 测试点
#include <iostream>
#include <string>
#include <memory>
#include <type_traits>
#include <utility>

#include "test_util.h"
#include "p5_2_ownership.h"

using namespace own5;

BT_TEST(P5_2, borrow_does_not_change_ownership) {
  auto up = Make(10);
  const Widget *before = up.get();
  Borrow(up);
  BT_CHECK(up != nullptr);              // 依然持有
  BT_CHECK(up.get() == before);         // 还是同一个对象
  BT_CHECK_EQ(up->v, 11);
}

BT_TEST(P5_2, observe_does_not_change_ownership) {
  auto up = Make(20);
  BT_CHECK_EQ(Observe(up.get()), 20);
  BT_CHECK(up != nullptr);
  BT_CHECK_EQ(up->v, 20);               // 没被改
}

BT_TEST(P5_2, observe_accepts_nullptr) {
  BT_CHECK_EQ(Observe(nullptr), -1);
}

BT_TEST(P5_2, move_transfers_ownership_and_empties_source) {
  auto up = Make(30);
  auto up2 = Take(std::move(up));
  BT_CHECK(up == nullptr);              // 源已交出
  BT_CHECK(up2 != nullptr);
  BT_CHECK_EQ(up2->v, 30);
}

BT_TEST(P5_2, take_by_value_destroys_at_end) {
  const int live0 = Widget::live;
  {
    auto up = Make(40);
    BT_CHECK_EQ(Widget::live, live0 + 1);
    BT_CHECK_EQ(TakeAndDestroy(std::move(up)), 40);
    BT_CHECK(up == nullptr);
    BT_CHECK_EQ(Widget::live, live0);   // 被调用者收下后在其作用域结束释放
  }
  BT_CHECK_EQ(Widget::live, live0);
}

BT_TEST(P5_2, unique_ptr_is_not_copyable) {
  static_assert(!std::is_copy_constructible_v<std::unique_ptr<Widget>>);
  static_assert(std::is_move_constructible_v<std::unique_ptr<Widget>>);
  BT_CHECK(true);
}

BT_TEST(P5_2, make_returns_fresh_ownership) {
  const int live0 = Widget::live;
  {
    auto a = Make(1);
    auto b = Make(2);
    BT_CHECK_EQ(Widget::live, live0 + 2);
    BT_CHECK(a.get() != b.get());
  }
  BT_CHECK_EQ(Widget::live, live0);
}

BT_TEST(P5_2, release_gives_up_ownership) {
  auto up = Make(50);
  Widget *raw = up.release();            // 放弃所有权
  BT_CHECK(up == nullptr);
  BT_CHECK(raw != nullptr);
  BT_CHECK_EQ(raw->v, 50);
  BT_CHECK_EQ(Widget::live, 1);
  delete raw;                            // 现在由你负责删
  BT_CHECK_EQ(Widget::live, 0);
}

BT_TEST(P5_2, reset_releases_old_and_takes_new) {
  const int live0 = Widget::live;
  auto up = Make(1);
  const Widget *first = up.get();
  up.reset(new Widget(2));               // 释放 v=1，接管 v=2
  BT_CHECK(up->v == 2);
  BT_CHECK(up.get() != first);
  BT_CHECK_EQ(Widget::live, live0 + 1);
  up.reset();                            // 释放，变空
  BT_CHECK(up == nullptr);
  BT_CHECK_EQ(Widget::live, live0);
}

BT_TEST(P5_2, reset_get_is_a_double_free_hazard_use_release_first) {
  // ⚠️ up.reset(up.get()) 会 double free！见 p5_2_ownership.h 的注释。
  // 正确做法：先 release 把所有权交出来（old 变 nullptr），再 reset。
  const int live0 = Widget::live;
  auto up = Make(7);
  Widget *raw = up.get();
  ResetKeepingSamePointer(up);
  BT_CHECK(up.get() == raw);             // 还是同一个对象
  BT_CHECK_EQ(up->v, 7);
  BT_CHECK_EQ(Widget::live, live0 + 1);  // 只被删过一次 → 还活着
}

BT_TEST(P5_2, reset_to_nullptr_releases) {
  const int live0 = Widget::live;
  auto up = Make(8);
  up.reset();                            // 等价于 up.reset(nullptr)
  BT_CHECK(up == nullptr);
  BT_CHECK_EQ(Widget::live, live0);
}

BT_TEST(P5_2, reset_to_different_pointer_deletes_old) {
  const int live0 = Widget::live;
  auto up = Make(1);
  Widget *first = up.get();
  up.reset(new Widget(2));
  BT_CHECK(up.get() != first);
  BT_CHECK_EQ(up->v, 2);
  BT_CHECK_EQ(Widget::live, live0 + 1);
}

BT_TEST(P5_2, bool_conversion) {
  std::unique_ptr<Widget> empty;
  auto full = Make(9);
  BT_CHECK(!empty);
  BT_CHECK(static_cast<bool>(full));
  if (full) BT_CHECK_EQ(full->v, 9);
  BT_CHECK_EQ(empty ? 1 : 0, 0);
}

BT_TEST(P5_2, no_leak_over_many_ownership_transfers) {
  const int live0 = Widget::live;
  {
    std::unique_ptr<Widget> cur = Make(0);
    for (int i = 1; i < 2000; ++i) {
      cur = Take(std::move(cur));        // 反复移交：任何一处漏掉都会泄漏
    }
    BT_CHECK(cur != nullptr);
  }
  BT_CHECK_EQ(Widget::live, live0);
}

BT_TEST(P5_2, widget_counter_is_balanced_after_all_tests) {
  BT_CHECK_EQ(Widget::live, 0);          // 前面所有测试都没有泄漏
}

namespace {
// 只在显式传 --demo-reset-self 时运行：会 double free（预期 abort）
void DemoResetSelf() {
  std::cerr << "[demo] up.reset(up.get()) —— 标准语义会先 delete 掉 old，再留下悬垂指针\n"
            << std::flush;
  auto up = Make(7);
  up.reset(up.get());
  std::cerr << "[demo] 走到这里说明没有崩（未定义行为，不要依赖）\n";
}
}  // namespace

int main(int argc, char **argv) {
  if (argc > 1 && std::string(argv[1]) == "--demo-reset-self") {
    DemoResetSelf();
    return 0;
  }
  return bt::RunAll("P5.2 所有权传参（借用 / 非拥有 / 移交）");
}
