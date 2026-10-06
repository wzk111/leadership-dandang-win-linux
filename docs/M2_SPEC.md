# WorkSidekick — M2 Implementation Specification

## Non-Activating Floating ActionBar

Repository:

```text
https://github.com/wzk111/leadership-dandang-win-linux
```

Current accepted baseline:

```text
main
commit: 5f9a65cca55c4f1f1a6b6b9c1d7ed00f4db32316

M0 = PASS
M1 = PASS
```

Read before implementation:

```text
PROJECT_SPEC.md
docs/M1_SPEC.md
docs/m1-completion.md
docs/m1-architecture.md
docs/linux-selection-compatibility.md
```

M2 builds directly on the M1 AT-SPI selection monitor.

Do not redesign M0/M1 unless an actual blocking defect is discovered.

---

# 1. M2 Goal

Implement the first real PopClip-style interaction.

Target full-capability flow:

```text
User selects text in another application
                ↓
AT-SPI
object:text-selection-changed
                ↓
AtSpiSelectionMonitor
                ↓
Selection {
    text
    sourceApplication
    anchorRect
}
                ↓
ActionBar appears near selected text
                ↓
┌──────────────────────────────────┐
│ Plain Speak │ Summarize │ Polish │
└──────────────────────────────────┘
                ↓
user explicitly clicks feature
                ↓
captured Selection snapshot
                ↓
PromptBuilder
                ↓
OpenAI
                ↓
ResultCard
                ↓
Copy result
```

The critical UX requirements are:

```text
automatic local toolbar
no automatic AI request
no clipboard requirement
source text snapshot preserved
source application focus preserved where platform permits
```

---

# 2. M2 Scope

M2 implements:

```text
ActionBar UI
automatic ActionBar display from M1 selection events
explicit AI actions using AT-SPI Selection
selection-based toolbar placement
non-activating window behavior
automatic-popup setting
toolbar hide/update behavior
X11/XWayland anchored positioning
Wayland capability-aware degradation
focus-preservation testing
privacy regression testing
```

M2 does NOT implement:

```text
global shortcut
X11 PRIMARY fallback
Ctrl+C input injection
Ctrl+V replacement
ydotool
uinput
Windows backend
Reply Composer
Profile
Relevance
Add Insight
Gemini
Claude
full final feature set
```

Those remain later milestones.

---

# 3. Core M2 Principle

The M1 AT-SPI selection is already captured before the ActionBar is clicked.

Therefore:

DO NOT try to retrieve selected text again when the user clicks a toolbar button.

DO NOT perform:

```text
toolbar click
    ↓
query accessibility again
```

and do NOT perform:

```text
toolbar click
    ↓
Ctrl+C
```

Instead:

```text
selection event
    ↓
immutable Selection snapshot
    ↓
ActionBar owns snapshot
    ↓
user clicks action
    ↓
that exact snapshot is sent to AppController
```

This avoids focus/selection lifetime problems.

---

# 4. Existing M1 Selection Model

Reuse:

```cpp
struct Selection
{
    QString text;
    std::optional<QRect> anchorRect;
    QString sourceApplication;
};
```

Do not modify it unless absolutely necessary.

M2 should treat each `Selection` supplied by the monitor as a snapshot.

---

# 5. AppController Extension

Add an explicit path for user-approved AT-SPI selections.

Recommended API:

```cpp
bool AppController::runSelection(
    Feature feature,
    const Selection& selection);
```

Implementation should eventually route into existing prompt/request logic.

Conceptually:

```cpp
bool AppController::runSelection(
    Feature feature,
    const Selection& selection)
{
    if (selection.text.trimmed().isEmpty())
        return false;

    return runText(feature, selection.text);
}
```

Do not expose unnecessary internals.

The important distinction is:

```text
captureClipboard()
```

remains the M0 manual mode.

New:

```text
runSelection()
```

means:

> the user explicitly clicked a feature for an already captured local AT-SPI selection.

Both eventually use the same:

```text
PromptBuilder
AI provider
ResultCard
```

pipeline.

---

# 6. Privacy Boundary

This invariant must remain true:

```text
selectionDetected
      ↓
NO AI
```

Only:

