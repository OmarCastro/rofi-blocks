#!/usr/bin/env bash

echo '{ "lines": [ "hello","world" ], "message": "lorem ipsum dolor sit amet", "prompt": "test prompt"}'

# END TEST SCRIPT
cd "$(dirname "${BASH_SOURCE[0]}")"
sleep 0.1
import -window root "../assets/tests/test-1/result.png" >&2
sleep 0.1

