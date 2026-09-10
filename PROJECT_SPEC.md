# WorkSidekick — Cross-Platform AI Selection Assistant

## PROJECT_SPEC.md

### Instructions for Codex

You are responsible for creating this project from scratch.

Read this document completely before making architectural decisions.

The reference project is:

`https://github.com/SecondServ/leadership-dandang`

It may be studied for product behavior and architectural ideas, but **do not copy source code verbatim**. Implement this project independently.

The primary development platform is:

```text
Ubuntu 22.04 LTS
```

The architecture must nevertheless support a later native Windows implementation without rewriting the AI, business logic, settings, feature definitions, or most UI code.

Use incremental milestones.

Do **not** attempt to implement every feature at once.

For each milestone:

1. build the project;
2. run available automated tests;
3. perform reasonable local smoke tests;
4. document what is working;
5. document known limitations;
6. do not claim functionality that has not been tested.

---

# 1. Project Goal

Build a lightweight desktop AI assistant inspired by PopClip-style interactions.

The user should be able to select text in almost any desktop application, invoke WorkSidekick, and perform AI actions on that selected text.

Example:

```text
Slack / Chrome / Teams / VSCode / Email

User selects:

"Can we align the deliverables and close the loop before EOD?"

                    ↓

             WorkSidekick toolbar

      ┌──────────────────────────────┐
      │ Plain Speak │ Summarize      │
      │ Polish      │ Reply          │
      │ Relevance   │ Add Insight    │
      └──────────────────────────────┘

                    ↓

                 AI API

                    ↓

              Result card

"This means they want everyone to confirm
the deliverables and finish the outstanding
items before the end of today."

                    ↓

                  Copy
```

The application must never automatically send messages.

The user remains in control of all generated output.

---

# 2. Product Principles

The project should follow these principles.

### Low friction

Selecting text and using AI should require as little interaction as possible.

Target UX:

```text
select → toolbar → action → result
```

### Human in the loop

AI produces drafts or analysis only.

Never automatically click Send, submit forms, or send messages.

### Privacy first

Only process text explicitly selected by the user.

Do not continuously collect application content.

Do not record clipboard history.

Do not log selected text.

Do not log API keys.

### Local-first configuration

Store settings locally.

Store secrets through the operating system's secure credential storage.

### Platform-native integration

Do not try to emulate macOS APIs.

Use the proper accessibility and window APIs for each platform.

---

# 3. Target Platforms

## Phase 1

Primary target:

```text
Ubuntu 22.04 LTS
```

Support both:

```text
X11
Wayland
```

but functionality may differ because of Wayland security restrictions.

## Phase 2

Target:

```text
Windows 10
Windows 11
```

The Windows implementation must reuse the same:

```text
core
AI
feature
prompt
settings
networking
most UI
```

layers.

Only platform integration code should differ.

---

# 4. Technology Stack

Use:

```text
Language:       C++20
UI Framework:   Qt 6
Build System:   CMake
Networking:     QtNetwork
JSON:           Qt JSON APIs
Testing:        Qt Test / CTest
```

Ubuntu 22.04 development should work with the standard GCC/CMake toolchain.

Avoid unnecessary third-party dependencies.

Platform dependencies are allowed where required.

Linux:

```text
AT-SPI2
D-Bus
libsecret
X11/XInput2 where required
XDG Desktop Portal where available
```

Windows:

```text
Win32
COM
Microsoft UI Automation
Windows Credential Manager
```

---

# 5. High-Level Architecture

The application shall use the following architecture:

```text
                     ┌────────────────────────┐
                     │        Qt UI           │
                     │                        │
                     │ Tray / Toolbar / Card  │
                     └────────────┬───────────┘
                                  │
                         AppController
                                  │
             ┌────────────────────┼────────────────────┐
             │                    │                    │
     SelectionService        FeatureService       SettingsService
             │                    │                    │
             │                    │                    │
             │              PromptBuilder              │
             │                    │                    │
             │                 AIService               │
             │                    │                    │
             │             Provider Adapter            │
             │                    │                    │
     Platform Backend         HTTP/API            SecretStore
             │
      ┌──────┴───────┐
      │              │
    Linux          Windows
      │              │
    AT-SPI       UI Automation
    X11          Win32 Hooks
    Portal       RegisterHotKey
```

