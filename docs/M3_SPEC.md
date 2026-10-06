# WorkSidekick — M3 Implementation Specification

## Linux Fallback Selection + Global Shortcut

Repository:

```text
https://github.com/wzk111/leadership-dandang-win-linux
```

Accepted baseline:

```text
main
dacde1e335a9bdcf3439209cd937a92dcf42f9ca

M0 = PASS
M1 = PASS
M2 = PASS
```

Before implementing M3, read:

```text
PROJECT_SPEC.md
docs/M1_SPEC.md
docs/M2_SPEC.md
docs/m1-completion.md
docs/m2-completion.md
docs/m1-architecture.md
docs/m2-overlay-compatibility.md
```

Do not redesign the already accepted M0–M2 implementation unless a real blocking defect is discovered.

---

# 1. M3 Objective

M3 makes WorkSidekick usable when the M2 automatic AT-SPI toolbar path is unavailable.

Implement:

```text
Global Shortcut
      +
Explicit Manual Trigger
      +
Selection Resolution Pipeline
```

Target user experience:

```text
CASE A — Best case

select text
    ↓
AT-SPI
    ↓
automatic ActionBar
    ↓
feature click
    ↓
AI
```

This existing M2 path remains unchanged.

Fallback:

```text
CASE B — AT-SPI text exists but automatic overlay cannot appear

select text
    ↓
press Ctrl+Alt+P
    ↓
use cached AT-SPI Selection
    ↓
manual Action Palette
    ↓
feature click
    ↓
AI
```

X11 fallback:

```text
CASE C — AT-SPI selection unavailable

select text with mouse
    ↓
press Ctrl+Alt+P
    ↓
X11 PRIMARY selection
    ↓
manual Action Palette
    ↓
feature click
    ↓
AI
```

Final fallback:

```text
CASE D — native Wayland / inaccessible application

select text
Ctrl+C
    ↓
press Ctrl+Alt+P
    ↓
Clipboard
    ↓
manual Action Palette
    ↓
feature click
    ↓
AI
```

No feature should automatically send text to AI merely because the global shortcut was pressed.

---

# 2. M3 Scope

Implement:

```text
IGlobalShortcut abstraction

X11 global shortcut backend

XDG Desktop Portal GlobalShortcuts backend

runtime shortcut capability detection

default shortcut Ctrl+Alt+P

manual trigger flow

AT-SPI → PRIMARY → Clipboard selection resolver

X11 PRIMARY support

manual Quick Action / Action Palette UI

native Wayland usable manual workflow

external --trigger command

single-instance trigger IPC

shortcut diagnostics

fallback diagnostics

settings / enable-disable shortcut

tests and CI
```

M3 does NOT implement:

```text
Ctrl+C injection

Ctrl+V injection

ydotool

uinput

automatic replacement

automatic message sending

Windows backend

Reply / Profile / Relevance

Gemini / Anthropic

browser extension

GNOME Shell extension
```

---

# 3. Preserve Existing Paths

Do not break these accepted paths.

M0:

```text
Clipboard
    ↓
Process Clipboard
    ↓
Feature
    ↓
AI
```

M1:

```text
AT-SPI
    ↓
Selection diagnostics
```

M2:

```text
AT-SPI
    ↓
automatic ActionBar
    ↓
explicit feature click
    ↓
AI
```

M3 adds another parallel entry point.

---

# 4. M3 Architectural Goal

Target architecture:

```text
                    ISelectionMonitor
                           │
                           │ latest AT-SPI Selection
                           ▼
                   SelectionResolver
                           ▲
                           │
         ┌─────────────────┼─────────────────┐
         │                 │                 │
       AT-SPI        X11 PRIMARY         Clipboard
         │                 │                 │
         └─────────────────┼─────────────────┘
                           │
                    ResolvedSelection
                           │
                    explicit trigger
                           │
                           ▼
                  ManualActionPalette
                           │
                    feature click
                           │
                           ▼
                     AppController
                           │
                           ▼
                          AI
```

Global shortcut architecture:

```text
                 IGlobalShortcut
                       │
             ┌─────────┴─────────┐
             │                   │
             ▼                   ▼
       X11GlobalShortcut    PortalGlobalShortcut
             │                   │
           xcb              native Wayland
```

---

# 5. Explicit Consent Boundary

Maintain this invariant:

```text
selection detected
      ↓
NO AI
```

Also:

```text
global shortcut pressed
      ↓
NO AI
```

Only:

```text
global shortcut
      ↓
local Action Palette
      ↓
USER FEATURE CLICK
      ↓
AI
```

may send selected text.

This must be covered by automated tests.

---

# 6. Selection Resolver

Introduce approximately:

```text
src/core/SelectionResolver.h
src/core/SelectionResolver.cpp
```

