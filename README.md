# Catime

Catime 是一个使用 C11 和 Win32 API 编写的 Windows 桌面时间工具，支持当前时间、倒计时、正计时、番茄钟、闹钟、托盘动画和插件显示。本仓库包含闹钟等功能的扩展实现。

## 按功能查文档

| 功能 | 内容 |
| --- | --- |
| [计时模式](docs/features/time-management.md) | 当前时间、倒计时、正计时、时间输入格式、番茄钟 |
| [闹钟与通知](docs/features/alarms-and-notifications.md) | 多个闹钟、重复规则、超时动作、通知和声音 |
| [显示与外观](docs/features/display-and-appearance.md) | 浮窗交互、字体、颜色、透明度、视觉效果 |
| [托盘与动画](docs/features/tray-and-animations.md) | 托盘菜单、自定义动画、系统状态图标 |
| [插件与 Markdown](docs/features/plugins-and-markdown.md) | 插件进程、`output.txt`、格式标签和安全确认 |
| [快捷键与命令行](docs/features/shortcuts-and-cli.md) | 全局快捷键、时间参数、单实例命令转发 |
| [配置与语言](docs/features/configuration-and-language.md) | 配置文件、资源目录、热加载、多语言资源 |
| [开发与构建](docs/development.md) | 工程结构、启动流程、构建和 CI 检查 |
| [项目集成与验证边界](docs/project-integration.md) | 启动调用链、源码结论与独立技术验证的边界 |

## 使用说明

- [中文产品介绍](i18n/zh_CN.md) · [English overview](i18n/en.md)
- [在线使用指南](https://cati.me/guide)
- 配置文件：`%LOCALAPPDATA%\Catime\config.ini`

## 构建

Linux 或 WSL 环境需要 CMake 和 MinGW-w64：

```bash
sudo apt install cmake mingw-w64
./build.sh Debug
```

Windows 环境可使用已安装 MinGW 和 CMake 的命令行：

```bat
build.bat Debug
```

更多构建参数、目录说明和 CI 信息见[开发与构建](docs/development.md)。

## 项目来源

上游项目：[vladelaina/Catime](https://github.com/vladelaina/Catime)。