```text
selectionDetected
      ↓
ActionBar appears locally
      ↓
USER CLICK
      ↓
AI request
```

is allowed.

Add tests proving this.

Automatic popup is NOT consent to upload.

Feature button click is the explicit consent boundary.

---

# 7. ActionBar Class

Create approximately:

```text
src/ui/ActionBar.h
src/ui/ActionBar.cpp
```

Recommended API:

```cpp
class ActionBar : public QWidget
{
    Q_OBJECT

public:
    explicit ActionBar(QWidget* parent = nullptr);

    void showForSelection(
        const Selection& selection,
        const QPoint& position);

    void updateSelection(
        const Selection& selection,
        const QPoint& position);

    void hideBar();

    std::optional<Selection>
    currentSelection() const;

signals:
    void featureChosen(
        Feature feature,
        const Selection& selection);

    void dismissed();
};
```

Exact naming may change if a cleaner Qt API is appropriate.

Do not let ActionBar call OpenAI directly.

---

# 8. ActionBar Initial Features

Use only the already implemented M0 features:

```text
Plain Speak
Summarize
Polish
```

Do NOT implement the full future product toolbar yet.

Buttons should come from:

```text
FeatureRegistry
```

rather than hard-coded duplicated metadata.

---

# 9. ActionBar Visual Requirements

Initial ActionBar should be:

```text
compact
frameless
always on top
single row
minimal padding
rounded background if straightforward
clear hover states
fast to appear
```

Conceptually:

```text
┌───────────────────────────────────┐
│ Plain Speak │ Summarize │ Polish  │
└───────────────────────────────────┘
```

Do not spend excessive development time on visual polish.

Correct interaction matters more than aesthetics in M2.

---

# 10. Window Flags

Start with appropriate Qt flags such as:

```cpp
Qt::Tool
Qt::FramelessWindowHint
Qt::WindowStaysOnTopHint
Qt::WindowDoesNotAcceptFocus
```

and attributes such as:

```cpp
Qt::WA_ShowWithoutActivating
```

where appropriate.

However:

Do not assume that setting flags means the desired behavior has been proven.

M2 must TEST actual focus behavior.

---

# 11. Platform Window Policy

Introduce a small platform abstraction for ActionBar behavior.

Suggested concept:

```cpp
enum class OverlayCapability
{
    AnchoredNonActivating,
    NonActivatingUnanchored,
    Unsupported
};
```

and approximately:

```cpp
class IPlatformWindowPolicy
{
public:
    virtual ~IPlatformWindowPolicy() = default;

    virtual OverlayCapability
    overlayCapability() const = 0;

    virtual bool configureActionBar(
        QWidget& window) = 0;

    virtual bool positionActionBar(
        QWidget& window,
        const QRect& selectionRect) = 0;

    virtual QString
    description() const = 0;
};
```

Keep it small.

Do not build a giant generic window-management framework.

---

# 12. Linux Backend Detection

Do not determine positioning capability only using:

```text
XDG_SESSION_TYPE
```

Also inspect:

```cpp
QGuiApplication::platformName()
```

Examples:

```text
xcb
wayland
wayland-egl
```

This distinction matters because a Wayland desktop can still run WorkSidekick through XWayland using Qt's `xcb` backend.

Diagnostics should report both:

```text
Desktop session
Qt platform backend
Overlay capability
```

---

# 13. X11 / XWayland Mode

If Qt is running through:

```text
xcb
```

M2 should attempt the full experience:

```text
AT-SPI selection
       ↓
anchorRect
       ↓
calculate toolbar position
       ↓
show non-activating ActionBar
```

This is the primary M2 implementation target.

---

# 14. Wayland Rule

For native Qt Wayland:

```text
QGuiApplication::platformName() = wayland...
```

DO NOT assume arbitrary top-level absolute positioning works.

Per this project specification, treat anchored top-level positioning as unsupported unless runtime testing proves otherwise through an appropriate standard mechanism.

Do NOT add compositor-specific hacks.

Do NOT use:

```text
ydotool
uinput
GNOME Shell extension
wmctrl
xdotool
privileged helper
private Wayland protocol hack
```

to fake compatibility.

---

# 15. Native Wayland Degradation