or another clean shared location.

Suggested result:

```cpp
enum class SelectionSource
{
    AtSpi,
    PrimarySelection,
    Clipboard
};

struct ResolvedSelection
{
    Selection selection;
    SelectionSource source;
};
```

Suggested abstraction:

```cpp
class IExplicitSelectionResolver
{
public:
    virtual ~IExplicitSelectionResolver() = default;

    virtual std::optional<ResolvedSelection>
    resolve() = 0;
};
```

The resolver is used ONLY after an explicit manual trigger.

---

# 7. Resolution Priority

When the user presses the global shortcut:

Use:

```text
1. latest valid AT-SPI Selection
        ↓ unavailable

2. X11 PRIMARY selection
        ↓ unavailable

3. regular Clipboard
        ↓ unavailable

4. show "No text available"
```

This order is important.

---

# 8. AT-SPI First

If M1 currently has:

```text
monitor.latestSelection()
```

and that Selection is still valid:

use it first.

Do not query accessibility again.

M1 already expires cached selections.

Reuse that behavior.

Do not add another long-lived accessibility cache.

---

# 9. Why AT-SPI Is Preferred

AT-SPI Selection contains:

```text
text
source application
possible screen rectangle
```

and therefore has better provenance than clipboard text.

If present, preserve:

```cpp
sourceApplication
anchorRect
```

inside the resolved selection.

Source:

```text
AT-SPI
```

must be shown locally in the manual action UI.

---

# 10. X11 PRIMARY Selection

On Qt xcb:

check:

```cpp
QClipboard::supportsSelection()
```

Then retrieve:

```cpp
clipboard->text(QClipboard::Selection)
```

This corresponds to X11's PRIMARY / global mouse selection.

Do NOT overwrite it.

Do NOT call:

```cpp
setText(...)
```

on PRIMARY.

This fallback is read-only.

---

# 11. PRIMARY Retrieval Timing

PRIMARY retrieval must occur directly in response to:

```text
global shortcut activation
external explicit trigger
manual tray trigger
```

Do not poll PRIMARY periodically.

Do not build PRIMARY history.

Do not monitor all clipboard changes.

---

# 12. PRIMARY Selection Result

For PRIMARY fallback:

```cpp
Selection {
    text = primaryText,
    anchorRect = std::nullopt,
    sourceApplication = "X11 PRIMARY"
}
```

Do not invent an application name.

Do not invent an anchor rectangle.

---

# 13. Clipboard Final Fallback

If AT-SPI and PRIMARY both fail:

read:

```cpp
QClipboard::Clipboard
```

only because the user explicitly triggered WorkSidekick.

Result:

```cpp
Selection {
    text = clipboardText,
    anchorRect = std::nullopt,
    sourceApplication = "Clipboard"
}
```

This is intentionally similar to M0.

---

# 14. Clipboard Staleness

The application cannot reliably know whether arbitrary clipboard contents correspond to the user's current selection.

Therefore the manual palette MUST visibly indicate:

```text
Source: Clipboard
```

and present a local preview.

This lets the user detect stale clipboard text before choosing an AI action.

Do not automatically send clipboard fallback content.

---

# 15. Size Limits

Reuse existing text limit:

```text
100,000 characters
```

Resolver must reject oversized PRIMARY or clipboard content.

Do not allow a fallback path to bypass existing PromptBuilder size restrictions.

---

# 16. Manual Action Palette

Create approximately:

```text
src/ui/ManualActionPalette.h
src/ui/ManualActionPalette.cpp
```

This is different from the automatic M2 ActionBar.

Purpose:

```text
explicit global shortcut
       ↓
normal usable action window
```

It may accept focus.

This is especially important for native Wayland.

---

# 17. Manual Palette UI

Suggested:

```text
┌─────────────────────────────────────────┐
│ WorkSidekick                            │
│                                         │
│ Source: AT-SPI / X11 PRIMARY / Clipboard│
│                                         │
│ "Can we align the deliverables..."      │
│                                         │
│ [Plain Speak] [Summarize] [Polish]      │
│                                         │
│                              [Close]     │
└─────────────────────────────────────────┘
```

Preview may truncate visually but must preserve the complete internal snapshot.

Example UI preview limit:

```text
500–1000 characters
```

Do NOT truncate text sent to AI unless existing limits require it.

---

# 18. Manual Palette Privacy

Text preview is:

```text
local only
not logged
not persisted
not copied automatically
not uploaded until feature click
```

Closing the palette clears its cached `Selection`.

---

# 19. Manual Palette Snapshot

Same rule as M2:

The palette owns a VALUE COPY of:

```cpp
ResolvedSelection
```

Do not query:

```text
AT-SPI
PRIMARY
Clipboard
```

