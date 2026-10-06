# M3 fallback compatibility

| Application | Session | Qt backend | AT-SPI text | Automatic M2 toolbar | Global shortcut backend | Global shortcut works | PRIMARY works | Clipboard fallback works | Manual palette works | Correct text sent | Notes |
|---|---|---|---|---|---|---|---|---|---|---|---|
| Synthetic fixture / tests | Xvfb + Openbox | xcb | M1 regression | M2 regression | X11 | PASS | PASS | PASS | PASS | Loopback HTTP | Independent test layers |
| Fake portal service | Isolated session D-Bus | No compositor | n/a | n/a | XDG Portal v1 protocol | PASS | n/a | n/a | Separate UI tests | Separate HTTP tests | Not a real portal/backend |
| GNOME Text Editor / gedit | GNOME X11 | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Real desktop unavailable |
| Firefox | GNOME X11 | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Real desktop unavailable |
| Chrome / Chromium | GNOME X11 | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Real desktop unavailable |
| VS Code | GNOME X11 | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Real desktop unavailable |
| GNOME Terminal | GNOME X11 | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Real desktop unavailable |
| Slack / Electron | GNOME X11 | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Real desktop unavailable |
| GNOME Text Editor / gedit | GNOME Wayland | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Real desktop unavailable |
| Firefox | GNOME Wayland | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Real desktop unavailable |
| Chrome / Chromium | GNOME Wayland | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Real desktop unavailable |
| VS Code | GNOME Wayland | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Real desktop unavailable |
| GNOME Terminal | GNOME Wayland | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Real desktop unavailable |
| Slack / Electron | GNOME Wayland | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Real desktop unavailable |

Native Wayland anchored automatic overlay remains unsupported by policy.
XWayland desktop interoperability, HiDPI, mixed DPI and physical multi-monitor
behavior are NOT TESTED. A Qt xcb process uses X11 shortcuts even on a Wayland
desktop; whether that is global across native Wayland applications depends on the
compositor and is NOT TESTED.

Desktop checks should use synthetic text. Record session, Qt backend, registered
shortcut backend and actual trigger description, plus the source label in the
manual palette. Verify no upload occurs until feature click; such a click can
incur API charges on a configured real account.

Evidence: [final implementation CI](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/37469142658), 2026-10-06, commit 35d2ec96da65e5f33ca7b8bcf541fd28c68a870e. All 16 CTest suites and the separate runtime integrations passed.
