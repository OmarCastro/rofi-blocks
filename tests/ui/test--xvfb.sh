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

function validate-screenshots(){
    while read -d $'\0' scrshtnum
    do
        RESULT="$(compare-result "$1" "$scrshtnum")"
        if [ "$RESULT" = "0" ]; then
            echo "ok $TEST_NUMBER - $TEST_NAME - screenshot $scrshtnum equal"; 
            echo "screenshot,$scrshtnum,pass" >> "$1/RESULT"
        else 
            echo "not ok $TEST_NUMBER - $TEST_NAME - screenshot $scrshtnum diffferent, $RESULT"; 
            echo "screenshot,$scrshtnum,fail" >> "$1/RESULT"
        fi
        TEST_NUMBER=$((TEST_NUMBER+1))
    done < <( find "$1" -mindepth 1 -maxdepth 1 -name "expected-screenshot-*.png" -type f -print0 | sed -znE 's|.*/expected-screenshot-([0-9]+).*|\1|p' )
}

function validate-texts(){
    while read -d $'\0' textnum
    do
        RESULT="$(diff -u "$1/expected-text-$textnum.txt" "$1/result-text-$textnum.txt")"
        IS_DIFF=$?
        echo "$RESULT" >  "$1/diff-text-$textnum.txt" 
        if [ "$IS_DIFF" = "0" ]; then
            echo "ok $TEST_NUMBER - $TEST_NAME - text $textnum equal"; 
            echo "text,$textnum,pass" >> "$1/RESULT"
        else 
            echo "not ok $TEST_NUMBER - $TEST_NAME - text $textnum different"; 
            echo "text,$textnum,fail" >> "$1/RESULT"
        fi
        TEST_NUMBER=$((TEST_NUMBER+1))
    done < <( find "$1" -mindepth 1 -maxdepth 1 -name "expected-text-*.txt" -type f -print0 | sed -znE 's|.*/expected-text-([0-9]+).*|\1|p' )
}

export TEST_NUMBER=1


echo "1..$(find . -mindepth 2 -maxdepth 2 \( -name "expected-screenshot-*.png" -o -name "expected-text-*.txt" \) -type f -printf '.' | wc -c)"
while read -d $'\0' file
do
    rm "$file/RESULT"
    TEST_NAME="$(head -1 "$file/DESCRIPTION")"
    test-rofi "$file/script"
    validate-screenshots "$file"
    validate-texts "$file"
done < <(find . -mindepth 1 -maxdepth 1 -name "UI*" -type d -print0 | sort -z)