again when the feature button is clicked.

Correct:

```text
shortcut
    ↓
resolve selection
    ↓
snapshot
    ↓
show palette
    ↓
feature click
    ↓
use snapshot
```

---

# 20. Manual Palette Feature Click

On click:

```text
copy current snapshot
      ↓
hide / clear palette
      ↓
AppController::runSelection()
```

Reuse M2's existing:

```cpp
runSelection()
```

Do not create another AI pipeline.

---

# 21. Native Wayland Manual Experience

Native Wayland does not need anchored absolute positioning for the manual palette.

The global shortcut is an explicit user action.

Therefore a normal compositor-managed window is acceptable.

Flow:

```text
Ctrl+Alt+P
     ↓
resolve AT-SPI / Clipboard
     ↓
show normal ManualActionPalette
```

It may activate normally.

Do NOT try to position it at arbitrary screen coordinates.

---

# 22. X11 Manual Palette

On X11/xcb:

it is acceptable to show the ManualActionPalette as a normal centered/top-level window.

Optionally, the existing compact ActionBar may be used near the cursor if implemented cleanly.

But this is NOT required.

Prefer behavior consistency and reliability over additional positioning code.

The M2 automatic ActionBar remains the anchored experience.

---

# 23. Global Shortcut Interface

Add approximately:

```text
src/platform/IGlobalShortcut.h
```

Suggested:

```cpp
struct GlobalShortcutStatus
{
    bool available = false;
    bool registered = false;

    QString backend;
    QString description;
    QString triggerDescription;
};

class IGlobalShortcut : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    virtual bool start() = 0;
    virtual void stop() = 0;

    virtual bool registerDefault() = 0;
    virtual void unregisterShortcut() = 0;

    virtual GlobalShortcutStatus status() const = 0;

signals:
    void activated();
    void statusChanged();
};
```

Exact API may be adjusted.

Keep it small.

---

# 24. Default Shortcut

Default preferred shortcut:

```text
Ctrl + Alt + P
```

Internal action ID:

```text
worksidekick.trigger
```

Do not hard-code the shortcut throughout the project.

Centralize its definition.

---

# 25. Shortcut Settings

Add:

```text
Enable global shortcut
```

Persist in QSettings.

Suggested key:

```text
shortcut/enabled
```

Recommended default:

```text
OFF
```

Reason:

portal registration may present OS permission/configuration UI.

The application should not unexpectedly open a permission dialog at first launch.

---

# 26. Enabling Shortcut

When user enables:

```text
Enable global shortcut
```

then:

```text
select backend
      ↓
attempt registration
      ↓
report real status
```

If registration fails:

keep WorkSidekick usable.

Do not crash.

Do not repeatedly show dialogs.

---

# 27. Disable Shortcut

Disabling must:

```text
unregister shortcut
close portal session where applicable
stop native hotkey backend
```

It must NOT:

```text
stop AT-SPI
disable M2 automatic toolbar
clear API settings
```

These controls are independent.

---

# 28. X11 Shortcut Backend

For Qt backend:

```text
xcb
```

implement:

```text
X11GlobalShortcut
```

Suggested location:

```text
src/platform/linux/X11GlobalShortcut.h
src/platform/linux/X11GlobalShortcut.cpp
```

Use native X11 global key grabbing.

Conceptually:

```text
XOpenDisplay
      ↓
XGrabKey Ctrl+Alt+P on root window
      ↓
wait for KeyPress
      ↓
emit activated()
```

Use a dedicated display connection.

Do not interfere with Qt's X11 connection.

---

# 29. X11 Threading

Do not block Qt GUI thread with:

```cpp
XNextEvent()
```

Use a dedicated worker thread.

It should sleep/block waiting for X events rather than high-frequency polling.

Provide a clean wake/shutdown mechanism.

No busy loop.

---

# 30. X11 Lock Modifiers

Global shortcut should still work when:

```text
Caps Lock
Num Lock
```

are enabled.

Handle reasonable lock modifier combinations when grabbing the shortcut.

Do not accidentally require the user to disable Caps Lock.

---

# 31. X11 Shortcut Collision

If:

```text
Ctrl+Alt+P
```

is already owned by another application/window manager:

registration must fail gracefully.

Diagnostics:

```text
Global shortcut:
X11 registration failed — key combination may already be in use
```

Do not override another application.

---

# 32. X11 Shutdown

On:

```text
disable shortcut
application quit
```

call:

```text
XUngrabKey
```

and close native resources.

No lingering grab.

---

# 33. Native Wayland Shortcut Backend

For:

```text
wayland
wayland-egl
```

prefer:

```text
org.freedesktop.portal.GlobalShortcuts
```

Create approximately:

