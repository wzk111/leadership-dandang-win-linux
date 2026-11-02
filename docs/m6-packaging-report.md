# M6 packaging report

Candidate: `0.7.0-rc1`, unsigned. This document distinguishes actual packages from
the separate developer/native-test bundle. See completion report for final CI SHA.

## Ubuntu DEB — verified delivery smoke

Ubuntu 22.04 amd64, Qt 6.2.4, CPack DEB / dpkg-shlibdeps. Explicit dependencies
include `qt6-qpa-plugins`, `qt6-wayland`, AT-SPI service and CA certificates in
addition to linked-library dependencies. No system libraries or privileged agent
are bundled. Installed paths: `/usr/bin/worksidekick`, desktop entry, hicolor SVG
icon, README/privacy/troubleshooting/third-party notices and build metadata.

Verified commands in CI:

```bash
bash scripts/package-linux.sh build dist/linux
bash scripts/test-linux-package.sh 'dist/linux/worksidekick_0.7.0~rc1_amd64.deb' dist/linux
```

The smoke uses the official Ubuntu 22.04 container pinned by manifest digest
`sha256:076e7bf82319619f28dd0aa9c02ab34b055ad37100395b4f49a2ffba04fb5e11`.
It runs apt installation with `--no-install-recommends`, verifies xcb/Wayland
plugins, `ldd`, desktop syntax and icon, launches the installed binary as an
ordinary user under Xvfb/D-Bus, removes the package and checks every package file
is gone while synthetic home settings/data survive. It does not run a GNOME
session or real provider request.

The minimal container has dpkg documentation exclusions. The harness logs those
rules and adds a path-include exception only for WorkSidekick docs before install;
the product package is unchanged. This fixed an actual clean-container failure.
The first clean install also exposed the missing xcb plugin dependency, which was
corrected rather than hidden by installing development packages.

Interactive desktop commands corresponding to the verified apt workflow:

```bash
sudo apt install './worksidekick_0.7.0~rc1_amd64.deb'
/usr/bin/worksidekick
sudo apt remove worksidekick
```

Only installation/removal needs root. Launch the app as your desktop user.
Previous-version upgrade, real packaged Secret Service access, GNOME launcher
activation/tray and native Wayland launch remain NOT TESTED.

## Windows ZIP and installer

`scripts/package-windows.ps1` rebuilds the current Release source, checks MSVC
x64 provenance, stages only app/helper, runs matching Qt 6.8.3 windeployqt and
copies app-local Microsoft redistributable CRT DLLs. No elevated CRT installer
is needed. Qt license texts, DLL inventory, source/tool metadata and an SVG icon
are included; tests, headers, build directories and diagnostics are excluded.

Inno Setup 6.7.0 comes from the official release and is SHA-256 verified before
execution. `CompilerProbe.iss` verifies the actual compiler preprocessor version.
PE version resources and the `/ ?` help text are not treated as version APIs.
The installer uses a stable AppId, current-user default directory and Start Menu
shortcut; desktop shortcut is optional, autostart absent. AppMutex prevents
replacement/uninstallation while a primary is running. Normal uninstall keeps
QSettings and the four Credential Manager entries.

Hosted-runner package smoke verifies extracted/installed launch with clean PATH
and Qt environment, helper startup, installed IPC forwarding, running-app uninstall
refusal, same-version reinstall, full-manifest removal and distinct synthetic
settings/vault-value retention. It refuses existing user data and only runs in a
disposable hosted account. It does not claim the packaged app itself read keys.

Windows package creation/installation final evidence: **NOT TESTED — pending the
currently running corrected CI**. This status must be updated from actual artifacts
before delivery. Previous-version upgrade and clean Windows machine remain
NOT TESTED; M5 supplied no predecessor installer. Code signing NOT CONFIGURED.

## Provenance, security and licenses

Product artifacts carry SHA256SUMS.txt and file manifests. The release workflow
resolves one immutable commit for both platforms and only uploads candidate
artifacts; it does not publish a GitHub Release. Qt patch versions and Inno are
pinned; compiler/runner/system package resolutions are recorded. Ubuntu apt is not
a frozen repository snapshot. Byte-for-byte reproducibility is NOT TESTED.

The package audit rejects obvious secret/development/fixture/private-directory
paths and is regression-tested. This is not a general secret-content certification.
Profile remains plaintext local QSettings; no credential migration is introduced.
See [third-party inventory](../packaging/THIRD_PARTY.md) and bundled Qt notices.
The Inno compiler identifies its tool license as non-commercial; review the
upstream license/tool-use terms before commercial production distribution.
No signing credentials are generated or purchased automatically.
