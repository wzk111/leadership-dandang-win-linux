# WorkSidekick — M6 Technical Specification

## Cross-Platform Stabilization, Packaging & Real-Desktop Acceptance

**Repository:** https://github.com/wzk111/leadership-dandang-win-linux

**Accepted baseline:** `02179d261f4a61f7da5a019f7e05d2dc514e3bf9`

**Previous milestones:** M0–M5 accepted at their respective code-level gates.

**Target platforms:** Windows 11 x64, Ubuntu 22.04 GNOME X11 and GNOME Wayland.

**Technology:** C++20, Qt 6, CMake, existing native Windows/Linux adapters.

**Target release:** WorkSidekick `0.7.0`, release candidate quality.

**Primary rule:** M6 is a stabilization and delivery milestone, not a product redesign.

---

## 1. Mission

Turn the current code-complete cross-platform application into a product that can be installed, launched, used, updated and uninstalled reliably.

The product must preserve its established workflow:

```text
Select text
    ↓
Automatic ActionBar (when supported)
    OR
Manual global shortcut / tray trigger
    ↓
Choose AI feature
    ↓
Optional Reply Composer
    ↓
AI result
    ↓
Refine / Copy
```

M6 has five objectives:

1. Real-desktop compatibility verification and focused fixes.
2. Long-running stability and native resource lifecycle testing.
3. Windows and Ubuntu packaging.
4. Security, privacy and upgrade/uninstall validation.
5. Reproducible release candidate with evidence.

Do not start M7 or new unrelated features.

---

## 2. Read the Existing Project First

Read:

```text
PROJECT_SPEC.md
docs/M1_SPEC.md
docs/M2_SPEC.md
docs/M3_SPEC.md
docs/M4_SPEC.md
docs/M5_SPEC.md

docs/m4-completion.md
docs/m5-completion.md
docs/m5-windows-compatibility.md
docs/m5-windows-architecture.md
docs/windows-architecture.md

README.md
docs/architecture.md
docs/privacy.md
docs/troubleshooting.md

CMakeLists.txt
.github/workflows/ubuntu.yml
.github/workflows/windows.yml
```

If a listed documentation path differs, locate the actual equivalent rather than creating a duplicate.

Before changing code, document the existing release and runtime architecture.

Reuse all current components wherever possible.

---

## 3. Scope and Non-Goals

### In scope

- Real Windows 11 application compatibility
- Real Ubuntu GNOME X11 compatibility
- Real Ubuntu GNOME Wayland fallback verification
- UI Automation / AT-SPI compatibility fixes
- Hotkey, tray, IPC and credential lifecycle hardening
- DPI and multi-monitor testing
- Windows installer
- Ubuntu DEB package
- Portable Windows ZIP
- Reproducible build artifacts
- Clean installation and upgrade tests
- Long-running stability testing
- Privacy and security review
- Documentation and release candidate

### Out of scope

- New AI features
- New AI providers
- Cloud sync
- Automatic text replacement
- Automatic message sending
- Clipboard history
- Selection history
- Screen recording
- Keylogging
- Browser extension
- Auto-update service
- Background AI agent
- Windows privilege escalation
- Wayland compositor security bypass

Avoid major architectural rewrites.

---

## 4. Gate Policy

Separate three levels of evidence.

**Level A — Code / CI**

Compilation, unit tests, simulated fixtures and synthetic integration.

**Level B — Native desktop**

Testing in an actual Windows or GNOME graphical session, against real applications, with real window-management behavior.

**Level C — Product delivery**

Installation, launch, upgrade, uninstall and long-running usage from a packaged artifact.

Never substitute Level A evidence for Level B or C.

An environment not available to Codex must be marked:

```text
NOT TESTED — environment unavailable
```

Do not infer PASS from mocks.

---

# Part A — Real Desktop Compatibility

## 5. Windows Application Matrix

Test these applications when available:

