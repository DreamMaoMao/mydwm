#!/usr/bin/bash



INTERNAL_MONITOR=$(xrandr -q | grep primary | cut -d " " -f 1)
EXTERNAL_MONITOR=$(xrandr -q | grep connected | grep -v primary | cut -d " " -f 1)


xrandr --output $INTERNAL_MONITOR --off

sudo pkill feh
feh --randomize --bg-fill $DWM/wallpaper/caoyuan.jpg & 