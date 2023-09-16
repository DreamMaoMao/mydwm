#! /bin/bash
# DWM自启动脚本 仅作参考
# 搭配 https://github.com/yaocccc/scripts 仓库使用 目录位置 $DWM/scripts
# 部分配置文件在 $DWM/scripts/config 目录下

_thisdir=$(
	cd $(dirname $0)
	pwd
)

settings() {
	#/usr/bin/picom --daemon &
	picom --experimental-backends --config $DWM/scripts/config/picom.conf >>/dev/null 2>&1 & # 开启picom
	feh --randomize --bg-fill $DWM/wallpaper/caoyuan.jpg & # 壁纸
	dunst -config ~/.config/dunst/hypr_dunstrc &     # 开启通知server
	echo "Xft.dpi: 140" | xrdb -merge
	redshift &
	parcellite &
	blueman-applet &
	nm-applet &
	fcitx & # 开启输入法
	/usr/lib/polkit-gnome/polkit-gnome-authentication-agent-1  &
	numlockx on
	$_thisdir/statusbar/statusbar.sh cron & # 开启状态栏定时更新


}

daemons() {
	[ $1 ] && sleep $1
	xss-lock -- $DWM/scripts/blurlock.sh & # 开启自动锁屏程序
}


settings 1 & # 初始化设置项
daemons 3 &  # 后台程序项
