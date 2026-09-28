# 显示与外观

主界面是无边框浮窗。常规模式支持点击穿透，让鼠标操作继续落到浮窗后方；编辑模式下可以移动和缩放浮窗，并通过菜单调整显示选项。

## 可调整项目

- **位置与尺寸**：可移动、缩放，设置启动时的窗口位置。
- **可见性**：控制窗口显示、透明度和置顶状态。
- **字体**：使用内置字体或指定自定义字体文件。
- **颜色**：文本颜色支持纯色和渐变色；颜色列表用于菜单和设置对话框。
- **文字效果**：支持无效果、Glow、Glass、Neon、Holographic 和 Liquid 等效果。
- **时间格式**：配置 12/24 小时制、秒数、毫秒显示和补零格式。

编辑模式关闭时，窗口会根据鼠标位置判断是否有可交互区域；窗口其他区域返回透明命中结果。Markdown 链接等可交互内容仍可单独点击。

## 渲染路径

普通时间文本由 drawing 模块测量和绘制；字体管理、图像显示和 Markdown 渲染分别由独立模块处理。插件输出的 Markdown 语法见[插件与 Markdown](plugins-and-markdown.md)。

## 源码入口

| 代码 | 职责 |
| --- | --- |
| [`window_core.c`](../../src/window/window_core.c) | 主浮窗创建、尺寸和位置 |
| [`window_visual_effects.c`](../../src/window/window_visual_effects.c) | 分层窗口、点击穿透和 DWM 效果 |
| [`window_events.c`](../../src/window_procedure/window_events.c) | 窗口交互和编辑模式事件 |
| [`drawing_render.c`](../../src/drawing/drawing_render.c) | 文本测量、绘制调度和 Markdown 分派 |
| [`font_manager.c`](../../src/font/font_manager.c) | 字体加载与管理 |
| [`color_parser.c`](../../src/color/color_parser.c) | 颜色和渐变解析 |
| [`drawing_effect.c`](../../src/drawing/drawing_effect.c) | 文字视觉效果 |
