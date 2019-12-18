#!/bin/bash

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
		{"text":"json with both flag", "urgent":true, "highlight": true}
	]}
EOF
)

	if ! printf "$TEXT"'\n'; then
		exit 1;
	fi
 	sleep 1;

done