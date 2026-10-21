# M5 Windows compatibility

Native adapter implementation and synthetic integration evidence do not establish
compatibility with every application. No paid AI provider calls are made by tests.

| Application | TextPattern selection | Automatic toolbar / focus | Status |
|---|---|---|---|
| Notepad | Not measured | Not measured | NOT TESTED |
| Microsoft Edge | Not measured | Not measured | NOT TESTED |
| Google Chrome | Not measured | Not measured | NOT TESTED |
| VS Code | Not measured | Not measured | NOT TESTED |
| Microsoft Word | Not measured | Not measured | NOT TESTED |
| Microsoft Outlook | Not measured | Not measured | NOT TESTED |
| Microsoft Teams | Not measured | Not measured | NOT TESTED |

Synthetic Win32 RichEdit/password fixtures and native API results are recorded in
[m5-completion.md](m5-completion.md). They contain only generated test text and
are deliberately distinct from the real-application matrix above.

## Acceptance procedure

Enable Automatic selection toolbar in Settings (default OFF), select ordinary
nonsecret text by dragging/double-clicking, and check Diagnostics counts, source,
anchor and backend state. Confirm the source foreground window does not change
when the toolbar appears. Clicking a feature must use the captured text even if
clipboard contents differ. Opening Reply or More must make no network request.

For keyboard selections, use Ctrl+Alt+P after enabling the shortcut; this performs
fresh UIA, then explicit Clipboard fallback. If registration is unavailable, use
the tray or worksidekick --trigger. Test restart, duplicate launch, disabled
shortcut, missing helper and unsupported TextPattern. Confirm cancellation and
closing during a query do not reopen a stale palette.

Test 100/125/150/200 percent scaling and physically connected mixed-DPI/negative
origin screens separately. Mathematical unit tests are not hardware acceptance.
Elevated/UIPI boundaries, custom editors, protected controls and remote sessions
may limit UIA. No administrator bypass is attempted. Copy text explicitly when
the application does not expose supported selected ranges.