```text
src/platform/linux/PortalGlobalShortcut.h
src/platform/linux/PortalGlobalShortcut.cpp
```

Use Qt DBus.

Add:

```text
Qt6::DBus
```

only where required.

---

# 34. Portal Capability Probe

Before registration:

inspect:

```text
org.freedesktop.portal.Desktop
/org/freedesktop/portal/desktop

org.freedesktop.portal.GlobalShortcuts
```

Determine whether the interface actually exists.

Do not assume availability because:

```text
xdg-desktop-portal
```

is installed.

---

# 35. Portal Version

Read:

```text
org.freedesktop.portal.GlobalShortcuts.version
```

and expose it in Diagnostics.

Support the API version actually available.

Do not require v2-only functionality unless necessary.

---

# 36. Portal Session

Correct lifecycle:

```text
CreateSession
      ↓
receive request response
      ↓
session_handle
      ↓
BindShortcuts
      ↓
user/system config interaction if required
      ↓
Activated
```

The API is asynchronous.

Do not block GUI thread waiting for D-Bus responses.

---

# 37. Portal Binding UX

Portal shortcut binding may present a system dialog.

This is expected.

Only initiate binding after an explicit user action such as:

```text
Enable global shortcut
```

or:

```text
Configure global shortcut
```

Do not trigger permission UI silently during application startup.

---

# 38. Portal Shortcut Description

Request:

```text
ID:
worksidekick.trigger

description:
Open WorkSidekick for selected text

preferred trigger:
Ctrl+Alt+P
```

The portal/backend/user may choose a different actual binding.

Do not assume the preferred trigger was accepted.

---

# 39. Trigger Description

After binding:

store/display the returned:

```text
trigger_description
```

where available.

Diagnostics example:

```text
Shortcut backend: XDG Desktop Portal
Registered: yes
Configured shortcut: Ctrl+Alt+P
```

---

# 40. Portal Activated Signal

Listen only to:

```text
GlobalShortcuts::Activated
```

for the WorkSidekick shortcut ID.

When received:

```text
emit IGlobalShortcut::activated()
```

Do NOT directly:

```text
call AI
read secrets
```

inside the portal callback.

Route back through normal application flow.

---

# 41. Portal Session Loss

Handle:

```text
portal process restart
session Closed
D-Bus disconnect
```

gracefully.

Status becomes unavailable/unregistered.

Do not crash.

The user can re-enable/re-register.

---

# 42. Portal Unavailable

A portal backend on the target Ubuntu installation may not expose GlobalShortcuts.

This is a supported runtime condition.

Display:

```text
Global shortcut unavailable through desktop portal.
Use tray trigger or configure the external WorkSidekick trigger.
```

Do not install privileged components.

---

# 43. Platform Factory

Extend:

```text
PlatformFactory
```

to produce:

```text
ISecretStore
ISelectionMonitor
IPlatformWindowPolicy
IGlobalShortcut
```

Selection of shortcut backend should be based primarily on:

```cpp
QGuiApplication::platformName()
```

Conceptually:

```text
xcb
    → X11GlobalShortcut

wayland*
    → PortalGlobalShortcut

other
    → unavailable backend
```

Do not scatter backend detection across UI files.

---

# 44. Manual Trigger Controller

Introduce a small orchestration path in `Application` or a focused controller.

Conceptually:

```cpp
void Application::triggerManualActions()
{
    const auto result = resolver_->resolve();

    if (!result) {
        showNoSelectionMessage();
        return;
    }

    manualPalette_.showSelection(*result);
}
```

Do not put selection resolution inside the palette.

---

# 45. Trigger Sources

All of these should call the SAME manual-trigger function:

```text
global shortcut

tray:
"Open Selection Actions"

external --trigger
```

Do not duplicate behavior.

---

# 46. Tray Menu

Add:

```text
Open Selection Actions
```

This must work even when global shortcut registration is unavailable.

Suggested tray layout:

```text
Open WorkSidekick
Open Selection Actions
Process Clipboard

Enable Automatic Toolbar
Enable Global Shortcut

Settings
Diagnostics
About
Quit
```

---

# 47. External Trigger

Add CLI support:

```bash
worksidekick --trigger
```

Purpose:

allow desktop environments or custom keyboard shortcuts to invoke the resident WorkSidekick process.

This is especially useful where:

```text
GlobalShortcuts portal unavailable
```

but the desktop allows launching a command from its own shortcut settings.

---

# 48. Single Instance

`worksidekick --trigger` must not launch multiple full WorkSidekick tray processes.

Implement a lightweight single-instance IPC mechanism.

Recommended Qt classes:

```text
QLocalServer
QLocalSocket
```

Primary instance:

```text
creates local IPC server
```

Second invocation:

