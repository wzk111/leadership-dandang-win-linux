# M3 completion — Linux Fallback Selection + Global Shortcut

**M3 Gate = PASS（代码级、Ubuntu 22.04 / Qt xcb 合成运行验证）。**
按 M3_SPEC 第 89–91 节，真实 Portal 不可用或未测试不阻塞代码级 PASS；
不能据此宣称真实 GNOME / Wayland 全面兼容。M4 未开始。

## Baseline / final implementation

- 已验收 M2 baseline: `dacde1e335a9bdcf3439209cd937a92dcf42f9ca`。
- 最终生产实现: `db02d8942244241a54535ceef2b003140d03e7f7`。
- 最终实现与测试: `35d2ec96da65e5f33ca7b8bcf541fd28c68a870e`。
- 文档收尾提交仅修改 Markdown，不改变已测试的实现。
- [实现 CI](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/37468808702)：PASS，job 112286585122。
- [最终测试 CI](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/37469142658)：PASS，job 112287741089；所有步骤成功。
- 环境：GitHub Actions Ubuntu 22.04，系统 Qt 6.2，Release；2026-10-06。
- 原始 [M3_SPEC.md](M3_SPEC.md) 完整保留。Windows 主机没有可用 Linux 桌面；
  本次 Linux 编译与运行证据来自 CI，没有修改主机虚拟化或桌面设置。

## 使用方式

在 Settings 或托盘启用 **Enable global shortcut**（默认 OFF）。
Qt xcb 注册 **Ctrl+Alt+P**；原生 wayland* 尝试 Portal，由桌面决定实际绑定。
选中或复制文本后触发快捷键，或点击托盘 **Open Selection Actions**，
或运行 `/absolute/path/to/worksidekick --trigger`。

手动面板先显示来源与本地预览；点击 Plain Speak / Summarize / Polish 才调用 AI。
模型与安全 API key 继续使用 M0 设置。自动工具条与快捷键是两个独立开关。
Portal 在重启后不会自动弹出授权/配置界面；点击 **Register / retry shortcut** 显式重试。
GNOME 自定义快捷键的配置步骤见 [troubleshooting](troubleshooting.md)。

## Files added / modified

| 区域 | 文件 / 职责 |
|---|---|
| Resolver | `src/core/SelectionResolver.{h,cpp}`，`src/platform/linux/LinuxSelectionResolver.{h,cpp}` |
| Manual UI | `src/ui/ManualActionPalette.{h,cpp}`；`SettingsWindow.{h,cpp}` 增加独立开关与重试按钮 |
| 应用集成 | `src/app/ApplicationManual.cpp`，`Application.{h,cpp}`；复用既有 controller |
| X11 | `src/platform/IGlobalShortcut.h`，`linux/X11GlobalShortcut.{h,cpp}` |
| Portal | `linux/PortalGlobalShortcut.{h,cpp}`、`PortalTransport.h`、`PortalTypes.h`、`QtPortalTransport.{h,cpp}` |
| IPC / CLI | `src/app/InstanceCoordinator.{h,cpp}`、`src/main.cpp` |
| Factory | `src/platform/PlatformFactory.h`、`linux/PlatformFactory.cpp` |
| 构建与 CI | `CMakeLists.txt`、`.github/workflows/ubuntu.yml`、`scripts/test-overlay.sh`；版本 0.4.0 |
| 测试 | `TestManual.cpp`、`TestManualFlow.cpp`、`TestInstance.cpp`、`InstancePeer.cpp`、`TestPortal.cpp`、`TestPortalDBus.cpp`、`TestX11Shortcut.cpp`、`PrimaryOwner.cpp` |
| 文档 | M3 spec / plan / completion / compatibility；README、architecture、privacy、troubleshooting |

AI provider、AppController、原生 AT-SPI session / monitor、LinuxSecretStore 没有改动。
既有 M0/M1/M2 测试保留，没有删除断言或放宽通过条件。

## Selection resolution / snapshot

