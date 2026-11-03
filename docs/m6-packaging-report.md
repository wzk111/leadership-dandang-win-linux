# M6 packaging report

Candidate: `0.7.0-rc1`, unsigned. This document distinguishes actual packages from
the separate developer/native-test bundle. Final source is `e672f9f`, with all
jobs passing in [candidate run 41647138067](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/41647138067).
See [completion report](m6-completion.md) for exact artifact links, hashes and
locally verified downloads. Both final platform package smokes passed.

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
Previous-version upgrade, GNOME launcher activation/tray and native Wayland launch
remain NOT TESTED.

The latest harness additionally mounts an external test-only state probe, never
included in the DEB. It uses the installed runtime and a normal isolated account
with a controlled Secret Service, checks production QSettings/Profile/preferences,
reads four distinct provider adapter values and verifies provider deletion
isolation. It restarts the isolated keyring on a new bus after removal and checks
the same settings and secrets again. Existing settings are rejected before any
seed write, and the harness verifies the refusal preserves their bytes. This is
adapter evidence, not a main-executable AI request. These checks passed in
[Ubuntu run 41644121055](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/41644121055)
at `f98a8fd`; the final candidate run is recorded in the completion report.

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

The external test-only probe reads production QSettings and four distinct native
provider adapters with installed Qt DLLs. No test/probe is installed. A preliminary
Windows lifecycle run correctly refused an existing registry key created by the
earlier developer startup smoke. CI now performs product account verification
before that smoke; the refusal guard was preserved rather than deleting unknown
data. Product ZIP creation and local Windows 11 clean-PATH startup already passed
at `e578af6`; final install/reinstall/uninstall results follow below.

Windows product creation and complete hosted-account lifecycle passed in
[package retest 41646289012](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/41646289012),
with product/probe artifacts from source `d3e7376` and corrected harness `d6254be`.
The manual `windows-package-retest.yml` workflow downloads only this repository's
artifacts into a fresh account and copies only the probe EXE into an empty folder,
so developer DLLs cannot shadow installed Qt. It records both provenances and
does not relabel the result as a new candidate build. The final same-source
candidate build is linked in the completion report.

The test caught a fixture error: repeated `New-Item -Force` on the same registry
group erased earlier fields. A temporary unique-key reproduction confirmed the
behavior; seeding now creates missing groups only and checks all values before
the first product launch. Both fixes retain the existing-data refusal guards.

Previous-version upgrade and clean Windows machine remain NOT TESTED; M5 supplied
no predecessor installer. Code signing NOT CONFIGURED.

## Provenance, security and licenses

Product artifacts carry SHA256SUMS.txt and file manifests. The release workflow
resolves one immutable commit for both platforms and only uploads candidate
artifacts; it does not publish a GitHub Release. Qt patch versions and Inno are
pinned; compiler/runner/system package resolutions are recorded. Ubuntu apt is not
a frozen repository snapshot. Byte-for-byte reproducibility is NOT TESTED.
Windows checksum files explicitly use LF; the strict Linux aggregator also
accepts older CRLF manifests. All final product and Windows metadata checksums
passed in CI and after download. The final Windows portable package also passed
local Windows 11 clean-environment smoke, with no remaining app/helper processes.

The package audit rejects obvious secret/development/fixture/private-directory
paths and is regression-tested. This is not a general secret-content certification.
Profile remains plaintext local QSettings; no credential migration is introduced.
See [third-party inventory](../packaging/THIRD_PARTY.md) and bundled Qt notices.
The Inno compiler identifies its tool license as non-commercial; review the
upstream license/tool-use terms before commercial production distribution.
No signing credentials are generated or purchased automatically.
