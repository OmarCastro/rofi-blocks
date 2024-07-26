#!/usr/bin/env bash

cd "$(dirname "${BASH_SOURCE[0]}")"

echo '{ "lines": [ "hello","world" ], "message": "lorem ipsum dolor sit amet", "prompt": "test prompt"}'

sleep 0.1
import -window root "../assets/tests/test-1/result.png" >&2
sleep 0.2

