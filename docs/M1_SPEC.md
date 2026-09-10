# WorkSidekick — M1 Implementation Specification

## Linux AT-SPI Selection Detection & Retrieval

Repository:

```text
https://github.com/wzk111/leadership-dandang-win-linux
```

Current baseline:

```text
main
baseline commit: 34259560044bfaa5df9d04e56ca3bf025037eb68
M0: ACCEPTED
```

Read the existing:

```text
PROJECT_SPEC.md
docs/m0-completion.md
docs/architecture.md
docs/linux-selection-compatibility.md
```

before modifying the project.

---

# 1. M0 Status

M0 is considered complete enough to proceed.

Do not redesign or rewrite the existing M0 architecture unless an actual regression or blocking design defect is found.

The existing M0 behavior must remain available:

```text
Clipboard
   ↓
Process Clipboard
   ↓
Plain Speak / Summarize / Polish
   ↓
OpenAI Responses API
   ↓
Result Card
   ↓
Copy
```

Existing secure key storage, OpenAI provider, workspace, result card and clipboard mode must continue working.

M1 extends M0.

It does not replace it.

---

# 2. M1 Goal

Implement Linux-native text selection detection and retrieval using AT-SPI2.

Target flow:

```text
User selects text in another application
              ↓
object:text-selection-changed
              ↓
AtSpiSelectionMonitor
              ↓
debounce
              ↓
AtSpiSelectionProvider
              ↓
selected text
source application
selection screen rectangle if available
              ↓
Selection
              ↓
Diagnostics
```

At the end of M1, WorkSidekick should be capable of detecting that a user selected text in an accessible application and retrieving that selection.

M1 is an integration/diagnostics milestone.

It is NOT yet the PopClip toolbar milestone.

---

# 3. Very Important Scope Boundary

Do NOT implement M2.

Specifically, do NOT implement:

```text
floating ActionBar
automatic toolbar popup
non-activating overlay
AI request on selection
global hotkey
X11 PRIMARY fallback
Ctrl+C injection
Ctrl+V injection
ydotool
uinput
Windows UI Automation
Windows hooks
reply/profile/full feature expansion
```

These belong to later milestones.

M1 should prove:

```text
Linux accessibility event
        +
selected text retrieval
        +
selection coordinates
```

reliably enough for M2 to build on.

---

# 4. Privacy Rule

Receiving an AT-SPI selection event must NEVER automatically call OpenAI.

The following is forbidden:

```text
selection event
     ↓
automatic API upload
```

Correct behavior:

```text
selection event
     ↓
local Selection object only
     ↓
Diagnostics / future toolbar
```

AI remains explicitly user-triggered.

Do not log actual selected text.

Allowed production log example:

```text
INFO atspi selection app=Firefox chars=184 anchor=yes
```

Forbidden:

```text
INFO selected_text="confidential company message..."
```

---

# 5. Existing Architecture to Preserve

Current shared structure already contains:

```cpp
struct Selection
{
    QString text;
    std::optional<QRect> anchorRect;
    QString sourceApplication;
};
```

and:

```cpp
class ISelectionProvider
{
public:
    virtual ~ISelectionProvider() = default;

    virtual std::optional<Selection>
    currentSelection() = 0;
};
```

Do not put AT-SPI types into `Selection`.

Never expose:

```text
AtspiAccessible*
AtspiText*
GError*
GObject*
```

outside the Linux platform implementation.

Platform-specific types must remain under:

```text
src/platform/linux/
```

---

# 6. New M1 Architecture

Introduce a selection monitor abstraction.

Suggested interface:

```cpp
class ISelectionMonitor : public QObject
{
    Q_OBJECT

public:
    explicit ISelectionMonitor(QObject* parent = nullptr)
        : QObject(parent) {}

    ~ISelectionMonitor() override = default;

    virtual bool start() = 0;
    virtual void stop() = 0;

    virtual bool isRunning() const = 0;
    virtual bool isAvailable() const = 0;

    virtual QString backendName() const = 0;
    virtual QString statusDescription() const = 0;

signals:
    void selectionDetected(const ws::Selection& selection);
    void selectionCleared();
    void statusChanged();
};
```

