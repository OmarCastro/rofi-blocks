#!/bin/bash
 
FOCUS_ENTRY=""

toLinesJson(){
	echo "$1" | sed -e 's/\\/\\\\/g' -e 's/\"/\\"/g' -e 's/.*/"&"/' | paste -sd "," -
}

log_action(){
 	TEXT=$(cat <<EOF | tr -d "\n" | tr -d "\t"
{
	"event format": "{{name_enum}} {{data}}",
	"prompt":"select an entry to focus to other entry",
	"message": "select an entry to focus to other entry",
	${FOCUS_ENTRY}
	"lines":[
		{"text": "focus entry 3", "data":"3"},
		{"text": "focus entry 2", "data":"2"},
		{"text": "focus entry 1000 will focus first entry since it does not exist", "data":"1000"},
		{"text": "focus entry 1", "data":"1"},
		{"text": "focus entry 0", "data":"0"}
	]
}
EOF
)
	printf '%s\n' "$TEXT"
	FOCUS_ENTRY=""
}

log_action


while IFS= read -r line; do
	case "$line" in
		"SELECT_ENTRY 0"    ) FOCUS_ENTRY='"active entry": 0,'   ; log_action ;;
		"SELECT_ENTRY 1"    ) FOCUS_ENTRY='"active entry": 1,'   ; log_action ;;
		"SELECT_ENTRY 2"    ) FOCUS_ENTRY='"active entry": 2,'   ; log_action ;;
		"SELECT_ENTRY 3"    ) FOCUS_ENTRY='"active entry": 3,'   ; log_action ;;
		"SELECT_ENTRY 1000" ) FOCUS_ENTRY='"active entry": 1000,'; log_action ;;
	esac
done