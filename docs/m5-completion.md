# M5 completion report

Date: 2026-10-28. Version: 0.6.0. Scope stops at M5.

**M5 Gate: PASS (code-level). No known M5 acceptance blockers remain.**

- Accepted M4 baseline: `903b84473f52662e86b9d9dc95a94d82f6be5562`.
- Final implementation commit: `12b9a678f629ef204e208d0456968856d0890cb0`.
- Windows Release / CTest / launch smoke: [40705092262](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/40705092262), job `122874937692`, **SUCCESS**.
- Ubuntu Release / regression / native gates: [40705092221](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/40705092221), job `122874937935`, **SUCCESS**.
- Subsequent delivery changes are documentation only. The implementation SHA above
  identifies the exact build used for final native validation.

## Build and M0–M4 regression

| Gate | Result / evidence |
|---|---|
| Windows x64 Release | PASS: Windows Server 2022, MSVC 2022, pinned Qt 6.8.3, Ninja |
| Windows CTest | 24/24 PASS; two opt-in desktop functions skip in CI and are covered locally below |
| Windows launch smoke | PASS in CI and locally; local exit code 0 |
| Ubuntu CTest | 23/23 PASS on Ubuntu 22.04 / Qt 6.2 |
| M0 | Clipboard/manual AI flow, provider/error/cancel and secure storage regression PASS |
| M1 | Selection unit tests and real AT-SPI events plus unavailable-registry gate PASS |
| M2 | Snapshot toolbar tests, X11 placement and source-focus gate PASS |
| M3 | Explicit resolver, manual palette, shortcut, portal and instance regression PASS |
| M4 | Six features, Profile, Reply, refinements, preferences and provider regression PASS |
| Linux native gates | Secret Service 5 PASS; AT-SPI runtime 3 + 3 PASS; overlay 3 PASS; X11 shortcut 4 PASS; all zero skipped |
| Provider protocols | Shared OpenAI Responses, Anthropic Messages, Gemini Interactions and compatible Responses fixtures PASS; no paid API calls |

QtTest entry counts include initialization and cleanup. CTest counts identify
test executables, not individual assertions.

## Final native Windows verification

The final CI artifact was downloaded and executed on Windows 11 with the normal
current-user token, using only generated fixture text and UUID-isolated credential
targets. This is native desktop/API evidence, separate from mock tests.

| Local executable | QtTest result |
|---|---|
| test_uia | 8 PASS, 1 SKIP: this run lacked programmatic foreground permission for nativeTextPattern |
| test_windowsmonitor | 6 PASS, zero skipped |
| test_windowspolicy | 11 PASS, zero skipped |
| test_windowssecret | 3 PASS, zero skipped |
| test_windowsshortcut | 4 PASS, zero skipped |
| test_windowsinstance | 5 PASS, zero skipped |
| test_windowsruntime | 3 PASS, zero skipped |

The complete native UIA TextPattern case passed in the final Windows CI:
**9 PASS, zero skipped**. Local runtime/policy cases independently exercised
fresh UIA retrieval on the same final implementation. Thus the local UIA skip is
recorded as a skip, rather than substituted for those actual execution results.

Local logs are retained under `.local/native-final-12b/`; CI publishes its logs
and validation binaries. No helper, fixture or application process remained after
validation. These tests make no paid AI request.