The exact interface may be adjusted if Qt ownership or testing benefits from another design.

Do not over-engineer it.

The important separation is:

```text
event monitoring
        ≠
selection extraction
```

---

# 7. Suggested New Files

Add approximately:

```text
src/platform/
├── ISelectionMonitor.h
│
└── linux/
    ├── AtSpiSelectionMonitor.h
    ├── AtSpiSelectionMonitor.cpp
    ├── AtSpiSelectionProvider.h
    ├── AtSpiSelectionProvider.cpp
    ├── AtSpiUtils.h
    └── AtSpiUtils.cpp
```

`AtSpiUtils` is optional.

Only introduce it if it genuinely improves ownership/error handling.

Add tests approximately:

```text
tests/
├── TestAtSpiSelection.cpp
└── TestSelectionMonitor.cpp
```

Names may be adjusted to fit the current repository style.

---

# 8. Build Dependency

Add AT-SPI development dependency.

Ubuntu:

```bash
sudo apt install libatspi2.0-dev
```

Use pkg-config:

```text
atspi-2
```

Update CMake in the Linux block.

Conceptually:

```cmake
pkg_check_modules(
    ATSPI
    REQUIRED
    IMPORTED_TARGET
    atspi-2
)
```

Linux platform target should link:

```text
PkgConfig::ATSPI
```

Do not add AT-SPI to Windows/shared core targets.

---

# 9. CI Update

Update:

```text
.github/workflows/ubuntu.yml
```

to install:

```text
libatspi2.0-dev
```

Existing M0 CI operations must remain:

```text
Release build
CTest
libsecret integration test
GUI smoke test
```

M1 tests must not break headless CI simply because there is no real Firefox/Chrome desktop session.

Real desktop compatibility and unit/integration tests are different gates.

---

# 10. AT-SPI Initialization

Use libatspi.

At startup of the AT-SPI backend:

```text
atspi_init()
```

should initialize connection to the accessibility registry.

Failure must not crash WorkSidekick.

If unavailable:

```text
AT-SPI: unavailable
```

should be reported in Diagnostics.

Clipboard M0 mode must continue working.

The application must degrade like:

```text
AT-SPI unavailable
        ↓
automatic selection detection unavailable
        ↓
manual clipboard M0 remains usable
```

---

# 11. Event Registration

Register an AT-SPI event listener for:

```text
object:text-selection-changed
```

Use the libatspi event listener API.

Conceptual flow:

```text
create AtspiEventListener
       ↓
register "object:text-selection-changed"
       ↓
receive AtspiEvent
       ↓
event->source
       ↓
schedule debounced extraction
```

Do not register broad unnecessary event families.

For example, do NOT register every:

```text
object:event
keyboard event
mouse event
text-changed event
caret event
```

unless there is a demonstrated requirement.

M1 only needs text selection changes.

---

# 12. Event Callback Requirements

The AT-SPI callback must remain lightweight.

Do NOT perform long blocking accessibility traversal directly inside the callback.

Callback responsibilities should roughly be:

```text
receive event
      ↓
validate source
      ↓
retain source safely if needed
      ↓
record candidate
      ↓
restart debounce timer
      ↓
return
```

If an `AtspiAccessible*` must survive after the callback returns, take an explicit GObject reference and release it after processing.

Do not keep stale accessible objects indefinitely.

---

# 13. Debounce

Selection events may fire multiple times while the user is dragging.

Example:

```text
H
He
Hel
Hell
Hello
```

Do not emit five final WorkSidekick selections.

Use a single-shot debounce.

Recommended initial value:

```text
80 ms
```

Acceptable tuning range:

```text
50–120 ms
```

Flow:

```text
selection event
      ↓
restart timer
      ↓
selection event
      ↓
restart timer
      ↓
80 ms quiet
      ↓
retrieve final selection
```

Do not use:

```cpp
sleep()
usleep()
```

on the Qt GUI thread.

Use:

```text
QTimer
```

or an appropriate event-driven mechanism.

---

# 14. AT-SPI Selection Extraction

For the event source:

```text
AtspiAccessible
```

