# dwm 

- 1.在有全屏窗口下打开一个非悬浮窗口，全屏窗口会自动退出全屏参与平铺，原版全屏会遮住后面打开的窗口

- 2.全屏状态下退出窗口，状态栏依旧会在，原版全屏窗口退出之后，状态栏会消失

- 3.动画优化，左tag的窗口会从左滑回来，右tag的窗口会从右边滑回来，原版都只能从左边滑回来

- 4.bar栏优化，这个改动太多，不好说，直观就是外观颜色的变化和多了网速查看和亮度控制功能

- 5.增加热区功能，鼠标移动到左下角可以全局触发预览视图，并且可以用鼠标左键点击窗口跳转到窗口所在tag，右键在overview下关闭窗口

- 6.增加鼠标中键触发全屏切换的功能

- 7.增加鼠标滑轮加super按键可以切换工作区功能

- 8.增加单窗口barder可显示功能

- 9.增加浮动窗口大小设置功能

- 10.右边数字键盘keycode转keysym异常修复,修正x11无法使用右边的数字键盘设置快捷键

![image](https://github.com/DreamMaoMao/superdwm/assets/30348075/cb62f0dc-088c-4ffd-a1c8-d0d3c82a2142)



https://github.com/DreamMaoMao/superdwm/assets/30348075/b6b0291a-1e5a-4be0-906e-9094839a68b3






# dependcy
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


# install
```
cd ~/.config
git clone https://github.com/DreamMaoMao/superdwm.git
cd superdwm
cp rofi -r ~/.config/
cp dunst -r ~/.config/
sed -i s#/home/user#$HOME# dwm.desktop
sudo cp dwm.desktop /usr/share/xsession/

sudo make clean install
```

# 按键配置请查看config.h里的注释

# reference
https://github.com/yaocccc/dwm