| Application | Selection | Auto ActionBar | Focus | Shortcut | Clipboard fallback |
|---|---|---|---|---|---|
| Notepad | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED |
| Edge | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED |
| Chrome | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED |
| VS Code | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED |
| Word | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED |
| Outlook | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED |
| Teams | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED |

Do not require paid Office licenses solely to complete CI.

For each actually available application, test:

- Mouse drag selection
- Double-click word selection
- Selection clear
- Repeated selection
- Selection containing whitespace and Unicode
- Source application focus preservation
- Correct toolbar placement
- Feature click
- Snapshot independence from clipboard
- Ctrl+Alt+P
- Clipboard fallback
- Close/reopen target application

Record the application version, OS build, Qt version and test result.

**Minimum Windows desktop gate:** Notepad and at least one real browser must demonstrate the complete selection → ActionBar → explicit action flow. If an interactive desktop is unavailable, do not claim full M6 PASS.

---

## 6. Windows UIA Compatibility

Inspect failures using the existing UIA diagnostics.

Possible limitations:

```text
TextPattern not available
TextPattern selection empty
Unsupported custom editor
Incorrect text range
Missing bounding rectangle
Foreground changed during query
Protected element
Cross-integrity boundary
```

Fix reproducible defects in the Windows adapter only.

Do not introduce broad UIA tree scanning or arbitrary process-memory reading.

Preserve source snapshot semantics.

---

## 7. Windows Keyboard Selection

Test selections created using:

```text
Shift + Arrow
Ctrl + Shift + Arrow
Ctrl + A
```

The M5 automatic monitor is primarily mouse-gesture based.

Manual shortcut must provide a correct fallback for keyboard selection.

Do not silently claim automatic keyboard selection detection unless actually implemented and verified.

Do not add a generic keyboard hook to achieve this.

---

## 8. Ubuntu GNOME X11 Matrix

When a real GNOME X11 session is available, test:

```text
GNOME Text Editor / gedit
Firefox
Chromium / Chrome
VS Code
GNOME Terminal
Electron application where available
```

Verify:

- AT-SPI selection event
- Exact selected text
- Anchor rectangle
- Automatic ActionBar appearance
- Source focus preservation
- X11 PRIMARY fallback
- Ctrl+Alt+P
- ManualActionPalette
- Explicit AI action
- Clipboard preservation

Do not infer GNOME results from Xvfb/Openbox.

---

## 9. Ubuntu GNOME Wayland Matrix

Test the same applications where possible.

Expected supported behavior may differ from X11.

Verify:

```text
Native Wayland:
AT-SPI detection where available
ManualActionPalette
Portal shortcut where supported
Clipboard fallback
External --trigger fallback
```

Do not require absolute-positioned automatic ActionBar on native Wayland.

Do not add GNOME Shell extensions or compositor hacks.

If GlobalShortcuts portal is absent:

```text
Portal shortcut = unavailable
Tray/manual trigger = available
--trigger = available
```

This is graceful degradation, not a bug.

However, do not claim Portal support on a real desktop based only on the existing fake D-Bus test.

---

## 10. Real Desktop Test Harness

Introduce non-sensitive test fixtures where useful.

Use deterministic text such as:

```text
WorkSidekick synthetic selection test.
The meeting is scheduled for Friday at 3 PM.
```

Tests must not require workplace messages, private emails or credentials.

Capture metadata:

```text
OS
session
Qt backend
application/version
selected character count
anchor availability
focus result
toolbar geometry
action invoked
HTTP request count
```

Do not log source text by default.

---

# Part B — DPI, Multi-Monitor and Focus

## 11. Display Matrix

Windows target configurations:

```text
Single monitor, 100%
Single monitor, 125%
Single monitor, 150%
Single monitor, 200%

Dual monitor, same DPI
Dual monitor, mixed DPI
Secondary monitor with negative X/Y origin
```

Linux target configurations:

```text
Single monitor, 100%
Fractional scaling where available
Multiple monitors where available
```

Separate mathematical coordinate tests from physical display tests.

---

