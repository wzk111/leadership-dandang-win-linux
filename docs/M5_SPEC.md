# WorkSidekick — M5 Technical Specification
## Windows Native Backend & Cross-Platform Integration

**Repository:** https://github.com/wzk111/leadership-dandang-win-linux

**Accepted baseline:** `903b84473f52662e86b9d9dc95a94d82f6be5562`

**Milestone status:** M0–M4 accepted. M5 not started.

**Primary Windows target:** Windows 11 x64, with Windows 10 x64 compatibility where practical.

**Technology:** C++20, Qt 6, CMake, Win32, COM, Microsoft UI Automation.

---

## 1. Mission

Implement a fully functional Windows version of WorkSidekick while preserving the shared product architecture developed during M0–M4.

The Windows implementation must reuse the existing:

- FeatureRegistry and PromptBuilder
- Profile and FeaturePreferences
- GenerationContext and refinements
- AIService and four AI providers
- AppController
- ActionBar, ManualActionPalette, ReplyComposer, ResultCard
- Settings and Diagnostics
- Selection and explicit-consent semantics

The Windows version must not be a second, independently maintained product.

**Success objective:**

```text
Select text in another Windows application
              ↓
Windows UI Automation
              ↓
Selection snapshot
              ↓
Floating ActionBar
              ↓
User clicks feature
              ↓
Existing AppController / AIService
              ↓
ResultCard
              ↓
Copy or refine
```

Fallback:

```text
Select text
     ↓
Ctrl+Alt+P
     ↓
UI Automation selection
     ↓
ManualActionPalette
```

Final fallback:

```text
Ctrl+C
     ↓
Ctrl+Alt+P
     ↓
Clipboard
     ↓
ManualActionPalette
```

Selection and shortcut activation must never automatically upload text to AI.

---

## 2. Mandatory Repository Review

Before editing any code, inspect:

```text
CMakeLists.txt

src/main.cpp
src/platform/PlatformFactory.h
src/platform/ISelectionProvider.h
src/platform/ISelectionMonitor.h
src/platform/IGlobalShortcut.h
src/platform/IPlatformWindowPolicy.h
src/platform/ISecretStore.h

src/app/Application.h
src/app/Application.cpp
src/app/ApplicationOverlay.cpp
src/app/ApplicationManual.cpp
src/app/ApplicationProduct.cpp
src/app/AppController.cpp
src/app/InstanceCoordinator.cpp

src/ai/AIService.*
src/core/Core.*
src/core/SelectionResolver.*

docs/m4-completion.md
docs/architecture.md
docs/privacy.md
```

Review the Linux implementations only to understand their interface contracts.

Do not port AT-SPI, X11 or Wayland internals to Windows.

Before implementation, document which source files are genuinely cross-platform and which depend on POSIX/Linux behavior.

---

## 3. M5 Scope

Implement these Windows-native adapters:

1. Windows UI Automation SelectionProvider
2. Windows selection event/gesture monitor
3. Windows global shortcut using `RegisterHotKey`
4. Windows window policy for a non-activating ActionBar
5. Windows Credential Manager secret storage
6. Windows selection resolver with clipboard fallback
7. Windows single-instance IPC support
8. Windows platform factory
9. Windows build and CI
10. Windows diagnostics and compatibility tests

Preserve all existing Linux behavior.

Do not implement new AI features, streaming, automatic paste or message sending.

---

## 4. Target Architecture

```text
                  Shared Qt Product
                  ┌────────────────┐
                  │ Application    │
                  │ AppController  │
                  │ AIService      │
                  │ PromptBuilder  │
                  │ FeatureRegistry│
                  │ Profile        │
                  └────────┬───────┘
                           │
                 Platform Interfaces
                           │
              ┌────────────┴───────────┐
              │                        │
         Linux Backends           Windows Backends
              │                        │
      AT-SPI / X11            UI Automation
      XDG Portal              Win32 Hooks
      libsecret               Credential Manager
      LinuxWindowPolicy       WindowsWindowPolicy
      Linux resolver          Windows resolver
```