Platform-specific APIs must not leak into business logic.

---

# 6. Repository Structure

Create approximately this structure:

```text
worksidekick/
│
├── CMakeLists.txt
├── README.md
├── PROJECT_SPEC.md
├── LICENSE
│
├── cmake/
│
├── src/
│   │
│   ├── app/
│   │   ├── AppController.h
│   │   ├── AppController.cpp
│   │   ├── Application.h
│   │   └── Application.cpp
│   │
│   ├── core/
│   │   ├── Selection.h
│   │   ├── Feature.h
│   │   ├── FeatureRegistry.h
│   │   ├── FeatureRegistry.cpp
│   │   ├── PromptBuilder.h
│   │   ├── PromptBuilder.cpp
│   │   ├── Settings.h
│   │   ├── Settings.cpp
│   │   ├── Profile.h
│   │   ├── Profile.cpp
│   │   └── Result.h
│   │
│   ├── ai/
│   │   ├── IAIProvider.h
│   │   ├── AIService.h
│   │   ├── AIService.cpp
│   │   ├── OpenAIProvider.h
│   │   ├── OpenAIProvider.cpp
│   │   ├── GeminiProvider.h
│   │   ├── GeminiProvider.cpp
│   │   ├── AnthropicProvider.h
│   │   └── AnthropicProvider.cpp
│   │
│   ├── platform/
│   │   ├── ISelectionProvider.h
│   │   ├── ISelectionMonitor.h
│   │   ├── IGlobalShortcut.h
│   │   ├── ISecretStore.h
│   │   ├── IPlatformWindowPolicy.h
│   │   ├── PlatformFactory.h
│   │   ├── PlatformFactory.cpp
│   │   │
│   │   ├── linux/
│   │   │   ├── AtSpiSelectionProvider.h
│   │   │   ├── AtSpiSelectionProvider.cpp
│   │   │   ├── AtSpiSelectionMonitor.h
│   │   │   ├── AtSpiSelectionMonitor.cpp
│   │   │   ├── X11SelectionProvider.h
│   │   │   ├── X11SelectionProvider.cpp
│   │   │   ├── LinuxShortcut.h
│   │   │   ├── LinuxShortcut.cpp
│   │   │   ├── LinuxSecretStore.h
│   │   │   ├── LinuxSecretStore.cpp
│   │   │   ├── LinuxWindowPolicy.h
│   │   │   └── LinuxWindowPolicy.cpp
│   │   │
│   │   └── windows/
│   │       ├── UIASelectionProvider.h
│   │       ├── UIASelectionProvider.cpp
│   │       ├── WindowsSelectionMonitor.h
│   │       ├── WindowsSelectionMonitor.cpp
│   │       ├── WindowsShortcut.h
│   │       ├── WindowsShortcut.cpp
│   │       ├── WindowsSecretStore.h
│   │       ├── WindowsSecretStore.cpp
│   │       ├── WindowsWindowPolicy.h
│   │       └── WindowsWindowPolicy.cpp
│   │
│   ├── ui/
│   │   ├── ActionBar.h
│   │   ├── ActionBar.cpp
│   │   ├── ResultCard.h
│   │   ├── ResultCard.cpp
│   │   ├── SettingsWindow.h
│   │   ├── SettingsWindow.cpp
│   │   ├── ProfileWindow.h
│   │   ├── ProfileWindow.cpp
│   │   ├── ReplyComposer.h
│   │   └── ReplyComposer.cpp
│   │
│   └── main.cpp
│
├── tests/
│   ├── TestPromptBuilder.cpp
│   ├── TestFeatureRegistry.cpp
│   ├── TestSettings.cpp
│   └── mocks/
│
└── resources/
    ├── icons/
    └── worksidekick.desktop
```

Do not create empty abstractions merely for appearance. If an interface is not needed yet, it may be introduced in the milestone where it becomes useful.

---

# 7. Core Data Model

Define a platform-independent selection structure.

Example:

```cpp
struct ScreenRect
{
    int x{};
    int y{};
    int width{};
    int height{};
};

struct Selection
{
    std::string text;
    std::optional<ScreenRect> anchor_rect;
    std::string source_application;
};
```

The exact types may use Qt equivalents such as:

```cpp
QString
QRect
std::optional
```

if that makes integration cleaner.

The rest of the application must only consume `Selection`.

