// P1.1 引用游乐场：引用是别名、const&、const 成员函数
#pragma once
#include <cstddef>
#include <vector>

struct Statistics {
  // 统计拷贝次数，用来在测试里证明"传 const& 不拷贝"。
  inline static int copies = 0;

  std::vector<int> data;                 // 故意 public，方便观察

  Statistics() = default;
  Statistics(const Statistics &o) : data(o.data) { ++copies; }   // 拷贝才计数
  Statistics &operator=(const Statistics &o) { data = o.data; ++copies; return *this; }
  Statistics(Statistics &&) noexcept = default;                  // 移动不计数
  Statistics &operator=(Statistics &&) noexcept = default;

  // 修改对象本身：必须是非 const 成员
  void AddValue(int x) { data.push_back(x); }

  // 只读：const 成员函数，编译器会检查它没改任何成员。
  // ⚠️ 陷阱：返回类型是 int，元素多/值大时会【有符号溢出】（UB）。
  //    真实项目里优先用下面的 64 位版本。
  int Sum() const {
    int s = 0;
    for (int v : data) s += v;
    return s;
  }

  // 64 位版本：同样只读，但不会溢出（除非数据本身超过 2^63）
  long long SumL() const {
    long long s = 0;
    for (int v : data) s += v;
    return s;
  }

  // 只读；空数据返回 0.0，避免除零。
  // 用 SumL 而不是 Sum，避免"求和先溢出、再算平均"。
  double Average() const {
    if (data.empty()) return 0.0;
    // 注意：必须转成 double，否则 size() 是 size_t、会发生整数除法
    return static_cast<double>(SumL()) / static_cast<double>(data.size());
  }

  size_t Size() const { return data.size(); }
  bool Empty() const { return data.empty(); }
};

// 通过 const& 借用：不拷贝、不修改
inline int ReportSum(const Statistics &s) { return s.Sum(); }
