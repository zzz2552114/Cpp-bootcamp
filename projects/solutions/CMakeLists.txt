# Bootcamp 解答：一次编译全部测试点
#
#   cmake -S . -B build && cmake --build build -j
#   ctest --test-dir build --output-on-failure      # 或直接跑 build/xxx_test
cmake_minimum_required(VERSION 3.14)
project(bootcamp_solutions CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

find_package(Threads REQUIRED)
enable_testing()

# 编译器相关警告选项（MSVC / 其它分开写，保证 Windows 上也能构建）
if(MSVC)
  set(BT_WARN /W4 /permissive-)
else()
  set(BT_WARN -Wall -Wextra -Wno-unused-parameter)
endif()

# 单文件测试目标
function(bt_single name)
  add_executable(${name} ${ARGN})
  target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(${name} PRIVATE Threads::Threads)
  target_compile_options(${name} PRIVATE ${BT_WARN})
  add_test(NAME ${name} COMMAND ${name})
endfunction()

# 多文件测试目标（额外源文件由调用方列出）
function(bt_multi name)
  add_executable(${name} ${ARGN})
  target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(${name} PRIVATE Threads::Threads)
  target_compile_options(${name} PRIVATE ${BT_WARN})
  add_test(NAME ${name} COMMAND ${name})
endfunction()

# ---------------- Stage 1 ----------------
bt_single(p1_1_statistics_test stage1/p1_1_statistics_test.cpp)
bt_single(p1_2_overload_test   stage1/p1_2_overload_test.cpp)
bt_single(p1_3_buffer_test     stage1/p1_3_buffer_test.cpp)
bt_single(p1_4_noexcept_test   stage1/p1_4_noexcept_test.cpp)
bt_single(p1_5_bench_test      stage1/p1_5_bench_test.cpp)

# ---------------- Stage 2 ----------------
bt_single(p2_1_functions_test      stage2/p2_1_functions_test.cpp)
bt_single(p2_2_stack_test          stage2/p2_2_stack_test.cpp)
bt_single(p2_3_specialization_test stage2/p2_3_specialization_test.cpp)
bt_multi(p2_4_minimath_test
         stage2/p2_4_minimath/p2_4_minimath_test.cpp
         stage2/p2_4_minimath/minimath/min.cpp)

# ---------------- Stage 3 ----------------
bt_single(p3_1_handle_test   stage3/p3_1_handle_test.cpp)
bt_single(p3_2_iterator_test stage3/p3_2_iterator_test.cpp)
bt_single(p3_3_rule_of_three_five_zero_test     stage3/p3_3_rule_of_three_five_zero_test.cpp)
bt_multi(p3_4_mylib_test
         stage3/p3_4_mylib/p3_4_mylib_test.cpp
         stage3/p3_4_mylib/mylib/geometry.cpp
         stage3/p3_4_mylib/mylib/stats.cpp)

# ---------------- Stage 4 ----------------
bt_single(p4_1_vector_invalidation_test    stage4/p4_1_vector_invalidation_test.cpp)
bt_single(p4_2_set_test       stage4/p4_2_set_comparator_test.cpp)
bt_single(p4_3_unordered_custom_key_test stage4/p4_3_unordered_custom_key_test.cpp)
bt_single(p4_4_auto_decltype_test      stage4/p4_4_auto_decltype_test.cpp)
bt_single(p4_5_log_analyzer_test       stage4/p4_5_log_analyzer_test.cpp)
bt_single(p4_5_log_analyzer_cli stage4/p4_5_log_analyzer_cli.cpp)

# ---------------- Stage 5 ----------------
bt_single(p5_1_binary_tree_test      stage5/p5_1_binary_tree_test.cpp)
bt_single(p5_2_ownership_test stage5/p5_2_ownership_test.cpp)
bt_single(p5_3_registry_test  stage5/p5_3_registry_test.cpp)
bt_single(p5_4_weak_ptr_test      stage5/p5_4_weak_ptr_test.cpp)

# ---------------- Stage 6 ----------------
bt_single(p6_1_counter_test stage6/p6_1_counter_test.cpp)
bt_single(p6_2_bank_test    stage6/p6_2_bank_test.cpp)
bt_single(p6_3_blocking_queue_test      stage6/p6_3_blocking_queue_test.cpp)
bt_single(p6_4_rwlock_map_test      stage6/p6_4_rwlock_map_test.cpp)

# 可选：死锁复现（默认不构建；-DBT_BUILD_DEMOS=ON）
option(BT_BUILD_DEMOS "构建会崩溃/挂起的演示程序" OFF)
if(BT_BUILD_DEMOS)
  add_executable(demo_deadlock stage6/p6_2_bank_test.cpp)
  target_compile_definitions(demo_deadlock PRIVATE DEMO_DEADLOCK)
  target_include_directories(demo_deadlock PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(demo_deadlock PRIVATE Threads::Threads)
  set_target_properties(demo_deadlock PROPERTIES OUTPUT_NAME p6_2_bank_deadlock)
endif()

# ---------------- Stage 7 ----------------
bt_single(p7_1_my_unique_ptr_test      stage7/p7_1_my_unique_ptr_test.cpp)
bt_single(p7_2_mini_buffer_pool_test stage7/p7_2_mini_buffer_pool_test.cpp)

# ---------------- 洛谷式 stdin/stdout 测试点（P4.5） ----------------
# run_tests.sh 需要 bash；Windows 上没有 bash 就跳过这个 test（用 make check 或手动跑）
find_program(BT_BASH bash)
if(BT_BASH)
  add_test(NAME p4_5_luogu_cases
           COMMAND ${BT_BASH}
                   ${CMAKE_CURRENT_SOURCE_DIR}/stage4/tests/run_tests.sh
                   $<TARGET_FILE:p4_5_log_analyzer_cli>)
else()
  message(STATUS "未找到 bash：跳过 p4_5_luogu_cases 测试（可手动运行 stage4/tests/run_tests.sh）")
endif()
