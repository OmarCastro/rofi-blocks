#!/bin/bash
 
ACTIONS="exit script with rofi"$'\n'\
"exit script without closing rofi"

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
	"message": "exit test\nscript running",
	"lines":[${JSON_LINES}]
}
EOF
)
	printf '%s\n' "$TEXT"
}

log_action

while read -r line; do
	case "$line" in
		"SELECT_ENTRY exit script with rofi" ) exit 0;;
		"SELECT_ENTRY exit script without closing rofi" ) 
			stdbuf -oL echo '{"close on exit": false, "message":"exit test\nscript ended\nNow all options will do nothing"}'
			sleep 0.1
			exit 0;;
		* ) 
			stdbuf -oL echo "{\"message\": \"exit test\nscript running\nreceived message: $(toStringJson "$line")\"}"


	esac
done