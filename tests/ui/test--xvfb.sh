#!/usr/bin/env bash
 
cd "$(dirname "${BASH_SOURCE[0]}")"

function test-rofi(){
    rofi -theme assets/rofi-theme.rasi -font "Arial 10" -modi blocks -show blocks -blocks-wrap "$1" > /dev/null
}


function compare-result(){
    DIFF_PIXEL_AMOUNT="$(compare -fuzz 5% -metric AE ./assets/tests/"$1"/expected.png ./assets/tests/"$1"/result.png assets/tests/"$1"/diff.png 2>&1)"
    DIFF_PIXEL_PERC=$(( $DIFF_PIXEL_AMOUNT * 100 / (1280*720) ))
    echo "$DIFF_PIXEL_PERC"
}


TEST_1_NAME="rofi should open blocks modi correctly"
test-rofi scripts/test-1.sh
TEST_1_RESULT="$(compare-result test-1)"


TEST_2_NAME="active entry should work on first paint"
test-rofi scripts/test-2.sh
TEST_2_RESULT="$(compare-result test-2)"


# report
echo "1..2"
if [ "$TEST_1_RESULT" -lt "1" ]; then echo "ok 1 - $TEST_1_NAME - below 1% pixel difference"; else echo "not ok 1 - $TEST_1_NAME - $TEST_1_RESULT% pixel difference"; fi
if [ "$TEST_2_RESULT" -lt "1" ]; then echo "ok 2 - $TEST_2_NAME - below 1% pixel difference"; else echo "not ok 2 - $TEST_2_NAME - $TEST_2_RESULT% pixel difference"; fi
