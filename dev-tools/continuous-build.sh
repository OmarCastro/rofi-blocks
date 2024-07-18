#!/usr/bin/env bash
cd "$(dirname "${BASH_SOURCE[0]}")"
cd ..

PROJECT_DIR=$(pwd)

build_project() {
    meson compile -C build || exit 1
}

rm -rf build
meson setup build -Db_coverage=true --reconfigure
build_project

while inotifywait -e close_write $PROJECT_DIR/src $PROJECT_DIR/build; do 
    build_project
done
