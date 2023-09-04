# from pynput import mouse, keyboardq
import time
# import os
import pyautogui
# import subprocess
# import keyboard as kb

inarea = False
pyautogui.FAILSAFE = False
# from PyHotKey import Key, keyboard_manager as manager

# super_press = False
# ctrl_press = False
# alt_press = False

# def is_vlc_active():
#     # 使用wmctrl命令来获取当前活动窗口的标题
#     title = os.popen(
#         'wmctrl -l | grep $(xprop -root | grep _NET_ACTIVE_WINDOW | head -1 | awk \'{print $5}\' | awk -F "x" \'{print $2}\')').read()
#     # 如果标题包含"VLC media player"，则返回True，否则返回False
#     print(title)
#     return "VLC media player" in title



# def on_click(x, y, button, pressed):
#     if button == mouse.Button.middle and pressed:
#         #  print("middle pressed")
#         pyautogui.keyDown("alt")
#         time.sleep(0.01)
#         pyautogui.keyDown("a")
#         time.sleep(0.01)
#         pyautogui.keyUp("a")
#         pyautogui.keyUp("alt")
#        # keyboard.press('alt')
#        # keyboard.press('a')
#        # keyboard.release('a')
#        # keyboard.release('alt')


# def on_scroll(x, y, dx, dy):

#     if super_press:
#         if dy < 0:
#             pyautogui.keyDown("right")
#             time.sleep(0.01)
#             pyautogui.keyUp("right")
#         else:
#             pyautogui.keyDown("left")
#             time.sleep(0.01)
#             pyautogui.keyUp("left")           # time.sleep(0.3)


# mouse_listener = mouse.Listener(on_click=on_click, on_scroll=on_scroll)
# mouse_listener.start()


# def kb_on_press(key):
#     global super_press
#     global alt_press
#     global ctrl_press
#     if ctrl_press == True or alt_press == True:
#         if key.scan_code == 79:
#             kb.press_and_release(2)
#         if key.scan_code == 80:
#             kb.press_and_release(3)
#         if key.scan_code == 81:
#             kb.press_and_release(4)
#         if key.scan_code == 75:
#             kb.press_and_release(5)
#         if key.scan_code == 76:
#             kb.press_and_release(6)

#     print(key.scan_code)



# kb.on_press(kb_on_press)

# def workspace1():
#     kb.press_and_release(2)
# def workspace2():
#     kb.press_and_release(3)
# def workspace3():
#     kb.press_and_release(4)
# def workspace4():
#     kb.press_and_release(5)
# def workspace5():
#     kb.press_and_release(6)

# kb.add_hotkey("ctrl+79",workspace1,suppress=True)
# kb.add_hotkey("ctrl+80",workspace2,suppress=True)
# kb.add_hotkey("ctrl+81",workspace3,suppress=True)
# kb.add_hotkey("ctrl+75",workspace4,suppress=True)
# kb.add_hotkey("ctrl+76",workspace5,suppress=True)

# kb.add_hotkey("alt+79",workspace1,suppress=True)
# kb.add_hotkey("alt+80",workspace2,suppress=True)
# kb.add_hotkey("alt+81",workspace3,suppress=True)
# kb.add_hotkey("alt+75",workspace4,suppress=True)
# kb.add_hotkey("alt+76",workspace5,suppress=True)

# def on_press(key):
#     global super_press
#     if key == keyboard.Key.cmd:
#         super_press = True


# def on_release(key):
#     global super_press
#     if key == keyboard.Key.cmd:
#         super_press = False


# with keyboard.Listener(
#         on_press=on_press,
#         on_release=on_release) as listener:
#     listener.join()



# 循环检测鼠标位置
while True:
    # 获取鼠标的当前位置
    mouse_x, mouse_y = pyautogui.position()

    # 如果鼠标在右上角
    if mouse_y > 1070 and mouse_x < 10 and not inarea:
        # 模拟按下win键
        # keyboard.press_and_release('win')
        pyautogui.keyDown("win")
        time.sleep(0.01)
        pyautogui.keyDown("a")
        time.sleep(0.01)
        pyautogui.keyUp("a")
        pyautogui.keyUp("win")
        inarea = True
    elif inarea and (mouse_y <= 1070 or mouse_x >= 10):
        inarea = False
    time.sleep(0.1)

