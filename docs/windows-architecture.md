# Windows architecture — M5 (0.6.0)

M5 preserves the M4 product layer: six features, Profile, Reply, refinements,
provider protocols, explicit generation and result copying. The platform factory
selects Windows adapters at build time. Linux sources and native CI gates remain.

## Selection and process boundary

WindowsSelectionMonitor installs WH_MOUSE_LL and an out-of-context foreground
WinEvent hook on the Qt event thread. Mouse callbacks copy only left-button
gesture metadata and queue processing; they perform no COM, clipboard or network
work. Drag/double-click completion starts an 80 ms debounce. Single clicks clear
old context; foreground changes, stop and expiry invalidate pending generations.
The cached selection expires after 45 seconds. Keyboard selections use the
explicit Ctrl+Alt+P query; automatic UIA text-selection event subscriptions are
not enabled in this milestone.

UIAQuery asynchronously launches the adjacent worksidekick_uia_worker.exe with
the captured HWND and excluded main-process PID. The windowless helper initializes
COM MTA, queries the focused element's TextPattern.GetSelection, releases every
COM interface and calls CoUninitialize. It does not traverse the desktop tree or
substitute Name/Value/document text. Password and own-process elements are rejected,
and foreground/focus identity is verified before accepting the result. Multiple
ranges preserve whitespace with newline separators, bounded to 100,000 UTF-16
code units. Missing geometry does not discard valid text.

A per-query helper isolates unresponsive accessibility providers from the GUI.
Its private inherited stdout transports at most 2 MiB of JSON; stderr is discarded.
Timeout is 2 seconds, cancellation invalidates process identity/sequence and kills
the child, and retired children are bounded. This local selected-text transport is
separate from the content-free single-instance command pipe. No text is logged.
The process-per-query startup cost is a known performance tradeoff; the <150 ms
target is not assumed achieved.

WindowsSelectionResolver queries fresh UIA before reading Clipboard on an explicit
trigger. It reuses SelectionResolver's validation and source ordering, has no
PRIMARY path, and discards results after foreground/context/generation changes.
Application waits without activating a palette, and guards late callbacks with
QPointer and a request generation. Only an explicit feature action invokes AI.

## Coordinates and focus

UIA returns physical screen rectangles; the last meaningful range rectangle is
used. WindowsWindowPolicy matches native HMONITOR handles through QWindowsScreen
(Qt 6.7+, validated with Qt 6.8.3), scales
screen-local offsets by DPR and retains Qt's virtual screen origin. It does not
divide global coordinates by a scale. QScreen friendly names are not device IDs.
The shared placement function clamps to
available screen geometry. Qt initializes application DPI awareness before the
first window; only the separate windowless helper sets its own DPI awareness.

ActionBar keeps the shared widget and captured Selection value. Qt no-focus/tool
flags, WS_EX_NOACTIVATE and WS_EX_TOOLWINDOW, WM_MOUSEACTIVATE/MA_NOACTIVATE, and
SetWindowPos/SWP_NOACTIVATE preserve source focus. Neither WS_EX_TRANSPARENT nor
WS_EX_LAYERED is added. A feature click processes the captured value, never a
fresh clipboard value. Real mixed-DPI hardware remains a separate acceptance test.

## OS services

- WindowsGlobalShortcut uses a private message-only HWND and application-range ID
  for RegisterHotKey(Ctrl+Alt+P, MOD_NOREPEAT). Qt's native event filter routes its
  WM_HOTKEY only. Failure preserves foreign bindings; stop unregisters its own
  binding and destroys its HWND. There is no keyboard hook.
- WindowsSecretStore uses asynchronous CredReadW/CredWriteW/CredDeleteW with
  CRED_TYPE_GENERIC and CRED_PERSIST_LOCAL_MACHINE, meaning persistence for this
  Windows user across logoff. Targets are WorkSidekick/openai, /anthropic,
  /gemini and /openai_compatible. CredentialService preserves provider isolation.
  No plaintext fallback or Linux-key migration is implemented.
- WindowsInstanceCoordinator is a separate source selected by CMake. SID, session
  and canonical directory determine endpoint/lock identity. A protected, owned,
  non-reparse directory and current-user pipe ACL isolate the server. QLockFile
  arbitrates startup; secondary instances validate the server process SID/session.
  Commands are bounded open/trigger messages, with acknowledgement/deadlines and
  a client cap. No text, credentials, arbitrary command or TCP endpoint is exposed.

## Build and evidence

Windows CI pins windows-2022, MSVC x64, Qt 6.8.3 and Ninja Release. Shared tests
run on both systems; native tests cover Credential Manager, shortcut ownership,
actual pipe/folder ACLs, startup races, UIA worker and focus/style behavior.
Desktop-unavailable tests explicitly skip and cannot establish desktop support.
See [completion](m5-completion.md) and [compatibility](m5-windows-compatibility.md)
for measured results and remaining limits.
