#!/bin/bash


toLinesJson(){
	echo "$1" | sed -e 's/\\/\\\\/g' -e 's/\"/\\"/g' -e 's/.*/"&"/' | paste -sd "," -
}

toStringJson(){
	echo "$1" | sed -e 's/\\/\\\\/g' -e 's/\"/\\"/g' -e '$!s/.*/&\\n/' | paste -sd "" -
}


top -c -b | while IFS= read -r line; do
	TOP="$line"
	while IFS= read -t 0.01 -r line; do
		TOP="$TOP"$'\n'"$line"
	done
	TOP="$(sed '/./,$!d' <<< "$TOP")"
	TOP_INFO="$(sed -n '1,/^\s*PID/p' <<< "$TOP")"
	TOP_PIDLIST="$(sed '1,/^\s*PID/d' <<< "$TOP")"
	JSON_LINES="$(toLinesJson "$TOP_PIDLIST")"
	JSON_MESSAGE="$(toStringJson "$TOP_INFO")"
 	TEXT=$(cat <<EOF | tr -d "\n" | tr -d "\t"
{
	"message": "${JSON_MESSAGE}",
	"prompt": "search",
	"lines":[${JSON_LINES}]
}
EOF
)

	if ! printf '%s\n' "$TEXT"; then
		exit 1;
	fi
done &

