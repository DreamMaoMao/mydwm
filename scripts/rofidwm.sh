#!/usr/bin/bash

# This script is used to run rofi with different configurations
# based on desired usage.
# You can call this script like this:
# rofi.sh [drun|config|window|outopts]

roficonf="$HOME/.config/rofi/"

case $1 in
drun)
	/usr/bin/rofi -config $roficonf/drun.rasi -show drun
	;;
window)
	(
		/usr/bin/rofi -config $roficonf/window.rasi -show windowcd
		xdotool keyup Tab
	) &
	xdotool keyup Tab
	xdotool keydown Tab
	;;
outopts)
	option=$(printf "\n\n⏻\n" |
		/usr/bin/rofi -config $roficonf/logoutdwm.rasi -dmenu -p "Select logout" -font "Caskaydia Cove Nerd Font 18")
	case $option in
	"")
		bspc wm -r
		;;
	"")
		sudo pkill dwm
		;;
	"")
		confirm=$(printf "Confirm reboot" | rofi -config $roficonf/logout.rasi -dmenu -font "Iosevka Curly 16")
		[[ $confirm == "Confirm reboot" ]] && reboot
		;;
	"⏻")
		confirm=$(printf "Confirm shut down" | rofi -config $roficonf/logout.rasi -dmenu -font "Iosevka Curly 16")
		[[ $confirm == "Confirm shut down" ]] && shutdown now
		;;
	esac
	;;
esac
