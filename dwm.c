/* See LICENSE file for copyright and license details.
 *
 * dynamic window manager is designed like any other X client as well. It is
 * driven through handling X events. In contrast to other X clients, a window
 * manager selects for SubstructureRedirectMask on the root window, to receive
 * events about window (dis-)appearance. Only one X connection at a time is
 * allowed to select for this event mask.
 *
 * The event handlers of dwm are organized in an array which is accessed
 * whenever a new event has been fetched. This allows event dispatching
 * in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag. Clients are organized in a linked client
 * list on each monitor, the focus history is remembered through a stack list
 * on each monitor. Each client contains a bit array to indicate the tags of a
 * client.
 *
 * Keys and tagging rules are organized as arrays and defined in config.h.
 *
 * To understand everything else, start reading main().
 */
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */
#include <X11/Xft/Xft.h>

#include "drw.h"
#include "util.h"
#include <X11/extensions/XInput2.h>
#include <math.h>
#include <signal.h>


/* macros */
#define BUTTONMASK (ButtonPressMask | ButtonReleaseMask)
#define CLEANMASK(mask)                                                        \
  (mask & ~(numlockmask | LockMask) &                                          \
   (ShiftMask | ControlMask | Mod1Mask | Mod2Mask | Mod3Mask | Mod4Mask |      \
    Mod5Mask))
#define INTERSECT(x, y, w, h, m)                                               \
  (MAX(0, MIN((x) + (w), (m)->wx + (m)->ww) - MAX((x), (m)->wx)) *             \
   MAX(0, MIN((y) + (h), (m)->wy + (m)->wh) - MAX((y), (m)->wy)))
#define ISVISIBLE(C)                                                           \
  ((C->mon->isoverview || C->isglobal ||                                       \
    C->tags & C->mon->tagset[C->mon->seltags]))
#define HIDDEN(C) ((getstate(C->win) == IconicState))
#define LENGTH(X) (sizeof X / sizeof X[0])
#define MOUSEMASK (BUTTONMASK | PointerMotionMask)
#define WIDTH(X) ((X)->w + 2 * (X)->bw)
#define HEIGHT(X) ((X)->h + 2 * (X)->bw)
#define TAGMASK ((1 << LENGTH(tags)) - 1)
#define TEXTW(X) (drw_fontset_getwidth(drw, (X)) + lrpad)
#define SYSTEM_TRAY_REQUEST_DOCK 0

/* XEMBED messages */
#define XEMBED_EMBEDDED_NOTIFY 0
#define XEMBED_WINDOW_ACTIVATE 1
#define XEMBED_FOCUS_IN 4
#define XEMBED_MODALITY_ON 10

#define XEMBED_MAPPED (1 << 0)
#define XEMBED_WINDOW_ACTIVATE 1
#define XEMBED_WINDOW_DEACTIVATE 2

#define VERSION_MAJOR 0
#define VERSION_MINOR 0
#define XEMBED_EMBEDDED_VERSION (VERSION_MAJOR << 16) | VERSION_MINOR

#define OPAQUE 0xffU

/* enums */
enum { CurNormal, CurResize, CurMove, CurLast }; /* cursor */
enum {
  SchemeNorm,              // 普通
  SchemeSel,               // 选中的
  SchemeSelGlobal,         // 全局并选中的
  SchemeSelScratchpad,     // 便签窗口
  SchemeSelFakeFull,       // 伪全屏并选中的
  SchemeSelFakeFullGLObal, // 伪全屏并全局并选中
  SchemeHid,               // 隐藏的
  SchemeSystray,           // 托盘
  SchemeNormTag,           // 普通标签
  SchemeSelTag,            // 选中的标签
  SchemeUnderline,         // 下划线
  SchemeBarEmpty,          // 状态栏空白部分
  SchemeStatusText,         // 状态栏文本
};                         /* color schemes */
enum {
  NetSupported,
  NetWMName,
  NetWMState,
  NetWMCheck,
  NetSystemTray,
  NetSystemTrayOP,
  NetSystemTrayOrientation,
  NetSystemTrayOrientationHorz,
  NetWMFullscreen,
  NetActiveWindow,
  NetWMWindowType,
  NetWMWindowTypeDialog,
  NetClientList,
  NetLast
};                                           /* EWMH atoms */
enum { Manager, Xembed, XembedInfo, XLast }; /* Xembed atoms */
enum {
  WMProtocols,
  WMDelete,
  WMState,
  WMTakeFocus,
  WMLast
}; /* default atoms */
enum {
  ClkTagBar,
  ClkLtSymbol,
  ClkStatusText,
  ClkWinTitle,
  ClkBarEmpty,
  ClkClientWin,
  ClkRootWin,
  ClkLast
};                                               /* clicks */
enum { UP, DOWN, LEFT, RIGHT };                  /* movewin */
enum { V_EXPAND, V_REDUCE, H_EXPAND, H_REDUCE }; /* resizewins */

enum {_NET_WM_STATE_REMOVE,_NET_WM_STATE_ADD,_NET_WM_STATE_TOGGLE};  //区分clientmessage是添加请求还是清除请求,第三个是切换请求

typedef struct {
  int i;
  unsigned int ui;
  float f;
  const void *v;
} Arg;

typedef struct {
  unsigned int click;
  unsigned int mask;
  unsigned int button;
  void (*func)(const Arg *arg);
  const Arg arg;
} Button;

typedef struct Monitor Monitor;
typedef struct Client Client;
struct Client {
  char name[256];
  char icon[10];
  float mina, maxa;
  int x, y, w, h;
  int oldx, oldy, oldw, oldh;
  int bw, oldbw;
  int overview_backup_x, overview_backup_y, overview_backup_w,
      overview_backup_h, overview_backup_bw;
  int fullscreen_backup_x, fullscreen_backup_y, fullscreen_backup_w,
      fullscreen_backup_h;
  int isactive;
  int basew, baseh, incw, inch, maxw, maxh, minw, minh;
  int taskw, no_limit_taskw;
  unsigned int tags;
  int isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen,
      isglobal, isnoborder, isscratchpad, overview_isfullscreenbak,
      overview_isfloatingbak;
  Client *next;
  Client *snext;
  Monitor *mon;
  Window win;
	int is_in_scratchpad;
	int is_scratchpad_show;
	int scratchpad_priority;  
};

typedef struct {
  unsigned int mod;
  KeySym keysym;
  void (*func)(const Arg *);
  const Arg arg;
} Key;

typedef struct {
  const char *symbol;
  void (*arrange)(Monitor *);
} Layout;

typedef struct Pertag Pertag;

struct Monitor {
  char ltsymbol[16];
  float mfact;
  int nmaster;
  int num;
  int by;             /* bar geometry */
  int bt;             /* number of tasks */
  int mx, my, mw, mh; /* screen size */
  int wx, wy, ww, wh; /* window area  */
  unsigned int seltags;
  unsigned int sellt;
  unsigned int tagset[2];
  int showbar;
  int topbar;
  Client *clients;
  Client *sel;
  Client *stack;
  Monitor *next;
  Window barwin;
  const Layout *lt[2];
  Pertag *pertag;
  uint isoverview;
  uint is_in_hotarea;
  int status_w;
};

typedef struct {
  const char *class;
  const char *instance;
  const char *title;
  unsigned int tags;
  int isfloating;
  int isglobal;
  int isnoborder;
  int monitor;
  uint floatposition;
  uint width;
  uint high;
} Rule;

typedef struct {
  const char *class;
  const char *style;
} Icon;

typedef struct Systray Systray;
struct Systray {
  Window win;
  Client *icons;
};

/* function declarations */
static void logtofile(const char *fmt, ...);
static void lognumtofile(unsigned int num);

static void tile(Monitor *m);
static void rtile(Monitor *m);
static void magicgrid(Monitor *m);
static void overview(Monitor *m);
static void grid(Monitor *m, uint gappo, uint uappi);

static void applyrules(Client *c);
static int applysizehints(Client *c, int *x, int *y, int *w, int *h,
                          int interact);
static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void attach(Client *c);
static void attachstack(Client *c);
static void buttonpress(XEvent *e);
static void checkotherwm(void);
static unsigned int judge_win_contain_state(Window w, Atom prop);
static void cleanup(void);
static void cleanupmon(Monitor *mon);
static void clientmessage(XEvent *e);
static void configure(Client *c);
static void configurenotify(XEvent *e);
static unsigned long get_win_pid(Window win);
static Atom getatompropfromwin(Window w, Atom prop);
static void configurerequest(XEvent *e);
static void clickstatusbar(const Arg *arg);
static Monitor *createmon(void);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void detachstack(Client *c);
static Monitor *dirtomon(int dir);

static void drawbar(Monitor *m);
static void drawbars(void);
static int drawstatusbar(Monitor *m, int bh, char *text);

static void enternotify(XEvent *e);
static void expose(XEvent *e);

static void focusin(XEvent *e);
static void focus(Client *c);
static void focusmon(const Arg *arg);
static void focusstack(const Arg *arg);

static void pointerfocuswin(Client *c);

static Atom getatomprop(Client *c, Atom prop);
static int getrootptr(int *x, int *y);
static long getstate(Window w);
static unsigned int getsystraywidth();
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);

static void grabbuttons(Client *c, int focused);
static void grabkeys(void);

static void hide(Client *c);
static void show(Client *c);
static void showtag(Client *c);
static void hidewin(const Arg *arg);
static void hideotherwins(const Arg *arg);
static void showonlyorall(const Arg *arg);
static int issinglewin(const Arg *arg);
static void restorewin(const Arg *arg);
static void toggle_scratchpad(const Arg *arg);
static void show_scratchpad(Client *c);

static void incnmaster(const Arg *arg);
static void keypress(XEvent *e);
static void killclient(const Arg *arg);
static void forcekillclient(const Arg *arg);

static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void motionnotify(XEvent *e);
static void movemouse(const Arg *arg);
static void movewin(const Arg *arg);
static void resizewin(const Arg *arg);
static Client *nexttiled(Client *c);
static void pop(Client *);
static void propertynotify(XEvent *e);
static void quit(const Arg *arg);
static void setup(void);
static void seturgent(Client *c, int urg);
static void sigchld(int unused);
static void spawn(const Arg *arg);
static Monitor *systraytomon(Monitor *m);

static Monitor *recttomon(int x, int y, int w, int h);
static void removesystrayicon(Client *i);
static void resize(Client *c, int x, int y, int w, int h, int interact);
static void resizebarwin(Monitor *m);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void resizemouse(const Arg *arg);
static void resizerequest(XEvent *e);
static void restack(Monitor *m);
static void clear_client(Client *fc);

static void run(void);
static void runAutostart(void);
static void scan(void);
static int sendevent(Window w, Atom proto, int m, long d0, long d1, long d2,
                     long d3, long d4);
static void sendmon(Client *c, Monitor *m);
static void setclientstate(Client *c, long state);
static void setfocus(Client *c);

static void selectlayout(const Arg *arg);
static void setlayout(const Arg *arg);

// static void client_flag_filter(Client *c);
static void fullscreen(const Arg *arg);
static void setfullscreen(Client *c);
static void fake_fullscreen(const Arg *arg);
static void set_fake_fullscreen(Client *c);
static void setmfact(const Arg *arg);

static void tag_client(const Arg *arg,Client *target_client);
static void tag(const Arg *arg);
static void tagmon(const Arg *arg);
static void tagtoleft(const Arg *arg);
static void tagtoright(const Arg *arg);

static void togglebar(const Arg *arg);
static void togglesystray();
static void togglefloating(const Arg *arg);
static void toggleallfloating(const Arg *arg);
static void toggleview(const Arg *arg);
static void toggleoverview(const Arg *arg);
static void togglewin(const Arg *arg);
static void toggleglobal(const Arg *arg);
static void toggleborder(const Arg *arg);

static void unfocus(Client *c, int setfocus);
static void unmanage(Client *c, int destroyed);
static void unmapnotify(XEvent *e);

static void updatebarpos(Monitor *m);
static void updatebars(void);
static void updateclientlist(void);
static int updategeom(void);
static void updatenumlockmask(void);
static void updatesizehints(Client *c);
static void updatestatus(void);
static void updatesystray(void);
static void updatesystrayicongeom(Client *i, int w, int h);
static void updatesystrayiconstate(Client *i, XPropertyEvent *ev);
static void updatetitle(Client *c);
static void updateicon(Client *c);
static void updatewindowtype(Client *c);
static void updatewmhints(Client *c);

static void setgap(const Arg *arg);

static void view(const Arg *arg);
static void viewtoleft(const Arg *arg);
static void viewtoright(const Arg *arg);
static void addtoleft(const Arg *arg);
static void addtoright(const Arg *arg);


static void exchange_client(const Arg *arg);
static void focusdir(const Arg *arg);
static unsigned int get_tags_first_tag(unsigned int tags);


static Client *wintoclient(Window w);
static Monitor *wintomon(Window w);
static Client *wintosystrayicon(Window w);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);
static void xinitvisual();
static void zoom(const Arg *arg);

static void inner_overvew_toggleoverview(const Arg *arg);
static void inner_overvew_killclient(const Arg *arg);
static void clear_fullscreen_flag(Client *c);
static uint get_border_type(Client *c);
static void toggle_hotarea(int x_root, int y_root);
static void xi_handler(XEvent xevent);
static void overview_restore(Client *c, const Arg *arg);
static void overview_backup(Client *c);
static void fullname_taskbar_activeitem(const Arg *arg);
static unsigned int want_restore_fullscreen(Client *target_client);
static unsigned int want_auto_fullscren(Client *c);

/* variables */
static Systray *systray = NULL;
static const char broken[] = "broken";
static char stext[1024];
static int screen;
static int sw, sh;      /* X display screen geometry width, height */
static int bh, blw = 0; /* bar geometry */
static int lrpad;       /* sum of left and right padding for text */
static int vp;          /* vertical padding for bar */
static int sp;          /* side padding for bar */
static int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int numlockmask = 0;
static void (*handler[LASTEvent])(
    XEvent *) = { // 给捕获的事件定义处理函数,左边是类型,右边是自定义函数
    [ButtonPress] = buttonpress, // 按键事件
    [ClientMessage] = clientmessage, //窗口给x11服务发送的请求比如全屏请求等
    [ConfigureRequest] = configurerequest, //客户端窗口请求大小和位置变化和边框改变,窗口请求不是用户触发
    [ConfigureNotify] = configurenotify, //窗口大小和位置变化和边框改变的时候触发
    [DestroyNotify] = destroynotify, //销毁事件
    [EnterNotify] = enternotify, // 鼠标移动进入窗口事件,比如聚焦的处理
    [Expose] = expose,  // 窗口暴露事件,由不可见变成可见的时候触发
    [FocusIn] = focusin,   //聚焦事件
    [KeyPress] = keypress, //键盘事件
    [MappingNotify] = mappingnotify, //键盘映射发生变化时触发
    [MapRequest] = maprequest,    //窗口映射触发,只有设置了映射窗口才能被看到
    [MotionNotify] = motionnotify, // 监视器事件,比如鼠标移动到某个位置的处理
    [PropertyNotify] = propertynotify, //窗口属性改变的时候触发
    [ResizeRequest] = resizerequest,  //客户端请求重置大小,不是用户触发的
    [UnmapNotify] = unmapnotify};   //窗口取消映射的时候触发,比如隐藏和杀死都会触发

static Atom wmatom[WMLast], netatom[NetLast], xatom[XLast];
static int running = 1;
static Cur *cursor[CurLast];
static Clr **scheme;
static Display *dpy;
static int xi_opcode;
static Drw *drw;
static int useargb = 0;
static Visual *visual;
static int depth;
static Colormap cmap;
static Monitor *mons, *selmon;
static Window root, wmcheckwin;
static int hiddenWinStackTop = -1;
static Client *hiddenWinStack[100];

/* configuration, allows nested code to access above variables */
#include "config.h"
#include "icons.h"

struct Pertag {
  unsigned int curtag, prevtag;          /* current and previous tag */
  int nmasters[LENGTH(tags) + 1];        /* number of windows in master area */
  float mfacts[LENGTH(tags) + 1];        /* mfacts per tag */
  unsigned int sellts[LENGTH(tags) + 1]; /* selected layouts */
  const Layout
      *ltidxs[LENGTH(tags) + 1][2]; /* matrix of tags and layouts indexes  */
  int showbars[LENGTH(tags) + 1];   /* display bar for the current tag */
};

/* function implementations */
void logtofile(const char *fmt, ...) {
  char buf[256];
  char cmd[256];
  va_list ap;
  va_start(ap, fmt);
  vsprintf((char *)buf, fmt, ap);
  va_end(ap);
  uint i = strlen((const char *)buf);

  sprintf(cmd, "echo '%.*s' >> ~/log", i, buf);
  system(cmd);
}

/* function implementations */
void lognumtofile(unsigned int num) {
  char cmd[256];
  sprintf(cmd, "echo '%x' >> ~/log", num);
  system(cmd);
}

//获取tags中最坐标的tag的tagmask
unsigned int get_tags_first_tag(unsigned int tags){
	unsigned int i,target,tag;
	tag = 0;
	for(i=0;!(tag & 1);i++){
		tag = tags >> i;
	}
	target = 1 << (i-1);	
	return target;
}


//获取窗口的进程pid
unsigned long get_win_pid(Window win) {
  Atom prop_name = XInternAtom(dpy, "_NET_WM_PID", False);
  Atom type;
  int format;
  unsigned long nitems;
  unsigned long bytes_after;
  unsigned char *prop = NULL;
  unsigned long pid = 0;

  if (XGetWindowProperty(dpy, win, prop_name, 0, 1, False, XA_CARDINAL,
                         &type, &format, &nitems, &bytes_after, &prop) == Success) {
    if (prop) {
      pid = *((unsigned long *) prop);
      XFree(prop);
    }
  }
  return pid;
}


// 扩展输入事件处理函数
static void xi_handler(XEvent xevent) {
  Client *pointer_in_client;           // 鼠标所在的窗口
  XGetEventData(dpy, &xevent.xcookie); // 获取扩展事件cookie对应的事件数据
  if (xevent.xcookie.evtype == XI_RawMotion) { // 鼠标移动事件
    Window root_return, child_return;
    int root_x_return, root_y_return;
    int win_x_return, win_y_return;
    unsigned int mask_return;

    XQueryPointer(dpy, root, &root_return,
                  &child_return, // 获取鼠标位置和鼠标所在的窗口
                  &root_x_return, &root_y_return, &win_x_return, &win_y_return,
                  &mask_return);

    toggle_hotarea(root_x_return,
                   root_y_return); // 判断窗口真全屏状态左下角触发热区

    if (child_return != None && mouse_move_toggle_focus == 1) {
      pointer_in_client =
          wintoclient(child_return); // window对象转换为client对象
      focus(pointer_in_client);      // 聚焦到鼠标所在的窗口
    }
  } else if (xevent.xcookie.evtype == XI_RawButtonPress){ //鼠标按键事件
    Window root_return, child_return;
    int root_x_return, root_y_return;
    int win_x_return, win_y_return;
    unsigned int mask_return;


    XQueryPointer(dpy, root, &root_return,
                  &child_return, // 获取鼠标位置和鼠标所在的窗口
                  &root_x_return, &root_y_return, &win_x_return, &win_y_return,
                  &mask_return);

    Atom net_wm_state_skip_pager =                        //_NET_WM_STATE_SKIP_PAGER一些窗口小部件有这个属性比如eww面板
    XInternAtom(dpy, "_NET_WM_STATE_SKIP_PAGER", False); //最后一个参数表示是否系统本来就存在该原子的时候才返回,这里表示可以自定义不管系统有没有

    Atom normal_win =                        //_NET_WM_STATE_SKIP_PAGER一些窗口小部件有这个属性比如eww面板
    XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_NORMAL", False); //最后一个参数表示是否系统本来就存在该原子的时候才返回,这里表示可以自定义不管系统有没有


    unsigned int isinbacklist = judge_win_contain_state(child_return, net_wm_state_skip_pager);

    Atom wtype = getatompropfromwin(child_return, netatom[NetWMWindowType]);

    if(!child_return || child_return == selmon->barwin || child_return == systray->win || isinbacklist == 1 || wtype != normal_win){
      goto FreeEventData;
    }
    pointer_in_client =
        wintoclient(child_return); // window对象转换为client对象
    focus(pointer_in_client);      // 聚焦到鼠标所在的窗口
    if(pointer_in_client && pointer_in_client->isfloating){
      XRaiseWindow(dpy,pointer_in_client->win);   //提升浮动窗口到顶层
    }
  } 

FreeEventData:
  XFreeEventData(dpy,
                 &xevent.xcookie); // 释放函数开头get到内存的数据防止内存泄露
}