## 12. Placement Acceptance

For each verified configuration:

- Toolbar appears near the intended selection.
- Toolbar remains inside usable screen bounds.
- Toolbar does not overlap important selection text unnecessarily.
- Qt logical geometry and native physical coordinates are consistent.
- No double DPI scaling.
- No sudden jump to primary monitor.
- Missing anchor triggers the existing safe fallback.

Record actual versus expected geometry.

Do not create unverified per-application pixel offsets.

---

## 13. Focus Preservation

Use OS-level verification.

Windows:

```cpp
GetForegroundWindow()
```

Linux X11:

```text
XGetInputFocus
```

Verify focus before and after automatic toolbar display.

Separately test:

```text
Toolbar appears
Toolbar is clicked
Reply Composer opens
ResultCard opens
```

The last two may legitimately activate the application.

Do not conflate toolbar focus preservation with normal result-window activation.

---

# Part C — Stability and Performance

## 14. Long-Running Lifecycle Tests

Test:

- 1,000 repeated selection events
- 500 toolbar show/hide cycles
- 200 manual shortcut activations
- 100 enable/disable monitor cycles
- 100 shortcut register/unregister cycles
- 100 manual palette open/close cycles
- 100 request cancellation scenarios using synthetic HTTP
- Repeated target application exit/relaunch
- Repeated credential-store reads
- Multiple second-instance launches
- App shutdown while a query is pending

These are targets for automated or controlled stress testing, not permission to perform uncontrolled input injection into user applications.

Use synthetic test windows and mock credentials.

---

## 15. Memory and Native Resources

Monitor:

```text
RSS / private working set
handle count
thread count
COM object lifecycle
UIA helper processes
Windows hook handles
D-Bus subscriptions
X11 grabs
local IPC sockets/pipes
Qt QObject lifetime
```

Look for growth after repeated operations.

A small amount of allocator caching is not automatically a leak.

Distinguish stable caches from monotonic growth.

Investigate reproducible leaks.

---

## 16. UIA Worker Recovery

Windows UIA can interact with unresponsive external accessibility providers.

Verify:

- Worker query timeout
- Cancellation
- Terminating a hung helper
- Subsequent request recovery
- No stale result delivery
- No zombie helper process
- Main UI remains responsive

Test the shutdown sequence explicitly.

Do not block GUI shutdown indefinitely waiting for a hung UIA call.

---

## 17. AT-SPI Recovery

Linux cases:

```text
Accessibility bus missing at startup
Accessibility bus disappears
Target app closes during extraction
Monitor stopped during extraction
Repeated start/stop
```

Current M1 has documented limitations around failed initialization.

M6 should investigate recoverability without destabilizing the event-loop architecture.

If full in-process recovery is unsafe, document that restart is required.

Do not fake a successful recovery.

---

## 18. Global Shortcut Lifecycle

Windows:

```text
RegisterHotKey
WM_HOTKEY
UnregisterHotKey
```

Linux:

```text
XGrabKey
Portal session
```

Verify:

- Collision handling
- Repeated registration
- Disable/re-enable
- Window/session restart
- Application exit
- No leftover shortcut registration
- No duplicate activation

Portal dialogs must never appear unexpectedly on startup.

---

## 19. Single Instance and IPC

Stress:

```text
Two processes start simultaneously
Twenty rapid --trigger calls
Primary exits during forwarding
Primary crashes
Stale socket/pipe
Second instance launched during shutdown
Different user/session isolation
Malformed command
Slow/idle client
```

Only one primary tray application may exist per intended user/session.

IPC must not transmit source text or API keys.

---

## 20. Performance Targets

Measure, do not assume.

Suggested evaluation targets:

```text
Idle CPU:
approximately below 1% on a typical desktop

Global shortcut → Manual Palette:
P95 below 300 ms

Selection event received → ActionBar visible:
P95 below 250 ms where native providers respond normally

Normal UI responsiveness:
no noticeable GUI freezes

Memory:
no unexplained monotonic growth during repeated use
```