```text
worksidekick --trigger
      ↓
connects to existing instance
      ↓
sends "trigger"
      ↓
exits
```

Existing instance then runs:

```text
triggerManualActions()
```

---

# 49. IPC Security

IPC must be local-user only.

Do not create:

```text
world-writable TCP port
public network listener
unauthenticated remote trigger
```

Prefer Unix local socket with user-only access.

Where supported:

```cpp
QLocalServer::UserAccessOption
```

or equivalent secure user-specific runtime location.

---

# 50. Stale Socket Handling

Do not blindly delete another active instance's socket.

Startup strategy:

```text
try connecting
      ↓
existing instance responds
      → use it

connection truly unavailable
      ↓
remove stale endpoint
      ↓
become primary
```

Avoid races where two WorkSidekick instances both become primary.

---

# 51. Trigger When No Existing Instance

If:

```bash
worksidekick --trigger
```

is called and no WorkSidekick is running:

it is acceptable for that invocation to become the primary instance.

After initialization:

```text
perform manual trigger
```

Do not require the user to launch the tray app manually first.

---

# 52. Explicit Trigger Selection Resolution

On:

```text
Ctrl+Alt+P
```

do:

```text
latest AT-SPI
      ↓
PRIMARY if xcb
      ↓
Clipboard
```

Example:

```text
AT-SPI = none
PRIMARY = "selected sentence"
Clipboard = "old copied password-free example"
```

Result must be:

```text
selected sentence
```

Source:

```text
X11 PRIMARY
```

---

# 53. No Automatic PRIMARY Monitoring

Do NOT:

```text
watch PRIMARY continuously
store PRIMARY history
trigger toolbar from PRIMARY change
```

M3 PRIMARY access is a manual-trigger fallback.

Automatic text detection remains M1 AT-SPI.

---

# 54. No Automatic Clipboard Monitoring

Likewise, do NOT:

```text
listen to clipboard all day
build clipboard history
automatically upload new clipboard contents
```

Read clipboard only when explicitly requested.

---

# 55. No Input Injection

Do not introduce production dependencies on:

```text
xdotool
ydotool
XTest
/dev/uinput
root
```

M3 does not simulate Ctrl+C.

On X11 we already have PRIMARY.

On Wayland the user can explicitly copy text before triggering if AT-SPI does not expose it.

---

# 56. Manual Fallback Error

If no text is available:

show a small normal message/palette:

```text
No selected text found.

Try:
1. Select text again and press the shortcut.
2. If that does not work, copy the text first.
```

Do not call AI.

Do not show stale previous Selection.

---

# 57. Selection Source Metadata

Manual palette should show one of:

```text
AT-SPI
X11 PRIMARY
Clipboard
```

Diagnostics should track the last manual resolution source WITHOUT logging the text.

Example:

```text
Last explicit trigger:
source = X11 PRIMARY
characters = 138
resolved = yes
```

---

# 58. Diagnostics M3 Section

Add approximately:

```text
Global shortcut

Enabled preference: yes
Backend: X11 / XDG Portal / unavailable
Available: yes
Registered: yes
Preferred trigger: Ctrl+Alt+P
Actual trigger: Ctrl+Alt+P
Portal version: n/a / 1 / 2
Last activation: ...
Last error: none


Manual selection fallback

AT-SPI cached selection: available
X11 PRIMARY supported: yes
Clipboard fallback: available
Last resolution source: AT-SPI
Last character count: 142

Automatic toolbar:
existing M2 diagnostics...
```

Never display actual selected text by default.

---

# 59. Settings UI

Add:

```text
[ ] Enable global shortcut
```

Optional:

```text
Configure shortcut
```

for portal environments if supported.

Do NOT build a full arbitrary keyboard shortcut editor yet unless implementation is trivial.

M3 may keep preferred shortcut fixed to:

```text
Ctrl+Alt+P
```

---

# 60. X11 PRIMARY Tests

Test resolver:

```text
AT-SPI available
PRIMARY available
Clipboard available
→ AT-SPI wins
```

```text
AT-SPI unavailable
PRIMARY available
Clipboard available
→ PRIMARY wins
```

```text
AT-SPI unavailable
PRIMARY empty
Clipboard available
→ Clipboard wins
```

```text
all empty
→ no selection
```

---

# 61. Clipboard Independence

Test:

```text
PRIMARY:
"PRIMARY TEXT"

Clipboard:
"OLD CLIPBOARD TEXT"
```

Trigger.

Manual palette must contain:

```text
PRIMARY TEXT
```

Click Polish.

HTTP body must contain:

```text
PRIMARY TEXT
```

and NOT:

```text
OLD CLIPBOARD TEXT
```

---

# 62. Shortcut Does Not Upload

Critical test:

```text
global shortcut activated
      ↓
palette displayed
```

