# M2 completion — Non-Activating Floating ActionBar

**M2 Gate = PASS（代码级 / Qt xcb 全能力后端）。**
真实 GNOME 应用兼容性与 CI 合成集成是不同验收项；未验证项如下明确标注。

## Baseline / final implementation

- Accepted M0/M1 baseline: `5f9a65cca55c4f1f1a6b6b9c1d7ed00f4db32316`。
- 功能实现提交: `2530feb96ed451dbd4dc0709efe7d86de6a0f840`。
- 最终实现与测量测试提交: `3df76c5c3f88decdf983038383defea09bf62d75`。
- 文档收尾提交只改 Markdown；不改变测试过的源代码。
- [功能实现 CI 全部通过](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/37462648242)，Ubuntu 22.04，2026-10-06。
- [最终测量口径 CI](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/37463066504)：全部通过（job 112267194111）。
- [用户 M2 原始规范](M2_SPEC.md)已保存；M3 未开始。

## 使用方式

在 Settings 勾选 **Automatic selection toolbar**，或在托盘勾选
**Enable Automatic Toolbar**。默认 OFF；开关立即生效并独立持久化，不需要先填好模型。
启用后重新选择文本。Qt xcb 且 AT-SPI 提供有效 anchor 时工具条显示；
点击 Plain Speak / Summarize / Polish 才发送选区。AI 仍需配置模型和安全保存的 API key。
Copy result 是唯一自动工具条流程中修改剪贴板的动作。

## Files added / changed

| 区域 | 文件及职责 |
|---|---|
| 共享 UI | `src/ui/ActionBar.{h,cpp}`：单实例工具条、FeatureRegistry 按钮、值语义快照、dismiss；`ActionBarPlacement.{h,cpp}`：纯几何计算 |
| 窗口策略 | `src/platform/IPlatformWindowPolicy.h`、`src/platform/linux/LinuxWindowPolicy.{h,cpp}`：后端能力、Qt 标志、选屏和定位 |
| 应用集成 | `src/app/Application.{h,cpp}`、`ApplicationOverlay.cpp`：monitor → toolbar、显式点击、生命周期和诊断 |
| AI 入口 | `src/app/AppController.{h,cpp}`：新增 runSelection，复用 runText |
| 设置 | `src/ui/SettingsWindow.{h,cpp}`：唯一新增开关，与托盘同步 |
| 创建 / 版本 | `src/platform/PlatformFactory.h`、Linux factory、`src/main.cpp`：注入策略；版本 0.3.0 |
| 构建 / CI | `CMakeLists.txt`、`.github/workflows/ubuntu.yml`、`scripts/test-overlay.sh` |
| 测试 | `TestActionBar.cpp`、`TestOverlay.cpp`、`TestWindowPolicy.cpp`、`TestOverlayRuntime.cpp`、扩展 `TestController.cpp` 与 `AccessibleFixture.cpp` |
| 文档 | M2_SPEC、m2-plan、m2-completion、m2-overlay-compatibility；README、architecture、privacy、troubleshooting |

M1 原生 AT-SPI session / monitor、AI provider、libsecret 实现均未改动。
M0/M1 既有测试保留；没有删掉断言或放宽原有通过条件。

## Architecture / snapshot ownership

M1 Selection → Application → ActionBar 是纯本地路径。
ActionBar 持有 `std::optional<Selection>` 值；B 到达替换 A。
点击先复制当前值，再隐藏并清空 bar，随后发出 featureChosen。
Application 只在该显式点击信号上调用 runSelection。
runSelection 复用现有 prompt、keyring、AI、ResultCard 管线，无 AT-SPI 或窗口知识。

Clear/45 秒过期、monitor stop/unavailable、dismiss、禁用、AI loading 均清空并隐藏 bar；
busy 时忽略新的工具条显示，不建立请求队列。重用同一窗口，没有轮询、动画、
输入钩子或反复创建窗口。M0 手动 captureClipboard 路径并行保留。
启用不回放旧选区；请求完成也不自动回放 busy 期间的选区。

## Window policy / X11 / Wayland

使用 Qt::Tool、FramelessWindowHint、WindowStaysOnTopHint、
WindowDoesNotAcceptFocus，配合 WA_ShowWithoutActivating 和按钮 NoFocus。
自动显示路径没有 activateWindow、raise、requestActivate 或 setFocus。
没有新增产品原生 X11 调用；Xlib 只用于 Linux 集成测试。

LinuxWindowPolicy 检查 QGuiApplication::platformName，独立于 XDG_SESSION_TYPE：
- xcb：AnchoredNonActivating，尝试锚定显示，包括 Wayland 桌面上的 Qt xcb。
- wayland / wayland-egl：Unsupported，禁用自动锚定工具条，保留 AT-SPI 与 M0。
- 其他后端：Unsupported。

真实 XWayland 桌面、原生 Wayland compositor：**NOT TESTED**。
能力分类与 unsupported 降级已经自动化测试；这不是原生 Wayland 运行验证。

## Placement

