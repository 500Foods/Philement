<!-- markdownlint-disable MD007 MD024 -->
# `POST /api/conduit/auth_chat` — Authenticated Single Chat

## Overview

Single-engine authenticated chat completion. Supports non-streaming and
streaming (real Server-Sent Events over HTTP). For the WebSocket
equivalent see
[`/docs/H/core/subsystems/websocket/websocket_chat.md`](/docs/H/core/subsystems/websocket/websocket_chat.md).
For multi-engine broadcast see
[`/docs/H/api/chat/auth_chats.md`](/docs/H/api/chat/auth_chats.md).

This document is for Hydrogen **Phase 9**. The plan that produced this
contract is [`CHAT_FINALE.md`](/docs/H/plans/complete/CHAT_FINALE_COMPLETE.md) (Phases 0–3).

## Authentication

Hydrogen JWT in the `Authorization` header.

```text
Authorization: Bearer <jwt>
```

The JWT must carry:

| Claim | Required value | Why |
| --- | --- | --- |
| `aud` | `"hydrogen-chat"` | chat-specific audience (Phase 0 lock) |
| `roles` | includes `"chat"` | chat role (Phase 0 lock) |
| `database` | non-empty | binds to ChatEngineCache (existing) |
| `sub` | non-empty | subject (existing) |

A JWT minted for `MCP.Resource` (the hosted-MCP connector) is **rejected
on purpose** so chat scoping is real, not decorative. A non-chat JWT
returns **HTTP 403** with a stable JSON envelope from
`send_jwt_error_response`. The full 1-hour account JWT is accepted when
its `aud`/`roles` match.

The MCP-Resource minted JWT is short-TTL (15-minute default) and rejected
here because chat and MCP are two independent policies. The minted token
is treated as **already leaked to xAI** on departure; the chat endpoint
has no use for it.

## Request Body

```json
{
  "engine": "ChatGPT 4o",
  "messages": [
    { "role": "system", "content": "You are a helpful assistant." },
    { "role": "user",   "content": "What is the capital of France?" }
  ],
  "temperature": 0.7,
  "max_tokens": 500,
  "stream": false,
  "reasoning": "low",
  "context_hashes": ["abc123", "def456"]
}
```

Field reference (parsed by `auth_chat_parse_request`):

