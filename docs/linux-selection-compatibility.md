# Linux selection compatibility

M0 implements manual clipboard only. AT-SPI selection retrieval is M1;
automatic popup is M2. No native application compatibility is claimed yet.

| Environment / application | Selection event | Text / rectangle | Automatic popup | Manual clipboard |
|---|---|---|---|---|
| Ubuntu 22.04 Xvfb, Qt test widgets | Not implemented | Not implemented | Not implemented | Automated test coverage |
| GNOME X11 | NOT TESTED | NOT TESTED | Not implemented | NOT TESTED on real desktop |
| GNOME Wayland | NOT TESTED | NOT TESTED | Not implemented | NOT TESTED on real desktop |
| Firefox | NOT TESTED | NOT TESTED | Not implemented | NOT TESTED |
| Chrome / Chromium | NOT TESTED | NOT TESTED | Not implemented | NOT TESTED |
| VS Code | NOT TESTED | NOT TESTED | Not implemented | NOT TESTED |
| gedit / GNOME Text Editor | NOT TESTED | NOT TESTED | Not implemented | NOT TESTED |
| Terminal | NOT TESTED | NOT TESTED | Not implemented | NOT TESTED |
| Slack / Electron | NOT TESTED | NOT TESTED | Not implemented | NOT TESTED |

M1 should record event detected, selected text retrieved, anchor available and
fallback required for each application, separately on GNOME X11 and Wayland.