It must not know whether the selection came from:

```text
AT-SPI
X11 PRIMARY
Windows UI Automation
clipboard
```

---

# 8. Selection Provider Interface

Create an abstraction similar to:

```cpp
class ISelectionProvider
{
public:
    virtual ~ISelectionProvider() = default;

    virtual std::optional<Selection>
    currentSelection() = 0;
};
```

Selection retrieval must follow a fallback strategy.

---

# 9. Linux Selection Architecture

Linux requires separate consideration for X11 and Wayland.

At startup detect:

```text
XDG_SESSION_TYPE
```

Typical values:

```text
x11
wayland
```

Expose the detected backend in the diagnostics/settings UI.

---

# 10. Linux Primary Selection Mechanism — AT-SPI2

AT-SPI should be the preferred Linux mechanism.

Listen for:

```text
object:text-selection-changed
```

When such an event occurs:

```text
AT-SPI event
      ↓
identify accessible object
      ↓
check Text interface
      ↓
get selection count
      ↓
get selected range
      ↓
get selected text
      ↓
obtain range / component screen coordinates if available
      ↓
Selection
```

Relevant concepts include:

```text
AtspiEventListener
AtspiText
atspi_text_get_n_selections()
atspi_text_get_selection()
atspi_text_get_text()
atspi_text_get_range_extents()
```

Do not continuously scrape accessible application trees.

Only react to accessibility selection events or explicit user invocation.

Debounce rapid duplicate events.

Suggested delay:

```text
50–120 ms
```

before final selection retrieval.

---

# 11. Linux X11 Fallback

On an X11 session, implement an additional fallback.

Linux X11 commonly exposes mouse-selected text through:

```text
PRIMARY selection
```

Therefore retrieval strategy should be approximately:

```text
AT-SPI
   ↓ failure

X11 PRIMARY
   ↓ failure

explicit Ctrl+C fallback
```

Do not confuse:

```text
PRIMARY
```

with:

```text
CLIPBOARD
```

PRIMARY usually represents mouse selection.

CLIPBOARD usually represents explicit copy operations.

When using simulated Ctrl+C:

1. snapshot current clipboard;
2. trigger Ctrl+C;
3. wait for clipboard change with timeout;
4. read text;
5. restore previous clipboard where practical.

Do not treat stale clipboard content as a valid new selection.

---

# 12. Linux Automatic Popup Fallback

AT-SPI events will not work equally well in every application.

For X11 only, an optional secondary monitor may use XInput2 to detect likely selection gestures:

```text
left mouse down
      ↓
record position

left mouse up
      ↓
calculate distance
      ↓
drag > threshold OR multi-click
      ↓
query selection
```

Suggested initial threshold:

```text
5 px
```

Do not query selection after every ordinary click.

---

# 13. Wayland Strategy

Wayland intentionally restricts arbitrary applications from:

```text
globally observing input
injecting keyboard events
obtaining unrestricted pointer state
```

Do not bypass this security model.

Do NOT make these mandatory dependencies:

```text
ydotool
/dev/uinput
root permissions
privileged daemons
```

They may be considered experimental developer-only fallbacks later, but not part of the normal application.

Preferred Wayland strategy:

```text
AT-SPI text-selection event
        ↓
works
        ↓
automatic toolbar
```

If AT-SPI cannot expose the selected text:

```text
manual clipboard mode
```

where the user:

```text
selects text
Ctrl+C
invokes WorkSidekick
```

and WorkSidekick processes current clipboard text.

---

# 14. Wayland Global Shortcut

Implement global shortcuts behind `IGlobalShortcut`.

Preferred modern mechanism:

```text
XDG Desktop Portal
org.freedesktop.portal.GlobalShortcuts
```

However:

**Do not assume Ubuntu 22.04 has a portal/backend version that supports GlobalShortcuts.**

Detect support at runtime.

If supported:

```text
register global WorkSidekick shortcut
```

If unsupported:

provide:

```text
tray menu trigger
```

and clearly show:

```text
Global shortcut unavailable on this desktop environment.
```

A later milestone may add documented GNOME custom-keybinding integration.

Do not silently modify GNOME settings in M0.

---

# 15. Windows Selection Architecture

Windows implementation should use Microsoft UI Automation as the primary mechanism.

Conceptual flow:

