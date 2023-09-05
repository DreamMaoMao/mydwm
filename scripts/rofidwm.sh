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
	option=$(printf "\n\n⏻\n\n󰌾\n󰒲\n" |
		/usr/bin/rofi -config $roficonf/logoutdwm.rasi -dmenu -p "Select logout" -font "JetBrainsMono Nerd Font 18")
	case $option in
	"")
		sudo reboot
		;;
	"")
		sudo pkill dwm
		;;
	"")
		sudo systemclt suspend
		;;
	"⏻")
		sudo shutdown -h now
		;;
	"󰌾")
		$DWM/scripts/blurlock.sh
		;;
	"󰒲")
		sudo systemctl hibernate
		;;
	esac
	;;
esac
