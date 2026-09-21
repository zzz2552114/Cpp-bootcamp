#!/usr/bin/env python3
# 独立生成 .in 与 .ans。
# 关键：.ans 用这份 Python 的朴素实现算出来，而不是用 C++ 程序自己算，
#       所以它是对 C++ 实现的真正"对拍/校验"。
import random, os

HERE = os.path.dirname(os.path.abspath(__file__))

def brute_ans(logs, ops):
    out = []
    for op in ops:
        p = op.split()
        if p[0] == "COUNT":
            out.append(str(logs.count(p[1])))
        elif p[0] == "UNIQUE":
            out.append(" ".join(sorted(set(logs))))
        elif p[0] == "FILTER":
            out.append(str(logs.count(p[1])))
        elif p[0] == "REMOVE":
            logs = [x for x in logs if x != p[1]]
        elif p[0] == "PRINT":
            out.append(" ".join(logs))
    return "\n".join(out) + ("\n" if out else "")

def write_case(name, logs, ops):
    inp = [str(len(logs))] + logs + [str(len(ops))] + ops
    with open(os.path.join(HERE, name + ".in"), "w") as f:
        f.write("\n".join(inp) + "\n")
    with open(os.path.join(HERE, name + ".ans"), "w") as f:
        f.write(brute_ans(list(logs), list(ops)))
    print(f"  {name}.in  N={len(logs):<7} Q={len(ops)}")

# 手写小样例
write_case("case01_basic",
           ["INFO", "ERROR", "INFO", "WARN", "ERROR", "INFO"],
           ["COUNT INFO", "COUNT ERROR", "COUNT WARN", "COUNT DEBUG", "UNIQUE",
            "FILTER ERROR", "PRINT"])
write_case("case02_single", ["INFO"], ["COUNT INFO", "UNIQUE", "PRINT", "REMOVE INFO", "PRINT"])
write_case("case03_empty", [], ["UNIQUE", "PRINT", "COUNT INFO", "FILTER INFO"])
write_case("case04_all_same", ["A"] * 5,
           ["COUNT A", "UNIQUE", "REMOVE A", "COUNT A", "PRINT", "UNIQUE"])
write_case("case05_remove_then_count",
           ["A", "B", "A", "C", "B", "A"],
           ["COUNT A", "REMOVE A", "COUNT A", "UNIQUE", "PRINT", "REMOVE B",
            "PRINT", "COUNT C", "UNIQUE"])
write_case("case06_remove_only_element",
           ["X"], ["PRINT", "REMOVE X", "PRINT", "UNIQUE", "COUNT X"])
write_case("case07_remove_nonexistent",
           ["A", "B"], ["REMOVE Z", "PRINT", "UNIQUE", "COUNT Z"])
write_case("case08_duplicate_removes",
           ["A", "A", "B"], ["REMOVE A", "REMOVE A", "COUNT A", "PRINT", "UNIQUE"])

# 随机中等规模
rnd = random.Random(12345)
LV = ["INFO", "WARN", "ERROR", "DEBUG", "TRACE"]
def rand_ops(q, lv):
    ops = []
    for _ in range(q):
        k = rnd.randrange(5)
        if k == 0: ops.append("COUNT " + rnd.choice(lv))
        elif k == 1: ops.append("UNIQUE")
        elif k == 2: ops.append("FILTER " + rnd.choice(lv))
        elif k == 3: ops.append("REMOVE " + rnd.choice(lv))
        else: ops.append("PRINT")
    return ops

write_case("case09_random_small",
           [rnd.choice(LV[:3]) for _ in range(50)],
           rand_ops(40, LV[:3]))
write_case("case10_random_medium",
           [rnd.choice(LV) for _ in range(5000)],
           rand_ops(300, LV))
write_case("case11_random_large",
           [rnd.choice(LV) for _ in range(200000)],
           rand_ops(400, LV))
write_case("case12_single_level_large",
           ["ONLY"] * 100000,
           ["COUNT ONLY", "UNIQUE", "REMOVE ONLY", "PRINT", "COUNT ONLY", "UNIQUE"])
write_case("case13_no_ops", ["A", "B"], [])
write_case("case14_many_unique",
           ["L%03d" % i for i in range(300)],
           ["UNIQUE", "COUNT L000", "COUNT L299", "FILTER L150", "REMOVE L150", "COUNT L150"])