The business and AI layers must not contain conditional branches such as:

```cpp
#ifdef _WIN32
    // Windows-specific selection behavior
#endif
```

Native Windows details belong inside `src/platform/windows/`.

---

## 5. Windows Platform Directory

Recommended structure:

```text
src/platform/windows/
├── WindowsPlatformFactory.cpp
├── UIASelectionProvider.h
├── UIASelectionProvider.cpp
├── WindowsSelectionMonitor.h
├── WindowsSelectionMonitor.cpp
├── WindowsGlobalShortcut.h
├── WindowsGlobalShortcut.cpp
├── WindowsWindowPolicy.h
├── WindowsWindowPolicy.cpp
├── WindowsSecretStore.h
├── WindowsSecretStore.cpp
├── WindowsSelectionResolver.h
├── WindowsSelectionResolver.cpp
├── WindowsPlatformDiagnostics.h
└── WindowsPlatformDiagnostics.cpp
```

Create helper abstractions only when they solve a real ownership, threading or testing problem.

Avoid unnecessary wrappers around the entire Windows API.

---

## 6. UI Automation Selection Retrieval

Implement `UIASelectionProvider` with Microsoft UI Automation.

Preferred retrieval sequence:

```text
Get foreground window
         ↓
Locate focused UI Automation element
         ↓
Check TextPattern support
         ↓
TextPattern.GetSelection()
         ↓
IUIAutomationTextRange::GetText()
         ↓
IUIAutomationTextRange::GetBoundingRectangles()
         ↓
Selection
```

Use the COM interfaces supported by the selected Windows SDK.

Relevant APIs include:

```cpp
IUIAutomation
IUIAutomationElement
IUIAutomationTextPattern
IUIAutomationTextRange
IUIAutomationTextRangeArray
```

Use `UIA_TextPatternId` and `UIA_TextPattern2Id` as applicable.

Do not assume all applications expose both.

### Required behavior

- Return valid nonempty selected text.
- Preserve original whitespace.
- Enforce the existing 100,000-character limit.
- Return a meaningful application/source label when available.
- Attempt selection screen coordinates.
- Treat missing geometry as nonfatal.
- Exclude password/protected elements.
- Handle unsupported UIA providers gracefully.
- Never traverse the entire desktop accessibility tree.

Do not read arbitrary text from the focused control when its selection is empty.

---

## 7. UI Automation COM Threading

Windows UI Automation must not block the Qt GUI thread.

Implement an appropriate COM worker architecture.

Recommended initial strategy:

```text
Qt GUI thread
      │
      │ request / event
      ▼
Windows UIA worker
      │
      │ COM initialized
      │ UI Automation queries
      ▼
Selection result
      │
      │ queued Qt signal
      ▼
GUI thread
```

Document:

- COM apartment choice and justification
- `CoInitializeEx` / `CoUninitialize` ownership
- COM object release
- event handler registration/removal
- shutdown ordering
- cross-thread marshaling where required

Do not pass apartment-affine COM pointers between threads without valid COM marshaling.

A hung accessibility provider must not permanently freeze the UI.

Use cancellation/staleness safeguards, and document limitations of blocking external COM calls.

---

## 8. Selection Monitoring

Implement Windows automatic selection detection.

Use a focused, lightweight mechanism.

Preferred initial approach:

```text
WH_MOUSE_LL
    ↓
WM_LBUTTONDOWN
WM_LBUTTONUP
    ↓
drag threshold / double click detection
    ↓
debounce
    ↓
UIA selection retrieval
    ↓
selectionDetected
```

Use `SetWindowsHookEx` with `WH_MOUSE_LL`.

Do not install a keyboard hook or keylogger.

The hook callback must return quickly.

**Never call UI Automation synchronously from the low-level mouse hook.**

Instead queue a request to the UIA worker.

