#!/usr/bin/bash

# Change brightness level with `light`.
# You can call this script like this:
# brightness.sh [up|down]

function get_brightness {
	var=$(light -G)
	echo "${var##* }" | sed 's/[^0-9][^.]*//g'
}

function send_notification {
	DIR=$(dirname "$0")
	brightness=$(get_brightness)
	icon_name="${HOME}/.config/rice_assets/Icons/b.png"

	# Send the notification
	dunstify "Brightness: $brightness%" -h int:value:$brightness -i "$icon_name" -t 1000 --replace=555 -u normal
}

case $1 in
up)
	brightnessctl set +1%
	send_notification
	;;
down)
	brightnessctl set 1%-
	send_notification
	;;
esac