void applyrules(Client *c) { // 读取config.h的窗口配置规则处理
  const char *class, *instance;
  unsigned int i;
  const Rule *r;
  Monitor *m;
  XClassHint ch = {NULL, NULL};

  /* rule matching */
  c->isfloating = 0;
  c->overview_isfloatingbak = 0;
  c->isglobal = 0;
  c->isfullscreen = 0;
  c->overview_isfullscreenbak = 0;
  c->isnoborder = 0;
  c->isscratchpad = 0;
  c->tags = 0;
  XGetClassHint(dpy, c->win, &ch);
  class = ch.res_class ? ch.res_class : broken;
  instance = ch.res_name ? ch.res_name : broken;

  for (i = 0; i < LENGTH(rules); i++) {

    r = &rules[i];

    // 当rule中定义了一个或多个属性时，只要有一个属性匹配，就认为匹配成功
    if ((r->title && !strcmp(c->name, r->title)) ||
        (r->class && !strcmp(class, r->class)) ||
        (r->instance && !strcmp(instance, r->instance))) {

      c->isfloating = r->isfloating;
      c->isglobal = r->isglobal;
      c->isnoborder = r->isnoborder;
      c->tags |= r->tags;
      c->bw = c->isnoborder ? 0 : borderpx;
      for (m = mons; m && m->num != r->monitor; m = m->next)
        ;
      if (m)
        c->mon = m;

      // 如果设定了isfloating 且设置窗口的长和宽
      if (r->isfloating && r->width != 0 && r->high != 0) {
        c->w = r->width;
        c->h = r->high;
      }

      // 如果设定了floatposition 设定窗口位置
      if (r->isfloating && r->floatposition != 0) {
        switch (r->floatposition) {
        case 1:
          c->x = selmon->wx + gappo;
          c->y = selmon->wy + gappo;
          break; // 左上
        case 2:
          c->x = selmon->wx + (selmon->ww - WIDTH(c)) / 2 - gappo;
          c->y = selmon->wy + gappo;
          break; // 中上
        case 3:
          c->x = selmon->wx + selmon->ww - WIDTH(c) - gappo;
          c->y = selmon->wy + gappo;
          break; // 右上
        case 4:
          c->x = selmon->wx + gappo;
          c->y = selmon->wy + (selmon->wh - HEIGHT(c)) / 2;
          break; // 左中
        case 0:  // 默认0，居中
        case 5:
          c->x = selmon->wx + (selmon->ww - WIDTH(c)) / 2;
          c->y = selmon->wy + (selmon->wh - HEIGHT(c)) / 2;
          break; // 中中
        case 6:
          c->x = selmon->wx + selmon->ww - WIDTH(c) - gappo;
          c->y = selmon->wy + (selmon->wh - HEIGHT(c)) / 2;
          break; // 右中
        case 7:
          c->x = selmon->wx + gappo;
          c->y = selmon->wy + selmon->wh - HEIGHT(c) - gappo;
          break; // 左下
        case 8:
          c->x = selmon->wx + (selmon->ww - WIDTH(c)) / 2;
          c->y = selmon->wy + selmon->wh - HEIGHT(c) - gappo;
          break; // 中下
        case 9:
          c->x = selmon->wx + selmon->ww - WIDTH(c) - gappo;
          c->y = selmon->wy + selmon->wh - HEIGHT(c) - gappo;
          break; // 右下
        }
      }

      break; // 有且只会匹配一个第一个符合的rule
    }
  }
  if (!strcmp(c->name, scratchpadname) || !strcmp(class, scratchpadname) ||
      !strcmp(instance, scratchpadname)) {
    c->isscratchpad = 1;
    c->isfloating = 1;
    c->isglobal = 1; // scratchpad is default global
  }
  if (ch.res_class)
    XFree(ch.res_class);
  if (ch.res_name)
    XFree(ch.res_name);
  c->tags =
      c->tags & TAGMASK ? c->tags & TAGMASK : c->mon->tagset[c->mon->seltags];
}

int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact) {
  int baseismin;
  Monitor *m = c->mon;

  /* set minimum possible */
  *w = MAX(1, *w);
  *h = MAX(1, *h);
  if (interact) {
    if (*x > sw)
      *x = sw - WIDTH(c);
    if (*y > sh)
      *y = sh - HEIGHT(c);
    if (*x + *w + 2 * c->bw < 0)
      *x = 0;
    if (*y + *h + 2 * c->bw < 0)
      *y = 0;
  } else {
    if (*x >= m->wx + m->ww)
      *x = m->wx + m->ww - WIDTH(c);
    if (*y >= m->wy + m->wh)
      *y = m->wy + m->wh - HEIGHT(c);
    if (*x + *w + 2 * c->bw <= m->wx)
      *x = m->wx;
    if (*y + *h + 2 * c->bw <= m->wy)
      *y = m->wy;
  }
  if (*h < bh)
    *h = bh;
  if (*w < bh)
    *w = bh;
  if (c->isfloating) {
    /* see last two sentences in ICCCM 4.1.2.3 */
    baseismin = c->basew == c->minw && c->baseh == c->minh;
    if (!baseismin) { /* temporarily remove base dimensions */
      *w -= c->basew;
      *h -= c->baseh;
    }
    /* adjust for aspect limits */
    if (c->mina > 0 && c->maxa > 0) {
      if (c->maxa < (float)*w / *h)
        *w = *h * c->maxa + 0.5;
      else if (c->mina < (float)*h / *w)
        *h = *w * c->mina + 0.5;
    }
    if (baseismin) { /* increment calculation requires this */
      *w -= c->basew;
      *h -= c->baseh;
    }
    /* adjust for increment value */
    if (c->incw)
      *w -= *w % c->incw;
    if (c->inch)
      *h -= *h % c->inch;
    /* restore base dimensions */
    *w = MAX(*w + c->basew, c->minw);
    *h = MAX(*h + c->baseh, c->minh);
    if (c->maxw)
      *w = MIN(*w, c->maxw);
    if (c->maxh)
      *h = MIN(*h, c->maxh);
  }
  return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

void arrange(Monitor *m) { // 布局管理
  if (m)
    showtag(m->stack);
  else
    for (m = mons; m; m = m->next)
      showtag(m->stack);
  if (m) {
    arrangemon(m);
    restack(m);
  } else
    for (m = mons; m; m = m->next)
      arrangemon(m);
}

void arrangemon(Monitor *m) { // 确认选用的布局
  if (m->isoverview) {
    strncpy(m->ltsymbol, overviewlayout.symbol, sizeof overviewlayout.symbol);
    overviewlayout.arrange(m);
  } else {
    strncpy(m->ltsymbol, m->lt[m->sellt]->symbol,
            sizeof m->lt[m->sellt]->symbol);
    m->lt[m->sellt]->arrange(m);
  }
}


void attach(Client *c) { // 新打开的窗口放入窗口链表中

  Client **tc;
  for (tc = &c->mon->clients; *tc; tc = &(*tc)->next){
    // 如果当前的tag中有新创建的窗口,就让当前tag中的全屏窗口退出全屏参与平铺
    if((*tc) && !c->isfloating && (*tc)->tags & c->tags){
      clear_fullscreen_flag(*tc);
    }
  }

  if (!newclientathead) {
    *tc = c;
    c->next = NULL;
  } else {
    c->next = c->mon->clients;
    c->mon->clients = c;
  }
}

void attachstack(Client *c) { // 放入栈中
  c->snext = c->mon->stack;
  c->mon->stack = c;
}

void buttonpress(XEvent *e) { // 鼠标按键事件处理函数
  unsigned int i, x, click, occ = 0;
  Arg arg = {0};
  Monitor *m;
  XButtonPressedEvent *ev = &e->xbutton;
  Client *c = wintoclient(ev->window);

  // 滚轮加按键事件直接操作,不进行下面鼠标位置判断
  for (i = 0; i < LENGTH(buttons); i++) {
    if (buttons[i].func && buttons[i].button == ev->button &&
        (ev->button == Button4 || ev->button == Button5) &&
        CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state) &&
        CLEANMASK(buttons[i].mask) != 0) {

      buttons[i].func(&buttons[i].arg);
      return;
    }
  }

  // 判断鼠标点击的位置
  click = ClkRootWin;
  /* focus monitor if necessary */
  if ((m = wintomon(ev->window)) && m != selmon) {
    unfocus(selmon->sel, 1);
    selmon = m;
    focus(NULL);
  }
  int status_w = selmon->status_w;
  int system_w = getsystraywidth();
  if (ev->window == selmon->barwin ||
      (!c && selmon->showbar &&
       (topbar ? ev->y <= selmon->wy
               : ev->y >= selmon->wy + selmon->wh))) { // 点击在bar上
    i = x = 0;
    blw = TEXTW(selmon->ltsymbol);

    if (selmon->isoverview) {
      x += TEXTW(overviewtag);
      i = ~0;
      if (ev->x > x)
        i = LENGTH(tags);
    } else {
      for (c = m->clients; c; c = c->next)
        occ |= c->tags == TAGMASK ? 0 : c->tags;
      do {
        /* do not reserve space for vacant tags */
        if (!(occ & 1 << i || m->tagset[m->seltags] & 1 << i))
          continue;
        x += TEXTW(tags[i]);
      } while (ev->x >= x && ++i < LENGTH(tags));
    }
    if (i < LENGTH(tags)) {
      click = ClkTagBar;
      arg.ui = 1 << i;
    } else if (ev->x < x + blw)
      click = ClkLtSymbol;
    else if (ev->x > selmon->ww - status_w - 2 * sp -
                         (selmon == systraytomon(selmon)
                              ? (system_w ? system_w + systraypinning + 2 : 0)
                              : 0)) {
      click = ClkStatusText;
      arg.i = ev->x - (selmon->ww - status_w - 2 * sp -
                       (selmon == systraytomon(selmon)
                            ? (system_w ? system_w + systraypinning + 2 : 0)
                            : 0));
      arg.ui = ev->button; // 1 => L，2 => M，3 => R, 5 => U, 6 => D
    } else {
      click = ClkBarEmpty;

      x += blw;
      c = m->clients;

      if (m->bt != 0)
        do {
          if (!ISVISIBLE(c))
            continue;
          else
            x += c->taskw;
        } while (ev->x > x && (c = c->next));

      if (c) {
        click = ClkWinTitle;
        arg.v = c;
      }
    }
  } else if ((c = wintoclient(ev->window))) {
    focus(c);
    restack(selmon);
    // 这句代码好像是多余的,因为不停止捕获,重新发送的事件还是会被再次拦截捕获,不会传到原本的客户端
    XAllowEvents(dpy, ReplayPointer, CurrentTime);
    click = ClkClientWin;
  }
  // 增加ClkRootWin也可以触发鼠标按键事件,这样tab无窗口才能使用鼠标中键加滚轮切换tab
  for (i = 0; i < LENGTH(buttons); i++)
    if (click == buttons[i].click && buttons[i].func &&
        buttons[i].button == ev->button &&
        CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state)) {

      buttons[i].func((click == ClkTagBar || click == ClkRootWin ||
                       click == ClkWinTitle || click == ClkStatusText) &&
                              buttons[i].arg.i == 0
                          ? &arg
                          : &buttons[i].arg);
    }
}

void checkotherwm(void) {
  xerrorxlib = XSetErrorHandler(xerrorstart);
  /* this causes an error if some other window manager is running */
  XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
  XSync(dpy, False);
  XSetErrorHandler(xerror);
  XSync(dpy, False);
}

void cleanup(void) {
  Arg a = {.ui = ~0};
  Layout foo = {"", NULL};
  Monitor *m;
  size_t i;
  view(&a);
  selmon->lt[selmon->sellt] = &foo;
  for (m = mons; m; m = m->next){
    while (m->stack){
      unmanage(m->stack, 0);
    }
  }
      
  XUngrabKey(dpy, AnyKey, AnyModifier, root);
  while (mons)
    cleanupmon(mons);
  if (showsystray) {
    XUnmapWindow(dpy, systray->win);
    XDestroyWindow(dpy, systray->win);
    free(systray);
  }
  for (i = 0; i < CurLast; i++)
    drw_cur_free(drw, cursor[i]);
  for (i = 0; i < LENGTH(colors) + 1; i++)
    free(scheme[i]);
  XDestroyWindow(dpy, wmcheckwin);
  drw_free(drw);
  XSync(dpy, False);
  XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
  XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
}

void cleanupmon(Monitor *mon) {
  Monitor *m;

  if (mon == mons)
    mons = mons->next;
  else {
    for (m = mons; m && m->next != mon; m = m->next)
      ;
    m->next = mon->next;
  }
  XUnmapWindow(dpy, mon->barwin);
  XDestroyWindow(dpy, mon->barwin);
  free(mon);
}

void clientmessage(XEvent *e) {
  XWindowAttributes wa;
  XSetWindowAttributes swa;
  XClientMessageEvent *cme = &e->xclient;
  Client *c = wintoclient(cme->window);
  Atom atom_max_horz = XInternAtom(dpy, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
  Atom atom_max_vert = XInternAtom(dpy, "_NET_WM_STATE_MAXIMIZED_VERT", False);
  Atom atom_hidden = XInternAtom(dpy, "_NET_WM_STATE_HIDDEN", False);
  Atom atom_change_state = XInternAtom(dpy, "WM_CHANGE_STATE", False);


  if (showsystray && cme->window == systray->win &&
      cme->message_type == netatom[NetSystemTrayOP]) {
    /* add systray icons */
    if (cme->data.l[1] == SYSTEM_TRAY_REQUEST_DOCK) {
      if (!(c = (Client *)calloc(1, sizeof(Client))))
        die("fatal: could not malloc() %u bytes\n", sizeof(Client));
      if (!(c->win = cme->data.l[2])) {
        free(c);
        return;
      }
      c->mon = selmon;
      c->next = systray->icons;
      systray->icons = c;
      XGetWindowAttributes(dpy, c->win, &wa);
      c->x = c->oldx = c->y = c->oldy = 0;
      c->w = c->oldw = wa.width;
      c->h = c->oldh = wa.height;
      c->oldbw = wa.border_width;
      c->bw = 0;
      c->isfloating = True;
      /* reuse tags field as mapped status */
      c->tags = 1;
      updatesizehints(c);
      updatesystrayicongeom(c, wa.width, wa.height);
      XAddToSaveSet(dpy, c->win);
      XSelectInput(dpy, c->win,
                   StructureNotifyMask | PropertyChangeMask |
                       ResizeRedirectMask);
      XReparentWindow(dpy, c->win, systray->win, 0, 0);
      XClassHint ch = {"dwmsystray", "dwmsystray"};
      XSetClassHint(dpy, c->win, &ch);
      /* use parents background color */
      swa.background_pixel = scheme[SchemeNorm][ColBg].pixel;
      XChangeWindowAttributes(dpy, c->win, CWBackPixel, &swa);
      sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime,
                XEMBED_EMBEDDED_NOTIFY, 0, systray->win,
                XEMBED_EMBEDDED_VERSION);
      /* FIXME not sure if I have to send these events, too */
      sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime,
                XEMBED_FOCUS_IN, 0, systray->win, XEMBED_EMBEDDED_VERSION);
      sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime,
                XEMBED_WINDOW_ACTIVATE, 0, systray->win,
                XEMBED_EMBEDDED_VERSION);
      sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime,
                XEMBED_MODALITY_ON, 0, systray->win, XEMBED_EMBEDDED_VERSION);
      XSync(dpy, False);
      resizebarwin(selmon);
      updatesystray();
      setclientstate(c, NormalState);
    }
    return;
  }
  if (!c)
    return;
  if (cme->message_type == netatom[NetWMState]) {
    if (cme->data.l[1] == netatom[NetWMFullscreen] ||
        cme->data.l[2] == netatom[NetWMFullscreen]){
          if(cme->data.l[0] == _NET_WM_STATE_ADD){
            c->isfullscreen = 0;
            setfullscreen(c);
          } else if(cme->data.l[0] == _NET_WM_STATE_REMOVE){
            c->isfullscreen = 1;
            setfullscreen(c);
          } else if(cme->data.l[0] == _NET_WM_STATE_TOGGLE){
            setfullscreen(c);
          } 
    } else if(cme->data.l[1] == atom_max_horz ||cme->data.l[1] == atom_max_vert || cme->data.l[2] == atom_max_horz ||cme->data.l[2] == atom_max_vert) {
      set_fake_fullscreen(c); //MAXIMIZED button toggle 
    } 
  } else if (cme->message_type == netatom[NetActiveWindow]) {
    if (c != selmon->sel && !c->isurgent)
      seturgent(c, 1);
    if (c && HIDDEN(c))
      show(c);
    if (c == selmon->sel)
      return;
    // 若不是当前显示器 则跳转到对应显示器
    if (c->mon != selmon) {
      focusmon(&(Arg){.i = +1});
    }
    // 若不是当前tag 则跳转到对应tag
    if (!ISVISIBLE(c)) {
      view(&(Arg){.ui = c->tags});
    }
  } else if(cme->message_type == atom_change_state) {
    hide(c);
  } 
  // else{
  //     logtofile(XGetAtomName(dpy,cme->message_type));
  //   }
}

