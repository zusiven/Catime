# 开发与构建

## 工程结构

| 路径 | 内容 |
| --- | --- |
| `src/main/` | WinMain、初始化、单实例和命令行转发 |
| `src/window/`、`src/window_procedure/` | 浮窗生命周期、交互和 Windows 消息处理 |
| `src/timer/`、`src/alarm/` | 计时、番茄钟阶段和闹钟 |
| `src/config/` | 配置加载、保存、迁移和热加载 |
| `src/tray/` | 托盘菜单、图标和动画 |
| `src/drawing/`、`src/markdown/`、`src/font/`、`src/color/` | 文本、图片、Markdown、字体和颜色渲染 |
| `src/plugin/` | 插件进程、输出文件和插件文本状态 |
| `src/dialog/`、`resource/` | 对话框实现、资源模板和运行时语言文件 |
| `include/` | 对应模块的公共声明 |

## 启动和消息流

以下是源码中的应用接线顺序；它不是 Windows 系统行为的验证记录。详细阶段入口和验证边界见[项目集成与验证边界](project-integration.md)。

```mermaid
flowchart TD
    A[WinMain] --> B[InitializeSubsystems<br/>日志、COM、GDI+、插件扫描]
    B --> C[InitializeApplicationSubsystem<br/>Markdown 初始化、读取配置和初始化字体]
    C --> D[非 CI smoke 时检查桌面快捷方式]
    D --> E[对话框语言初始化]
    E --> F[单实例检查与已有实例命令转发]
    F --> G[CreateMainWindow]
    G --> H[WM_CREATE<br/>快捷键、窗口设置、配置监视]
    H --> I[SetupMainWindow<br/>插件数据、启动参数、主计时器]
    I --> J[Windows 消息循环]
    J --> K[WindowProcedure 和消息处理模块]
    K --> J
```

`WindowProcedure` 将窗口事件分发到 `window_procedure/` 下的处理模块。多媒体定时器回调会向窗口线程投递消息；计时状态和主要 UI 更新由窗口线程处理。此描述来自代码路径，运行时定时精度未在本次文档复核中测量。

## 构建

### Linux 或 WSL + MinGW-w64

提供的 `build.sh` 使用 i686 MinGW 交叉编译器和 CMake：

```bash
sudo apt install cmake mingw-w64
./build.sh Debug
./build.sh Release
```

默认构建产物位于 `build/`。`build.sh` 还接受输出目录作为第二个参数。

### Windows + MinGW

```bat
build.bat Debug
build.bat Release
```

脚本会在仓库的 `build/` 目录中配置并构建。CMake 项目本身也包含 MSVC 编译选项；需要调整工具链时可直接使用 CMake 配置项目。

## GitHub Actions

`Catime CI` 已停用，其历史配置保存在 [`.github/workflows/build.yml.old`](../.github/workflows/build.yml.old)。GitHub Actions 不会运行 `.old` 文件。当前保留的 [Release Catime](../.github/workflows/release.yml) 工作流用于构建并发布 Windows x86 exe。

本地构建配置见 [`CMakeLists.txt`](../CMakeLists.txt)、[`build.sh`](../build.sh) 和 [`build.bat`](../build.bat)。

## 稳定版发布

Windows x86 稳定版通过 `v*` 标签触发构建并创建 GitHub Release，发布的 exe 不做代码签名。版本修改、标签命令及发布后核验步骤见[Windows x86 稳定版发布流程](releasing.md)。