If native Wayland cannot safely position the ActionBar relative to the AT-SPI rectangle:

do NOT pretend that the anchored PopClip experience is supported.

Preferred M2 behavior:

```text
AT-SPI selection detection continues
            ↓
automatic anchored ActionBar disabled
            ↓
Diagnostics explains limitation
            ↓
M0 manual clipboard workflow remains available
```

Optionally implement an experimental compositor-managed unanchored ActionBar only if it:

```text
does not steal focus
is not misleading
works reliably
is clearly reported as UNANCHORED
```

But it is NOT required for M2 PASS.

Do not let optional Wayland work jeopardize the X11 implementation.

M3 will introduce better manual trigger/fallback behavior.

---

# 16. Placement Algorithm

For anchored-capable backends:

Input:

```text
selectionRect
toolbarSize
available screen geometry
```

Preferred initial placement:

```text
centered horizontally above selection
```

Example:

```text
                ActionBar
        ┌─────────────────────┐
        │ Plain │ Summary │...│
        └─────────────────────┘

        selected text selected text
        └──────────────────────────┘
```

Suggested gap:

```text
6–10 px
```

If insufficient space above:

```text
place below selection
```

Then clamp horizontally and vertically to the selected monitor's usable geometry.

---

# 17. Screen Selection

Determine target `QScreen` from the center of:

```text
Selection::anchorRect
```

where possible.

Then use that screen's:

```cpp
availableGeometry()
```

to clamp placement.

Fallback order:

```text
anchor screen
      ↓
screenAt(anchor center)
      ↓
primaryScreen
```

Do not assume a single monitor.

---

# 18. Missing Anchor Rectangle

AT-SPI text retrieval may succeed while geometry fails.

M1 explicitly allows this.

M2 must not discard the selected text.

If:

```text
Selection.text = valid
anchorRect = nullopt
```

then:

For X11:

```text
do not show an incorrectly positioned automatic toolbar
```

unless a reliable alternative anchor is available.

Keep the selection locally cached.

Diagnostics should report:

```text
Selection detected but automatic ActionBar unavailable:
no anchor rectangle
```

M3 can later provide manual invocation.

---

# 19. HiDPI Rule

M1 has not proven AT-SPI → Qt coordinate equivalence under:

```text
HiDPI
mixed DPI
multiple monitors
fractional scaling
```

Do not silently invent complicated scaling heuristics.

M2 should:

```text
preserve raw AT-SPI rectangle
calculate Qt placement through one isolated placement component
record placement metadata in Diagnostics
```

If additional conversion is required by real tests, implement it with documented evidence.

Do not claim HiDPI support until tested.

---

# 20. ActionBar Snapshot Ownership

When:

```text
selectionDetected(selectionA)
```

arrives:

ActionBar stores:

```text
selectionA
```

If a later event gives:

```text
selectionB
```

before the user clicks:

replace the ActionBar snapshot with:

```text
selectionB
```

Click must use:

```text
selectionB
```

not a pointer/reference to mutable monitor state.

Use value semantics.

---

# 21. Toolbar Update Behavior

On new valid selection:

```text
new Selection
      ↓
replace snapshot
      ↓
recalculate position
      ↓
move existing ActionBar
```

Prefer reusing one ActionBar instance.

Do not destroy/recreate a top-level window after every selection unless required.

This reduces flickering.

---

# 22. Selection Cleared

When M1 emits:

```text
selectionCleared()
```

ActionBar should hide.

Also clear its cached Selection.

Do not leave stale text associated with an invisible toolbar.

---

# 23. Selection Expiry

M1 already expires cached selection after approximately:

```text
45 seconds
```

When that causes:

```text
selectionCleared()
```

ActionBar must disappear.

Do not create a second independent long-lived selection history.

---

# 24. Explicit Dismiss

Provide a dismiss control if useful:

```text
×
```

or allow Escape if achievable without stealing focus.

For M2, clicking dismiss should:

```text
hide current ActionBar
```

It should NOT necessarily disable automatic popup permanently.

Permanent enable/disable belongs to Settings/tray menu.

---

# 25. Automatic Popup Setting

Add persisted setting:

```text
Automatic selection toolbar
```

Conceptually:

```text
ui/automaticPopup = true/false
```

