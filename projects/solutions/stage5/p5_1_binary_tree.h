// P5.1 UniquePtr 二叉树：ownership tree
#pragma once
#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

namespace tree5 {

struct Node {
  int value;
  std::unique_ptr<Node> left;     // Node 拥有它的左孩子
  std::unique_ptr<Node> right;    // Node 拥有它的右孩子
  explicit Node(int v) : value(v) {}
};

class BinaryTree {
public:
  BinaryTree() = default;
  // 不允许拷贝（unique_ptr 成员自动 =delete），但移动是允许的
  BinaryTree(BinaryTree &&) noexcept = default;
  BinaryTree &operator=(BinaryTree &&) noexcept = default;

  void Insert(int x) { Insert(root_, x); }

  bool Contains(int x) const { return Contains(root_, x); }

  int Height() const { return Height(root_); }     // 空树 = 0

  int Size() const { return Size(root_); }

  std::vector<int> InOrder() const {
    std::vector<int> out;
    InOrder(root_, out);
    return out;
  }

  bool Empty() const { return root_ == nullptr; }
  const Node *Root() const { return root_.get(); }

private:
  // 用 unique_ptr<Node>& 引用定位"该往哪个 slot 放"，插入就是给 slot 赋值
  static void Insert(std::unique_ptr<Node> &slot, int x) {
    if (!slot) {
      slot = std::make_unique<Node>(x);
      return;
    }
    if (x < slot->value) {
      Insert(slot->left, x);
    } else {
      Insert(slot->right, x);        // 重复值一律放右子树
    }
  }

  static bool Contains(const std::unique_ptr<Node> &n, int x) {
    if (!n) return false;
    if (x == n->value) return true;
    return x < n->value ? Contains(n->left, x) : Contains(n->right, x);
  }

  static int Height(const std::unique_ptr<Node> &n) {
    if (!n) return 0;
    return 1 + std::max(Height(n->left), Height(n->right));
  }

  static int Size(const std::unique_ptr<Node> &n) {
    if (!n) return 0;
    return 1 + Size(n->left) + Size(n->right);
  }

  static void InOrder(const std::unique_ptr<Node> &n, std::vector<int> &out) {
    if (!n) return;
    InOrder(n->left, out);
    out.push_back(n->value);
    InOrder(n->right, out);
  }

  std::unique_ptr<Node> root_;   // 整棵树唯一的 owner 链起点
};

}  // namespace tree5
