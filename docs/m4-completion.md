# M4 completion report

Date: 2026-10-21. Version: 0.5.0. Scope stops at M4.

**M4 Gate: PASS. No known acceptance blockers.**

- Accepted M3 baseline: `068c87a815c0f9220e8a5b8cdece2b6408b5abeb`.
- Final implementation commit: `f35b95abae3694b462192340ed1e4c1abe6250b8`.
- Initial full M4 GREEN: `09f48dea549c9066347371f3c40f288d7687ad19`,
  [CI 39606960597](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/39606960597).
- Final Release build: PASS. CTest: **22/22 PASS**.
- Final implementation CI: [39608747439](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/39608747439), job `119251716331`: **SUCCESS**.
- All subsequent delivery changes are documentation only. The implementation SHA above is the exact tested source; the final documentation commit is recorded in the delivery message.

## Product implementation

| Area | Behavior |
|---|---|
| Plain Speak | Explains jargon, preserving facts |
| Summarize | Extracts key points, actions, known owners/deadlines |
| Polish | Default / Professional / Friendly / Concise |
| Reply | Local composer with seven stances and optional intent; Generate is consent |
| Relevance | Uses optional enabled Profile; acknowledges missing context |
| Add Insight | Useful observation/question/suggestion without invented context |
| Profile | Seven bounded fields, explicit Save/Clear, inclusion default OFF |
| Customization | Stable enabled/quick IDs; 3–5 quick actions where possible; More transfers snapshot |
| Regenerate | Original source/feature/variant/Profile/provider/model snapshot |
| Shorter / Longer | Original task/source plus latest successful result |
| Tone | Friendlier / More Direct with the same snapshot |
| Result lifecycle | Single-flight, cancel/close, previous result retained on refinement failure |

FeatureRegistry is the authoritative metadata source for all entrypoints and
variants. PromptBuilder validates bounds/variants and constructs feature-specific
instructions. Source, Profile, intent and previous result are separate data values
where applicable. General actions never include Profile. Prompt boundaries are
best-effort model instructions, not a formal injection-proof guarantee.

Profile is stored in local QSettings as plain text, not encrypted. It is included
only in Reply/Relevance/Add Insight when enabled. Save and Clear make no AI call.
Reply opening/closing makes no request; it owns a copied source snapshot and sends
only after Generate Reply. Source is bounded at 100,000 UTF-16 units, intent at
2,000, individual Profile fields at their displayed limits, serialized Profile at
8,000, and HTTP responses at 4 MiB.

GenerationContext contains original PromptRequest/settings and the latest
successful result, never a key. It is memory-only, cleared on close, and not a
conversation history. Refinements never reread clipboard, PRIMARY, AT-SPI or
current Profile/settings. Late completion after close cannot reopen the result.
Closing a request also restores workspace/settings controls.

## Providers, credentials and migration

AIService dispatches one request from its captured provider ID. OpenAI uses
Responses; Anthropic uses Messages; Gemini uses current Interactions steps; the
compatible adapter requires Responses at an HTTPS base + /responses. Common
contracts cover single-flight, timeout, cancellation, size bounds, no redirects
and sanitized failures. Only final visible text is extracted; thinking/user input
is excluded. Gemini safety handling uses documented error codes, not message
matching or invented status values.

CredentialService routes separate asynchronous stores, locks provider selection
during operations and reports status only. Linux keeps the original libsecret
schema `io.worksidekick.ApiKey`, `account=openai`; new accounts are `anthropic`,
`gemini`, `openai_compatible`. Isolated real Secret Service tests cover separation,
deleting one without affecting others and reading a credential written through
the original OpenAI account. No user's real keyring is used.

Old `ai/model` migrates only if `ai/openai/model` is absent; the old value remains.
Provider-specific model/base URL/token settings are independent. Switching clears
unsaved key input, preserves model configurations and makes no key read or HTTP
request. Test provider requires confirmation and uses synthetic text with no
selection/Profile. No permanent model ID is hardcoded.

## Files

