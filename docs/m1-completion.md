# M1 completion — Linux AT-SPI selection detection

**M1 Gate = PASS（规范第 42 节代码级门槛）。** 真实 GNOME 应用兼容性尚未验收；不把 Xvfb 结果外推到 GNOME X11 / Wayland。

## 版本与证据

- M0 accepted baseline: `34259560044bfaa5df9d04e56ca3bf025037eb68`。
- 最终通过测试的实现提交: `1be0a17c63419d6814e7c654c4b9d8f931979785`；其后收尾提交仅更新文档。
- [Ubuntu 22.04 CI — 全部成功](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/34493879850)，job `102927284553`，2026-09-10。
- 原始 M0 规范保留在 PROJECT_SPEC.md；用户 M1 文档原文保留在 [M1_SPEC.md](M1_SPEC.md)。
- 本机为 Windows，WSL 虚拟化不可用；Linux 构建和运行证据来自实际 Ubuntu CI，不声称本机 Linux 桌面已测。

## 实现与变更文件

- `src/platform/ISelectionMonitor.h`：共享 Qt 接口、状态及选区信号。
- `src/platform/linux/AtSpiSelectionProvider.{h,cpp}`：可测试的范围、文本、应用名和可选矩形提取。
- `src/platform/linux/IAtSpiSession.h`、`AtSpiNativeSession.cpp`：libatspi 生命周期、原生事件和 GLib 工作线程。
- `src/platform/linux/AtSpiSelectionMonitor.{h,cpp}`：80 ms debounce、序号过期过滤、去重、停止清理和 45 秒缓存过期。
- `src/platform/PlatformFactory.h`、Linux factory、`src/main.cpp`、`src/app/Application.{h,cpp}`：启动、注入和诊断集成。
- `src/ui/SelectionDiagnostics.{h,cpp}`：默认仅元数据，显式点击才显示本地文本快照。
- `CMakeLists.txt`、`.github/workflows/ubuntu.yml`：libatspi / GObject 依赖及真实运行测试。
- `tests/TestAtSpiSelection.cpp`、`TestSelectionMonitor.cpp`、`TestAtSpiRuntime.cpp`、`AccessibleFixture.cpp`、`TestFlow.cpp`：提取、生命周期、原生事件和隐私回归。
- README、架构、隐私、排错、兼容性矩阵、M1 计划及本报告同步更新。

M0 的 core、AI、AppController 和 LinuxSecretStore 实现相对 accepted baseline 没有改动。

## 架构与事件循环

libatspi 独占一个持久 QThread 中的 GLib default context。Linux factory 在 QApplication 创建前设置进程内 QT_NO_GLIB，避免 GUI 和原生线程争用同一 context。GUI 使用 Qt Unix dispatcher。

原生回调只保留当前事件 source 并发出序号；GUI 单次定时器合并事件，通过 GAsyncQueue / 自定义 GSource 请求工作线程提取；结果通过 queued Qt signals 返回。没有周期轮询、桌面树扫描或 GUI 线程上的同步提取。

Start / Stop 非阻塞且幂等；Stop 注销监听并清缓存，重新 Start 恢复监听。libatspi 只初始化一次，销毁时清理。初始化失败后显示 unavailable；修复总线后需重启应用。销毁可能等待尚未结束的 D-Bus 调用。详见 [架构](m1-architecture.md)。

## Build / Tests / CI

| 检查 | 结果 |
|---|---|
| Ubuntu 22.04 Release，系统 CMake / Qt 6.2 | PASS |
| CTest | 7/7 PASS，0 failed |
| 独立 Secret Service round trip | PASS |
| GUI smoke | PASS |
| 真实 AT-SPI，独立 Qt QTextEdit 进程 + Xvfb + session D-Bus | PASS |
| 重复选择 / 清空 / Stop / Start 后再次检测 | PASS |
| 无效 AT-SPI 总线地址，unavailable 状态与 GUI timer 响应 | PASS |
| M0 UI → 本地模拟 HTTP → 结果复制 | PASS |
| 选区事件及显式本地预览不触发 AI、不改剪贴板 | PASS |

提取测试覆盖空选区、错误、无效范围、多范围首个有效值、超长文本提前拒绝和矩形失败不影响文本。monitor 测试覆盖事件合并、去重、停止后迟到结果和待执行定时器取消。

TDD 记录：`6177bb0` 建立接口及测试时，[CI](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/34492326256) 中新 M1 测试按预期失败，M0 通过；随后实现使其转绿。构建中发现 GObject 链接依赖及 AT-SPI runtime package 缺失，已分别显式加入 `gobject-2.0` 和 `at-spi2-core`。

可复现命令见 README。原生集成命令：

```bash
dbus-run-session -- xvfb-run -a env QT_LINUX_ACCESSIBILITY_ALWAYS_ON=1 ./build/test_atspi_runtime realEvents
env AT_SPI_BUS_ADDRESS=unix:path=/nonexistent/worksidekick-test-bus ./build/test_atspi_runtime unavailableRegistry
```

## AT-SPI / Selection 结果

- Initialization：真实总线成功初始化；缺失总线不崩溃，显示 unavailable。
- Listener：只注册 `object:text-selection-changed`；跨进程选区事件实测到达。
- Text：准确得到 fixture 的 `synthetic selection`，源应用名非空；原文空白不被归一化。
- Rectangle：真实 fixture 返回非空、正宽高的屏幕矩形。没有验证真实桌面定位精度、多显示器或 HiDPI；失败时文本仍有效。
- 重复选区、清空及监听重启通过；没有依赖 clipboard fallback。

## X11 / Wayland 兼容性

[完整矩阵](linux-selection-compatibility.md) 分别列出 GNOME X11 与 GNOME Wayland 的 gedit、Firefox、Chrome/Chromium、VS Code、GNOME Terminal、Slack/Electron，均为 **NOT TESTED**。目前唯一原生集成证据是 Ubuntu 22.04 Xvfb 中独立 Qt QTextEdit。各应用是否需要后续 fallback 为 Unknown。

## Privacy / Ownership / M0

选区仅留在内存中，不自动发送 AI，不写日志或文件，不改剪贴板。默认诊断只有计数、来源、长度与矩形；文本快照需显式点击，并在新选区、清空、停止、过期或隐藏面板时清除。没有新增历史记录。

原生 accessible / Text / listener 使用 GObject 引用释放；字符串、range、rectangle、GError 及 AtspiEvent 使用对应释放方式。排除密码控件和自身进程来源，限制读取范围、文本长度及调用超时。重复事件已测，但未执行 ASan / Valgrind 或长时间 RSS 分析；不宣称完成泄漏压力验证。

M0 手动复制 → Process Clipboard → 用户选择 AI 操作 → Copy result 路径及安全密钥存储保留，回归通过。网络测试用本地模拟服务；本轮未用真实付费 OpenAI API 再测试。

## Known blockers / 下一里程碑

代码级 M1 无已知阻塞。未完成项为真实 GNOME X11/Wayland 应用矩阵、HiDPI / 多显示器定位、长时间资源分析和 Windows 构建；这些不能视为已兼容。AT-SPI 不暴露 Text 或选区的应用会报告不可用/无选区，继续使用 M0 手动模式。

建议下一里程碑：**M2 — Non-activating floating ActionBar**，先处理坐标与焦点保持验证。本次停在 M1，未实现 M2，也未加入 X11 PRIMARY、全局快捷键或输入注入。
