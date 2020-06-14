#!/bin/bash

cd "$(dirname "${BASH_SOURCE[0]}")"

rofi -modi blocks -show blocks -blocks-wrap scripts/input_test.sh "$@"