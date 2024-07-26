#!/usr/bin/env bash
cd "$(dirname "${BASH_SOURCE[0]}")"
cd ..

meson setup build-test -Db_coverage=true --warnlevel 2 || exit 1
meson compile -C build-test || exit 1
meson install -C build-test
meson test --wrap='valgrind --leak-check=full' -C build-test
ninja coverage -C build-test
ls build-test/meson-logs/testlog.json || cp build-test/meson-logs/testlog-valgrind.json build-test/meson-logs/testlog.json 
ls build-test/meson-logs/testlog.txt || cp build-test/meson-logs/testlog-valgrind.txt build-test/meson-logs/testlog.txt 
dev-tools/run docs
rm -rf build-docs/reports/ui-images
cp -r tests/ui/assets/tests build-docs/reports/ui-images