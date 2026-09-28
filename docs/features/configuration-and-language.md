# 配置与语言

## 文件位置

主配置文件位于：

```text
%LOCALAPPDATA%\Catime\config.ini
```

首次运行时程序会创建 `Catime` 配置目录及资源子目录：

```text
%LOCALAPPDATA%\Catime\resources\audio
%LOCALAPPDATA%\Catime\resources\fonts
%LOCALAPPDATA%\Catime\resources\animations
%LOCALAPPDATA%\Catime\resources\plugins
```

## 配置分组

源码使用的主要 INI 分组如下：

| INI 分组 | 内容 |
| --- | --- |
| `[General]` | 通用应用设置 |
| `[Display]` | 浮窗显示、尺寸和外观参数 |
| `[Timer]` | 当前时间格式、倒计时和超时动作 |
| `[Pomodoro]` | 番茄钟时长数组和循环数 |
| `[Alarm]` | 闹钟列表 |
| `[Notification]` | 通知显示和声音设置 |
| `[Hotkeys]` | 全局快捷键 |
| `[Colors]` | 颜色选项 |
| `[RecentFiles]` | 最近使用的文件列表，由专用逻辑读写 |
| `[Animation]` | 托盘动画及状态图标参数，由动画模块单独读取 |
| `[Options]` | 配置写入逻辑无法按已知键名前缀归类时使用的回退组 |
| `[PluginTrust]` | 插件信任记录，由插件安全配置模块管理 |

`[Options]` 和 `[PluginTrust]` 不属于 `config_defaults.c` 中的常规配置快照分组。默认值与常规配置项元数据集中在 [`config_defaults.c`](../../src/config/config_defaults.c)，应用运行时配置结构定义在 [`config.h`](../../include/config.h)。

配置解析和写入支持 UTF-8。更新时先写临时文件再替换目标文件，并使用进程内锁和跨进程互斥量保护写入。程序也监视配置文件变化；外部编辑完成后会通知主窗口重新处理配置。

## 语言资源

运行时界面文本位于 `resource/languages/*.ini`，语言选择和字符串查找由 `src/language.c` 管理。`i18n/*.md` 是仓库产品介绍的多语言版本，不是运行时界面语言文件。

## 源码入口

| 代码 | 职责 |
| --- | --- |
| [`config_path.c`](../../src/config/config_path.c) | 配置文件和资源目录位置 |
| [`config_ini.c`](../../src/config/config_ini.c) | UTF-8 INI 解析、缓存和原子写入 |
| [`config_core.c`](../../src/config/config_core.c) | 配置初始化、版本处理和整体协调 |
| [`config_loader.c`](../../src/config/config_loader.c) | 配置值装载到配置快照 |
| [`config_applier.c`](../../src/config/config_applier.c) | 将快照应用到运行时状态 |
| [`config_watcher.c`](../../src/config/config_watcher.c) | 配置文件变化监视 |
| [`language.c`](../../src/language.c) | 运行时语言加载与查找 |
