# Privacy — M5

## Windows native boundaries

Windows observes only left-button gesture metadata through WH_MOUSE_LL and
foreground-window changes to request selected TextPattern ranges. It installs no
keyboard hook and reads no arbitrary control/document text. Password controls and
the app's own process are excluded. Keyboard selections use an explicit shortcut.
The windowless UIA helper returns bounded selection JSON through its private
parent pipe; it is canceled on timeout or stale context. This is local processing,
not upload, logging or persistent history. The toolbar and diagnostics never
automatically request AI.

Windows keys use Credential Manager generic targets WorkSidekick/openai,
WorkSidekick/anthropic, WorkSidekick/gemini and WorkSidekick/openai_compatible.
LOCAL_MACHINE persistence here means this user's credentials survive logoff;
it does not grant other users access. No key is stored in QSettings or migrated
from Linux. Windows QSettings may use the user registry; Profile remains plain
text. Single-instance named pipes carry only bounded open/trigger commands and
use current-user ACLs plus session identity checks, never selected text or keys.

Selecting text, showing the toolbar, opening Reply, editing Profile, switching
providers, saving preferences, triggering a shortcut and displaying diagnostics
make no AI request. Process Clipboard and explicit manual triggers read locally.
AI requires a feature click, Generate reply, a result refinement, or confirmation
of the synthetic connection test. API charges may apply.

Plain Speak, Summarize and Polish send source text and instructions, never Profile.
Reply, Relevance and Add Insight include saved Profile only when inclusion is
enabled (default OFF). Reply also includes optional intent and chosen stance.
Source and context are separate prompt data fields; this is an instruction
boundary, not a guarantee against all prompt injection.

Regenerate sends the original request snapshot. Adjustments send that snapshot
and the latest successful result. Later selection, Profile or provider changes
do not alter the context. Closing cancels locally and clears it. Failed
adjustments preserve the previous successful result for review/copy.

## Storage

The seven Profile fields are saved explicitly in local QSettings as **plain text,
not encrypted**. Clear Profile removes them and disables inclusion. Avoid entering
secrets. Settings also contain feature IDs, model, language, token limit, provider,
compatible base URL and UI preferences. They contain no API keys.

Keys use libsecret / Secret Service with separate provider accounts. Original
`io.worksidekick.ApiKey` / `account=openai` remains unchanged. Other accounts are
`anthropic`, `gemini` and `openai_compatible`. Deleting one preserves others.
Masked key input clears on save, provider change and close. Saved keys are never
read into it. An unavailable keyring has no plaintext fallback.

Selected text and generated content have no persistent app history. Current
selection/result snapshots exist in memory. Qt strings and network buffers cannot
guarantee complete memory zeroization. Copy result intentionally changes the
clipboard, which an external clipboard manager may retain.

## Network and diagnostics

Production endpoints require HTTPS and TLS validation, without redirects.
Compatible URLs reject embedded credentials, query strings and fragments.
HTTP is a constructor-injected loopback test facility only. Keys use headers,
not URLs. Synthetic tests use no paid API.

OpenAI Responses and Gemini Interactions set `store=false`; Anthropic uses a
stateless Messages request. This does not promise zero provider retention.
Each provider's data policies apply. Compatible hosts must support Responses,
not only Chat Completions.

Errors use generic descriptions, never raw response/keyring bodies. The app does
not log source, generated text, Profile contents or keys. Diagnostics show feature
names, Profile enabled/character count, provider/model configured status, credential
availability and last request provider/duration/error only. Selection previews
require the explicit M1 preview action. Qt/system libraries may emit generic logs.

Cancellation cannot recall already sent data, guarantee remote cancellation or
reverse API charges.

## Preserved Linux boundaries

AT-SPI listens only for selection events, extracts bounded selected ranges and
expires its cache after 45 seconds. Toolbar display never uploads. Manual
resolution reads cached AT-SPI, supported X11 PRIMARY, then Clipboard only after
explicit trigger. No polling or clipboard history is added. Hidden palettes clear
their snapshots. Keyboard handling observes only the registered shortcut.

IPC uses a user-restricted local socket with Linux peer-UID checks and sends
commands only. No general input monitoring, production input injection, automatic
paste/replacement, message sending or privileged runtime daemon is added. Native
Linux selection/overlay/shortcut implementations are preserved from M1–M3.
