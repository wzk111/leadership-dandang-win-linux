# Troubleshooting

## CMake cannot find Qt / WrapOpenGL

Install qt6-base-dev and libgl1-mesa-dev. M0 supports the Ubuntu 22.04 package
Qt 6.2.4. Use a clean build directory after changing toolchains.

## No tray icon

Some GNOME configurations have no enabled tray extension. Use the main window
and Quit button. Diagnostics reports whether Qt detected a tray. Installing or
enabling a tray extension is a desktop choice; M0 does not alter GNOME settings.

## Secret store unavailable or locked

Run inside your normal logged-in desktop session with its session D-Bus. Ensure
gnome-keyring is installed and unlock the login keyring using your desktop's
Passwords and Keys application. No plaintext fallback is used.
Do not run WorkSidekick with sudo. A bare SSH/CI session may lack a Secret Service.
Secret access runs off the UI thread; an unlock prompt may keep it pending.

## No copied text / old text shown

Select text and explicitly press Ctrl+C, then Process Clipboard. This command
intentionally consumes current clipboard text; it cannot infer whether that text
is a fresh selection. It does not automatically monitor, simulate copy or read
X11 PRIMARY. Empty capture clears the old preview. Choose Process Clipboard again
to refresh a nonempty preview.

## API key missing / authentication / rate limits

Save an API key using the dedicated secure-key button. Check that your API project
can access the configured model. 401/403 and 429 are shown separately; 429 may
also indicate quota or billing. There are no automatic retries or extra charges
from retry loops. Test AI Connection sends a synthetic message only after confirmation.

## Network / JSON / incomplete response

Check HTTPS connectivity to api.openai.com. Requests expire after 30 seconds.
Incomplete, empty or malformed responses produce errors instead of stale results.
Generic rejected-request errors can include an unsupported model. Correct the
model in Settings and try again. Raw error bodies are deliberately not displayed.

## Wayland

Install qt6-wayland to use Qt's native Wayland plugin. Otherwise Qt may use
XWayland. M0 uses manual clipboard mode and has no global shortcut or automatic
popup. Actual GNOME Wayland desktop behavior is NOT TESTED in the Windows-hosted
development environment. No uinput/ydotool/root workaround is installed.

## Windows development host

The current host's WSL2 Ubuntu cannot start because virtualization is unavailable.
The repository is built and tested in Ubuntu 22.04 CI. No BIOS, optional Windows
features or existing WSL distributions were changed. Windows desktop functionality
remains M5.

When reporting a problem, include OS, session type, Qt platform, model availability
and sanitized error category. Do not attach private clipboard text, keys, raw API
responses or your settings/credential dumps.

## M1 AT-SPI diagnostics

Install libatspi2.0-dev for building and at-spi2-core for the runtime bus. Run as your desktop user, with the normal
session D-Bus. Do not run with sudo. If Diagnostics reports registry unavailable,
check that the desktop accessibility bus is running; restart WorkSidekick after
restoring the bus. Clipboard AI remains usable. Stop/Start monitoring can retry
listener registration when the initialized registry is available.

Listener registration failure is shown separately from text retrieval failure.
Some apps/controls have no Text interface or report selection count zero. Select
text in a known accessible editor, wait for the 80 ms debounce, then Test Selection.
The button does not fall back to clipboard. Text retrieval errors may indicate a
closed/unresponsive source or inaccessible control. A valid text result with no
geometry is still success; geometry support varies by app and compositor.

Show Last Selection Preview is explicit and local. A new selection, monitor stop,
45-second expiry or hiding Diagnostics clears the visible preview. WorkSidekick's
own controls and password fields are ignored. Unsupported apps need a later
fallback; do not disable Wayland or install input injection tools for M1.

The reproducible synthetic integration command (not a real desktop test) is:

```bash
dbus-run-session -- xvfb-run -a env QT_LINUX_ACCESSIBILITY_ALWAYS_ON=1 ./build/test_atspi_runtime realEvents
```

This starts a separate synthetic Qt text editor. It requires the development/test
build and desktop accessibility bus packages. It performs no external API calls.
