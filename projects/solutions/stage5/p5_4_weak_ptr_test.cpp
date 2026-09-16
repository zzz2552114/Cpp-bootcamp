// P5.4 测试点
#include <iostream>
#include <memory>
#include <string>

#include "test_util.h"
#include "p5_4_weak_ptr.h"

using namespace weak5;

BT_TEST(P5_4, shared_ptr_cycle_leaks) {
  ResetLive();
  std::shared_ptr<BadNode> a, b;
  BadNode *ra = nullptr, *rb = nullptr;
  {
    a = std::make_shared<BadNode>("A");
    b = std::make_shared<BadNode>("B");
    a->next = b;                     // A → B
    b->next = a;                     // B → A   ← 成环
    BT_CHECK_EQ(a.use_count(), 2);   // 外部 a + B 的 next
    BT_CHECK_EQ(b.use_count(), 2);
    ra = a.get();
    rb = b.get();
  }
  a.reset();
  b.reset();                         // 外部的两个引用都放掉了
  BT_CHECK_EQ(g_live, 2);            // ★ 但对象仍然活着：环内互相持有 → 泄漏

  // 手工断环，把内存清掉（否则这个进程会一直泄漏）。
  // ★ 只需断开【一条】边：A→B 一断，B 的强引用数归零 → B 析构 → B→A 也释放 → A 析构。
  //   如果再写一句 rb->next.reset()，那时 B 已经被析构，就是对已释放内存的访问（ASan 会报）。
  ra->next.reset();
  BT_CHECK_EQ(g_live, 0);            // 一条边断掉，两个对象级联析构
  (void)rb;
  ResetLive();
}

BT_TEST(P5_4, weak_ptr_breaks_the_cycle) {
  ResetLive();
  {
    auto a = std::make_shared<GoodNode>("X");
    auto b = std::make_shared<GoodNode>("Y");
    a->next = b;                     // 强引用
    b->prev = a;                     // 弱引用：不增加 A 的计数
    BT_CHECK_EQ(a.use_count(), 1);   // 只有外部 a
    BT_CHECK_EQ(b.use_count(), 2);   // 外部 b + A 的 next
    BT_CHECK_EQ(g_live, 2);
  }
  BT_CHECK_EQ(g_live, 0);            // ★ 两个都正常析构，没有泄漏
  ResetLive();
}

BT_TEST(P5_4, weak_ptr_does_not_increase_use_count) {
  ResetLive();
  auto p = std::make_shared<GoodNode>("Z");
  BT_CHECK_EQ(p.use_count(), 1);
  std::weak_ptr<GoodNode> w = p;
  BT_CHECK_EQ(p.use_count(), 1);     // weak 不增加计数
  BT_CHECK_EQ(w.use_count(), 1);     // 观察到的还是强引用数
  BT_CHECK(!w.expired());
  ResetLive();
}

BT_TEST(P5_4, weak_ptr_lock_returns_shared_ptr) {
  ResetLive();
  auto p = std::make_shared<GoodNode>("L");
  std::weak_ptr<GoodNode> w = p;
  {
    auto locked = w.lock();
    BT_CHECK(locked != nullptr);
    BT_CHECK_EQ(locked->name, std::string("L"));
    BT_CHECK_EQ(p.use_count(), 2);   // lock 期间多一个强引用
  }
  BT_CHECK_EQ(p.use_count(), 1);
  ResetLive();
}

BT_TEST(P5_4, weak_ptr_expires_when_object_dies) {
  ResetLive();
  std::weak_ptr<GoodNode> w;
  {
    auto p = std::make_shared<GoodNode>("D");
    w = p;
    BT_CHECK(!w.expired());
  }
  BT_CHECK(w.expired());             // 对象已析构
  BT_CHECK(w.lock() == nullptr);     // lock 返回空
  BT_CHECK_EQ(w.use_count(), 0);
  ResetLive();
}

BT_TEST(P5_4, weak_ptr_on_empty) {
  std::weak_ptr<GoodNode> w;
  BT_CHECK(w.expired());
  BT_CHECK(w.lock() == nullptr);
  BT_CHECK_EQ(w.use_count(), 0);
}

BT_TEST(P5_4, weak_ptr_reset) {
  ResetLive();
  auto p = std::make_shared<GoodNode>("R");
  std::weak_ptr<GoodNode> w = p;
  BT_CHECK(!w.expired());
  w.reset();
  BT_CHECK(w.expired());
  BT_CHECK_EQ(p.use_count(), 1);     // 不影响强引用
  ResetLive();
}

BT_TEST(P5_4, parent_child_pattern_no_leak) {
  ResetLive();
  {
    auto parent = std::make_shared<Parent>("P");
    auto child = std::make_shared<Child>("C");
    parent->child = child;           // 父拥有子
    child->parent = parent;          // 子弱引用父
    BT_CHECK_EQ(parent.use_count(), 1);
    BT_CHECK_EQ(child.use_count(), 2);

    // 从子访问父
    if (auto p = child->parent.lock()) {
      BT_CHECK_EQ(p->name, std::string("P"));
      BT_CHECK_EQ(p->child->name, std::string("C"));
    } else {
      BT_CHECK(false);
    }
    BT_CHECK_EQ(g_live, 2);
  }
  BT_CHECK_EQ(g_live, 0);
  ResetLive();
}

BT_TEST(P5_4, child_outlives_parent_weak_ref_expires) {
  ResetLive();
  std::shared_ptr<Child> child;
  {
    auto parent = std::make_shared<Parent>("P2");
    child = std::make_shared<Child>("C2");
    parent->child = child;
    child->parent = parent;
  }
  // parent 已析构；child 被外部 hold 着
  BT_CHECK_EQ(g_live, 1);
  BT_CHECK(child->parent.expired());       // 弱引用正确过期
  BT_CHECK(child->parent.lock() == nullptr);
  child.reset();
  BT_CHECK_EQ(g_live, 0);
  ResetLive();
}

BT_TEST(P5_4, many_cycles_cleaned_up) {
  ResetLive();
  for (int k = 0; k < 2000; ++k) {
    auto a = std::make_shared<GoodNode>("a");
    auto b = std::make_shared<GoodNode>("b");
    a->next = b;
    b->prev = a;
    BT_CHECK_EQ(a.use_count(), 1);
  }
  BT_CHECK_EQ(g_live, 0);
  ResetLive();
}

BT_MAIN("P5.4 weak_ptr 与循环引用")
