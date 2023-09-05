# 之前就有的功能


- 支持布局 tile(磁块)、magicgrid(进阶的网格布局)

- 键盘移动/调整窗口大小 且移动/调整时有窗口间吸附效果

- 窗口隐藏

- 窗口可自定义是否全局(在所有tag内展示)

- 更好的浮动窗口支持

- 优化后的status2d 状态栏，可用鼠标点击操作

- 系统托盘支持

- overview 概述

- mod + tab, 在窗口间切换 有浮动窗口时仅在浮动窗口切换

- mod + [tag], 切换tag到指定目录时 可指定一个cmd，若目标tag无窗口 则执行该tag

# 改动 

- 1.在有全屏窗口下打开一个非悬浮窗口，全屏窗口会自动退出全屏参与平铺，原版全屏会遮住后面打开的窗口

- 2.全屏状态下退出窗口，状态栏依旧会在，原版全屏窗口退出之后，状态栏会消失

- 3.动画优化，左tag的窗口会从左滑回来，右tag的窗口会从右边滑回来，原版都只能从左边滑回来

- 4.bar栏优化，这个改动太多，不好说，直观就是外观颜色的变化和多了网速查看和亮度控制功能

- 5.增加热区功能，鼠标移动到左下角可以全局触发预览视图，并且可以用鼠标左键点击窗口跳转到窗口所在tag，右键在overview下关闭窗口

- 6.增加鼠标中键点击窗口触发窗口全屏切换的功能

- 7.增加鼠标滑轮加super按键可以切换工作区功能

- 8.增加单窗口barder可显示功能

- 9.增加浮动窗口大小设置功能

- 10.右边数字键盘keycode转keysym异常修复,修正x11无法使用右边的数字键盘设置快捷键

- 11.增加左键点击布局图表切换到网格布局,右键切换栈布局是从上入栈还是从下入栈 

![image](https://github.com/DreamMaoMao/superdwm/assets/30348075/cb62f0dc-088c-4ffd-a1c8-d0d3c82a2142)



https://github.com/DreamMaoMao/superdwm/assets/30348075/b6b0291a-1e5a-4be0-906e-9094839a68b3






# 运行需要的相关工具包
```
sudo pacman -S network-manager-applet
sudo pacman -S nemo
sudo pacman -S rofi
sudo pacman -S konsole
sudo pacman -S gnome-system-monitor 
sudo pacman -S fcitx-qt5 fcitx fcitx-configtool
sudo pacman -S xorg-xrdb
sudo pacman -S brightnessctl 
sudo pacman -S bluez bluez-utils
sudo pacman -S sysstat
sudo pacman -S xorg-xsetroot
sudo pacman -S xss-lock 
sudo pacman -S libpulse
sudo pacman -S fish
sudo pacman -Sy base-devel


yay -S acpi dunst jq alsa-utils polkit-gnome  rofi-blocks-git light numlockx nemo flameshot feh lm_sensors i3lock i3lock-color mantablockscreen network-manager-applet playerctl python3 parcellite redshift upower xorg-xinit xprintidle xorg wmctrl xdotool tumbler


```
# 安装特定版本的picom
```
cd ~/Downloads
git clone https://github.com/yaocccc/picom.git
cd picom/
meson --buildtype=release . build --prefix=/usr -Dwith_docs=true
sudo ninja -C build install
```


# 安装dwm
```
cd ~/.config
git clone https://github.com/DreamMaoMao/superdwm.git
cd superdwm
cp rofi -r ~/.config/
cp dunst -r ~/.config/
cp fish -r ~/.config/
cp konsole -r ~/.local/share/
sed -i s#/home/user#$HOME# dwm.desktop
sudo cp dwm.desktop /usr/share/xsession/

sudo make clean install
```

# 终端设置(默认用的konsole)
```
chsh -s /usr/bin/fish

git clone https://github.com/oh-my-fish/oh-my-fish
cd oh-my-fish
bin/install --offline
omf install bira



```

# 按键配置修改请查看config.h里的注释

# reference
https://github.com/yaocccc/dwm
