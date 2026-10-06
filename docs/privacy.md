# Privacy

- Only Process Clipboard reads the current clipboard into the working preview.
  Diagnostics Test Clipboard is another explicit read and shows only its length.
- AI runs only on a feature click, or an explicitly confirmed synthetic connection
  test. M0 has no automatic monitoring, hooks, accessibility scanning or uploads.
- Requests contain the selected preview and action instructions. No profile,
  screenshots, app contents, clipboard history or conversation history is sent.
- API keys use libsecret/Secret Service. Settings contain model, language and the automatic-toolbar preference.
  The key field is masked and cleared after save and on close.
- Requests use the fixed official HTTPS endpoint, normal TLS validation and no
  automatic redirects. Tests inject only a loopback HTTP endpoint.
- Responses API requests set store=false. This disables API response storage;
  it is not a promise of zero retention by the provider. Review
  [OpenAI data controls](https://platform.openai.com/docs/guides/your-data).
- The application emits no private-content logs and does not show raw API or
  keyring error payloads. Qt/system libraries may emit generic diagnostics.
- Text and keys necessarily exist in process memory while used. Qt strings and
  network buffers cannot guarantee complete memory zeroization. There is no
  on-disk content history. The result is cleared when its window closes.
- Copy result intentionally replaces the clipboard. Other applications or an OS
  clipboard manager may retain it; WorkSidekick does not implement such a history.
- Cancelling stops local waiting/network activity; it cannot recall text already
  transmitted to the API or guarantee reversal of provider charges.
- This application never clicks Send, submits forms, injects keystrokes or requires
  a privileged runtime process.

## M1 local accessibility extension

M1 additionally observes object:text-selection-changed. It retrieves only the event
source's selected range into local memory, with size limits and a 45-second expiry.
There is no AI/upload connection from this monitor. Source text is absent from
metadata; Show Last Selection Preview is required to display it. Preview clears
on change/stop/expiry/hide. No selected text is logged, persisted or copied by M1.
This updates the earlier M0-only statement that the app has no automatic monitor;
there is still no keyboard monitoring, general text-change listener or scraping.

## M2 explicit selection action

Automatic toolbar display is local and is not permission to upload. Only a
Plain Speak, Summarize or Polish click sends the captured toolbar Selection
through the existing AI pipeline. New selections replace the stored value; a
hidden/cleared toolbar cannot execute a stale selection. No accessibility or
clipboard read is performed when clicking the toolbar.

The toolbar shows feature labels, not source text. Diagnostics automatically
shows capability, raw rectangle, requested placement and status only. The M1
opt-in preview remains separate. The automatic-popup preference is non-sensitive
and stored in QSettings, alongside the existing model/language keys.

Clipboard contents are untouched until an explicit Copy result. No global input
hooks, input injection, automatic replacement, message sending, profile collection,
new AI provider or selection history is added.

## M3 explicit manual fallback

Shortcut activation and --trigger are local consent to read/preview, not consent
to upload. On explicit trigger only, the resolver tries cached AT-SPI, read-only
X11 PRIMARY where supported, then Clipboard. The palette visibly identifies its
source so a user can reject stale clipboard data. The complete value snapshot is
preserved behind the bounded local preview; only a feature click sends it to AI.

No periodic PRIMARY/clipboard reads, change listener, history or content logging
is introduced. Closing/hiding the palette clears its snapshot and preview.
Shortcut settings are independent of automatic toolbar settings. The default is
OFF; portal permission/configuration UI requires explicit enable/retry.

X11 observes only the registered key combination, not general keyboard content.
XTest belongs solely to the integration-test target; it is not linked by the
production shortcut backend. No production input injection, automatic paste,
replacement or message sending is implemented.

Single-instance IPC uses only a user-restricted local socket and commands, no
public TCP port. Linux peer uid is checked. No selected text or API key crosses
this IPC channel. The existing Secret Service storage remains unchanged.
