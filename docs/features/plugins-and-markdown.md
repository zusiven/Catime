# 插件与 Markdown

插件由独立进程运行，通过共享输出文件把文本交给 Catime 显示。Catime 监视输出文件的变化，并在插件模式激活时刷新浮窗。

## 插件目录和启动

插件目录：

```text
%LOCALAPPDATA%\Catime\resources\plugins
```

程序根据源码中的扩展名列表扫描脚本插件；有专用解释器配置的脚本由对应解释器启动，其他列表中的脚本交由 Windows Shell 打开。首次运行或文件内容变化后，Catime 会显示安全确认。选择“信任并运行”会记录路径和 SHA-256；选择“运行一次”只允许本次启动。支持的扩展名见 [`plugin_extensions.h`](../../include/plugin/plugin_extensions.h)。

插件以当前用户身份运行。信任确认用于识别文件是否与已记录版本一致，不会把插件放入隔离环境。

## `output.txt` 输出

活动插件模式读取以下文件：

```text
%LOCALAPPDATA%\Catime\resources\plugins\output.txt
```

内容使用 UTF-8，可以是纯文本、多行文本或 Markdown。文件在插件模式关闭时会被忽略；启用插件模式后，修改内容会刷新显示。`<notify>` 和 `<exit>` 等控制标签会被处理，不会作为普通显示文本保留。

通知标签示例：

```text
<notify>任务完成</notify>
<notify:catime:5000>休息一下</notify>
```

支持的类型包括默认通知、`catime`、`os` 和 `modal`。其中 `os` 会显示托盘提示，不是 Windows 原生 toast；Catime 通知可以带毫秒超时参数，最大值为 60000。`<notify>` 标签处理本身不调用通知声音播放路径。

## Markdown 与显示标签

完整 Markdown 放在 `<md>...</md>` 中。实现支持标题、列表、引用、链接、粗体、斜体、行内代码和删除线。`<color:...>...</color>` 与 `<font:...>...</font>` 可用于指定文本颜色或字体；颜色标签支持纯色和渐变色。

示例：

```text
<md>
# Focus
- **Work** 25:00
- *Break* 05:00
</md>
```

链接可交互；图片支持由 Markdown 图片语法解析并渲染。具体语法以 [`markdown_parser.h`](../../include/markdown/markdown_parser.h) 为准。

## 源码入口

| 代码 | 职责 |
| --- | --- |
| [`plugin_manager.c`](../../src/plugin/plugin_manager.c) | 扫描、启动、停止和信任检查入口 |
| [`plugin_process.c`](../../src/plugin/plugin_process.c) | 插件进程和解释器管理 |
| [`plugin_data.c`](../../src/plugin/plugin_data.c) | 监视 `output.txt`、处理标签和更新显示文本 |
| [`config_plugin_security.c`](../../src/config/config_plugin_security.c) | 插件 SHA-256 信任记录 |
| [`markdown_parser.c`](../../src/markdown/markdown_parser.c) | Markdown 标签和内容解析 |
| [`drawing_markdown_stb.c`](../../src/drawing/drawing_markdown_stb.c) | Markdown 文本布局与绘制 |