Record sample size, machine, operating system and measurement boundaries.

These targets are not guarantees across arbitrarily slow third-party accessibility providers.

---

# Part D — Packaging

## 21. Windows Packaging

Produce:

```text
WorkSidekick-0.7.0-windows-x64.zip
WorkSidekick-0.7.0-windows-x64-setup.exe
```

The ZIP must be portable.

The installer should support standard per-user installation without administrator privileges.

Choose a mature packaging solution such as:

```text
Inno Setup
```

or:

```text
NSIS
```

Prefer the tool that integrates cleanly with CI.

Do not write a custom installer from scratch.

---

## 22. Windows Runtime Dependencies

Use the Qt deployment tooling:

```text
windeployqt
```

Include required runtime components:

- Qt platform plugin
- Required Qt libraries
- Required Microsoft runtime dependencies, according to the deployment strategy
- UIA helper executable
- App icons and resources

Do not ship:

- Development headers
- Test credentials
- Test fixtures
- Build directories
- Debug-only files

Run the installed application on a clean Windows environment, not only the build machine.

---

## 23. Windows Installer Behavior

Installer should:

- Install under the current user's application directory by default.
- Create a Start Menu shortcut.
- Offer a desktop shortcut if desired.
- Register uninstall information.
- Support upgrade from the previous packaged build.
- Avoid duplicate running instances.
- Preserve user settings and credentials on upgrade.
- Not install a Windows Service.
- Not require system-wide hooks or admin rights.

Autostart may be optional but must default OFF.

Do not create a scheduled task silently.

---

## 24. Windows Uninstallation

Normal uninstall removes:

```text
Program files
Shortcuts
Application registration
Packaged resources
```

Handle user data explicitly.

Recommended behavior:

```text
Keep settings and credentials by default
Offer clearly labeled removal of local application data
```

If a complete removal option is implemented, ensure it only removes WorkSidekick-owned data and credentials.

Never delete unrelated Credential Manager entries.

Document how to perform complete manual removal if needed.

---

## 25. Portable ZIP

The portable ZIP should contain a runnable folder.

Acceptance:

```text
Extract ZIP
Run worksidekick.exe
Configure provider key
Use manual clipboard mode
Exit cleanly
```

Do not assume the user has Qt installed.

Portable deployment must not depend on developer absolute paths.

---

## 26. Ubuntu Packaging

Produce:

```text
worksidekick_0.7.0_amd64.deb
```

Use CPack DEB or another standard packaging mechanism.

Prefer a normal application package, not a custom root-level installation script.

Include:

```text
Binary
.desktop entry
Icon
Documentation
Runtime dependency declarations
```

Install into standard Linux filesystem locations.

Do not bundle or replace system accessibility services.

Do not install privileged background agents.

---

## 27. Ubuntu Runtime Dependencies

Accurately declare libraries needed by the compiled application.

Review:

```text
Qt 6
libsecret
AT-SPI
X11
D-Bus
system libraries
```

Do not rely on build-time `-dev` packages being installed on the user's machine.

Test package installation on a clean Ubuntu 22.04 environment.

Use apt/dpkg dependency resolution rather than embedding arbitrary downloaded system libraries.

---

## 28. Linux Desktop Integration

Verify:

- `.desktop` launcher works
- Application icon displays
- Tray behavior is documented
- Settings directory is created safely
- Keyring integration works
- `--trigger` executable path is usable
- Startup environment variables are handled
- Application exits cleanly

Avoid depending on GNOME tray support where it is unavailable.

A normal workspace-window fallback must remain usable.

---

## 29. Package Reproducibility

CI must identify:

```text
Git commit SHA
Application version
Compiler
Qt version
OS runner
Package tool
```

Generate SHA-256 checksums for each release artifact.

Example:

```text
SHA256SUMS.txt
```

Do not claim byte-for-byte reproducibility unless tested.

A reproducible process and pinned toolchain do not automatically imply identical output bytes.