### Important compatibility enhancement

Where appropriate, support UIA `TextSelectionChangedEvent` as an additional event source for keyboard-created selections, provided it can be implemented reliably without broad text surveillance.

Both event sources must feed one debounced selection pipeline.

Keep keyboard selection support separate from general keyboard monitoring.

---

## 9. Monitor Lifecycle

Preserve the existing `ISelectionMonitor` contract.

Support:

```text
start
stop
status
latestSelection
selectionDetected
selectionCleared
statusChanged
```

Test:

- repeated start/stop
- hook registration failure
- COM initialization failure
- invalid foreground window
- unsupported TextPattern
- empty selection
- stale async completion
- app shutdown with work pending

A late worker result must not resurrect a cleared selection or show an old toolbar.

Maintain a generation/sequence counter similar to the existing Linux implementation.

---

## 10. Selection Screen Coordinates

Extract UI Automation bounding rectangles in physical screen coordinates as documented by UIA.

The selected range can provide several rectangles.

Do not assume multi-line selections produce a single rectangle.

For M5, select a stable anchor strategy:

- Prefer the last meaningful selection rectangle when appropriate for toolbar placement.
- Alternatively use the union, if testing shows better behavior.
- Keep the strategy explicit and unit-testable.

Convert coordinates into Qt window geometry consistently.

### DPI handling

Windows may run at:

```text
100%
125%
150%
200%
mixed DPI
```

AT-SPI/Linux coordinates and Windows UIA coordinates must not be treated as interchangeable.

Before creating the first Qt window, verify the application's DPI awareness.

Do not indiscriminately call `SetProcessDpiAwarenessContext` after Qt has already initialized DPI behavior.

Use a tested conversion between UIA screen coordinates and Qt logical coordinates.

Do not apply the same device-pixel ratio twice.

Test negative-coordinate secondary monitors.

---

## 11. Windows Non-Activating ActionBar

Implement `WindowsWindowPolicy`.

Use the existing Qt ActionBar.

Apply suitable Win32 extended styles where necessary:

```cpp
WS_EX_NOACTIVATE
WS_EX_TOOLWINDOW
```

Use:

```cpp
SetWindowPos(..., SWP_NOACTIVATE)
```

when appropriate.

The ActionBar should:

- Stay above the source application.
- Appear near the selection.
- Not activate merely by appearing.
- Accept mouse button clicks.
- Preserve its Selection snapshot.
- Disappear on selection clear.
- Avoid stealing keyboard focus during display.

Do not indiscriminately apply `WS_EX_TRANSPARENT`, which can make the toolbar unclickable.

Do not add `WS_EX_LAYERED` unless required by a verified visual/behavioral requirement.

### Focus verification

A valid Windows focus test should examine foreground window state using `GetForegroundWindow`, not only `QWidget::hasFocus()`.

Verify source focus before and after automatic display.

Click behavior and ResultCard activation should be tested separately.

---

## 12. Windows Global Shortcut

Implement `WindowsGlobalShortcut` using:

```cpp
RegisterHotKey()
UnregisterHotKey()
WM_HOTKEY
```

Preferred shortcut:

```text
Ctrl+Alt+P
```

Use:

```cpp
MOD_CONTROL | MOD_ALT | MOD_NOREPEAT
```

where supported.

Register on a thread/window with a real message loop.

Qt native event filtering is acceptable if reliable.

Do not block the GUI thread with a custom polling loop.

Handle:

- shortcut collision
- registration failure
- repeated activation
- disable/re-enable
- clean shutdown
- OS-reserved combinations
- multiple-instance behavior

Shortcut activation invokes the existing manual selection flow.

It never calls AI directly.

---

## 13. Windows Selection Resolver

Preserve the current shared resolver semantics.

Windows priority:

```text
Valid UIA Selection
       ↓
Clipboard
       ↓
No selection
```

Do not implement X11 PRIMARY on Windows.

Recommended:

