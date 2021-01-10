#!/bin/bash
 
ACTIONS=""

toLinesJson(){
	echo "$1" | sed -e 's/\\/\\\\/g' -e 's/\"/\\"/g' -e 's/.*/"&"/' | paste -sd "," -
}

echo '{"input action":"send", "prompt":"updating input also logs action" }'

log_action(){
 	TEXT=$(cat <<EOF | tr -d "\n" | tr -d "\t"
{
	"message": "${JSON_MESSAGE}",
	"lines":[${ACTIONS}]
}
EOF
)
	printf '%s\n' "$TEXT"
}

if IFS= read -r line; then
	ACTIONS="{\"text\":$(toLinesJson "$line"), \"data\":\"$(date -Ins)\"}"
	log_action
fi

while IFS= read -r line; do
	ACTIONS="{\"text\":$(toLinesJson "$line"), \"data\":\"$(date -Ins)\"},"$'\n'"$ACTIONS"
	log_action
done