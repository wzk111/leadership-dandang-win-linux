# M6 implementation plan

> For agentic workers: execute using subagent-driven-development, with isolated file ownership for independent packaging and stability tasks and spec/quality review before delivery.

Goal: deliver WorkSidekick 0.7.0-rc1 packages and repeatable stabilization evidence without redesigning M0–M5.

Accepted baseline: `02179d261f4a61f7da5a019f7e05d2dc514e3bf9` (clean main at start).

## Existing architecture before changes

The Qt Widgets application owns shared controller, provider protocols, snapshot UI,
Profile and settings. PlatformFactory supplies native selection, credentials,
window policy, explicit resolver and shortcut adapters. Windows UIA runs in a
bounded disposable helper; Linux AT-SPI uses the existing event-loop session.
Single-instance IPC sends only open/trigger. Credentials live in OS vaults;
Profile lives in plaintext QSettings. No action automatically uploads a selection.

Windows CI builds MSVC/Qt 6.8.3 and publishes developer binaries including tests;
it has no product installer. Ubuntu 22.04 builds distribution Qt 6.2 with native
synthetic X11/AT-SPI/Secret Service gates; it has no DEB. Existing runtime install
rules include only executables and the Linux desktop entry. No release workflow
exists. Native desktop and clean-machine delivery are separate acceptance gates.

## Tasks and verification

- [x] Preserve the supplied specification; verify baseline and inventory architecture.
- [ ] Windows delivery: `packaging/windows/`, `scripts/package-windows.ps1`,
  `.github/workflows/windows.yml`. Stage only app/helper; windeployqt and app-local
  MSVC runtime; Inno Setup per-user installer, stable AppId, optional desktop
  shortcut, no autostart. Smoke extracted/installed app with clean PATH; verify
  upgrade preserves isolated settings; uninstall removes program only.
- [ ] Ubuntu delivery: `packaging/linux/`, `scripts/package-linux.sh`,
  `.github/workflows/ubuntu.yml`. CPack DEB with shlibdeps, desktop/icon/docs,
  explicit plugin dependencies; inspect manifest and install/remove in fresh
  Ubuntu 22.04 container, with user-owned configuration retained.
- [ ] Shared package security: manifest audit rejects test binaries, secrets,
  debug/build/dev files; SHA256SUMS and build metadata identify exact commit and
  tools. Include license/dependency inventory and redistribution guidance.
- [ ] Stability: `tests/TestStability.cpp`, platform stress tests and scripts.
  Exercise 1000 selections, 500 bars, 200 activations, 100 monitor/shortcut
  restarts, palettes, synthetic HTTP cancellations and IPC races. Sample resources
  after warmup and batches; retain actual counts and distinguish mocks/native.
- [ ] Recovery: verify existing UIA timeout/cancel/shutdown/recovery tests and
  AT-SPI loss/stop coverage; only fix reproduced defects with regression tests.
- [ ] Version and UX: set CMake/runtime 0.7.0; reuse normal workspace as GNOME
  tray fallback; concise first-run instructions without automatic requests.
- [ ] Release orchestration: `.github/workflows/release.yml`, explicit manual/tag
  builds and artifact summary only, never automatic GitHub Release publication.
- [ ] Run preserved Windows/Ubuntu CI, download actual artifacts, inspect hashes
  and local package/native tests. Native desktop work uses only synthetic text
  in owned test documents; unavailable environments are NOT TESTED.
- [ ] Spec review then quality review, resolve findings, publish concise completion,
  desktop, stability, packaging and checklist reports. Record exact tested SHA,
  actual CI URLs, artifact paths/checksums, missing evidence and M6 gate. Stop at M6.

## Acceptance boundaries

Only real Windows Notepad/browser use counts toward that desktop gate. Xvfb and
native API fixtures do not establish GNOME or real-application acceptance.
Clean-container install is Level C smoke, not real GNOME. No signing certificate
is configured; no paid API calls, production Release publication or M7 work is
authorized by this plan. Use PARTIAL if required desktop environments remain
unverified, while completing every feasible packaging and CI task.