```cpp
WindowsSelectionResolver
    : IExplicitSelectionResolver
```

Use existing `SelectionResolver` composition if possible.

Do not introduce a new second resolver algorithm.

### Clipboard privacy

Read clipboard only after:

- the user presses the shortcut
- the user invokes the tray action
- the user invokes `--trigger`
- the user explicitly chooses Process Clipboard

Do not implement continuous clipboard monitoring.

---

## 14. Clipboard Fallback

Use the existing Qt clipboard interface on Windows.

Do not automatically simulate Ctrl+C in M5.

Reason: UIA already provides the preferred selection path, and automatic input injection introduces focus and clipboard integrity risks.

If UIA cannot retrieve a selection:

```text
No UIA selection
      ↓
User previously copied text
      ↓
Clipboard
      ↓
ManualActionPalette
```

Manual palette must clearly label:

```text
Source: Clipboard
```

so users can identify stale text before requesting AI.

---

## 15. Windows Credential Manager

Replace Linux libsecret with Windows Credential Manager behind the existing `ISecretStore` and CredentialService architecture.

Preferred APIs:

```cpp
CredReadW
CredWriteW
CredDeleteW
CredFree
```

Use `CRED_TYPE_GENERIC`.

Separate provider credentials.

Suggested targets:

```text
WorkSidekick/openai
WorkSidekick/anthropic
WorkSidekick/gemini
WorkSidekick/openai_compatible
```

Use an appropriate `CRED_PERSIST` setting and document the persistence behavior.

Do not place provider keys in QSettings.

Do not use plaintext files.

Do not show keys in Diagnostics.

### Migration

Linux and Windows use different operating-system credential stores.

Do not attempt to move credentials between machines automatically.

On Windows, fresh configuration is acceptable.

However, provider separation and the shared CredentialService semantics must be maintained.

Use synthetic credentials for integration tests.

---

## 16. Windows Single-Instance Behavior

The existing M3 implementation uses `InstanceCoordinator`, `QLockFile`, and `QLocalServer`.

Review it for POSIX assumptions.

In particular:

- `XDG_RUNTIME_DIR` is Linux-specific.
- `SO_PEERCRED` is Linux-specific.
- Unix socket permissions are not directly portable.
- POSIX filesystem modes are not equivalent to Windows ACLs.

Do not add `#ifdef _WIN32` throughout the existing Linux implementation.

Prefer platform-specific instance transport or security policies.

### Required behavior

```text
worksidekick.exe
```

starts the primary resident instance.

```text
worksidekick.exe --trigger
```

forwards an explicit trigger to the existing primary instance.

A second ordinary launch must not create another tray process.

### IPC security

Use Windows-compatible local IPC with current-user access control.

Qt local sockets / named pipes are acceptable when correctly secured.

Never expose a public TCP listener.

Do not transmit selected text or API keys over the instance-control IPC.

Allowed commands remain bounded:

```text
open
trigger
```

Handle stale instance state and simultaneous process startup safely.

---

## 17. Windows Platform Factory

Extend `PlatformFactory` to create:

```text
WindowsSecretStore
WindowsSelectionMonitor
WindowsSelectionResolver
WindowsGlobalShortcut
WindowsWindowPolicy
```

Select implementations at compile time.

Do not force Linux dependencies onto Windows.

Current shared code should continue relying on the same interfaces.

---

## 18. Windows App Startup

Review `main.cpp`.

Ensure platform initialization occurs before `QApplication` where necessary.

Linux currently initializes X11/GLib-related behavior.

Windows must not execute Linux initialization.

Construct dependencies in a safe lifetime order:

```text
QApplication
     ↓
QSettings
     ↓
PlatformFactory
     ↓
CredentialService
     ↓
AIService
     ↓
AppController
     ↓
SelectionMonitor
     ↓
Application
```

Retain the existing tray behavior.

Closing the workspace should not exit the resident application when the tray is available.

If the Windows tray is unavailable, provide a usable window-based fallback.

