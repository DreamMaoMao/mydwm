#! /bin/bash
# CPU 获取CPU使用率和温度的脚本

tempfile=$(
	cd $(dirname $0)
	cd ..
	pwd
)/temp

this=_cpu
icon_color="^c#ddca9e^^b#201b140xff^"
text_color="^c#ddca9e^^b#201b140xff^"
signal=$(echo "^s$this^" | sed 's/_//')

with_temp() {
	# check
	[ ! "$(command -v sensors)" ] && echo command not found: sensors && return

	temp_text=$(sensors | grep Tctl | awk '{printf "%d°C", $2}')
	text=" $cpu_text% $temp_text "
}

update() {
	cpu_icon="󰻠"
	cpu_text=$(top -n 1 -b | sed -n '1p' | awk '{print $10}')
	if [[ "$cpu_text" == "average:" ]];then
		cpu_text=$(top -n 1 -b | sed -n '1p' | awk '{print $11}')
	elif [[ "$cpu_text" == "load" ]];then
		cpu_text=$(top -n 1 -b | sed -n '1p' | awk '{print $13}')
	elif [[ "$cpu_text" == "users," ]];then
		cpu_text=$(top -n 1 -b | sed -n '1p' | awk '{print $13}')
	fi
	cpu_text=${cpu_text::-1}
	icon=" $cpu_icon"
	text="$cpu_text "

	with_temp

	sed -i '/^export '$this'=.*$/d' $tempfile
	printf "export %s='%s%s%s%s%s'\n" $this "$signal" "" "" "$text_color" "$icon$text" >>$tempfile
}

notify() {
	notify-send "󰻠 CPU tops" "\n$(ps axch -o cmd:15,%cpu --sort=-%cpu | head)\\n\\n(100% per core)" -r 9527
}

call_btop() {
	pid1=$(ps aux | grep 'st -t statusutil' | grep -v grep | awk '{print $2}')
	pid2=$(ps aux | grep 'st -t statusutil_cpu' | grep -v grep | awk '{print $2}')
	mx=$(xdotool getmouselocation --shell | grep X= | sed 's/X=//')
	my=$(xdotool getmouselocation --shell | grep Y= | sed 's/Y=//')
	kill $pid1 && kill $pid2 || st -t statusutil_cpu -g 82x25+$((mx - 328))+$((my + 20)) -c FGN -e btop
}

click() {
	case "$1" in
	L) /usr/bin/rofi -config ~/.config/rofi/themes/fullscreen-preview.rasi -show drun;;
	M) ;;
	R) gnome-system-monitor;;
	U) ;;
	D) ;;
	esac
}

case "$1" in
click) click $2 ;;
notify) notify ;;
*) update ;;
esac