Use QSettings because this is not sensitive.

Recommended initial default:

```text
enabled on anchored-capable backend
disabled/no-op on unsupported backend
```

If simpler and safer during M2 development, default OFF is acceptable provided the user can enable it and the behavior is documented.

Be consistent.

---

# 26. Tray Menu

Add:

```text
Enable Automatic Toolbar
```

as a checkable tray action.

It should stay synchronized with Settings.

Disabling it:

```text
hides current ActionBar
```

but does NOT stop M1 AT-SPI Diagnostics monitoring.

The monitor is still useful for diagnostics and future trigger modes.

---

# 27. Action Click Flow

Correct flow:

```text
ActionBar contains Selection snapshot
        ↓
user clicks "Polish"
        ↓
copy Selection snapshot locally
        ↓
hide ActionBar
        ↓
AppController::runSelection(
    Feature::Polish,
    snapshot)
        ↓
loading ResultCard
        ↓
AI
```

The network request must contain the Selection snapshot.

It must NOT use whatever currently happens to be in the clipboard.

---

# 28. Clipboard Independence Test

This must be explicitly tested.

Example:

```text
AT-SPI selected text:
    "SOURCE FROM ATSPI"

Clipboard contains:
    "UNRELATED CLIPBOARD"

User clicks Plain Speak.
```

Request must contain:

```text
SOURCE FROM ATSPI
```

and must NOT contain:

```text
UNRELATED CLIPBOARD
```

The clipboard must remain unchanged until the user explicitly chooses:

```text
Copy result
```

---

# 29. Focus Preservation Goal

When ActionBar appears automatically:

the source application should remain active wherever the platform/window manager allows this behavior.

Full-capability expectation:

```text
Firefox focused
      ↓
select text
      ↓
ActionBar appears
      ↓
Firefox remains foreground/focused
```

Do not call:

```cpp
activateWindow()
raise()
requestActivate()
setFocus()
```

on the ActionBar during automatic display unless required and justified.

Showing and raising must be separated from activation.

---

# 30. Clicking Without Keyboard Focus

ActionBar buttons should respond to mouse clicks even though the ActionBar does not request keyboard focus.

Test this.

If Qt alone cannot provide the desired combination:

```text
clickable
+
does not become keyboard-focused
+
stays above
```

the Linux window policy may use narrowly scoped native X11 behavior.

Keep that code inside:

```text
src/platform/linux/
```

---

# 31. Native X11 Adjustment

Do not start with raw X11 code unnecessarily.

First test normal Qt flags on the `xcb` backend.

Only if required, add minimal X11 integration.

Do not scatter X11 calls throughout UI classes.

Potential X11-specific behavior belongs in:

```text
LinuxWindowPolicy
```

or equivalent.

---

# 32. ResultCard Behavior

Do not redesign ResultCard during M2.

Once the user clicks an ActionBar feature, the selection snapshot is already safe.

Therefore ResultCard may behave as a normal application window.

It may activate normally.

Existing:

```text
Loading
Success
Error
Cancel
Copy
Close
```

behavior should remain.

---

# 33. Busy State

If an AI request is already running:

ActionBar must not start another concurrent request accidentally.

Possible behavior:

```text
disable feature buttons
```

or:

```text
hide toolbar and show existing busy state
```

Reuse existing `AppController::busy()` semantics.

Do not create a second overlapping request queue in M2.

---

# 34. AI Failure

If the ActionBar user action fails because of:

```text
missing API key
missing model
network failure
rate limit
timeout
```

reuse existing ResultCard/error handling.

Do not duplicate AI error logic inside ActionBar.

---

# 35. Monitor Failure During Runtime

If AT-SPI stops/unavailable:

```text
hide ActionBar
clear cached snapshot
update Diagnostics
```

M0 manual clipboard mode remains usable.

No crash.

---

# 36. Diagnostics Upgrade

Extend Diagnostics with M2 fields such as:

```text
Automatic toolbar: enabled
Overlay capability: AnchoredNonActivating
Qt platform: xcb

Last selection anchor:
x=...
y=...
w=...
h=...

Last toolbar placement:
x=...
y=...
w=...
h=...

Placement mode:
ABOVE_SELECTION

Source focus preserved:
NOT TESTED / yes / no
```