所有显式入口汇入 Application::triggerManualActions，在窗口激活前解析：
**有效 M1 缓存 → 支持的 X11 PRIMARY → Clipboard**。回调惰性执行；
前一来源有效就不读取后续来源。空白或超过 100,000 UTF-16 单元的文本无效。
AT-SPI 保留原应用与 anchor；PRIMARY / Clipboard 不伪造 anchor。

AT-SPI 只读 M1 当前运行监视器的缓存，不新增原生查询或轮询。
M1 的清除、停止与 45 秒过期规则继续生效。缓存仍可能与用户当前意图不同，
因此面板显示来源与预览，让用户在上传前核对。

PRIMARY 仅在 Qt xcb 且 supportsSelection 为真时读取 QClipboard::Selection。
跨进程 fixture 同时持有不同 PRIMARY / Clipboard 文本，测试验证：
优先选 PRIMARY、普通剪贴板不变、PRIMARY 不被应用修改；
PRIMARY 空时回退到 Clipboard。原生 Wayland 跳过 PRIMARY。

ManualActionPalette 是普通可聚焦窗口，不依赖绝对定位。只保留一个值语义快照；
最多预览前 1,000 字符，功能按钮仍使用完整原始快照。再次触发替换快照；
空触发、关闭或隐藏清除文本并禁用按钮。AI 忙时显示提示，不读取新来源、
不排队、不发起并发请求。只有功能点击连接既有 runSelection 管线。

## X11 shortcut / collision

专用 worker 与独立 X display，XGrabKey 注册 Ctrl+Alt+P，
覆盖所有 root 与 Caps Lock / Num Lock 组合。只观察注册组合，
不监听普通按键内容。XInitThreads 在 QApplication 之前调用。
阻塞 poll 等待 X fd 或退出 pipe；没有 GUI 忙循环或周期性轮询。

短暂、串行的 X error trap 检测抢占冲突；失败清理自身 grab，
不抢占其他程序。停止 / 退出释放抓取；queued callback 有 generation 检查。
键盘映射在运行中改变时需要 disable / re-enable。

Xvfb 内实际 XTest 按键测试验证触发、锁键组合、第二个注册者失败、
注销后可重新注册。同一测试也在 Openbox 中通过。
XTest 只链接测试目标，生产路径没有输入注入。

## Portal implementation / runtime

Qt 后端 wayland* 选择 Portal。运行时异步 Properties.Get 查询
org.freedesktop.portal.GlobalShortcuts 的 version。支持 v1 的
CreateSession → BindShortcuts → Activated；不依赖 v2 ConfigureShortcuts。

- 请求前订阅带 handle_token 的 Response，避免回复竞态。
- 仅显式启用 / 重试可创建和绑定；保存启用偏好后的启动只 probe。
- action ID 固定为 worksidekick.trigger；请求 CTRL+ALT+p，显示返回的实际 trigger_description。
- 精确核对 session / action ID，过滤无关激活。
- 支持取消、错误、超时、Session.Closed、服务 owner 消失与 DBus 断开；
  不自动重弹配置，清理 request / session，忽略迟到回复。
- 方法等待 10 秒；用户配置请求最多 180 秒；全部为异步。
- 如果激活信号订阅失败，报告不可用，不误报成功。

Qt 6.2 的实际测试暴露了 QDBusObjectPath 在订阅前未注册的问题：
D-Bus 信号签名正确，但 connect 返回 false。现在提前注册类型并检查结果。
另一个回归确保仅 probe 后的 Portal 消失也会清除 available 状态。

测试分为 mock state machine 与隔离 session D-Bus 上的 fake portal service。
后者确实经过 Qt DBus 编解码，验证 version=1、preferred_trigger、异步请求、
返回实际绑定 Super+P、Activated、Session.Closed、服务消失、取消和不可用。
这是协议集成测试，**不是真实桌面 Portal 测试**。

- Portal code / protocol: PASS。
- Probe / graceful unavailable fallback: PASS（模拟服务及服务不存在场景）。
- Fake service version: 1；状态机另覆盖 version 2。
- Real portal version / runtime: **NOT TESTED**。
- GNOME 权限配置 UI、真实组合键、原生 Wayland 窗口激活: **NOT TESTED**。

## --trigger / single instance / IPC security

