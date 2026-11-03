# M6 desktop compatibility

Date: 2026-11-02. Evidence levels follow [M6 specification](M6_SPEC.md).
These are controlled synthetic documents in real applications, not workplace data.

## Windows 11 native session (Level B)

Windows 11 Home build 26200; AMD Ryzen 5 5600, 12 logical processors; Qt 6.8.3,
Windows QPA. Desktop executable: native validation artifact from `dc9b614` /
[Windows run 41452656064](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/41452656064).
Later packaging fixes do not change application/selection code. This is not an
installer or clean-machine test.

| Application/version | Selection | Automatic ActionBar | Focus on appearance | Shortcut | Clipboard fallback |
|---|---|---|---|---|---|
| Notepad 11.2607.14.0 | PASS: 119 Unicode/whitespace characters, exact fixture | PASS: observed near selected line | PASS: GetForegroundWindow remained `6490626` | PASS: fresh UIA 119 chars, Reply preview | NOT TESTED in Notepad |
| Chrome 156.0.8078.74 | PASS: paragraph 38 chars including range whitespace; double-click 9 chars | PASS: above selected paragraph | PASS: GetForegroundWindow remained browser handle `16648568`; repeated run `132952` | PASS: UIA keyboard selection and manual Reply preview | PASS: selected text cleared, Source Clipboard, 83 chars |
| Edge | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED |
| VS Code | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED |
| Word | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED |
| Outlook | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED |
| Teams | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED |

Chrome textarea keyboard selection: Shift+Right → 1 character; Ctrl+Shift+Right
→ 13; Ctrl+A → 83. Ctrl+Alt+P resolved the 1-character selection through UIA.
Automatic keyboard selection detection is **not implemented or claimed**.

Both applications reached the local Reply Composer through an explicit manual
palette action. No Generate Reply button or paid API was invoked. A 1-character
Reply snapshot stayed unchanged after copying a different 83-character selection.
Clipboard fallback required an explicit shortcut after clearing the source selection.
Repeated browser test windows were closed/reopened between controlled sessions.
The two feature preferences temporarily enabled for testing were restored OFF.

Direct automatic-toolbar feature click in these real applications is **NOT TESTED**:
the desktop automation inventory excludes the nonactivating tool window, and
rejects a click addressed to the source browser when that point belongs to the
toolbar. This is a test-tool boundary, not an observed product failure. Native
fixture feature-click/focus tests separately pass; those do not replace this Level B
gate. Therefore the complete required Windows ActionBar → action gate remains pending.

## Display and focus evidence

Mathematical 100/125/150/200 percent and negative-origin mapping is Level A.
Current native policy test verifies actual PMv2 awareness, placement on every
connected QScreen, and source focus/selection across show, hide and feature click
in an owned RichEdit fixture. Local M6 policy result: 11 PASS, zero skips.
Physical display metadata before testing (both effective DPI 96 = 100%): primary
`(0,0) 1920×1080`, work area `1920×1032`; secondary portrait
`(1920,-371) 1080×1920`, work area `1080×1872`. The native policy placement test
passes for both actual HMONITOR/QScreen identities, including negative Y.
Reply Composer activation is intentional and is separate from toolbar focus.

Follow-up on 2026-11-03: temporarily set the portrait secondary to **125%** using
Windows Display Settings while keeping the primary at 100%. Native
`GetScaleFactorForMonitor` confirmed 100/125, with physical secondary geometry
`(1920,-371) 1080×1920` and work area `1080×1860`. The same native policy suite
passed **11 tests, zero failures/skips**, including actual HMONITOR/QScreen
placement on both screens, PMv2 and the owned fixture focus/click scenario.
This is physical mixed-DPI native-policy evidence, not a Notepad/browser flow
on every screen. Raw result: `.local/m6-native-local/policy-mixed125.out`.
The secondary was restored to **100%**, verified through Settings and the native
scale API; its work area returned to `1080×1872`. Settings was closed afterward.
Physical 150/200% and additional mixed-DPI configurations remain NOT TESTED.

## Ubuntu GNOME sessions

| Session/application group | Result |
|---|---|
| Real GNOME X11: text editor, Firefox, Chromium, VS Code, terminal, Electron | NOT TESTED — environment unavailable |
| Real GNOME Wayland: same application group/manual/portal/clipboard | NOT TESTED — environment unavailable |
| Xvfb/Openbox synthetic AT-SPI, X11 focus, shortcut and lifecycle tests | Level A only; see CI/stability report |
| Fresh Ubuntu 22.04 container DEB launch/removal | Limited Level C smoke; not GNOME |

Installed WSL Ubuntu cannot start because virtualization is unavailable
(`HCS_E_HYPERV_NOT_INSTALLED`). No system virtualization setting was changed.
Native Wayland absolute automatic toolbar remains unsupported. Manual palette,
clipboard and `/usr/bin/worksidekick --trigger` are the fallback; real desktop
portal availability and activation behavior remain unverified. A missing
GlobalShortcuts portal is graceful degradation, not a claimed portal PASS.