---

## 19. Shared UI Reuse

M5 must not rewrite:

```text
ActionBar
ManualActionPalette
ReplyComposer
ProfileWindow
ResultCard
SettingsWindow
WorkspaceWindow
```

Only make targeted cross-platform fixes.

Preserve:

- six product features
- quick action preferences
- Reply stances
- Profile controls
- provider switching
- refinement context
- clipboard Copy Result
- global shortcut setting
- automatic toolbar setting

Any styling changes must be shared and avoid Linux regressions.

---

## 20. Diagnostics

Extend existing Diagnostics with Windows metadata.

Example:

```text
Platform: Windows 11
Qt platform: windows

Selection backend:
Microsoft UI Automation

UIA initialized: yes
Selection monitor: running
TextPattern supported: yes

Last selection:
Application: Notepad
Characters: 143
Anchor available: yes

Automatic overlay:
WindowsNonActivating

Global shortcut:
Win32 RegisterHotKey
Ctrl+Alt+P
Registered: yes

Selection resolver:
UIA → Clipboard

Secret store:
Windows Credential Manager

AI provider:
OpenAI
```

Do not log selected text.

Do not display saved credential values.

---

## 21. Windows CI

Add:

```text
.github/workflows/windows.yml
```

Recommended GitHub Actions environment:

```yaml
runs-on: windows-2022
```

Use MSVC and a pinned, compatible Qt 6 installation.

Ensure Qt and compiler architecture match.

Required build:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Adjust the generator for the selected MSVC environment if required.

Do not merely copy the Linux CI script into Windows.

### CI scope

Windows CI should cover:

- shared core tests
- all four provider protocol tests
- prompt and profile tests
- UI tests
- Windows platform backend tests
- credential round trip
- global shortcut unit/integration tests where runner permits
- single-instance tests

Desktop-interaction tests may require a dedicated interactive session.

Do not mark UIA cross-application behavior PASS if Windows CI does not actually provide an interactive desktop.

---

## 22. Preserve Ubuntu CI

Keep:

```text
.github/workflows/ubuntu.yml
```

functionally unchanged except necessary shared test additions.

Linux must still pass:

```text
Release build
CTest
libsecret
GUI smoke
AT-SPI event integration
X11 overlay
X11 shortcut
Portal fake-service integration
IPC tests
M4 provider tests
```

Do not weaken Linux assertions to make Windows compile.

---

## 23. Windows Testing Strategy

Use three levels of testing.

### Level A — Unit tests

Mock Windows platform interfaces.

Test:

```text
selection normalization
empty selection
multiple text ranges
invalid coordinates
DPI conversion
stale async results
shortcut state transitions
window policy configuration
```

### Level B — Native API integration

Use synthetic local applications/windows.

Test:

```text
UI Automation TextPattern
selection change
global shortcut
Win32 focus preservation
Credential Manager round trip
instance IPC
```

### Level C — Real application testing

When an interactive Windows desktop is available, test:

```text
Notepad
Microsoft Edge
Google Chrome
VS Code
Microsoft Word
Outlook
Teams
```

Use synthetic nonprivate text.

Each application must be reported separately.

---

## 24. Compatibility Matrix

Create:

```text
docs/m5-windows-compatibility.md
```

Use:

| App | UIA text | Auto detect | Anchor | Focus preserved | Hotkey | Clipboard fallback |
|---|---|---|---|---|---|---|
| Notepad | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED |
| Edge | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED |
| Chrome | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED |
| VS Code | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED |
| Word | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED |
| Outlook | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED |
| Teams | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED |

Replace values only after real tests.

---

## 25. Windows Security Boundaries

Do not attempt to bypass:

- UAC secure desktop
- elevated-process integrity boundaries
- protected password fields
- corporate endpoint restrictions
- inaccessible application sandboxes

WorkSidekick should normally run without administrator privileges.

If UIA access fails:

```text
UIA unavailable
      ↓
manual clipboard fallback
```

Do not silently request administrator privileges.

No DLL injection.

No process-memory scraping.

No screen recording.

No keylogging.

---

## 26. Privacy Regression

All previous rules remain mandatory.

Test:

```text
UIA selection
    → 0 HTTP

automatic ActionBar
    → 0 HTTP

Win32 global shortcut
    → 0 HTTP

ManualActionPalette
    → 0 HTTP

Reply Composer open
    → 0 HTTP
```

Only explicit actions may send AI requests.

Test:

```text
UIA Selection A
Clipboard B

Click Polish
```

The request must use A, not B.

Test:

```text
Selection A
Selection B
Click Reply
```

Reply Composer must own B.

Test:

```text
Generate result from A
select B
click Shorter
```

Refinement must use the original A context.

---

## 27. Resource Ownership

Review Windows native resource lifetime explicitly.

Use RAII for:

```text
HWND
HHOOK
HANDLE
COM interface pointers
HKEY if used
Credential Manager allocations
```

Use appropriate smart pointers or scoped wrappers.

Do not mix Qt QObject ownership with COM reference ownership accidentally.

Global hooks and hotkeys must be released on shutdown.

No leaked hidden helper windows.

No orphan worker threads.

---

## 28. Performance Targets

Targets, not assumed PASS values:

```text
Idle CPU: approximately <1%

Selection event to UIA snapshot:
typically <150 ms where provider responds promptly

ActionBar appearance:
prompt after selection extraction

Global shortcut to palette:
typically <250 ms

No blocking UI thread

No continuous clipboard polling

No broad UIA tree scanning
```

Measure actual latency in a supported environment.

Do not invent performance results.

---

## 29. Suggested Implementation Phases

### M5.1 — Cross-Platform Build Foundation

- Audit Linux-only assumptions.
- Make shared targets compile under MSVC.
- Add Windows CMake platform targets.
- Add basic Windows CI.

Gate: shared M4 code compiles on Windows.

### M5.2 — Credentials and Manual Clipboard

- Windows Credential Manager.
- Windows SelectionResolver.
- Existing manual palette.
- Shared AI workflow.

Gate:

```text
Copy text
→ Process Clipboard / manual palette
→ AI
→ ResultCard
```

works on Windows.

### M5.3 — Windows Shortcut and IPC

- RegisterHotKey.
- Single-instance Windows transport.
- `--trigger`.
- Tray integration.

Gate:

```text
Ctrl+Alt+P
→ ManualActionPalette
```

without duplicate processes.

### M5.4 — UI Automation Selection

- COM worker.
- TextPattern selection.
- source application.
- bounding rectangles.
- selection monitor.

Gate: selected text retrieved from supported applications.

### M5.5 — Floating ActionBar

- WindowsWindowPolicy.
- non-activating placement.
- DPI-aware geometry.
- selection monitor integration.

Gate:

```text
Select text
→ ActionBar
→ click
→ correct AI text
```

### M5.6 — Regression and Delivery

- Windows CI.
- Ubuntu CI.
- compatibility matrix.
- privacy review.
- documentation.

Do not skip the intermediate gates.

---

## 30. M5 Acceptance Criteria

M5 can be classified code-level PASS when:

- [ ] Windows x64 Release build succeeds.
- [ ] Windows automated tests pass.
- [ ] Ubuntu M0–M4 regression tests remain green.
- [ ] Windows Credential Manager stores independent provider keys.
- [ ] Windows manual clipboard mode works.
- [ ] Win32 global shortcut works in a suitable interactive environment.
- [ ] Windows `--trigger` works.
- [ ] Single-instance protection works.
- [ ] UIA selected-text retrieval works.
- [ ] UIA missing/empty selection degrades safely.
- [ ] Selection monitor is event-driven.
- [ ] UIA coordinates are handled consistently with Qt DPI.
- [ ] ActionBar preserves source focus when automatically displayed.
- [ ] Feature click uses the captured Selection snapshot.
- [ ] Shortcut or selection alone makes zero AI requests.
- [ ] Reply, Profile, refinements and provider switching remain functional.
- [ ] Windows native resources are released correctly.
- [ ] Platform-specific code remains isolated.
- [ ] Documentation and CI evidence are recorded.