```text
focused window
      ↓
focused AutomationElement
      ↓
TextPattern
      ↓
GetSelection()
      ↓
TextPatternRange
      ↓
GetText()
      ↓
GetBoundingRectangles()
      ↓
Selection
```

Relevant APIs may include:

```text
IUIAutomation
IUIAutomationElement
IUIAutomationTextPattern
IUIAutomationTextRange
```

Use COM safely with RAII wrappers.

---

# 16. Windows Selection Detection

For automatic popup, use a low-level mouse hook:

```text
SetWindowsHookExW(WH_MOUSE_LL)
```

Detect:

```text
button down
button up
drag distance
double click where appropriate
```

After a likely selection gesture:

```text
wait briefly
        ↓
UI Automation selection query
        ↓
non-empty text?
        ↓
show ActionBar
```

The hook callback must remain extremely lightweight.

Never perform:

```text
network requests
AI calls
heavy COM traversal
blocking waits
```

inside the hook callback.

Queue work to the application event loop.

---

# 17. Windows Selection Fallback

Some custom controls or Electron applications may not expose useful UI Automation text ranges.

Fallback:

```text
backup clipboard
        ↓
SendInput(Ctrl+C)
        ↓
wait for clipboard change
        ↓
read text
        ↓
restore clipboard
```

Only perform this when the user explicitly invokes an action or when necessary after a likely selection gesture.

---

# 18. Global Shortcut

Define:

```cpp
class IGlobalShortcut
{
public:
    virtual ~IGlobalShortcut() = default;

    virtual bool registerShortcut(...) = 0;
    virtual void unregisterShortcut() = 0;
};
```

Windows backend:

```text
RegisterHotKey()
WM_HOTKEY
```

Linux backend:

```text
XDG Desktop Portal where supported
X11-native solution where appropriate
```

Default shortcut:

```text
Ctrl + Alt + P
```

It must be configurable later.

---

# 19. Floating Action Bar

The toolbar is one of the most important parts of the application.

Requirements:

```text
frameless
always on top
small
fast
appears near selection
must not steal focus from source application
```

Initial buttons:

```text
Plain Speak
Summarize
Polish
Reply
Relevance
Add Insight
```

Qt may use flags such as:

```text
Qt::Tool
Qt::FramelessWindowHint
Qt::WindowStaysOnTopHint
Qt::WindowDoesNotAcceptFocus
Qt::WA_ShowWithoutActivating
```

but platform-specific window policy must verify actual behavior.

Do not rely only on Qt flags if they are insufficient.

---

# 20. Windows Non-Activating Toolbar

The Windows backend should apply appropriate native extended styles.

Likely:

```text
WS_EX_NOACTIVATE
WS_EX_TOOLWINDOW
```

and position through:

```text
SetWindowPos(
    HWND_TOPMOST,
    ...,
    SWP_NOACTIVATE
)
```

The objective is:

```text
source application keeps focus
selection remains active
toolbar remains clickable
```

If necessary, handle first-click behavior explicitly.

---

# 21. Linux Non-Activating Toolbar

Use Qt window flags first.

Verify independently on:

```text
GNOME X11
GNOME Wayland
```

Do not assume identical compositor behavior.

The toolbar should not unnecessarily activate WorkSidekick before the selected text has been captured.

Important ordering:

```text
user selects text
      ↓
toolbar appears without activation
      ↓
user chooses action
      ↓
capture selected text FIRST
      ↓
toolbar can disappear
      ↓
result UI may activate normally
```

---

# 22. Toolbar Position

Preferred anchor priority:

```text
selection bounding rectangle
        ↓ unavailable
current pointer / selection-event location
        ↓ unavailable
center near foreground window
```

Clamp UI to the active monitor's available geometry.

Handle multiple monitors.

Never allow the toolbar to appear completely off-screen.

---

# 23. Result Card

After an AI action, show a larger result card.

States:

```text
Loading
Success
Error
```

Success actions:

```text
Copy
Regenerate
Close
```

Later:

```text
Replace Selection
Shorter
Longer
Change Tone
```

The result card may accept focus because the source selection should already have been captured.

Use asynchronous requests.

The UI must remain responsive.

---

# 24. Feature Model

Use a platform-independent enum.

Example:

```cpp
enum class Feature
{
    PlainSpeak,
    Summarize,
    Polish,
    Reply,
    Relevance,
    AddInsight
};
```

