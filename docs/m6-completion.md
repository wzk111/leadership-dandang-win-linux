# M6 completion

Delivery verification: 2026-11-03.

M6 Gate: **PARTIAL**. Real GNOME acceptance and the complete real Windows
automatic ActionBar → feature-click flow remain unverified. The implementation
stops at M6; no M7 work or GitHub Release publication is included.

Baseline SHA: `02179d261f4a61f7da5a019f7e05d2dc514e3bf9`.
Final implementation/test SHA: `e672f9f3d0486b0627b13e1019ef974fee2cbd2a`.
Final documentation commit: the commit containing this report; its exact SHA is
reported in the delivery message. Package metadata identifies its built source
SHA independently of later documentation edits.

## Build and regression evidence

Final candidate [41647138067](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/41647138067)
passed all jobs at the candidate SHA above. Both builds are Release:

- [Windows CI](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/41647138067/job/125955411083):
  **26/26 CTest passed**, shared stress **19 passed / zero skipped**; ZIP/installer
  creation and full hosted install/reinstall/IPC/uninstall/credential smoke PASS.
- [Ubuntu CI](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/41647138067/job/125955411267):
  **25/25 CTest passed**; preserved Secret Service, AT-SPI, X11/shortcut and native
  listener gates PASS; DEB clean-container install/removal/state probe PASS.
- [Candidate summary](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/41647138067/job/125958974843):
  product ZIP, installer, DEB and Windows metadata/manifest checksums PASS.

Package-specific external probes are not registered as ordinary CTest tests;
they require isolated hosted/container accounts and are never installed.
Two interactive Windows test functions skip in hosted CI; their local opt-in
results are recorded below separately rather than counted as hosted desktop PASS.

| Previous milestone | Regression coverage retained |
|---|---|
| M0 | Core/settings, secure-store adapters, network, UI/controller, end-to-end and launch smoke |
| M1 | Selection pipeline, AT-SPI real event fixture, unavailable registry and secure-store integration |
| M2 | ActionBar placement/policy, X11 focus fixture and Windows native policy |
| M3 | Explicit fallback, manual palette, shortcuts/portal protocol, IPC and trigger forwarding |
| M4 | Four provider protocols, features/Profile/Reply/refinement, privacy and cancellation |
| M5 | Windows UIA helper, native hooks/hotkey/policy/credentials, process deadlines and recovery |

No paid provider request was made. Tests use loopback HTTP, synthetic text and
synthetic credentials. Passing regression fixtures do not establish real GNOME
or all third-party application compatibility.

## Desktop and stability

[Desktop matrix](m6-desktop-compatibility.md) records Windows 11 build 26200,
Notepad 11.2607.14.0 and Chrome 156.0.8078.74. Both show automatic toolbar appearance
without stealing foreground focus and reach Reply through an explicit manual
palette action. Chrome keyboard selection and explicit clipboard fallback pass.
Direct real-app automatic-toolbar feature click remains NOT TESTED because the
automation surface cannot address the nonactivating toolbar. Other listed Windows
applications remain NOT TESTED. GNOME X11 and GNOME Wayland: **NOT TESTED —
environment unavailable**; WSL cannot start with current virtualization support.

Windows DPI: mathematical 100/125/150/200% mapping passes; physical setup has two
100% monitors with a negative-Y secondary. Native policy verifies both monitor
identities and placement. Follow-up physical 100% / 125% mixed-DPI native policy
run passed **11 tests, zero skips**, with negative-Y secondary; original 100/100
was restored and verified. Other physical scale combinations/GNOME fractional
scaling remain NOT TESTED.

Local opt-in Windows tests on application code `dc9b614`: policy **11 passed**,
runtime **3 passed**, stability **5 passed**, all zero skipped. Later changes
affect packaging/test harnesses/docs, not that application implementation.

[Stability report](m6-stability-report.md) records per-provider 1,000 selections,
500 toolbar cycles, 200 manual activations and 100 monitor restarts, plus 100
HTTP cancellations across providers, 100 IPC races/recoveries, 100 native hook and
hotkey lifecycles and ten helper recovery cycles. Passing controlled suites record
zero unexpected crashes and zero unbounded hangs. Forced helper/primary termination
is intentional. Real multi-hour crash/hang rates: NOT TESTED.

Resource samples show stable handles/threads in hook/hotkey cycles and bounded
initial caches in IPC/helper cases; there is no claim of complete leak freedom.
Dedicated COM/D-Bus allocation accounting, real native P95, AT-SPI bus-loss/target-
exit recovery and multi-hour soak remain NOT TESTED. Failed AT-SPI initialization
requires restart. See the report for measurement boundaries and actual values.

## Delivery, privacy and security

