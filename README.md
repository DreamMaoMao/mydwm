![2024-01-19_16-56](https://github.com/DreamMaoMao/mydwm/assets/30348075/e2f543b9-78ca-46ff-8ac9-4c5a34d4547d)



https://github.com/DreamMaoMao/mydwm/assets/30348075/d5a898ad-a50b-4206-b161-08c1b5892c06



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

- 21.新增加xinput2 扩展输入事件支持,解决非根窗口鼠标移动事件无法捕获的问题

- 22.支持鼠标在非聚焦窗口内,不需要移出窗口再移动回来聚焦,只需要稍微动一下鼠标就能聚焦

- 23.支持真全屏窗口左下角依旧可以触发热区

- 24.增加切换tag方式,不管左右是否有窗口都可以直接切到相临前后的数字窗口
  




# 运行需要的相关工具包
```
sudo pacman -S network-manager-applet
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
sudo pacman -S networkmanager 
sudo pacman -S gdm
sudo pacman -S nemo nemo-fileroller
sudo pacman -S ffmpegthumbnailer

yay -S blueman acpi dunst jq alsa-utils polkit-gnome  rofi-blocks-git light numlockx  flameshot feh lm_sensors   mantablockscreen network-manager-applet playerctl python3 parcellite redshift upower xorg-xinit xprintidle xorg wmctrl xdotool tumbler pavucontrol ttf-jetbrains-mono-nerd eww-x11 cpufrequtils asciidoc xclip update-grub os-prober efibootmgr grub-customizer yazi xface4-terminal


```
# 安装特定版本的picom
```
cd ~/Downloads
git clone https://github.com/DreamMaoMao/mypicom.git
cd mypicom/
meson --buildtype=release . build --prefix=/usr 
sudo ninja -C build install
```


# 安装dwm
```
cd ~/.config
git clone https://github.com/DreamMaoMao/mydwm.git
mv mydwm dwm && cd dwm
cp rofi -r ~/.config/
cp dunst -r ~/.config/
cp fish -r ~/.config/
cp konsole -r ~/.local/share/
cp eww -r ~/.config/
cp parcellite -r ~/.config/
cp redshift.conf -r ~/.config/
sed -i s#/home/user#$HOME# dwm.desktop
sudo cp dwm.desktop /usr/share/xsessions/

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
`建议不要用带有cpp格式化的编辑器编辑,不然格式化后可能会自动换行导致配置代码结构非常混乱`
![image](https://github.com/DreamMaoMao/superdwm/assets/30348075/c71bb970-bbd5-4421-b9bc-c4bd887ade92)


# 应用对应的nerd icon 可以在icons.h中设置
`左边是xprop 命令执行后点击所需要查询的窗口得到的wm_class,右边是nerd font icon`

![image](https://github.com/DreamMaoMao/superdwm/assets/30348075/e8a2a005-9f6d-413a-a887-8e5de8410100)

nerd icon 可以在这里查询: [nerd font icon](https://www.nerdfonts.com/cheat-sheet)

![image](https://github.com/DreamMaoMao/superdwm/assets/30348075/8c315ca4-a3fe-43bc-8bf4-029eb20be8a1)

# reference
https://github.com/yaocccc/dwm
