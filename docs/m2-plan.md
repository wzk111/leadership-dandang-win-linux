# M2 implementation plan

Baseline: 5f9a65cca55c4f1f1a6b6b9c1d7ed00f4db32316. User approved M2_SPEC.md.
Execution: inline with TDD and completion verification. No M3 work.

Architecture: one reusable ActionBar owns a value Selection; Application wires events and explicit clicks.
A small IPlatformWindowPolicy configures Qt window flags and returns placement metadata.
Linux xcb supports anchored positioning; native Wayland and other backends disable automatic overlays.
Automatic popup defaults OFF on every backend; Settings and tray apply immediately and persist independently of model/key configuration.

- [x] Write placement and snapshot widget tests, build with minimal stubs, observe failing assertions in Ubuntu CI.
  Files: src/ui/ActionBarPlacement.{h,cpp}, ActionBar.{h,cpp}, tests/TestActionBar.cpp, CMakeLists.txt.
  Placement contract: optional<BarPlacement> placeActionBar(QRect anchor, QSize bar, QRect available).
  Test exact above/below/clamp positions, invalid/oversized geometry; clicked Feature and copied snapshot, dismiss and hidden stale clicks.
- [x] Implement tested placement and widget. Qt::Tool | FramelessWindowHint | WindowStaysOnTopHint |
  WindowDoesNotAcceptFocus; WA_ShowWithoutActivating; buttons NoFocus. No activation or native hacks.
- [x] Add IPlatformWindowPolicy and LinuxWindowPolicy. Detect QGuiApplication::platformName, not session alone.
  Map xcb to AnchoredNonActivating; wayland* to Unsupported. Select screenAt(anchor.center), then primary.
  Retain raw rectangle; one isolated calculation with 8px gap and availableGeometry clamping.
- [x] Add fake-monitor Application integration tests and AppController::runSelection tests, observe RED.
  Test disabled/enabled, clear-before-click, A→B, busy, missing anchor, monitor failure, unsupported policy.
  HTTP test requires zero passive calls, one click call with B and no clipboard content; Copy Result alone changes clipboard.
- [x] Implement runSelection through runText, Settings/tray checkbox, Application signal wiring and metadata-only diagnostics.
  Hide/clear toolbar on clear, failure, disabled and loading; do not stop monitor or query selection on click.
- [x] Preserve M0/M1 tests; add native xcb/Xvfb AT-SPI overlay integration and system XGetInputFocus comparison.
  Use separate fixture with deterministic hold selection mode and reported native window id.
  QTest mouse click only our own toolbar; no input injection into third-party processes.
- [x] CI Release configure/build, CTest, isolated keyring, GUI smoke, M1 runtime, M2 overlay.
  Commands: cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release; cmake --build build --parallel 2;
  xvfb-run -a ctest --test-dir build --output-on-failure; existing CI runtime commands unchanged.
- [x] Record measured results and NOT TESTED desktop limitations in m2-completion.md and m2-overlay-compatibility.md.
  Update README, architecture, privacy and troubleshooting. Commit/push and stop at M2.

Completed 2026-10-06: final implementation 3df76c5; CI 37463066504 passed all gates. 10/10 CTest plus keyring, smoke, real AT-SPI and Xvfb/Openbox overlay. Details and untested desktop limits: m2-completion.md. M3 not started.