Then:

```text
HTTP requests = 0
secret reads = 0
```

Only after:

```text
feature button click
```

should:

```text
HTTP requests = 1
```

---

# 63. Shortcut Repeated Activation

If palette is already visible and the user presses shortcut again:

resolve selection again.

Replace existing snapshot.

Do not stack multiple palette windows.

Example:

```text
trigger → selection A
trigger → selection B
click feature
```

Must send:

```text
B
```

---

# 64. Empty Second Trigger

Test:

```text
trigger
→ selection A displayed

selection removed
clipboard empty

trigger again
```

Old A must be cleared.

No stale A may remain available for feature execution.

---

# 65. Busy AI Behavior

If AI is currently busy:

global shortcut may:

```text
open palette disabled
```

or:

```text
show "AI request in progress"
```

but must not queue another request.

Keep existing no-overlapping-request semantics.

---

# 66. Existing M2 Automatic Toolbar

Global shortcut settings must NOT alter:

```text
Automatic selection toolbar
```

These settings are independent.

Possible combinations:

```text
automatic ON
shortcut ON

automatic OFF
shortcut ON

automatic ON
shortcut OFF

automatic OFF
shortcut OFF
```

All must behave predictably.

---

# 67. Native Wayland Goal

For native Wayland, M3 should finally provide a usable first-class fallback:

```text
AT-SPI selection if available
      ↓
portal global shortcut
      ↓
manual palette
```

or:

```text
Ctrl+C
      ↓
portal global shortcut
      ↓
Clipboard
      ↓
manual palette
```

Anchored automatic ActionBar remains unsupported under native Wayland unless future standard APIs provide a correct solution.

---

# 68. Ubuntu 22.04 Portal Reality

Do not assume GlobalShortcuts exists on every Ubuntu 22.04 installation.

Runtime probe it.

M3 PASS does not require every Ubuntu 22.04 machine to provide this portal.

If unavailable:

```text
Portal shortcut = unavailable
External --trigger = available
Tray manual trigger = available
```

This is acceptable.

---

# 69. GNOME Custom Shortcut Fallback

Document, but DO NOT automatically configure:

```text
Settings
→ Keyboard
→ Custom Shortcuts
```

command:

```bash
/path/to/worksidekick --trigger
```

This gives users a Wayland-safe fallback if the portal backend cannot provide GlobalShortcuts.

Do not run:

```text
gsettings
dconf
```

to silently change desktop shortcuts.

---

# 70. X11 Shortcut Integration Test

In Ubuntu CI + Xvfb/Openbox:

test actual X11 shortcut registration.

Requirements:

```text
register Ctrl+Alt+P
      ↓
synthetic TEST-ONLY key event
      ↓
IGlobalShortcut::activated
```

Input simulation may be used ONLY inside the integration test.

Do not link test injection libraries into the production binary.

If XTest is used:

```text
libxtst-dev
```

should be test-only.

---

# 71. X11 Collision Test

Where practical:

attempt two grabs for the same combination.

Verify second registration reports failure.

Do not silently claim both succeeded.

---

# 72. Portal Unit Tests

Do not require a real desktop portal in basic CTest.

Separate:

```text
portal state machine tests
```

from:

```text
real portal desktop test
```

Use a fake/mock session abstraction or test D-Bus service where reasonable.

Test:

```text
interface absent
CreateSession failure
BindShortcuts rejected
successful session
Activated signal
session closed
stop/unregister
```

---

# 73. Portal Real Integration

If CI does not provide a GlobalShortcuts portal backend:

mark:

```text
NOT TESTED
```

Do not fake a successful real portal test.

This is acceptable for M3 PASS if:

```text
portal implementation/state machine is tested
absence degrades correctly
external trigger works
```

---

# 74. IPC Tests

Test:

```text
primary app instance
      ↓
second process --trigger
      ↓
existing process receives one trigger
      ↓
second process exits
```

Also test:

```text
normal second launch
```

does not create two tray applications.

Exact behavior may be:

```text
activate/open existing instance
```

or simply exit cleanly.

Document it.

---

# 75. M0 Regression

Continue passing:

```text
Clipboard
→ Process Clipboard
→ Feature
→ HTTP
→ ResultCard
→ Copy
```

---

# 76. M1 Regression

Continue passing:

```text
AT-SPI
event
text
application
rectangle
debounce
stop/start
unavailable bus
```

---

# 77. M2 Regression

Continue passing:

```text
automatic ActionBar
selection snapshot
zero HTTP before click
latest selection wins
clipboard independence
X11 placement
X11 source-focus preservation
unsupported Wayland capability
```

Do NOT weaken any M2 tests.

---

# 78. Privacy Regression

Verify:

