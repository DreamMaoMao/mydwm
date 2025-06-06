#! /bin/bash
# 电池电量
# 需要安装acpi或者upower

tempfile=$(cd $(dirname $0);cd ..;pwd)/temp

this=_bat
icon_color="^c#ddca9e^^b#201b140xff^"
text_color="^c#ddca9e^^b#201b140xff^"
signal=$(echo "^s$this^" | sed 's/_//')

get_by_acpi() {
    [ ! "$(command -v acpi)" ] && echo command not found: acpi && return
    bat_text=$(acpi -b | sed '2,$d' | awk '{print $4}' | grep -Eo "[0-9]+")
    [ ! "$bat_text" ] && bat_text=$(acpi -b | sed '2,$d' | awk -F'[ %]' '{print $5}' | grep -Eo "[0-9]+")
    [ ! "$(acpi -b | grep 'Battery 0' | grep Discharging)" ] && 
    [ -n "$(acpi -a | grep on-line)" ] && charge_icon=" "
    _time="可用时间: $(acpi | sed 's/^Battery 0: //g' | awk -F ',' '{print $3}' | sed 's/^[ ]//g' | awk '{print $1}')"
    [ "$_time" = "可用时间: " ] && _time=""
}

get_by_upower() {
    [ -n "$bat_text" ] && [ $bat_text -gt 0 ] && return
    [ ! "$(command -v upower)" ] && echo command not found: upower && return
    bat=$(upower -e | grep BAT)
    bat_text=$(upower -i $bat | awk '/percentage/ {print $2}' | grep -Eo '[0-9]+')
    [ -n "$(upower -i $bat | grep 'state:.*fully-charged')" ] && charge_icon=" "
}

update() {
    get_by_acpi
    get_by_upower
    [ -z $bat_text ] && bat_text=0
    if   [ "$bat_text" -ge 95 ]; then bat_icon=""; charge_icon="";
    elif [ "$bat_text" -ge 90 ]; then bat_icon="";
    elif [ "$bat_text" -ge 80 ]; then bat_icon="";
    elif [ "$bat_text" -ge 70 ]; then bat_icon="";
    elif [ "$bat_text" -ge 60 ]; then bat_icon="";
    elif [ "$bat_text" -ge 50 ]; then bat_icon="";
    elif [ "$bat_text" -ge 40 ]; then bat_icon="";
    elif [ "$bat_text" -ge 30 ]; then bat_icon="";
    elif [ "$bat_text" -ge 20 ]; then bat_icon="";
    elif [ "$bat_text" -ge 10 ]; then bat_icon="";
    else bat_icon=""; fi

    icon=" $charge_icon$bat_icon"
    text="$bat_text% "

    sed -i '/^export '$this'=.*$/d' $tempfile
    printf "export %s='%s%s%s%s%s'\n" $this "$signal" "" "" "$text_color" "$icon $text|" >> $tempfile
}

notify() {
    update
    notify-send "$bat_icon Battery" "\n剩余: $bat_text%\n$_time" -r 9527
}

click() {
    case "$1" in
        L) $DWM/scripts/rofidwm.sh outopts;;
        R) eww  open-many --toggle background-closer system-menu;;
    esac
}

case "$1" in
    click) click $2 ;;
    notify) notify ;;
    *) update ;;
esac
