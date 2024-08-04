#!/usr/bin/env bash
 
cd "$(dirname "${BASH_SOURCE[0]}")"

function test-rofi(){
    rofi -theme "./rofi-theme.rasi" -font "Arial 10" -modi blocks -show blocks -blocks-wrap "$1" > /dev/null
}


function compare-result(){
    DIFF_PIXEL_AMOUNT="$(compare -fuzz 5% -metric AE ./"$1"/expected-screenshot-"$2".png ./"$1"/result-screenshot-"$2".png "$1"/diff-screenshot-"$2".png 2>&1)"
    DIFF_PIXEL_PERC=$(echo "$((DIFF_PIXEL_AMOUNT * 100)) $((1280 * 720))" | awk '{printf "%.2f%\n", $1/$2}')
    if [ "$DIFF_PIXEL_AMOUNT" = "0" ]; then
        echo 0
    else
        echo "$DIFF_PIXEL_AMOUNT different pixels ($DIFF_PIXEL_PERC)"
    fi
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
        if [ "$RESULT" = "0" ]; then
            echo "ok $TEST_NUMBER - $TEST_NAME - screenshot $scrshtnum equal"; 
            echo "pass" > "$file/RESULT"
        else 
            echo "not ok $TEST_NUMBER - $TEST_NAME - screenshot $scrshtnum diffferent, $RESULT"; 
            echo "fail" > "$file/RESULT"
        fi
        TEST_NUMBER=$((TEST_NUMBER+1))
    done < <( find "$file" -mindepth 1 -maxdepth 1 -name "expected-screenshot-*.png" -type f -print0 | sed -znE 's|.*/expected-screenshot-([0-9]+).*|\1|p' )
done < <(find . -mindepth 1 -maxdepth 1 -name "UI*" -type d -print0 | sort -z)