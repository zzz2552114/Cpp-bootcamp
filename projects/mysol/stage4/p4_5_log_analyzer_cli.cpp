// P4.5 命令行版（用于洛谷式 stdin/stdout 测试点）
//
// 输入格式：
//   N                 日志条数
//   s1 ... sN         每行一个 level（不含空格）
//   Q                 操作数
//   op1 ... opQ       每行一个操作：
//     COUNT <level>     → 输出当前日志中该 level 的出现次数
//     UNIQUE            → 输出所有不同 level，字典序升序，空格分隔（无内容则空行）
//     FILTER <level>    → 输出该 level 的条数
//     REMOVE <level>    → 删除该 level 的所有条目（无输出）
//     PRINT             → 输出当前日志（空格分隔，无内容则空行）
#include <iostream>
#include <string>
#include <vector>

#include "p4_5_log_analyzer.h"

int main() {
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);

  int n = 0;
  if (!(std::cin >> n)) return 0;

  std::vector<std::string> logs;
  logs.reserve(n < 0 ? 0 : static_cast<size_t>(n));
  for (int i = 0; i < n; ++i) {
    std::string s;
    std::cin >> s;
    logs.push_back(s);
  }

  int q = 0;
  std::cin >> q;
  for (int i = 0; i < q; ++i) {
    std::string op;
    std::cin >> op;
    if (op == "COUNT") {
      std::string lv;
      std::cin >> lv;
      std::cout << log5::CountOf(log5::CountLevels(logs), lv) << "\n";
    } else if (op == "UNIQUE") {
      std::cout << log5::Join(log5::UniqueLevels(logs)) << "\n";
    } else if (op == "FILTER") {
      std::string lv;
      std::cin >> lv;
      std::cout << log5::Filter(logs, lv).size() << "\n";
    } else if (op == "REMOVE") {
      std::string lv;
      std::cin >> lv;
      log5::RemoveLevel(logs, lv);
    } else if (op == "PRINT") {
      std::cout << log5::Join(logs) << "\n";
    }
  }
  return 0;
}
