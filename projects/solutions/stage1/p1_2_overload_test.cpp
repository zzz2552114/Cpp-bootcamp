// P1.2 测试点：每种实参类别会选中哪个重载
#include <iostream>
#include <utility>

#include "test_util.h"
#include "p1_2_overload.h"

BT_TEST(P1_2, nonconst_lvalue_picks_T_ref) {
  int a = 1;
  BT_CHECK_EQ(Which(a), std::string("lvalue ref"));
}

BT_TEST(P1_2, const_variable_picks_const_ref) {
  const int c = 2;
  BT_CHECK_EQ(Which(c), std::string("const lvalue ref"));
}

BT_TEST(P1_2, literal_picks_rvalue_ref) {
  BT_CHECK_EQ(Which(10), std::string("rvalue ref"));
}

BT_TEST(P1_2, temporary_object_picks_rvalue_ref) {
  BT_CHECK_EQ(Which(1 + 2), std::string("rvalue ref"));
}

BT_TEST(P1_2, std_move_makes_lvalue_pick_rvalue_ref) {
  int a = 1;
  BT_CHECK_EQ(Which(std::move(a)), std::string("rvalue ref"));
  BT_CHECK(a == 1);   // std::move 只是 cast，没有真的动 a
}

BT_TEST(P1_2, const_ref_view_picks_const_ref) {
  int a = 1;
  BT_CHECK_EQ(CallWithConst(a), std::string("const lvalue ref"));
}

BT_TEST(P1_2, after_move_source_is_still_an_lvalue) {
  int a = 1;
  (void)Which(std::move(a));
  BT_CHECK_EQ(Which(a), std::string("lvalue ref"));   // a 本身仍是左值
}

BT_TEST(P1_2, const_rvalue_picks_const_ref) {
  const int c = 5;
  // std::move(c) 得到 const int&&，无法绑定到 int&&，只能退到 const int&
  BT_CHECK_EQ(Which(std::move(c)), std::string("const lvalue ref"));
}

BT_MAIN("P1.2 重载解析（lvalue / const lvalue / rvalue）")