```text
AT-SPI selection → no AI

PRIMARY read → no AI

Clipboard fallback read → no AI

global shortcut → no AI

--trigger → no AI

feature click → AI
```

No text history.

No clipboard history.

No logging selected text.

---

# 79. Logging

Allowed:

```text
manual_trigger source=primary chars=84
shortcut backend=x11 registered=true
portal version=1
```

Forbidden:

```text
manual_trigger text="..."
clipboard="..."
primary="..."
```

---

# 80. Performance

Global shortcut backends should be event-driven.

Do NOT add:

```text
keyboard polling
clipboard polling
PRIMARY polling
busy loops
```

Idle CPU should remain approximately unchanged.

---

# 81. CMake Changes

Likely add:

```text
Qt6::DBus
```

for portal implementation.

Linux-only production dependencies may include:

```text
X11
```

for X11 hotkey backend.

Keep test-only XTest separate if used.

Do not compile Linux backends into Windows target later.

---

# 82. Recommended New Files

Approximate structure:

```text
src/core/
├── SelectionResolver.h
└── SelectionResolver.cpp

src/platform/
├── IGlobalShortcut.h
└── IExplicitSelectionResolver.h

src/platform/linux/
├── LinuxSelectionResolver.h
├── LinuxSelectionResolver.cpp
├── X11GlobalShortcut.h
├── X11GlobalShortcut.cpp
├── PortalGlobalShortcut.h
└── PortalGlobalShortcut.cpp

src/ui/
├── ManualActionPalette.h
└── ManualActionPalette.cpp

src/app/
├── InstanceCoordinator.h
└── InstanceCoordinator.cpp
```

Names may differ.

Do not create abstractions that add no real value.

---

# 83. CI

Rename workflow when appropriate:

```text
Ubuntu M3
```

Preserve ALL accepted gates:

```text
Release build

CTest

Secret Service integration

GUI smoke

AT-SPI real event integration

X11 overlay focus / placement
```

Add:

```text
X11 PRIMARY fallback integration where reliable

X11 global shortcut integration

single-instance / --trigger integration
```

Portal real runtime test only if reproducible.

---

# 84. Real Desktop Compatibility

Create:

```text
docs/m3-fallback-compatibility.md
```

Columns:

```text
Application

Session

Qt backend

AT-SPI text

Automatic M2 toolbar

Global shortcut backend

Global shortcut works

PRIMARY works

Clipboard fallback works

Manual palette works

Correct text sent

Notes
```

Applications:

```text
GNOME Text Editor / gedit

Firefox

Chrome / Chromium

VS Code

GNOME Terminal

Slack / Electron
```

Sessions:

```text
GNOME X11

GNOME Wayland
```

Unknown rows:

```text
NOT TESTED
```

---

# 85. Real GNOME Testing

If a real Linux desktop is available, prioritize testing:

```text
Firefox
Chrome / Chromium
VS Code
GNOME Text Editor
```

with non-sensitive synthetic text.

Record:

```text
exact session
Qt backend
shortcut backend
selection source used
```

Do not test using private workplace messages.

---

# 86. Diagnostics Must Explain Fallback

Example X11:

```text
Qt backend: xcb

Automatic toolbar:
AnchoredNonActivating

Global shortcut:
X11
Ctrl+Alt+P
Registered

Manual selection resolver:
AT-SPI → PRIMARY → Clipboard

PRIMARY:
supported
```

Example Wayland with portal:

```text
Qt backend: wayland

Automatic toolbar:
unsupported

Global shortcut:
XDG Desktop Portal
Registered

Manual resolver:
AT-SPI → Clipboard
```

Example Wayland without portal:

```text
Automatic toolbar:
unsupported

Global shortcut portal:
unavailable

External trigger:
worksidekick --trigger

Tray manual action:
available
```

---

# 87. README

Document three user modes.

### Mode 1 — Automatic

```text
select text
→ ActionBar
```

Requires supported M2 backend.

### Mode 2 — Global shortcut

```text
select / copy
→ Ctrl+Alt+P
→ Manual Palette
```

### Mode 3 — Desktop custom shortcut

```text
select / copy
→ custom keybinding executes worksidekick --trigger
→ Manual Palette
```

---

# 88. Troubleshooting

Add cases:

```text
Ctrl+Alt+P already in use

portal interface unavailable

portal permission/configuration cancelled

PRIMARY unsupported

AT-SPI selection stale

clipboard empty

second instance cannot connect

stale local socket

native Wayland automatic popup unavailable
```

Provide recovery without requiring root.

---

# 89. M3 Gate

M3 may be classified PASS when:

```text
Ubuntu 22.04 Release build PASS

all M0 tests PASS

all M1 tests PASS

all M2 tests PASS

SelectionResolver implemented

AT-SPI is first priority

X11 PRIMARY fallback works on xcb

Clipboard fallback works

ManualActionPalette implemented

manual palette uses immutable snapshot

global shortcut alone sends zero AI requests

feature click sends exactly one AI request

X11 global shortcut backend implemented

X11 real shortcut integration PASS

shortcut collision handled

portal backend implemented

portal availability probed at runtime

portal absence handled gracefully

portal registration is asynchronous

portal permission UI only follows explicit enable action

--trigger implemented

single-instance IPC implemented

tray manual trigger implemented

global shortcut can be enabled / disabled

Diagnostics updated

no clipboard/selection monitoring history added

no production input injection

M0/M1/M2 behavior preserved

documentation updated
```

---

# 90. M3 PASS Does Not Mean

Do not claim:

```text
GlobalShortcuts portal works on every Ubuntu 22.04 install

every Electron application exposes AT-SPI

native Wayland supports anchored ActionBar

HiDPI fully verified

multi-monitor fully verified

Windows implemented

Ctrl+C injection implemented
```

Those require separate evidence.

---

# 91. Portal Unavailability Does Not Block M3 PASS

M3 can PASS if:

```text
X11 shortcut:
PASS

X11 PRIMARY:
PASS

manual palette:
PASS

external --trigger:
PASS

portal implementation:
PASS code-level

real portal runtime:
NOT TESTED / unavailable

graceful fallback:
PASS
```

Never turn a missing portal into fake success.

---

# 92. Security Review

Before completion confirm:

```text
No keylogging

No global keyboard content monitoring

Only registered shortcut is observed

No root requirement

No uinput

No automatic Ctrl+C

No text logs

No clipboard history

No PRIMARY history

IPC local-user only

Shortcut activation does not upload text

Only feature click uploads selected text

API key remains in Secret Service
```

---

# 93. Suggested Implementation Order

Implement in this order:

```text
SelectionResolver interface + tests
        ↓
X11 PRIMARY support
        ↓
ManualActionPalette
        ↓
manual trigger application flow
        ↓
tray "Open Selection Actions"
        ↓
IGlobalShortcut
        ↓
X11GlobalShortcut
        ↓
X11 integration test
        ↓
PortalGlobalShortcut state machine
        ↓
runtime portal probe
        ↓
Settings / diagnostics
        ↓
single-instance IPC
        ↓
--trigger
        ↓
regression / privacy tests
        ↓
documentation
```

Do not start with portal complexity before the resolver/manual flow works.

---

# 94. Completion Report

Create:

```text
docs/m3-completion.md
```

Include:

```text
baseline commit

final implementation commit

files added / modified

selection resolution architecture

AT-SPI fallback result

X11 PRIMARY result

Clipboard fallback result

manual palette

X11 shortcut implementation

shortcut collision behavior

portal implementation

portal runtime result

portal version

--trigger implementation

single-instance behavior

IPC security

CI run

CTest result

M0 regression

M1 regression

M2 regression

privacy regression

GNOME X11 results

GNOME Wayland results

known limitations

recommended M4
```

All unavailable tests:

```text
NOT TESTED
```

---

# 95. Required Final Codex Response

After M3:

STOP.

Do not begin M4.

Respond:

```text
M3 Gate = PASS / PARTIAL / FAIL

Baseline:
Final commit:

Build:
CTest:

M0 regression:
M1 regression:
M2 regression:

Selection resolver:
AT-SPI:
X11 PRIMARY:
Clipboard fallback:

Manual palette:

X11 global shortcut:
Shortcut collision:

Portal availability:
Portal implementation:
Real portal runtime:

--trigger:
Single instance:
IPC security:

Privacy regression:

GNOME X11:
GNOME Wayland:

Known blockers:

Recommended next milestone:
M4 — Full Feature Set / Product UX Expansion
```

Do not infer untested results.

---

# Final M3 Target Experience

## X11

```text
select text
    ↓
automatic toolbar

OR

select text
    ↓
Ctrl+Alt+P
    ↓
AT-SPI / PRIMARY
    ↓
Manual Palette
    ↓
feature
```

## Wayland with portal

```text
select text
    ↓
Ctrl+Alt+P
    ↓
AT-SPI

or

Ctrl+C
    ↓
Ctrl+Alt+P
    ↓
Clipboard
    ↓
Manual Palette
    ↓
feature
```

## Wayland without portal

```text
select/copy text
    ↓
desktop custom shortcut:
worksidekick --trigger

or tray:
Open Selection Actions
    ↓
Manual Palette
    ↓
feature
```

At the end of M3, WorkSidekick should remain useful even when the ideal M2 automatic anchored overlay path is unavailable, while preserving the project's central rule:

**selection and shortcut activation are local; only an explicit feature click may send text to AI.**