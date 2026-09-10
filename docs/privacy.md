# Privacy

- Only Process Clipboard reads the current clipboard into the working preview.
  Diagnostics Test Clipboard is another explicit read and shows only its length.
- AI runs only on a feature click, or an explicitly confirmed synthetic connection
  test. M0 has no automatic monitoring, hooks, accessibility scanning or uploads.
- Requests contain the selected preview and action instructions. No profile,
  screenshots, app contents, clipboard history or conversation history is sent.
- API keys use libsecret/Secret Service. Settings contain only model and language.
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
