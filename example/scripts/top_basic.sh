#!/bin/bash

#kills top subproccess on exit
cleanup() {
        local pids=$(jobs -pr)
        [ -n "$pids" ] && kill $pids
}
trap "cleanup" EXIT

toLinesJson(){
	echo "$1" | sed -e 's/\\/\\\\/g' -e 's/\"/\\"/g' -e 's/.*/"&"/' | paste -sd "," -
}

toStringJson(){
	echo "$1" | sed -e 's/\\/\\\\/g' -e 's/\"/\\"/g' -e '$!s/.*/&\\n/' | paste -sd "" -
}

top -c -b | while IFS= read -r line; do
	TOP="$line"
	while true; do
		IFS= read -t 0.01 -r line;
		VAR=$?
		if ((VAR == 0)); then # read nother line successfully
			TOP="$TOP"$'\n'"$line"
		elif (( VAR > 128 )); then # timeout happened
			break;
		else # any other reason
			exit
		fi
	done
	TOP="$(sed '/./,$!d' <<< "$TOP")"
	TOP_INFO="$(sed -n '1,/^\s*PID/p' <<< "$TOP")"
	TOP_PIDLIST="$(sed '1,/^\s*PID/d' <<< "$TOP")"
	JSON_LINES="$(toLinesJson "$TOP_PIDLIST")"
	JSON_MESSAGE="$(toStringJson "$TOP_INFO")"
 	TEXT=$(cat <<EOF | tr -d "\n\t"
{
	"message": "${JSON_MESSAGE}",
	"prompt": "search",
	"lines":[${JSON_LINES}]
}
EOF
)

	printf '%s\n' "$TEXT"
done &

wait $!