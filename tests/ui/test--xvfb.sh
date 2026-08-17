#!/usr/bin/env bash


cd "$(dirname "${BASH_SOURCE[0]}")"
(
    # install npm packages if not installed yet
    cd ../../dev-tools/scripts
    [ -d "node_modules" ] || npm install
)

function test-rofi(){
    rofi -theme "./rofi-theme.rasi" -font "Arial 10" -modi blocks -show blocks -blocks-wrap "$1" > /dev/null
}

function compare-images(){
    cd ../../dev-tools/scripts
    npx its-an-image-comparison -t 5 -a -f "%d %p" $1 $2 $3
}


function compare-result(){
    RESULT="$(compare-images "$1"/expected-screenshot-"$2".png "$1"/result-screenshot-"$2".png "$1"/diff-screenshot-"$2".png 2>&1)"
    read -r DIFF_PIXEL_AMOUNT DIFF_PIXEL_PERC <<< "$RESULT"
    if [ "$DIFF_PIXEL_AMOUNT" = "0" ]; then
        echo 0
    else
        echo "$DIFF_PIXEL_AMOUNT different pixels ($DIFF_PIXEL_PERC%)"
    fi
}

function validate-screenshots(){
    while read -d $'\0' scrshtnum
    do
        RESULT="$(compare-result "$(realpath $1)" "$scrshtnum")"
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
    rm -rf "$file/RESULT"
    TEST_NAME="$(head -1 "$file/DESCRIPTION")"
    test-rofi "$file/script"
    validate-screenshots "$file"
    validate-texts "$file"
done < <(find . -mindepth 1 -maxdepth 1 -name "UI*" -type d -print0 | sort -z)