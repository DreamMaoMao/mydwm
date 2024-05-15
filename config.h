#include <X11/XF86keysym.h>

static int const single_auto_exit_hide = 0; /*如果隐藏窗口是tag中唯一的窗口那么自动显示一个隐藏窗口*/
static int const auto_fullscreen = 0; /* overview模式左键点击退出overview后自动假全屏窗口*/
static int const mouse_move_toggle_focus = 1; /* 在非聚焦窗口移动鼠标是否触发聚焦,不启用的话只有从另一个窗口移动到当前窗口才聚焦*/
static int const taskbar_icon = 1;  /* 任务栏使用的是icon而不是title*/
static int const taskbar_icon_exclude_active = 1;  /* 任务栏活动窗口强制使用title*/
static int const taskbar_movemouse_focus = 0;           /* 是否启用鼠标在taskbar移动的时候自动切换焦点 */
static int const tag_circle = 0;           /* 是否启用工作区循环 */
static int const enable_hotarea = 1;           /* 是否启用热区 */
static int const hotarea_size = 10;           /* 热区大小10x10 */

static int const no_stack_show_border = 1;           /* 一个窗口也显示border */
static int showsystray = 1;           /* 是否显示托盘栏 */
static int newclientathead = 1; /* 定义新窗口在栈顶还是栈底 */
static const unsigned int borderpx = 5; /* 窗口边框大小 */
static const unsigned int systraypinning =
    1; /* 托盘跟随的显示器 0代表不指定显示器 */
static const unsigned int systrayspacing = 1;  /* 托盘间距 */
static const unsigned int systrayspadding = 1; /* 托盘和状态栏的间隙 */
static int gappi = 10; /* 窗口与窗口 缝隙大小 */
static int gappo = 10; /* 窗口与边缘 缝隙大小 */
static const int _gappo =
    10; /* 窗口与窗口 缝隙大小 不可变 用于恢复时的默认值 */
static const int _gappi =
    10; /* 窗口与边缘 缝隙大小 不可变 用于恢复时的默认值 */
static const int vertpad = 5;        /* vertical padding of bar */
static const int sidepad = 5;        /* horizontal padding of bar */
static const int overviewgappi = 24; /* overview时 窗口与边缘 缝隙大小 */
static const int overviewgappo = 60; /* overview时 窗口与窗口 缝隙大小 */
static const int showbar = 1;        /* 是否显示状态栏 */
static const int topbar = 1;         /* 指定状态栏位置 0底部 1顶部 */
static const float mfact = 0.6;      /* 主工作区 大小比例 */
static const int nmaster = 1;        /* 主工作区 窗口数量 */
static const unsigned int snap = 10;          /* 边缘依附宽度 */
static const unsigned int baralpha = 0xc0;    /* 状态栏透明度 */
static const unsigned int borderalpha = 0xdd; /* 边框透明度 */
static const char *fonts[] = {"JetBrainsMono Nerd Font:style=Bold:size=15",
                                };
static const char *colors[][3] = {
    /* 颜色设置 ColFg, ColBg, ColBorder */
    [SchemeNorm] = {"#ffb871", "#3c2003", "#444444"},
    [SchemeSel] = {"#2d1802", "#ad741f", "#ad741f"},
    [SchemeSelScratchpad] = {"#ffffff", "#37474F", "#516c93"},
    [SchemeSelGlobal] = {"#ffffff", "#37474F", "#b153a7"},
    [SchemeSelFakeFull] = {"#ffffff", "#37474F", "#5d8e5d"},
    [SchemeSelFakeFullGLObal] = {"#ffffff", "#37474F", "#881519"},
    [SchemeHid] = {"#462503", NULL, NULL},
    [SchemeSystray] = {NULL, "#dabb77", NULL},
    [SchemeUnderline] = {"#6f0d62", NULL, NULL},
    [SchemeNormTag] = {"#a02a6b", "#d8cbba", NULL},
    [SchemeSelTag] = {"#ffffff", "#a02a6b", NULL},
    [SchemeBarEmpty] = {NULL, "#111111", NULL},
};
static const unsigned int alphas[][3] = {
    /* 透明度设置 ColFg, ColBg, ColBorder */
    [SchemeNorm] = {OPAQUE, baralpha, borderalpha},
    [SchemeSel] = {OPAQUE, baralpha, borderalpha},
    [SchemeSelGlobal] = {OPAQUE, baralpha, borderalpha},
    [SchemeSelScratchpad] = {OPAQUE, baralpha, borderalpha},
    [SchemeSelFakeFull] = {OPAQUE, baralpha, borderalpha},
    [SchemeSelFakeFullGLObal] = {OPAQUE, baralpha, borderalpha},
    [SchemeNormTag] = {OPAQUE, baralpha, borderalpha},
    [SchemeSelTag] = {OPAQUE, baralpha, borderalpha},
    [SchemeBarEmpty] = {0, 0x11, 0},
    [SchemeStatusText] = {OPAQUE, 0x88, 0},
};


