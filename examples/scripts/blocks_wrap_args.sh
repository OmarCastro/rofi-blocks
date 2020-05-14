#!/bin/bash
 
ACTIONS=""

if (( "$#" )); then 
  ACTIONS="$1" 
  shift 
fi

while (( "$#" )); do 
   ACTIONS="$ACTIONS"$'\n'"$1" 
  shift 
done

toLinesJson(){
	echo "$1" | sed -e 's/\\/\\\\/g' -e 's/\"/\\"/g' -e 's/.*/"&"/' | paste -sd "," -
}
echo '{"close on exit": false,"prompt":"filter args","message": "here shows the arguments passed when using -blocks-wrap argument as command line"  }'
log_action(){
	JSON_LINES="$(toLinesJson "$ACTIONS")"
 	TEXT=$(cat <<EOF | tr -d "\n" | tr -d "\t"
{
	"lines":[${JSON_LINES}]
}
EOF
)
	printf '%s\n' "$TEXT"
}

log_action