---

# Part E — Release Security & Privacy

## 30. API Credentials

Windows:

```text
Credential Manager
```

Linux:

```text
Secret Service / libsecret
```

Verify provider key separation after packaged installation.

Do not migrate credentials into plaintext files.

Do not expose saved keys during upgrade or uninstall.

---

## 31. Settings and Profile

Profile is locally stored in plaintext QSettings.

Document that accurately.

Verify:

```text
Profile persists across upgrades.
Profile is not included in unrelated AI requests.
Clear Profile persists successfully.
No profile content appears in diagnostics.
```

Do not silently introduce cloud sync.

---

## 32. Network Privacy

Preserve the established rule:

```text
Selection detected       → 0 HTTP
Automatic toolbar shown  → 0 HTTP
Global shortcut pressed  → 0 HTTP
Manual palette opened    → 0 HTTP
Reply Composer opened    → 0 HTTP
```

Only explicit actions send AI requests.

Check all four providers.

Test failure and cancellation paths.

Avoid logging raw provider bodies, selected text, keys or Profile content.

---

## 33. Packaging Security

Do not ship:

```text
API keys
.env secrets
Test secret files
Developer credentials
Credential fixtures
Private workplace text
Local diagnostic traces
```

Inspect packaged file manifests.

Add automated checks for obviously forbidden paths and filenames.

Do not claim signed artifacts unless they are actually signed.

Windows code signing is recommended but can be recorded as `NOT CONFIGURED` if no certificate is available.

Do not create or purchase signing credentials automatically.

---

## 34. Software Bill of Materials

Generate a basic dependency inventory or SBOM if practical.

Record:

```text
Qt
Compiler/runtime
Native libraries
Installer tooling
Other third-party dependencies
```

Review known license obligations before releasing artifacts publicly.

Do not assume bundled runtime libraries have no redistribution requirements.

---

# Part F — CI and Release Delivery

## 35. Windows CI

Preserve:

```text
.github/workflows/windows.yml
```

Continue running:

```text
Release build
All CTest suites
Windows smoke test
Windows native backend integration
```

Add:

```text
Portable ZIP packaging
Installer build
Package file manifest
Artifact checksum
Portable launch smoke
Installer verification where runner permissions allow
```

A headless runner cannot prove all real UI behavior.

Maintain the native interactive test gate separately.

---

## 36. Ubuntu CI

Preserve:

```text
.github/workflows/ubuntu.yml
```

Continue running:

```text
Release build
CTest
Secret Service tests
GUI smoke
Real synthetic AT-SPI event test
X11 overlay/focus test
X11 shortcut test
Portal fake-service tests
IPC tests
M4 product/provider regression
```

Add:

```text
DEB packaging
Package dependency inspection
Fresh package installation smoke
Checksum generation
```

Do not weaken existing tests.

---

## 37. Release Workflow

Create a dedicated workflow:

```text
.github/workflows/release.yml
```

Requirements:

- Explicit tag or manual invocation
- Build from the selected commit
- Use pinned dependency versions
- Run platform tests
- Generate packages
- Generate checksums
- Upload artifacts
- Generate a release summary

Do not publish a production/latest release automatically merely because a development branch CI passed.

A candidate tag such as:

```text
v0.7.0-rc1
```

is suitable.

Actual GitHub Release publication should occur only after the required gates and explicit authorization.

---

## 38. Release Artifact Names

Recommended:

```text
WorkSidekick-0.7.0-rc1-windows-x64.zip

WorkSidekick-0.7.0-rc1-windows-x64-setup.exe

worksidekick_0.7.0~rc1_amd64.deb

SHA256SUMS.txt
```

Use consistent versioning.

Do not claim an installer exists unless the workflow actually created it and verified the artifact.

---

# Part G — Upgrade and Usability

## 39. Upgrade Testing

Test:

```text
Old version installed
        ↓
Install new version
        ↓
Restart WorkSidekick
```

Verify:

- Provider selection preserved
- Model settings preserved
- Feature preferences preserved
- Profile preserved
- Automatic toolbar setting preserved
- Global shortcut setting preserved
- Provider credentials accessible
- No duplicate tray process
- No stale helper or IPC resources

Do not perform destructive migration without backup/recovery design.

---

## 40. Startup and First-Run UX

First launch should make it reasonably clear how to use the application.

Minimum instructions:

```text
1. Choose an AI provider.
2. Save its API key securely.
3. Enter a model ID.
4. Select or copy text.
5. Use the toolbar or global shortcut.
6. Review AI output before copying.
```

Do not force a complicated onboarding wizard.

Use a small first-run explanation if useful.

Privacy must be understandable without reading source code.

---

## 41. User Documentation

Update README with platform-specific instructions.

Windows:

```text
Install
Launch
Configure provider
Enable shortcut
Enable automatic toolbar
Use selection actions
Troubleshoot UIA failures
Uninstall
```

Ubuntu:

```text
Install DEB
Launch
X11 vs Wayland differences
Enable shortcut
Portal fallback
AT-SPI troubleshooting
Secret Service/keyring
Uninstall
```

Provide exact commands only after verifying them.

---

# Part H — M6 Acceptance

## 42. Required Code/CI Gate

M6 must have:

- [ ] Windows Release build PASS
- [ ] Ubuntu Release build PASS
- [ ] All existing tests PASS
- [ ] Windows native integration tests PASS
- [ ] Linux native integration tests PASS
- [ ] No known severe regression
- [ ] No automatic AI upload
- [ ] No plaintext credential persistence
- [ ] No uncontrolled history/logging
- [ ] Stability test suite created and passing
- [ ] Package builds reproducibly through documented CI configuration
- [ ] Checksums generated

---

## 43. Required Packaging Gate

- [ ] Windows portable ZIP created
- [ ] Portable ZIP launches successfully
- [ ] Windows installer created
- [ ] Installer installs successfully
- [ ] Installer uninstalls successfully
- [ ] Windows upgrade preserves settings
- [ ] Ubuntu DEB created
- [ ] DEB dependencies inspected
- [ ] DEB installation verified
- [ ] Installed Linux application launches
- [ ] Installed Linux application uninstalls
- [ ] No secrets or development fixtures in packages

---

## 44. Required Real Desktop Gate

For full M6 PASS:

- [ ] At least Notepad + one browser verified on Windows 11
- [ ] Windows automatic toolbar focus verified in real usage
- [ ] Windows global shortcut verified in real usage
- [ ] Windows clipboard fallback verified
- [ ] At least one GNOME X11 text application verified, or a specifically documented environment blocker remains
- [ ] Native Wayland manual workflow verified where a Wayland environment is available
- [ ] Real desktop compatibility matrix updated honestly
- [ ] At least one nontrivial DPI/monitor configuration validated on Windows

If the necessary real desktop environment is unavailable, M6 may be:

```text
PARTIAL — CI and packaging complete, real desktop acceptance pending
```

Do not declare full PASS merely because the code builds.

A real environment limitation is not permission to fabricate a test result.

---

## 45. Stability Gate

Use a repeatable stress scenario.

Minimum:

```text
1,000 selection events

500 ActionBar cycles

200 shortcut activations

100 monitor restarts

100 cancellation tests

100 IPC restart/race cycles
```

Report:

```text
Pass/fail
Crash count
Hang count
Memory before/after
Handle/thread changes
Environment
Unresolved defects
```

Do not claim zero leaks without sufficient evidence.

---

## 46. Bug Severity

Classify defects:

**P0:** Data leakage, credential exposure, destructive behavior.

**P1:** Crashes, deadlocks, inability to start, broken core AI flow, repeated unauthorized request.

**P2:** Major app-specific incompatibility, focus loss, severe placement errors, broken packaging.

**P3:** Cosmetic/layout/documentation issues.

P0 and P1 must be resolved before a full release-candidate PASS.

