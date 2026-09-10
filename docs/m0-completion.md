# M0 验收报告

## 当前结论

M0 代码已实现，最终 Ubuntu 22.04 Release CI 已通过配置、编译、5 组 CTest、
真实 libsecret 临时密钥环测试和程序窗口启动冒烟测试，失败数为 0。
已验证源码提交：`3472bd2`；此后的文档提交不改变源码。

**不能将此等同于全部桌面验收完成。** GNOME 实机托盘、真实 OpenAI
账户调用、GNOME X11/Wayland 外部应用剪贴板行为均为 **NOT TESTED**。
当前 Windows 主机的 WSL2 因虚拟化未启用而不能启动；未修改系统配置。

## 1. 创建的文件

- 原始规格：PROJECT_SPEC.md（保留用户原文）。
- 构建与仓库：CMakeLists.txt、.gitignore、.gitattributes、Ubuntu CI。
- core：Selection、FeatureRegistry、PromptBuilder、Settings。
- ai：IAIProvider、OpenAIProvider。
- platform：ISelectionProvider、ClipboardSelectionProvider、ISecretStore、
  LinuxSecretStore、PlatformFactory。
- app：AppController、Application、main.cpp。
- ui：WorkspaceWindow、SettingsWindow、ResultCard；诊断窗口由 Application 组装。
- tests：TestCore、TestAI、TestUI、TestController、TestFlow、TestSecret。
- scripts/resources：隔离密钥环测试脚本、桌面启动文件。
- 文档：README、架构、隐私、排错、Linux/Windows 兼容矩阵、实施计划和本报告。

没有创建 M1–M5 的空实现，也没有擅自选择项目开源许可证。

## 2. 实现的架构

Qt 窗口 → AppController → PromptBuilder → IAIProvider → QtNetwork Responses API。
平台接口隔离密钥存储和剪贴板读取，Linux libsecret 调用在工作线程中执行。
Settings 只持久化模型和输出语言，密钥不进入 QSettings。
三个 M0 功能不携带 profile。

## 3. 构建命令

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
```

Debug 与最终 Release 构建均已通过，使用 GCC 11、Ubuntu 系统 CMake、Qt 6.2.4。
完整依赖安装命令见 README。

## 4. 测试命令

```bash
xvfb-run -a ctest --test-dir build --output-on-failure
dbus-run-session -- bash scripts/test-keyring.sh
xvfb-run -a ./build/worksidekick --smoke-test
```

桌面会话可直接运行 CTest，无需 Xvfb。密钥环测试使用临时 HOME 和专用
D-Bus 会话，不访问用户已有密钥。

## 5. 自动测试结果

最终 Release 验证通过：
[Ubuntu CI 34490343698](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/34490343698)。
5/5 CTest 通过，密钥环保存/读取/删除通过，启动冒烟通过。
新增的排队验证忙碌/取消、密钥环忙碌反馈回归均通过。

首轮 Debug 实现也已通过：
[Ubuntu CI 34489690010](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/34489690010)。

- core：三种功能元数据、提示词、空输入、超长输入、语言、profile 排除、设置往返。
- ai：Responses 输出解析、401/403/429/500、网络失败、空响应、无效 JSON、
  拒绝与不完整响应、缺少 key/model、异步 HTTP、请求字段、超时、取消。
- ui：显式剪贴板读取、空剪贴板、加载/成功/错误、纯文本结果、复制、关闭取消。
- controller：读取本身不上传、用户动作才请求、重叠保护、请求快照、
  空捕获清除旧内容、取消密钥读取后不补发请求。
- flow：实际 Qt 界面点击 → 本地 HTTP 服务器 → 结果显示 → 复制；
  设置窗口打开、密钥字段清空、密钥不进入普通设置。
- secret：真实 libsecret 保存/读取/删除临时凭据。

先行测试基线成功编译后按预期失败：
[CI 34489156561](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/34489156561)。
最初两次配置失败定位到 OpenGL 开发头文件缺失，已补齐依赖。

## 6. 手动与冒烟行为

- 自动化窗口启动、Settings、Diagnostics、结果卡：通过最终 Release CI 冒烟。
- 自动化点击与剪贴板复制：通过 TestFlow。
- 人工在 GNOME 桌面操作：**NOT TESTED**。
- 托盘图标在真实 GNOME 面板显示、菜单交互、关窗驻留：**NOT TESTED**。
- 实际付费 OpenAI 账户与所选模型：**NOT TESTED**，未提供或使用真实 API key。
- 无 OpenAI 真实调用费用；本次测试 API 请求仅发给 loopback 服务器。

## 7. 已知限制

M0 手动读取当前剪贴板，无法判断该内容是不是刚刚复制；不伪装为自动划词。
没有 AT-SPI、浮动工具条、全局快捷键、回复/profile 或 Windows 原生后端。
GNOME 无托盘时主窗口可用，最后窗口关闭时退出。
系统密钥环等待解锁时，界面不会阻塞，但尚无密钥环操作超时/取消接口。
取消网络请求不能撤回已经发往服务端的内容。
无真实 API 测试，不能保证特定账户、模型权限或生成质量。

## 8. 安装的依赖

依赖安装在 GitHub Actions 的临时 Ubuntu 22.04 环境中：
build-essential、cmake、ninja-build、pkg-config、qt6-base-dev、
libgl1-mesa-dev、libsecret-1-dev、gnome-keyring、xvfb、dbus-x11。
未在当前 Windows 主机安装 Qt 或修改 WSL。

## 9. 安全与隐私

仅显式动作上传，固定 HTTPS 官方端点，禁止自动重定向，store=false。
密钥走系统密钥环；普通设置只有模型和语言。错误消息使用固定文案，
不回显原始 API/keyring 错误正文。源码未包含私密内容日志路径。
未实现任何自动发送、监控输入、截屏或剪贴板历史功能。
详见 privacy.md。

## 10. M1 建议与剩余验收

先在 Ubuntu 桌面完成以下 M0 验收：
保存真实 API key → 配置可用模型 → 从外部应用复制文本 → 执行三个操作 →
复制结果 → 检查托盘菜单与关窗驻留，并分别记录 X11/Wayland 表现。

之后按照用户提供的 M1 指令实施：
1. 引入 AT-SPI selection provider/monitor，复用 Selection。
2. 监听选择事件并短时去抖，不进行持续树扫描。
3. 诊断面板显示明确请求的选区信息，日志只记录长度和坐标是否可用。
4. 对 Firefox、Chromium、VS Code、gedit、Terminal、Electron 建兼容矩阵。
5. M1 不实现 M2 自动浮动工具条。

**本次未开始 M1。**
