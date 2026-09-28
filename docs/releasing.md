# Windows x86 稳定版发布

## 发布入口

推送到 `main` 只更新源码。正式发布由 `.github/workflows/release.yml` 中的 `v*` 标签触发。推送 `v2026.09.28` 后，GitHub Actions 会构建 x86 exe、提交 SignPath 签名、创建 GitHub Release，并更新 Winget。

当前工作流调用 `build.sh`，使用 i686 MinGW 编译器；发布资产只有 x86 exe。

## 版本号

发布 `v2026.09.28` 前，在 `resource/resource.h` 中设置：

```c
#define CATIME_VERSION "2026.09.28"
#define CATIME_VERSION_MAJOR 2026
#define CATIME_VERSION_MINOR 9
#define CATIME_VERSION_PATCH 28
#define CATIME_VERSION_BUILD 0
```

应用内版本字符串不带 `v`；Git 标签带 `v`。`resource/catime.rc` 使用这些宏生成 exe 文件属性。工作流分别读取头文件版本和标签版本，不会检查两者是否一致，因此发布前需要人工核对。

`CATIME_VERSION` 也作为配置文件的 `CONFIG_VERSION`。版本不匹配时，程序会进入 `MigrateConfig`：重建配置文件并恢复当前元数据中仍受支持的设置键，也会保留闹钟槽位的时间、启用状态、重复规则、日期和消息。应用内更新检查会去掉 Release 标签的 `v` 前缀，并按三段数字比较版本。

## 发布前检查

1. 确认目标代码和版本修改已提交到 `main` 并推送；等待该提交的 `Catime CI` 检查通过。推送 `main` 本身不会创建 Release。
2. 确认远端尚无同名标签：

   ```bash
   git ls-remote --tags origin 'refs/tags/v2026.09.28*'
   ```

   命令无输出表示远端没有这个标签。不要移动或覆盖已发布的标签。

3. 在 WSL 或 Linux 上本地构建并确认架构：

   ```bash
   ./build.sh Release artifacts/win32
   file artifacts/win32/catime.exe
   ```

   通过条件：构建成功，`file` 显示 `PE32 executable` 和 `Intel i386`。构建脚本在 `build/` 保存中间文件，并将 exe 复制到 `artifacts/win32/`；该目录已被 `.gitignore` 忽略。

## 创建发布

确认远端 `main` 已包含待发布代码和版本号修改、工作区干净后，在该提交上创建并推送标签：

```bash
git switch main
git pull --ff-only origin main
git status --short --branch
git push origin main
git tag -a v2026.09.28 -m "Release v2026.09.28"
git push origin v2026.09.28
```

如果版本提交已经推送，`git push origin main` 会显示已是最新。只有在 `main` 的 CI 通过后才推送版本标签。

标签推送后，`Release Catime` 工作流会依次构建、签名并创建 GitHub Release。Release 资产名为 `catime_2026.09.28.exe`；随后工作流提交对应版本到 Winget。无需手动上传本地 `artifacts/win32/catime.exe`。

## 发布结果检查

在 GitHub Actions 中确认 `Release Catime` 的构建、签名和发布任务均成功。再检查 GitHub Release：

- 标签和版本为 `v2026.09.28`。
- 资产包含 `catime_2026.09.28.exe`，文件属性版本为 `2026.9.28.0`。
- exe 架构为 x86（PE32 / Intel i386）。
- Winget 更新任务成功。

当前工作流的 Release 正文是 `.github/workflows/release.yml` 中的固定文本，不会从提交记录生成更新说明。需要本次变更说明时，应在打标签前更新工作流正文，或在 Release 创建后编辑正文。

如果工作流失败，先查看失败任务并修复原因，再从 Actions 重跑该次工作流；不要强制移动已推送的标签。
