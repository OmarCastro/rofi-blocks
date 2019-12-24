#!/bin/bash
 
ACTIONS=""

toLinesJson(){
	echo "$1" | sed -e 's/\\/\\\\/g' -e 's/\"/\\"/g' -e 's/.*/"&"/' | paste -sd "," -
}

echo '{"input action":"send", "prompt":"updating input also logs action" }'

log_action(){
	JSON_LINES="$(toLinesJson "$ACTIONS")"
 	TEXT=$(cat <<EOF | tr -d "\n" | tr -d "\t"
{
	"message": "${JSON_MESSAGE}",
	"lines":[${JSON_LINES}]
}
EOF
)
	printf '%s\n' "$TEXT"
}

if IFS= read -r line; then
	ACTIONS="$line"
	log_action
fi

while IFS= read -r line; do
	ACTIONS="$line"$'\n'"$ACTIONS"
	log_action
done