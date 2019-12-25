#!/bin/bash

tmp_dir=$(mktemp -d -t tmp-XXXXXXXXXX)
 
#kills top subproccess on exit
cleanup() {
        local pids=$(jobs -pr)
        [ -n "$pids" ] && kill $pids
        rm -rf $tmp_dir

}
trap "cleanup" EXIT

toLinesJson(){
	echo "$1" | sed -e 's/\\/\\\\/g' -e 's/\"/\\"/g' -e 's/.*/"&"/' | paste -sd "," -
}

toStringJson(){
	echo "$1" | sed -e 's/\\/\\\\/g' -e 's/\"/\\"/g' -e '$!s/.*/&\\n/' | paste -sd "" -
}

touch $tmp_dir/pid
selected_pid_get(){ cat $tmp_dir/pid; }
selected_pid_set(){ echo "$1" > $tmp_dir/pid; kill -USR1 $$; }
selected_pid_clear(){ :> $tmp_dir/pid; kill -USR1 $$; }

touch $tmp_dir/top
top_get(){ cat $tmp_dir/top; }
top_set(){ echo "$1" > $tmp_dir/top; kill -USR1 $$; }

touch $tmp_dir/sort
sort_column_get(){ cat $tmp_dir/sort; }
sort_column_set(){ 
	if [[ "$(sort_column_get)" == "$1 desc" ]];then
		ORDER="asc"
	else
 		ORDER="desc"
	fi
	echo "$1 $ORDER" > $tmp_dir/sort; kill -USR1 $$; 
}
echo "%CPU desc" > $tmp_dir/sort

echo '{"event format": "{{name_enum}} {{value}}"}'

IFS=

top -c -b | while read -r line; do
	TOP="$line"
	while true; do
		read -t 0.01 -r line;
		VAR=$?
		if ((VAR == 0)); then # read nother line successfully
			TOP="$TOP"$'\n'"$line"
		elif (( VAR > 128 )); then # timeout happened
			break;
		else # any other reason
			exit
		fi
	done
	top_set "$TOP"
done &

declare -A columnLabels;
columnLabels['PID']='Proccess id (PID)'
columnLabels['USER']='User name'
columnLabels['PR']='Priority' 
columnLabels['NI']='Nice value' 
columnLabels['VIRT']='Virual Memory Size' 
columnLabels['RES']='Resident Memory Size' 
columnLabels['SHR']='Shared Memory Size' 
columnLabels['S']='Process Status' 
columnLabels['%CPU']='CPU Usage' 
columnLabels['%MEM']='Memory Usage (RES)' 
columnLabels['TIME']='CPU Time' 
columnLabels['TIME+']='CPU Time, hundredths' 
columnLabels['COMMAND']='Command' 

declare -A statusLabels;

statusLabels['D']='uninterruptible sleep'
statusLabels['I']='idle'
statusLabels['R']='running'
statusLabels['S']='sleeping'
statusLabels['T']='stopped by job control signal'
statusLabels['t']='stopped by debugger during trace'
statusLabels['Z']='zombie'

# warning: if there is a custom top configuration (.toprc) surprises are to be expected
getPidInfo(){
	TOP="$(top_get | sed '/./,$!d')"
	(sed -n '7p' <<< "$TOP"; grep -m1 -P "^\s+$SELECTED_PID\s+" <<< "$TOP") \
		| tr -s " " \
		| IFS=" " awk '{ for (i=1; i<=NF; i++) RtoC[i]= (RtoC[i]? RtoC[i] FS $i: $i) } END{ for (i in RtoC) print RtoC[i] }' \
		| while IFS=" " read -r label val; do
			case "$label" in
				COMMAND ) while read line; do val="$val $line"; done;;
				S ) val="${statusLabels[$val]}";;
				'%CPU' | '%MEM' ) val="$val %" ;;
			esac
			echo "<b>${columnLabels[$label]}:</b> $val"
		done
}

sort_message(){
	SORT="$(sort_column_get)"
	case "${SORT#* }" in
		'asc' ) REPLACEMENT="<b>&</b>";;
		'desc' ) REPLACEMENT="<b><i>&</i></b>";;
	esac
	sed '7s|'"${SORT%% *}"'|'"$REPLACEMENT"'|'
}

sort_list(){
	case "$(sort_column_get)" in
		"PID asc" ) sort -h -k1;;
		"PID desc" ) sort -h -k1 -r;;
		"%CPU asc" ) sort -h -k9;;
		"%CPU desc" ) sort -h -k9 -r;;
		"%MEM asc" ) sort -h -k10;;
		"%MEM desc" ) sort -h -k10 -r;;
		* ) cat
	esac

}

printNextTick(){
	SELECTED_PID="$(selected_pid_get)"

	if [[ -z "$SELECTED_PID" ]]; then

		TOP="$(top_get | sed '/./,$!d')"
		TOP_INFO="$(sed -n '1,/^\s*PID/p' <<< "$TOP" | sort_message)"
		TOP_PIDLIST="$(sed '1,/^\s*PID/d' <<< "$TOP" | sort_list)"
		JSON_LINES="$(toLinesJson "$TOP_PIDLIST")"
		JSON_MESSAGE="$(toStringJson "$TOP_INFO")"
	else
		SELECTED_PID_INFO="$(getPidInfo)"
		LINES="return
terminate (send SIGTERM signal)
kill (send SIGKILL signal)"
		JSON_LINES="$(toLinesJson "$LINES")"
		JSON_MESSAGE="$(toStringJson "$SELECTED_PID_INFO")"

	fi
	 	TEXT=$(cat <<EOF | tr -d "\n\t"
{
	"message": "${JSON_MESSAGE}",
	"prompt": "search",
	"lines":[${JSON_LINES}]
}
EOF
)

	printf '%s\n' "$TEXT"
}

trap printNextTick USR1

while read -r line; do
	case "$line" in
		"CUSTOM_KEY 1" ) sort_column_set "PID";;
		"CUSTOM_KEY 2" ) sort_column_set "%CPU";;
		"CUSTOM_KEY 3" ) sort_column_set "%MEM";;
		"SELECT_ENTRY return" ) selected_pid_clear;;
		"SELECT_ENTRY terminate (send SIGTERM signal)" ) kill -s SIGTERM "$(selected_pid_get)"; selected_pid_clear;;
		"SELECT_ENTRY kill (send SIGKILL signal)" ) kill -s SIGKILL "$(selected_pid_get)"; selected_pid_clear;;
		SELECT_ENTRY* ) selected_pid_set "$( tr -s " " <<< "$line" | cut -d" " -f2,2)";;

	esac
done