void configure(Client *c) {
  XConfigureEvent ce;

  ce.type = ConfigureNotify;
  ce.display = dpy;
  ce.event = c->win;
  ce.window = c->win;
  ce.x = c->x;
  ce.y = c->y;
  ce.width = c->w;
  ce.height = c->h;
  ce.border_width = c->bw;
  ce.above = None;
  ce.override_redirect = False;
  XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void configurenotify(XEvent *e) {
  Monitor *m;
  Client *c;
  XConfigureEvent *ev = &e->xconfigure;
  int dirty;

  /* TODO: updategeom handling sucks, needs to be simplified */
  if (ev->window == root) {
    dirty = (sw != ev->width || sh != ev->height);
    sw = ev->width;
    sh = ev->height;
    if (updategeom() || dirty) {
      drw_resize(drw, sw, bh);
      updatebars();
      for (m = mons; m; m = m->next) {
        for (c = m->clients; c; c = c->next)
          if (c->isfullscreen)
            resizeclient(c, m->mx, m->my, m->mw, m->mh);
        resizebarwin(m);
      }
      focus(NULL);
      arrange(NULL);
    }
  }
}

void configurerequest(XEvent *e) {
  Client *c;
  Monitor *m;
  XConfigureRequestEvent *ev = &e->xconfigurerequest;
  XWindowChanges wc;

  if ((c = wintoclient(ev->window))) {
    if (ev->value_mask & CWBorderWidth)
      c->bw = ev->border_width;
    else if (c->isfloating) {
      m = c->mon;
      if (ev->value_mask & CWX) {
        c->oldx = c->x;
        c->x = m->mx + ev->x;
      }
      if (ev->value_mask & CWY) {
        c->oldy = c->y;
        c->y = m->my + ev->y;
      }
      if (ev->value_mask & CWWidth) {
        c->oldw = c->w;
        c->w = ev->width;
      }
      if (ev->value_mask & CWHeight) {
        c->oldh = c->h;
        c->h = ev->height;
      }
      if ((c->x + c->w) > m->mx + m->mw && c->isfloating)
        c->x = m->mx + (m->mw / 2 - WIDTH(c) / 2); /* center in x direction */
      if ((c->y + c->h) > m->my + m->mh && c->isfloating)
        c->y = m->my + (m->mh / 2 - HEIGHT(c) / 2); /* center in y direction */
      if ((ev->value_mask & (CWX | CWY)) &&
          !(ev->value_mask & (CWWidth | CWHeight)))
        configure(c);
      if (ISVISIBLE(c))
        XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
    } else
      configure(c);
  } else {
    wc.x = ev->x;
    wc.y = ev->y;
    wc.width = ev->width;
    wc.height = ev->height;
    wc.border_width = ev->border_width;
    wc.sibling = ev->above;
    wc.stack_mode = ev->detail;
    XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
  }
  XSync(dpy, False);
}

Monitor *createmon(void) { // 初始化新的显示器数据结构
  Monitor *m;
  unsigned int i;

  m = ecalloc(1, sizeof(Monitor));
  m->tagset[0] = m->tagset[1] = 1;
  m->mfact = mfact;
  m->nmaster = nmaster;
  m->showbar = showbar;
  m->topbar = topbar;
  m->lt[0] = &layouts[0];
  m->lt[1] = &layouts[1 % LENGTH(layouts)];
  strncpy(m->ltsymbol, layouts[0].symbol, sizeof m->ltsymbol);
  m->pertag = ecalloc(1, sizeof(Pertag));
  m->pertag->curtag = m->pertag->prevtag = 1;
  m->isoverview = 0;
  m->is_in_hotarea = 0;
  m->status_w = 0;

  for (i = 0; i <= LENGTH(tags); i++) {
    m->pertag->nmasters[i] = m->nmaster;
    m->pertag->mfacts[i] = m->mfact;

    m->pertag->ltidxs[i][0] = m->lt[0];
    m->pertag->ltidxs[i][1] = m->lt[1];
    m->pertag->sellts[i] = m->sellt;

    m->pertag->showbars[i] = m->showbar;
  }

  return m;
}

void destroynotify(XEvent *e) {
  Client *c;
  XDestroyWindowEvent *ev = &e->xdestroywindow;

  if ((c = wintoclient(ev->window)))
    unmanage(c, 1);
  else if ((c = wintosystrayicon(ev->window))) {
    removesystrayicon(c);
    resizebarwin(selmon);
    updatesystray();
  }
}

void detach(Client *c) {
  Client **tc;

  for (tc = &c->mon->clients; *tc && *tc != c; tc = &(*tc)->next)
    ;
  *tc = c->next;
}

void detachstack(Client *c) {
  Client **tc, *t;

  for (tc = &c->mon->stack; *tc && *tc != c; tc = &(*tc)->snext)
    ;
  *tc = c->snext;

  if (c == c->mon->sel) {
    for (t = c->mon->stack; t && !ISVISIBLE(t); t = t->snext)
      ;
    c->mon->sel = t;
  }
}

Monitor *dirtomon(int dir) {
  Monitor *m = NULL;

  if (dir > 0) {
    if (!(m = selmon->next))
      m = mons;
  } else if (selmon == mons)
    for (m = mons; m->next; m = m->next)
      ;
  else
    for (m = mons; m->next != selmon; m = m->next)
      ;
  return m;
}

void drawbar(Monitor *m) { // 绘制bar

  int x, empty_w;
  int w = 0;
  int system_w = 0, tasks_w = 0, status_w;
  unsigned int i, occ = 0, n = 0, urg = 0, scm;
  Client *c;
  int boxw = 2;
  char *task_content;

  if (!m->showbar)
    return;

  XLowerWindow(dpy,m->barwin);
  XLowerWindow(dpy,systray->win);


  // 获取系统托盘的宽度
  if (showsystray && m == systraytomon(m))
    system_w = getsystraywidth();

  // 绘制STATUSBAR
  status_w = drawstatusbar(m, bh, stext);

  m->status_w = status_w;

  // 判断tag显示数量
  for (c = m->clients; c; c = c->next) {
    if (ISVISIBLE(c))
      n++;
    occ |= c->tags == TAGMASK ? 0 : c->tags;
    if (c->isurgent)
      urg |= c->tags;
  }

  // 绘制TAGS
  x = 0;

  // 代表为overview tag状态
  if (m->isoverview) {
    w = TEXTW(overviewtag);
    drw_setscheme(drw, scheme[SchemeSelTag]);
    drw_text(drw, x, 0, w, bh, lrpad / 2, overviewtag, 0);
    drw_setscheme(drw, scheme[SchemeUnderline]);
    drw_rect(drw, x, bh - boxw, w + lrpad, boxw, 1, 0);
    x += w;
  } else {
    for (i = 0; i < LENGTH(tags); i++) {
      /* do not draw vacant tags */
      if (!(occ & 1 << i || m->tagset[m->seltags] & 1 << i))
        continue;

      w = TEXTW(tags[i]);
      drw_setscheme(drw,
                    scheme[m->tagset[m->seltags] & 1 << i ? SchemeSelTag
                                                          : SchemeNormTag]);
      drw_text(drw, x, 0, w, bh, lrpad / 2, tags[i], urg & 1 << i);
      if (m->tagset[m->seltags] & 1 << i) {
        drw_setscheme(drw, scheme[SchemeUnderline]);
        drw_rect(drw, x + 2, bh - boxw, w + lrpad - 4, boxw, 1, 0);
      }
      x += w;
    }
  }

  // 绘制模式图标
  w = TEXTW(overviewlayout.symbol);
  drw_setscheme(drw, scheme[SchemeNorm]);
  x = drw_text(drw, x, 0, w, bh, lrpad / 2, m->ltsymbol, 0);

  // 绘制TASKS
  for (c = m->clients; c; c = c->next) {
    // 判断是否需要绘制 && 判断颜色设置
    if (!ISVISIBLE(c))
      continue;
    if (m->sel == c)
      scm = SchemeSel;
    else if (HIDDEN(c))
      scm = SchemeHid;
    else
      scm = SchemeNorm;
    drw_setscheme(drw, scheme[scm]);

    // task是否使用图标表示
    if (taskbar_icon == 1 && c->no_limit_taskw == 0 && !(m->sel == c && taskbar_icon_exclude_active == 1)) {
      task_content = c->icon;
    } else {
      task_content = c->name;
    }

    // 绘制TASK
    if (c->no_limit_taskw == 1) {
      w = TEXTW(task_content);
    } else {
      w = MIN(TEXTW(task_content), TEXTW("                                          "));
    }

    // 剩余的空白区域宽度
    empty_w = m->ww - x - status_w - system_w;

    if (w > empty_w) { // 如果当前TASK绘制后长度超过最大宽度
      w = empty_w;
      x = drw_text(drw, x, 0, w, bh, lrpad / 2, "...", 0);
      c->taskw = w;
      tasks_w += w;
      break;
    } else {
      x = drw_text(drw, x, 0, w, bh, lrpad / 2, task_content, 0);
      c->taskw = w;
      tasks_w += w;
    }
  }
  /** 空白部分的宽度 = 总宽度 - 状态栏的宽度 - 托盘的宽度 - sp (托盘存在时
   * 额外多-一个 systrayspadding) */
  empty_w = m->ww - x - status_w - system_w - 2 * sp -
            (system_w ? systrayspadding : 0);
  if (empty_w > 0) {
    drw_setscheme(drw, scheme[SchemeBarEmpty]);
    drw_rect(drw, x, 0, empty_w, bh, 1, 1);
  }

  m->bt = n;
  drw_map(drw, m->barwin, 0, 0, m->ww - system_w, bh);

  resizebarwin(m);
}

// 绘制全部显示器的bar
void drawbars(void) {
  Monitor *m;

  for (m = mons; m; m = m->next)
    drawbar(m);
}

int drawstatusbar(Monitor *m, int bh, char *stext) {
  int status_w = 0, i, w, x, len, system_w = 0;
  short isCode = 0;
  char *text;
  char *p;
  char buf8[8], buf5[5];
  uint textsalpha;

  if (showsystray && m == systraytomon(m))
    system_w = getsystraywidth();

  len = strlen(stext) + 1;
  if (!(text = (char *)malloc(sizeof(char) * len)))
    die("malloc");
  p = text;
  memcpy(text, stext, len);

  /* compute width of the status text */
  w = 0;
  i = -1;
  while (text[++i]) {
    if (text[i] == '^') {
      if (!isCode) {
        isCode = 1;
        text[i] = '\0';
        w += TEXTW(text) - lrpad;
        text[i] = '^';
      } else {
        isCode = 0;
        text = text + i + 1;
        i = -1;
      }
    }
  }
  if (!isCode)
    w += TEXTW(text) - lrpad;
  else
    isCode = 0;
  text = p;

  // w += 2; /* 1px padding on both sides */
  x = m->ww - w - system_w - 2 * sp -
      (system_w ? systrayspadding : 0); // 托盘存在时 额外多-一个systrayspadding

  drw_setscheme(drw, scheme[LENGTH(colors)]);
  drw->scheme[ColFg] = scheme[SchemeNorm][ColFg];
  drw->scheme[ColBg] = scheme[SchemeNorm][ColBg];
  drw_rect(drw, x, 0, w, bh, 1, 1);
  x++;

  /* process status text */
  i = -1;
  while (text[++i]) {
    if (text[i] == '^' && !isCode) {
      isCode = 1;

      text[i] = '\0';
      w = TEXTW(text) - lrpad;
      drw_text(drw, x, 0, w, bh, 0, text, 0);
      status_w += w;

      x += w;

      /* process code */
      while (text[++i] != '^') {
        if (text[i] == 'c') {
          memcpy(buf8, (char *)text + i + 1, 7);
          buf8[7] = '\0';
          i += 7;

          textsalpha = alphas[SchemeStatusText][ColFg];
          if (text[i + 1] != '^') {
            memcpy(buf5, (char *)text + i + 1, 4);
            buf5[4] = '\0';
            i += 4;
            sscanf(buf5, "%x", &textsalpha);
          }

          drw_clr_create(drw, &drw->scheme[ColFg], buf8, textsalpha);
        } else if (text[i] == 'b') {
          memcpy(buf8, (char *)text + i + 1, 7);
          buf8[7] = '\0';
          i += 7;

          textsalpha = alphas[SchemeStatusText][ColBg];
          if (text[i + 1] != '^') {
            memcpy(buf5, (char *)text + i + 1, 4);
            buf5[4] = '\0';
            i += 4;
            sscanf(buf5, "%x", &textsalpha);
          }
          drw_clr_create(drw, &drw->scheme[ColBg], buf8, textsalpha);
        } else if (text[i] == 's') {
          while (text[i + 1] != '^')
            i++;
        } else if (text[i] == 'd') {
          drw->scheme[ColFg] = scheme[SchemeNorm][ColFg];
          drw->scheme[ColBg] = scheme[SchemeNorm][ColBg];
        }
      }

      text = text + i + 1;
      i = -1;
      isCode = 0;
    }
  }

  if (!isCode) {
    w = TEXTW(text) - lrpad;
    drw_text(drw, x, 0, w, bh, 0, text, 0);
    status_w += w;
  }

  drw_setscheme(drw, scheme[SchemeNorm]);
  free(p);

  return status_w - 2;
}

// 点击状态栏时执行的func
// 传入参数为 i  => 鼠标点击的位置相对于左边界的距离
// 传入参数为 ui => 鼠标按键, 1 => 左键, 2 => 中键, 3 => 右键, 4 => 滚轮向上, 5
// => 滚轮向下
long lastclickstatusbartime = 0; // 用于避免过于频繁的点击和滚轮行为
void clickstatusbar(const Arg *arg) {
  if (!arg->i && arg->i < 0)
    return;

  // 用于避免过于频繁的点击和滚轮行为
  struct timeval tv;
  gettimeofday(&tv, NULL);
  long now = tv.tv_sec * 1000 + tv.tv_usec / 1000;
  if (now - lastclickstatusbartime < 100)
    return;
  lastclickstatusbartime = now;

  int offset = -1;
  int status_w = 0;
  int iscode = 0, issignal = 0, signalindex = 0;
  char signal[20];
  char text[100];
  char *button = "L";
  int limit, max = sizeof(stext);

  while (stext[++offset] != '\0') {
    // 左侧^
    if (stext[offset] == '^' && !iscode) {
      iscode = 1;
      offset++;
      if (stext[offset] == 's') { // 查询到s->signal
        issignal = 1;
        signalindex = 0;
        memset(signal, '\0', sizeof(signal));
      } else {
        issignal = 0;
      }
      continue;
    }

    // 右侧^
    if (stext[offset] == '^' && iscode) {
      iscode = 0;
      issignal = 0;
      continue;
    }

    if (issignal) { // 逐位读取signal
      signal[signalindex++] = stext[offset];
    }

    // 是普通文本
    if (!iscode) {
      // 查找到下一个^ 或 游标到达最后
      limit = 0;
      while (stext[offset + ++limit] != '^' && offset + limit < max)
        ;
      if (offset + limit == max)
        break;

      memset(text, '\0', sizeof(text));
      strncpy(text, stext + offset, limit);
      offset += --limit;
      status_w += TEXTW(text) - lrpad;
      if (status_w > arg->i)
        break;
    }
  }

  switch (arg->ui) {
  case Button1:
    button = "L";
    break;
  case Button2:
    button = "M";
    break;
  case Button3:
    button = "R";
    break;
  case Button4:
    button = "U";
    break;
  case Button5:
    button = "D";
    break;
  }

  memset(text, '\0', sizeof(text));
  sprintf(text, "%s %s %s &", statusbarscript, signal, button);
  system(text);
}

void enternotify(XEvent *e) {
  Client *c;
  Monitor *m;
  XCrossingEvent *ev = &e->xcrossing;

  if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) &&
      ev->window != root)
    return;
  c = wintoclient(ev->window);
  m = c ? c->mon : wintomon(ev->window);
  if (m != selmon) {
    unfocus(selmon->sel, 1);
    selmon = m;
  } else if (!c || c == selmon->sel)
    return;
  focus(c);
}

void expose(XEvent *e) { // 窗口暴露事件,由不可见变成可见的时候触发
  Monitor *m;
  XExposeEvent *ev = &e->xexpose;

  if (ev->count == 0 && (m = wintomon(ev->window))) {
    drawbar(m);
    if (m == selmon)
      updatesystray();
  }
}

uint get_border_type(Client *c) {  //判断border颜色
  if (c->isglobal && !c->isfullscreen) {
    return SchemeSelGlobal;
  } else if (c->is_in_scratchpad) {
    return SchemeSelScratchpad;
  } else if (c->isfullscreen && !c->isglobal) {
    return SchemeSelFakeFull;
  } else if (c->isglobal && c->isfullscreen) {
    return SchemeSelFakeFullGLObal;
  } else {
    return SchemeSel;
  }
}

// 聚焦函数
void focus(Client *c) {
  uint border_type = SchemeSel;

  if (c &&
      c->isactive) { // 已经设置过了什么也不做,针对设置了全局鼠标聚焦监测mouse_move_toggle_focus
    return;          // 避免反复设置损耗性能
  }

  if (!c || !ISVISIBLE(c) || HIDDEN(c))
    for (c = selmon->stack; c && (!ISVISIBLE(c) || HIDDEN(c)); c = c->snext)
      ;
  if (selmon->sel && selmon->sel != c)
    unfocus(selmon->sel, 0);

  if (c) {
    if (c->mon != selmon)
      selmon = c->mon;
    if (c->isurgent)
      seturgent(c, 0);
    detachstack(c);
    attachstack(c);
    grabbuttons(c, 1);
    c->isactive = 1; // 标记已经设置过了
    // 设置窗口的border
    border_type = get_border_type(c);
    XSetWindowBorder(dpy, c->win, scheme[border_type][ColBorder].pixel);
    setfocus(c);
    // if (c->isfloating) {
    //   XRaiseWindow(dpy,
    //                c->win); // 浮动窗口聚焦后把视图提到最高层,不被其他窗口覆盖
    // }
  } else {
    XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
    XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
  }
  selmon->sel = c;
  drawbars();
}

/* there are some broken focus acquiring clients needing extra handling */
void focusin(XEvent *e) {
  XFocusChangeEvent *ev = &e->xfocus;

  if (selmon->sel && ev->window != selmon->sel->win)
    setfocus(selmon->sel);
}

void focusmon(const Arg *arg) {
  Monitor *m;

  if (!mons->next)
    return;
  if ((m = dirtomon(arg->i)) == selmon)
    return;
  unfocus(selmon->sel, 0);
  selmon = m;
  focus(NULL);
  pointerfocuswin(NULL);
}

void focusstack(const Arg *arg) {
  Client *tempClients[100];
  Client *c = NULL, *tc = selmon->sel;
  int last = -1, cur = 0, issingle = issinglewin(NULL);

  if (!tc)
    tc = selmon->clients;
  if (!tc)
    return;

  for (c = selmon->clients; c; c = c->next) {
    if (ISVISIBLE(c) && (issingle || !HIDDEN(c))) {
      last++;
      tempClients[last] = c;
      if (c == tc)
        cur = last;
    }
  }
  if (last < 0)
    return;

  if (arg && arg->i == -1) {
    if (cur - 1 >= 0)
      c = tempClients[cur - 1];
    else
      c = tempClients[last];
  } else {
    if (cur + 1 <= last)
      c = tempClients[cur + 1];
    else
      c = tempClients[0];
  }

  if (issingle && single_auto_exit_hide) {
    if (c)
      hideotherwins(&(Arg){.v = c});
  } else {
    if (c) {
      pointerfocuswin(c);
      restack(selmon);
    }
  }
}

void pointerfocuswin(Client *c) {
  if (c) {
    XWarpPointer(dpy, None, root, 0, 0, 0, 0, c->x + c->w / 2, c->y + c->h / 2);
    focus(c);
  } else
    XWarpPointer(dpy, None, root, 0, 0, 0, 0, selmon->wx + selmon->ww / 3,
                 selmon->wy + selmon->wh / 2);
}

Atom getatomprop(Client *c, Atom prop) {
  int di;
  unsigned long dl;
  unsigned char *p = NULL;
  Atom da, atom = None;
  /* FIXME getatomprop should return the number of items and a pointer to
   * the stored data instead of this workaround */
  Atom req = XA_ATOM;
  if (prop == xatom[XembedInfo])
    req = xatom[XembedInfo];

  if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, req, &da,
                         &di, &dl, &dl, &p) == Success &&
      p) {
    atom = *(Atom *)p;
    if (da == xatom[XembedInfo] && dl == 2)
      atom = ((Atom *)p)[1];
    XFree(p);
  }
  return atom;
}


Atom getatompropfromwin(Window w, Atom prop) {
  int di;
  unsigned long dl;
  unsigned char *p = NULL;
  Atom da, atom = None;
  /* FIXME getatomprop should return the number of items and a pointer to
   * the stored data instead of this workaround */
  Atom req = XA_ATOM;
  if (prop == xatom[XembedInfo])
    req = xatom[XembedInfo];

  if (XGetWindowProperty(dpy, w, prop, 0L, sizeof atom, False, req, &da,
                         &di, &dl, &dl, &p) == Success &&
      p) {
    atom = *(Atom *)p;
    if (da == xatom[XembedInfo] && dl == 2)
      atom = ((Atom *)p)[1];
    XFree(p);
  }
  return atom;
}


unsigned int 
judge_win_contain_state(Window w, Atom want_net_wm_state) 
{

    Atom net_wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
    // Atom net_wm_state_skip_pager = XInternAtom(dpy, "_NET_WM_STATE_SKIP_PAGER", False);

    Atom actual_type;
    int actual_format;
    unsigned long nitems;
    unsigned long bytes_after;
    unsigned char *prop;

    int status = XGetWindowProperty(dpy, w, net_wm_state, 0, (~0L), False, AnyPropertyType,
                                    &actual_type, &actual_format, &nitems, &bytes_after, &prop);

    if (status == Success && actual_type == XA_ATOM && nitems > 0)
    {
        Atom *atoms = (Atom *)prop;
        int i;
        for (i = 0; i < nitems; i++)
        {
            if (atoms[i] == want_net_wm_state)
            {
                XFree(prop);
                return 1;
            }
        }
        XFree(prop);
    }
    return 0;
}