| Field | Required | Type | Notes |
| --- | --- | --- | --- |
| `messages` | yes | array of `{role, content}` | `content` may be a string or an array of multimodal parts (Responses / Chat Completions). Order is preserved. |
| `engine` | no | string | Human-readable engine name. Resolved to provider + model + endpoint via `chat_engine_cache_lookup_by_name`. If absent the database default engine is used. |
| `temperature` | no | number, 0.0–2.0 | Passed through to the provider. Omitted / `< 0` falls back to engine default. No hardcoded `1.0` (Phase 1). |
| `max_tokens` | no | integer, 1–128000 | Mapped to `max_tokens` (Chat Completions / Anthropic) or `max_output_tokens` (Responses). |
| `stream` | no | bool | Default `false`. `true` returns Server-Sent Events (see [Streaming](#streaming)). |
| `reasoning` | no | string | Effort label (`"low"`, `"medium"`, `"high"`, or vendor-specific). Passed through; providers that do not support it ignore it. |
| `context_hashes` | no | array of strings | Hash references resolved via `chat_storage_resolve_media_in_content` before the provider sees the body. Per-hash hit/miss stats are returned on the response when `context_hashes` is present. |
| `media` | not used here | — | Use the dedicated WebSocket `media_upload` frame, or `POST /api/media/upload`. |

There is **no `tools` field in client JSON.** When hosted MCP is enabled
on the engine (`use_responses_api = true` and `MCP.Resource` reachable),
Hydrogen injects `tools: [{type: "mcp", ...}]` into the **provider**
body (not the response to the client). Clients never see MCP wire.

## Non-Streaming Response

`HTTP 200 OK` with `Content-Type: application/json`:

```json
{
  "success": true,
  "result": {
    "content": "The capital of France is Paris.",
    "model": "gpt-4o",
    "finish_reason": "stop",
    "response_time_ms": 850,
    "tokens": { "prompt": 25, "completion": 10, "total": 35 }
  },
  "context_hashing": {
    "hashes_used": 2,
    "hashes_missed": 0,
    "bandwidth_saved_bytes": 412,
    "bandwidth_saved_percent": 38.4
  },
  "raw_provider_response": {}
}
```

`context_hashing` is present only when the request supplied
`context_hashes`. The same stats are reported on streaming
`chat_done` and on the WebSocket equivalent (see
[`websocket_chat.md`](/docs/H/core/subsystems/websocket/websocket_chat.md)).

## Streaming

`stream: true` returns real Server-Sent Events. The transport is MHD
incremental response driven by the multi-curl worker:

```text
HTTP/1.1 200 OK
Content-Type: text/event-stream
Cache-Control: no-cache

data: {"type":"chat_chunk","id":"req-abc","chunk":{"content":"The capital ","reasoning_content":null,"extra_fields":null,"index":0,"finish_reason":null}}

data: {"type":"chat_chunk","id":"req-abc","chunk":{"content":"of France","reasoning_content":null,"extra_fields":null,"index":1,"finish_reason":null}}

data: {"type":"chat_chunk","id":"req-abc","chunk":{"content":" is Paris.","reasoning_content":null,"extra_fields":null,"index":2,"finish_reason":"stop"}}

data: {"type":"chat_done","id":"req-abc","result":{"content":"The capital of France is Paris.","model":"gpt-4o","finish_reason":"stop","response_time_ms":850,"tokens":{"prompt":25,"completion":10,"total":35}},"context_hashing":{"hashes_used":2,"hashes_missed":0,"bandwidth_saved_bytes":412,"bandwidth_saved_percent":38.4},"raw_provider_response":{}}
```

Each SSE frame is `data: <json>\n\n`. The MHD incremental callback is
driven by a non-blocking pipe fed by the multi-curl worker thread; the
client sees events as they arrive from the provider.

### Failure mode

If the multi-curl worker fails before any data is delivered (provider
4xx/5xx, network error, engine not found), the response is a single SSE
error event:

```text
data: {"type":"chat_error","id":"req-abc","error":"<message>"}

```

The client must distinguish SSE error events from chat_error payload by
the `"type":"chat_error"` field. Real chunk errors mid-stream abort the
connection cleanly without leaving the CURL easy handle or stream buffers
leaked (Phase 4 fix to `chat_proxy_multi_stream_stop`).

## Errors

| Status | Body | Cause |
| --- | --- | --- |
| `400` | `{"error":"Endpoint not found"}` | Wrong path; the legacy `/api/auth_chat/stream` stub is **not** registered (Phase 3 removed it). |
| `400` | `{"success":false,"error":"Invalid request body"}` | Body did not parse as JSON. |
| `400` | `{"success":false,"error":"Missing or invalid 'messages' array"}` | `messages` field missing or not an array. |
| `400` | `{"success":false,"error":"Temperature must be between 0.0 and 2.0"}` | Validation range. |
| `400` | `{"success":false,"error":"max_tokens must be between 1 and 128000"}` | Validation range. |
| `401` | `{"success":false,"error":"Authentication required ..."}` | Missing / malformed `Authorization` header. |
| `403` | `{"success":false,"error":"JWT not authorized for chat"}` | `validate_chat_jwt_claims` rejected `aud` or `role` (or the token is the MCP-Resource minted token). |
| `404` | `{"success":false,"error":"Engine '<name>' not found"}` | `chat_engine_cache_lookup_by_name` miss. |
| `500` | `{"success":false,"error":"engine_key not yet loaded"}` / `"database_queue not available"` | Engine cache / Database subsystem not ready; usually transient during startup. |
| `502`/`504` | SSE error event then connection close | Upstream provider failure propagated. |
| `429` | `{"success":false,"error":"rate_limited","message":"...","error_code":4291}` | Per-sub request cap exceeded (`4291`) or token budget exceeded (`4292`). Phase 10b. Fails closed per request; the sub recovers after `Chat.RateLimit.IntervalSeconds` elapses. |

`401` / `403` paths use the `send_jwt_error_response` helper. After that
helper queues the response, the route handler must return **`MHD_YES`**
(not `MHD_NO`) so MHD actually delivers the queued body — Phase 5 fixed
this in `auth_chat.c:373`.

## Provider Routing

| Engine flag | Wire format | Notes |
| --- | --- | --- |
| `use_responses_api = true` (xAI / OpenAI) | `/v1/responses` | `input` instead of `messages`, `max_output_tokens` instead of `max_tokens`. Hosted MCP connector injected when MCP Resource is reachable. |
| `provider == "anthropic"` | `/v1/messages` | Anthropic Messages. Temperature emitted (Phase 1). |
| else | `/v1/chat/completions` | OpenAI-compatible (Ollama, Groq, OpenRouter, Gradient AI). |

`additional_params` overlay merges into all three builders.

## Supported Providers

| Provider | Engine collection flag | Wire format |
| --- | --- | --- |
| xAI (Grok, `grok-4.6`) | `use_responses_api: true` | `/v1/responses` |
| OpenAI | `use_responses_api: true` | `/v1/responses` |
| Anthropic | always | `/v1/messages` |
| Ollama | always | `/v1/chat/completions` |
| Groq / OpenRouter / Gradient AI | always | `/v1/chat/completions` |

## Configuration

Per-engine JSON lives in the `chat_engines` collection of the bound
database. Engine `use_responses_api` is loaded by
`chat_engine_cache_load_from_database`. Local MCP is a per-engine
`local_mcp` block (default off):

```json
"local_mcp": {
  "enabled": true,
  "servers": [{
    "url": "https://mcp.example.com/mcp",
    "authorization": "Bearer ...",
    "allowed_tools": ["System.Info"]
  }]
}
```

Empty `allowed_tools` skips that server (fail-closed; never "all tools").

### Responses API `store` (data residency)

For engines that route through `/v1/responses` (xAI Grok, OpenAI when
`use_responses_api = true`), the Responses body carries a top-level
`store` field. **Default `false`** — opt-in to provider-side retention.

| Engine field | Body field | Default | Effect |
| --- | --- | --- | --- |
| `"store": true` | `"store": true` | — | xAI retains request and response bodies for 30 days |
| `"store": false` (or omitted) | `"store": false` | false | xAI does not retain request and response bodies |

This is a **behavior change** for engines that previously omitted the
field: xAI's own default is `store=true` (30-day retention), so any
engine JSON that does not set `store` was, before Phase 10a, implicitly
opting in to retention. After Phase 10a, the same engine JSON opts out
by default. Operators who want retention must explicitly add
`"store": true` to their engine collection. The field is ignored by
Chat Completions (`/v1/chat/completions`), Anthropic (`/v1/messages`),
and Ollama adapters.

### Per-sub Rate Limiting (Phase 10b)

Subsystem-wide rate limiting is configured under the new top-level
`Chat` section in `hydrogen.json`:

```json
"Chat": {
  "RateLimit": {
    "Enabled": false,
    "MaxRequestsPerInterval": 60,
    "IntervalSeconds": 60,
    "MaxTokensPerInterval": 100000
  }
}
```

| Field | Default | Effect |
| --- | --- | --- |
| `Enabled` | `false` | Master switch. `false` short-circuits the check and every request is allowed. |
| `MaxRequestsPerInterval` | `60` | Per-`sub` request cap inside the window. `0` disables the request cap (token cap still applies). |
| `IntervalSeconds` | `60` | Fixed-window length. `<= 0` fails open. |
| `MaxTokensPerInterval` | `100000` | Per-`sub` token-budget cap. Input tokens are estimated at request entry from concatenated message content (chars/4 heuristic). Output tokens are read from the streaming provider's `usage.completion_tokens`; non-streaming and local providers emit no usage, so output is recorded as `0` and the token cap throttles them by request count only. `0` disables the token cap. |

The bucket key is the JWT `sub` claim. Different `sub`s are
independent — throttling one user does not affect another. WebSocket
chokepoints re-key per message so a connection that rotates its JWT
mid-session cannot bypass the limit by changing `sub`.

When `Enabled=true` and a request exceeds either cap the endpoint
returns `429 Too Many Requests` with
`{success:false, error:"rate_limited", message:"...", error_code:<4291|4292>}`.
`4291` = request cap exceeded, `4292` = token budget exceeded. The
sub's bucket is **not** advanced on a throttle, so a throttled request
does not consume any capacity.

The module fails **open** on allocation error (`calloc` returning
NULL), on missing/disabled config, and on NULL/empty `sub` — the same
posture as `mailrelay_event_check_rate_limit` for mail relay events.
This is intentional: an internal fault must not lock out legitimate
users.

## Testing

- **Test 59** — REST + WS coverage. After Phase 3 the stream test
  asserts real SSE (200 with chunks) instead of 501. After Phase 4 the
  mock engine exercises `media:` resolution parity and `context_hashes`
  stats on `chat_done`.
- **Unity** — `auth_chat_test_helpers.c` (parameter resolution),
  `auth_jwt_helper_test.c` (chat JWT policy),
  `req_builder_test_temperature.c` (temperature / overlay / Responses
  routing), `resp_parser_test_responses.c` (Responses SSE parsing),
  `req_builder_test_hosted_mcp.c` (hosted MCP connector shape),
  `req_builder_test_responses_store.c` (Responses `store` knob),
  `chat_rate_limit_test.c` (per-sub request + token bucket, envelope
  shape, fail-open paths).

## Related Documentation

- [`/docs/H/api/chat/auth_chats.md`](/docs/H/api/chat/auth_chats.md) — multi-engine broadcast (non-stream only).
- [`/docs/H/core/subsystems/websocket/websocket_chat.md`](/docs/H/core/subsystems/websocket/websocket_chat.md) — WebSocket equivalent (single socket, both stream + non-stream).
- [`/docs/H/core/subsystems/mcp/mcp.md`](/docs/H/core/subsystems/mcp/mcp.md) — MCP server; Grok talks to the public Resource via hosted MCP.
- [`CHAT_FINALE.md`](/docs/H/plans/complete/CHAT_FINALE_COMPLETE.md) — plan of record.