- Core: `Core.*`, `Profile.*`, `FeaturePreferences.*`, `GenerationContext.h`.
- Controller/application: `AppController.*`, `Application.*`,
  `ApplicationProduct.cpp`, `main.cpp`.
- UI: ProfileWindow, ReplyComposer, ResultCard, SettingsWindow, ActionBar,
  ManualActionPalette and WorkspaceWindow.
- AI: AIService, HttpAIProvider, AnthropicProvider, GeminiProvider,
  OpenAICompatibleProvider, IAIProvider/OpenAIProvider extensions.
- Credentials: ISecretStore, CredentialService, LinuxSecretStore and factory.
- Verification: six new CTest suites, original assertion updates for six features,
  real keyring tests, isolated-keyring marker and existing CI gate preservation.
- Docs: M4 specification/plan, this report, provider compatibility, README,
  architecture, privacy and troubleshooting.

## Verification evidence

All builds/tests run on Ubuntu 22.04 CI (Release, Qt 6.2), not this Windows host.
The final implementation passed 22/22 CTest suites with zero failures. Release
configure/build, isolated Secret Service (5 QtTest entries, zero skipped), launch
smoke, real AT-SPI event/unavailable-registry checks, Openbox X11 overlay focus
and global shortcut checks all passed in the linked final CI run. M0, M1, M2,
M3 and privacy regressions are PASS. Both new error-handling fixes are GREEN.

| Gate | Coverage |
|---|---|
| M0 regression | Core/API/UI/controller/manual clipboard; result copy/cancel; keyring; launch smoke |
| M1 regression | AT-SPI parser/monitor plus real accessibility events and unavailable registry |
| M2 regression | Placement/window-policy plus Openbox X11 source-focus integration |
| M3 regression | Resolver/manual/IPC, portal mock DBus, X11 registered shortcut |
| M4 core | Six features, profile applicability/bounds, preferences normalization, idempotent migration |
| M4 generation | Reply consent, immutable context, refinement chain, failure preservation, late read cancellation |
| M4 widgets/integration | Profile Save/Clear, composer, busy/close recovery, More snapshot, all-disabled actions, provider models/key clearing |
| M4 HTTP | Four protocol/auth/error/timeout/cancel suites; actual loopback Profile privacy and refinement bodies |
| M4 credentials | Fake routing plus real isolated provider accounts and legacy OpenAI compatibility |
| Privacy | Local UI zero requests/reads, content-free diagnostics, no private error echo, no persistent generation history |

TDD evidence:
- [37476009430](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/37476009430):
  compile passed; old/core generation suites passed; 3 new suites failed on stubs.
- [39606960597](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/39606960597):
  22/22 plus Secret Service, smoke, native AT-SPI and X11 gates passed.
- [39607247200](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/39607247200):
  only the 12 newly specified Gemini safety-code cases failed before the fix.
- [39608136790](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/39608136790):
  only Profile deletion persistence reporting failed (21/22 suites passed);
  a directory used as the INI target deterministically produces a write error.

Final GREEN for both added failure tests: [39608747439](https://github.com/wzk111/leadership-dandang-win-linux/actions/runs/39608747439).

Reviews followed specification then code-quality checks. Findings addressed include
workspace recovery after close, truthful Profile upload guidance, documented
Gemini safety classification, and Profile deletion persistence reporting.

## Limits and next milestone

Real OpenAI API: **NOT TESTED**.
Real Anthropic API: **NOT TESTED**.
Real Gemini API: **NOT TESTED**.
Real compatible server: **NOT TESTED**.

Paid live API tests are not required for M4 PASS by the accepted specification.
No credentials or paid calls were used. Provider model access, actual output
quality and future protocol revisions remain account-dependent.

Real GNOME X11/Wayland desktop app matrix, multi-monitor/HiDPI/fractional scaling,
native Wayland portal approval and native Windows builds: **NOT TESTED**.
CI Xvfb/Openbox is narrower than those environments. Native Wayland anchored
overlay remains unsupported; the established explicit manual path remains.

No streaming, provider discovery, persistent history, automatic send/paste/replace
or Windows backend is added. Recommended next milestone is **M5 — Windows Native
Backend**, only after the user's next instruction. M5 has not started.