int getrootptr(int *x, int *y) {
  int di;
  unsigned int dui;
  Window dummy;

  return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

long getstate(Window w) {
  int format;
  long result = -1;
  unsigned char *p = NULL;
  unsigned long n, extra;
  Atom real;

  if (XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False,
                         wmatom[WMState], &real, &format, &n, &extra,
                         (unsigned char **)&p) != Success)
    return -1;
  if (n != 0)
    result = *p;
  XFree(p);
  return result;
}

unsigned int getsystraywidth() {
  unsigned int w = 0;
  Client *i;
  if (showsystray)
    for (i = systray->icons; i;
         w += MAX(i->w, bh) + systrayspacing, i = i->next)
      ;
  return w ? w + systrayspacing : 0;
}

int gettextprop(Window w, Atom atom, char *text, unsigned int size) {
  char **list = NULL;
  int n;
  XTextProperty name;

  if (!text || size == 0)
    return 0;
  text[0] = '\0';
  if (!XGetTextProperty(dpy, w, &name, atom) || !name.nitems)
    return 0;
  if (name.encoding == XA_STRING)
    strncpy(text, (char *)name.value, size - 1);
  else {
    if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 &&
        *list) {
      strncpy(text, *list, size - 1);
      XFreeStringList(list);
    }
  }
  text[size - 1] = '\0';
  XFree(name.value);
  return 1;
}

void grabbuttons(Client *c, int focused) {
  updatenumlockmask();
  {
    unsigned int i, j;
    unsigned int modifiers[] = {0, LockMask, numlockmask,
                                numlockmask | LockMask};
    XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
    if (!focused)
      XGrabButton(dpy, AnyButton, AnyModifier, c->win, False, BUTTONMASK,
                  GrabModeSync, GrabModeSync, None, None);
    for (i = 0; i < LENGTH(buttons); i++)
      /*overview模式可以捕获任何在配置定义的鼠标按键事件，非overivew模式只能捕获
      键盘加鼠标事件，是为了让在overview模式可以用鼠标切换到窗口所在的tag,此外还要兼容非
      overview视图下的中键全屏*/
      if (buttons[i].click == ClkClientWin && c->mon->isoverview == 1) {
        for (j = 0; j < LENGTH(modifiers); j++)
          XGrabButton(dpy, buttons[i].button, buttons[i].mask | modifiers[j],
                      c->win, False, BUTTONMASK, GrabModeAsync, GrabModeSync,
                      None, None);
      } else if (buttons[i].click == ClkClientWin && c->mon->isoverview == 0 &&
                 CLEANMASK(buttons[i].mask) != 0) {
        for (j = 0; j < LENGTH(modifiers); j++)
          XGrabButton(dpy, buttons[i].button, buttons[i].mask | modifiers[j],
                      c->win, False, BUTTONMASK, GrabModeAsync, GrabModeSync,
                      None, None);
      } else if (buttons[i].click == ClkClientWin && c->mon->isoverview == 0 &&
                 CLEANMASK(buttons[i].mask) == 0 &&
                 buttons[i].button ==
                     Button2) { // 允许捕获非overview模式的单独中键
        for (j = 0; j < LENGTH(modifiers); j++)
          XGrabButton(dpy, buttons[i].button, buttons[i].mask | modifiers[j],
                      c->win, False, BUTTONMASK, GrabModeAsync, GrabModeSync,
                      None, None);
      }
  }
}

void grabkeys(void) {
  updatenumlockmask();
  {
    unsigned int i, j;
    unsigned int modifiers[] = {0, LockMask, numlockmask,
                                numlockmask | LockMask};
    KeyCode code;

    XUngrabKey(dpy, AnyKey, AnyModifier, root);
    for (i = 0; i < LENGTH(keys); i++)
      if ((code = XKeysymToKeycode(dpy, keys[i].keysym)))
        for (j = 0; j < LENGTH(modifiers); j++)
          XGrabKey(dpy, code, keys[i].mod | modifiers[j], root, True,
                   GrabModeAsync, GrabModeAsync);
  }
}

void hide(Client *c) {
  if (!c || HIDDEN(c))
    return;

  Window w = c->win;
  static XWindowAttributes ra, ca;

  // more or less taken directly from blackbox's hide() function
  XGrabServer(dpy);
  XGetWindowAttributes(dpy, root, &ra);
  XGetWindowAttributes(dpy, w, &ca);
  // prevent UnmapNotify events
  XSelectInput(dpy, root, ra.your_event_mask & ~SubstructureNotifyMask);
  XSelectInput(dpy, w, ca.your_event_mask & ~StructureNotifyMask);
  XUnmapWindow(dpy, w);
  setclientstate(c, IconicState);
  XSelectInput(dpy, root, ra.your_event_mask);
  XSelectInput(dpy, w, ca.your_event_mask);
  XUngrabServer(dpy);

  hiddenWinStack[++hiddenWinStackTop] = c;
  focus(c->snext);
  arrange(c->mon);
}

void hideotherwins(const Arg *arg) {
  Client *c = (Client *)arg->v, *tc = NULL;
  for (tc = selmon->clients; tc; tc = tc->next)
    if (tc != c && ISVISIBLE(tc))
      hide(tc);
  show(c);
  focus(c);
}

void showonlyorall(const Arg *arg) {
  Client *c;
  if (issinglewin(NULL) || !selmon->sel) {
    for (c = selmon->clients; c; c = c->next)
      if (ISVISIBLE(c))
        show(c);
  } else
    hideotherwins(&(Arg){.v = selmon->sel});
}

void incnmaster(const Arg *arg) {
  int nmaster = selmon->nmaster + arg->i;
  if (selmon->bt <= 1)
    nmaster = 1;
  else if (nmaster >= 3)
    nmaster = 1;
  selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag] =
      MAX(nmaster, 1);
  arrange(selmon);
}

#ifdef XINERAMA
static int isuniquegeom(XineramaScreenInfo *unique, size_t n,
                        XineramaScreenInfo *info) {
  while (n--)
    if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org &&
        unique[n].width == info->width && unique[n].height == info->height)
      return 0;
  return 1;
}
#endif /* XINERAMA */

// 键盘按键事件处理函数
void keypress(XEvent *e) {
  unsigned int i;
  KeySym keysym;
  XKeyEvent *ev;

  ev = &e->xkey;
  keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);
  // 纠正右边数字键盘的keycode to keysym错误
  switch (keysym) {
  case 0xff9e:
    keysym = 0xffb0;
    break;
  case 0xff9c:
    keysym = 0xffb1;
    break;
  case 0xff99:
    keysym = 0xffb2;
    break;
  case 0xff9b:
    keysym = 0xffb3;
    break;
  case 0xff96:
    keysym = 0xffb4;
    break;
  case 0xff9d:
    keysym = 0xffb5;
    break;
  case 0xff98:
    keysym = 0xffb6;
    break;
  case 0xff95:
    keysym = 0xffb7;
    break;
  case 0xff97:
    keysym = 0xffb8;
    break;
  case 0xff9a:
    keysym = 0xffb9;
    break;
  default:;
    break;
  }
  for (i = 0; i < LENGTH(keys); i++)
    if (keysym == keys[i].keysym &&
        CLEANMASK(keys[i].mod) == CLEANMASK(ev->state) && keys[i].func)
      keys[i].func(&(keys[i].arg));
}

void killclient(const Arg *arg) {
  Client *c;
  int n = 0;

  if (!selmon->sel)
    return;
  if (!sendevent(selmon->sel->win, wmatom[WMDelete], NoEventMask,
                 wmatom[WMDelete], CurrentTime, 0, 0, 0)) {
    XGrabServer(dpy);
    XSetErrorHandler(xerrordummy);
    XSetCloseDownMode(dpy, DestroyAll);
    XKillClient(dpy, selmon->sel->win);
    XSync(dpy, False);
    XSetErrorHandler(xerror);
    XUngrabServer(dpy);
  }
  for (c = selmon->clients; c; c = c->next)
    if (ISVISIBLE(c) && !HIDDEN(c))
      n++;
  if (n <= 1)
    focusstack(NULL);
}

void clear_client(Client *fc) {
  Client *c;
  int n = 0;

  if (!fc)
    return;
  if (!sendevent(fc->win, wmatom[WMDelete], NoEventMask,
                 wmatom[WMDelete], CurrentTime, 0, 0, 0)) {
    XGrabServer(dpy);
    XSetErrorHandler(xerrordummy);
    XSetCloseDownMode(dpy, DestroyAll);
    XKillClient(dpy, fc->win);
    XSync(dpy, False);
    XSetErrorHandler(xerror);
    XUngrabServer(dpy);
  }
  for (c = selmon->clients; c; c = c->next)
    if (ISVISIBLE(c) && !HIDDEN(c))
      n++;
  if (n <= 1)
    focusstack(NULL);
}


void forcekillclient(const Arg *arg) {
  if (!selmon->sel)
    return;
  killclient(arg);
  unmanage(selmon->sel, 1);
}

void managefloating(Client *c) {
  Client *tc;
  int d1 = 0, d2 = 0, tx, ty;
  int tryed = 0;
  while (tryed++ < 10) {
    int dw, dh, existed = 0;
    dw = (selmon->ww / 20) * d1, dh = (selmon->wh / 20) * d2;
    tx = c->x + dw, ty = c->y + dh;
    for (tc = selmon->clients; tc; tc = tc->next) {
      if (ISVISIBLE(tc) && !HIDDEN(tc) && tc != c && tc->x == tx &&
          tc->y == ty) {
        existed = 1;
        break;
      }
    }
    if (!existed) {
      c->x = tx;
      c->y = ty;
      break;
    } else {
      while (d1 == 0)
        d1 = rand() % 7 - 3;
      while (d2 == 0)
        d2 = rand() % 7 - 3;
    }
  }
}

void manage(Window w, XWindowAttributes *wa) {
  Client *c, *t = NULL;
  Window trans = None;
  XWindowChanges wc;

  c = ecalloc(1, sizeof(Client));
  c->win = w;
  /* geometry */
  c->x = c->oldx = c->overview_backup_x = c->fullscreen_backup_x = wa->x;
  c->y = c->oldy = c->overview_backup_y = c->fullscreen_backup_y = wa->y;
  c->w = c->oldw = c->overview_backup_w = c->fullscreen_backup_w = wa->width;
  c->h = c->oldh = c->overview_backup_h = c->fullscreen_backup_h = wa->height;
  // c->oldbw = wa->border_width;
  c->bw = c->oldbw = c->overview_backup_bw = borderpx;
  c->isfloating = 0;
  c->isfullscreen = 0;
  c->no_limit_taskw = 0;
  c->isactive = 0;
  updatetitle(c);
  updateicon(c);
  if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
    c->mon = t->mon;
    c->tags = t->tags;
  } else {
    c->mon = selmon;
    applyrules(c);
  }
  wc.border_width = c->bw;

  if (c->x + WIDTH(c) > c->mon->mx + c->mon->mw)
    c->x = c->mon->mx + c->mon->mw - WIDTH(c);
  if (c->y + HEIGHT(c) > c->mon->my + c->mon->mh)
    c->y = c->mon->my + c->mon->mh - HEIGHT(c);
  c->x = MAX(c->x, c->mon->mx);
  /* only fix client y-offset, if the client center might cover the bar */
  c->y = MAX(c->y,
             ((c->mon->by == c->mon->my) && (c->x + (c->w / 2) >= c->mon->wx) &&
              (c->x + (c->w / 2) < c->mon->wx + c->mon->ww))
                 ? bh
                 : c->mon->my);

  if (c->isfloating) {
    if (c->x == 0 && c->y == 0) {
      c->x = selmon->wx + (selmon->ww - c->w) / 2;
      c->y = selmon->wy + (selmon->wh - c->h) / 2;
    }
    managefloating(c);
  }

  XConfigureWindow(dpy, w, CWBorderWidth, &wc);
  XSetWindowBorder(dpy, w, scheme[SchemeNorm][ColBorder].pixel);
  configure(c); /* propagates border_width, if size doesn't change */
  updatewindowtype(c);
  updatesizehints(c);
  updatewmhints(c);
  XSelectInput(dpy, w,
               EnterWindowMask | FocusChangeMask | PropertyChangeMask |
                   StructureNotifyMask);
  grabbuttons(c, 0);
  if (!c->isfloating)
    c->isfloating = c->oldstate = trans != None || c->isfixed;
  if (c->isfloating)
    XRaiseWindow(dpy, c->win);
  attach(c);
  attachstack(c);
  XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32,
                  PropModeAppend, (unsigned char *)&(c->win), 1);
  XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w,
                    c->h); /* some windows require this */
  if (!HIDDEN(c))
    setclientstate(c, NormalState);
  if (c->mon == selmon)
    unfocus(selmon->sel, 0);
  c->mon->sel = c;
  arrange(c->mon);
  if (!HIDDEN(c))
    XMapWindow(dpy, c->win);
  focus(NULL);
}

void mappingnotify(XEvent *e) {
  XMappingEvent *ev = &e->xmapping;

  XRefreshKeyboardMapping(ev);
  if (ev->request == MappingKeyboard)
    grabkeys();
}

void maprequest(XEvent *e) {
  static XWindowAttributes wa;
  XMapRequestEvent *ev = &e->xmaprequest;
  Client *i;
  if ((i = wintosystrayicon(ev->window))) {
    sendevent(i->win, netatom[Xembed], StructureNotifyMask, CurrentTime,
              XEMBED_WINDOW_ACTIVATE, 0, systray->win, XEMBED_EMBEDDED_VERSION);
    resizebarwin(selmon);
    updatesystray();
  }

  if (!XGetWindowAttributes(dpy, ev->window, &wa))
    return;
  if (wa.override_redirect)
    return;
  if (!wintoclient(ev->window))
    manage(ev->window, &wa);
}

void toggle_hotarea(int x_root, int y_root) {
  // 左下角热区坐标计算,兼容多显示屏
  Arg arg = {0};
  unsigned hx = selmon->mx + hotarea_size;
  unsigned hy = selmon->my + selmon->mh - hotarea_size;

  if (enable_hotarea == 1 && selmon->is_in_hotarea == 0 && y_root > hy &&
      x_root < hx && x_root >= selmon->mx &&
      y_root <= (selmon->my + selmon->mh)) {
    toggleoverview(&arg);
    selmon->is_in_hotarea = 1;
  } else if (enable_hotarea == 1 && selmon->is_in_hotarea == 1 &&
             (y_root <= hy || x_root >= hx || x_root < selmon->mx ||
              y_root > (selmon->my + selmon->mh))) {
    selmon->is_in_hotarea = 0;
  }
}

void motionnotify(XEvent *e) {
  static Monitor *mon = NULL;
  Monitor *m;
  XMotionEvent *ev = &e->xmotion;

  if (ev->window != root)
    return;

  // 判断鼠标的位置自动聚焦到鼠标所在的标题的窗口
  unsigned int i, x, occ = 0;
  Client *c = wintoclient(ev->window);

  // click = ClkRootWin;
  /* focus monitor if necessary */
  if ((m = wintomon(ev->window)) && m != selmon) {
    unfocus(selmon->sel, 1);
    selmon = m;
    focus(NULL);
  }

  int status_w = selmon->status_w;
  int system_w = getsystraywidth();
  if (ev->window == selmon->barwin ||
      (!c && selmon->showbar &&
       (topbar ? ev->y_root <= selmon->wy
               : ev->y_root >= selmon->wy + selmon->wh))) { // 点击在bar上
    i = x = 0;
    blw = TEXTW(selmon->ltsymbol);

    if (selmon->isoverview) {
      x += TEXTW(overviewtag);
      i = ~0;
      if (ev->x_root > x)
        i = LENGTH(tags);
    } else {
      for (c = m->clients; c; c = c->next)
        occ |= c->tags == TAGMASK ? 0 : c->tags;
      do {
        /* do not reserve space for vacant tags */
        if (!(occ & 1 << i || m->tagset[m->seltags] & 1 << i))
          continue;
        x += TEXTW(tags[i]);
      } while (ev->x_root >= x && ++i < LENGTH(tags));
    }
    if (i < LENGTH(tags)) { // 点击在tab上
      // click = ClkTagBar;
      // arg.ui = 1 << i;
    } else if (ev->x_root < x + blw) { // 点击在布局标签上
      // click = ClkLtSymbol;
    } else if (ev->x_root >
               selmon->ww - status_w - 2 * sp -
                   (selmon == systraytomon(selmon)
                        ? (system_w ? system_w + systraypinning + 2 : 0)
                        : 0)) {
      // click = ClkStatusText;
      // arg.i = ev->x_root - (selmon->ww - status_w - 2 * sp -
      //                  (selmon == systraytomon(selmon)
      //                       ? (system_w ? system_w + systraypinning + 2 : 0)
      //                       : 0));
      // arg.ui = ev->button; // 1 => L，2 => M，3 => R, 5 => U, 6 => D
    } else {
      // click = ClkBarEmpty;

      x += blw;
      c = m->clients;

      if (m->bt != 0)
        do {
          if (!ISVISIBLE(c))
            continue;
          else
            x += c->taskw;
        } while (ev->x_root > x && (c = c->next));

      if (c && taskbar_movemouse_focus == 1) {
        // click = ClkWinTitle; //鼠标在标题栏上
        // arg.v = c;
        focus(c); // 焦点切换到该标题对应的窗口
      }
    }
  }

  // 监测热区触发overview
  // toggle_hotarea(ev->x_root,ev->y_root);

  if ((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) {
    unfocus(selmon->sel, 1);
    selmon = m;
    focus(NULL);
  }
  mon = m;
}

// 鼠标拖拽窗口
void movemouse(const Arg *arg) {
  int x, y, ocx, ocy, nx, ny;
  Client *c;
  Monitor *m;
  XEvent ev;
  Time lasttime = 0;

  if (!(c = selmon->sel))
    return;
  if (c->isfullscreen) /* no support moving fullscreen windows by mouse */
    return;
  restack(selmon);
  ocx = c->x;
  ocy = c->y;
  if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                   None, cursor[CurMove]->cursor, CurrentTime) != GrabSuccess)
    return;
  if (!getrootptr(&x, &y))
    return;
  do {
    XMaskEvent(dpy, MOUSEMASK | ExposureMask | SubstructureRedirectMask, &ev);
    switch (ev.type) {
    case ConfigureRequest:
    case Expose:
    case MapRequest:
      handler[ev.type](&ev);
      break;
    case MotionNotify:
      if ((ev.xmotion.time - lasttime) <= (1000 / 120))
        continue;
      lasttime = ev.xmotion.time;

      nx = ocx + (ev.xmotion.x - x);
      ny = ocy + (ev.xmotion.y - y);
      if (abs(selmon->wx - nx) < snap)
        nx = selmon->wx;
      else if (abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < snap)
        nx = selmon->wx + selmon->ww - WIDTH(c);
      if (abs(selmon->wy - ny) < snap)
        ny = selmon->wy;
      else if (abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < snap)
        ny = selmon->wy + selmon->wh - HEIGHT(c);
      if (!c->isfloating && (abs(nx - c->x) > snap || abs(ny - c->y) > snap)) {
        c->isfloating = 1;
        arrange(selmon);
        if (ev.xmotion.x - nx < c->w / 2 && ev.xmotion.y - ny < c->h / 2 &&
            (c->w > selmon->ww * 0.5 || c->h > selmon->wh * 0.5)) {
          resize(c, nx, ny, c->w > selmon->ww * 0.5 ? c->w / 2 : c->w,
                 c->h > selmon->wh * 0.5 ? c->h / 2 : c->h, 0);
          break;
        }
      }
      if (c->isfloating)
        resize(c, nx, ny, c->w, c->h, 1);
      break;
    }
  } while (ev.type != ButtonRelease);
  XUngrabPointer(dpy, CurrentTime);
  if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
    sendmon(c, m);
    selmon = m;
    focus(NULL);
  }
}

