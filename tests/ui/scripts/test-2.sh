#!/usr/bin/env bash

cd "$(dirname "${BASH_SOURCE[0]}")"

echo '{ "lines": [ "one","two","three" ], "active entry": 1}'

sleep 0.1
import -window root "../assets/tests/test-2/result.png" >&2
sleep 0.2

