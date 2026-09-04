<!-- markdownlint-disable MD007 MD024 -->
# `POST /api/conduit/auth_chats` — Authenticated Multi-Engine Broadcast

## Overview

Authenticated broadcast chat: one request, many engines, one consolidated
response. **Non-streaming only** — there is no SSE / streaming variant
of `auth_chats`. For a single-engine stream, use
[`/docs/H/api/chat/auth_chat.md`](/docs/H/api/chat/auth_chat.md) with
`stream: true` (or the WebSocket `chat` frame).

This document is for Hydrogen **Phase 9**. The plan that produced this
contract is [`CHAT_FINALE.md`](/docs/H/plans/complete/CHAT_FINALE_COMPLETE.md) (Phases 0–3).

## Authentication

Identical to
[`/docs/H/api/chat/auth_chat.md`](/docs/H/api/chat/auth_chat.md):

- `Authorization: Bearer <jwt>`
- JWT must carry `aud = "hydrogen-chat"`, `roles` includes `"chat"`,
  `database` non-empty.
- MCP-Resource minted JWT is rejected on purpose (403).
- Non-chat JWT returns 403 via `send_jwt_error_response` from
  `auth_chat.c`.

## Request Body

```json
{
  "engines": ["ChatGPT 4o", "Grok 4.6", "Llama 3 70B"],
  "messages": [
    { "role": "system", "content": "You are a helpful assistant." },
    { "role": "user",   "content": "Compare your answers: what is the capital of France?" }
  ],
  "temperature": 0.5,
  "max_tokens": 500,
  "reasoning": "low"
}
```

Field reference (parsed by `auth_chats_parse_request`):

| Field | Required | Type | Notes |
| --- | --- | --- | --- |
| `engines` | yes | array of 1–10 strings | Engine names from the database ChatEngineCache. Same names as `auth_chat.engine`. > 10 engines returns 400. |
| `messages` | yes | array of `{role, content}` | Same shape as `auth_chat.messages`. Order is preserved; sent verbatim to each engine. |
| `temperature` | no | number | Passed through to each engine. Omitted / `< 0` falls back to engine default. |
| `max_tokens` | no | integer, 1–128000 | Passed through to each engine. Mapped to `max_tokens` or `max_output_tokens` depending on engine wire format. |
| `reasoning` | no | string | Effort label; passed through where supported. |
| `stream` | **not accepted** | — | Multi-engine broadcast is non-stream only. |
| `context_hashes` | **not accepted** | — | Multi-engine broadcast does not run the bandwidth-optimization path. Use `auth_chat` (single-engine) if you need it. |
| `media` | not used here | — | Use the dedicated WebSocket `media_upload` frame, or `POST /api/media/upload`. |

There is **no `tools` field in client JSON.** Hosted MCP, when enabled
on a given engine, is still injected at the provider-body layer.

## Response

`HTTP 200 OK` with `Content-Type: application/json`:

```json
{
  "success": true,
  "results": [
    {
      "engine": "ChatGPT 4o",
      "model": "gpt-4o",
      "content": "The capital of France is Paris.",
      "finish_reason": "stop",
      "response_time_ms": 920,
      "tokens": { "prompt": 25, "completion": 12, "total": 37 }
    },
    {
      "engine": "Grok 4.6",
      "model": "grok-4.6",
      "content": "Paris is the capital of France.",
      "finish_reason": "stop",
      "response_time_ms": 1100,
      "tokens": { "prompt": 25, "completion": 9, "total": 34 }
    },
    {
      "engine": "Llama 3 70B",
      "success": false,
      "error": "Provider returned 503"
    }
  ]
}
```

Per-engine outcomes are independent. A failure on one engine does not
abort the others; the failed entry carries `success: false` and an
`error` string. The top-level `success` is true if the response shape is
valid — check each entry's `success` field to detect per-engine
failures.

## Errors

| Status | Body | Cause |
| --- | --- | --- |
| `400` | `{"success":false,"error":"Missing or invalid 'engines' array"}` | `engines` missing or not an array. |
| `400` | `{"success":false,"error":"Engines array must contain 1-10 engine names"}` | Empty or > 10 engines. |
| `400` | `{"success":false,"error":"Missing or invalid 'messages' array"}` | `messages` field missing or not an array. |
| `401` | `{"success":false,"error":"Authentication required ..."}` | Missing / malformed `Authorization` header. |
| `403` | `{"success":false,"error":"JWT not authorized for chat"}` | `validate_chat_jwt_claims` rejected. |
| `404` | `{"success":false,"error":"Engine '<name>' not found"}` | One or more engine names did not resolve via `chat_engine_cache_lookup_by_name`. |

`401` / `403` paths use `send_jwt_error_response`; the route handler
must return `MHD_YES` so MHD delivers the queued body (Phase 5 fix in
`auth_chats.c:372`).

## Provider Routing

Each engine is routed independently by its own `provider` /
`use_responses_api` flags. Different engines in the same broadcast can
target different providers (OpenAI Responses + Anthropic Messages +
Ollama Chat Completions in the same call).

## Supported Providers

Same matrix as `auth_chat` (see
[`/docs/H/api/chat/auth_chat.md`](/docs/H/api/chat/auth_chat.md#supported-providers)).

## Configuration

Same per-engine JSON config in the `chat_engines` collection; same
`local_mcp` block (default off).

Per-sub rate limiting (`Chat.RateLimit.*`, Phase 10b) is shared with
`auth_chat` and applies once per inbound broadcast — completion tokens
across the fanout are aggregated into the sub's token budget. See
[`auth_chat.md` Per-sub Rate Limiting](/docs/H/api/chat/auth_chat.md#per-sub-rate-limiting-phase-10b).

## Testing

- **Test 59** — exercises broadcast alongside the single-engine path.
- **Unity** — `auth_chats_test_helpers.c` (parameter resolution).

## Related Documentation

- [`/docs/H/api/chat/auth_chat.md`](/docs/H/api/chat/auth_chat.md) — single-engine chat (streaming + non-stream).
- [`/docs/H/core/subsystems/websocket/websocket_chat.md`](/docs/H/core/subsystems/websocket/websocket_chat.md) — WebSocket equivalent.
- [`CHAT_FINALE.md`](/docs/H/plans/complete/CHAT_FINALE_COMPLETE.md) — plan of record.