Do not include selected text automatically.

---

# 37. Wayland Diagnostics

Example native Wayland status:

```text
Qt platform: wayland
AT-SPI monitor: running
Selection retrieval: available
Anchored ActionBar: unsupported on current backend
Automatic toolbar: inactive
Fallback: manual clipboard mode
```

This is preferable to falsely displaying:

```text
Automatic toolbar: working
```

when the compositor controls placement.

---

# 38. ActionBar Tests

Create unit/UI tests for:

```text
show with selection
snapshot stored
new selection replaces snapshot
selection clear hides bar
dismiss hides bar
disabled automatic popup prevents show
enabled popup allows show
feature button emits correct Feature
feature button emits correct Selection snapshot
bar does not directly know about AI provider
```

Use controlled fake placement policy where helpful.

---

# 39. Privacy Integration Test

Add an integration test equivalent to:

```text
fake monitor emits selection
        ↓
ActionBar appears
        ↓
HTTP requests == 0
```

Then:

```text
user clicks Plain Speak
        ↓
HTTP requests == 1
```

Also verify:

```text
request body contains AT-SPI text
clipboard value unchanged
```

This is one of the most important M2 tests.

---

# 40. Stale Selection Race Test

Test:

```text
selection A
    ↓
ActionBar shown

selection B
    ↓
ActionBar updated

click Polish
```

Exactly:

```text
selection B
```

must be sent.

Never A.

---

# 41. Clear-before-click Race

Test:

```text
selection A
    ↓
ActionBar shown

selectionCleared
    ↓
bar hidden

attempt feature execution
```

No AI request should occur from the stale selection.

---

# 42. M0 Regression

Continue testing:

```text
manual clipboard
      ↓
Process Clipboard
      ↓
feature
      ↓
AI
      ↓
result
```

M0 path must remain available independently of ActionBar.

---

# 43. M1 Regression

Continue testing:

```text
AT-SPI initialization
listener registration
real synthetic selection event
selection retrieval
source app name
rectangle
debounce
stop/start
unavailable registry
```

Do not weaken M1 tests to make M2 pass.

---

# 44. X11 Integration Test

Extend CI's Xvfb integration test where practical.

Target:

```text
external accessible fixture
      ↓
select synthetic text
      ↓
AT-SPI event reaches WorkSidekick
      ↓
ActionBar appears
      ↓
placement is based on anchor
```

If feasible, verify X11 input focus before and after automatic ActionBar display.

The focused external application/window should remain unchanged after display.

Use X11 APIs only in the integration test or Linux policy where needed.

Do not require a real Firefox installation in CI.

---

# 45. Focus Test Evidence

A valid focus-preservation test should measure system/window focus, not merely:

```cpp
ActionBar::hasFocus() == false
```

because that does not prove the external source app stayed focused.

On X11/Xvfb, use a reliable system-level focus check if practical.

If not achieved:

mark:

```text
FOCUS PRESERVATION = PARTIAL / NOT VERIFIED
```

Do not infer it.

---

# 46. Placement Test

Use deterministic fixture geometry where possible.

Given:

```text
selectionRect
screenRect
toolbarSize
```

unit-test:

```text
above placement
below fallback
left screen clamp
right screen clamp
top clamp
bottom clamp
```

Keep placement math separately testable from QWidget behavior.

Suggested class:

```text
ActionBarPlacement
```

or equivalent lightweight helper.

---

# 47. Performance

M1 already uses event-driven AT-SPI.

Preserve that.

M2 must not add:

```text
60 Hz polling
continuous pointer polling
desktop tree scanning
repeated window recreation
busy waits
sleep()
```

Target:

```text
selection settles
      ↓
M1 debounce ~80 ms
      ↓
ActionBar visible shortly thereafter
```

Approximate desired total:

```text
< 150–200 ms
```

under normal local conditions.

Measure in synthetic tests where practical.

---

# 48. Flicker

Repeated AT-SPI events from one drag should not cause:

```text
show
hide
show
hide
show
```

M1 already debounces the extraction.

M2 should reuse an existing ActionBar and move/update it.

No animations are required.

Reliability before animation.

