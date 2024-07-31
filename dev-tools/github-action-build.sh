#!/usr/bin/env bash
cd "$(dirname "${BASH_SOURCE[0]}")"
cd ..

echo "==============="
echo "=== COMPILE ==="
echo "==============="

meson setup build-test -Db_coverage=true --warnlevel 2 -Dui_test=true || exit 1
meson compile -C build-test || exit 1

echo "==============="
echo "=== INSTALL ==="
echo "==============="


sudo meson install -C build-test

echo "================"
echo "===== TEST ====="
echo "================"

meson test --wrap='valgrind --leak-check=full' -C build-test
ninja coverage -C build-test
ls build-test/meson-logs/testlog.json || cp build-test/meson-logs/testlog-valgrind.json build-test/meson-logs/testlog.json 
ls build-test/meson-logs/testlog.txt || cp build-test/meson-logs/testlog-valgrind.txt build-test/meson-logs/testlog.txt 

echo "================"
echo "=== DOCUMENT ==="
echo "================"

dev-tools/run docs