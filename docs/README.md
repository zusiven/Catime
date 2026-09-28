# 项目文档

本目录按功能说明 Catime 的用户可见行为和对应源码。产品介绍和在线使用指南仍以 [中文介绍](../i18n/zh_CN.md)、[English overview](../i18n/en.md) 和[在线指南](https://cati.me/guide)为入口。

这里的功能页记录的是 Catime 如何把功能接入应用，包括状态管理、调用路径、配置和源码入口；它们是项目集成说明，不是对 Windows API、桌面环境或设备行为的独立验证报告。当前结论来自源码阅读，文中涉及的实际窗口、托盘、通知、睡眠恢复等运行时行为尚未在本次复核中验证。边界和启动集成图见[项目集成与验证边界](project-integration.md)。

## 功能文档

| 文档 | 覆盖范围 |
| --- | --- |
| [计时模式](features/time-management.md) | 系统时钟、倒计时、正计时、番茄钟、时间输入 |
| [闹钟与通知](features/alarms-and-notifications.md) | 闹钟规则、触发行为、通知类型、声音和超时动作 |
| [显示与外观](features/display-and-appearance.md) | 浮窗、点击穿透、编辑模式、字体、颜色和效果 |
| [托盘与动画](features/tray-and-animations.md) | 托盘交互、动画素材、内置状态图标 |
| [插件与 Markdown](features/plugins-and-markdown.md) | 插件运行、安全确认、输出文件和富文本语法 |
| [快捷键与命令行](features/shortcuts-and-cli.md) | 全局快捷键、命令行时间参数、单实例转发 |
| [配置与语言](features/configuration-and-language.md) | INI 设置、资源位置、热加载和语言文件 |

## 开发文档

- [开发与构建](development.md)：源码目录、启动和消息分发流程、构建命令、CI 检查。
- [Windows x86 稳定版发布](releasing.md)：日期版本号、标签触发、GitHub Release 和 exe 核验。
- [GitHub Actions 发布工作流](github-actions-release.md)：工作流触发、版本校验、构建产物流转、凭据和常见故障处理。

## 阅读约定

- 文档中的源码链接以仓库根目录为基准，介绍当前代码中的实现位置。
- 用户操作以托盘菜单、设置对话框和现有产品指南为准；配置键和协议细节以源码为准。
- `i18n/*.md` 是产品介绍的语言版本；本目录是面向当前代码仓库的功能和开发文档。
- 文档描述源码中的路径和条件，不单独证明对应功能在特定 Windows 版本、桌面会话或设备上运行成功。
- 如果要记录可独立复现的 Win32 或其他技术能力，应另建技术记录和同目录的可运行验证夹具；通用 CI 启动 smoke 不覆盖单项能力验证。