/* 自定义脚本位置 */
static const char *autostartscript = "$DWM/autostart.sh";
static const char *statusbarscript = "$DWM/statusbar/statusbar.sh";

/* 自定义tag名称 */
/* 自定义特定实例的显示状态 */

static const char *tags[] = { 
    "1", // tag:0  key:1  desc:
    "2", // tag:1  key:2  desc:
    "3", // tag:2  key:3  desc:qq
    "4", // tag:4  key:4  desc:chrome
    "5", // tag:5  key:5  desc:
    "6", // tag:6  key:6  desc:obs
    "7", // tag:7  key:7  desc:
    "8", // tag:8  key:8  desc:
    "9", // tag:9  key:9  desc:
};

/* 自定义窗口显示规则 */
/* class instance title 主要用于定位窗口适合哪个规则 */
/* tags mask 定义符合该规则的窗口的tag 0 表示当前tag */
/* isfloating 定义符合该规则的窗口是否浮动 */
/* isglobal 定义符合该规则的窗口是否全局浮动 */
/* isnoborder 定义符合该规则的窗口是否无边框 */
/* monitor 定义符合该规则的窗口显示在哪个显示器上 -1 为当前屏幕 */
/* floatposition 定义符合该规则的窗口显示的位置 0 中间，1到9分别为9宫格位置，例如1左上，9右下，3右上 */
/* width 悬浮窗口的宽度 */
/* high 悬浮窗口的高度 */


static const Rule rules[] = {
    /* class                 instance              title             tags mask     isfloating  isglobal  isnoborder monitor floatposition width high */
    /** 优先级高 越在上面优先度越高 */
    { NULL,                  NULL,                "图片查看器",        0,            1,          0,          0,        -1,      0,           0,       0},       // qq图片查看器        浮动
    { NULL,                  NULL,                "图片查看",          0,            1,          0,          0,        -1,      0,           0,       0},       // 微信图片查看器      浮动
    { NULL,                  NULL,                "未命名的播放列表",          0,            1,          0,          0,        -1,      5,           0,       0},       // 微信图片查看器      浮动


    /** 普通优先度 */
    {"obs",                  NULL,                 NULL,             1 << 5,       0,          0,          0,        -1,      0,            0,       0},       // obs        tag6 
    {"Google-chrome",               NULL,                 NULL,             1 << 3,       0,          0,          0,        -1,      0,            0,       0},       // chrome     tag4 
    {"Microsoft-edge",               NULL,                 NULL,             1 << 4,       0,          0,          0,        -1,      0,            0,       0},       // chrome     tag4 
    {"qtmv",                NULL,                 NULL,             0,            1,          0,          0,        -1,      5,            1200,       800},  // music      浮动
    {"yesplaymusic",                NULL,                 NULL,             0,            1,          0,          0,        -1,      5,            1570,       1010},  // music      浮动
    {"electron-netease-cloud-music",                NULL,                 NULL,             0,            1,          0,          0,        -1,      5,     1200,   800},  // music      浮动
    {"baidunetdisk",                NULL,                 NULL,             0,            1,          0,          0,        -1,      5,            0,       0},  // 百度云网盘      浮动
    {"alixby3",                NULL,                 NULL,             0,            1,          0,          0,        -1,      5,            1200,       800},  // 阿里云网盘      浮动
    {"QQ",                  NULL,                  NULL,             1 << 2,       0,          0,          0,        -1,      0,            0,       0},       // qq         tag3 
    {"flameshot",            NULL,                 NULL,             0,            1,          0,          0,        -1,      0,            0,       0},       // 火焰截图            浮动
    {"Blueman-manager",      NULL,                 NULL,             0,            1,          0,          0,        -1,      5,            0,       0},       // blueman            浮动
    {"com.xunlei.download",              NULL,                 NULL,             0,            1,          0,          0,        -1,      5,            600,       900},       // 迅雷            浮动
    {"Clash-verge",    NULL,                 NULL,             0,            1,          0,          0,        -1,      5,            1590,       892},       // clash            浮动
    {"Pavucontrol",                  NULL,                 NULL,             0,            1,          0,          0,        -1,      5,            0,       0},       // pavucontrol            浮动
    {"Dragon",    NULL,                 NULL,             0,            1,          0,          1,        -1,      5,            0,       0},       // dragon            noborder
    {"qxdrag.py",    NULL,                 NULL,             0,            1,          0,          0,        -1,      5,            400,       300},       // dragon            noborder
    {NULL,    NULL,                 "rofi - Networks",             0,            1,          0,          1,        -1,      5,            400,       300},       // dragon            noborder


    /** 优先度低 越在上面优先度越低 */
    { NULL,                  NULL,                "crx_",            0,            1,          0,          0,        -1,      0,            0,        0}, // 错误载入时 会有crx_ 浮动
    { NULL,                  NULL,                "broken",          0,            1,          0,          0,        -1,      0,            0,        0}, // 错误载入时 会有broken 浮动
};
static const char *overviewtag = "OVERVIEW";
static const Layout overviewlayout = { "󰃇",  overview };

