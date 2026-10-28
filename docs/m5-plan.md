# M5 implementation plan and pre-code platform audit

Baseline: 903b84473f52662e86b9d9dc95a94d82f6be5562. User approved M5_SPEC.md.
Use writing-plans, test-driven-development, subagent-driven-development and verification-before-completion. Continue established commit/push workflow. Stop before M6.

## Mandatory audit (completed before code edits)

Inspected CMakeLists, main, platform interfaces/factory, Application and overlay/manual/product wiring, AppController, InstanceCoordinator, AIService/Core/SelectionResolver, M4 report, architecture and privacy.

- Shared: core, Profile, feature preferences, prompts, generation/refinement, all AI/network providers, controller, credential router and Qt widgets.
- Targeted changes: resolver currently synchronous; Windows requires asynchronous fresh UIA resolution without blocking GUI. Add default asynchronous interface bridge preserving Linux behavior. Add UIA source label and neutral diagnostics.
- Linux only: src/platform/linux/*, libsecret/AT-SPI/X11/DBus, GLib/XInitThreads startup, native Linux tests.
- InstanceCoordinator.cpp: runtime directory, POSIX permissions, uid/SO_PEERCRED and socket-file stale cleanup require a Windows-specific implementation selected by CMake. Preserve Linux source.
- TestInstance.cpp: runtime environment, socket-file existence and Unix permission assertions are Linux-specific. Keep unchanged on Ubuntu; add equivalent Windows named-pipe/process tests.
- Windows tooling on this host: CMake available; no discovered Qt/MSVC command environment. CI supplies pinned Qt/MSVC; deploy CI build artifacts for local native validation if feasible.
- Do not claim real application support from mock tests.

## Design

Reuse all existing product interfaces, adding only resolveAsync(context, callback) defaulting to resolve() for Linux. Windows uses a fresh UIA query before any palette activation; clipboard fallback remains an explicit-trigger-only callback through shared resolver composition.

UIA executes in a short-lived helper process (MTA COM, no windows), controlled asynchronously by QProcess. Timeout/kill and sequence checks isolate hung external COM providers, with no orphan COM threads or apartment pointers crossing boundaries. IPC for this helper is bounded child stdout only, distinct from content-free instance-control named pipes. Mouse hook queues debounced retrieval, never performs COM. No keyboard hook. Keyboard-created selections remain retrievable by explicit shortcut; selection event support added only if reliable.

UIA returns physical rectangles; WindowsWindowPolicy maps screen-local physical offsets to Qt logical screen geometry once, preserving screen origins and negative coordinates. Qt owns per-monitor DPI awareness from QApplication initialization; helper declares matching awareness before COM queries.

Windows instance transport uses current-user-secured QLocalServer named pipes, per-user/session endpoint identity and lifetime arbitration. Windows Credential Manager uses independent generic targets with LOCAL_MACHINE persistence (same user, survives logoff; no cross-machine migration).

## Phases and gates

1. M5.1 Foundation: pinned Windows MSVC/Qt CI, shared M4 tests; platform source boundaries. Build/test shared targets on Windows and unchanged Ubuntu.
2. M5.2 Credentials/manual: native credential round trips with unique synthetic targets; asynchronous resolver and clipboard-only privacy tests.
3. M5.3 Shortcut/IPC: RegisterHotKey + Qt native message loop; collision/disable/re-enable; secured named pipes, simultaneous startup, stale owner, CLI forwarding.
4. M5.4 UIA: focused element TextPattern only; exclude own/protected/password; bounded whitespace-preserving multi-range text, optional last meaningful rectangle, worker timeout/stale lifecycle tests; native fixture TextPattern query.
5. M5.5 Overlay: NOACTIVATE/TOOLWINDOW, click acceptance and GetForegroundWindow focus evidence; pure DPI/negative monitor tests plus interactive native fixture.
6. M5.6 Full Windows and Ubuntu regression, local native validation where available, separate real-app matrix, privacy/resource review, docs and exact SHA/CI evidence.

Each phase has tests or specified native checks before implementation. No empty-adapter milestone PASS. Native desktop evidence may force PARTIAL when unavailable. All real-app and performance results remain NOT TESTED until measured.

## Execution evidence

- Foundation `fbcc7a9`: Windows `39613122678` and Ubuntu `39613122698` both SUCCESS.
- Async entry RED `88b2c40`: Windows and Ubuntu failed the new AsyncManual tests
  because the old entry called resolve synchronously and registered no completion.
  The application was then changed to use generation/QPointer-guarded callbacks.
- Native test cases were specified/written before their corresponding adapters.
  First full MSVC build exposed the UIA/Qt header order; it was corrected along
  with explicit AUTOMOC inclusion of InstanceCoordinator.h. No Linux adapter or
  Ubuntu workflow source was changed.
- Independent reviews covered COM/resource lifetime, protected selection, callback
  reentrancy, shortcut ID ownership, named-pipe ACLs and deadline handling. Findings
  corrected: application-range hotkey ID, foreground-change silent discard,
  incomplete-type constructor boundary, completed-signal deletion guard and
  nonnegative IPC remaining timeout. Tests cover the relevant regressions.
- Final evidence, exact implementation SHA and any desktop skips belong in
  [m5-completion.md](m5-completion.md), not inferred from this plan.
- On resumption, native diagnostics confirmed that Qt screen friendly names do
  not identify Win32 display devices. Matching native HMONITOR identities fixes
  actual two-monitor placement while preserving screen-local DPI conversion.
- Pipe ACL inspection with SE_FILE_OBJECT returned error 87; SE_KERNEL_OBJECT
  verified the actual current-user-only DACL. Production permissions were not
  relaxed. Fast owner close later exposed waitForBytesWritten(false) before ACK
  validation; forwarding now treats a bounded exact acknowledgment as authoritative.
- Synchronous SendInput on the hook-owning test thread delayed delivery about
  1 second and lost the initial down. Worker-thread input with the Qt event loop
  pumping restored native double-click detection and real Ctrl+Alt+P activation.
  Native source focus and unchanged selection across toolbar show/hide/click
  were measured separately. All input injection stays inside opt-in tests.