determine whether it exposes the Text interface.

Use the supported text interface rather than scanning the whole accessibility tree.

Conceptually:

```text
event source
     ↓
Text interface available?
     ↓ yes
get number of selections
     ↓
for active selection
     ↓
get start/end offsets
     ↓
get text
```

Relevant API concepts:

```text
atspi_accessible_get_text_iface()
atspi_text_get_n_selections()
atspi_text_get_selection()
atspi_text_get_text()
```

Always check recoverable errors.

A failure for one application is not fatal to WorkSidekick.

---

# 15. Multiple Selections

AT-SPI can theoretically expose more than one active selection.

For M1:

Prefer the first valid non-empty selection.

Record selection count in diagnostics/debug metadata if useful.

Do not invent complicated multi-selection concatenation behavior yet.

If no non-empty selection exists:

```text
selectionCleared()
```

may be emitted.

---

# 16. Text Validation

After retrieving text:

```text
trim for emptiness check
```

but preserve the original text content in `Selection.text`.

Do not silently normalize or rewrite user content.

Apply a reasonable protection limit.

The existing AI layer currently rejects >100,000 characters.

M1 should avoid caching unreasonable accessibility payloads.

Suggested selection retrieval limit:

```text
100,000 characters
```

If the range is larger:

do not call the AI.

Diagnostics may report:

```text
Selection too large
```

Do not crash or allocate unbounded memory.

---

# 17. Source Application

Populate:

```cpp
Selection::sourceApplication
```

using the AT-SPI application's accessible information.

Preferred result examples:

```text
Firefox
Google Chrome
code
Slack
gedit
gnome-terminal
```

Use the containing application accessible where appropriate.

Do not recursively crawl the desktop tree merely to derive an application name.

Failure to obtain a name is not fatal.

Fallback:

```text
Unknown application
```

or empty string.

---

# 18. Selection Rectangle

M1 should attempt to populate:

```cpp
Selection::anchorRect
```

using AT-SPI screen coordinates.

Preferred coordinate type:

```text
ATSPI_COORD_TYPE_SCREEN
```

For the selected range, attempt:

```text
atspi_text_get_range_extents()
```

using:

```text
startOffset
endOffset
ATSPI_COORD_TYPE_SCREEN
```

Convert valid:

```text
x
y
width
height
```

to:

```cpp
QRect
```

Only set `anchorRect` when coordinates are meaningful.

Reject obviously invalid rectangles such as:

```text
negative width
negative height
zero-sized unusable rectangle
nonsensical extreme coordinate values
```

A missing rectangle must NOT cause the text retrieval itself to fail.

This distinction is important:

```text
text success + rectangle failure
```

is still an M1 success.

---

# 19. Coordinate Semantics

Store the rectangle using the screen coordinate values returned by AT-SPI.

Do not prematurely transform them for Qt popup positioning in M1.

M2 will own toolbar-position conversion and multi-monitor behavior.

M1 should record enough information to determine whether AT-SPI returns usable screen geometry.

---

# 20. Error Ownership / RAII

libatspi uses GLib/GObject ownership.

Do not scatter manual cleanup throughout business code.

Handle correctly:

```text
GError*
gchar*
AtspiRange*
AtspiRect*
GObject references
AtspiEventListener*
```

Prefer small C++ RAII wrappers/deleters where they materially improve safety.

Do not introduce a giant wrapper framework.

No leaks should occur during repeated text selections.

---

# 21. Qt / GLib Event Loop

AT-SPI event delivery depends on GLib/D-Bus event processing.

Do not assume callbacks work simply because registration returned success.

Verify event delivery.

Use a design that integrates safely with the Qt application.

Avoid calling:

```text
atspi_event_main()
```

directly on the Qt GUI thread because it owns a blocking event loop.

If the normal Qt/GLib integration on Ubuntu is used, verify it experimentally.

If a dedicated GLib event context/thread is required, encapsulate it entirely inside the Linux AT-SPI monitor.

Do not allow cross-thread UI access.

Any signal consumed by QWidget/UI code must arrive safely through Qt queued event delivery where necessary.

Document the final strategy in:

```text
docs/architecture.md
```

---

# 22. Runtime State

The monitor should have clear states.

Conceptually:

```text
Unavailable
Stopped
Starting
Running
Error
```

Diagnostics should be able to answer:

```text
AT-SPI available?
initialized?
listener registered?
listener running?
last event received?
last selection successful?
```

Avoid reporting:

```text
AT-SPI available = yes
```

solely because the library was linked.

Runtime accessibility registry availability matters.

---

# 23. Application Integration

M1 must NOT replace the current:

```text
ClipboardSelectionProvider
```

used for the M0 explicit AI workflow.

M0 AI actions should continue using the explicitly processed clipboard.

In M1, AT-SPI data is initially used for:

```text
Diagnostics
compatibility testing
architecture validation
```

not automatic AI requests.

Therefore architecture may temporarily be:

```text
ClipboardSelectionProvider
        ↓
AppController
        ↓
AI

AtSpiSelectionMonitor
        ↓
Diagnostics
```

This is intentional.

M2 will connect the AT-SPI selection into the toolbar workflow.

---

# 24. Application Ownership

`Application` should own or receive the Linux selection monitor through an abstraction rather than constructing AT-SPI implementation details throughout UI code.

Prefer something conceptually like:

```text
PlatformFactory
    ├── createSecretStore()
    └── createSelectionMonitor()
```

If a factory extension is clean and small, use it.

Avoid:

```cpp
#ifdef Q_OS_LINUX
#include <atspi/...>
#endif
```

inside:

```text
Application.cpp
WorkspaceWindow.cpp
AppController.cpp
Core.cpp
```

AT-SPI stays in the Linux platform layer.

---

# 25. Diagnostics Upgrade

Current M0 diagnostics says approximately:

```text
AT-SPI availability: not probed (M1)
AT-SPI listener: not implemented (M1)
```

Replace this with real runtime information.

Example:

```text
WorkSidekick 0.2 — M1

Platform: Linux
Desktop: GNOME
Session: wayland

AT-SPI library: available
AT-SPI registry: connected
AT-SPI listener: running

Events received: 14

Last selection:
Application: Firefox
Characters: 184
Text retrieved: yes
Anchor available: yes
Anchor: x=820 y=422 w=311 h=38

AI triggered automatically: NO
Selection mode for AI: MANUAL CLIPBOARD
```

Do NOT display actual selected text by default.

---

# 26. Explicit Selection Preview

For M1 development, Diagnostics needs a way to verify that the actual retrieved text is correct.

Add an explicit user-controlled diagnostic action such as:

```text
Show Last Selection Preview
```

or:

```text
Test AT-SPI Selection
```

Only after the user explicitly clicks it may Diagnostics display the last selected text.

Requirements:

```text
not logged
not persisted
not uploaded
not copied automatically
```

Clearly label it:

```text
Local diagnostic preview — not sent to AI
```

This preserves M1 testability without turning Diagnostics into passive text surveillance.

---

# 27. Cache Lifetime

Do not build selection history.

Keep at most:

```text
latest selection
```

in memory.

Clear it when:

```text
selection becomes empty
monitor stops
application exits
```

Optionally clear stale selected text after a short period such as:

```text
30–60 seconds
```

if doing so does not complicate M1 unnecessarily.

Never persist the selection to disk.

---

# 28. Diagnostics "Test Selection"

The existing:

```text
Test Selection
```

button currently says selection capture is unavailable in M0.

Update it for M1.

Suggested behavior:

```text
click Test Selection
      ↓
show metadata for latest AT-SPI selection
      ↓
optionally show local preview
```

If no AT-SPI selection has been detected:

```text
No AT-SPI text selection detected yet.
Select text in another accessible application and try again.
```

Do not fall back to clipboard inside this button.

The point is specifically to validate AT-SPI.

---

# 29. Unit-Test Architecture

Do not require a real Firefox instance for ordinary CI unit tests.

Extract enough logic so selection handling can be tested with controlled/mocked inputs.

Test at minimum:

```text
empty selection
single valid selection
multiple selection → first valid one
text without rectangle
text with rectangle
invalid rectangle ignored
oversized selection
accessibility error
event debounce
duplicate rapid events
selection cleared
monitor start/stop idempotence
```

Tests must verify:

```text
AT-SPI selection event never invokes AI provider
```

This privacy invariant is important.

---

# 30. Integration Test Strategy

Where practical, add an AT-SPI integration test using a small synthetic accessible application/widget.

Do not make CI depend on:

```text
Firefox
Chrome
Slack
external network
real OpenAI
```

If the Ubuntu CI environment cannot provide a functional accessibility bus, separate the test into:

```text
unit test: required
AT-SPI runtime integration: best effort / clearly reported
real desktop application test: manual
```

Do not fake a passing real-desktop result.

---

# 31. CI Behavior

The following must still pass:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
xvfb-run -a ctest --test-dir build --output-on-failure
dbus-run-session -- bash scripts/test-keyring.sh
xvfb-run -a ./build/worksidekick --smoke-test
```

Add new tests without regressing these.

If AT-SPI runtime integration requires another invocation, document and add it only when reproducible.

---

# 32. Real Desktop Compatibility Matrix

Update:

```text
docs/linux-selection-compatibility.md
```

Use these columns:

```text
Environment / Application
Session
Selection event detected
Text retrieved
Text correct
Anchor available
Anchor reasonable
Fallback required
Notes
```

Target applications:

```text
GNOME Text Editor / gedit
Firefox
Chrome / Chromium
VS Code
GNOME Terminal
Slack or another Electron application
```

Test separately where possible on:

```text
GNOME X11
GNOME Wayland
```

Do not infer results.

If not tested, write:

```text
NOT TESTED
```

---

# 33. Expected Compatibility Reality

Do not try to "fix" every application during M1.

Expected result may look like:

```text
gedit                 event/text/rect works
Firefox               event/text works
Chrome                partial
VS Code               text works, geometry varies
Terminal              implementation-specific
Slack/Electron        partial or unavailable
```

These examples are NOT expected results and must not be copied into the matrix as facts.

Measure actual behavior.

The purpose of M1 is to discover this compatibility landscape.

---

# 34. Wayland

M1 must work through AT-SPI only.

Do not add:

```text
ydotool
xdotool
uinput
global pointer hooks
clipboard injection
```

for Wayland.

AT-SPI is independent of the M3 fallback strategy.

Record:

```text
XDG_SESSION_TYPE
```

in diagnostics and compatibility results.

---

# 35. X11

Even if running under X11, M1 must still test AT-SPI.

Do not implement:

```text
X11 PRIMARY
XInput2 gesture monitor
```

yet.

Those belong to M3.

This gives us clean measurements of:

```text
AT-SPI alone
```

before fallback layers obscure failures.

---

# 36. No Floating Toolbar Yet

This is important.

When text selection is detected:

DO NOT display the future PopClip-style toolbar.

At most update Diagnostics internally.

Reason:

M2 needs separate work for:

```text
focus preservation
non-activating Qt windows
GNOME compositor differences
selection lifetime
screen positioning
multi-monitor support
```

Mixing that into M1 would make debugging much harder.

---

# 37. Version / Diagnostics Label

Update application milestone identification from:

```text
M0
```

to:

```text
M1
```

where appropriate.

Do not necessarily declare a public semantic release yet.

It is acceptable to keep application version:

```text
0.1.x
```

or move to:

```text
0.2.0
```

if consistent across CMake/application/docs.

Do not spend significant time on release packaging.

---

# 38. Documentation

Update:

```text
README.md
docs/architecture.md
docs/linux-selection-compatibility.md
docs/troubleshooting.md
```

Add:

```text
docs/m1-completion.md
```

README should explain:

```text
M0 manual clipboard mode still works
M1 adds local AT-SPI detection
selection alone does not call AI
automatic floating toolbar is still not implemented
Wayland/X11 compatibility depends on application accessibility support
```

---

# 39. Troubleshooting

Add practical diagnostics for:

```text
AT-SPI registry unavailable
event listener registration failure
application exposes no Text interface
selection count = 0
text retrieval error
selection retrieved but no geometry
```

Useful commands may be documented if verified.

Do not tell users to disable Wayland or install privileged input injection tools for M1.

---

# 40. Security Review

Before completing M1, verify:

```text
no selected text in logs
no selection persistence
no automatic AI call from accessibility event
no API key regression
no new root requirement
no keylogging
no general text-change monitoring
no screenshot capture
```

Only:

```text
object:text-selection-changed
```

should be needed for the core M1 monitoring path.

---

# 41. Performance

Selection detection must be event driven.

Do NOT periodically scan:

```text
desktop accessibility tree
focused application tree
all accessible objects
```

Targets:

```text
idle CPU approximately unchanged from M0
no high-frequency polling
debounced selection processing
no blocking GUI thread
```

Repeated selection should not leak memory.

---

# 42. M1 Acceptance Criteria

M1 is complete when all of the following code-level gates are satisfied:

```text
Ubuntu 22.04 Release build passes

