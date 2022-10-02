#!/bin/bash

cd "$(dirname "${BASH_SOURCE[0]}")"

toggleMarkup="true"
while printf ''; do
 	TEXT=$(cat <<EOF | sed 's/\\/\\\\/g' | tr -d "\n" | tr -d "\t"
{
	"message": "Updating message!! \\n\\"Current time\\" : %(%H:%M:%S)T ",
	"overlay": "Current overlay : %(%H:%M:%S)T ",
	"prompt": "prompt %(%H:%M:%S)T ",
	"lines":[
		"also updates menu option text!! %(%H:%M:%S)T", 
		"string", 
		{"text":"json object"}, 
		{"text":"json with urgent flag", "urgent":true},
		{"text":"json with highlight flag", "highlight": true},
		{"text":"json with nonselectable", "nonselectable": true},
        {"text":"multi-byte unicode: •"},
        {"text":"icon unicode character: 😀"},
        {"text":"folder icon", "icon":"folder"},
		{"text":"json <i>with</i> <b>markup</b> <b><i>flag</i></b>", "markup": true},
		{"text":"json <i>toggling</i> <b>markup</b> flag", "markup": $toggleMarkup},
		{"text":"json <i>without</i> <b>markup</b> <b><i>flag</i></b>", "markup": false},
		{"text":"json with <b><i>all</i></b> flags", "urgent":true, "highlight": true, "markup": true}
	]}
EOF
)

	if ! printf "$TEXT"'\n'; then
		exit 1;
	fi
 	sleep 1;
 	if [ "$toggleMarkup" = "true" ]; then toggleMarkup="false"; else toggleMarkup="true"; fi

done | rofi -modi blocks -show blocks -show-icons "$@" 
