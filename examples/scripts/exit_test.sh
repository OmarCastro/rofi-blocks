#!/bin/bash
 
ACTIONS="exit"$'\n'\
"exit script only"

toLinesJson(){
	echo "$1" | sed -e 's/\\/\\\\/g' -e 's/\"/\\"/g' -e 's/.*/"&"/' | paste -sd "," -
}
echo '{"input action":"send", "prompt":"updating input also logs action", "event format": "{{name_enum}} {{value}}"}'
log_action(){
	JSON_LINES="$(toLinesJson "$ACTIONS")"
 	TEXT=$(cat <<EOF | tr -d "\n" | tr -d "\t"
{
	"message": "exit test",
	"lines":[${JSON_LINES}]
}
EOF
)
	printf '%s\n' "$TEXT"
}

log_action

while read -r line; do
	case "$line" in
		"SELECT_ENTRY exit" ) exit 0;;
		"SELECT_ENTRY exit script only" ) 
			stdbuf -oL echo '{"close on exit": false, "message":"done"}'
			sleep 0.1
			exit 0
;;

	esac
done