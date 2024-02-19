#! /bin/bash
# DWM自启动脚本 仅作参考
# 搭配 https://github.com/yaocccc/scripts 仓库使用 目录位置 $DWM/scripts
# 部分配置文件在 $DWM/scripts/config 目录下

_thisdir=$(
	cd $(dirname $0)
	pwd
)

settings() {
	pkill -f xdg-desktop-portal-hyprland
	pkill -f xdg-desktop-portal-gnome
	pkill clash
	picom --experimental-backends --config $DWM/scripts/config/picom.conf & # 开启picom
	# picom --config $DWM/scripts/config/picom.conf &
	feh --randomize --bg-fill $DWM/wallpaper/caoyuan.jpg & # 壁纸
	echo "Xft.dpi: 140" | xrdb -merge                      #dpi缩放
	cp ~/.local/share/x11/konsole -r ~/.local/share/
	cp ~/.config/fcitx/dwm_profile ~/.config/fcitx/profile -f
	# 开启输入法
	fcitx &
	# bash /opt/apps/com.baidu.fcitx-baidupinyin/files/bin/bd-qimpanel.watchdog.sh &	#百度拼音输入法
	dunst -config ~/.config/dunst/dwm_dunstrc & # 开启通知server
	systemctl --user unmask xdg-desktop-portal-gnome
	systemctl --user mask xdg-desktop-portal-hyprland
	/usr/libexec/xdg-desktop-portal &
	redshift &
	parcellite &
	blueman-applet &
	nm-applet &
	/usr/lib/polkit-gnome/polkit-gnome-authentication-agent-1 &
	numlockx on
	$_thisdir/statusbar/statusbar.sh cron & # 开启状态栏定时更新
	[ -e /dev/sda1 ] && udisksctl mount -t ntfs -b /dev/sda4
	python3 ~/tool/sign.py &
	cp ~/.config/eww/System-Menu/eww.yuck.x11  ~/.config/eww/System-Menu/eww.yuck
	eww daemon &
	plank &
}

daemons() {
	[ $1 ] && sleep $1
	xss-lock -- $DWM/scripts/blurlock.sh & # 开启自动锁屏程序
}

settings 1 & # 初始化设置项
#daemons 3 &  # 后台程序项
