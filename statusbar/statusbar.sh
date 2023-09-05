#! /bin/bash

thisdir=$(cd $(dirname $0);pwd)
tempfile=$thisdir/temp
touch $tempfile

# 设置某个模块的状态 update cpu mem ...
update() {
    [ ! "$1" ] && refresh && return                                      # 当指定模块为空时 结束
    bash $thisdir/packages/$1.sh                                         # 执行指定模块脚本
    shift 1; update $*                                                   # 递归调用
}

# 处理状态栏点击
click() {
    [ ! "$1" ] && return                                                 # 未传递参数时 结束
    bash $thisdir/packages/$1.sh click $2                                # 执行指定模块脚本
    update $1                                                            # 更新指定模块
    refresh                                                              # 刷新状态栏
}

# 更新状态栏
refresh() {
    _music='';_wifi='';_cpu='';_mem='';_date='';_vol='';_light='';_bat=''# 重置所有模块的状态为空
    source $tempfile
    xsetroot -name "$_music$_wifi$_cpu$_mem$_date$_vol$_light$_bat"      

}

# 启动定时更新状态栏 不同的模块有不同的刷新周期 注意不要重复启动该func
cron() {
    wifi_update = 5
    echo > $tempfile                                           # 清空 temp 文件
    let i=0
    while true; do
        is_net=$( cat $DWM/statusbar/netflag )
        if [ -n "$is_net" ];then
            wifi_update=1
        else
            wifi_update=5
        fi 
        to=()                                                            # 存放本次需要更新的模块
        [ $((i % 3)) -eq 0 ]  && to=(${to[@]} vol light)        # 每 20秒  更新 vol light 
        [ $((i % 30)) -eq 0 ] && to=(${to[@]} bat)                      # 每 300秒 更新 bat
        [ $((i % 2)) -eq 0 ] && to=(${to[@]} cpu)                      # 每 300秒 更新 bat
        [ $((i % 5)) -eq 0 ]   && to=(${to[@]} date mem )               # 每 5秒   更新 date cpu mem
        [ $((i % wifi_update )) -eq 0 ]   && to=(${to[@]} wifi )  
        update ${to[@]}                                                  # 将需要更新的模块传递给 update
        sleep 1; let i+=1
    done &
}

# cron 启动定时更新状态栏
# update 更新指定模块 `update cpu` `update mem` `update date` `update vol` `update bat` 等
# updateall 更新所有模块 | check 检查模块是否正常(行为等于updateall)
# * 处理状态栏点击 `cpu 按键` `mem 按键` `date 按键` `vol 按键` `bat 按键` 等
case $1 in
    cron) cron ;;
    update) shift 1; update $* ;;
    updateall|check) update  music wifi cpu mem date vol light bat ;;
    *) click $1 $2 ;; # 接收clickstatusbar传递过来的信号 $1: 模块名  $2: 按键(L|M|R|U|D)
esac
