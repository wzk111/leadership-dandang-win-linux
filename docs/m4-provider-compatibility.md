# M4 provider compatibility

Protocols checked against official documentation on 2026-10-21. Runtime tests use
loopback HTTP fixtures; **real paid APIs are NOT TESTED**. Model access, billing,
quota and output quality require a user's own account.

| Provider | Production endpoint | Contract | Live API |
|---|---|---|---|
| OpenAI | `https://api.openai.com/v1/responses` | Bearer; instructions/input; store=false; output_text | NOT TESTED |
| Anthropic | `https://api.anthropic.com/v1/messages` | x-api-key; anthropic-version=2023-06-01; system/messages/max_tokens; text blocks | NOT TESTED |
| Gemini | `https://generativelanguage.googleapis.com/v1beta/interactions` | x-goog-api-key; system_instruction/input/generation_config; store=false; model_output text | NOT TESTED |
| OpenAI-compatible | HTTPS base + `/responses` | Responses and Bearer; no Chat Completions fallback | NOT TESTED |

AIService routes one active request using its captured provider ID. Failures do
not switch provider or reuse another provider's key. All adapters enforce a
30-second timeout, 4 MiB response bound, cancellation and no redirects. Compatible
URLs reject credentials/query/fragment. HTTP is enabled only for injected tests.

Fixtures cover headers/body, success, 401/403, 429, server/redirect errors, malformed
JSON, empty output, size limit, timeout and cancellation. Anthropic separates text
from thinking, refusal from incomplete output. Gemini excludes thought/user_input
steps and maps documented HTTP 400 safety codes to a safe refusal. Unknown codes
remain generic errors. Raw error bodies are never rendered. No tools, images,
background jobs, streaming or previous response/interaction IDs are requested.

Gemini uses the May 2026 `steps` schema, not obsolete `outputs[]`. Future revisions
may need adapter changes. Sources:
[Interactions migration](https://ai.google.dev/gemini-api/docs/interactions-breaking-changes-may-2026),
[Interactions API](https://ai.google.dev/api/interactions-api),
[safety settings](https://ai.google.dev/gemini-api/docs/safety-settings),
[API errors](https://ai.google.dev/gemini-api/docs/api-errors).
Anthropic follows [Messages](https://platform.claude.com/docs/en/api/messages/create)
and [stop reasons](https://platform.claude.com/docs/en/build-with-claude/handling-stop-reasons).
OpenAI follows [Responses](https://platform.openai.com/docs/api-reference/responses).

Models are configured explicitly, without permanent hardcoded names. UI token
limits are 1–16,384; models may impose different constraints. Compatible services
must accept the fields sent here, including `store` and `max_output_tokens`.

Configuration uses `ai/provider` and `ai/<provider>/{model,baseUrl,maxOutputTokens}`.
Legacy `ai/model` migrates to `ai/openai/model` only if absent, preserving the old
setting. Original OpenAI keyring schema/account is unchanged. Other providers use
separate accounts; isolated keyring integration tests protect user credentials.
