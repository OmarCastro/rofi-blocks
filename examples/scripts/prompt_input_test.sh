#!/bin/bash
 
ACTIONS="clear input"$'\n'\
"set input as \"lorem ipsum\""$'\n'\
"clear prompt"$'\n'\
"set prompt as \"lorem ipsum\""$'\n'\
"hide message (empty string)"$'\n'\
"hide message (null)"$'\n'\
"set message as \"hello there\""$'\n'\
"hide overlay (empty string)"$'\n'\
"hide overlay (null)"$'\n'\
"set overlay as \"overlay\""$'\n'\
"filter by input"$'\n'\
"do not filter by input"

toStringJson(){
	echo "$1" | sed -e 's/\\/\\\\/g' -e 's/\"/\\"/g' -e '$!s/.*/&\\n/' | paste -sd "" -
}

toLinesJson(){
	echo "$1" | sed -e 's/\\/\\\\/g' -e 's/\"/\\"/g' -e 's/.*/"&"/' | paste -sd "," -
}
echo '{"event format": "{{name_enum}} {{value}}"}'
log_action(){
	JSON_LINES="$(toLinesJson "$ACTIONS")"
 	TEXT=$(cat <<EOF | tr -d "\n" | tr -d "\t"
{
	"input action": "send",
	"message": "message",
	"lines":[${JSON_LINES}]
}
EOF
)
	printf '%s\n' "$TEXT"
}

log_action

while read -r line; do
	case "$line" in
		"SELECT_ENTRY clear input" )
			stdbuf -oL echo '{"input": ""}'
			;;
		"SELECT_ENTRY clear prompt" )
			stdbuf -oL echo '{"prompt": ""}'
			;;
		"SELECT_ENTRY hide message (empty string)" ) 
			stdbuf -oL echo '{"message": ""}'
			;;
		"SELECT_ENTRY hide message (null)" ) 
			stdbuf -oL echo '{"message": null}'
			;;
		"SELECT_ENTRY hide overlay (empty string)" ) 
			stdbuf -oL echo '{"overlay": ""}'
			;;
		"SELECT_ENTRY hide overlay (null)" ) 
			stdbuf -oL echo '{"overlay": null}'
			;;
		"SELECT_ENTRY set input as \"lorem ipsum\"" ) 
			stdbuf -oL echo '{"input": "lorem ipsum"}'
			;;
		"SELECT_ENTRY set prompt as \"lorem ipsum\"" ) 
			stdbuf -oL echo '{"prompt": "lorem ipsum"}'
			;;
		"SELECT_ENTRY set message as \"hello there\"" ) 
			stdbuf -oL echo '{"message": "hello there"}'
			;;
		"SELECT_ENTRY set overlay as \"overlay\"" ) 
			stdbuf -oL echo '{"overlay": "overlay"}'
			;;
		"SELECT_ENTRY filter by input" ) 
			stdbuf -oL echo '{"input action": "filter"}'
			;;
		"SELECT_ENTRY do not filter by input" ) 
			stdbuf -oL echo '{"input action": "send"}'
			;;

	esac
done