P2 must be fixed or clearly scoped as unsupported with a viable fallback, and release acceptance must justify the exception.

P3 can remain documented.

---

# Part I — Implementation Plan

## 47. Suggested Implementation Order

### Phase 1 — Baseline and Evidence

- Lock M5 baseline.
- Inventory CI/tests/platform support.
- Record real desktop availability.
- Create M6 test matrix.
- Identify packaging prerequisites.

### Phase 2 — Compatibility

- Windows real application checks.
- GNOME X11 checks.
- GNOME Wayland fallback checks.
- Fix reproducible blockers.

### Phase 3 — Stability

- Stress tests.
- Native resource lifecycle.
- UIA worker recovery.
- Shortcut / IPC lifecycle.
- Memory and handles.

### Phase 4 — Packaging

- Windows portable ZIP.
- Windows installer.
- Ubuntu DEB.
- Clean install/uninstall.
- Upgrade verification.

### Phase 5 — Release Verification

- Windows CI.
- Ubuntu CI.
- Package smoke tests.
- Security/privacy review.
- Checksums.
- Release-candidate documentation.

Keep the main branch buildable throughout.

---

## 48. Documentation Deliverables

Create:

```text
docs/M6_SPEC.md

docs/m6-plan.md

docs/m6-completion.md

docs/m6-desktop-compatibility.md

docs/m6-stability-report.md

docs/m6-packaging-report.md

docs/m6-release-checklist.md
```

Update:

```text
README.md
docs/architecture.md
docs/privacy.md
docs/troubleshooting.md
```

Avoid redundant multi-page reports with identical content.

Provide concise evidence-backed summaries.

---

## 49. M6 Completion Report

Include:

```text
M6 Gate: PASS / PARTIAL / FAIL

Baseline SHA:
Final SHA:

Windows Release build:
Ubuntu Release build:

Windows CTest:
Ubuntu CTest:

M0 regression:
M1 regression:
M2 regression:
M3 regression:
M4 regression:
M5 regression:

Windows real desktop:
Windows app matrix:

GNOME X11:
GNOME Wayland:

Windows DPI:
Windows multi-monitor:

Stress cycles:
Crash count:
Hang count:
Resource growth:

Windows portable ZIP:
Windows installer:
Windows installation:
Windows upgrade:
Windows uninstallation:

Ubuntu DEB:
Ubuntu installation:
Ubuntu uninstallation:

Artifact checksums:

Privacy regression:
Credential storage:
Security findings:

P0 defects:
P1 defects:
P2 defects:
P3 defects:

Release candidate artifacts:

Known limitations:

Recommended next milestone:
M7 — Optional Enhancements / Production Feedback
```

Everything untested must be explicitly `NOT TESTED`.

---

## 50. Required Codex Stop Condition

After M6 implementation:

**STOP. Do not begin M7.**

Deliver:

1. Final commit SHA.
2. Windows and Ubuntu CI links.
3. Test results.
4. Downloadable release artifacts or exact artifact locations.
5. SHA-256 checksums.
6. Desktop compatibility matrix.
7. Packaging report.
8. Stability report.
9. Known limitations.
10. Explicit M6 Gate classification.

If real Windows or GNOME sessions cannot be accessed, finish every feasible CI/packaging task and report:

```text
M6 Gate = PARTIAL
Reason = Real-desktop acceptance remains unverified
```

Do not pause the entire milestone merely because one platform is unavailable.

Do not invent test evidence.

---

# Final M6 Objective

WorkSidekick should progress from:

```text
Cross-platform source code
        +
Native synthetic tests
```

to:

```text
Windows installer
        +
Windows portable package
        +
Ubuntu DEB
        +
Real desktop validation
        +
Long-running stability evidence
        +
Privacy/security review
        +
Reproducible release candidate
```

The key outcome is not simply more code.

**M6 succeeds when a user can install WorkSidekick on supported platforms, use its essential workflow reliably, and uninstall or upgrade it without losing control of their data or credentials.**