// 窗口位置移动
void movewin(const Arg *arg) {
  Client *c, *tc;
  int nx, ny;
  int buttom, top, left, right, tar;
  c = selmon->sel;
  if (!c || c->isfullscreen)
    return;
  if (!c->isfloating)
    togglefloating(NULL);
  nx = c->x;
  ny = c->y;
  switch (arg->ui) {
  case UP:
    tar = -99999;
    top = c->y;
    ny -= c->mon->wh / 4;
    for (tc = c->mon->clients; tc; tc = tc->next) {
      // 若浮动tc c的顶边会穿过tc的底边
      if (!ISVISIBLE(tc) || !tc->isfloating || tc == c)
        continue;
      if (c->x + WIDTH(c) < tc->x || c->x > tc->x + WIDTH(tc))
        continue;
      buttom = tc->y + HEIGHT(tc) + gappi;
      if (top > buttom && ny < buttom) {
        tar = MAX(tar, buttom);
      };
    }
    ny = tar == -99999 ? ny : tar;
    ny = MAX(ny, c->mon->wy + gappo);
    break;
  case DOWN:
    tar = 99999;
    buttom = c->y + HEIGHT(c);
    ny += c->mon->wh / 4;
    for (tc = c->mon->clients; tc; tc = tc->next) {
      // 若浮动tc c的底边会穿过tc的顶边
      if (!ISVISIBLE(tc) || !tc->isfloating || tc == c)
        continue;
      if (c->x + WIDTH(c) < tc->x || c->x > tc->x + WIDTH(tc))
        continue;
      top = tc->y - gappi;
      if (buttom < top && (ny + HEIGHT(c)) > top) {
        tar = MIN(tar, top - HEIGHT(c));
      };
    }
    ny = tar == 99999 ? ny : tar;
    ny = MIN(ny, c->mon->wy + c->mon->wh - gappo - HEIGHT(c));
    break;
  case LEFT:
    tar = -99999;
    left = c->x;
    nx -= c->mon->ww / 6;
    for (tc = c->mon->clients; tc; tc = tc->next) {
      // 若浮动tc c的左边会穿过tc的右边
      if (!ISVISIBLE(tc) || !tc->isfloating || tc == c)
        continue;
      if (c->y + HEIGHT(c) < tc->y || c->y > tc->y + HEIGHT(tc))
        continue;
      right = tc->x + WIDTH(tc) + gappi;
      if (left > right && nx < right) {
        tar = MAX(tar, right);
      };
    }
    nx = tar == -99999 ? nx : tar;
    nx = MAX(nx, c->mon->wx + gappo);
    break;
  case RIGHT:
    tar = 99999;
    right = c->x + WIDTH(c);
    nx += c->mon->ww / 6;
    for (tc = c->mon->clients; tc; tc = tc->next) {
      // 若浮动tc c的右边会穿过tc的左边
      if (!ISVISIBLE(tc) || !tc->isfloating || tc == c)
        continue;
      if (c->y + HEIGHT(c) < tc->y || c->y > tc->y + HEIGHT(tc))
        continue;
      left = tc->x - gappi;
      if (right < left && (nx + WIDTH(c)) > left) {
        tar = MIN(tar, left - WIDTH(c));
      };
    }
    nx = tar == 99999 ? nx : tar;
    nx = MIN(nx, c->mon->wx + c->mon->ww - gappo - WIDTH(c));
    break;
  }
  resize(c, nx, ny, c->w, c->h, 1);
  pointerfocuswin(c);
  restack(selmon);
}

// 窗口大小缩放
void resizewin(const Arg *arg) {
  Client *c, *tc;
  int nh, nw;
  int buttom, top, left, right, tar;
  c = selmon->sel;
  if (!c || c->isfullscreen)
    return;
  if (!c->isfloating)
    togglefloating(NULL);
  nw = c->w;
  nh = c->h;
  switch (arg->ui) {
  case H_EXPAND: // 右
    tar = 99999;
    right = c->x + WIDTH(c);
    nw += selmon->ww / 16;
    for (tc = c->mon->clients; tc; tc = tc->next) {
      // 若浮动tc c的右边会穿过tc的左边
      if (!ISVISIBLE(tc) || !tc->isfloating || tc == c)
        continue;
      if (c->y + HEIGHT(c) < tc->y || c->y > tc->y + HEIGHT(tc))
        continue;
      left = tc->x - gappi;
      if (right < left && (c->x + nw) > left) {
        tar = MIN(tar, left - c->x - 2 * c->bw);
      };
    }
    nw = tar == 99999 ? nw : tar;
    if (c->x + nw + gappo + 2 * c->bw > selmon->wx + selmon->ww)
      nw = selmon->wx + selmon->ww - c->x - gappo - 2 * c->bw;
    break;
  case H_REDUCE: // 左
    nw -= selmon->ww / 16;
    nw = MAX(nw, selmon->ww / 10);
    break;
  case V_EXPAND: // 下
    tar = -99999;
    buttom = c->y + HEIGHT(c);
    nh += selmon->wh / 8;
    for (tc = c->mon->clients; tc; tc = tc->next) {
      // 若浮动tc c的底边会穿过tc的顶边
      if (!ISVISIBLE(tc) || !tc->isfloating || tc == c)
        continue;
      if (c->x + WIDTH(c) < tc->x || c->x > tc->x + WIDTH(tc))
        continue;
      top = tc->y - gappi;
      if (buttom < top && (c->y + nh) > top) {
        tar = MAX(tar, top - c->y - 2 * c->bw);
      };
    }
    nh = tar == -99999 ? nh : tar;
    if (c->y + nh + gappo + 2 * c->bw > selmon->wy + selmon->wh)
      nh = selmon->wy + selmon->wh - c->y - gappo - 2 * c->bw;
    break;
  case V_REDUCE: // 上
    nh -= selmon->wh / 8;
    nh = MAX(nh, selmon->wh / 10);
    break;
  }
  resize(c, c->x, c->y, nw, nh, 1);
  XWarpPointer(dpy, None, root, 0, 0, 0, 0, c->x + c->w - 2 * c->bw,
               c->y + c->h - 2 * c->bw);
  restack(selmon);
}

// 查找下一个需要布局窗口
Client *nexttiled(Client *c) {
  // for (; c && (c->isfullscreen || c->isfloating || !ISVISIBLE(c) ||
  // HIDDEN(c)); c = c->next)
  //   ;
  // return c;
  for (;; c = c->next) {
    if (c == NULL) {
      return c;
    }
    if (!c->isfullscreen && !c->isfloating && ISVISIBLE(c) && !HIDDEN(c)) {
      return c;
    }
  }
}

void pop(Client *c) {
  detach(c);
  c->next = c->mon->clients;
  c->mon->clients = c;
  focus(c);
  arrange(c->mon);
  pointerfocuswin(c);
}

void propertynotify(XEvent *e) {
  Client *c;
  Window trans;
  XPropertyEvent *ev = &e->xproperty;

  if ((c = wintosystrayicon(ev->window))) {
    if (ev->atom == XA_WM_NORMAL_HINTS) {
      updatesizehints(c);
      updatesystrayicongeom(c, c->w, c->h);
    } else
      updatesystrayiconstate(c, ev);
    resizebarwin(selmon);
    updatesystray();
  }
  if ((ev->window == root) && (ev->atom == XA_WM_NAME))
    updatestatus();
  else if (ev->state == PropertyDelete)
    return; /* ignore */
  else if ((c = wintoclient(ev->window))) {
    switch (ev->atom) {
    default:
      break;
    case XA_WM_TRANSIENT_FOR:
      if (!c->isfloating && (XGetTransientForHint(dpy, c->win, &trans)) &&
          (c->isfloating = (wintoclient(trans)) != NULL))
        arrange(c->mon);
      break;
    case XA_WM_NORMAL_HINTS:
      updatesizehints(c);
      break;
    case XA_WM_HINTS:
      updatewmhints(c);
      drawbars();
      break;
    }
    if (ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
      updatetitle(c);
      if (c == c->mon->sel)
        drawbar(c->mon);
    }
    if (ev->atom == netatom[NetWMWindowType])
      updatewindowtype(c);
  }
}

void quit(const Arg *arg) 
{ 
  Client *c;
  Monitor *m;
  for (m = mons; m; m = m->next){
    for (c = m->clients; c; c = c->next)
      clear_client(c);
  }
  running = 0; 
}

Monitor *recttomon(int x, int y, int w, int h) {
  Monitor *m, *r = selmon;
  int a, area = 0;

  for (m = mons; m; m = m->next)
    if ((a = INTERSECT(x, y, w, h, m)) > area) {
      area = a;
      r = m;
    }
  return r;
}

void removesystrayicon(Client *i) {
  Client **ii;

  if (!showsystray || !i)
    return;
  for (ii = &systray->icons; *ii && *ii != i; ii = &(*ii)->next)
    ;
  if (ii)
    *ii = i->next;
  free(i);
}

void resize(Client *c, int x, int y, int w, int h, int interact) {
  if (applysizehints(c, &x, &y, &w, &h, interact)) {
    resizeclient(c, x, y, w, h);
  }
}

void resizebarwin(Monitor *m) {
  unsigned int w = m->ww;
  uint system_w = getsystraywidth();
  if (showsystray && m == systraytomon(m))
    w -= system_w;
  XMoveResizeWindow(
      dpy, m->barwin, m->wx + sp, m->by + vp,
      w - 2 * sp - (m == systraytomon(m) && system_w ? systrayspadding : 0),
      bh); // 如果托盘存在 额外减去systrayspadding
}

void resizeclient(Client *c, int x, int y, int w, int h) {
  XWindowChanges wc;

  c->oldx = c->x;
  c->x = wc.x = x;
  c->oldy = c->y;
  c->y = wc.y = y;
  c->oldw = c->w;
  c->w = wc.width = w;
  c->oldh = c->h;
  c->h = wc.height = h;
  wc.border_width = c->bw;

  if (((nexttiled(c->mon->clients) == c && !nexttiled(c->next))) &&
      !c->isfullscreen && !c->isfloating && no_stack_show_border == 0) {
    c->w = wc.width += c->bw * 2;
    c->h = wc.height += c->bw * 2;
    wc.border_width = 0;
  }
  XConfigureWindow(dpy, c->win, CWX | CWY | CWWidth | CWHeight | CWBorderWidth,
                   &wc);
  configure(c);
  XSync(dpy, False);
}

void resizemouse(const Arg *arg) {
  int ocx, ocy, nw, nh;
  Client *c;
  Monitor *m;
  XEvent ev;
  Time lasttime = 0;

  if (!(c = selmon->sel))
    return;
  if (c->isfullscreen) /* no support resizing fullscreen windows by mouse */
    return;
  restack(selmon);
  ocx = c->x;
  ocy = c->y;
  if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                   None, cursor[CurResize]->cursor, CurrentTime) != GrabSuccess)
    return;
  XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1,
               c->h + c->bw - 1);
  do {
    XMaskEvent(dpy, MOUSEMASK | ExposureMask | SubstructureRedirectMask, &ev);
    switch (ev.type) {
    case ConfigureRequest:
    case Expose:
    case MapRequest:
      handler[ev.type](&ev);
      break;
    case MotionNotify:
      if ((ev.xmotion.time - lasttime) <= (1000 / 120))
        continue;
      lasttime = ev.xmotion.time;

      nw = MAX(ev.xmotion.x - ocx - 2 * c->bw + 1, 1);
      nh = MAX(ev.xmotion.y - ocy - 2 * c->bw + 1, 1);
      if (c->mon->wx + nw >= selmon->wx &&
          c->mon->wx + nw <= selmon->wx + selmon->ww &&
          c->mon->wy + nh >= selmon->wy &&
          c->mon->wy + nh <= selmon->wy + selmon->wh) {
        if (!c->isfloating &&
            (abs(nw - c->w) > snap || abs(nh - c->h) > snap)) {
          c->isfloating = 1;
          arrange(selmon);
        }
      }
      if (c->isfloating)
        resize(c, c->x, c->y, nw, nh, 1);
      break;
    }
  } while (ev.type != ButtonRelease);
  XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1,
               c->h + c->bw - 1);
  XUngrabPointer(dpy, CurrentTime);
  while (XCheckMaskEvent(dpy, EnterWindowMask, &ev))
    ;
  if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
    sendmon(c, m);
    selmon = m;
    focus(NULL);
  }
}

void resizerequest(XEvent *e) {
  XResizeRequestEvent *ev = &e->xresizerequest;
  Client *i;

  if ((i = wintosystrayicon(ev->window))) {
    updatesystrayicongeom(i, ev->width, ev->height);
    resizebarwin(selmon);
    updatesystray();
  }
}

void restack(Monitor *m) {
  Client *c;
  XEvent ev;
  XWindowChanges wc;

  drawbar(m);
  if (!m->sel)
    return;
  if (m->sel->isfloating)
    XRaiseWindow(dpy, m->sel->win);
  wc.stack_mode = Below;
  wc.sibling = m->barwin;
  for (c = m->stack; c; c = c->snext)
    if (!c->isfloating && ISVISIBLE(c)) {
      XConfigureWindow(dpy, c->win, CWSibling | CWStackMode, &wc);
      wc.sibling = c->win;
    }
  XSync(dpy, False);
  while (XCheckMaskEvent(dpy, EnterWindowMask, &ev))
    ;
}

void run(void) {
  XEvent xevent;
  /* main event loop */
  XSync(dpy, False);
  while (running && !XNextEvent(dpy, &xevent)) {
    if (xevent.xcookie.type != GenericEvent ||
        xevent.xcookie.extension != xi_opcode) {
      /* 不是一个xinput2扩展事件,而是xlib的事件 */
      if (handler[xevent.type])
        handler[xevent.type](&xevent); /* call handler */
    } else {                           // xinput2扩展输入事件
      xi_handler(xevent);              // 处理扩展输入事件
    }
  }
}

void runAutostart(void) {
  char cmd[100];
  sprintf(cmd, "%s &", autostartscript);
  system(cmd);
}

void scan(void) {
  unsigned int i, num;
  Window d1, d2, *wins = NULL;
  XWindowAttributes wa;

  if (XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
    for (i = 0; i < num; i++) {
      if (!XGetWindowAttributes(dpy, wins[i], &wa) || wa.override_redirect ||
          XGetTransientForHint(dpy, wins[i], &d1))
        continue;
      if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
        manage(wins[i], &wa);
    }
    for (i = 0; i < num; i++) { /* now the transients */
      if (!XGetWindowAttributes(dpy, wins[i], &wa))
        continue;
      if (XGetTransientForHint(dpy, wins[i], &d1) &&
          (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
        manage(wins[i], &wa);
    }
    if (wins)
      XFree(wins);
  }
}

void sendmon(Client *c, Monitor *m) {
  if (c->mon == m)
    return;
  unfocus(c, 1);
  detach(c);
  detachstack(c);
  c->mon = m;
  c->tags = m->tagset[m->seltags]; /* assign tags of target monitor */
  attach(c);
  attachstack(c);
  focus(NULL);
  arrange(NULL);
}

void setclientstate(Client *c, long state) {
  long data[] = {state, None};

  XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
                  PropModeReplace, (unsigned char *)data, 2);
}

void tagtoleft(const Arg *arg) {
  if (selmon->sel != NULL &&
      __builtin_popcount(selmon->tagset[selmon->seltags] & TAGMASK) == 1 &&
      selmon->tagset[selmon->seltags] > 1) {
    tag(&(Arg){.ui = selmon->tagset[selmon->seltags] >> 1});
  }
}

void tagtoright(const Arg *arg) {
  if (selmon->sel != NULL &&
      __builtin_popcount(selmon->tagset[selmon->seltags] & TAGMASK) == 1 &&
      selmon->tagset[selmon->seltags] & (TAGMASK >> 1)) {
    tag(&(Arg){.ui = selmon->tagset[selmon->seltags] << 1});
  }
}

int sendevent(Window w, Atom proto, int mask, long d0, long d1, long d2,
              long d3, long d4) {
  int n;
  Atom *protocols, mt;
  int exists = 0;
  XEvent ev;

  if (proto == wmatom[WMTakeFocus] || proto == wmatom[WMDelete]) {
    mt = wmatom[WMProtocols];
    if (XGetWMProtocols(dpy, w, &protocols, &n)) {
      while (!exists && n--)
        exists = protocols[n] == proto;
      XFree(protocols);
    }
  } else {
    exists = True;
    mt = proto;
  }
  if (exists) {
    ev.type = ClientMessage;
    ev.xclient.window = w;
    ev.xclient.message_type = mt;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = d0;
    ev.xclient.data.l[1] = d1;
    ev.xclient.data.l[2] = d2;
    ev.xclient.data.l[3] = d3;
    ev.xclient.data.l[4] = d4;
    XSendEvent(dpy, w, False, mask, &ev);
  }
  return exists;
}

void setfocus(Client *c) {
  if (!c->neverfocus) {
    XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
    XChangeProperty(dpy, root, netatom[NetActiveWindow], XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&(c->win), 1);
  }
  sendevent(c->win, wmatom[WMTakeFocus], NoEventMask, wmatom[WMTakeFocus],
            CurrentTime, 0, 0, 0);
}

void setfullscreen(Client *c) {
  unsigned int border_type;
  if (!c->isfullscreen) {
    XChangeProperty(
        dpy, c->win, netatom[NetWMState], XA_ATOM, 32, PropModeReplace,
        (unsigned char *)&netatom[NetWMFullscreen],
        1); // 设置窗口在x11状态,实际xprop并没看到改对了,状态还是normal
    c->isfullscreen = 1;
    c->oldstate = c->isfloating;
    c->oldbw = c->bw;
    c->bw = 0;
    c->isfloating = 0; // 全屏不浮动才能自动退出全屏参与平铺
    c->fullscreen_backup_x = c->x;
    c->fullscreen_backup_y = c->y;
    c->fullscreen_backup_w = c->w;
    c->fullscreen_backup_h = c->h;
    resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
    XRaiseWindow(dpy, c->win);

  } else {

    XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)0, 0);

    c->isfullscreen = 0;
    c->isfloating = c->oldstate;
    c->bw = borderpx;
    resizeclient(c, c->fullscreen_backup_x, c->fullscreen_backup_y,
                 c->fullscreen_backup_w, c->fullscreen_backup_h);
    border_type = get_border_type(c); // 确认窗口边框的颜色
    XSetWindowBorder(dpy, c->win, scheme[border_type][ColBorder].pixel);

    arrange(c->mon);

  }
}

// 假全屏切换
void set_fake_fullscreen(Client *c) {
  uint border_type;
  if (!c->isfullscreen) {
    // XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
    //                 PropModeReplace, (unsigned char
    //                 *)&netatom[NetWMFullscreen], 1);
    c->isfullscreen = 1;
    c->oldstate = c->isfloating;
    // c->oldbw = c->bw;
    // c->bw = 0;
    c->isfloating = 0; // 全屏不浮动才能自动退出全屏参与平铺
    // 要多减一个gappo,因为mon->wx,wh不包含右边和下边看不见的10px区域
    // 鼠标在屏幕左上都不可以超出去的,右下可以超出10px
    c->fullscreen_backup_x = c->x;
    c->fullscreen_backup_y = c->y;
    c->fullscreen_backup_w = c->w;
    c->fullscreen_backup_h = c->h;
    resizeclient(c, c->mon->wx + gappo, c->mon->wy + gappo,
                 c->mon->ww - (gappo * 2) - gappo,
                 c->mon->wh - (gappo * 2) - gappo);
    XRaiseWindow(dpy, c->win);        // 提升窗口到顶层
    border_type = get_border_type(c); // 确认窗口边框的颜色
    XSetWindowBorder(dpy, c->win, scheme[border_type][ColBorder].pixel);

  } else {
    XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)0, 0);
    c->isfullscreen = 0;
    c->isfloating = c->oldstate;
    c->bw = borderpx;
    resizeclient(c, c->fullscreen_backup_x, c->fullscreen_backup_y,
                 c->fullscreen_backup_w, c->fullscreen_backup_h);
    border_type = get_border_type(c); // 确认窗口边框的颜色
    XSetWindowBorder(dpy, c->win, scheme[border_type][ColBorder].pixel);
    arrange(c->mon);

  }
}

