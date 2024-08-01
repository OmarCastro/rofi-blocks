#!/usr/bin/env bash
 
cd "$(dirname "${BASH_SOURCE[0]}")"

function test-rofi(){
    rofi -theme "./rofi-theme.rasi" -font "Arial 10" -modi blocks -show blocks -blocks-wrap "$1" > /dev/null
}


function compare-result(){
    DIFF_PIXEL_AMOUNT="$(magick compare -fuzz 5% -metric AE ./"$1"/expected-screenshot-"$2".png ./"$1"/result-screenshot-"$2".png "$1"/diff-screenshot-"$2".png 2>&1)"
    DIFF_PIXEL_PERC=$(( $DIFF_PIXEL_AMOUNT * 100 / (1280*720) ))
    echo "$DIFF_PIXEL_PERC"
}

echo "1..$(find . -mindepth 1 -maxdepth 1 -name "UI*" -type d -printf '.' | wc -c)"
find . -mindepth 1 -maxdepth 1 -name "UI*" -type d -print0 | while read -d $'\0' file
do
    TEST_NAME="$(head -1 "$file/DESCRIPTION")"
    test-rofi "$file/script"
    RESULT="$(compare-result "$file" "1")"
    if [ "$RESULT" -lt "1" ]; then
        echo "ok 1 - $TEST_NAME - below 1% pixel difference"; 
        echo "pass" > "$file/RESULT"
    else 
        echo "not ok 1 - $TEST_NAME - $RESULT% pixel difference"; 
        echo "fail" > "$file/RESULT"
    fi
done