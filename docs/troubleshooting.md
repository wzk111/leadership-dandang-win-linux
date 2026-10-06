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

## M2 toolbar does not appear

Automatic selection toolbar defaults OFF. Enable it in Settings (applies immediately)
or the tray's Enable Automatic Toolbar. Then make a new selection; enabling does not
replay a previously cached selection. Dismiss hides this selection only.
Disabling hides the toolbar while leaving M1 diagnostics monitoring active.

Check Diagnostics for Qt platform, capability and overlay status. On xcb, a
running AT-SPI monitor, nonempty selection and valid anchor are required.
No anchor rectangle means text is still cached for M1 local diagnostics, but no
automatic toolbar can be placed. A busy AI request hides the toolbar; finish/cancel
and make a new selection. Stop/unavailable/45-second expiry clears the toolbar.

Native Qt wayland / wayland-egl deliberately disables anchored automatic toolbar
positioning. AT-SPI diagnostics and manual Process Clipboard remain available.
A Wayland desktop running Qt xcb is an XWayland attempt, not verified native
Wayland placement. No compositor hacks or privileged input helpers are used.

## M2 placement or focus differs on your desktop

Include session, Qt backend, raw anchor, requested placement and scaling settings
from Diagnostics. Do not include private text. HiDPI, mixed DPI, fractional scaling,
physical multi-monitor and real GNOME apps are NOT TESTED. Window-manager behavior
can vary; disabling the toolbar preserves manual mode.

Synthetic X11 focus testing additionally requires test-only packages
`libx11-dev openbox xvfb dbus-x11`:

```bash
dbus-run-session -- xvfb-run -a env QT_LINUX_ACCESSIBILITY_ALWAYS_ON=1 bash scripts/test-overlay.sh
```

This launches Openbox inside the isolated Xvfb display and a synthetic accessible
fixture; do not run it directly on your normal desktop DISPLAY. It uses no paid API.

## M3 shortcut and fallback

Enable global shortcut in Settings or the tray. Default is OFF. Use
Register / retry shortcut after cancellation or a failure. This preference is
independent of Automatic selection toolbar.

- **Ctrl+Alt+P already used:** X11 registration fails rather than overriding the
  other application. Release the conflicting desktop binding or use tray / --trigger,
  then retry. Keyboard-map changes during a session may require disable/re-enable.
- **Portal unavailable:** Diagnostics reports the real probe result/version.
  An installed xdg-desktop-portal package does not guarantee GlobalShortcuts.
  Use tray Open Selection Actions or a desktop custom shortcut for --trigger.
- **Portal cancelled / lost:** no automatic permission-dialog retry. Explicitly
  re-enable or use Register / retry shortcut. A saved enabled preference does not
  silently bind on native Wayland startup; explicit retry is required each session.
  Pending configuration is bounded to three minutes.
- **PRIMARY unsupported:** only Qt xcb with supportsSelection participates.
  On native Wayland use AT-SPI or explicitly copy first.
- **Unexpected AT-SPI selection:** cached AT-SPI is preferred until M1 clears or
  expires it. Inspect the source/preview and close without action if it is wrong.
  Stop M1 monitoring to force fallback while troubleshooting; do not upload blindly.
- **Empty text:** select again, or copy first; whitespace and >100,000-character
  input are rejected. Empty retrigger clears the old palette snapshot.
- **AI busy:** no new selection is queued. Finish/cancel then trigger again.
- **Native Wayland automatic popup absent:** still expected. The M3 manual palette
  is a normal compositor-managed window; automatic anchored M2 remains unsupported.

### External trigger / GNOME custom shortcut

Run `/absolute/path/to/worksidekick --trigger`. If absent, it starts the primary
instance and opens selection actions. Otherwise it forwards one local command.
Normal second launch opens the existing workspace.

In GNOME Settings → Keyboard → Custom Shortcuts, add a shortcut whose command is
the absolute executable path followed by --trigger. Labels may vary by desktop.
WorkSidekick does not run gsettings/dconf or change your desktop configuration.
On native Wayland a compositor may place or focus a normal window according to
its own activation policy; real desktop behavior is NOT TESTED.

### Second instance / stale socket

Run both invocations as the same logged-in user with the same XDG_RUNTIME_DIR.
The endpoint is in its worksidekick subdirectory. If another instance is starting
or temporarily unresponsive, retry after it is ready. Never delete a live socket.
A dead process's lock/socket is recovered automatically after checking ownership
and obtaining the primary lifetime lock. Unsafe directory ownership/symlinks or
permission failures are reported without replacing a running instance.
Exit code 2 means local coordination failed. Do not use sudo as a workaround.
`--smoke-test` is an isolated developer mode and intentionally bypasses IPC.
