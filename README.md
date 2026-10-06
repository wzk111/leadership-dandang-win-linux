# WorkSidekick

A C++20 / Qt 6 desktop assistant for explicitly chosen text. On supported Linux
backends, select text and click **Plain Speak**, **Summarize**, or **Polish** on the
floating toolbar. Manual clipboard mode remains available. Review the result,
then copy it yourself. AI uses the official OpenAI Responses API.

**Target:** Ubuntu 22.04 LTS. Native Windows integration is planned for M5.
This is an independent implementation inspired by
[leadership-dandang](https://github.com/SecondServ/leadership-dandang); its source
code was not copied. The original requirements are preserved in
[PROJECT_SPEC.md](PROJECT_SPEC.md).

## Build on Ubuntu 22.04

Run these commands in an Ubuntu desktop terminal:

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config \
  qt6-base-dev libgl1-mesa-dev libsecret-1-dev libatspi2.0-dev at-spi2-core gnome-keyring libx11-dev libxtst-dev

git clone https://github.com/wzk111/leadership-dandang-win-linux.git
cd leadership-dandang-win-linux
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./build/worksidekick
```

OpenGL headers are needed by Qt Widgets even though this app does not render 3D.
M1 adds libatspi; no XInput2, input injection, root daemon or global-hook dependency
is added. Root is used only for installing system packages.

Optional install into your user account:

```bash
cmake --install build --prefix "$HOME/.local"
"$HOME/.local/bin/worksidekick"
```

For a Wayland-native Qt session, also install `qt6-wayland`. This does not imply
verified GNOME Wayland compatibility; see the acceptance report. Qt may otherwise
use XWayland. Actual platform/session information appears in Diagnostics.

## M1: local selection diagnostics

M0 manual clipboard AI mode remains available and is accepted. M1 adds AT-SPI2
selection detection under Linux, using only accessibility selection events.
**Selecting text never calls AI. M2 can show the local toolbar when enabled.**

Open Diagnostics, select text in another accessible application, then use
**Test Selection** to inspect event count, application, character count and raw
screen rectangle. **Show Last Selection Preview** explicitly reveals a local
snapshot, never uploaded or copied automatically. Preview is hidden by default.
Stop monitoring clears the cache; latest text also expires after 45 seconds.

Support depends on each application's accessibility implementation under both
X11 and Wayland. See [M1 acceptance](docs/m1-completion.md) and the
[compatibility matrix](docs/linux-selection-compatibility.md). M2 overlay evidence is recorded separately.

## M2: selection → ActionBar → explicit AI action

Configure your model and secure API key using the first-use steps below. In
Settings, enable **Automatic selection toolbar**, or check **Enable Automatic
Toolbar** in the tray menu. The preference applies immediately and defaults **OFF**.

On Qt **xcb** (X11, or an XWayland attempt), select text in an accessible application.
A compact toolbar appears above the selection, or below when needed, and stays
within the selected screen's available area. Click **Plain Speak**, **Summarize** or
**Polish** to process that exact captured selection. No clipboard copy is required.
Only **Copy result** replaces clipboard contents.

**Displaying the toolbar never sends an AI request.** A newer selection replaces
the snapshot; clear/expiry, dismiss, monitoring failure, disabling, or an in-flight
AI request hides the bar. Enabling waits for a new selection. Dismiss does not
disable future popups.

On native Qt **wayland / wayland-egl**, anchored automatic popup is disabled.
AT-SPI diagnostics continue; use the M3 manual palette or **Process Clipboard**. Qt backend and
desktop session are reported separately in Diagnostics. M3 adds a manual shortcut/fallback path described below; no compositor workaround or input injection is used.

Missing anchor geometry keeps the M1 local selection available but prevents
automatic placement. HiDPI, fractional/mixed scaling, real multi-monitor behavior
and real GNOME app compatibility are **NOT TESTED**.
See [M2 verification](docs/m2-completion.md) and
[overlay compatibility](docs/m2-overlay-compatibility.md).

## M3: manual selection actions and global shortcut

Three entry modes are available:

1. **Automatic:** enable Automatic selection toolbar, select text, then click the
   M2 ActionBar. Requires supported Qt xcb overlay and AT-SPI geometry.
2. **Global shortcut:** enable **Enable global shortcut** in Settings or the tray,
   then select/copy text and press the registered shortcut (preferred **Ctrl+Alt+P**).
3. **External / desktop custom shortcut:** execute
   `/absolute/path/to/worksidekick --trigger`. GNOME users can assign this command
   in Settings → Keyboard → Custom Shortcuts. No desktop settings are changed automatically.

The tray's **Open Selection Actions** uses the same manual path and works even when
shortcut registration is unavailable.

The manual resolver tries **cached AT-SPI → X11 PRIMARY → Clipboard**. PRIMARY is
read-only and only available on Qt xcb when supported. Native Wayland skips PRIMARY.
The normal, focusable palette shows the source and a local preview; check it for
stale clipboard content before clicking a feature. The full snapshot is preserved
even when the preview is shortened. Shortcut/trigger alone sends **no AI request**.

Shortcut preference defaults **OFF**, independently of the automatic toolbar.
X11 registers Ctrl+Alt+P with Caps/Num Lock handling; conflicts fail without taking
another application's binding. Native Wayland probes the GlobalShortcuts portal
and shows its actual returned binding. Portal configuration requires an explicit
enable or **Register / retry shortcut** action, including after restarting with a
saved enabled preference. No repeated permission dialogs are opened automatically.

If the portal is unavailable, tray and --trigger remain available. Real GNOME portal,
native Wayland focus behavior and XWayland cross-application coverage are **NOT TESTED**.
The manual palette does not require arbitrary absolute positioning.

Only one resident instance runs per user runtime directory. A second --trigger
forwards one command and exits; normal second launch opens the existing workspace.
With no instance, --trigger starts one and opens the palette. No text travels over IPC.
See [M3 verification](docs/m3-completion.md) and
[fallback compatibility](docs/m3-fallback-compatibility.md).

## First use / manual clipboard mode

1. Launch the app and open **Settings**.
2. Enter a Responses-compatible model ID available to your OpenAI API project.
   There is intentionally no permanent hard-coded model.
3. Set the output language and click **Save settings**.
4. Enter your API key and click **Save API key securely**. The key is stored by
   libsecret in the desktop's Secret Service (normally GNOME Keyring).
5. Copy text in another application using Ctrl+C.
6. In the tray menu or main window, click **Process Clipboard**.
7. Review the captured preview and choose **Plain Speak**, **Summarize**, or **Polish**.
8. Wait for the result, then click **Copy result**.

Saving settings does not save the key; the secure-key button does that separately.
A keyring unlock dialog may appear. Keys are never read back into the settings
field. **Delete saved key** removes this app's stored OpenAI credential.

Loading a preview makes **no API call**. Only choosing a feature sends that
preview. Changing the clipboard afterward does not change an in-flight request.
Cancel or close the result card to cancel. Copying a result deliberately replaces
the current clipboard; there is no clipboard history or simulated Ctrl+C.

An OpenAI API account with API access/billing is required. No key is bundled.
The network timeout is 30 seconds; failures appear in the result card.
The model's availability and generated output must be validated with your account.

## Tray and diagnostics

When a system tray is detected, closing ordinary windows keeps the app running.
Use **Quit** to exit. Without a tray, the main window remains available and closing
the last window exits, so the app cannot become an invisible orphan.

Diagnostics reports OS, desktop, session, Qt platform, M0 limitations and the last
secure-store status. **Test Clipboard** reports only character count.
**Test AI Connection** asks before sending a synthetic message (API charges may
apply); it does not read or send your clipboard. **Test Selection** explains that
native AT-SPI metadata is shown in the selection panel.

## Milestone boundaries

- **M0:** manual clipboard, three actions, settings, secure keys, asynchronous API,
  result/copy/cancel, diagnostics and tray.
- **M1:** local AT-SPI detection and explicit diagnostic preview.
- **M2:** non-activating selection ActionBar on supported backends.
- **M3:** explicit AT-SPI / PRIMARY / clipboard fallback, global shortcut and --trigger.
- **M4:** reply/profile/features and additional AI providers.
- **M5:** native Windows integrations.

M2 automatic popup and M3 manual actions coexist. PRIMARY is read only after explicit trigger on Qt xcb. Manual clipboard mode remains available on either X11 or Wayland.
No GNOME settings are changed. Windows CMake configurations expose shared library/test targets, but Windows
builds are NOT TESTED. M0 does not produce a native Windows application.

## Tests

On a desktop, the regular CTest command above runs core, networking, UI,
controller and end-to-end tests. All network tests use local synthetic responses.

For a headless Ubuntu machine:

```bash
sudo apt install -y xvfb dbus-x11 libx11-dev libxtst-dev openbox
xvfb-run -a ctest --test-dir build --output-on-failure
dbus-run-session -- bash scripts/test-keyring.sh
xvfb-run -a ./build/worksidekick --smoke-test
dbus-run-session -- xvfb-run -a env QT_LINUX_ACCESSIBILITY_ALWAYS_ON=1 ./build/test_atspi_runtime realEvents
env AT_SPI_BUS_ADDRESS=unix:path=/nonexistent/worksidekick-test-bus ./build/test_atspi_runtime unavailableRegistry
dbus-run-session -- xvfb-run -a env QT_LINUX_ACCESSIBILITY_ALWAYS_ON=1 bash scripts/test-overlay.sh
```

The keyring script creates an isolated temporary HOME/keyring and uses synthetic
credentials. It must be run through `dbus-run-session` as shown.
Smoke mode opens the windows using synthetic data, sends nothing and exits.

[Ubuntu CI](https://github.com/wzk111/leadership-dandang-win-linux/actions/workflows/ubuntu.yml)
builds with Ubuntu's system CMake and Qt 6.2, then runs these checks.
See [M3 verification](docs/m3-completion.md), [M2 verification](docs/m2-completion.md), [M1 verification](docs/m1-completion.md) and [M0 verification](docs/m0-completion.md) for evidence and **NOT TESTED**
items. CI is not a substitute for a GNOME desktop and a live API account.

## Privacy and documentation

No selected text, generated content, profile data or API keys are logged or
persisted by the application. Requests use HTTPS, do not follow redirects and set
`store: false`. OpenAI's own data policies still apply. M0 never sends messages,
scrapes apps, records clipboard history or monitors input.

- [Architecture](docs/architecture.md)
- [Privacy](docs/privacy.md)
- [Troubleshooting](docs/troubleshooting.md)
- [Linux compatibility](docs/linux-selection-compatibility.md)
- [Windows compatibility](docs/windows-selection-compatibility.md)
- [M3 specification](docs/M3_SPEC.md)
- [M3 implementation plan](docs/m3-plan.md)

No project license has been selected yet; no open-source license grant is implied.
