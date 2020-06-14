#!/bin/bash
 
ACTIONS="clear input"$'\n'\
"set input as \"lorem ipsum\""$'\n'\
"clear prompt"$'\n'\
"set prompt as \"lorem ipsum\""$'\n'\
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

		"SELECT_ENTRY set input as \"lorem ipsum\"" ) 
			stdbuf -oL echo '{"input": "lorem ipsum"}'
			;;

		"SELECT_ENTRY set prompt as \"lorem ipsum\"" ) 
			stdbuf -oL echo '{"prompt": "lorem ipsum"}'
			;;
		"SELECT_ENTRY filter by input" ) 
			stdbuf -oL echo '{"input action": "filter"}'
			;;
		"SELECT_ENTRY do not filter by input" ) 
			stdbuf -oL echo '{"input action": "send"}'
			;;

	esac
done