--trigger 有实例时转发 trigger 命令并退出；没有实例则启动并打开面板。
普通第二次启动转发 open 命令，显示现有 workspace。CLI 与托盘走相同解析路径。

IPC 使用用户 runtime 下的 0700 子目录、UserAccessOption 文件系统本地 socket，
Linux SO_PEERCRED 检查 uid。拒绝叶目录符号链接与非本用户目录。
只有 trigger / open 固定有界命令；无文本、key、公共 TCP 或特权操作。
连接数、帧大小与空闲时间均有限制，转发须收到确认。

QLockFile 在主实例整个生命周期持有；只有持锁者可以清理不可达旧 endpoint。
活实例无响应或启动竞争时失败并要求重试，不删活 socket、不产生第二个托盘进程。
测试覆盖格式错误命令、目录权限、跨进程转发、杀死旧进程后的 stale 恢复、
生产可执行文件冷启动与第二次 --trigger。同时启动两个子进程的测试也通过：只有一个 primary，失败方退出后重试可达。

--smoke-test 为开发验证专用，故意绕过单实例协调。
不同登录会话应使用各自正确的 XDG_RUNTIME_DIR。

## Verification / regression

| 项目 | 结果 / 证据 |
|---|---|
| Ubuntu 22.04 Release | PASS，系统 CMake / Qt 6.2 |
| CTest | PASS，16/16；包含既有 10 组与新增 6 组 |
| M0 | PASS：core、AI、UI、controller、flow；独立 keyring 集成与 launch smoke |
| M1 | PASS：解析 / monitor；真实 libatspi 合成事件与 registry unavailable |
| M2 | PASS：ActionBar / overlay / window policy；Openbox 系统焦点与定位 |
| M3 resolver / palette | PASS：优先级、惰性读取、界限、快照、替换、清空 |
| M3 privacy / HTTP | PASS：触发时 0 request / 0 key read；显式点击恰好 1 个 loopback HTTP 请求，文本为捕获快照 |
| X11 PRIMARY | PASS：独立进程持有 selection；读取不写入普通 Clipboard |
| X11 shortcut | PASS：真实 X server 被动抓取 + 测试按键，含 Openbox |
| Portal | PASS：状态机及隔离 DBus fake service；真实桌面 NOT TESTED |
| --trigger / IPC | PASS：生产 CLI、跨进程、权限 / 无效帧 / stale 恢复 / 并发启动 |
| GNOME X11 | NOT TESTED，见完整兼容性矩阵 |
| GNOME Wayland | NOT TESTED，见完整兼容性矩阵 |
| Live OpenAI account | NOT TESTED；测试只用本地合成响应，无付费请求 |
| Windows application | NOT TESTED / M5，未实现原生后端 |

TDD 的 RED 证据分别见 runs
[37465213840](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/37465213840)（resolver/palette）、
[37465631057](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/37465631057)（shortcut）、
[37465871497](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/37465871497)（manual flow/IPC）。
这些构建先成功编译、再在占位实现断言处失败；最终实现通过相应测试。

## Privacy review / limitations / next milestone

没有 keylogging、普通键盘内容监视、Ctrl+C 注入、uinput、自动粘贴/替换/发送、
clipboard/PRIMARY 历史或源文本日志。新设置只有非敏感 shortcut/enabled。
诊断仅显示来源、长度、时间、能力、注册状态与错误；不会为刷新诊断读取 Clipboard。
关闭手动面板清除其文本；请求仍由已有安全 keyring 与 AI 管线处理。

真实 GNOME X11 / Wayland 各应用、真实 Portal、XWayland 与原生应用互通、
HiDPI / mixed DPI / 物理多屏：**NOT TESTED**，未伪造结果。
没有真实 Linux 桌面可用是本次现场验收限制；不是已知代码级阻塞。
Native Wayland 自动锚定 ActionBar 仍按 M2 策略禁用。
Portal 缺失可使用托盘或桌面自定义快捷键调用 --trigger。

Recommended next milestone: **M4 — Full Feature Set / Product UX Expansion**。
**在 M3 停止，等待用户提供 M4 文档。**