unsigned int want_auto_fullscren(Client *c) {
	int nodeNumInTargetTag = 1;
  Client *fc;

	// if client is fullscreen before,don't make it fullscreen
	if (!c || c->overview_isfullscreenbak) {
		return 0;
	}

	// caculate the number of clients that will be in the same workspace with pWindow(don't contain itself)
  for (fc = selmon->clients; fc; fc = fc->next) {
    if (fc != selmon->sel && fc->tags == selmon->sel->tags) {
      nodeNumInTargetTag++;
    }
  }

	// if only one client in workspace(pWindow itself), don't make it fullscreen
	if(nodeNumInTargetTag > 1) {
		return 1;
	} else {
		return 0;
	}
}

void fullscreen(const Arg *arg) {
  if (!selmon->sel) { // 显示器有窗口
    return;
  }

  if (selmon->isoverview && want_auto_fullscren(selmon->sel) && !auto_fullscreen) {
			toggleoverview(&(Arg){0});
      setfullscreen(selmon->sel);
  } else if (selmon->isoverview && (!want_auto_fullscren(selmon->sel) || auto_fullscreen)) {
    toggleoverview(&(Arg){0});
  } else {
    setfullscreen(selmon->sel);
  }

	selmon->sel->is_scratchpad_show = 0;
	selmon->sel->is_in_scratchpad = 0;
	selmon->sel->scratchpad_priority = 0;
}

void fake_fullscreen(const Arg *arg) {
  if (!selmon->sel) { // 显示器有窗口
    return;
  }

  if (selmon->isoverview && want_auto_fullscren(selmon->sel) && !auto_fullscreen) {
			toggleoverview(&(Arg){0});
      set_fake_fullscreen(selmon->sel);
  } else if (selmon->isoverview && (!want_auto_fullscren(selmon->sel) || auto_fullscreen)) {
    toggleoverview(&(Arg){0});
  } else {
    set_fake_fullscreen(selmon->sel);
  }

	selmon->sel->is_scratchpad_show = 0;
	selmon->sel->is_in_scratchpad = 0;
	selmon->sel->scratchpad_priority = 0;
}

void selectlayout(const Arg *arg) {
  const Layout *cur = selmon->lt[selmon->sellt];
  const Layout *target = cur == arg->v ? &layouts[0] : arg->v;
  setlayout(&(Arg){.v = target});
}

void setlayout(const Arg *arg) {
  if (arg->v != selmon->lt[selmon->sellt])
    selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag] ^= 1;
  if (arg->v)
    selmon->lt[selmon->sellt] =
        selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt] =
            (Layout *)arg->v;
  arrange(selmon);
}

void setmfact(const Arg *arg) {
  float f;

  if (!arg)
    return;
  f = arg->f < 1.0 ? arg->f + selmon->mfact : arg->f - 1.0;
  if (f < 0.05 || f > 0.95)
    return;
  selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag] = f;
  arrange(selmon);
}

void setup(void) {
  int i;
  XSetWindowAttributes wa;
  Atom utf8string;

  /* clean up any zombies immediately */
  sigchld(0);

  /* init screen */
  screen = DefaultScreen(dpy);
  sw = DisplayWidth(dpy, screen);
  sh = DisplayHeight(dpy, screen);
  root = RootWindow(dpy, screen);
  xinitvisual();
  drw = drw_create(dpy, screen, root, sw, sh, visual, depth, cmap);
  if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
    die("no fonts could be loaded.");
  lrpad = drw->fonts->h;
  bh = drw->fonts->h + 2;
  sp = sidepad;
  vp = (topbar == 1) ? vertpad : -vertpad;
  updategeom();
  /* init atoms */
  utf8string = XInternAtom(dpy, "UTF8_STRING", False);
  wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
  wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
  wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
  wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
  netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
  netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
  netatom[NetSystemTray] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False);
  netatom[NetSystemTrayOP] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
  netatom[NetSystemTrayOrientation] =
      XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION", False);
  netatom[NetSystemTrayOrientationHorz] =
      XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION_HORZ", False);
  netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
  netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
  netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
  netatom[NetWMFullscreen] =
      XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
  netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
  netatom[NetWMWindowTypeDialog] =
      XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
  netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
  xatom[Manager] = XInternAtom(dpy, "MANAGER", False);
  xatom[Xembed] = XInternAtom(dpy, "_XEMBED", False);
  xatom[XembedInfo] = XInternAtom(dpy, "_XEMBED_INFO", False);
  /* init cursors */
  cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
  cursor[CurResize] = drw_cur_create(drw, XC_sizing);
  cursor[CurMove] = drw_cur_create(drw, XC_fleur);
  /* init appearance */
  scheme = ecalloc(LENGTH(colors) + 1, sizeof(Clr *));
  scheme[LENGTH(colors)] = drw_scm_create(drw, colors[0], alphas[0], 3);
  for (i = 0; i < LENGTH(colors); i++)
    scheme[i] = drw_scm_create(drw, colors[i], alphas[i], 3);
  /* init system tray */
  updatesystray();
  /* init bars */
  updatebars();
  updatestatus();
  /* supporting window for NetWMCheck */
  wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
  XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
                  PropModeReplace, (unsigned char *)&wmcheckwin, 1);
  XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
                  PropModeReplace, (unsigned char *)"dwm", 3);
  XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
                  PropModeReplace, (unsigned char *)&wmcheckwin, 1);
  /* EWMH support per view */
  XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
                  PropModeReplace, (unsigned char *)netatom, NetLast);
  XDeleteProperty(dpy, root, netatom[NetClientList]);
  /* select events */
  wa.cursor = cursor[CurNormal]->cursor;
  wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask |
                  ButtonPressMask | PointerMotionMask | EnterWindowMask |
                  LeaveWindowMask | StructureNotifyMask | PropertyChangeMask;
  XChangeWindowAttributes(dpy, root, CWEventMask | CWCursor, &wa);

  // 引入xinput2扩展的原始输入事件监测
  /* 检查XInput2扩展的可用性 */
  int event, error;
  if (!XQueryExtension(dpy, "XInputExtension", &xi_opcode, &event, &error)) {
    printf("XInput扩展不可用。\n");
    return;
  }

  /* 检查XInput2扩展的版本 */
  int major = 2, minor = 0;
  if (XIQueryVersion(dpy, &major, &minor) == BadRequest) {
    printf("不支持XInput2扩展的版本%d.%d。\n", major, minor);
    return;
  }

  // xlib非扩展的窗口事件
  XSelectInput(dpy, root, wa.event_mask);

  /* 设置掩码以接收所有主设备的事件 */
  unsigned char mask_bytes[XIMaskLen(XI_LASTEVENT)];
  memset(mask_bytes, 0, sizeof(mask_bytes));
  XISetMask(mask_bytes, XI_RawMotion); // 鼠标移动事件捕获
  XISetMask(mask_bytes, XI_RawButtonPress); // 鼠标按键事件捕获

  XIEventMask evmasks[1];
  evmasks[0].deviceid = XIAllMasterDevices;
  evmasks[0].mask_len = sizeof(mask_bytes);
  evmasks[0].mask = mask_bytes;

  XISelectEvents(dpy, root, evmasks, 1);

  grabkeys();
  focus(NULL);
}

void seturgent(Client *c, int urg) {
  XWMHints *wmh;

  c->isurgent = urg;
  if (!(wmh = XGetWMHints(dpy, c->win)))
    return;
  wmh->flags = urg ? (wmh->flags | XUrgencyHint) : (wmh->flags & ~XUrgencyHint);
  XSetWMHints(dpy, c->win, wmh);
  XFree(wmh);
}

void show(Client *c) {
  if (!c || !HIDDEN(c))
    return;

  XMapWindow(dpy, c->win);
  setclientstate(c, NormalState);
  hiddenWinStackTop--;
  arrange(c->mon);
}

/*
该方法为显示当前tag下的窗口的func，切换时会将原窗口下的win放到屏幕之外
窗口所属的tag在当前监视器tag的右边就扔到右边隐藏,窗口所属的tag在当前监视器tag的左边就扔到左边隐藏
*/
void showtag(Client *c) {
  if (!c)
    return;
  if (ISVISIBLE(c)) {

    /** 将可见的client从屏幕边缘移动到屏幕内 */
    XMoveWindow(dpy, c->win, c->x, c->y);
    // if (c->isfloating && !c->isfullscreen)
    //   resize(c, c->x, c->y, c->w, c->h, 0);
    showtag(c->snext);
  } else {
    /* 将不可见的client移动到屏幕之外 */

    showtag(c->snext);

    // 判断tags是不是2的次方根,从而判断窗口是不是只是属于一个tag
    unsigned int c_is_one_tag = 1;
    if (c->tags & (c->tags - 1)) { // 去掉一个1，判断是否为0
      c_is_one_tag = 0;
    }
    // 通过get_tag_bit_position 解析tags,判断要移动的窗口属于哪个tag
    // 如果要移动的窗口只属于一个tag而且他在当前监视器所在tag的右边,就往右边隐藏
    if (c_is_one_tag == 1 &&
        get_tag_bit_position(c->tags) > c->mon->pertag->curtag) {
      XMoveWindow(dpy, c->win, WIDTH(c) * 10, c->y);
    } else {
      XMoveWindow(dpy, c->win, WIDTH(c) * -10, c->y);
    }
  }
}

void sigchld(int unused) {
  if (signal(SIGCHLD, sigchld) == SIG_ERR)
    die("can't install SIGCHLD handler:");
  while (0 < waitpid(-1, NULL, WNOHANG))
    ;
}

void spawn(const Arg *arg) {
  if (fork() == 0) {
    if (dpy)
      close(ConnectionNumber(dpy));
    setsid();
    execvp(((char **)arg->v)[0], (char **)arg->v);
    fprintf(stderr, "dwm: execvp %s", ((char **)arg->v)[0]);
    perror(" failed");
    exit(EXIT_SUCCESS);
  }
}

void tag_client(const Arg *arg,Client *target_client) {
  if (target_client && !target_client->isglobal && arg->ui & TAGMASK) {
    target_client->tags = arg->ui & TAGMASK;

    Client **tc;
    for (tc = &target_client; *tc; tc = &(*tc)->next){
      // 让目标tag中的全屏窗口退出全屏参与平铺
      if(!target_client->isfloating && (*tc) != target_client && (*tc) && (*tc)->isfullscreen  && (*tc)->tags & target_client->tags){
        clear_fullscreen_flag(*tc);
      }
    }
    focus(NULL);
    arrange(selmon);
    view(&(Arg){.ui = arg->ui});
  } else{
    view(arg);
  }
  //如果是浮动的移动过去就让他置于顶层
  if(target_client && target_client->isfloating){
    XRaiseWindow(dpy, target_client->win);
    focus(target_client);
  }else {
    focus(target_client);
  }
}

void tag(const Arg *arg) {
  if(selmon->sel)
    tag_client(arg,selmon->sel);
}

void tagmon(const Arg *arg) {
  if (!selmon->sel || !mons->next)
    return;
  sendmon(selmon->sel, dirtomon(arg->i));
  focusmon(&(Arg){.i = +1});
  if (selmon->sel && selmon->sel->isfloating) {
    resize(selmon->sel, selmon->mx + (selmon->mw - selmon->sel->w) / 2,
           selmon->my + (selmon->mh - selmon->sel->h) / 2, selmon->sel->w,
           selmon->sel->h, 0);
  }
  pointerfocuswin(selmon->sel);
}

void togglesystray() {
  if (showsystray) {
    showsystray = 0;
    XUnmapWindow(dpy, systray->win);
  } else {
    showsystray = 1;
  }
  updatesystray();
  updatestatus();
}

void togglebar(const Arg *arg) {
  Client *fc;
  selmon->showbar = selmon->pertag->showbars[selmon->pertag->curtag] =
      !selmon->showbar;
  updatebarpos(selmon);
  resizebarwin(selmon);
  if (showsystray) {
    XWindowChanges wc;
    if (!selmon->showbar)
      wc.y = -bh;
    else if (selmon->showbar) {
      wc.y = 0;
      if (!selmon->topbar)
        wc.y = selmon->mh - bh;
    }
    XConfigureWindow(dpy, systray->win, CWY, &wc);
  }

  // 在bar栏改动后让当前tag的假全屏窗口重新适应屏幕窗口大小
  for (fc = selmon->clients; fc; fc = fc->next) {
    if (fc->isfullscreen && fc->bw != 0) {
      resizeclient(fc, fc->mon->wx + gappo, fc->mon->wy + gappo,
                   fc->mon->ww - (gappo * 2) - gappo,
                   fc->mon->wh - (gappo * 2) - gappo);
      XRaiseWindow(dpy, fc->win); // 提升窗口到顶层
    }
  }
  // 非悬浮和全屏窗口也重新调整大小
  arrange(selmon);
  updatesystray();
}

void togglefloating(const Arg *arg) {
  if (!selmon->sel)
    return;
  if (selmon->sel->isfullscreen) {
    fullscreen(NULL);
    if (selmon->sel->isfloating)
      return;
  }

  selmon->sel->isfloating = !selmon->sel->isfloating || selmon->sel->isfixed;

  if (selmon->sel->isfloating) {
    selmon->sel->x = selmon->wx + selmon->ww / 6,
    selmon->sel->y = selmon->wy + selmon->wh / 6, managefloating(selmon->sel);
    resize(selmon->sel, selmon->sel->x, selmon->sel->y, selmon->ww / 3 * 2,
           selmon->wh / 3 * 2, 0);
  } else {
		selmon->sel->is_scratchpad_show = 0;
		selmon->sel->is_in_scratchpad = 0;
		selmon->sel->scratchpad_priority = 0;    
  }

  uint border_type = get_border_type(selmon->sel);
  XSetWindowBorder(dpy, selmon->sel->win, scheme[border_type][ColBorder].pixel);
  arrange(selmon);
  pointerfocuswin(selmon->sel);
}

void toggleallfloating(const Arg *arg) {
  Client *c = NULL;
  int somefloating = 0;

  if (!selmon->sel || selmon->sel->isfullscreen)
    return;

  for (c = selmon->clients; c; c = c->next)
    if (ISVISIBLE(c) && !HIDDEN(c) && c->isfloating) {
      somefloating = 1;
      break;
    }

  if (somefloating) {
    for (c = selmon->clients; c; c = c->next)
      if (ISVISIBLE(c) && !HIDDEN(c))
        c->isfloating = 0;
    arrange(selmon);
  } else {
    for (c = selmon->clients; c; c = c->next)
      if (ISVISIBLE(c) && !HIDDEN(c)) {
        c->isfloating = 1;
        resize(c, c->x + 2 * snap, c->y + 2 * snap, MAX(c->w - 4 * snap, snap),
               MAX(c->h - 4 * snap, snap), 0);
      }
  }
  pointerfocuswin(selmon->sel);
}


void show_scratchpad(Client *c) {
	c->is_scratchpad_show = 1;
  c->tags = selmon->tagset[selmon->seltags];
  show(c);
  focus(c);
  restack(selmon);
  clear_fullscreen_flag(c);
  c->isfloating = 1;
  c->w = selmon->ww * 0.5;
  c->h = selmon->wh * 0.8;
  c->x = selmon->wx + (selmon->ww - c->w) / 2;
  c->y = selmon->wy + (selmon->wh - c->h) / 2;
  resizeclient(c, c->x, c->y, c->w,c->h);
  arrange(selmon);
  pointerfocuswin(c);
}

void toggle_scratchpad(const Arg *arg) {
	Client *c,*target_show_client=NULL;
	Client *tempClients[100],*tempbackup; //只支持100个在便利签中的客户端
	int  z,j,k,i = 0;
	int is_hide_anction = 0;
	for (c = selmon->clients; c; c = c->next) {
		if(c->is_in_scratchpad && c->is_scratchpad_show && (selmon->tagset[selmon->seltags] & c->tags) == 0 ) {
			unsigned int target = get_tags_first_tag(selmon->tagset[selmon->seltags]); 
			tag_client(&(Arg){.ui = target},c);
			return;
		} else if (c->is_in_scratchpad && c->is_scratchpad_show && (selmon->tagset[selmon->seltags] & c->tags) != 0) {
			c->is_scratchpad_show = 0;
			c->scratchpad_priority = c->scratchpad_priority + 10;
			hide(c);
			is_hide_anction = 1;
		} else if (c->is_in_scratchpad && !c->is_scratchpad_show) {
			if (target_show_client != NULL) {
				if (c->scratchpad_priority < target_show_client->scratchpad_priority)
					target_show_client = c;
			} else {
				target_show_client = c;
			}
		} 

		if(c->is_in_scratchpad) {
			tempClients[i] = c;
			i++;
		}
	}

	if(is_hide_anction) {
		for(j=0;j<i-1;j++) //把优先级从左到右排序
			for(k=j+1;k<i;k++) {
				if(tempClients[j]->scratchpad_priority > tempClients[k]->scratchpad_priority) {
					tempbackup = tempClients[j];
					tempClients[j] = tempClients[k];
					tempClients[k] = tempbackup;
				}
			}
		for(z=0;z<i;z++) { //重新根据排序设置优先级的值,以免上面优先级做加法过多溢出
			tempClients[z]->scratchpad_priority = z;
		}
	} else {
		if(!target_show_client)
			return;
		show_scratchpad(target_show_client);
	}

}

void restorewin(const Arg *arg) {
  int i = hiddenWinStackTop;
  while (i > -1) {
    if (HIDDEN(hiddenWinStack[i]) && ISVISIBLE(hiddenWinStack[i])) {
      show(hiddenWinStack[i]);
      focus(hiddenWinStack[i]);
			hiddenWinStack[i]->is_scratchpad_show = 0;
			hiddenWinStack[i]->is_in_scratchpad = 0;
			hiddenWinStack[i]->scratchpad_priority = 0;
      restack(selmon);
      // need set j<hiddenWinStackTop+1. Because show will reduce
      // hiddenWinStackTop value.
      for (int j = i; j < hiddenWinStackTop + 1; ++j) {
        hiddenWinStack[j] = hiddenWinStack[j + 1];
      }
      return;
    }
    --i;
  }
}

void hidewin(const Arg *arg) {
  if (!selmon->sel)
    return;
  Client *c = (Client *)selmon->sel;
	c->is_in_scratchpad = 1;
	c->is_scratchpad_show = 0;
  hide(c);
}

int issinglewin(const Arg *arg) {
  Client *c = NULL;
  int cot = 0;

  for (c = selmon->clients; c; c = c->next) {
    if (ISVISIBLE(c) && !HIDDEN(c))
      cot++;
    if (cot > 1)
      return 0;
  }
  return 1;
}

void togglewin(const Arg *arg) {
  Client *c = (Client *)arg->v;
  if (c == selmon->sel)
    hide(c);
  else {
    if (HIDDEN(c))
      show(c);
    focus(c);
    restack(selmon);
  }
}

void toggleview(const Arg *arg) {
  unsigned int newtagset =
      selmon->tagset[selmon->seltags] ^ (arg->ui & TAGMASK);

  if (newtagset) {
    selmon->tagset[selmon->seltags] = newtagset;
    focus(NULL);
    arrange(selmon);
  }
}

void toggleglobal(const Arg *arg) {
  if (!selmon->sel)
    return;
  if (selmon->sel->isscratchpad) // is scratchpad always global
    return;
  selmon->sel->isglobal ^= 1;
  selmon->sel->tags =
      selmon->sel->isglobal ? TAGMASK : selmon->tagset[selmon->seltags];
  focus(NULL);
}

void toggleborder(const Arg *arg) {
  if (!selmon->sel)
    return;
  selmon->sel->isnoborder ^= 1;
  selmon->sel->bw = selmon->sel->isnoborder ? 0 : borderpx;
  int diff = (selmon->sel->isnoborder ? -1 : 1) * borderpx;
  // TODO: 当有动画效果时 会有闪烁问题
  resizeclient(selmon->sel, selmon->sel->x - diff, selmon->sel->y - diff,
               selmon->sel->w, selmon->sel->h);
  focus(NULL);
}

void unfocus(Client *c, int setfocus) {
  if (!c)
    return;
  c->isactive = 0; // 取消已经在窗口设置过聚焦的标志
  grabbuttons(c, 0);
  XSetWindowBorder(dpy, c->win, scheme[SchemeNorm][ColBorder].pixel);
  if (setfocus) {
    XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
    XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
  }
}

