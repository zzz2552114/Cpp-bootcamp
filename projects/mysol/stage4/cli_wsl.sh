#!/usr/bin/env bash
# 把 P4.5 CLI 转发给 WSL 里的 Linux 版 ./cli。（本文件必须 LF 换行）
#
# 准备（Git Bash）：
#   cd /e/cpp-feature/projects/mysol/stage4
#   wsl.exe -e bash -c 'g++ -std=c++17 p4_5_log_analyzer_cli.cpp -o cli'
#
# 对拍（Git Bash）：
#   bash ../../tests/stage4/tests/run_tests.sh ./cli_wsl.sh
#
# 不向 wsl.exe 传 /mnt/... 这种以 / 开头的参数：Git Bash 会把它们改写成
# Windows 路径。这里只用相对路径 ./cli —— wsl.exe 会把当前 Windows 目录
# 自动翻译成 /mnt/...，所以先 cd 到脚本所在目录即可。
set -u

cd "$(dirname "$0")" || exit 1
exec wsl.exe -e bash -c './cli'
