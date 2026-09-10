# WorkSidekick

A C++20 / Qt 6 desktop assistant for text you explicitly copy. M0 provides
**Plain Speak**, **Summarize**, and **Polish** using the official OpenAI Responses
API. Review the result, then copy it yourself.

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
  qt6-base-dev libgl1-mesa-dev libsecret-1-dev gnome-keyring

git clone https://github.com/wzk111/leadership-dandang-win-linux.git
cd leadership-dandang-win-linux
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./build/worksidekick
```

OpenGL headers are needed by Qt Widgets even though this app does not render 3D.
No AT-SPI, XInput2, input injection, root daemon or global-hook dependencies are
needed for M0. Root is used only for installing system packages.

Optional install into your user account:

```bash
cmake --install build --prefix "$HOME/.local"
"$HOME/.local/bin/worksidekick"
```

For a Wayland-native Qt session, also install `qt6-wayland`. This does not imply
verified GNOME Wayland compatibility; see the acceptance report. Qt may otherwise
use XWayland. Actual platform/session information appears in Diagnostics.

## First use

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
native selection capture is deferred.

## Milestone boundaries

- **M0:** manual clipboard, three actions, settings, secure keys, asynchronous API,
  result/copy/cancel, diagnostics and tray.
- **M1–M3:** AT-SPI detection, automatic popup, X11 fallback and global shortcuts.
- **M4:** reply/profile/features and additional AI providers.
- **M5:** native Windows integrations.

Automatic selection, automatic popup and Ctrl+Alt+P are **not implemented in M0**.
X11 PRIMARY is not read. On either X11 or Wayland, manually copy text first.
No GNOME settings are changed. Windows CMake configurations expose shared library/test targets, but Windows
builds are NOT TESTED. M0 does not produce a native Windows application.

## Tests

On a desktop, the regular CTest command above runs core, networking, UI,
controller and end-to-end tests. All network tests use local synthetic responses.

For a headless Ubuntu machine:

```bash
sudo apt install -y xvfb dbus-x11
xvfb-run -a ctest --test-dir build --output-on-failure
dbus-run-session -- bash scripts/test-keyring.sh
xvfb-run -a ./build/worksidekick --smoke-test
```

The keyring script creates an isolated temporary HOME/keyring and uses synthetic
credentials. It must be run through `dbus-run-session` as shown.
Smoke mode opens the windows using synthetic data, sends nothing and exits.

[Ubuntu CI](https://github.com/wzk111/leadership-dandang-win-linux/actions/workflows/ubuntu.yml)
builds with Ubuntu's system CMake and Qt 6.2, then runs these checks.
See [M0 verification](docs/m0-completion.md) for actual evidence and **NOT TESTED**
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
- [Implementation plan](docs/m0-plan.md)

No project license has been selected yet; no open-source license grant is implied.