Metadata should define:

```text
display name
description
requires profile
requires additional user input
default output language
model tier
temperature / creativity level where applicable
```

Do not encode feature-specific UI logic throughout the project.

Centralize it in `FeatureRegistry`.

---

# 25. Initial AI Features

## Plain Speak

Convert corporate jargon or complicated language into clear everyday language.

Output should:

```text
preserve facts
preserve numbers
preserve deadlines
remove unnecessary jargon
explain important acronyms when useful
```

## Summarize

Summarize long workplace messages.

Prefer:

```text
What happened
What matters
What is required
Deadline / owner if present
```

## Polish

Rewrite the user's selected draft.

Preserve original meaning.

Provide selectable styles later:

```text
Professional
Concise
Friendly
Tactful
Structured
```

## Reply

Generate a reply based on selected context.

Later support stances:

```text
Agree
Decline
Clarify
Question
Empathize
Redirect responsibility
```

## Relevance

Use optional user profile information to determine:

```text
Does this concern me?
What action is expected from me?
How urgent is it?
```

## Add Insight

Generate a useful contribution to an ongoing workplace discussion.

Avoid empty corporate language.

---

# 26. Prompt Architecture

Prompt logic belongs entirely in:

```text
PromptBuilder
```

UI must not contain prompt strings.

Example interface:

```cpp
struct PromptRequest
{
    Feature feature;
    QString selectedText;
    QString userIntent;
    QString variant;
    QString profile;
    QString outputLanguage;
};

struct Prompt
{
    QString system;
    QString user;
};
```

Then:

```cpp
Prompt PromptBuilder::build(
    const PromptRequest& request);
```

Prompts must never include API credentials.

---

# 27. AI Provider Interface

Define:

```cpp
class IAIProvider
{
public:
    virtual ~IAIProvider() = default;

    virtual void generate(
        const AIRequest& request,
        std::function<void(AIResult)> completion) = 0;
};
```

Prefer Qt signals/slots or futures if cleaner.

The design must allow:

```text
OpenAI
Gemini
Anthropic
OpenAI-compatible providers
```

without changing the UI.

---

# 28. AI Implementation Order

Do not implement every provider immediately.

### M0

Implement one provider first.

Recommended:

```text
OpenAI official API
```

Use the current official Responses API rather than hard-coding obsolete model assumptions.

Model should be configurable.

Do not hard-code a model name as a permanent product assumption.

### Later

Add:

```text
GeminiProvider
AnthropicProvider
OpenAICompatibleProvider
```

through the same interface.

---

# 29. API Request Behavior

All requests must support:

```text
timeout
cancellation
HTTP error handling
rate-limit handling
authentication error handling
empty-response handling
JSON parsing failure
```

Default timeout:

```text
30 seconds
```

Do not block the GUI thread.

Never print:

```text
API key
selected text
generated private message contents
```

to production logs.

---

# 30. Secret Storage

Create:

```cpp
class ISecretStore
```

Linux:

```text
libsecret / GNOME Keyring
```

Windows:

```text
Windows Credential Manager
```

Do not store API keys in:

```text
settings.ini
JSON files
QSettings plaintext
environment dumps
logs
```

For developer mode only, optionally support an environment variable.

Example:

```text
WORKSIDEKICK_OPENAI_API_KEY
```

This must never be written back to disk.

---

# 31. Settings

Non-sensitive settings may use:

```text
QSettings
```

Settings include:

```text
selected AI provider
model
base URL where relevant
output language
automatic popup enabled
global shortcut
toolbar orientation
enabled features
feature order
theme
```

API keys are NOT stored in QSettings.

---

# 32. User Profile

Optional profile:

```text
name
job role
team
responsibilities
projects
preferred working language
```

Profile is used only for features that need context.

For example:

```text
Relevance
Add Insight
```

Store locally.

Do not upload profile unless required for the selected AI action.

---

# 33. Clipboard Safety

Create a clipboard helper/guard.

Requirements:

```text
read current clipboard
snapshot text/content where practical
detect clipboard change
restore previous clipboard after temporary copy
```

Avoid this bug:

```text
no selection
      ↓
Ctrl+C does nothing
      ↓
old clipboard still contains text
      ↓
application wrongly treats old text as selection
```

Use clipboard sequence/change information where available.

