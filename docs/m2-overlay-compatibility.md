# M2 overlay compatibility

Capability selection uses the Qt backend, not only XDG_SESSION_TYPE. Qt xcb attempts
anchored non-activating overlays; native wayland* disables them. This is capability
policy, not evidence that every application on that desktop works.

| Application | Session | Qt backend | AT-SPI selection | Anchor | ActionBar shown | Placement reasonable | Source focus preserved | Button clickable | Correct text sent | Notes |
|---|---|---|---|---|---|---|---|---|---|---|
| Synthetic Qt QTextEdit | Xvfb + Openbox | xcb | PASS | PASS | PASS | PASS: anchor calculation + screen bounds | PASS: XGetInputFocus before/after show | PASS: QTest on own button | PASS in separate loopback HTTP test | CI 37463066504; real GNOME and physical mouse path NOT TESTED |
| GNOME Text Editor / gedit | GNOME X11 | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Real desktop unavailable |
| Firefox | GNOME X11 | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Real desktop unavailable |
| Chrome / Chromium | GNOME X11 | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Real desktop unavailable |
| VS Code | GNOME X11 | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Real desktop unavailable |
| GNOME Terminal | GNOME X11 | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Real desktop unavailable |
| Slack / Electron | GNOME X11 | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Real desktop unavailable |
| GNOME Text Editor / gedit | GNOME Wayland | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Real desktop unavailable |
| Firefox | GNOME Wayland | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Real desktop unavailable |
| Chrome / Chromium | GNOME Wayland | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Real desktop unavailable |
| VS Code | GNOME Wayland | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Real desktop unavailable |
| GNOME Terminal | GNOME Wayland | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Real desktop unavailable |
| Slack / Electron | GNOME Wayland | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Real desktop unavailable |

Real XWayland behavior, HiDPI, mixed/fractional scaling, physical multi-monitor
placement and long-duration resource profiling are NOT TESTED.
Pure placement tests include a negative-coordinate virtual screen; this is not a
claim of physical multi-monitor validation.

For desktop verification, use synthetic nonprivate text, enable Automatic selection
toolbar, and record both session and Qt backend in Diagnostics. Check that the
source remains focused when the bar appears, placement fits the selected screen,
and each feature uses the displayed selection snapshot rather than clipboard text.
AI clicks use your configured API and may incur charges. On native Wayland, verify
that no anchored popup appears and manual clipboard mode remains usable.