| Required native behavior | Result / scope |
|---|---|
| Win32 selection / UIA TextPattern | PASS: external RichEdit selected text, whitespace preservation, empty/caret, password exclusion, own-process exclusion and 100,000-character boundary |
| Multiple ranges | Joining/limit/last-meaningful-rectangle unit tests PASS; real multi-range app behavior NOT TESTED |
| Event-driven monitor | Real WH_MOUSE_LL double-click produced exactly one selection, initialized UIA and a valid physical anchor; no polling or keyboard hook |
| Keyboard selections | Fresh UIA through explicit shortcut path; automatic keyboard-selection events are not enabled |
| Selection rectangle / DPI | Physical anchor verified; 100/125/150/200 percent and negative-origin unit cases PASS; native HMONITOR placement across two 100 percent monitors, including negative Y, PASS |
| Qt DPI awareness | Actual per-monitor V2 context verified after QApplication initialization and before the first test widget |
| Nonactivating ActionBar / source focus | Native styles and WM_MOUSEACTIVATE PASS; GetForegroundWindow and UIA selection unchanged across show, hide and native feature click |
| Captured selection | Feature click emitted the copied source snapshot; shared flow tests use selection A despite clipboard B |
| Global shortcut / collision | Real Ctrl+Alt+P input activated once without changing selection; registration, foreign-binding collision, disable/re-enable and RAII release PASS |
| Clipboard fallback / ManualActionPalette | Windows resolver prefers fresh UIA A over Clipboard B; only an explicit fallback reads clipboard; async/cancel/stale-context and shared palette tests PASS |
| Single instance / --trigger | Native competing owners, open/trigger forwarding and killed-owner recovery PASS; final stress: 100 races, 200 forwarded commands, zero failures |
| IPC security | Actual protected folder DACL and current-user-only pipe DACL verified; peer SID/session checks, bounded protocol, rejected invalid commands and idle-client deadline PASS |
| Credential Manager / provider separation | Four independent synthetic provider keys written/read; deleting Anthropic preserved the other three; LOCAL_MACHINE persistence checked; temporary targets removed |
| Reply / Profile / refinement | Shared Windows and Ubuntu product tests PASS; no duplicate Windows product/controller/provider implementation |
| Privacy | Detection, toolbar display and shortcut do not automatically invoke AI; shared tests assert explicit generation and snapshot boundaries |
| Native resource ownership | Restart/stop/cancel/timeout/stale-generation tests, shortcut release, worker cleanup and instance crash recovery PASS |

One synthetic runtime sample received a selection about 154 ms after input
injection completed. This is not a latency benchmark or a claim that the
specification's performance targets have been certified.

## Corrections made during native verification

- Qt reports display friendly names, which need not equal DISPLAY device IDs.
  Native HMONITOR matching fixes screen identification and preserves one
  screen-local physical-to-logical conversion.
- Pipe security inspection requires SE_KERNEL_OBJECT. Using SE_FILE_OBJECT
  returned error 87; actual current-user ACLs were verified without relaxing them.
- Direct SendInput on the hook-owning test thread delayed events about one second
  and lost the initial down. Test-only worker injection lets Qt pump the actual
  hook; all four events and the selection now arrive. Production injects no input.
- An earlier CI forwarding failure was captured as a false write-wait result.
  Forwarding now sends once and validates the bounded exact acknowledgment,
  including buffered data at EOF, instead of rejecting before reading it.
  The final CI, delayed-writer cases and 200-command native stress pass.
- Independent reviews also corrected application-range hotkey ownership,
  stale foreground completion, AUTOMOC inclusion, callback deletion reentrancy,
  positive IPC remaining deadlines and platform-neutral credential guidance.

## Real application matrix and remaining limits

| Application | Status |
|---|---|
| Notepad | NOT TESTED |
| Edge | NOT TESTED |
| Chrome | NOT TESTED |
| VS Code | NOT TESTED |
| Word | NOT TESTED |
| Outlook | NOT TESTED |
| Teams | NOT TESTED |

These results do not claim universal accessibility support, elevated/UIPI access,
physical mixed-DPI acceptance, performance certification, live provider API
compatibility or Linux Wayland anchored-overlay support. Profile remains local
plain text; OS credentials are separate per platform and are not migrated.

See [Windows compatibility](m5-windows-compatibility.md),
[Windows architecture](windows-architecture.md), [privacy](privacy.md) and
[troubleshooting](troubleshooting.md) for setup and operational boundaries.

**Recommended next milestone:** M6 — Cross-Platform Stabilization, Packaging &
Real-Desktop Acceptance. M6 has not been started.
