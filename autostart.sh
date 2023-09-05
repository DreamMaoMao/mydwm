#! /bin/bash
# DWM自启动脚本 仅作参考
# 搭配 https://github.com/yaocccc/scripts 仓库使用 目录位置 ~/scripts
# 部分配置文件在 ~/scripts/config 目录下

_thisdir=$(
	cd $(dirname $0)
	pwd
)

settings() {
	#/usr/bin/picom --daemon &
	picom --experimental-backends --config ~/scripts/config/picom.conf >>/dev/null 2>&1 & # 开启picom
	#python /home/wrq/scripts/hotarea.py &
	feh --randomize --bg-fill ~/Images/caoyuan.jpg & # 壁纸
	dunst -config ~/.config/dunst/hypr_dunstrc &     # 开启通知server
	systemctl --user unmask xdg-desktop-portal-gnome
	systemctl --user mask xdg-desktop-portal-hyprland
	/usr/libexec/xdg-desktop-portal &
	# xmodmap -e "pointer = 1 25 3 4 5 6 7 2"
	echo "Xft.dpi: 140" | xrdb -merge
	redshift &
	parcellite &
	blueman-applet &
	nm-applet &
	cp /home/wrq/.config/fcitx/dwm_profile /home/wrq/.config/fcitx/profile -f
	fcitx & # 开启输入法
	bash /opt/apps/com.baidu.fcitx-baidupinyin/files/bin/bd-qimpanel.watchdog.sh &
	/usr/local/libexec/polkit-gnome-authentication-agent-1 &
	numlockx on
	$_thisdir/statusbar/statusbar.sh cron & # 开启状态栏定时更新
	[ -e /dev/sda1 ] && udisksctl mount -b /dev/sda4
	python3 /home/wrq/tool/sign.py &

	#[ $1 ] && sleep $1
	#xset -b                     # 关闭蜂鸣器
	#syndaemon -i 1 -t -K -R -d  # 设置使用键盘时触控板短暂失效
	#~/scripts/set_screen.sh two # 设置显示器

}

daemons() {
	[ $1 ] && sleep $1
	xss-lock -- ~/scripts/blurlock.sh & # 开启自动锁屏程序
	lemonade server &                   # 开启lemonade 远程剪切板支持
	#	flameshot &                         # 截图要跑一个程序在后台 不然无法将截图保存到剪贴板
}

#cron() {
#	[ $1 ] && sleep $1
#	let i=10
#	while true; do
#		[ $((i % 10)) -eq 0 ] && ~/scripts/set_screen.sh check # 每10秒检查显示器状态 以此自动设置显示器
#		sleep 10
#		let i+=10
#	done
#}

settings 1 & # 初始化设置项
daemons 3 &  # 后台程序项
#cron 5 &     # 定时任务项