/* 自定义布局  */
static const Layout layouts[] = {
    { "󱞬",  tile },         /* 主次栈 */
    { "﩯",  magicgrid },    /* 网格 */
    { "󱟀",  rtile },         /* 主次栈,尾部入栈 */
};

#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }
#define AltMask Mod1Mask
#define SuperMask Mod4Mask
#define TAGKEYS(KEY, TAG, cmd) \
    { ControlMask,              KEY, view,       {.ui = 1 << TAG, .v = cmd} }, \
    { AltMask,    KEY, tag,        {.ui = 1 << TAG} }, \
    { SuperMask|ControlMask,  KEY, toggleview, {.ui = 1 << TAG} }, \

static Key keys[] = {
    /* modifier                 key              function          argument */
    { SuperMask,                XK_equal,        togglesystray,    {0} },                     /* super +            |  切换 托盘栏显示状态 */

    { SuperMask,                XK_Tab,          focusstack,       {.i = +1} },               /* super tab          |  本tag内切换聚焦窗口 */
    { SuperMask|ShiftMask,      XK_Tab,          focusstack,       {.i = -1} },               /* super shift tab    |  本tag内切换聚焦窗口 */
    { ControlMask,              XK_Left,         viewtoleft,       {0} },                     /* ctrl left          |  聚焦到左边有窗口的tag */
    { ControlMask,              XK_Right,        viewtoright,      {0} },                     /* ctrl right         |  聚焦到右边有窗口的tag */
    { ControlMask,              XK_Up,           viewtoleft,       {0} },                     /* ctrl up            |  聚焦到左边有窗口的tag */
    { ControlMask,              XK_Down,         viewtoright,      {0} },                     /* ctrl down          |  聚焦到右边有窗口的tag */
    { SuperMask,                XK_Left,         addtoleft,        {0} },                     /* super left         |  聚焦到左边的tag */
    { SuperMask,                XK_Right,        addtoright,       {0} },                     /* super right        |  聚焦到右边的tag */
    { SuperMask,                XK_Up,           addtoleft,        {0} },                     /* super up           |  聚焦到左边的tag */
    { SuperMask,                XK_Down,         addtoright,       {0} },                     /* super down         |  聚焦到右边的tag */
    { ControlMask|SuperMask,    XK_Left,         tagtoleft,        {0} },                     /* ctrl alt left      |  将本窗口移动到左边tag */
    { ControlMask|SuperMask,    XK_Right,        tagtoright,       {0} },                     /* ctrl alt right     |  将本窗口移动到右边tag */

    { AltMask,                  XK_Tab,            toggleoverview,   {0} },                     /* alt tab           |  显示所有tag 或 跳转到聚焦窗口的tag */
    { SuperMask,                XK_comma,        setmfact,         {.f = -0.05} },            /* super ,            |  缩小主工作区 */
    { SuperMask,                XK_period,       setmfact,         {.f = +0.05} },            /* super .            |  放大主工作区 */

    { SuperMask,                  XK_i,            hidewin,                 {0} },                     /* super i            |  隐藏窗口,放入便签 */
    { SuperMask|ShiftMask,        XK_i,            restorewin,              {0} },                     /* super shift i      |  取消隐藏 窗口 */
    { AltMask,                    XK_z,            toggle_scratchpad,       {0} },                     /* alt z             |  标签的循环切换 */

    { AltMask,    XK_s,         zoom,             {0} },                                      /* alt s              |  将当前聚焦窗口置为主窗口 */

    { AltMask,                  XK_backslash,    togglefloating,   {0} },                     /* alt \            |  开启/关闭 聚焦目标的float模式 */
    { AltMask|ShiftMask,        XK_backslash,    toggleallfloating,{0} },                     /* alt shift \      |  开启/关闭 全部目标的float模式 */
    { AltMask,                  XK_a,            fake_fullscreen,       {0} },                /* alt a              |  开启/关闭 假全屏      */
    { AltMask,                  XK_f,            fullscreen,       {0} },                     /* alt f              |  开启/关闭 真全屏   */
    { SuperMask,                XK_h,            togglebar,        {0} },                     /* super h            |  开启/关闭 状态栏 */
    { SuperMask,                XK_g,            toggleglobal,     {0} },                     /* super g            |  开启/关闭 全局 */
    { SuperMask,                XK_u,            toggleborder,     {0} },                     /* super u            |  开启/关闭 边框 */
    { SuperMask,                XK_e,            incnmaster,       {.i = +1} },               /* super e            |  改变主工作区窗口数量 (1 2中切换) */

    { SuperMask,                XK_j,            focusmon,         {.i = +1} },               /* super j            |  光标移动到另一个显示器 */
    { SuperMask|ShiftMask,      XK_j,            tagmon,           {.i = +1} },               /* super shift j      |  将聚焦窗口移动到另一个显示器 */

    { ControlMask,              XK_0,            view,             {.ui= ~0} },               /* ctrl 0             |  显示所有workspace */
    { ControlMask,              XK_KP_0,         view,             {.ui= ~0} },               /* ctrl 0(右边数字键)   |  显示所有workspace */
    { AltMask,                  XK_0,            tag,              {.ui= ~0} },               /* alt 0              |  窗口在所有workspace都显示 */
    { AltMask,                  XK_KP_0,         tag,              {.ui= ~0} },               /* alt 0(右边数字键)    |  窗口在所有workspace都显示 */

    { AltMask,                  XK_q,            killclient,       {0} },                     /* alt q              |  关闭窗口 */
    { AltMask|ControlMask,      XK_q,            forcekillclient,  {0} },                     /* alt ctrl q         |  强制关闭窗口(处理某些情况下无法销毁的窗口) */
    { SuperMask,                XK_m,            quit,             {0} },                     /* super m            |  退出dwm */

	{ SuperMask,                XK_n,            selectlayout,     {.v = &layouts[1]} },      /* super n            |  切换到网格布局 */
	{ SuperMask|ShiftMask,      XK_n,            selectlayout,     {.v = &layouts[2]} },      /* shift super n      |  切换到栈布局入栈方式 */
	{ SuperMask,                XK_o,            showonlyorall,    {0} },                     /* super o            |  切换 只显示一个窗口 / 全部显示 */

    { SuperMask|ControlMask,    XK_equal,        setgap,           {.i = -6} },               /* super ctrl +       |  窗口增大 */
    { SuperMask|ControlMask,    XK_minus,        setgap,           {.i = +6} },               /* super ctrl -       |  窗口减小 */
    { SuperMask|ControlMask,    XK_space,        setgap,           {.i = 0} },                /* super ctrl space   |  窗口重置 */

    { AltMask|ControlMask,      XK_Up,           movewin,          {.ui = UP} },              /* alt ctrl up      |  移动窗口 */
    { AltMask|ControlMask,      XK_Down,         movewin,          {.ui = DOWN} },            /* alt ctrl down    |  移动窗口 */
    { AltMask|ControlMask,      XK_Left,         movewin,          {.ui = LEFT} },            /* alt ctrl left    |  移动窗口 */
    { AltMask|ControlMask,      XK_Right,        movewin,          {.ui = RIGHT} },           /* alt ctrl right   |  移动窗口 */

    { SuperMask|AltMask,        XK_Up,           resizewin,        {.ui = V_REDUCE} },        /* super alt up       |  调整窗口大小 */
    { SuperMask|AltMask,        XK_Down,         resizewin,        {.ui = V_EXPAND} },        /* super alt down     |  调整窗口大小 */
    { SuperMask|AltMask,        XK_Left,         resizewin,        {.ui = H_REDUCE} },        /* super alt left     |  调整窗口大小 */
    { SuperMask|AltMask,        XK_Right,        resizewin,        {.ui = H_EXPAND} },        /* super alt right    |  调整窗口大小 */

         /* alt l              | 二维聚焦窗口 */
    { AltMask,                  XK_Left,         focusdir,         {.i = LEFT } },            /* alt left           |  本tag内切换聚焦窗口 */
    { AltMask,                  XK_Right,        focusdir,         {.i = RIGHT } },           /* alt right          |  本tag内切换聚焦窗口 */
    { AltMask,                  XK_Up,           focusdir,         {.i = UP } },              /* alt up             |  本tag内切换聚焦窗口 */
    { AltMask,                  XK_Down,         focusdir,         {.i = DOWN } },            /* alt down           |  本tag内切换聚焦窗口 */

    { SuperMask|ShiftMask,        XK_Up,           exchange_client,  {.i = UP } },              /* super shift up       | 二维交换窗口 (仅平铺) */
    { SuperMask|ShiftMask,        XK_Down,         exchange_client,  {.i = DOWN } },            /* super shift down     | 二维交换窗口 (仅平铺) */
    { SuperMask|ShiftMask,        XK_Left,         exchange_client,  {.i = LEFT} },             /* super shift left     | 二维交换窗口 (仅平铺) */
    { SuperMask|ShiftMask,        XK_Right,        exchange_client,  {.i = RIGHT } },           /* super shift right    | 二维交换窗口 (仅平铺) */

    /* spawn + SHCMD 执行对应命令(已下部分建议完全自己重新定义) */
    { AltMask,                  XK_Return, spawn, SHCMD("xfce4-terminal") },  
    { SuperMask,                XK_Return, spawn, SHCMD("google-chrome") },
    { ControlMask,              XK_Return, spawn, SHCMD("bash ~/tool/clash.sh") },                                                                                              /* alt enter      | 打开终端             */
    { SuperMask,                XK_d,      spawn, SHCMD("/usr/bin/rofi -config ~/.config/rofi/themes/trans.rasi -show run") },                                                       /* super d          | rofi: 执行run          */
    { AltMask,                  XK_space,  spawn, SHCMD("/usr/bin/rofi -config ~/.config/rofi/themes/trans.rasi -show drun") },                                                      /* alt space        | rofi: 执行drun          */
    { SuperMask|ControlMask,    XK_Return, spawn, SHCMD("konsole -e  yazi") },                                                                                                          /* ctrl win enter   | rofi: nautilus 文件浏览器          */
    { ControlMask,              XK_space,  spawn, SHCMD("rofi -theme ~/.config/rofi/themes/fancy2.rasi -modi blocks -show blocks -blocks-wrap ~/.config/rofi/search.py") },              /* ctrl space       | rofi: 执行自定义脚本   */
    { SuperMask,                XK_space,  spawn, SHCMD("/usr/bin/rofi -config ~/.config/rofi/themes/trans.rasi -show website") },     /* super space      | rofi: 执行自定义脚本   */
    { SuperMask|AltMask,        XK_Return, spawn, SHCMD("rofi -theme ~/.config/rofi/themes/fancy2.rasi -modi blocks -show blocks -blocks-wrap ~/tool/movie.py") },     /* super space      | rofi: 执行自定义脚本   */
    { SuperMask,                XK_l,      spawn, SHCMD("$DWM/scripts/blurlock.sh") },                                   /* super l     | 锁定屏幕               */
    { AltMask,                  XK_period, spawn, SHCMD("$DWM/scripts/volume.sh up") },                                 /* alt >       | 音量加                 */
    { AltMask,                  XK_comma,  spawn, SHCMD("$DWM/scripts/volume.sh down") },                               /* alt <       | 音量减                 */
    { ControlMask,              XK_period, spawn, SHCMD("$DWM/scripts/brightness.sh up") },                             /* ctrl >      | 亮度加                 */
    { ControlMask,              XK_comma,  spawn, SHCMD("$DWM/scripts/brightness.sh down") },                           /* ctrl <      | 亮度减    */
    { AltMask|ControlMask,      XK_a,      spawn, SHCMD("flameshot gui") },             /* ctrl alt a  | 截图                   */
    { AltMask|SuperMask,        XK_q,      spawn, SHCMD("kill -9 $(xprop | grep _NET_WM_PID | awk '{print $3}')") }, /* super alt q | 选中某个窗口并强制kill */
    { SuperMask,                XK_p,      spawn, SHCMD("bash $DWM/scripts/monitor.sh") },                              /* super p     | 关闭内部显示器 */
    { SuperMask|ControlMask,    XK_m,      spawn, SHCMD("$DWM/scripts/rofidwm.sh outopts") },
    // { AltMask|ControlMask,      XK_Return, spawn, SHCMD("konsole -e \"zellij -s temp --config /home/wrq/.config/zellij/tempconfigx11.kdl\"") },  /* super alt return | zellij 临时会话 */
    { AltMask|ControlMask,      XK_Return, spawn, SHCMD("konsole -e \"~/tool/ter-multiplexer.sh\"") },  /* super alt return | zellij 临时会话 */
    { SuperMask,                XK_b,      spawn, SHCMD("systemctl suspend") },      /* super b     | 系统挂起 */ 
    { AltMask|ControlMask,      XK_t,      spawn, SHCMD("bash ~/tool/shotTranslate.sh shot") },      /* ctrl alt t    | 截屏翻译 */ 
    { SuperMask,                XK_c,      spawn, SHCMD("rofi -theme  ~/.config/rofi/themes/fancy2.rasi -modi blocks -show blocks -blocks-wrap ~/.config/rofi/history.py") },      /* super c    | 浏览器历史 */ 


    /* alt key : 跳转到对应tag (可附加一条命令 若目标目录无窗口，则执行该命令) */
    /* ctrl key : 将聚焦窗口移动到对应tag */
    /* key tag cmd */
    TAGKEYS(XK_1, 0, 0)
    TAGKEYS(XK_2, 1, 0)
    TAGKEYS(XK_3, 2, 0)
    TAGKEYS(XK_4, 3, 0)
    TAGKEYS(XK_5, 4, 0)
    TAGKEYS(XK_6, 5, 0)
    TAGKEYS(XK_7, 6, 0)
    TAGKEYS(XK_8, 7, 0)
    TAGKEYS(XK_9, 8, 0)
    TAGKEYS(XK_KP_1, 0, 0)
    TAGKEYS(XK_KP_2, 1, 0)
    TAGKEYS(XK_KP_3, 2, 0)
    TAGKEYS(XK_KP_4, 3, 0)
    TAGKEYS(XK_KP_5, 4, 0)
    TAGKEYS(XK_KP_6, 5, 0)
    TAGKEYS(XK_KP_7, 6, 0)
    TAGKEYS(XK_KP_8, 7, 0)
    TAGKEYS(XK_KP_9, 8, 0)
    // TAGKEYS(XK_9, 3, "obs")  ,后面产生表示跳转到tab并打开一个应用
    // TAGKEYS(XK_c, 4, "google-chrome-stable")
    // TAGKEYS(XK_m, 5, "$DWM/scripts/music_player.sh")
    // TAGKEYS(XK_0, 6, "linuxqq")
    // TAGKEYS(XK_w, 7, "/opt/apps/com.qq.weixin.deepin/files/run.sh")
    // TAGKEYS(XK_y, 8, "/opt/apps/com.qq.weixin.work.deepin/files/run.sh")
};

