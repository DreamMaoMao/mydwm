#!/usr/bin/env python

import sys
import subprocess

websitename = [
           "助手~~~ai\0icon\x1f~/.config/rofi/icons/newbing.png\n",
           "无站~~~bt\0icon\x1f~/.config/rofi/icons/btnull.jpeg\n",
           "B站~~~bl\0icon\x1f~/.config/rofi/icons/bilibili.png\n",
           "飞极~~~fj\0icon\x1f~/.config/rofi/icons/feijisu.png\n",
           "豆瓣~~~db\0icon\x1f~/.config/rofi/icons/douban.png\n",
           "鸭子~~~dg\0icon\x1f~/.config/rofi/icons/ddg.png\n",           
           "谷歌~~~go\0icon\x1f~/.config/rofi/icons/google.png\n",
           "必应~~~bi\0icon\x1f~/.config/rofi/icons/bing.png\n",
           "咪咕~~~mg\0icon\x1f~/.config/rofi/icons/migu.png\n",
           "油管~~~yt\0icon\x1f~/.config/rofi/icons/youtube.png\n",
            "饭桶~~~gi\0icon\x1f~/.config/rofi/icons/github.png\n",
           "百度~~~bd\0icon\x1f~/.config/rofi/icons/baidu.png\n",
           "音磁~~~hf\0icon\x1f~/.config/rofi/icons/hifini.png\n"
           ]

websiteurl = {
           "助手":"https://www.bing.com/search?q=%E4%BD%A0%E5%A5%BD&PC=U316&FORM=CHROMN&showconv=1",
           "无站":"https://www.btnull.nu/",
           "B站":"https://bilibili.com/",
           "飞极":"http://fjisu.vip/",
           "豆瓣":"https://movie.douban.com/",
           "鸭子":"https://duckduckgo.com/",
           "谷歌":"https://google.com/",
           "必应":"https://bing.com/",
           "咪咕":"http://www.miguvideo.com/",
           "油管":"https://www.youtube.com/",
            "饭桶":"https://github.com/",
            "百度":"https://www.baidu.com/",            
           "音磁":"https://www.hifini.com/"}




if len(sys.argv) < 2:
    for web in websitename:
        print(web)
elif "~~~" in sys.argv[1]:
    datalist = sys.argv[1].split("~~~")
    url = websiteurl[datalist[0]]
    subprocess.run("nohup google-chrome  '{0}'  > /dev/null 2>&1 & ".format(url), shell=True)

