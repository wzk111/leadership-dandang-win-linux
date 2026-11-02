# WorkSidekick 0.7.0-rc1 for Ubuntu 22.04 amd64

This is an unsigned release candidate. Install from the downloaded folder with
`sudo apt install './worksidekick_0.7.0~rc1_amd64.deb'`; run `worksidekick` as your
ordinary desktop user, or open the WorkSidekick desktop launcher.

In Settings choose a provider, save its key to Secret Service and enter a model
ID. Copy text and click Process Clipboard, then choose an action. Review output
before Copy. Selecting text alone never sends a request. Automatic toolbar and
Ctrl+Alt+P are optional and default OFF. Reply opens a local preview first.

On GNOME without a tray, keep the normal workspace open. On native Wayland use
the manual palette/clipboard or `/usr/bin/worksidekick --trigger`; anchored
automatic placement is unsupported. A missing GlobalShortcuts portal does not
prevent the external trigger. Explicit retry is needed for portal registration;
there is no automatic permission dialog at startup.

Keys require an unlocked Secret Service/keyring. There is no plaintext fallback.
Profile is local plaintext QSettings. See [privacy](privacy.md) and
[troubleshooting](troubleshooting.md) included beside this file.

Quit before update or removal. `sudo apt remove worksidekick` removes the program
and desktop resources, retaining your settings/keyring entries. For complete
removal, delete each provider key in Settings and Clear Profile before quitting;
see privacy.md for exact WorkSidekick-owned data locations. No service, autostart
entry or privileged runtime agent is installed.

Clean-container installation smoke is distinct from real GNOME X11/Wayland
acceptance. Current evidence and limitations: [project documentation](https://github.com/wzk111/leadership-dandang-win-linux/tree/main/docs).
Third-party inventory: [THIRD_PARTY.md](THIRD_PARTY.md).
