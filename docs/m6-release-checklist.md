# M6 release checklist

This is an evidence checklist, not authorization to publish a production release.

- [x] M5 baseline locked; supplied M6 specification preserved.
- [x] Application 0.7.0 and candidate package naming implemented.
- [x] Shared four-provider privacy/cancellation/shutdown stress passing in CI.
- [x] Native Windows hook/hotkey/helper recovery tests passing.
- [x] Linux regression, Secret Service, AT-SPI, X11 and portal fake-service gates preserved.
- [x] DEB generated; dependencies/manifest inspected; fresh-container install/non-root launch/removal passed.
- [x] Native Windows local focus/runtime/lifecycle tests passed, zero skipped.
- [x] Real Notepad/Chrome selection, automatic toolbar appearance and manual Reply paths observed.
- [x] Real Chrome keyboard UIA and explicit clipboard fallback observed.
- [x] Physical two-monitor 100% setup including negative Y validated with native policy tests.
- [x] Windows product ZIP/installer generation and fresh-account lifecycle retest passed (artifact source `d3e7376`, harness `d6254be`).
- [x] Final `e672f9f` same-source Windows/Ubuntu build, product package lifecycle and post-download SHA-256 verification all passed.
- [x] Same-source candidate release workflow passed end to end, including strict SHA-256 verification (run 41647138067, `e672f9f`).
- [ ] Complete real-app automatic-toolbar feature-click acceptance pending (test-tool window boundary).
- [ ] Clean Windows machine deployment NOT TESTED — environment unavailable.
- [ ] Previous packaged-version upgrade NOT TESTED — no predecessor installer.
- [x] Linux installed-runtime production adapters read four distinct provider values; deletion isolation and post-uninstall retention passed.
- [x] Windows installed-runtime production adapters read four distinct provider values; settings/vault retention and program removal passed.
- [ ] Main-executable AI credential use NOT TESTED; external adapter probes do not generate an AI request from the main app.
- [ ] GNOME X11/Wayland acceptance NOT TESTED — environment unavailable.
- [x] Physical primary 100% / secondary 125% native policy/placement passed 11 tests with zero skips; original 100/100 restored.
- [ ] Other physical scale combinations and GNOME fractional scaling NOT TESTED.
- [ ] Native AT-SPI bus-loss/target-exit recovery NOT TESTED; restart limitation documented.
- [ ] Multi-hour soak, complete COM/D-Bus allocation accounting and real native P95 latency NOT TESTED.
- [ ] Fully frozen system dependency graph / identical output bytes NOT TESTED.
- [ ] Code signing NOT CONFIGURED; commercial redistribution/license review pending.
- [ ] GitHub Release publication NOT AUTHORIZED and not performed.
- [x] M7 has not started.

Required final gate is PARTIAL while required real-desktop/product-delivery evidence
above remains outstanding. Do not relabel synthetic tests or container smoke as
real desktop acceptance. See completion, desktop, stability and packaging reports.