---

# 49. Own-Application Selection

M1 already filters WorkSidekick's own process.

Preserve this.

Selecting text inside:

```text
Settings
Diagnostics
ResultCard
Workspace
```

must not cause recursive ActionBar popups.

Add a regression test if practical.

---

# 50. Real Desktop Compatibility Matrix

Create:

```text
docs/m2-overlay-compatibility.md
```

Use columns conceptually equivalent to:

```text
Application
Session
Qt backend
AT-SPI selection
Anchor
ActionBar shown
Placement reasonable
Source focus preserved
Button clickable
Correct text sent
Notes
```

Target real apps:

```text
GNOME Text Editor / gedit
Firefox
Chrome / Chromium
VS Code
GNOME Terminal
Slack / Electron
```

For:

```text
GNOME X11
GNOME Wayland
```

when available.

Never infer values.

Use:

```text
NOT TESTED
```

when unavailable.

---

# 51. Real Desktop M1 Debt

While doing M2, if a real Ubuntu GNOME environment becomes available, also fill the M1 compatibility matrix.

This is useful but not required to begin coding M2.

Do not delay automated M2 work solely because the Codex environment lacks a real GNOME desktop.

---

# 52. No Input Injection

M2 must NOT simulate:

```text
Ctrl+C
Ctrl+V
mouse clicks
keyboard input
```

against third-party applications.

The whole benefit of M1 is that M2 already has selected text.

Input injection belongs only to a later fallback decision if ever required.

---

# 53. No Automatic Replacement

M2 only supports:

```text
Copy result
```

through the existing ResultCard.

Do NOT implement:

```text
Replace selected text
Paste automatically
Send message
```

yet.

---

# 54. Settings

Add the minimal new setting only:

```text
Automatic selection toolbar
```

Do not start implementing the full future settings architecture.

Existing:

```text
model
output language
API key
```

must remain unchanged.

---

# 55. CMake

Add new M2 sources to the correct existing targets.

Shared UI logic should remain shared.

Linux platform policy should only compile on Linux.

Do not create Windows implementation placeholders unless required for compilation.

Windows backend remains M5.

---

# 56. CI

Rename workflow milestone if appropriate:

```text
Ubuntu M2
```

Preserve all current gates:

```text
Release configure
Release build
CTest
secure-store integration
GUI smoke
AT-SPI real-event integration
```

Add M2 overlay integration where reliable.

Latest green M1 behavior must remain green.

---

# 57. Documentation

Add:

```text
docs/M2_SPEC.md
docs/m2-completion.md
docs/m2-overlay-compatibility.md
```

Update:

```text
README.md
docs/architecture.md
docs/privacy.md
docs/troubleshooting.md
```

Document the native Wayland positioning limitation clearly.

Do not say:

```text
Linux fully supports automatic PopClip toolbar
```

unless both relevant session types have actually been demonstrated.

---

# 58. Architecture After M2

Target:

```text
             Other Linux Application
                       │
                       │ text selection
                       ▼
             AtSpiSelectionMonitor
                       │
                       │ Selection
                       ▼
                  Application
                       │
             ┌─────────┴─────────┐
             │                   │
             ▼                   ▼
        Diagnostics           ActionBar
                                  │
                         NO NETWORK YET
                                  │
                         explicit user click
                                  │
                                  ▼
                           AppController
                                  │
                                  ▼
                           PromptBuilder
                                  │
                                  ▼
                           OpenAIProvider
                                  │
                                  ▼
                            ResultCard
                                  │
                             Copy result
```

M0 manual path remains parallel:

```text
ClipboardSelectionProvider
          │
     Process Clipboard
          │
     AppController
```

---

# 59. Important Architectural Boundary

ActionBar should know:

```text
Feature
Selection
```

ActionBar should NOT know:

```text
API key
OpenAI URL
Prompt contents
libsecret
AT-SPI objects
GObject
clipboard implementation
```

AppController should know:

```text
Feature
Selection text
AI workflow
```

AppController should NOT know:

```text
AT-SPI
X11
Wayland
toolbar placement
```

Platform window policy should know:

```text
Qt/native window capability
positioning
focus policy
```

It should NOT know:

```text
AI
prompts
selected text semantics
```