void unmanage(Client *c, int destroyed) {
  Monitor *m = c->mon;
  XWindowChanges wc;

  // 从窗口栈中移除
  detach(c);
  detachstack(c);

  if (!destroyed) {
    wc.border_width = c->oldbw;
    XGrabServer(dpy); /* avoid race conditions */
    XSetErrorHandler(xerrordummy);
    XConfigureWindow(dpy, c->win, CWBorderWidth, &wc); /* restore border */
    XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
    setclientstate(c, WithdrawnState);
    XSync(dpy, False);
    XSetErrorHandler(xerror);
    XUngrabServer(dpy);
  }

  // 取消窗口的相关事件监听
  XSelectInput(dpy, c->win, NoEventMask);
  // 释放窗口资源
  free(c);
  focus(NULL);
  updateclientlist();
  arrange(m);
  if(selmon->clients == NULL){
		if(selmon->isoverview){
			Arg arg = {0};
			toggleoverview(&arg);
		}	    
  }
}

void unmapnotify(XEvent *e) {
  Client *c;
  XUnmapEvent *ev = &e->xunmap;
  XLowerWindow(dpy,systray->win);
  XLowerWindow(dpy,selmon->barwin);
  if ((c = wintoclient(ev->window))) {
    if (ev->send_event)
      setclientstate(c, WithdrawnState);
    else
      unmanage(c, 0);
  } else if ((c = wintosystrayicon(ev->window))) {
    /* KLUDGE! sometimes icons occasionally unmap their windows, but do
     * _not_ destroy them. We map those windows back */
    XMapRaised(dpy, c->win);
    updatesystray();
  }
}

void updatebars(void) {
  unsigned int w;
  Monitor *m;
  Atom window_type,dock_type;
  XSetWindowAttributes wa = {.override_redirect = True,
                             .background_pixel = 0,
                             .border_pixel = 0,
                             .colormap = cmap,
                             .event_mask = ButtonPressMask | ExposureMask};
  XClassHint ch = {"dwm", "dwm"};
  for (m = mons; m; m = m->next) {
    if (m->barwin)
      continue;
    w = m->ww;
    if (showsystray && m == systraytomon(m))
      w -= getsystraywidth();
    m->barwin = XCreateWindow(dpy, root, m->wx, m->by, m->ww, bh, 0, depth,
                              InputOutput, visual,
                              CWOverrideRedirect | CWBackPixel | CWBorderPixel |
                                  CWColormap | CWEventMask,
                              &wa);
    XDefineCursor(dpy, m->barwin, cursor[CurNormal]->cursor);
    if (showsystray && m == systraytomon(m))
      XMapRaised(dpy, systray->win);
    XMapRaised(dpy, m->barwin);
    XSetClassHint(dpy, m->barwin, &ch);

    window_type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False); // 获取窗口类型的原子
    dock_type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False); // 获取dock类型的原子
    XChangeProperty(dpy, m->barwin, window_type, XA_ATOM, 32, PropModeReplace, (unsigned char *)&dock_type, 1); // 将窗口类型设置为dock
  }
}

void updatebarpos(Monitor *m) {
  m->wy = m->my;
  m->wh = m->mh;
  if (m->showbar) {
    m->wh = m->wh - vertpad - bh;
    m->by = m->topbar ? m->wy : m->wy + m->wh + vertpad;
    m->wy = m->topbar ? m->wy + bh + vp : m->wy;
  } else
    m->by = -bh - vp;
}

void updateclientlist() {
  Client *c;
  Monitor *m;

  XDeleteProperty(dpy, root, netatom[NetClientList]);
  for (m = mons; m; m = m->next)
    for (c = m->clients; c; c = c->next)
      XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32,
                      PropModeAppend, (unsigned char *)&(c->win), 1);
}

int updategeom(void) {
  int dirty = 0;

#ifdef XINERAMA
  if (XineramaIsActive(dpy)) {
    int i, j, n, nn;
    Client *c;
    Monitor *m;
    XineramaScreenInfo *info = XineramaQueryScreens(dpy, &nn);
    XineramaScreenInfo *unique = NULL;

    for (n = 0, m = mons; m; m = m->next, n++)
      ;
    /* only consider unique geometries as separate screens */
    unique = ecalloc(nn, sizeof(XineramaScreenInfo));
    for (i = 0, j = 0; i < nn; i++)
      if (isuniquegeom(unique, j, &info[i]))
        memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
    XFree(info);
    nn = j;
    if (n <= nn) { /* new monitors available */
      for (i = 0; i < (nn - n); i++) {
        for (m = mons; m && m->next; m = m->next)
          ;
        if (m)
          m->next = createmon();
        else
          mons = createmon();
      }
      for (i = 0, m = mons; i < nn && m; m = m->next, i++)
        if (i >= n || unique[i].x_org != m->mx || unique[i].y_org != m->my ||
            unique[i].width != m->mw || unique[i].height != m->mh) {
          dirty = 1;
          m->num = i;
          m->mx = m->wx = unique[i].x_org;
          m->my = m->wy = unique[i].y_org;
          m->mw = m->ww = unique[i].width;
          m->mh = m->wh = unique[i].height;
          updatebarpos(m);
        }
    } else { /* less monitors available nn < n */
      for (i = nn; i < n; i++) {
        for (m = mons; m && m->next; m = m->next)
          ;
        while ((c = m->clients)) {
          dirty = 1;
          m->clients = c->next;
          detachstack(c);
          c->mon = mons;
          attach(c);
          attachstack(c);
        }
        if (m == selmon)
          selmon = mons;
        cleanupmon(m);
      }
    }
    free(unique);
  } else
#endif /* XINERAMA */
  {    /* default monitor setup */
    if (!mons)
      mons = createmon();
    if (mons->mw != sw || mons->mh != sh) {
      dirty = 1;
      mons->mw = mons->ww = sw;
      mons->mh = mons->wh = sh;
      updatebarpos(mons);
    }
  }
  if (dirty) {
    selmon = mons;
    selmon = wintomon(root);
  }
  return dirty;
}

void updatenumlockmask(void) {
  unsigned int i, j;
  XModifierKeymap *modmap;

  numlockmask = 0;
  modmap = XGetModifierMapping(dpy);
  for (i = 0; i < 8; i++)
    for (j = 0; j < modmap->max_keypermod; j++)
      if (modmap->modifiermap[i * modmap->max_keypermod + j] ==
          XKeysymToKeycode(dpy, XK_Num_Lock))
        numlockmask = (1 << i);
  XFreeModifiermap(modmap);
}

void updatesizehints(Client *c) {
  long msize;
  XSizeHints size;

  if (!XGetWMNormalHints(dpy, c->win, &size, &msize))
    /* size is uninitialized, ensure that size.flags aren't used */
    size.flags = PSize;
  if (size.flags & PBaseSize) {
    c->basew = size.base_width;
    c->baseh = size.base_height;
  } else if (size.flags & PMinSize) {
    c->basew = size.min_width;
    c->baseh = size.min_height;
  } else
    c->basew = c->baseh = 0;
  if (size.flags & PResizeInc) {
    c->incw = size.width_inc;
    c->inch = size.height_inc;
  } else
    c->incw = c->inch = 0;
  if (size.flags & PMaxSize) {
    c->maxw = size.max_width;
    c->maxh = size.max_height;
  } else
    c->maxw = c->maxh = 0;
  if (size.flags & PMinSize) {
    c->minw = size.min_width;
    c->minh = size.min_height;
  } else if (size.flags & PBaseSize) {
    c->minw = size.base_width;
    c->minh = size.base_height;
  } else
    c->minw = c->minh = 0;
  if (size.flags & PAspect) {
    c->mina = (float)size.min_aspect.y / size.min_aspect.x;
    c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
  } else
    c->maxa = c->mina = 0.0;
  c->isfixed = (c->maxw && c->maxh && c->maxw == c->minw && c->maxh == c->minh);
}

void updatestatus(void) {
  Monitor *m;
  if (!gettextprop(root, XA_WM_NAME, stext, sizeof(stext)))
    strcpy(stext, "^c#2D1B46^^b#335566^:) ^d^"); // 默认的状态栏文本
  for (m = mons; m; m = m->next)
    drawbar(m);
  updatesystray();
}

void updatesystrayicongeom(Client *i, int w, int h) {
  if (i) {
    i->h = bh;
    if (w == h)
      i->w = bh;
    else if (h == bh)
      i->w = w;
    else
      i->w = (int)((float)bh * ((float)w / (float)h));
    applysizehints(i, &(i->x), &(i->y), &(i->w), &(i->h), False);
    /* force icons into the systray dimenons if they don't want to */
    if (i->h > bh) {
      if (i->w == i->h)
        i->w = bh;
      else
        i->w = (int)((float)bh * ((float)i->w / (float)i->h));
      i->h = bh;
    }
  }
}

void updatesystrayiconstate(Client *i, XPropertyEvent *ev) {
  long flags;
  int code = 0;

  if (!showsystray || !i || ev->atom != xatom[XembedInfo] ||
      !(flags = getatomprop(i, xatom[XembedInfo])))
    return;

  if (flags & XEMBED_MAPPED && !i->tags) {
    i->tags = 1;
    code = XEMBED_WINDOW_ACTIVATE;
    XMapRaised(dpy, i->win);
    setclientstate(i, NormalState);
  } else if (!(flags & XEMBED_MAPPED) && i->tags) {
    i->tags = 0;
    code = XEMBED_WINDOW_DEACTIVATE;
    XUnmapWindow(dpy, i->win);
    setclientstate(i, WithdrawnState);
  } else
    return;
  sendevent(i->win, xatom[Xembed], StructureNotifyMask, CurrentTime, code, 0,
            systray->win, XEMBED_EMBEDDED_VERSION);
}

void updatesystray(void) {
  XSetWindowAttributes wa;
  XWindowChanges wc;
  Client *i;
  Monitor *m = systraytomon(NULL);
  unsigned int x = m->mx + m->mw;
  unsigned int w = 1;

  if (!showsystray)
    return;
  if (!systray) {
    /* init systray */
    if (!(systray = (Systray *)calloc(1, sizeof(Systray))))
      die("fatal: could not malloc() %u bytes\n", sizeof(Systray));
    systray->win = XCreateSimpleWindow(dpy, root, x - sp, m->by + vp, w, bh, 0,
                                       0, scheme[SchemeSystray][ColBg].pixel);
    wa.event_mask = ButtonPressMask | ExposureMask;
    wa.override_redirect = True;
    wa.background_pixel = scheme[SchemeSystray][ColBg].pixel;
    XSelectInput(dpy, systray->win, SubstructureNotifyMask);
    XChangeProperty(dpy, systray->win, netatom[NetSystemTrayOrientation],
                    XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)&netatom[NetSystemTrayOrientationHorz], 1);
    XChangeWindowAttributes(
        dpy, systray->win, CWEventMask | CWOverrideRedirect | CWBackPixel, &wa);
    XMapRaised(dpy, systray->win);
    XSetSelectionOwner(dpy, netatom[NetSystemTray], systray->win, CurrentTime);
    if (XGetSelectionOwner(dpy, netatom[NetSystemTray]) == systray->win) {
      sendevent(root, xatom[Manager], StructureNotifyMask, CurrentTime,
                netatom[NetSystemTray], systray->win, 0, 0);
      XSync(dpy, False);
    } else {
      fprintf(stderr, "dwm: unable to obtain system tray.\n");
      free(systray);
      systray = NULL;
      return;
    }
  }
  for (w = 0, i = systray->icons; i; i = i->next) {
    /* make sure the background color stays the same */
    wa.background_pixel = scheme[SchemeSystray][ColBg].pixel;
    XChangeWindowAttributes(dpy, i->win, CWBackPixel, &wa);
    XMapRaised(dpy, i->win);
    w += systrayspacing;
    i->x = w;
    XMoveResizeWindow(dpy, i->win, i->x + 3, 0 + 3, MAX(i->w - 6, bh - 6),
                      bh - 6); // 限制过大的图标
    w += MAX(i->w, bh);
    if (i->mon != m)
      i->mon = m;
  }
  w = w ? w + systrayspacing : 1;
  x -= w;
  XMoveResizeWindow(dpy, systray->win, x - sp, m->by + vp, w, bh);
  wc.x = x - sp;
  wc.y = m->by + vp;
  wc.width = w;
  wc.height = bh;
  wc.stack_mode = Above;
  wc.sibling = m->barwin;
  XConfigureWindow(dpy, systray->win,
                   CWX | CWY | CWWidth | CWHeight | CWSibling | CWStackMode,
                   &wc);
  XMapWindow(dpy, systray->win);
  XMapSubwindows(dpy, systray->win);
  XSync(dpy, False);
}

void updateicon(Client *c) {
  int i;
  XClassHint ch = {NULL, NULL};
  XGetClassHint(dpy, c->win, &ch);
  if (ch.res_class) {
    for (i = 0; i < LENGTH(icons); i++) {
      if (!strcmp(ch.res_class, (&icons[i])->class)) {
        strcpy(c->icon, (&icons[i])->style);
        return;
      }
    }
  }
  strcpy(c->icon, default_icon);
  return;
}

void updatetitle(Client *c) {
  if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
    gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
  if (c->name[0] == '\0') /* hack to mark broken clients */
    strcpy(c->name, broken);
}

void updatewindowtype(Client *c) {
  Atom state = getatomprop(c, netatom[NetWMState]);
  Atom wtype = getatomprop(c, netatom[NetWMWindowType]);

  if (state == netatom[NetWMFullscreen])
    setfullscreen(c);
  if (wtype == netatom[NetWMWindowTypeDialog])
    c->isfloating = 1;
}




void updatewmhints(Client *c) {
  XWMHints *wmh;

  if ((wmh = XGetWMHints(dpy, c->win))) {
    if (c == selmon->sel && wmh->flags & XUrgencyHint) {
      wmh->flags &= ~XUrgencyHint;
      XSetWMHints(dpy, c->win, wmh);
    } else
      c->isurgent = (wmh->flags & XUrgencyHint) ? 1 : 0;
    if (wmh->flags & InputHint)
      c->neverfocus = !wmh->input;
    else
      c->neverfocus = 0;
    XFree(wmh);
  }
}

void setgap(const Arg *arg) {
  gappi = arg->i ? MAX(gappi + arg->i, 0) : _gappi;
  gappo = arg->i ? MAX(gappo + arg->i, 0) : _gappo;
  arrange(selmon);
}

void view(const Arg *arg) {
  int i;
  unsigned int tmptag;
  Client *c;
  int n = 0;

  selmon->seltags ^= 1; /* toggle sel tagset */
  if (arg->ui & TAGMASK) {
    selmon->tagset[selmon->seltags] = arg->ui & TAGMASK;
    selmon->pertag->prevtag = selmon->pertag->curtag;

    if (arg->ui == ~0)
      selmon->pertag->curtag = 0;
    else {
      for (i = 0; !(arg->ui & 1 << i); i++)
        ;
      selmon->pertag->curtag = i + 1;
    }
  } else {
    tmptag = selmon->pertag->prevtag;
    selmon->pertag->prevtag = selmon->pertag->curtag;
    selmon->pertag->curtag = tmptag;
  }

  selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag];
  selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag];
  selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag];
  selmon->lt[selmon->sellt] =
      selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt];
  selmon->lt[selmon->sellt ^ 1] =
      selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt ^ 1];

  if (selmon->showbar != selmon->pertag->showbars[selmon->pertag->curtag])
    togglebar(NULL);

  focus(NULL);
  arrange(selmon);

  // 若当前tag无窗口 且附加了v参数 则执行
  if (arg->v) {
    for (c = selmon->clients; c; c = c->next)
      if (c->tags & arg->ui && !HIDDEN(c) && !c->isglobal)
        n++;
    if (n == 0) {
      spawn(&(Arg){.v = (const char *[]){"/bin/sh", "-c", arg->v, NULL}});
    }
  }

  Client **tc;
  for (tc = &selmon->clients; *tc; tc = &(*tc)->next){
    // 目标窗口有全屏窗口的话把它提升到最高层
    if((*tc) && (*tc)->isfullscreen){
      XRaiseWindow(dpy, (*tc)->win);
      focus(*tc);;
    }
  }

}

/* overview 模式左键跳转窗口 */
void inner_overvew_toggleoverview(const Arg *arg) {
  if (selmon->isoverview) {
    toggleoverview(arg);
  }
}

/* overview 模式右键关闭窗口 */
void inner_overvew_killclient(const Arg *arg) {
  if (selmon->isoverview) {
    killclient(arg);
  }
}

// 显示所有tag 或 跳转到聚焦窗口的tag
void toggleoverview(const Arg *arg) {

  uint target = selmon->sel && selmon->sel->tags != TAGMASK ? selmon->sel->tags
                                                            : selmon->seltags;
  selmon->isoverview ^= 1;
  Client *c;
  Client *target_client =selmon->sel ;
  // 正常视图到overview,退出所有窗口的浮动和全屏状态参与平铺,
  // overview到正常视图,还原之前退出的浮动和全屏窗口状态
  if (selmon->isoverview) {
    for (c = selmon->clients; c; c = c->next) {
      overview_backup(c);
    }
  } else {
    for (c = selmon->clients; c; c = c->next) {
      overview_restore(c, &(Arg){.ui = target});
    }
    //退出overview自动全屏
    if(auto_fullscreen) {
      set_fake_fullscreen(target_client);
    }
  }

  view(&(Arg){.ui = target});

  if(target_client){
    XRaiseWindow(dpy,target_client->win);
    focus(target_client);
  }
}

void viewtoleft(const Arg *arg) {
  unsigned int target = selmon->tagset[selmon->seltags], pre;
  Client *c;
  unsigned int circle = 1;
  unsigned max_target = 1;
  unsigned tags_num = TAGMASK;

  while (1) {
    pre = target;
    target >>= 1;
    if (target == pre) {
      if (circle == 0 || tag_circle == 0) {
        return;
      }
      for (tags_num >>= 1; tags_num != 0; tags_num >>= 1) { // 计算最大的tag的值
        max_target <<= 1;
      }
      target = max_target; // 向左边尽头找不到有窗口的tag,重新从最右边开始找
      circle = 0; // 只重新找一次
    }
    for (c = selmon->clients; c; c = c->next) {
      if (c->isglobal && c->tags == TAGMASK)
        continue;
      if (c->tags & target &&
          __builtin_popcount(selmon->tagset[selmon->seltags] & TAGMASK) == 1 &&
          (selmon->tagset[selmon->seltags] > 1 || tag_circle == 1)) {
        view(&(Arg){.ui = target});
        return;
      }
    }
  }
}

void addtoleft(const Arg *arg) {
  unsigned int target = selmon->tagset[selmon->seltags];
  unsigned int tag_len = LENGTH(tags);
  target >>= 1;
  if (target == 0 && tag_circle){ //如果开启了循环到最左边的下一个是右边最后一个tag
    target = (1 << (tag_len - 1));
  } else if(target == 0 && !tag_circle) {
    return;
  }
  view(&(Arg){.ui = target});
  return;
}

void viewtoright(const Arg *arg) {
  unsigned int target = selmon->tagset[selmon->seltags];
  Client *c;
  unsigned int circle = 1;
  while (1) {
    target = target == 0 ? 1 : target << 1;
    if (!(target & TAGMASK)) {
      if (circle == 0 || tag_circle == 0) {
        return;
      }
      target = 1; // 向右边尽头找不到有窗口的tag,重新从最左边开始找
      circle = 0; // 只重新找一次
    }
    for (c = selmon->clients; c; c = c->next) {
      if (c->isglobal && c->tags == TAGMASK)
        continue;
      if (c->tags & target &&
          __builtin_popcount(selmon->tagset[selmon->seltags] & TAGMASK) == 1 &&
          (selmon->tagset[selmon->seltags] & (TAGMASK >> 1) || tag_circle)) {
        view(&(Arg){.ui = target});
        return;
      }
    }
  }
}

void addtoright(const Arg *arg) {
  unsigned int target = selmon->tagset[selmon->seltags];
  target <<= 1;
  if (!(target & TAGMASK) && tag_circle){ //如果开启了循环到最右边边的下一个是左边第一个tag
    target = 1;
  } else if(!(target & TAGMASK) && !tag_circle) {
    return;
  }
  view(&(Arg){.ui = target});
  return;
}