默认在选区上方水平居中，8 px 间隙；上方不足则放到下方；再对目标屏幕
availableGeometry 四边夹取。screenAt(anchor.center) 优先，找不到则 primaryScreen。
工具条无法放入屏幕时不显示。缺失/无效 anchor 不丢掉 M1 已缓存的文本，也不伪造位置。

纯逻辑测试覆盖 above、below、left/right/top/bottom clamp、负坐标屏幕以及无效/过大尺寸。
真实合成窗口检查最终 Qt geometry 等于 anchor 算法结果并在屏幕内。
保留原始 AT-SPI 矩形，不加猜测式 DPI 转换；诊断分别显示原始 anchor 与请求的 Qt placement。

## Focus preservation evidence

CI 在隔离 Xvfb 中启动 Openbox；测试先确认 window manager 已存在。
独立 Qt QTextEdit fixture 显示并提供窗口 ID；测试建立源窗口焦点，
fixture 自己在定时器中选择合成文本，没有向第三方编辑器注入鼠标或键盘。

系统 XGetInputFocus 在自动工具条显示前后返回相同的源窗口 ID：**PASS**。
随后 QTest 向本应用工具条按钮发送测试鼠标事件，功能信号和快照正确，
并再次检查系统焦点不变：**PASS**。

按钮测试不等价于真实物理鼠标穿过所有桌面 window-manager 路径。
为隔离工具条行为，原生焦点测试中点击时断开 Application 的 AI 接收端，
避免允许正常激活的 ResultCard 干扰测量；完整点击 → HTTP 在独立集成测试覆盖。
真实 GNOME / 用户鼠标交互焦点：**NOT TESTED**。

## Tests / regression / privacy

| Gate | 结果 |
|---|---|
| Ubuntu 22.04 Release configure / build | PASS |
| CTest | 10/10 PASS |
| M0 clipboard → HTTP → ResultCard → Copy | PASS |
| libsecret 隔离 keyring round trip | PASS |
| GUI smoke | PASS |
| M1 提取、debounce、stop/start、真实 event / text / app / rectangle | PASS |
| M1 unavailable registry 与 GUI 响应 | PASS |
| 工具条本地显示，等待事件循环后 HTTP=0、secret reads=0 | PASS |
| 显式点击后 HTTP=1，payload 为选区 B，不含 A 或无关剪贴板 | PASS |
| Copy result 前剪贴板不变 | PASS |
| 清空后旧按钮 click 不再发请求；busy 无重叠请求 | PASS |
| Settings/tray 同步、默认 OFF、持久化；关闭不停止 monitor | PASS |
| 缺失 anchor / unsupported / monitor failure 安全隐藏 | PASS |
| Xvfb + Openbox 原生事件 → ActionBar → 定位 / 系统焦点 | PASS |
| 本进程 QTextEdit 选区不递归弹出工具条 | PASS |

TDD 失败基线：
- `9dfd8c7`，[CI 37461877192](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/37461877192)：
  原 7 组通过，新工具条/定位断言按预期失败。
- `447b645`，[CI 37462112033](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/37462112033)：
  runSelection、工具条生命周期与 HTTP 集成测试在缺失实现处失败。
- `2530feb`：全部转绿。

所有 HTTP 验证使用 loopback synthetic endpoint。本轮真实付费 OpenAI 调用：**NOT TESTED**。
选择文本、显示 toolbar 和诊断均不自动上传；无选区日志、落盘历史或自动复制。
设置只保存非敏感偏好；密钥仍使用 Secret Service。窗口缓存按 M1 清理信号释放，
不声称 Qt 隐式共享字符串实现了安全内存擦除。

## Performance

没有新增周期轮询、busy wait 或产品 sleep。M1 80 ms debounce 保留。
最终测量记录从 monitor 收到原生事件到 toolbar 可见，包含 debounce 和提取，
不包含源应用到 accessibility bus 的传播时间。最终 CI 合成样本为 **93 ms**（包含 debounce / extraction，排除 source-to-bus transit）。
该合成单次样本不构成长时间延迟保证；长时间 RSS / ASan / Valgrind：**NOT TESTED**。

## Real desktop compatibility / known blockers

[完整矩阵](m2-overlay-compatibility.md)列出 GNOME X11 / GNOME Wayland 的
gedit、Firefox、Chromium、VS Code、GNOME Terminal、Slack/Electron。
这些真实应用组合均为 **NOT TESTED**。开发宿主仍为 Windows，Linux 证据来自 Ubuntu CI。

代码级无已知阻塞。HiDPI、fractional/mixed DPI、真实多显示器、XWayland desktop、
Windows build/backend 均 **NOT TESTED**。负坐标单元测试不等于多显示器实测。
native Wayland 锚定能力刻意标为 unsupported，不阻塞规范定义的 M2 PASS。
没有全局快捷键、PRIMARY fallback、模拟 Ctrl+C/Ctrl+V、自动替换或消息发送。

## Recommended next milestone

**M3 — Linux Fallbacks + Global Shortcut**。
建议继续保留显式点击授权和快照传递；Linux fallback 与全局快捷键分别通过平台接口，
运行时探测可用性，优先补齐真实 GNOME 兼容性矩阵，再决定缺失 anchor/AT-SPI 的手动触发策略。
本次停在 M2，未实施 M3。
