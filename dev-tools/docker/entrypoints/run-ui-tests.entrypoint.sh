#!/usr/bin/env bash
cp -r /repo /repo-copy
cd /repo-copy
rm -rf build build-docs build-test
bash dev-tools/github-action-build.sh