/*清除全屏标志,还原全屏时清0的border*/
void clear_fullscreen_flag(Client *c) {
  if (c->isfullscreen) {
    c->isfullscreen = 0;
    c->bw = borderpx; // 恢复非全屏的border
    XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)0,
                    0); // 清除窗口在x11服务中标记的全屏属性
  }
}

unsigned int want_restore_fullscreen(Client *target_client) {
  Client *c = NULL;
  for (c = nexttiled(target_client->mon->clients); c; c = nexttiled(c->next)) {
    if (c && c != target_client && c->tags == target_client->tags && c == selmon->sel) {
      return 0;
    }
  }
  return 1;
}

// 普通视图切换到overview时保存窗口的旧状态
void overview_backup(Client *c) {
  c->overview_isfloatingbak = c->isfloating;
  c->overview_isfullscreenbak = c->isfullscreen;
  c->overview_backup_x = c->x;
  c->overview_backup_y = c->y;
  c->overview_backup_w = c->w;
  c->overview_backup_h = c->h;
  c->overview_backup_bw = c->bw;
  if (c->isfloating) {
    c->isfloating = 0;
  }
  if (c->isfullscreen) {
    if (c->bw == 0) { // 真全屏窗口清除x11全屏属性
      XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
                      PropModeReplace, (unsigned char *)0, 0);
    }
    c->isfullscreen = 0; // 清除窗口全屏标志
  }
  c->bw = borderpx; // 恢复非全屏的border
}

// overview切回到普通视图还原窗口的状态
void overview_restore(Client *c, const Arg *arg) {
  c->isfloating = c->overview_isfloatingbak;
  c->isfullscreen = c->overview_isfullscreenbak;
  c->overview_isfloatingbak = 0;
  c->overview_isfullscreenbak = 0;
  c->bw = c->overview_backup_bw;
  if (c->isfloating) {
    XRaiseWindow(dpy, c->win); // 提升悬浮窗口到顶层
    resize(c, c->overview_backup_x, c->overview_backup_y, c->overview_backup_w,
           c->overview_backup_h, 0);
  }
  if (c->isfullscreen) {
    
    if(!want_restore_fullscreen(c)) {
       c->isfullscreen = c->overview_isfullscreenbak = 0;
      return;
    }

    if (c->overview_backup_bw == 0) { // 真全屏窗口设置x11全屏属性
      XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
                      PropModeReplace,
                      (unsigned char *)&netatom[NetWMFullscreen],
                      1); // 设置窗口在x11状态
    }
    resizeclient(c, c->overview_backup_x, c->overview_backup_y,
                 c->overview_backup_w, c->overview_backup_h);
  }
}

// 栈底部入栈布局位置大小计算
void tile(Monitor *m) {
  newclientathead = 1; // 头部入栈
  unsigned int i, n, mw, mh, sh, my,
      sy; // mw: master的宽度, mh: master的高度, sh: stack的高度, my:
          // master的y坐标, sy: stack的y坐标
  Client *c;

  for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++)
    ;
  if (n == 0)
    return;

  if (n > m->nmaster)
    mw = m->nmaster ? (m->ww + gappi) * m->mfact : 0;
  else
    mw = m->ww - 2 * gappo + gappi;

  mh = m->nmaster == 0 ? 0
                       : (m->wh - 2 * gappo - gappi * (m->nmaster - 1)) /
                             m->nmaster; // 单个master的高度
  sh = n == m->nmaster ? 0
                       : (m->wh - 2 * gappo - gappi * (n - m->nmaster - 1)) /
                             (n - m->nmaster); // 单个stack的高度

  for (i = 0, my = sy = gappo, c = nexttiled(m->clients); c;
       c = nexttiled(c->next), i++) {

    if (i < m->nmaster) {
      resize(c, m->wx + gappo, m->wy + my, mw - 2 * c->bw - gappi,
             mh - 2 * c->bw, 0);
      my += HEIGHT(c) + gappi;
    } else {
      resize(c, m->wx + mw + gappo, m->wy + sy,
             m->ww - mw - 2 * c->bw - 2 * gappo, sh - 2 * c->bw, 0);
      sy += HEIGHT(c) + gappi;
    }
  }
}

// 栈头部入栈布局位置大小计算
void rtile(Monitor *m) {
  newclientathead = 0; // 尾部入栈
  unsigned int i, n, mw, mh, sh, my,
      sy; // mw: master的宽度, mh: master的高度, sh: stack的高度, my:
          // master的y坐标, sy: stack的y坐标
  Client *c;

  for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++)
    ;
  if (n == 0)
    return;

  if (n > m->nmaster)
    mw = m->nmaster ? (m->ww + gappi) * m->mfact : 0;
  else
    mw = m->ww - 2 * gappo + gappi;

  mh = m->nmaster == 0 ? 0
                       : (m->wh - 2 * gappo - gappi * (m->nmaster - 1)) /
                             m->nmaster; // 单个master的高度
  sh = n == m->nmaster ? 0
                       : (m->wh - 2 * gappo - gappi * (n - m->nmaster - 1)) /
                             (n - m->nmaster); // 单个stack的高度

  for (i = 0, my = sy = gappo, c = nexttiled(m->clients); c;
       c = nexttiled(c->next), i++) {

    if (i < m->nmaster) {
      resize(c, m->wx + gappo, m->wy + my, mw - 2 * c->bw - gappi,
             mh - 2 * c->bw, 0);
      my += HEIGHT(c) + gappi;
    } else {
      resize(c, m->wx + mw + gappo, m->wy + sy,
             m->ww - mw - 2 * c->bw - 2 * gappo, sh - 2 * c->bw, 0);
      sy += HEIGHT(c) + gappi;
    }
  }
}

void magicgrid(Monitor *m) { grid(m, gappo, gappi); }

void overview(Monitor *m) { grid(m, overviewgappo, overviewgappi); }

// 网格布局窗口大小和位置计算
void grid(Monitor *m, uint gappo, uint gappi) {
  unsigned int i, n;
  unsigned int cx, cy, cw, ch;
  unsigned int dx;
  unsigned int cols, rows, overcols;
  Client *c;

  for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++)
    ;
  if (n == 0)
    return;
  if (n == 1) {
    c = nexttiled(m->clients);
    cw = (m->ww - 2 * gappo) * 0.7;
    ch = (m->wh - 2 * gappo) * 0.8;
    resize(c, m->wx + (m->mw - cw) / 2, m->wy + (m->wh - ch) / 2,
           cw - 2 * c->bw, ch - 2 * c->bw, 0);
    return;
  }
  if (n == 2) {
    c = nexttiled(m->clients);
    cw = (m->ww - 2 * gappo - gappi) / 2;
    ch = (m->wh - 2 * gappo) * 0.65;
    resize(c, m->mx + cw + gappo + gappi, m->my + (m->mh - ch) / 2 + gappo,
           cw - 2 * c->bw, ch - 2 * c->bw, 0);
    resize(nexttiled(c->next), m->mx + gappo, m->my + (m->mh - ch) / 2 + gappo,
           cw - 2 * c->bw, ch - 2 * c->bw, 0);

    return;
  }

  for (cols = 0; cols <= n / 2; cols++)
    if (cols * cols >= n)
      break;
  rows = (cols && (cols - 1) * cols >= n) ? cols - 1 : cols;
  ch = (m->wh - 2 * gappo - (rows - 1) * gappi) / rows;
  cw = (m->ww - 2 * gappo - (cols - 1) * gappi) / cols;

  overcols = n % cols;
  if (overcols)
    dx = (m->ww - overcols * cw - (overcols - 1) * gappi) / 2 - gappo;
  for (i = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++) {
    cx = m->wx + (i % cols) * (cw + gappi);
    cy = m->wy + (i / cols) * (ch + gappi);
    if (overcols && i >= n - overcols) {
      cx += dx;
    }
    resize(c, cx + gappo, cy + gappo, cw - 2 * c->bw, ch - 2 * c->bw, 0);
  }
}

Client *wintoclient(Window w) {
  Client *c;
  Monitor *m;

  for (m = mons; m; m = m->next)
    for (c = m->clients; c; c = c->next)
      if (c->win == w)
        return c;
  return NULL;
}

Client *wintosystrayicon(Window w) {
  Client *i = NULL;

  if (!showsystray || !w)
    return i;
  for (i = systray->icons; i && i->win != w; i = i->next)
    ;
  return i;
}

Monitor *wintomon(Window w) {
  int x, y;
  Client *c;
  Monitor *m;

  if (w == root && getrootptr(&x, &y))
    return recttomon(x, y, 1, 1);
  for (m = mons; m; m = m->next)
    if (w == m->barwin)
      return m;
  if ((c = wintoclient(w)))
    return c->mon;
  return selmon;
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's). Other types of errors call Xlibs
 * default error handler, which may call exit. */
int xerror(Display *dpy, XErrorEvent *ee) {
  if (ee->error_code == BadWindow ||
      (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch) ||
      (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable) ||
      (ee->request_code == X_PolyFillRectangle &&
       ee->error_code == BadDrawable) ||
      (ee->request_code == X_PolySegment && ee->error_code == BadDrawable) ||
      (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch) ||
      (ee->request_code == X_GrabButton && ee->error_code == BadAccess) ||
      (ee->request_code == X_GrabKey && ee->error_code == BadAccess) ||
      (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
    return 0;
  fprintf(stderr, "dwm: fatal error: request code=%d, error code=%d\n",
          ee->request_code, ee->error_code);
  return xerrorxlib(dpy, ee); /* may call exit */
}

int xerrordummy(Display *dpy, XErrorEvent *ee) { return 0; }

/* Startup Error handler to check if another window manager
 * is already running. */
int xerrorstart(Display *dpy, XErrorEvent *ee) {
  die("dwm: another window manager is already running");
  return -1;
}

Monitor *systraytomon(Monitor *m) {
  Monitor *t;
  int i, n;
  if (!systraypinning) {
    if (!m)
      return selmon;
    return m == selmon ? m : NULL;
  }
  for (n = 1, t = mons; t && t->next; n++, t = t->next)
    ;
  for (i = 1, t = mons; t && t->next && i < systraypinning; i++, t = t->next)
    ;
  if (n < systraypinning)
    return mons;
  return t;
}

void xinitvisual() {
  XVisualInfo *infos;
  XRenderPictFormat *fmt;
  int nitems;
  int i;

  XVisualInfo tpl = {.screen = screen, .depth = 32, .class = TrueColor};
  long masks = VisualScreenMask | VisualDepthMask | VisualClassMask;

  infos = XGetVisualInfo(dpy, masks, &tpl, &nitems);
  visual = NULL;
  for (i = 0; i < nitems; i++) {
    fmt = XRenderFindVisualFormat(dpy, infos[i].visual);
    if (fmt->type == PictTypeDirect && fmt->direct.alphaMask) {
      visual = infos[i].visual;
      depth = infos[i].depth;
      cmap = XCreateColormap(dpy, root, visual, AllocNone);
      useargb = 1;
      break;
    }
  }

  XFree(infos);

  if (!visual) {
    visual = DefaultVisual(dpy, screen);
    depth = DefaultDepth(dpy, screen);
    cmap = DefaultColormap(dpy, screen);
  }
}

void zoom(const Arg *arg) {
  Client *c = selmon->sel;

  if (c && (c->isfloating || c->isfullscreen))
    return;
  if (c == nexttiled(selmon->clients))
    if (!c || !(c = nexttiled(c->next)))
      return;
  pop(c);
}

Client *direction_select(const Arg *arg) {
  Client *tempClients[100];
  Monitor *m;
  Client *c = NULL, *tc = selmon->sel;
  int last = -1, issingle = issinglewin(NULL);
  // int cur = 0;
  if (tc &&
      tc->isfullscreen) /* no support for focusstack with fullscreen windows */
    return NULL;
  if (!tc)
    tc = selmon->clients;
  if (!tc)
    return NULL;

  for (c = selmon->clients; c; c = c->next) {
    if (ISVISIBLE(c) && (issingle || !HIDDEN(c))) {
      last++;
      tempClients[last] = c;
      // if (c == tc)
      // cur = last;
    }
  }

   for (m = mons; m; m = m->next) {
    if(selmon && m != selmon && m->sel) {
      last++;
      tempClients[last] = m->sel;      
    }
   }

  if (last < 0)
    return NULL;
  int sel_x = tc->x;
  int sel_y = tc->y;
  long long int distance = LLONG_MAX;
  // int temp_focus = 0;
  Client *tempFocusClients = NULL;

  switch (arg->i) {
  case UP:
    for (int _i = 0; _i <= last; _i++) {
      if (tempClients[_i]->y < sel_y && tempClients[_i]->x == sel_x) {
        int dis_x = tempClients[_i]->x - sel_x;
        int dis_y = tempClients[_i]->y - sel_y;
        long long int tmp_distance = dis_x * dis_x + dis_y * dis_y; // 计算距离
        if (tmp_distance < distance) {
          distance = tmp_distance;
          tempFocusClients = tempClients[_i];
        }
      }
    }
    if(!tempFocusClients){
      for (int _i = 0; _i <= last; _i++) {
        if (tempClients[_i]->y < sel_y) {
          int dis_x = tempClients[_i]->x - sel_x;
          int dis_y = tempClients[_i]->y - sel_y;
          long long int tmp_distance = dis_x * dis_x + dis_y * dis_y; // 计算距离
          if (tmp_distance < distance) {
            distance = tmp_distance;
            tempFocusClients = tempClients[_i];
          }
        }
      }
    }
    break;
  case DOWN:
    for (int _i = 0; _i <= last; _i++) {
      if (tempClients[_i]->y > sel_y && tempClients[_i]->x == sel_x) {
        int dis_x = tempClients[_i]->x - sel_x;
        int dis_y = tempClients[_i]->y - sel_y;
        long long int tmp_distance = dis_x * dis_x + dis_y * dis_y; // 计算距离
        if (tmp_distance < distance) {
          distance = tmp_distance;
          tempFocusClients = tempClients[_i];
        }
      }
    }
    if(!tempFocusClients){
      for (int _i = 0; _i <= last; _i++) {
        if (tempClients[_i]->y > sel_y ) {
          int dis_x = tempClients[_i]->x - sel_x;
          int dis_y = tempClients[_i]->y - sel_y;
          long long int tmp_distance = dis_x * dis_x + dis_y * dis_y; // 计算距离
          if (tmp_distance < distance) {
            distance = tmp_distance;
            tempFocusClients = tempClients[_i];
          }
        }
      }      
    }
    break;
  case LEFT:
    for (int _i = 0; _i <= last; _i++) {
      if (tempClients[_i]->x < sel_x && tempClients[_i]->y == sel_y) {
        int dis_x = tempClients[_i]->x - sel_x;
        int dis_y = tempClients[_i]->y - sel_y;
        long long int tmp_distance = dis_x * dis_x + dis_y * dis_y; // 计算距离
        if (tmp_distance < distance) {
          distance = tmp_distance;
          tempFocusClients = tempClients[_i];
        }
      }
    }
    if(!tempFocusClients){
      for (int _i = 0; _i <= last; _i++) {
        if (tempClients[_i]->x < sel_x) {
          int dis_x = tempClients[_i]->x - sel_x;
          int dis_y = tempClients[_i]->y - sel_y;
          long long int tmp_distance = dis_x * dis_x + dis_y * dis_y; // 计算距离
          if (tmp_distance < distance) {
            distance = tmp_distance;
            tempFocusClients = tempClients[_i];
          }
        }
      }      
    }
    break;
  case RIGHT:
    for (int _i = 0; _i <= last; _i++) {
      if (tempClients[_i]->x > sel_x && tempClients[_i]->y == sel_y) {
        int dis_x = tempClients[_i]->x - sel_x;
        int dis_y = tempClients[_i]->y - sel_y;
        long long int tmp_distance = dis_x * dis_x + dis_y * dis_y; // 计算距离
        if (tmp_distance < distance) {
          distance = tmp_distance;
          tempFocusClients = tempClients[_i];
        }
      }
    }
    if(!tempFocusClients){
      for (int _i = 0; _i <= last; _i++) {
        if (tempClients[_i]->x > sel_x) {
          int dis_x = tempClients[_i]->x - sel_x;
          int dis_y = tempClients[_i]->y - sel_y;
          long long int tmp_distance = dis_x * dis_x + dis_y * dis_y; // 计算距离
          if (tmp_distance < distance) {
            distance = tmp_distance;
            tempFocusClients = tempClients[_i];
          }
        }
      }      
    }
  }

  return tempFocusClients;
}

void focusdir(const Arg *arg) {
  Client *c = NULL;

  if (!selmon->sel) {
      focusmon(&(Arg){.i = +1});
      return;
  }

  c = direction_select(arg);

  if (c) {
    pointerfocuswin(c);
    restack(c->mon);
  }

}

void exchange_two_client(Client *c1, Client *c2) {
  if (c1 == NULL || c2 == NULL || c1->mon != c2->mon) {
    return;
  }

  // 先找c1的上一个节点
  Client head1;
  Client *headp1 = &head1;
  headp1->next = selmon->clients;
  Client *tmp1 = headp1;
  for (; tmp1 != NULL; tmp1 = tmp1->next) {
    if (tmp1->next != NULL) {
      if (tmp1->next == c1)
        break;
    } else {
      break;
    }
  }

  // 再找c2的上一个节点
  Client head2;
  Client *headp2 = &head2;
  headp2->next = selmon->clients;
  Client *tmp2 = headp2;
  for (; tmp2 != NULL; tmp2 = tmp2->next) {
    if (tmp2->next != NULL) {
      if (tmp2->next == c2)
        break;
    } else {
      break;
    }
  }

  if (tmp1 == NULL) { /* gDebug("tmp1==null"); */
    return;
  }
  if (tmp2 == NULL) { /*  gDebug("tmp2==null"); */
    return;
  }
  if (tmp1->next == NULL) { /*  gDebug("tmp1->next==null"); */
    return;
  }
  if (tmp2->next == NULL) { /* gDebug("tmp2->next==null");  */
    return;
  }

  // 当c1和c2为相邻节点时
  if (c1->next == c2) {
    c1->next = c2->next;
    c2->next = c1;
    tmp1->next = c2;
  } else if (c2->next == c1) {
    c2->next = c1->next;
    c1->next = c2;
    tmp2->next = c1;
  } else { // 不为相邻节点
    tmp1->next = c2;
    tmp2->next = c1;
    Client *tmp = c1->next;
    c1->next = c2->next;
    c2->next = tmp;
  }

  // 当更换节点为头节点时，重置头节点
  if (c1 == selmon->clients) {
    selmon->clients = c2;
  } else if (c2 == selmon->clients) {
    selmon->clients = c1;
  }

  focus(c1);
  arrange(c1->mon);
  pointerfocuswin(c1);
}

void exchange_client(const Arg *arg) {
  Client *c = selmon->sel;
  if (!c || c->isfloating || c->isfullscreen)
    return;
  exchange_two_client(c, direction_select(arg));
}

// 切换task标签完整title(或者icon),限长title
void fullname_taskbar_activeitem(const Arg *arg) {
  if (!selmon->sel)
    return;
  if (selmon->sel->no_limit_taskw == 1) {
    selmon->sel->no_limit_taskw = 0;
  } else {
    selmon->sel->no_limit_taskw = 1;
  }
  drawbar(selmon);
}

int main(int argc, char *argv[]) {
  if (argc == 2 && !strcmp("-v", argv[1]))
    die("dwm-6.3");
  else if (argc != 1)
    die("usage: dwm [-v]");
  if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
    fputs("warning: no locale support\n", stderr);
  if (!(dpy = XOpenDisplay(NULL)))
    die("dwm: cannot open display");
  checkotherwm();
  setup();
#ifdef __OpenBSD__
  if (pledge("stdio rpath proc exec", NULL) == -1)
    die("pledge");
#endif /* __OpenBSD__ */
  scan();
  runAutostart();
  run();
  cleanup();
  XCloseDisplay(dpy);
  return EXIT_SUCCESS;
}
