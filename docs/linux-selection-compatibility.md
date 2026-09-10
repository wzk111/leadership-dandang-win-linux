# Linux selection compatibility — M1

M0 is accepted by the user. This matrix tracks **AT-SPI alone**, independently
of M0 clipboard mode. No X11 PRIMARY, injected input, global shortcut or toolbar.
Synthetic CI results are recorded in m1-completion.md after verification.

| Environment / Application | Session | Selection event detected | Text retrieved | Text correct | Anchor available | Anchor reasonable | Fallback required | Notes |
|---|---|---|---|---|---|---|---|---|
| Synthetic Qt QTextEdit / Ubuntu 22.04 Xvfb | X11 virtual display | Yes | Yes | Yes | Yes | Positive dimensions; desktop placement NOT TESTED | Not used | Separate-process fixture; CI 34493879850 |
| GNOME Text Editor / gedit | GNOME X11 | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Unknown | No real GNOME session available |
| Firefox | GNOME X11 | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Unknown | No real desktop test |
| Chrome / Chromium | GNOME X11 | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Unknown | No real desktop test |
| VS Code | GNOME X11 | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Unknown | No real desktop test |
| GNOME Terminal | GNOME X11 | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Unknown | No real desktop test |
| Slack / Electron | GNOME X11 | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Unknown | No real desktop test |
| GNOME Text Editor / gedit | GNOME Wayland | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Unknown | No real GNOME session available |
| Firefox | GNOME Wayland | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Unknown | No real desktop test |
| Chrome / Chromium | GNOME Wayland | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Unknown | No real desktop test |
| VS Code | GNOME Wayland | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Unknown | No real desktop test |
| GNOME Terminal | GNOME Wayland | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Unknown | No real desktop test |
| Slack / Electron | GNOME Wayland | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Unknown | No real desktop test |

To test a row: start monitoring in Diagnostics, select known synthetic text in the
application, return to Diagnostics, use Test Selection for metadata and explicitly
Show Last Selection Preview to check correctness. Record raw screen coordinates
and whether they match the source text. Deselect and verify clearing. Do not use
private workplace text for compatibility reports. No AI action is needed.