---

# 34. System Tray

The application should primarily run as a background tray utility.

Use:

```text
QSystemTrayIcon
```

Tray menu:

```text
Open WorkSidekick
Process Clipboard
Enable Automatic Popup
Profile
Settings
Diagnostics
About
Quit
```

Closing ordinary windows must not terminate the tray process.

---

# 35. Diagnostics Window

A diagnostics screen is important because Linux desktop integration varies heavily.

Display:

```text
OS
desktop environment
XDG_SESSION_TYPE
Qt version
AT-SPI available
AT-SPI event listener running
X11 available
GlobalShortcuts portal available
registered shortcut status
secret store available
AI provider configured
automatic popup enabled
```

Do not expose secrets.

Add:

```text
Test Selection
Test Clipboard
Test AI Connection
```

buttons.

This will greatly simplify debugging across different Linux machines.

---

# 36. Privacy and Security Requirements

The application must not:

```text
record all typed text
record clipboard history
monitor conversations continuously
send background screenshots
automatically send generated messages
upload profile unnecessarily
run privileged input daemons
require root
```

Only transmit selected/copied text after an explicit user action.

Automatic popup detection may observe selection metadata, but must not call the AI API automatically.

AI is called only after the user chooses an AI feature.

---

# 37. Application State Flow

Recommended state machine:

```text
Idle

 ↓ selection detected

SelectionAvailable

 ↓

ToolbarVisible

 ↓ feature chosen

CaptureSelection

 ↓ success

BuildingPrompt

 ↓

RequestingAI

 ↓
 ├── success → ResultVisible
 └── error   → ErrorVisible

 ↓

Idle
```

The controller should prevent overlapping accidental actions.

An explicit regenerate action is allowed.

---

# 38. Error Handling

Provide human-readable errors for:

```text
No selected text
Accessibility unavailable
AT-SPI unavailable
Global shortcut unavailable
Clipboard unavailable
API key missing
Authentication failed
Rate limited
Network unavailable
Request timeout
Unsupported application
Empty AI response
```

Do not crash because one platform backend is unavailable.

Gracefully degrade.

---

# 39. Logging

Provide levels:

```text
ERROR
WARN
INFO
DEBUG
```

Production logs may include:

```text
selection backend used
selection character count
provider
request duration
HTTP status
feature
```

Production logs must NOT include:

```text
actual selected text
API key
full generated reply
profile contents
```

Example acceptable:

```text
INFO feature=Polish selection_chars=231 provider=openai duration_ms=841
```

---

# 40. M0 — Ubuntu Minimal Vertical Slice

Implement this first.

Goal:

```text
Select/copy text
      ↓
invoke WorkSidekick
      ↓
perform AI action
      ↓
show result
      ↓
copy result
```

Required M0 functionality:

```text
Ubuntu 22.04 build
Qt tray application
Settings window
API key secure storage
AI provider
Process Clipboard command
Plain Speak
Summarize
Polish
Result Card
Copy result
basic error handling
diagnostics
```

M0 does NOT require automatic selection popup.

This is deliberate.

First prove:

```text
UI → Prompt → API → Result
```

works reliably.

---

# 41. M0 Acceptance Criteria

M0 is complete when:

```text
cmake configure succeeds
build succeeds
application launches
tray icon appears
settings can be opened
API key can be securely saved
clipboard text can be processed
AI result appears without blocking UI
result can be copied
missing API key is handled
network errors are handled
no sensitive text is logged
```

Add a README containing exact Ubuntu installation and build commands.

---

# 42. M1 — Linux AT-SPI Selection

Implement:

```text
AtSpiSelectionProvider
AtSpiSelectionMonitor
```

Goal:

```text
user selects text
      ↓
AT-SPI selection event
      ↓
WorkSidekick detects selected text
```

Initially log only:

```text
application
text length
selection rectangle availability
```

Do not log actual selected text.

Add a developer diagnostics panel displaying the detected selection.

---

# 43. M1 Acceptance Matrix

Test where available:

```text
Firefox
Chrome/Chromium
VS Code
GNOME Text Editor / gedit
Terminal
Slack or another Electron app
```

For each application record:

```text
selection event detected?
text retrieved?
anchor rectangle retrieved?
automatic popup viable?
fallback required?
```

Commit this matrix as:

```text
docs/linux-selection-compatibility.md
```

---

# 44. M2 — Automatic Floating Toolbar

After M1 works:

```text
selection detected
      ↓
ActionBar appears
```

Requirements:

```text
does not steal source focus
appears within roughly 150 ms after settled selection where possible
multi-monitor safe
does not appear for normal clicks
closes when selection disappears
does not repeatedly flicker
```

Toolbar click must capture/retain the selected text before focus changes.

---

# 45. M3 — Linux Fallbacks

Add:

```text
X11 PRIMARY selection fallback
X11 selection gesture fallback
global shortcut abstraction
XDG Portal runtime detection
clipboard mode for Wayland
```

At this point Linux should have these modes:

```text
FULL
AT-SPI + automatic toolbar

PARTIAL
AT-SPI + manual trigger

CLIPBOARD
manual copy + WorkSidekick trigger
```

Diagnostics should clearly show which mode is active.

---

# 46. M4 — Full Feature Set

Add:

```text
Reply Composer
Relevance/Profile
Add Insight
tone variants
regenerate
provider selection
Gemini
Anthropic
OpenAI-compatible APIs
feature customization
```

Do not implement automatic message sending.

---

# 47. M5 — Windows Backend

Only begin Windows after platform interfaces have stabilized.

Implement:

```text
UIASelectionProvider
WindowsSelectionMonitor
WindowsShortcut
WindowsSecretStore
WindowsWindowPolicy
```

Do not fork the whole application into a separate Windows project.

Expected layout:

```text
same AppController
same UI
same FeatureRegistry
same PromptBuilder
same AIService

different platform backend
```

---

# 48. Windows Acceptance Matrix

Test:

```text
Notepad
Microsoft Edge
Google Chrome
VS Code
Teams
Outlook where available
```

Record:

```text
UI Automation selection
selection rectangle
automatic detection
clipboard fallback
focus preservation
```

---

# 49. Unit Tests

At minimum test:

```text
FeatureRegistry metadata
PromptBuilder output
empty selected text
language handling
profile inclusion/exclusion
settings serialization
AI error mapping
clipboard stale-content protection logic where mockable
```

Platform APIs should be wrapped so business logic can be tested with mocks.

Example:

```text
MockSelectionProvider
MockSecretStore
MockAIProvider
MockGlobalShortcut
```

---

# 50. Threading Requirements

The Qt main thread handles UI only.

Never perform blocking:

```text
HTTP
clipboard polling loops
COM traversal
AT-SPI long operations
sleep()
```

on the UI thread.

Prefer:

```text
signals/slots
QTimer
async Qt networking
worker thread where necessary
```

The mouse hook callback on Windows must return immediately.

---

# 51. Performance Goals

Approximate targets:

```text
Idle CPU                < 1%
Idle memory             preferably < 150 MB
Toolbar popup           < 150 ms after settled selection
Selection detection     < 100 ms typical
AI UI loading response  immediate
Application startup     < 2 seconds where practical
```

Do not poll accessibility APIs at high frequency.

Prefer event-driven architecture.

---

# 52. Compatibility and Graceful Degradation

Never design around the assumption:

```text
"every application exposes accessibility perfectly"
```

Selection resolution should behave approximately:

```text
                 ┌─ AT-SPI
Linux ───────────┤
                 ├─ X11 PRIMARY
                 └─ Clipboard mode

                 ┌─ UI Automation
Windows ─────────┤
                 └─ Clipboard fallback
```

If one mechanism fails, the entire application must remain usable.

---

# 53. Coding Style

Use modern C++.

Prefer:

```text
RAII
smart pointers
const correctness
small focused classes
clear ownership
std::optional
enum class
no global mutable state
```

Avoid:

```text
raw owning pointers
giant singleton controllers
blocking sleeps in GUI code
platform #ifdef scattered through core code
```

Platform conditionals should mostly live under:

```text
src/platform/
```

---

# 54. CMake Requirements

Top-level CMake must:

```text
require C++20
find Qt6 components
build shared core sources
select Linux platform backend on Linux
select Windows backend on Windows
enable tests
```

Conceptually:

```cmake
if(UNIX AND NOT APPLE)
    target_sources(worksidekick PRIVATE ${LINUX_SOURCES})
elseif(WIN32)
    target_sources(worksidekick PRIVATE ${WINDOWS_SOURCES})
endif()
```