Preserve these boundaries.

---

# 60. M2 Acceptance Gate

M2 may be classified PASS when the code-level/full-capability backend satisfies:

```text
Ubuntu 22.04 Release build PASS

all M0 tests PASS

all M1 tests PASS

ActionBar implemented

AT-SPI Selection snapshot connected to ActionBar

selection event alone causes zero AI requests

feature click causes one explicit AI request

AI request uses AT-SPI Selection, not clipboard

clipboard remains unchanged before Copy Result

new selection replaces stale ActionBar snapshot

selectionCleared hides and clears ActionBar

ActionBar does not call AI directly

automatic popup can be enabled/disabled

anchored placement implemented on supported Qt backend

screen clamping tested

missing anchor handled gracefully

unsupported native Wayland positioning handled honestly

Diagnostics reports overlay capability

X11/Xvfb ActionBar integration passes

focus preservation tested or explicitly marked unverified

M0 manual clipboard workflow still passes

M1 AT-SPI runtime tests still pass

privacy regression tests pass

documentation updated
```

---

# 61. M2 PASS Does Not Mean

Do NOT interpret M2 PASS as:

```text
all Linux apps supported
Wayland anchored placement solved
HiDPI solved
multi-monitor fully validated
Electron selection solved
global shortcut implemented
Windows implemented
```

Those claims require independent evidence.

---

# 62. Wayland Does Not Block M2 PASS

Native Wayland anchored placement limitations do NOT automatically make M2 FAIL.

M2 can PASS with:

```text
X11 / Qt xcb:
FULL anchored ActionBar

Native Wayland:
AT-SPI M1 preserved
anchored overlay capability reported unsupported
automatic anchored popup disabled
M0 fallback preserved
```

The important requirement is honest capability detection and graceful degradation.

Do not compromise desktop security to erase this distinction.

---

# 63. M2 Completion Report

When implementation is finished, create:

```text
docs/m2-completion.md
```

Include:

```text
baseline commit
final implementation commit
files added/changed
ActionBar architecture
snapshot ownership
AppController changes
window flags
platform window policy
Qt backend detection
X11 positioning strategy
Wayland behavior
focus preservation method/result
placement algorithm
tests
CI run
M0 regression
M1 regression
privacy regression
real GNOME X11 results
real GNOME Wayland results
known limitations
recommended M3 design
```

Every untested item must say:

```text
NOT TESTED
```

---

# 64. Required Codex Final Response

Stop after M2.

Do NOT automatically begin M3.

Return:

```text
M2 Gate = PASS / PARTIAL / FAIL

Baseline:
Final commit:

Build:
CTest:
M0 regression:
M1 regression:
AT-SPI runtime:
ActionBar:
Automatic popup:
Explicit-click AI boundary:
AT-SPI text vs clipboard test:
Placement:
X11/Xvfb:
GNOME X11:
Native Wayland:
Focus preservation:
HiDPI:
Multi-monitor:
Privacy regression:

Known blockers:

Recommended next milestone:
M3 — Linux Fallbacks + Global Shortcut
```

Use:

```text
NOT TESTED
```

instead of guessing.

---

# 65. First Implementation Order

Implement M2 in this order:

```text
ActionBarPlacement pure logic
        ↓
ActionBar widget
        ↓
platform window policy
        ↓
fake-monitor UI tests
        ↓
AppController::runSelection
        ↓
explicit click → AI integration
        ↓
automatic-popup setting
        ↓
M1 monitor integration
        ↓
X11/Xvfb focus/placement integration
        ↓
Wayland capability handling
        ↓
documentation
        ↓
completion report
```

Do not start from native X11 hacks.

First make the shared architecture correct.

---

# Final M2 Objective

At the end of M2, the successful supported-platform experience should be:

```text
select text
    ↓
toolbar appears
    ↓
click Plain Speak / Summarize / Polish
    ↓
AI processes exactly that selected text
    ↓
result appears
    ↓
user copies it
```

with:

```text
no automatic upload
no clipboard dependency
no focus stealing during automatic popup
no input injection
no automatic sending
```

This is the milestone where WorkSidekick first becomes a genuine system-wide AI selection assistant rather than a clipboard application.