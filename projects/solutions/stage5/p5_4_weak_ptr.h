// P5.4 weak_ptr 打破循环引用
#pragma once
#include <memory>
#include <string>
#include <utility>

namespace weak5 {

inline int g_live = 0;
inline void ResetLive() { g_live = 0; }

// 全用 shared_ptr：A→B→A 成环，谁都放不掉
struct BadNode {
  std::string name;
  std::shared_ptr<BadNode> next;
  explicit BadNode(std::string n) : name(std::move(n)) { ++g_live; }
  ~BadNode() { --g_live; }
};

// 用 weak_ptr 剪断一条边：强引用只朝一个方向
struct GoodNode {
  std::string name;
  std::shared_ptr<GoodNode> next;      // 拥有后继（强）
  std::weak_ptr<GoodNode> prev;        // 指回前任（弱，不增加计数）
  explicit GoodNode(std::string n) : name(std::move(n)) { ++g_live; }
  ~GoodNode() { --g_live; }
};

// 父子结构：父拥有子，子弱引用父（典型用法）
struct Child;
struct Parent {
  std::string name;
  std::shared_ptr<Child> child;
  explicit Parent(std::string n) : name(std::move(n)) { ++g_live; }
  ~Parent() { --g_live; }
};
struct Child {
  std::string name;
  std::weak_ptr<Parent> parent;        // 弱引用 → 不构成环
  explicit Child(std::string n) : name(std::move(n)) { ++g_live; }
  ~Child() { --g_live; }
};

}  // namespace weak5
