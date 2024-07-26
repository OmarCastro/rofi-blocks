#!/usr/bin/env bash
 
cd "$(dirname "${BASH_SOURCE[0]}")"


TEST_1_NAME="rofi should open blocks modi correctly"
rofi -theme /usr/share/rofi/themes/gruvbox-light.rasi -modi blocks -show blocks -blocks-wrap scripts/test-1.sh "$@" > /dev/null
TEST_1_RESULT="$(compare -fuzz 1% -metric AE ./assets/tests/test-1/expected.png ./assets/tests/test-1/result.png assets/tests/test-1/diff.png 2>&1)"


TEST_2_NAME="active entry should work on first paint"
rofi -theme /usr/share/rofi/themes/gruvbox-light.rasi -modi blocks -show blocks -blocks-wrap scripts/test-2.sh "$@" > /dev/null
TEST_2_RESULT="$(compare -fuzz 1% -metric AE ./assets/tests/test-2/expected.png ./assets/tests/test-2/result.png assets/tests/test-2/diff.png 2>&1)"


# report
echo "1..2"
if [ "$TEST_1_RESULT" = "0" ]; then echo "ok 1 - $TEST_1_NAME"; else echo "not ok 1 - $TEST_1_NAME - $TEST_1_RESULT"; fi
if [ "$TEST_2_RESULT" = "0" ]; then echo "ok 2 - $TEST_2_NAME"; else echo "not ok 2 - $TEST_2_NAME - $TEST_2_RESULT"; fi
