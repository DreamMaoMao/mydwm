#! /bin/bash
# DATE 获取日期和时间的脚本

tempfile=$(
	cd $(dirname $0)
	cd ..
	pwd
)/temp

this=_date
icon_color="^c#e4e1db^^b#a21d570xff^"
text_color="^c#e4e1db^^b#a21d570xff^"
signal=$(echo "^s$this^" | sed 's/_//')

update() {
	time_text="$(date '+%m/%d %H:%M')"
	case "$(date '+%I')" in
	"01") time_icon="" ;;
	"02") time_icon="" ;;
	"03") time_icon="" ;;
	"04") time_icon="" ;;
	"05") time_icon="" ;;
	"06") time_icon="" ;;
	"07") time_icon="" ;;
	"08") time_icon="" ;;
	"09") time_icon="" ;;
	"10") time_icon="" ;;
	"11") time_icon="" ;;
	"12") time_icon="" ;;
	esac

	icon=" $time_icon"
	text="$time_text "

	sed -i '/^export '$this'=.*$/d' $tempfile
	printf "export %s='%s%s%s%s%s'\n" $this "$signal" "" "" "$text_color" "$icon $text" >>$tempfile
}

notify() {
	_cal=$(cal --color=always | sed 1,2d | sed 's/..7m/<b><span color="#ff79c6">/;s/..0m/<\/span><\/b>/')
	_todo=$(cat ~/.todo.md | sed 's/\(- \[x\] \)\(.*\)/<span color="#ff79c6">\1<s>\2<\/s><\/span>/' | sed 's/- \[[ |x]\] //')
	notify-send "  Calendar" "\n$_cal\n————————————————————\n$_todo" -r 9527
}

call_todo() {
	pid1=$(ps aux | grep 'st -t statusutil' | grep -v grep | awk '{print $2}')
	pid2=$(ps aux | grep 'st -t statusutil_todo' | grep -v grep | awk '{print $2}')
	mx=$(xdotool getmouselocation --shell | grep X= | sed 's/X=//')
	my=$(xdotool getmouselocation --shell | grep Y= | sed 's/Y=//')
	kill $pid1 && kill $pid2 || st -t statusutil_todo -g 50x15+$((mx - 200))+$((my + 20)) -c FGN -e nvim ~/.todo.md
}

click() {
	case "$1" in
	L) rofi -theme  ~/.config/rofi/themes/fancy2.rasi -modi blocks -show blocks -blocks-wrap ~/.config/rofi/history.py ;;
	R) rofi -theme  ~/.config/rofi/themes/fancy2.rasi -modi blocks -show blocks -blocks-wrap ~/.config/rofi/recentfile.py ;;
	esac
}

case "$1" in
click) click $2 ;;
notify) notify ;;
*) update ;;
esac
