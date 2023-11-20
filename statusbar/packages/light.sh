#!/usr/bin/bash
# 亮度脚本

tempfile=$(cd $(dirname $0);cd ..;pwd)/temp

this=_light
icon_color="^c#442266^^b#dedbd40xff^"
text_color="^c#442266^^b#dedbd40xff^"
signal=$(echo "^s$this^" | sed 's/_//')

# check
[ ! "$(command -v brightnessctl)" ] && echo command not found: brightnessctl && exit

update() {
    light=$( brightnessctl get )
    light_text=$( echo "${light##* }" | sed 's/[^0-9][^.]*//g' )
    if (( $light_text < 50 ));then 
        light_icon="";
    else 
        light_icon="󱩎"; 
    fi

    icon=" $light_icon"
    text="$light_text% "

    sed -i '/^export '$this'=.*$/d' $tempfile
    printf "export %s='%s%s%s%s%s'\n" $this "$signal" "" "" "$text_color" "$icon $text" >> $tempfile
}

notify() {
    update
    notify-send -r 9527 -h int:value:$vol_text -h string:hlcolor:#dddddd "$vol_icon Volume"
}

blueman() {
    blueman-manager
}

click() {
    case "$1" in
        L)  ;; # 打开设置面板
        M)  ;; # 
        R)  ;; #
        U) brightnessctl set +1%; ;; # 亮度加
        D) brightnessctl set 1%-; ;; # 亮度减
    esac
}

case "$1" in
    click) click $2 ;;
    notify) notify ;;
    *) update ;;
esac