existing M0 tests pass

new AT-SPI tests pass

AT-SPI runtime availability is detected

object:text-selection-changed listener is registered when available

selection text can be extracted from event source

source application can be recorded where exposed

selection screen rectangle is attempted

rectangle failure does not invalidate text success

rapid events are debounced

selection text is not logged

selection is not uploaded automatically

M0 manual clipboard AI flow still works

Diagnostics exposes real M1 status

compatibility matrix updated honestly

documentation updated
```

Real desktop applications that cannot be tested in the current environment must be explicitly marked:

```text
NOT TESTED
```

---

# 43. M1 Success Definition

The important M1 Gate is NOT:

```text
works in every Linux application
```

The important Gate is:

```text
AT-SPI backend implemented correctly
            +
event-driven selection detection works
            +
selected text retrieval works in supported accessible apps
            +
geometry retrieval is attempted
            +
unsupported apps fail gracefully
            +
M0 remains intact
```

---

# 44. Do Not Proceed to M2 Automatically

After M1 implementation:

STOP.

Do not start the floating toolbar.

Produce:

```text
docs/m1-completion.md
```

and report results.

The M1 completion report must include:

```text
baseline commit
final commit
files added/modified
AT-SPI architecture
Qt/GLib event-loop strategy
build result
test result
CI result
selection event result
text retrieval result
rectangle retrieval result
X11 tests
Wayland tests
per-application compatibility table
known limitations
memory/ownership considerations
privacy review
recommended M2 design
```

If something could not be tested:

```text
NOT TESTED
```

Do not infer success.

---

# 45. Required Codex Final Response

When finished, respond with a concise summary containing:

```text
M1 Gate = PASS / PARTIAL / FAIL

Build:
Tests:
CI:
AT-SPI initialization:
Listener:
Selection text:
Selection rectangle:
X11:
Wayland:
Privacy regression:
M0 regression:

Known blockers:

Recommended next milestone:
M2 — Non-activating floating ActionBar
```

Do not proceed to M2 unless explicitly instructed.

---

# Final M1 Data Flow

Target architecture after this milestone:

```text
                  ┌─────────────────────┐
                  │    Other Linux App  │
                  │ Firefox / VS Code   │
                  │ Chrome / gedit ...  │
                  └──────────┬──────────┘
                             │
                 text selection changed
                             │
                             ▼
              ┌──────────────────────────┐
              │ AtSpiSelectionMonitor    │
              │                          │
              │ object:text-selection-   │
              │ changed                  │
              └────────────┬─────────────┘
                           │
                       debounce
                           │
                           ▼
              ┌──────────────────────────┐
              │ AtSpiSelectionProvider   │
              │                          │
              │ text                     │
              │ source application       │
              │ screen rectangle         │
              └────────────┬─────────────┘
                           │
                           ▼
                    ws::Selection
                           │
                           ▼
                    Diagnostics
                           │
                  NO automatic AI call


Existing M0 path remains:

ClipboardSelectionProvider
          │
          ▼
    AppController
          │
          ▼
      AI Provider
          │
          ▼
      Result Card
```

M1 should establish a clean, tested Linux accessibility foundation for M2 without prematurely implementing UI overlay behavior.
