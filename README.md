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

# 改动日志

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

- 11.增加左键点击布局图表切换到网格布局,右键切换布局是从顶入栈还是从底入栈 

- 12.增加浮动和全屏窗口在overview模式会自动退出浮动和全屏参与平铺,退出overview模式自动还原浮动和全屏的状态

- 13.增加全屏状态只有在当前tag 新打开窗口才会退出参与平铺,其他情况不自动退出全屏

- 14.增加假全屏模式(保留之前的真全屏),假全屏不会占据整个屏幕,而只会像一个tag中只有一个窗口的一样遮蔽其他窗口,保留bar和间隙,此状态仍然可以用鼠标左下角触发overview

- 15.新增border 颜色标识窗口的不同状态,灰色:非选中,橙色:普通选中,紫色:全局且选中,绿色:假全屏选中,红色:假全屏且全局且选中

- 16.增加tag可以循环切换
  
- 17.优化bar栏在各个布局下隐藏和显示后窗口的重新适应屏幕

- 18.增加鼠标中键选中taskbar可以切换完整标题和部分标题

- 19.增加当鼠标在bar的tasktitle部分移动的时候,窗口焦点会自动移动到title对应的窗口上

- 20.增加taskbar部分标题 由icon代替,icon字样可以在icons.h中设置

![image](https://github.com/DreamMaoMao/superdwm/assets/30348075/357a1c44-0a01-422c-821d-e6ce5e24bbaa)

![image](https://github.com/DreamMaoMao/superdwm/assets/30348075/5be684bb-9ed6-454e-b715-1297f43d98e8)

![image](https://github.com/DreamMaoMao/superdwm/assets/30348075/6c434c0f-09ff-4a45-94c0-592d60ee91b4)



https://github.com/DreamMaoMao/superdwm/assets/30348075/b89478fd-f1d4-42c4-9fff-675b41af3efc






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
sudo pacman -S base-devel
sudo pacman -S meson ninja
sudo pacman -S inetutils 
sudo pacman -S  networkmanager 
sudo pacman -S  gdm

yay -S blueman acpi dunst jq alsa-utils polkit-gnome  rofi-blocks-git light numlockx nemo flameshot feh lm_sensors   mantablockscreen network-manager-applet playerctl python3 parcellite redshift upower xorg-xinit xprintidle xorg wmctrl xdotool tumbler pavucontrol ttf-jetbrains-mono-nerd eww-x11


```
# 安装特定版本的picom
```
cd ~/Downloads
git clone https://github.com/DreamMaoMao/picom.git
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
cp eww -r ~/.config/
cp parcellite -r ~/.config/
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
# service
```
sudo systemctl start bluetooth

sudo systemctl enable bluetooth

sudo systemctl enable gdm

sudo systemctl enable NetworkManager

```


# 按键配置修改请查看config.h里的注释

# reference
https://github.com/yaocccc/dwm
