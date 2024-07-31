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
rm -rf build-docs/reports/ui
cp -r tests/ui build-docs/reports/ui
cp -r test-utils/ui/reports/assets/* build-docs/reports/ui
while IFS= read -d $'\0' -r DIR_PATH ; do 
    DIR_NAME="$(basename $DIR_PATH)"
    cp build-docs/reports/ui/report.html "build-docs/reports/ui/$DIR_NAME"
    mv "build-docs/reports/ui/$DIR_NAME/script" "build-docs/reports/ui/$DIR_NAME/script.txt"
    node dev-tools/scripts/build-html.js reports/ui/$DIR_NAME/report.html
done < <(find build-docs/reports/ui -mindepth 1 -maxdepth 1 -type d -print0)
