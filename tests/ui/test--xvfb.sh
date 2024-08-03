#!/usr/bin/env bash
 
cd "$(dirname "${BASH_SOURCE[0]}")"

function test-rofi(){
    rofi -theme "./rofi-theme.rasi" -font "Arial 10" -modi blocks -show blocks -blocks-wrap "$1" > /dev/null
}


function compare-result(){
    DIFF_PIXEL_AMOUNT="$(compare -fuzz 5% -metric AE ./"$1"/expected-screenshot-"$2".png ./"$1"/result-screenshot-"$2".png "$1"/diff-screenshot-"$2".png 2>&1)"
    DIFF_PIXEL_PERC=$(( $DIFF_PIXEL_AMOUNT * 100 / (1280*720) ))
    echo "$DIFF_PIXEL_PERC"
}

export TEST_NUMBER=1


echo "1..$(find . -mindepth 2 -maxdepth 2 -name "expected-screenshot-*.png" -type f -printf '.' | wc -c)"
while read -d $'\0' file
do
    TEST_NAME="$(head -1 "$file/DESCRIPTION")"
    test-rofi "$file/script"
    while read -d $'\0' scrshtnum
    do
        RESULT="$(compare-result "$file" "$scrshtnum")"
        if [ "$RESULT" -lt "1" ]; then
            echo "ok $TEST_NUMBER - $TEST_NAME - screenshot $scrshtnum below 1% pixel difference"; 
            echo "pass" > "$file/RESULT"
        else 
            echo "not ok $TEST_NUMBER - $TEST_NAME - screenshot $scrshtnum, $RESULT% pixel difference"; 
            echo "fail" > "$file/RESULT"
        fi
        TEST_NUMBER=$((TEST_NUMBER+1))
    done < <( find "$file" -mindepth 1 -maxdepth 1 -name "expected-screenshot-*.png" -type f -print0 | sed -znE 's|.*/expected-screenshot-([0-9]+).*|\1|p' )
done < <(find . -mindepth 1 -maxdepth 1 -name "UI*" -type d -print0 | sort -z)