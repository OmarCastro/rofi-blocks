#!/usr/bin/env bash
 
cd "$(dirname "${BASH_SOURCE[0]}")"

function test-rofi(){
    rofi -theme assets/rofi-theme.rasi -font "Arial 8" -modi blocks -show blocks -blocks-wrap "$1" > /dev/null
}

function compare-result(){
    compare -fuzz 5% -metric AE ./assets/tests/"$1"/expected.png ./assets/tests/"$1"/result.png assets/tests/"$1"/diff.png 2>&1
}


TEST_1_NAME="rofi should open blocks modi correctly"
test-rofi scripts/test-1.sh
TEST_1_RESULT="$(compare-result test-1)"


TEST_2_NAME="active entry should work on first paint"
test-rofi scripts/test-2.sh
TEST_2_RESULT="$(compare-result test-2)"


# report
echo "1..2"
if [ "$TEST_1_RESULT" -lt "2" ]; then echo "ok 1 - $TEST_1_NAME"; else echo "not ok 1 - $TEST_1_NAME - $TEST_1_RESULT"; fi
if [ "$TEST_2_RESULT" -lt "2" ]; then echo "ok 2 - $TEST_2_NAME"; else echo "not ok 2 - $TEST_2_NAME - $TEST_2_RESULT"; fi