Do not compile Windows sources on Linux or vice versa.

---

# 55. Ubuntu Development Dependencies

README should provide a command approximately equivalent to:

```bash
sudo apt update

sudo apt install \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    qt6-base-dev \
    libatspi2.0-dev \
    libsecret-1-dev \
    libx11-dev \
    libxi-dev
```

Verify actual required packages during implementation and remove anything unused.

Prefer Ninja builds.

Suggested:

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

---

# 56. Runtime Diagnostics

On startup log something similar to:

```text
WorkSidekick 0.1

Platform: Linux
Desktop: GNOME
Session: wayland

AT-SPI: available
AT-SPI listener: active
X11 backend: unavailable
Global shortcut portal: unavailable
Secret store: available

Selection mode:
AT-SPI + clipboard fallback
```

This must contain no sensitive content.

---

# 57. Documentation Deliverables

Maintain:

```text
README.md
PROJECT_SPEC.md
docs/architecture.md
docs/linux-selection-compatibility.md
docs/windows-selection-compatibility.md
docs/privacy.md
docs/troubleshooting.md
```

Architecture documentation should include a Mermaid diagram if practical.

---

# 58. README Requirements

README must explain:

```text
what WorkSidekick does
supported platforms
Ubuntu build steps
how to configure API
how to use clipboard mode
how automatic selection works
X11 vs Wayland limitations
privacy behavior
troubleshooting
```

Do not claim Wayland functionality that has not been tested.

---

# 59. Explicit Non-Goals

Do NOT implement:

```text
automatic sending
continuous chat scraping
keylogging
screen recording
background screenshots
browser extensions
cloud accounts
conversation-history upload
root daemons
uinput-based keyboard injection
employee monitoring
```

This is an explicit-user-action productivity tool.

---

# 60. Important Implementation Rule

Do not begin by solving every Linux desktop compatibility problem.

Use this development order:

```text
M0
clipboard → AI → result

M1
AT-SPI selection retrieval

M2
automatic floating toolbar

M3
X11 / Wayland fallbacks

M4
feature expansion

M5
Windows
```

A working vertical slice is more valuable than a large amount of untested platform integration code.

---

# 61. First Codex Task

Start now with **M0 only**.

Create the repository skeleton and implement an Ubuntu 22.04 application that can:

```text
run in system tray
open Settings
securely store an OpenAI API key
read explicitly copied clipboard text
run Plain Speak / Summarize / Polish
send request asynchronously
show loading state
show result card
copy result
show useful errors
open Diagnostics
```

Also implement the platform interfaces required for future work, but do not create large placeholder implementations for M1–M5.

Use mocks where useful.

Do not implement automatic selection monitoring yet.

---

# 62. M0 Completion Report

When M0 is finished, return a report containing:

```text
1. Files created
2. Architecture implemented
3. Build command
4. Test command
5. Tests passed/failed
6. Manual behavior tested
7. Known issues
8. Dependencies installed
9. Security/privacy notes
10. Recommended M1 implementation plan
```

Do not proceed to M1 until M0 builds successfully.

If something cannot be tested in the current environment, clearly state:

```text
NOT TESTED
```

rather than assuming it works.

---

# Final Architecture Goal

The finished system should look conceptually like:

```text
                         WorkSidekick
                              │
                         AppController
                              │
       ┌──────────────────────┼───────────────────────┐
       │                      │                       │
 SelectionService        FeatureEngine             UI
       │                      │                       │
       │                 PromptBuilder          ActionBar
       │                      │                 ResultCard
       │                  AIService              Settings
       │                      │
       │              ┌───────┼────────┐
       │              │       │        │
       │            OpenAI Gemini  Anthropic
       │
 ┌─────┴─────────────────────────────────────────────┐
 │                Platform Layer                    │
 │                                                  │
 │ Linux                            Windows          │
 │ ─────                            ───────          │
 │ AT-SPI                           UI Automation    │
 │ X11 PRIMARY                      WH_MOUSE_LL      │
 │ XDG Portal                       RegisterHotKey   │
 │ libsecret                        Credential Mgr   │
 └──────────────────────────────────────────────────┘
```

The critical design objective is:

**platform integration is replaceable; product logic is shared.**

Build the Linux vertical slice first and keep the Windows backend in mind from day one.
