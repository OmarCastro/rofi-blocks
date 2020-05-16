#!/bin/bash
 
ACTIONS=""
default_custom_format="{{name_enum}} {{value}}"
custom_format="${format:-$default_custom_format}"

toStringJson(){
	sed -e 's/\\/\\\\/g' -e 's/\"/\\"/g' -e '$!s/.*/&\\n/' | paste -sd "" -
}

toLinesJson(){
	sed -e 's/\\/\\\\/g' -e 's/\"/\\"/g' -e 's/.*/"&"/' | paste -sd "," -
}

TEXT=$(cat <<EOF | tr -d "\n" | tr -d "\t"
{
	"input action":"send",
	"prompt":"updating input also logs action",
	"event format":"${custom_format}",
	"message": "Same as action logger but with different event format \n
		the format used is \"${custom_format}\" \n
		valid parameters: \n
		 {{name}} - event name in readable format\n
		 {{name_enum}} - event name in enumerated format\n
		 {{name_escaped}} - event name in readable format, escaped to work on a json string\n
		 {{value}} - event related information\n
		 {{value_escaped}} - event related information, escaped to work on a json string\n
		 \n
		 You can change the format by setting the environment variable 'format' when running
		 this script
	"
}
EOF
)
printf '%s\n' "$TEXT"

log_action(){
	JSON_LINES="$(toLinesJson <<< "$ACTIONS")"
	echo "{\"lines\":[${JSON_LINES}]}"
}

if IFS= read -r line; then
	ACTIONS="$line"
	log_action
fi

while IFS= read -r line; do
	ACTIONS="$line"$'\n'"$ACTIONS"
	log_action
done