static Button buttons[] = {
    /* click               event mask       button            function       argument  */
    /* 点击窗口标题栏操作 */
    { ClkWinTitle,         0,               Button2,          fullname_taskbar_activeitem, {0} },                     // 中键        |  点击标题     |  切换完整title和部分title
    { ClkWinTitle,         0,               Button1,          togglewin,     {0} },                                   // 左键        |  点击标题     |  切换窗口显示状态
    { ClkWinTitle,         0,               Button3,          killclient,    {0} },                                   // 右键        |  点击标题     |  退出窗口
    /* 点击窗口操作 */
    { ClkClientWin,        SuperMask,       Button1,          movemouse,     {0} },                                   // super+左键  |  拖拽窗口     |  拖拽窗口
    { ClkClientWin,        SuperMask,       Button3,          resizemouse,   {0} },                                   // super+右键  |  拖拽窗口     |  改变窗口大小
    { ClkClientWin,        0,               Button2,          fake_fullscreen,    {0} },                                   // 中键  |  点击窗口     |  窗口假全屏
    { ClkClientWin,        SuperMask,       Button4,          viewtoleft,{0} },                                       // super+鼠标滚轮上  |  鼠标滚轮上   | 切换到上一个tag      
    { ClkClientWin,        SuperMask,       Button5,          viewtoright,{0} },                                      // super+鼠标滚轮下  |  鼠标滚轮 下  | 切换到下一个tag
    { ClkClientWin,        0,               Button1,          inner_overvew_toggleoverview,     {0} },                    //左键  |  overview视图点击窗口 |退出overview跳转到窗口所在tag 
    { ClkClientWin,        0,               Button3,          inner_overvew_killclient,       {0} },                    //右键  |  overview视图点击窗口 |关闭该窗口
    { ClkClientWin,        AltMask,         Button1,          spawn, SHCMD("bash ~/tool/shotTranslate.sh shot") },      //alt + 左键  |  触发截图翻译 


    { ClkRootWin,          SuperMask,       Button4,          viewtoleft,{0} },                                       // super+鼠标滚轮上  |  鼠标滚轮上     | 切换到上一个tag    
    { ClkRootWin,          SuperMask,       Button5,          viewtoright,{0} },                                        // super+鼠标滚轮下  |  鼠标滚轮 下  | 切换到下一个tag
    /* 点击tag操作 */
    { ClkTagBar,           0,               Button1,          view,          {0} },                                   // 左键        |  点击tag      |  切换tag
	{ ClkTagBar,           0,               Button3,          toggleview,    {0} },                                   // 右键        |  点击tag      |  切换是否显示tag
    { ClkTagBar,           SuperMask,       Button1,          tag,           {0} },                                   // super+左键  |  点击tag      |  将窗口移动到对应tag
    { ClkTagBar,           0,               Button4,          viewtoleft,    {0} },                                   // 鼠标放tag图表滚轮上  |  tag          |  向前切换tag
	{ ClkTagBar,           0,               Button5,          viewtoright,   {0} },                                   // 鼠标放tag图表滚轮下  |  tag          |  向后切换tag
    /* 点击状态栏操作 */
    { ClkStatusText,       0,               Button1,          clickstatusbar,{0} },                                   // 左键        |  点击状态栏   |  根据状态栏的信号执行 $DWM/scripts/dwmstatusbar.sh $signal L
    { ClkStatusText,       0,               Button2,          clickstatusbar,{0} },                                   // 中键        |  点击状态栏   |  根据状态栏的信号执行 $DWM/scripts/dwmstatusbar.sh $signal M
    { ClkStatusText,       0,               Button3,          clickstatusbar,{0} },                                   // 右键        |  点击状态栏   |  根据状态栏的信号执行 $DWM/scripts/dwmstatusbar.sh $signal R
    { ClkStatusText,       0,               Button4,          clickstatusbar,{0} },                                   // 鼠标滚轮上  |  状态栏       |  根据状态栏的信号执行 $DWM/scripts/dwmstatusbar.sh $signal U
    { ClkStatusText,       0,               Button5,          clickstatusbar,{0} },                                   // 鼠标滚轮下  |  状态栏       |  根据状态栏的信号执行 $DWM/scripts/dwmstatusbar.sh $signal D
    /*点击布局图标*/
    { ClkLtSymbol,           0,             Button1,          selectlayout,{.v = &layouts[1]} },                                                        // 左键        |  点击布局图表      |  切换网格和栈布局                                                                                                   //
    { ClkLtSymbol,           0,             Button3,          selectlayout,{.v = &layouts[2]} },                                                        // 右键        |  点击布局图表      |  切换从头部入栈还是尾部                                                                                                      //
    /* 点击bar空白处 */
    { ClkBarEmpty,         0,               Button1,          spawn, SHCMD("/usr/bin/rofi -config ~/.config/rofi/dwmwin.rasi -show window") },         // 左键        |  bar空白处    |  rofi 执行 window
    { ClkBarEmpty,         0,               Button3,          spawn, SHCMD("/usr/bin/rofi -theme ~/.config/rofi/dwmwin.rasi -show website") },               // 右键        |  bar空白处    |  rofi 执行 drun
};