**Important:** A green headless Windows CI alone is insufficient to claim complete automatic selection/overlay support. A code-level PASS requires suitable native integration evidence; unsupported real applications may remain `NOT TESTED`, but fabricated UIA/focus evidence is not acceptable.

---

## 31. Known Platform Limitations

M5 PASS must not imply:

```text
Every Windows application supports UIA

All elevated applications can be inspected

All Electron editors expose TextPattern

Mixed-DPI monitors have been tested unless actually measured

Microsoft Word/Outlook/Teams are fully compatible without testing

Linux Wayland anchored overlay is supported

Real provider APIs have all been tested
```

Do not infer these from compilation or synthetic tests.

---

## 32. Documentation Deliverables

Create:

```text
docs/M5_SPEC.md
docs/m5-plan.md
docs/m5-completion.md
docs/m5-windows-compatibility.md
docs/windows-architecture.md
```

Update:

```text
README.md
docs/architecture.md
docs/privacy.md
docs/troubleshooting.md
```

Document Windows setup, build, launch, permissions and troubleshooting.

---

## 33. M5 Completion Report

The final report must include:

```text
M5 Gate = PASS / PARTIAL / FAIL

Baseline commit:
Final implementation commit:

Windows Release build:
Windows CTest:
Ubuntu CI:
M0 regression:
M1 regression:
M2 regression:
M3 regression:
M4 regression:

Win32 selection:
UIA TextPattern:
Selection monitor:
Selection rectangle:
DPI handling:

Non-activating ActionBar:
Windows focus verification:

Global shortcut:
Shortcut collision:

Clipboard fallback:
ManualActionPalette:

Single instance:
--trigger:
IPC security:

Windows Credential Manager:
Provider key separation:

Reply/Profile/Refinement:
Provider protocol regression:

Privacy regression:

Notepad:
Edge:
Chrome:
VS Code:
Word:
Outlook:
Teams:

Known blockers:

Recommended next milestone:
M6 — Cross-Platform Stabilization, Packaging & Real-Desktop Acceptance
```

Anything not actually tested must be `NOT TESTED`.

---

## 34. Required Development Discipline

Implement iteratively.

Do not create all Windows adapters as empty stubs and declare M5 complete merely because CMake builds.

For each phase:

1. Add a failing test or a clearly specified native validation.
2. Implement the smallest functioning backend.
3. Run the relevant Windows tests.
4. Run shared tests.
5. Run Ubuntu regression CI.
6. Record evidence.
7. Continue to the next phase.

Where a native desktop test is impossible in the execution environment, report the exact limitation rather than replacing it with an unrelated unit test.

---

## 35. Stop Condition

After completing M5:

**STOP. Do not start M6 automatically.**

Provide the implementation report, commit SHA, CI links and remaining native-desktop validation requirements.

Do not claim real Windows desktop compatibility solely from mocks or CI build success.

---

# Final M5 Objective

The intended result is **one WorkSidekick product**, with shared business logic and two platform implementations:

```text
                  WorkSidekick
                       │
              Shared C++ / Qt
                       │
        ┌──────────────┴──────────────┐
        │                             │
       Linux                        Windows
        │                             │
   AT-SPI + X11                    UI Automation
   XDG Portal                      RegisterHotKey
   libsecret                       Credential Manager
   Linux overlay                   Windows overlay
        │                             │
        └──────────────┬──────────────┘
                       │
             Same six AI features
             Same Provider system
             Same Profile/Reply
             Same ResultCard
             Same privacy model
```

M5 is complete when Windows supports the same core product workflow through native platform adapters without sacrificing the Linux implementation.
