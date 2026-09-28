# 托盘与动画

Catime 使用 Windows 系统托盘图标提供常驻入口。托盘菜单将计时控制与外观设置分开。

## 菜单入口

- **左键单击**：打开计时菜单，可暂停或继续、重启计时、切换时间显示、选择番茄钟和闹钟等。
- **右键单击**：打开设置菜单，可切换编辑模式、超时动作、快捷键、格式、字体、颜色、插件和托盘动画等。

具体菜单内容会随当前运行状态变化。

## 托盘图标内容

动画设置支持 GIF、WebP、静态图片以及由图像文件组成的动画目录。内置内容包括应用 Logo、CPU 和内存占用、电池百分比及 Caps Lock 状态。动画速度可以使用原始速度或根据系统状态进行调整。

动画资源目录为：

```text
%LOCALAPPDATA%\Catime\resources\animations
```

托盘动画与主窗口文字渲染使用不同模块；更换托盘图标不会改变主窗口的字体或颜色设置。

## 源码入口

| 代码 | 职责 |
| --- | --- |
| [`tray_events.c`](../../src/tray/tray_events.c) | 托盘鼠标事件和菜单入口 |
| [`tray_menu.c`](../../src/tray/tray_menu.c) | 左键、右键菜单的组合与显示 |
| [`tray_animation_loader.c`](../../src/tray/tray_animation_loader.c) | 动画素材和内置状态图标选择 |
| [`tray_animation_decoder.c`](../../src/tray/tray_animation_decoder.c) | GIF/WebP 动画帧解码 |
| [`tray_animation_core.c`](../../src/tray/tray_animation_core.c) | 动画状态、帧切换和速度调整 |