Final `e672f9f` Windows ZIP/installer lifecycle and Linux installed-runtime
settings/keyring persistence passed. Windows emits portable LF checksums; the
strict Linux verifier also accepts older CRLF manifests. A synthetic CRLF failure
was reproduced and both corrected LF output and backward-compatible verification
passed locally before the complete candidate workflow succeeded.

Windows installation: per-user registration/Start Menu and clean-environment
launch PASS. Windows uninstallation: full manifest/registration/shortcut removal,
retained settings and four provider vault values PASS. Running-app uninstall was
refused; 20 triggers and a normal second launch left exactly one owned primary.
Ubuntu installation/uninstallation: clean Ubuntu 22.04 apt dependency resolution,
non-root Xvfb launch, all package files removed, real QSettings/Profile/preferences
and four synthetic Secret Service values retained PASS. GNOME is not implied.

## Candidate downloads and checksums

Canonical artifact: [worksidekick-0.7.0-rc1-candidate](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/41647138067/artifacts/13236695205).
Separate [Windows packages](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/41647138067/artifacts/13235657382)
and [Ubuntu package/evidence](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/41647138067/artifacts/13236735571)
are from the same run/source. Artifact retention currently expires 2027-02-01;
GitHub authentication may be required. This is an unsigned candidate, not a
published GitHub Release.

| Product file | SHA-256 |
|---|---|
| `WorkSidekick-0.7.0-rc1-windows-x64.zip` | `624f713949e34d89f1992d06cc781b2cfbba2816dd2762c688c4c8ec6e4074c8` |
| `WorkSidekick-0.7.0-rc1-windows-x64-setup.exe` | `fd8e58e7c2738e1434499b2b8640c9c47aeb329abefa18a3088f860c875242bc` |
| `worksidekick_0.7.0~rc1_amd64.deb` | `650353b77f64c1d3316c7c04669f0b832f5bcbe15dc5476c4c8953cd933e702b` |

Downloaded and rehashed all product files plus the Windows metadata, inventory
and manifest locally. The outer candidate ZIP also matches GitHub's archive
digest `47cc89f24eed7ff701339d79423d7bd2057845888d8df788585db72023cf50f1`.
Local root: `C:\Codex Projects\leadership-dandang-win-linux\dist\m6-0.7.0-rc1\`.
Windows files are in `windows-release-packages/`; DEB in
`ubuntu-release-packages/`. Each directory includes its SHA256SUMS.txt and
runtime/install evidence. The outer download is `dist/worksidekick-0.7.0-rc1-candidate.zip`.

The final extracted Windows product passed its local Windows 11 smoke with clean
PATH/Qt environment and exit 0, no leftover app/helper processes. Its source
metadata is exactly `e672f9f`; audit reports **41 files, zero violations**.
This machine is not a certified clean Windows image. CI-native test artifacts
are deliberately separate and are not the product downloads.

## Remaining delivery boundaries

Windows upgrade is same-version reinstall only. Previous-version upgrade is
NOT TESTED: M5 has no predecessor packaged installer. Clean Windows machine
deployment is NOT TESTED; the hosted runner has developer tools even when its
PATH and Qt environment are cleared. Signing is NOT CONFIGURED.

Privacy regression covers zero requests before explicit action, Profile routing,
source snapshot independence, cancellation, late secret suppression and secret-
free diagnostics. Production keys stay in Credential Manager / Secret Service;
Profile stays plaintext QSettings. Installer removal retains user data by default.
External test-only probes verify adapter reads rather than claiming that the main
executable generated an AI request. No production key or private document is used.

Security findings addressed: missing Qt xcb runtime dependency; product/test
artifact separation; package private-directory audit bypass; pinned installer
download verification; actual Inno/Qt/MSVC toolchain provenance; running-app
installer mutex. Package audit checks known paths, not arbitrary secret contents.
Third-party notices/inventory are included; commercial redistribution and signing
review are pending. Frozen apt graph and byte-for-byte reproducibility: NOT TESTED.

| Severity | Open findings / boundaries |
|---|---|
| P0 | No known reproduced blocker in the verified scope; no global absence claim |
| P1 | No known reproduced blocker in the verified scope; real desktop/clean-machine gates remain open |
| P2 | Acceptance gaps: complete real-app toolbar action, GNOME sessions, other physical scale combinations, predecessor upgrade, multi-hour soak |
| P3 | Signing and commercial redistribution review pending; package icons remain the existing SVG resource |

These acceptance gaps are not invented confirmed product bugs. See
[packaging report](m6-packaging-report.md) and [release checklist](m6-release-checklist.md)
for exact deployment boundaries. Recommended next work is to complete the missing
M6 desktop gates. M7 optional enhancements/production feedback should start only
after the user accepts this delivery.
