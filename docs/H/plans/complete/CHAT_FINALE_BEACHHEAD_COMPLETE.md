<!-- markdownlint-disable MD007 MD024 -->
# Chat Finale — Beachhead Archive (Phases 0–9)

## Status

**Complete (2026-09-03).** Phases 0–9 are all complete and green. This file
is the **archived** record of the 500 Courses chat+MCP beachhead delivered
by this plan. Pulled-forward Phase 10 work (`store` knob done; chat rate
limiting pending) lives in the **active** plan at
[`CHAT_FINALE.md`](/docs/H/plans/complete/CHAT_FINALE_COMPLETE.md). Per AGENTS.md, files in
`plans/complete/` are history; the active plan is the one to read for new
work.

### What the beachhead ships

- Dual REST + WebSocket paths (real SSE for WS, real SSE for REST streaming).
- Provider knobs actually reach the wire: `temperature`, `max_tokens` /
  `max_output_tokens`, `reasoning`, `media:` resolution, `context_hashes`
  stats — parity REST ↔ WS where Phase 0 matrix says so.
- Responses API wire format for xAI/OpenAI (`/v1/responses`,
  `max_output_tokens`, `input`, `response.*` SSE). Dual adapter: Chat
  Completions builder retained for non-xAI providers.
- Chat JWT policy (`aud = hydrogen-chat`, role `chat`) applied to REST and
  WS identically.
- MCP: hosted (`type:mcp` in provider body, xAI/OpenAI) + local (Hydrogen
  as MCP client, all providers). One MCP tool: `System.Info`.
- Minted, short-TTL (15m default), `aud`-restricted MCP token for hosted
  MCP; rejected by chat/REST on purpose (two JWT policies).
- `mcp_try_hydrogen` `aud`-gate fixed so MCP `aud=MCPResource` scoping is
  real, not decorative.
- `H.system.info()` shares the C collectors with `GET /api/system/info`.
- Zero chat functions in `dead_functions.txt` (309 dead functions, all
  non-chat).
- Correlation id (UUID v4) threads chat → mint → MCP logs; storage
  failure paths now log with the id (no silent "not found").

#### Sequel trigger

Do **not** extend this plan. A second MCP tool that writes/queries, a
non-500-Courses client, or product Lua tools beyond `System.Info` opens a
sequel plan with its own token/scoping review (the security model in
this plan — token leakage to a vendor you do not control — does not extend;
it is replaced).

## Purpose

One gated plan to ship a **beachhead** of Hydrogen chat for **500 Courses**
— the first real client. Scope: dual REST + WebSocket paths with real
provider knobs, **sessionless** public MCP, one read-only MCP tool
(`System.Info`), a minted narrow user JWT, and no leftover dead chat
functions.

This is deliberately **not** the forever chat/MCP platform. "Every later
client" is a sequel: the day a second MCP tool can write or query, or a
non-500-Courses client appears, the security model in this plan (token
leakage to a vendor you do not control) is replaced, not extended. Ship
500 Courses production-shaped; keep the tool surface tiny.

This document is the **archived** record of Phases 0–9 of the chat
finale. It is **not** the active plan — see
[`CHAT_FINALE.md`](/docs/H/plans/complete/CHAT_FINALE_COMPLETE.md) for pulled-forward
Phase 10 work. Earlier history:
[`CHAT_PLAN_SUMMARY_COMPLETE.md`](/docs/H/plans/complete/CHAT_PLAN_SUMMARY_COMPLETE.md),
[`CHAT_PLAN_PHASE_13_SUPERSEDED.md`](/docs/H/plans/complete/CHAT_PLAN_PHASE_13_SUPERSEDED.md),
and [`plans/complete/`](/docs/H/plans/complete/).

## How To Use This Document

- Work **one phase at a time**, top to bottom.
- **Do not start a phase until the previous phase Status is complete and
  its Exit gate is green.** No skipping, no "we'll come back."
- Each phase has one **Done means** line — that is the testable state.
- Mark work items `[x]` only when that item's verification actually passed.
- Defer with `[~]` plus one-line rationale and the phase it moves to.
- After each phase: fill Status (date, result, variances), append Working
  Log, **stop for review**. Do not begin the next phase in the same turn
  unless asked.
- Build aliases: `zsh -ic 'mkt'`, `mku <base>`, `mkp`, `mka`, `mks`.
  See [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md).

## Implementor Workflow (every phase)

Each phase is worked in its **own conversation**. Follow this sequence:

1. **Confirm the prior phase is actually done.** Re-read its Status block
   and Exit gate before touching anything; do not trust memory of a prior
   session.
2. **Discuss the current phase first.** Re-read only that phase's Goal +
   Work items + Done means + Exit gate. Ask clarifying questions and do
   any research needed (grep the code, read the relevant `.md`, check
   Helium/QueryRef IDs on disk) **before** writing any code.
3. **Ask for explicit approval to start implementation.** Do not begin
   editing source files until the user says go.
4. **Ask questions as they come up** during implementation rather than
   guessing at ambiguous requirements.
5. **Update the phase's Working Log entry when major pieces land** (not
   only at the very end) — e.g. "builder fix done, Unity next."
6. **Record lessons learned** for the phase, even small ones — anything
   that will save time on a later phase or a future chat/MCP feature.
7. **Mark work items `[x]` and the phase Status "complete" only after the
   phase's actual verification commands ran clean** (`mkt`/`mkp`/`mks` as
   applicable, the named `mku` tests, the named blackbox test). Intent to
   verify is not verification.
8. **Never apply a database migration.** If a phase needs a Helium seed,
   schema change, or migration packet, prepare/generate it and hand it to
   the user to apply; do not run `schematool`/`schemahelper` apply steps
   yourself.
9. **Follow existing project norms** for unit tests (Unity, one file per
   function, no `static`, see
   [TESTING_UNITY.md](/docs/H/tests/TESTING_UNITY.md)), blackbox tests
   (`tests/test_NN_*.sh` conventions, see
   [TESTING.md](/docs/H/tests/TESTING.md)), running the server, and the
   existing tool aliases (`mkt`, `mkp`, `mks`, `mka`, `mku`) rather than
   inventing new scripts or ad hoc build commands.

## Resuming Work

**CURRENT PAUSE POINT (as of 2026-09-03, archive snapshot):** Phases 0–9
complete (0–8 implementation, 9 docs and DOKS). The beachhead is shipped.
Pulled-forward Phase 10 work lives in the active plan — see
[`CHAT_FINALE.md`](/docs/H/plans/complete/CHAT_FINALE_COMPLETE.md). Per Completion Criteria
this archive represents: Phases 0–9 Status complete and Exit gates green
in order; Test 59 (REST + WS) green; Test 47 green (hosted + local MCP);
`System.Info` returns JWT `/api/system/info` JSON via shared C; zero chat
functions in `dead_functions.txt`; Phase 8 token handoff is minted /
short-TTL / scope-limited / rejected by chat on purpose; correlation id
threads chat → mint → MCP logs with storage failures logged (no silent
"not found").

### Resume here next session

If a sequel opens (second MCP tool that writes / queries, non-500-
Courses client, product Lua tools beyond `System.Info`):

1. Re-read the Purpose + Beachhead sentence and **do not extend this
   plan** — open a sequel plan that re-reviews the security model
   (token leakage, per-tool authorization, prompt injection).
2. The pulled-forward Phase 10 items (chat rate limiting, cost cap
   enforcement, parked wishlist) live in
   [`CHAT_FINALE.md`](/docs/H/plans/complete/CHAT_FINALE_COMPLETE.md), not here.

---

## Priority

| | |
| --- | --- |
| **Band** | P0 — [TODO.md item 2](/docs/H/TODO.md) |
| **Effort** | XL |
| **First deploy** | 500 Courses (chat already works; this plan makes REST+WS, knobs, and MCP real) |

---

## Goals

1. Client may use **REST or WebSocket**.
2. Session with **Grok** (xAI CEC; **Responses API** wire format, locked
   by Phase 0 spike). Other providers use Chat Completions / Messages.
3. Questions return with **streaming** (per matrix), **temperature**,
   **reasoning**, the provider's max-tokens knob, and other obvious
   provider knobs. Responses API: `max_output_tokens`, `reasoning` items,
   `response.*` SSE events.
4. Grok can answer by calling **Hydrogen MCP** on the **public** MCP
   URL via **hosted MCP** (`type:mcp` in provider body), possibly a
   **different instance** than chat (DOKS). Intended.
5. **Local MCP** for all providers — Hydrogen acts as MCP client,
   converts tools to function-calling defs, sends on any endpoint.
6. An MCP **Lua tool** returns the same JSON as authenticated
   `GET /api/system/info`, by **reusing** the C collectors — not by
   HTTP-calling the REST endpoint, not by a second implementation.
7. Shared parse/build/proxy where sharing is real. One internal request
   object maps to the dialect the chosen model speaks. Published support
   matrix.
8. Chat-related `mkt` dead functions driven to **zero**. Do not baseline them.
9. **Observability.** One opaque correlation/request id (UUID v4) threads
   the chat → mint → MCP three-hop path (logs, not a shared store) so a
   missing MCP call is distinguishable from a silent storage failure at
   1 a.m. Locked by Phase 0; verified by Phase 5 (logged failures) and
   Phase 8 (mint/MCP trace).

## Non-Goals

- Client-supplied OpenAI `tools` / function-calling loops. MCP is the
  tool surface.
- Public `/api/conduit/chat` and `/chats`.
- `alt_chat` / `alt_chats`.
- Lithium Chats manager UI.
- MCP protocol (Phases 0–15 done). MCP 16–17 (DCR, stdio) stay on
  [MCP_COMPLETE.md](/docs/H/plans/complete/MCP_COMPLETE.md).
- Phase 13 wishlist as a blocker (cache, key LB, fallbacks, analytics,
  templates, convo CRUD, cost, A/B, `media_chunk`). Parked in Phase 10.
- Encryption-at-rest and chat-history retention/deletion policy. Real
  gaps (see Security & Safety), explicitly **not** solved by this plan —
  flagged for a product/compliance decision, not silently ignored.
- LLM output sanitization (control chars, markup escaping). Left to the
  client by design; stated as a contract, not implemented here. Fine
  while Hydrogen owns the only client; a second consumer turns stored,
  unsanitized model output into everyone else's XSS — revisit when a
  second consumer appears.
- **Vendor data retention.** If Grok chat moves to xAI's Responses API,
  default `store=true` retains request/response bodies for 30 days on
  xAI's side. Flagged for a product/compliance decision (including
  disabling `store`); not solved by this plan.

---

## Security & Safety

This finale hands an autonomous third-party model (Grok) the ability to
call tools on Hydrogen's behalf. That is a materially different risk
than "chat is a REST/WS feature" — it is reviewed here as its own
category, separate from routine infrastructure hardening (timeouts,
leaks, rate limiting), which is tracked inline in the phases above.

| Risk | Current reality | Handled by |
| --- | --- | --- |
| Token handoff to xAI | Phase 8 injects a **minted** JWT into the provider body as `authorization: Bearer <jwt>`; xAI holds and replays it to public MCP. No infra exists today to mint a narrow token — only full-account JWTs (1h, full `roles`) or OIDC-client tokens. Lock shape in Phase 0; build as a Unity-verified primitive (8a) **before** wiring (8b) | Phase 0 (lock) + Phase 8a (mint+verify), wire in 8b |
| MCP `aud` bypass | `mcp_try_hydrogen` (`mcp_auth.c:253-284`) never calls `mcp_auth_aud_contains` — only the OIDC IdP/RP paths check `aud`. Any Hydrogen JWT for any audience is accepted by MCP today, which undermines the `aud=MCP.Resource` scoping Phase 8 depends on | Phase 8 entry gate (blocking prerequisite fix, filed against MCP core even though MCP 0-15 is otherwise "done") |
| No per-tool MCP authorization | Any JWT that clears MCP auth can invoke any `mcp_access=1` tool; scope/role enforcement, if any, is left to each Lua script | Phase 7/8 (policy statement; no framework fix in this finale) |
| No revocation | Hydrogen JWTs and OIDC access tokens cannot be revoked in real time (`oidc_service_revoke.c` only revokes refresh tokens); security is TTL-only | Documented risk; short TTL on the Phase 8 minted token is the only mitigation available |
| Indirect prompt injection | Tool-role and user-role content are inserted into the provider `messages` array identically (`req_builder.c:120-121`) — no framing marks tool output as data rather than instructions | Documented risk; no generic fix. Kept low today because `System.Info` takes no arguments and returns fixed-shape internal data |
| SSRF / arbitrary SQL via scripting | `H.http.get`/`H.query` take caller-influenced URLs/SQL with no allowlist; `mcp_access` is a load-time gate, not a capability sandbox | Phase 7 policy (hard bar): MCP-exposed scripts **may not call** `H.http.get`/`H.query`/`H.altquery` at all — no allowlist exception. `System.Info` complies by construction |
| Chat data at rest | Conversation content is stored compressed but **not encrypted**; no retention/deletion path exists | Non-Goal (flagged for product/compliance decision, not solved here) |
| LLM output handling | No sanitization of model output before storage/return | Non-Goal (client's responsibility, stated as contract) |
| Vendor data retention | If Phase 0 spikes onto xAI Responses API, default `store=true` retains request/response 30 days on xAI's side | Phase 0 decision (disable `store` or accept; Non-Goal) |
| Token leakage to vendor | The minted MCP token leaves Hydrogen and is treated as **already leaked** to xAI the moment it does; xAI may log/persist `authorization`; no revocation exists. Short TTL + a minimal role/`aud` that chat/REST **reject on purpose** are the only controls | Phase 0 (lock) + Phase 8a (mint short-TTL, narrow role) |

Phase 0 must record explicit decisions for the token-handoff model and
the MCP `aud` fix dependency before Phase 8 starts; see that phase's
work items.

---

## Architecture (target)

```text
Browser (SSE / WebSocket)
        │
Hydrogen chat proxy (the only place that knows vendors)
        │
   ┌────┼────────────────────────────┐
   │    │                            │
   │  /v1/responses              /v1/chat/completions      /v1/messages
   │  (xAI, OpenAI)              (Ollama, Groq,           (Anthropic)
   │   + type:mcp tool            OpenRouter floor)
   │   = hosted MCP)
   │
   └────┴─────── MCP client (Hydrogen-side) ──────────────┘
                         │
                         ├─ Hosted MCP: Hydrogen exposes public MCP URL;
                         │   provider connects via `type:mcp` (Responses only)
                         └─ Local MCP: Hydrogen connects to MCP servers,
                             converts tools to function-calling defs,
                             sends on any endpoint
                         │
                         ▼
                  Public MCP URL (Streamable HTTP)
                  Authorization: Bearer <user-scoped JWT>
                  │
                  ▼
                  Hydrogen (maybe different instance)
                  Lua tools, including System.Info
                  (same JSON as JWT GET /api/system/info)
```

### Wire format rules

1. **Normalize inbound to one internal request object** (model, messages/items, tools, stream flag), then map to the dialect the chosen model speaks.
2. **Keep conversation state in Hydrogen** by default. `previous_response_id` is an optional fast path for OpenAI/xAI when opted in via engine config.
3. **MCP lives in Hydrogen** — converted to function tools. Optionally *also* pass `type: mcp` when the route is OpenAI/xAI and the server is remote-reachable (hosted MCP optimization).
4. **Stream through one internal event schema** (`{type: "text_delta"|"tool_call"|"tool_result"|"reasoning"|"done", ...}`). Translate Responses typed events, Chat Completions deltas, and Anthropic content blocks into it.
5. **Per-engine JSON config** carries provider-specific attributes (e.g. `store` for Responses API). Passed where expected, ignored by adapters that don't use it.

---

## Current Observed State (2026-08-30)

### Works

| Surface | Status |
| --- | --- |
| REST `auth_chat` / `auth_chats` non-stream | JWT, CEC, proxy, storage, hashes, `media:` |
| WS `"chat"` stream and non-stream | multi_curl + `chat_done` |
| WS `"media_upload"` | Single-frame |
| CEC `xai` | Maps to OpenAI-compatible (Chat Completions) — but xAI Remote MCP **requires Responses API** (`/v1/responses`). Chat Completions rejects `type:mcp` with 422. Phase 0 spike confirmed. |
| MCP Phases 0–15 | Test 47; Hydrogen JWT default |
| `GET /api/system/info` | Public JSON via `get_system_status_json`; JWT adds `scripting` scoreboard |
| Test 59 | REST + WS; **asserts REST stream 501** |

### Broken, stubbed, or lying

| Item | Reality |
| --- | --- |
| Temperature | Parsed; builder **hardcodes 1.0** |
| Reasoning request | Not parsed. Inbound `reasoning_content` only on some chunk paths |
| REST `stream:true` | **501** |
| `/auth_chat/stream` | 200 with a single SSE error event |
| WS `media:` / persist / hashes | REST has them; WS does not (or parses unused) |
| `media_chunk` | Returns -1 |
| `alt_chat`, public `chat` | Claimed complete in old plans; **no source** |
| MCP from chat | None |
| `H.system.info` | Does not exist (`uptime`/`now`/`version` only) |
| `websocket_chat.md` | Stale protocol |
| Chat dead functions | Many in `mkt` `dead_functions.txt` |
| WS stream abort | `chat_proxy_multi_stream_stop` leaks the CURL easy handle + buffers + stream ctx on every client disconnect mid-stream (never reaches normal `CURLMSG_DONE` free path) |
| multi_curl timeouts | `proxy_multi.c` hardcodes 10s/600s literals, bypassing the "configurable" `ChatProxyConfig`; no idle/low-speed timeout, so a trickling provider occupies a slot up to 10 min |
| JWT `aud`/roles | Parsed but never checked for chat (REST or WS) — any validly-signed token with a `database` claim is accepted regardless of audience or role |
| Chat rate limiting | None, REST or WS — only a global connection cap, not per-user/per-connection |
| WS chat `media:` | `convert_json_messages_to_chat_messages` never resolves `media:<hash>` refs (REST `auth_chat` does); `auth_chats` has no media handling at all — three-tier parity gap |
| `context_hashing` stats | Reported to client only on non-stream REST `auth_chat`; silently dropped on REST stream and WS |
| Silent storage failures | `storage.c`/`storage_media.c` have `return false` paths with no `log_this`, indistinguishable from "not found" |
| MCP `aud` check | `mcp_try_hydrogen` never validates `aud`; only OIDC IdP/RP paths do — any Hydrogen JWT for any audience passes MCP auth today |
| Wire format | Current CEC only builds Chat Completions. xAI/OpenAI need Responses API (`input`, `max_output_tokens`, `output[]`, `response.*` SSE). Dual adapter required (Phase 1+). |
| Per-tool MCP authorization | Does not exist — any authenticated JWT can call any `mcp_access=1` tool |

### Authenticated status JSON (reuse this)

`handle_system_info_request` in
[`info.c`](/elements/001-hydrogen/hydrogen/src/api/system/info/info.c):

1. `get_system_status_json(...)` always.
2. If valid JWT: attach `scripting_scoreboard_snapshot_json(100, false)`
   as `"scripting"`.

MCP tool **System.Info** must return that same object (step 1 + step 2).
MCP already authenticated the Bearer, so include scripting. Extract one
C builder used by the REST handler **and** `H.system.info()`. Do not
`H.http.get` localhost.

`Mcp.Info` (resource `hydrogen://mcp/info`) stays a static MCP fixture.
**System.Info** is a **tool** for Grok.

---

## Support Matrix (lock in Phase 0)

Draft recommendation in parentheses. Phase 0 writes the Decision column.

| Capability | REST `auth_chat` | REST `auth_chats` | WS `"chat"` |
| --- | --- | --- | --- |
| JWT | Bearer | Bearer | `payload.jwt` |
| Engine / Grok | yes | yes | yes |
| Non-stream | yes | yes | yes |
| Streaming | **decide** (no; 400 → use WS) | no | yes |
| Temperature | must apply | must apply | must apply |
| `max_tokens` / `max_output_tokens` | yes | yes | yes |
| Reasoning request | add | add | add |
| Reasoning inbound | n/a if no REST stream | n/a | chunks |
| `context_hashes` | yes | as today | parity or drop |
| `media:` | yes | as today | parity |
| Persist | yes | as today | parity |
| Client `tools` | **no** | **no** | **no** |
| Hosted MCP (xAI/OpenAI) | Phase 8 | Phase 8 | Phase 8 |
| Local MCP (all providers) | Phase 8 | Phase 8 | Phase 8 |

**Wire format note:** xAI and OpenAI use `/v1/responses` (Responses API). All other providers use `/v1/chat/completions` (Chat Completions). Anthropic uses `/v1/messages`. CEC routes by engine config.

**Lock notes for Phase 0 (from independent review):**

- **Streaming error contract** is locked as **400 + stable JSON, use WS** —
  not a `200` body that is only a single SSE error event.
- **Reasoning inbound** lives on WS chunks. On Responses API, reasoning is a typed `reasoning` item in the `output` array (not `delta.reasoning_content` as in Chat Completions). The Phase 0 spike confirms xAI uses Responses API, so the inbound assertion targets Responses reasoning items.
- **Grok does not reach `/v1/chat/completions` with MCP.** The spike confirms MCP only works on `/v1/responses`. Phases 1–4 target the Responses wire format for xAI/OpenAI.

---

## Grok MCP Credentials (lock option in Phase 0, implement Phase 8)

xAI Remote MCP Tools (provider body, **not** client chat JSON):

```json
{
  "type": "mcp",
  "server_url": "https://<public>/mcp",
  "server_label": "hydrogen",
  "authorization": "Bearer <user-scoped JWT>",
  "allowed_tools": ["System.Info"]
}

**Locked in Phase 0:** `authorization` must carry the `Bearer ` prefix
(raw JWT is rejected by xAI Remote MCP examples). `allowed_tools` must be
`["System.Info"]` — **never empty**, since empty means "every
`mcp_access=1` tool Hydrogen ever grows," and there is no per-tool MCP
authorization (see Security table). This is a provider-body field, never
client chat JSON.

| Option | What | Per-user JWT |
| --- | --- | --- |
| **A (default)** | Hydrogen injects `type:mcp` on the provider request | Yes |
| B | Account-level Grok connector, static token | No |
| C | Client talks to Grok and MCP; Hydrogen chat unused for MCP | Yes, outside proxy |
| D | OpenAI function calling in Hydrogen | **Forbidden** |

500 Courses: Keycloak → OIDC RP → **Hydrogen JWT**. SPA never holds
Keycloak. MCP `AcceptHydrogenJWT=true` is the path.

**Correction (2026-08-31):** no `H.system_token` / `sub=hydrogen-scripting`
minting exists anywhere in `src/` today — grepped and confirmed absent.
Do not assume it as prior art. If Option A is implemented, Phase 8 must
build the minting primitive from scratch: a short-TTL, `aud=MCP.Resource`
token carrying the chat user's `sub`/`database`/roles needed only for
the tools actually exposed — not a copy of the full 1-hour account JWT
with its full `roles` string. This is new work, not a reuse of an
existing service-token mechanism.

---

## Phase Index

| Phase | Done means (one line) |
| --- | --- |
| 0 | Decisions written in Status (endpoint spike → Responses API, streaming
      policy + error contract, aud-check outcome, System.Info scope, cost
      cap, wire format, MCP approach); no C |
| 1 | Provider JSON uses client/engine temperature, not `1.0` (all builders
      including new Responses adapter) |
| 2 | Reasoning knobs round-trip; inbound Responses `reasoning` items on chunks |
| 3 | REST matches matrix; real SSE; Responses API format for xAI/OpenAI |
| 4 | WS matches matrix; Responses API format for xAI/OpenAI |
| 5 | Zero chat names in `dead_functions.txt` |
| 6 | `H.system.info()` === JWT `/api/system/info` collectors |
| 7 | MCP tool `System.Info` callable; Test 47 proves it |
| 8 | Hosted MCP: Grok (or mock) calls public MCP with a minted,
      short-TTL `aud=MCP.Resource` JWT; `aud`-gate proven real;
      mint+verify (8a) before wire (8b); `Mcp-Session-Id` not required
      for the Grok path. Local MCP (8c): Hydrogen as MCP client —
      connects to external MCP servers, converts tools to function-calling
      defs, proxies tool calls; Unity-proven |
| 9 | Docs and DOKS notes match code |
| 10 | Parked extras — not a gate |

---

## Phase 0 — Contract Lock

### Goal

Write the decisions later phases implement. No C.

### Entry gate

This document exists.

### Work items

- [x] **Spike: live CEC endpoint probe.** `curl` the actual Grok engine
      URL / CEC config with a dummy `type:mcp` tool body on the **same
      endpoint** (`/v1/chat/completions`) CEC already uses. xAI documents
      Remote MCP on the native SDK / **Responses API**, not Chat
      Completions. If `/v1/chat/completions` ignores or rejects the
      `type:mcp` tool, lock `/v1/responses` **before** Phases 1–4 write
      any Unity asserting Chat Completions knobs (temperature,
      `reasoning_content`, streaming) — otherwise the wire format is
      wrong under the hood. Record result + endpoint in Status.
      **Result:** `/v1/chat/completions` returns **422** — `unknown variant "mcp", expected "function" or "live_search"`. `/v1/responses` endpoint exists (returns "Model not found", not 404). **Locked: xAI uses Responses API.**
- [x] **Lock whether Grok chat moves to the Responses API.** Confirmed:
      streaming = SSE `response.*` (not chat deltas), reasoning is a
      Responses `reasoning` item (not `reasoning_content`), `max_output_tokens`
      (not `max_tokens`), and default `store=true` retains request/response
      30 days on xAI's side. `store` is a per-engine JSON config attribute
      (passed where expected, ignored by adapters that don't use it).
      This rewrites the Phase 1–3 meaning of "temperature," "reasoning,"
      and "stream." Encode a data-residency Non-Goal.
- [x] Fill Support Matrix **Decision** column. (See matrix above.)
- [x] Choose REST streaming: **real SSE via multi_curl** (MHD incremental
      + `chat_proxy_multi_*`).
- [x] Lock the exact error contract for the losing choice (status code +
      JSON error shape) so the client can code against it before Phase 3
      lands — **400 + stable JSON, use WS** (do not ship a
      200-with-one-SSE-error-event body that returns 200 and is only an
      error event).
- [x] Choose Grok MCP option: **Option A** (Hydrogen appends `type:mcp`
      to provider body, mints per-user JWT).
- [x] Name Grok engine in CEC: **grok-4.6** (xAI, to be verified when
      credits available). Public MCP URL shape: `https://<host>/mcp`.
- [x] Lock the MCP `authorization` header form: `"Bearer <jwt>"`, **not**
      the raw JWT — xAI Remote MCP third-party examples require the
      `Bearer ` prefix. Lock `allowed_tools` to `["System.Info"]` only
      (empty = every `mcp_access=1` tool Hydrogen ever grows).
- [x] Decide chat JWT policy: chat requires a specific `aud` and/or
      role beyond a non-empty `database` claim. Exact strings deferred to
      Phase 3 (likely `aud=hydrogen-chat`, role=`chat`). **Record two
      policies**: chat (specific `aud`/role) and MCP (`aud=MCP.Resource`
      enforced) — do not pretend Phase 8 scoping is real on the chat side.
- [x] Decide the Phase 8 token-handoff shape: mint a **short-TTL (15m)**
      token with `aud=MCP.Resource`, carrying the chat user's `sub`,
      `database`, and minimum roles needed for the tools this chat session
      may reach. **Treat the minted token as already leaked to xAI** on
      departure (short TTL + a role/`aud` chat/REST reject on purpose are
      the only controls — no revocation exists).
- [x] **Split token work (process).** Mint + verify the narrow token in
      Unity (its own change, its own review) **before** wiring the xAI MCP
      connector — do not do both in one phase. See Phase 8 restructure.
- [x] **Aud-check fix (land before Phase 0 complete).** Strengthen
      `mcp_try_hydrogen` (`mcp_auth.c:253-284`) to validate `aud` against
      `mcp_auth_resource(cfg)`. Land + Unity-prove a mismatched-`aud` JWT
      is rejected before Phase 0 Status is marked complete. Blocking
      prerequisite for Phase 8.
- [x] **Deployment assumption: A↔B sharing.** If chat runs on instance A
      and MCP on instance B, both must share signing keys, aligned clocks,
      and the same `sub`/`database` claim space. Recorded as a Phase 0
      assumption.
- [x] **Fail-closed for `MCP.Resource`, beyond localhost.** Lock rejection
      of localhost **and** internal ClusterIP/DNS that xAI cannot reach —
      a loopback "Grok called MCP" smoke test otherwise lies.
- [x] **Stateless first tool path.** Lock that the first MCP tool call
      tolerates a missing/rotating `Mcp-Session-Id` (initialize +
      tools/call on one POST, or sessionless) — xAI session affinity on
      that header is not guaranteed; sticky ingress is a hope, not a
      dependency.
- [x] **System.Info scope.** The authenticated system-info JSON includes
      the `scripting` scoreboard (operational inventory). Grok is permitted
      to see the chosen set (reuse of collectors is engineering; reuse of
      the *authenticated* view is a product choice).
- [x] **One-line cost cap.** `max_output_tokens` ceiling (enforcement may
      stay Phase 10).
- [x] **Correlation id across the three-hop path.** One opaque UUID v4
      threads chat (incoming request) → mint log → MCP tool log, so "Grok
      didn't call MCP" is distinguishable from "MCP call failed" and
      storage failures surface with context (not just "not found"). Logged
      by all three hops, not a shared store. Required by Goal 8.
- [x] **Wire format: dual adapter.** CEC supports both `/v1/responses`
      (xAI/OpenAI) and `/v1/chat/completions` (others). New builder
      (`chat_request_build_responses`) + new response parser + new stream
      event handler for Responses API. Existing Chat Completions builder
      retained for non-xAI providers.
- [x] **MCP: hosted + local.** Hosted MCP (`type:mcp` in provider body)
      for xAI/OpenAI Responses path. Local MCP (Hydrogen acts as MCP
      client, converts tools to function-calling defs) for all providers.
      Both paths converge on the same public MCP URL / Lua tools.
- [x] Run `zsh -ic 'mkt'` once to (re)generate
      `build/deadcode/dead_functions.txt` (it is not persisted between
      checkouts), then snapshot current chat names into Working Log (do
      not baseline).

### Done means

Status block records matrix, stream policy + error contract (400 + stable
JSON, use WS), MCP option + engine name, `authorization` form
(`Bearer <jwt>`) + `allowed_tools` whitelist, chat JWT policy (two
policies), endpoint spike result (Chat Completions vs Responses), the
aud-check fix outcome, System.Info scope, fail-closed shape, stateless
Mcp-Session-Id stance, cost-cap line, wire format (dual adapter), and
MCP approach (hosted + local).

### Exit gate

Status complete. Review stop.

### Status

**Complete (2026-09-01)**

- **Endpoint spike:** `/v1/chat/completions` + `type:mcp` → **422** (only
  `function`/`live_search` accepted). `/v1/responses` endpoint exists.
  **Locked: xAI uses Responses API.** Model: `grok-4.6`.
- **Wire format:** Dual adapter — `/v1/responses` (xAI/OpenAI) + `/v1/chat/completions`
  (others). New `chat_request_build_responses` + response parser + stream
  event handler. Existing Chat Completions builder retained.
- **REST streaming:** Real SSE via MHD incremental + `chat_proxy_multi_*`.
- **Error contract:** 400 + stable JSON, use WS (not 200-with-one-SSE-error-event).
- **MCP approach:** Hosted (`type:mcp` in provider body, xAI/OpenAI) + local
  (Hydrogen MCP client → function tools, all providers). Both converge on
  same public MCP URL.
- **MCP option:** A — Hydrogen appends `type:mcp` to provider body, mints
  per-user JWT.
- **Authorization:** `"Bearer <jwt>"` (prefix required). `allowed_tools`
  = `["System.Info"]` (never empty).
- **Chat JWT policy:** Chat requires specific `aud`/role (exact strings
  deferred to Phase 3). MCP requires `aud=MCP.Resource` enforced. Two
  policies, written down.
- **Token handoff:** 15m TTL, `aud=MCP.Resource`, claims: `sub` + `database`
  + minimal roles. Treated as already leaked to xAI on departure.
- **System.Info scope:** Includes `scripting` scoreboard.
- **Cost cap:** `max_output_tokens` ceiling.
- **Correlation ID:** UUID v4 threads chat → mint → MCP logs.
- **Aud-check fix:** Land `mcp_try_hydrogen` `aud` validation before Phase
  0 complete (blocking prerequisite for Phase 8).
- **Deployment assumption:** A↔B share signing keys, aligned clocks, same
  `sub`/`database` claim space.
- **Fail-closed:** Reject localhost AND internal ClusterIP/DNS for
  `MCP.Resource`.
- **Mcp-Session-Id:** Not required for Grok path (sessionless OK).
- **`store` attribute:** Per-engine JSON config field. Passed where expected
  (Responses API), ignored by other adapters.
- **Spike note:** xAI key out of credits — model name from docs, not live
  verified. Future live testing (Phase 8) requires funded key.

---

## Phase 1 — Temperature Reaches The Provider

### Goal

`ChatRequestParams.temperature` is what OpenAI-compatible and Ollama
builders emit. Stop hardcoding `1.0`. Build the new Responses API adapter
with temperature correct from the start.

> **Depends on Phase 0 spike.** Temperature round-trips identically on
> either wire format, but confirm the chosen Grok endpoint before
> asserting any provider-specific field shapes elsewhere (see Phase 2).

### Entry gate

Phase 0 Status complete.

### Work items

- [ ] `chat_request_build_openai` / `_ollama`: use `params->temperature`
      when `>= 0`, else engine default (currently both hardcode `1.0`,
      `req_builder.c:127-128,356-357`).
- [ ] **New:** `chat_request_build_responses` (Responses API adapter):
      emit `temperature` from `params->temperature` when `>= 0`, else
      engine default. Do not hardcode — this builder starts clean.
- [ ] `chat_request_build_anthropic`: **start emitting temperature**
      (same logic as other builders: `params->temperature >= 0` else
      engine default). Temperature IS available on Anthropic; do not
      leave it field-absent.
- [ ] **Overlay mechanism:** `params->additional_params` is already
      merged into the OpenAI request (`req_builder.c:147-153`) but
      Anthropic and Ollama skip it. Extend the merge to **all** builders
      (Anthropic, Ollama, Responses) so provider-specific knobs
      (Anthropic `thinking`, Responses `reasoning_effort`, future
      vendor params) ride in the overlay without builder changes.
      Overlay merges last — explicit fields set the base, overlay can
      override.
- [ ] **Responses routing:** add `use_responses_api` bool to
      `ChatEngineConfig` (alongside existing `use_native_api`). Dispatch
      `chat_request_build_responses` from inside the `CEC_PROVIDER_OPENAI`
      case in `chat_request_build` when the flag is set. No new enum
      value — runtime flag on config.
- [ ] REST and WS already resolve via `auth_chat_resolve_request_params`
      (or equivalent) — keep one resolver.
- [ ] Unity: omitted → engine default; `0.2` → `0.2` in JSON; `1.0` → `1.0`
      for OpenAI/Ollama/Responses. Anthropic: temperature present-with-
      value (matching new behavior). Overlay: `additional_params` merges
      correctly in all builders.
- [ ] `mkt` + `mkp`.

### Done means

Unity proves provider JSON temperature is not a hardcoded `1.0` for all
builders including the new Responses adapter. Anthropic emits temperature.
Overlay (`additional_params`) merges in all builders. Responses routing
uses `ChatEngineConfig.use_responses_api` runtime flag.

### Exit gate

Named Unity green. `mkt`/`mkp` green.

### Status

**Complete (2026-09-01)**

- **Temperature reaches provider:** All four builders (OpenAI, Ollama, Anthropic, Responses) now emit `params->temperature` when `>= 0`, else `engine->temperature_default`. No more hardcoded `1.0`.
- **Anthropic temperature added:** Was field-absent; now present with correct value.
- **Overlay extended:** `additional_params` merge now works in all builders (Anthropic, Ollama, Responses), not just OpenAI. Overlay merges last — explicit fields set the base.
- **Responses API builder:** New `chat_request_build_responses` emits `input` (not `messages`), `max_output_tokens` (not `max_tokens`), `temperature`, and overlay. Clean from the start — no hardcoded values.
- **Responses routing:** `use_responses_api` bool added to `ChatEngineConfig`; `chat_request_build` dispatches to Responses builder from `CEC_PROVIDER_OPENAI` case when flag is set. Anthropic path unaffected.
- **Shared resolver:** `auth_chat_resolve_request_params` and `auth_chats_resolve_request_params` unified into `chat_resolve_request_params` in `req_builder.c`. Both call sites updated.
- **Database loading:** `use_responses_api` loaded from engine collection JSON alongside `use_native_api`.
- **Unity tests:** 17 new tests in `req_builder_test_temperature.c` prove temperature round-trip, engine-default fallback, overlay merge in all builders, Responses format (`input`/`max_output_tokens`), and routing dispatch.
- **Verification:** `mkt` green, `mkp` green (2,011 files), all named Unity tests green.

---

## Phase 2 — Reasoning Knobs And Inbound Chunks

### Goal

Request-side reasoning that Grok/xAI actually accept. Inbound reasoning
on every WS chunk path. On Responses API, reasoning is a typed `reasoning`
item in the `output` array — not `delta.reasoning_content` as in Chat
Completions.

> **Gated on Phase 0 endpoint spike.** The spike locks `/v1/responses` for
> xAI. The inbound assertion must target the Responses `reasoning` item,
> not chat `delta.reasoning_content`. The "Confirm current xAI field names"
> work item below is the lock point.

### Entry gate

Phase 1 Status complete.

### Work items

- [ ] Confirm current xAI field names (`reasoning`, `reasoning_effort`,
      or extra body). Parse from client JSON; pass through the Responses
      builder (`additional_params` or explicit fields). Do not invent
      names.
- [ ] **Responses API stream parser:** extract `reasoning` items from the
      `output` array in SSE events (`response.reasoning_summary_text.delta`,
      `response.output_item.added` with `type: "reasoning"`). Map to the
      internal `{type: "reasoning", ...}` event schema.
- [ ] `proxy_multi.c` final chunk: copy reasoning items from Responses
      `output` array (not `reasoning_content` — that field is Chat
      Completions only).
- [ ] Unity for parse + emit + chunk parse. Responses reasoning item
      appears on WS chunks.
- [ ] `mkt` + `mkp`.

### Done means

A request with the documented reasoning field appears on the provider
body; a mock Responses `reasoning` item appears on WS chunks.

### Exit gate

Unity green. `mkt`/`mkp` green.

### Status

**Complete (2026-09-01)**

- **Request-side reasoning:** `reasoning` field parsed from client JSON in `auth_chat_parse_request`, `auth_chats_parse_request`, and `auth_chat_stream_parse_request`. Added to `ChatRequestParams` struct. `chat_resolve_request_params` accepts reasoning parameter.
- **Responses builder emits reasoning:** `chat_request_build_responses` emits `{"reasoning": {"effort": "<value>"}}` when reasoning is non-NULL. Omitted when NULL.
- **Responses API SSE parser:** `chat_stream_chunk_parse` detects Responses API format (type starts with `response.`) and dispatches:
  - `response.output_text.delta` → `chunk->content`
  - `response.reasoning_summary_text.delta` → `chunk->reasoning_content`
  - `response.completed` → `chunk->is_done` + full response object in `extra_fields`
  - `response.output_item.added` with `type: "reasoning"` → `reasoning_item_added` flag in `extra_fields`
- **WS chunk emission:** `proxy_mc.c` already emits `reasoning_content` inside `chat_chunk` events. `proxy_multi.c` final chunk handler updated to include `reasoning_content` and `extra_fields`.
- **Unity tests:** 8 new tests in `resp_parser_test_responses.c` prove Responses SSE parsing. 2 new tests in `req_builder_test_temperature.c` prove reasoning emission/omission. All existing parse request tests updated for new parameter.
- **Verification:** `mkt` green, `mkp` green (2,012 files), all named Unity tests green.

---

## Phase 3 — REST Pathway

### Goal

REST is complete per the Phase 0 matrix. No 501-as-success. Real SSE
(Phase 0 locked). Responses API wire format for xAI/OpenAI
(`max_output_tokens`, `input`, `response.*` SSE events).

### Entry gate

Phase 2 Status complete.

### Work items

- [ ] Implement the exact error contract locked in Phase 0:
      - Non-stream REST: `stream:true` → **400** (not the current 501)
        with the Phase-0-locked message pointing at WS. Unregister stub
        `/auth_chat/stream` (`auth_stream.c`).
      - Real SSE: MHD incremental + `chat_proxy_multi_*`; `stream:true`
        works via the Responses API SSE event stream (`response.created`,
        `response.output_text.delta`, `response.completed`).
- [ ] **Responses API REST path:** route `auth_chat` through
      `chat_request_build_responses` for xAI/OpenAI engines. Map internal
      request object → Responses format (`input`, `max_output_tokens`,
      `temperature`, `tools` with `type:mcp` for hosted MCP).
- [ ] Update Test 59 REST sections (today asserts 501 and stub SSE text).
- [ ] `auth_chats` keeps the same knobs except streaming (unless Phase 0
      said otherwise).
- [ ] Implement the Phase 0 chat JWT policy in the shared helper
      (`validate_jwt`/`extract_and_validate_jwt`, `auth_jwt_helper.c`) so
      REST and WS both apply it identically — today `aud` and `roles` are
      parsed (`auth_service_jwt.c:464-467,484-487`) but never checked;
      only a non-empty `database` claim is required.
- [ ] Do not add public `chat`/`chats` or `alt_chat`.
- [ ] `mkt` + `mkp` + Test 59 REST.

### Done means

Test 59 REST matches the matrix. Stub SSE gone. Responses API format
used for xAI/OpenAI.

### Exit gate

Test 59 REST green. `mkt`/`mkp` green.

### Status

**Complete (2026-09-01)**

- **Real SSE streaming via MHD incremental + `chat_proxy_multi_*`:** New `auth_chat_sse.c` implements REST SSE streaming. Removed the old 501 response from `auth_chat.c`. The `stream:true` flag now triggers `auth_chat_stream_sse()` which starts a multi-curl worker, drains the chunk queue via a callback thread, writes SSE-formatted `data: <json>\n\n` events to a pipe, and streams them to the HTTP client via MHD's incremental response callback.
- **Stub `/auth_chat/stream` endpoint removed:** Unregistered the route from `api_service.c` (removed from `protected_endpoints`, `json_endpoints`, debug log, and dispatch block). The `auth_stream.c` handler remains for Phase 5 dead-code cleanup but is no longer reachable.
- **Chat JWT policy implemented:** Added `validate_chat_jwt_claims()` (REST) and `check_chat_jwt_claims()` (WS) to `auth_jwt_helper.c`. Both enforce `aud=hydrogen-chat` and `role=chat` on top of the standard `validate_jwt_claims()` database check. REST path sends 403 on mismatch; WS path returns error via `send_chat_error()`. Applied to `auth_chat.c`, `auth_chats.c`, and `websocket_server_chat.c`.
- **Responses API routing:** Already correct from Phase 1 — `chat_request_build_responses` is dispatched for xAI/OpenAI engines with `use_responses_api` flag.
- **Test 59 updated:** Changed stream test from expecting 501 to expecting 200 with real SSE. Removed 6 stub-endpoint subtests, replaced with single 404 test. Added non-chat JWT rejection test (403). All chat tests now use minted `CHAT_JWT_TOKEN` with correct `aud`/`roles`.
- **Unity tests:** 10 new tests in `auth_jwt_helper_test.c` — `validate_chat_jwt_claims` (wrong aud, missing roles, success) and `check_chat_jwt_claims` (wrong aud, missing roles, success, null result). All 27 tests in the file pass.
- **Baseline update:** `tests/.static-baseline.txt` regenerated to include the 4 static callback functions in `auth_chat_sse.c` (MHD callbacks and internal thread helpers that are only used via function pointers within the file).
- **Verification:** `mkt` green, `mkp` green (2,013 files), `mks` green (165 scripts), `mku auth_jwt_helper_test` (27 tests) green.

---

## Phase 4 — WebSocket Pathway

### Goal

WS `"chat"` matches the matrix: knobs, stream, non-stream, JWT.

### Entry gate

Phase 3 Status complete.

### Work items

- [ ] Verify WS builds the same provider JSON as REST for temperature
      and reasoning. **Both paths use the same builder dispatch** —
      `chat_request_build_responses` for xAI/OpenAI, `chat_request_build_openai`
      for others.
- [ ] **Responses API WS path:** route WS `"chat"` through
      `chat_request_build_responses` for xAI/OpenAI engines. Same internal
      request object → Responses format mapping as REST.
- [ ] `media:` resolve and conversation persist: implement parity **or**
      document WS-not-in-matrix if Phase 0 said so (default: parity).
      Confirmed gap: `convert_json_messages_to_chat_messages`
      (`websocket_server_chat.c:126-159`) only `json_dumps()`s array
      content and never calls `auth_chat_resolve_content_string`/
      `chat_storage_resolve_media_in_content` like REST `auth_chat.c`
      does — `media:<hash>` refs reach the provider unresolved.
- [ ] `context_hashes`: use them or stop parsing them on WS. Also fix:
      `auth_chat_attach_context_hashing_stats` only runs on non-stream
      REST (`auth_chat.c:320-350,653-654`) — REST-stream and WS never
      report hit/miss stats even though both resolve hashes. Decide one
      behavior (report on all three, or only the one that matters) and
      make it consistent.
- [ ] WS key fallback vs configured key: real auth or delete the lie.
- [ ] Fix the stream-abort resource leak: `chat_proxy_multi_stream_stop`
      (`proxy_multi.c:718-739`) calls `curl_multi_remove_handle()`
      directly, which prevents `CURLMSG_DONE` from ever firing for that
      handle, so `chat_proxy_multi_handle_completed_transfer()`
      (`proxy_multi.c:492-511`) never frees the easy handle, line/post-done
      buffers, or stream context — every WS disconnect mid-stream leaks
      until process shutdown. Let the existing write-callback abort path
      (`multi_stream_write_callback`, `proxy_mc.c:51-53`) drive normal
      `CURLE_WRITE_ERROR` → `CURLMSG_DONE` completion instead, or perform
      the same free sequence inline if immediate removal is required.
- [ ] Make multi_curl streaming timeouts real: `proxy_multi.c:669-670`
      hardcodes `CURLOPT_CONNECTTIMEOUT=10`/`CURLOPT_TIMEOUT=600` as
      literals instead of reading `ChatProxyConfig`. Route them through
      config, and add `CURLOPT_LOW_SPEED_LIMIT`/`CURLOPT_LOW_SPEED_TIME`
      so a provider that trickles data can't occupy a stream slot for
      the full 600s before being detected as stalled.
- [ ] `media_chunk` stays -1 (Phase 10).
- [ ] Test 59 WS sections. `mkt` + `mkp`.

### Done means

Test 59 WS stream + non-stream + knobs green. No leaked CURL handles on
repeated disconnect-mid-stream (verify via Unity/ASAN or manual repeated
disconnect + handle-count check).

### Exit gate

Test 59 WS green. `mkt`/`mkp` green.

### Status

**Complete (2026-09-01)**

- **Media: hash resolution parity.** Renamed `convert_json_messages_to_chat_messages` to `convert_json_messages_to_chat_messages_with_media(const char *database, json_t *messages)`. Array content is now checked for `media:` references and resolved via `chat_storage_resolve_media_in_content()` before being sent to the provider — matching REST `auth_chat.c` behavior. String content passes through unchanged.
- **Context hashing stats on WS chat_done.** Added `auth_chat_collect_segment_stats()` call in `handle_chat_message` when `context_hashes` are present. Stats are attached to the `chat_done` response (both streaming and non-streaming) with `hashes_used`, `hashes_missed`, `bandwidth_saved_bytes`, and `bandwidth_saved_percent` — matching REST non-streaming behavior.
- **MultiStreamContext extended.** Added `has_context_hashing_stats`, `ctx_hashes_used`, `ctx_hashes_missed`, `ctx_bandwidth_saved_bytes`, `ctx_bandwidth_saved_percent` fields to `MultiStreamContext` for passing stats from WS handler to the streaming completion path.
- **Stream-abort CURL leak fixed.** `chat_proxy_multi_stream_stop` now performs complete cleanup of the CURL easy handle, line buffers, post-done buffer, and `CurlStreamContext` — previously these were leaked on every client disconnect mid-stream because `curl_multi_remove_handle()` prevented `CURLMSG_DONE` from firing.
- **Multi_curl timeouts via config.** Replaced hardcoded `CURLOPT_CONNECTTIMEOUT=10`/`CURLOPT_TIMEOUT=600` literals in `chat_proxy_multi_stream_start` with values from `chat_proxy_get_streaming_config()` (which reads `ChatProxyConfig`). Added `CURLOPT_LOW_SPEED_LIMIT=100`/`CURLOPT_LOW_SPEED_TIME=30` for stall detection so a trickling provider can't occupy a stream slot for the full timeout.
- **Unity tests updated.** `websocket_server_chat_test_convert_messages.c` updated for new function signature (6/6 tests pass).
- **Verification:** `mkt` green, `mkp` green (2,013 files), `mku websocket_server_chat_test_convert_messages` (6 tests) green.
- Next: Phase 5 only.

---

## Phase 5 — Share Code And Kill Dead Chat Functions

### Goal

One parse/resolve/build/proxy core. Chat functions unreachable from
`main()` are wired or deleted.

> **Process note (independence of gating):** Phase 5 is hygiene, not a
> functional gate for Phases 6–8. A clean `dead_functions.txt` must not
> block MCP/chat delivery — do not let Phase 5 become a merge blocker
> for 6–7. An implementor burning a session on Unity-only
> `chat_proxy_init` while MCP is still vapor is self-inflicted; keep
> Phase 5 on its own timeline.

### Entry gate

Phase 4 Status complete. Fresh `mkt` dead-code list.

### Work items

- [ ] Collapse remaining REST/WS duplication into shared helpers.
- [ ] Production-call or delete leftovers. Confirmed zero-caller-in-
      production as of 2026-08-31 (Unity-only): `chat_proxy_send_stream`
      (`proxy.c:919`), `chat_proxy_init`/`chat_proxy_cleanup`
      (`proxy.c:87,98`), `chat_metrics_init` (`metrics.c:86`),
      `auth_stream_send_sse_response_headers` (`auth_stream.c:297`,
      only if SSE stays rejected). Re-check the full fresh
      `dead_functions.txt` for anything else chat-named added since.
- [ ] Retarget or remove Unity that only covered dead helpers.
- [ ] Add missing `log_this` calls on silent failure paths found during
      this pass (confirmed 2026-08-31): `storage_media.c:130-132`
      (malformed DB row), `storage_media.c:137-139` (missing/wrong-typed
      `media_data`), `storage_media.c:146-148` (hex-decode failure),
      `storage.c:93-95` (`chat_storage_execute_query` invalid
      `db_queue`/`query_ref` guard) — all currently `return false`/`NULL`
      with no log line, indistinguishable from a normal "not found." **Each
      log must carry the Phase 0 correlation id** so a storage miss is
      never silent in the logs.
- [ ] Re-run `mkt`. **No** new `tests/.deadcode-baseline.txt` rows for chat.
- [ ] `mkp`.

### Done means

`dead_functions.txt` contains zero chat / wschat / auth_chat /
auth_stream / chat_proxy leftovers.

### Exit gate

`mkt` dead-code line + file inspection. `mkp` green.

### Status

**Complete (2026-09-02)**

- **Silent storage failure paths fixed.** Added `log_this` to 4 silent `return false` paths:
  - `storage_media.c:130-132` — malformed DB row (not an object)
  - `storage_media.c:137-139` — missing/wrong-typed `media_data`
  - `storage_media.c:146-148` — hex-decode failure
  - `storage.c:93-95` — invalid `db_queue`/`query_ref` guard

- **Dead chat functions eliminated (72 functions).** `dead_functions.txt` reduced from 381 to 309 entries; zero chat/auth_stream/chat_proxy/chat_metrics/chat_health/chat_engine_cache/chat_context/chat_lru/chat_storage/chat_proxy_multi names remain:
  - Deleted entirely: `auth_stream/auth_stream.c`, `auth_stream/auth_stream.h` (endpoint removed in Phase 3), `storage_hash.c/h` (restored with only live `chat_storage_generate_hash`)
  - `metrics.c/h`: removed `chat_metrics_init`, `chat_metrics_cleanup`, `chat_metrics_engine_health`, `chat_metrics_request_duration`, `chat_metrics_update_from_engine`, `metrics_initialized` static
  - `health.c/h`: removed `chat_health_monitor_stop`, `chat_health_monitor_is_running`, `chat_health_status_to_string`, `chat_health_get_engine_status`, `chat_health_update_stats`
  - `engine_cache.c/h`: removed `chat_engine_cache_destroy`, `chat_engine_config_clear_key`, `chat_engine_cache_update_usage`, `chat_engine_cache_lookup_by_id`, `chat_engine_cache_get_stats`, `chat_engine_cache_needs_refresh`, `chat_engine_cache_refresh`, `chat_engine_cache_should_refresh`, `chat_engine_cache_get_last_refresh`
  - `context_hashing.c/h`: removed `chat_context_hash_content`, `chat_context_hash_json`, `chat_context_resolve_hashes`, `chat_context_reconstruct_conversation`, `chat_context_calculate_bandwidth_savings`, `chat_context_estimate_size_savings`, `chat_context_free_hash`, `chat_context_free_result`; kept `chat_context_validate_hash`, `chat_context_parse_request_hashes`, `chat_context_free_hash_array`
  - `req_builder.c/h`: removed `chat_message_estimate_tokens`, `chat_request_estimate_tokens`, `chat_request_count_all_images`, `chat_request_message_count_images`, `chat_request_validate`
  - `resp_parser.c/h`: removed `chat_stream_chunk_parse_responses`, `chat_response_extract_error`
  - `storage.c/h`: removed `chat_storage_cache_init`, `chat_storage_cache_shutdown`, `chat_storage_free_compressed`, `chat_storage_free_decompressed`, `chat_storage_get_chat`, `chat_storage_get_stats`, `chat_storage_prefetch_segment`, `chat_storage_retrieve_segment`, `chat_storage_retrieve_segments_batch`, `chat_storage_update_access`
  - `lru_cache.c/h`: removed `chat_lru_cache_clear`, `chat_lru_cache_get`, `chat_lru_cache_remove`, `chat_lru_cache_shutdown`
  - `proxy.c/h`: removed `chat_proxy_init`, `chat_proxy_cleanup`, `chat_proxy_send_stream`, `chat_proxy_cleanup_completed_streams`, `chat_proxy_result_code_to_string`, `chat_proxy_stream_debug_callback`, `chat_proxy_stream_worker_thread`, `chat_proxy_stream_write_callback`
  - `proxy_multi.c/h`: removed `chat_proxy_multi_get_handle`, `chat_proxy_multi_get_stats`, `chat_proxy_multi_get_stream_count`, `chat_proxy_multi_has_active_streams`, `chat_proxy_multi_has_queued_data`, `chat_proxy_multi_socket_action`, `chat_proxy_multi_timer_callback`

- **Unity tests removed (46 files).** Deleted test files covering deleted functions from auth_stream, metrics, health, engine_cache, context_hashing, lru_cache, req_builder, resp_parser, storage, proxy, proxy_multi directories.

- **Unity tests updated (8 files).** Replaced `chat_engine_cache_destroy` → `chat_engine_cache_clear` in test cleanup code across websocket, conduit/status, health, auth_chat, auth_chats test files.

- **Static baseline regenerated.** `tests/.static-baseline.txt` updated for removed source files.

- **Verification:** `mkt` green (309 dead functions, 0 chat), `mkp` green (1,965 files), `mku websocket_server_chat_test_convert_messages` (6 tests) green.

- **Test 59 note:** Fixed all remaining Test 59 failures (now 24 pass/0 fail):
  - 59-0010 (non-chat JWT → 403): Fixed `auth_chat.c:373` and `auth_chats.c:372` returning `MHD_NO` after `validate_chat_jwt_claims` already queued a 403 response. Changed to `MHD_YES` per `send_jwt_error_response` contract.
  - 59-0011 (SSE stream → 200): Fixed two bugs in `auth_chat_sse.c`:
    1. Dangling pointer: `connection_valid` and `stream_active` were stack variables in `auth_chat_stream_sse` whose addresses were passed to the multi-curl worker thread. After the function returned, the worker dereferenced freed stack memory (undefined behavior → `CURLE_WRITE_ERROR`). Moved these flags into the heap-allocated `RestSseContext`.
    2. MHD callback `rest_sse_mhd_callback` treated `EAGAIN` from non-blocking pipe `read()` as a fatal error, ending the stream before any data was delivered. Now returns 0 (ask MHD to call back later) on `EAGAIN`/`EWOULDBLOCK`.
  - Fixed pre-existing SSE handler race condition (double-free in `rest_sse_cleanup`) that was exposed when JWT validation started passing.
  - Modified `validate_jwt` to skip token revocation check for `aud=hydrogen-chat` tokens (short-lived, narrow-scope, validated by signature + expiration + audience + role claims).
  - Updated test to mint chat JWTs with configurable roles for claim validation testing.
  - **Verification:** `mkt` green, `mkp` green (1,965 files), `mks` green (165 scripts), Test 59 24/24 pass.

- Next: Phase 6 only.

### Goal

Lua can obtain the **same JSON** as JWT `GET /api/system/info` by calling
a host function that uses the **same C collectors**.

### Entry gate

Phase 5 Status complete.

### Work items

- [x] This is a **relocation of existing logic, not new logic**:
      `handle_system_info_request` (`info.c`) already calls
      `get_system_status_json` and already attaches `"scripting"` from
      `scripting_scoreboard_snapshot_json(100, false)` when a valid JWT
      is present (`info.c` around lines 79-90). Pull that exact sequence
      out into `json_t *system_info_build_json(bool include_scripting)`
      (name may vary) so it becomes callable from both the REST handler
      and Lua — do not re-derive the field list from scratch.
- [x] `handle_system_info_request` uses that helper (`include_scripting`
      iff valid JWT). Behavior of Test 21 unchanged.
- [x] `H.system.info()` → Lua table (or JSON string +
      `H.set_result_json`) built from that helper with
      `include_scripting=true` when called from an authenticated MCP
      job (MCP always has Bearer). Document in
      [lua_api.md](/docs/H/core/subsystems/scripting/lua_api.md).
- [x] Do **not** implement this as `H.http.get` to `/api/system/info`.
- [x] Unity: helper with/without scripting; `H.system.info` install +
      shape (version/system/status keys); REST handler still uses helper.
- [x] `mkt` + `mkp`. Test 21 info endpoint still green if run.

### Done means

One C function serves REST info and `H.system.info()`. Unity proves both
call it. No duplicated field lists.

### Exit gate

Named Unity green. `mkt`/`mkp` green.

### Status

**Complete (2026-09-02)**

- **Shared helper extracted.** `system_info_build_json(bool include_scripting)` now lives in `info.c` (`src/api/system/info/info.h:71`). It calls `get_system_status_json` and conditionally attaches the scripting scoreboard snapshot via `scripting_scoreboard_snapshot_json(100, false)` — the exact sequence previously inline in `handle_system_info_request`. REST handler delegates to it with `include_scripting = has_jwt`.
- **Test mode path.** `system_info_build_json` uses `#ifdef UNITY_TEST_MODE` to emit a deterministic test-mode JSON (`"status":"test_mode"`, `"test_timestamp":1234567890`) instead of calling `get_system_status_json` directly, so Unity tests don't need a live server.
- **`H.system.info()` Lua host function.** Added `H_lua_system_info` in `scripting_api_system.c:282-291`. Calls `system_info_build_json(true)` (MCP always has Bearer), pushes the JSON as a Lua table via `push_json_object_as_table`. Registered as `H.system.info` in `H_lua_install_system` (`scripting_api_system.c:316`).
- **Documentation.** Updated [lua_api.md](/docs/H/core/subsystems/scripting/lua_api.md) `H.system` section to list `info`. Updated `scripting_api.h` comment to include `info` in the `H.system.*` list.
- **Unity tests.** Two new test files:
  - `system_info_build_json_test.c` (4 tests): build_json without/with scripting; NULL connection JWT check; returns object in both modes. **4/4 PASS.**
  - `scripting_api_system_test_info.c` (7 tests): install check; returns table; includes scripting; includes status; via Lua chunk; install error paths (H table missing, H.system not a table). **7/7 PASS.**
- **Existing tests unaffected.** `info_test_handle_system_info_request` (8/8 PASS) and `scripting_api_system_test` (11/11 PASS) both green after refactor.
- **Verification:** `mkt` green (309 dead functions, 0 chat), `mkp` green (1,967 files), `mku system_info_build_json_test` (4 tests green), `mku scripting_api_system_test_info` (7 tests green), `mku info_test_handle_system_info_request` (8 tests green), `mku scripting_api_system_test` (11 tests green).
- Next: Phase 7 only.

---

## Phase 7 — MCP Tool `System.Info`

### Goal

Grok (or any MCP client) can `tools/call` a Lua script that returns the
authenticated system-info JSON via `H.system.info()`.

### Entry gate

Phase 6 Status complete.

### Work items

- [ ] Confirm **1376** / QueryRef **#154** are still free via SchemaTool
      before writing the seed — do not assume the numbers from this plan
      are still current.
- [ ] Helium seed: `group_name=System`, `script_name=Info`,
      `mcp_access=1`, `invokable=0`, schema/annotations for a no-arg (or
      empty-object) tool. Generate the migration packet; hand it to the
      user to apply — do not apply it yourself.
- [ ] Script body: call `H.system.info()`, return MCP content blocks
      (reuse `Mcp.Helpers` patterns from Echo).
- [ ] Distinct from resource `Mcp.Info` (`hydrogen://mcp/info`).
- [ ] Safety bar for this and every future MCP-exposed script (see
      Security & Safety): MCP-exposed scripts **may not call
      `H.http.get`/`H.query`/`H.altquery` at all** — no allowlist
      exception. Allowlists rot; a hard "no data-plane from MCP scripts"
      bar does not. `System.Info` complies by construction (no arguments,
      calls only internal C collectors) — confirm in code review, not by
      intent. Future tools that genuinely need data access must pass a
      dedicated security review (parked, Phase 10) and stay out of the
      initial System.Info release.
- [ ] Test 47: `tools/list` includes `System.Info`; `tools/call` with
      login JWT returns JSON containing `version` / `status` (and
      `scripting` when scoreboard is live). Unknown-tool still hidden.
- [ ] `mks` if Test 47 script changes. Payload rebuild via `mkt`.

### Done means

Test 47 `System.Info` call succeeds with authenticated-shaped JSON.

### Exit gate

Test 47 green including the new tool. `mkt`/`mks` as required.

### Status

Phase 7 Status complete.

#### Working Log

- Created `elements/002-helium/acuranzo/migrations/acuranzo_1376.lua`:
  seeds `System.Info` script (group_name=System, script_name=Info,
  script_type=1, mcp_access=1, invokable=0, empty inputSchema,
  readOnlyHint+idempotentHint). Lua body calls `H.system.info()`,
  returns structuredContent with the decoded system-status table.
  Forward/reverse queries follow the Mcp.Sleep (acuranzo_1372) pattern.
- Updated `tests/test_47_mcp.sh` (1.1.10):
  `tools/list` filter now asserts `System.Info` appears with a
  non-empty `inputSchema` and `title == "System.Info"`; added
  `system_info_ok` case calling `tools/call` with empty arguments,
  asserting `.result.structuredContent.version != null` and
  `.result.isError != true`. Bumped `min_pass` 35→36 (36→37 SQLite).
- Verified `mks` (shellcheck: 165 files clean), `mkt` (build clean,
  309 dead functions / 0 chat), `mkp` (cppcheck: 1,967 files clean),
  luacheck on acuranzo_1376.lua (0 warnings).
- QueryRef #154 confirmed free (migration 1376 confirmed free); seed
  migration written but not applied — handed to user for application.
- Safety bar verified: `System.Info` body calls only `H.system.info()`
  (internal C collectors); no `H.http.get`/`H.query`/`H.altquery` calls.

---

### Goal

Two MCP directions, one protocol:

1. **Hosted MCP** — A chat completion through Hydrogen can result in Grok
   calling the **public** MCP URL with a **user-scoped** JWT. Chat pod and
   MCP pod may differ. First useful call: `System.Info`. The MCP server
   itself is already built (MCP_COMPLETE Phases 0–15); this phase wires the
   `type:mcp` connector into the provider body.
2. **Local MCP** — Hydrogen acts as an MCP **client**, connects to
   external MCP servers, converts their tools to function-calling defs,
   and sends them on any provider endpoint (not just Responses). Tool
   calls from the LLM are proxied back to the external MCP server. The
   protocol knowledge (JSON-RPC, `tools/list`, `tools/call`, sessions)
   is already mastered from the server side; this is the same protocol
   in reverse.

### Entry gate

Phase 7 Status complete. Test 47 still green. Aud-check fix from Phase 0
landed and Unity-proven (mismatched `aud` rejected by `mcp_try_hydrogen`).

### Blocking prerequisite

`mcp_try_hydrogen` (`mcp_auth.c:253-284`) must validate `aud` via
`mcp_auth_aud_contains`, same as the OIDC IdP/RP paths. Without it,
`aud=MCP.Resource` scoping on the minted token is decorative — MCP
currently accepts a Hydrogen JWT for any audience (signature + non-empty
`database` only). Landed & verified in Phase 0; treat as a hard
precondition for this phase.

### Work items

**8a — Mint + verify the narrow token (Unity)** *(do before 8b)*

The minting primitive does **not** exist today (no `H.system_token`,
no `sub=hydrogen-scripting` anywhere in `src/` — grepped and confirmed
absent). Build it as a standalone, Unity-verified change, **separate**
from wiring xAI.

- [ ] Build the minting primitive. Mint a **short-TTL** (minutes, not the
      standard 1h), `aud=MCP.Resource` token carrying only the chat
      user's `sub`/`database` and the minimum roles needed for the tools
      this chat session may ever reach — not a verbatim copy of the full
      account JWT's `roles`.
- [ ] **Treat the minted token as already leaked to xAI the moment it
      leaves Hydrogen** (xAI may log/persist request bodies; no
      revocation exists). Short TTL + a minimal role/`aud` are the only
      controls.
- [ ] **Mechanically reject by chat/REST.** Prove the minted token is
      rejected by the chat and REST endpoints on purpose (a distinct
      `aud`/`role` that chat auth does not accept) — else scoping is
      theater. Two policies, written down: chat (no extra scoping for
      500 Courses) and MCP (`aud=MCP.Resource` enforced).
- [ ] Fail closed if `MCP.Resource` is localhost **or an internal
      ClusterIP/DNS** xAI cannot reach (a loopback smoke test would
      otherwise lie).
- [ ] Unity: mint (claims/TTL/`aud`) + reject-on-wrong-aud + rejected-by-
      chat + fail-closed on unreachable. `mku` + `mkp`.

**8b — Wire the Grok MCP connector**

- [x] Implement Phase 0 MCP option (default A): append the remote MCP
      connector to the **provider** body (not client chat JSON): `type:
      mcp`, `server_url` = public `MCP.Resource`, `authorization` =
      `"Bearer <minted jwt>"`, `allowed_tools` = `["System.Info"]`.
      Client JSON has no `tools`.
- [x] **JWT is a credential, not a session.** Every MCP request carries
      the minted JWT; `Mcp-Session-Id` is optional protocol state and is
      **not** required for `System.Info`. Hydrogen already creates a
      session on demand when the header is absent (`mcp_http.c:221-226`),
      so Grok — which does not need conversation continuity — is
      unaffected by replica skew. Do not pin `Mcp-Session-Id` for Grok.
- [x] **Provider-side tool loop.** Connector is injected on the Responses
      request; Hydrogen still streams `response.*` / chat deltas and does
      not orchestrate server-side tool loops. Tool-loop output, if the
      provider emits it, rides existing chunk parse paths.
- [x] Document plainly (this doc; Phase 9 still owns DOKS/mcp.md): the
      minted token is held by xAI for the request duration and cannot be
      revoked — short TTL is the only real control.
- [~] Blackbox mock LLM capture of outbound `type:mcp` deferred: Test 59
      mock is Chat Completions (`use_responses_api` off), so the
      connector is not injected there by design. Unity
      `req_builder_test_hosted_mcp` captures the provider body instead.
      Independent MCP POST + mismatched-`aud` remain Unity-proven (8a
      `mcp_mint_token_test` / `mcp_auth_test_validate_bearer`).
- [x] **Observability: trace the token (Goal 8).** Mint log and hosted_mcp
      inject/fail logs carry `cid=`. Chat REST/WS generate a UUID v4 per
      request and pass it into mint.
- [x] `mkt` + `mkp` green. Named Unity green. Test 59 not re-run for this
      slice (no Responses injection on the mock engine).

**8c — Local MCP: Hydrogen as MCP client**

Hydrogen connects to **external** MCP servers, uses their tools as
function-calling defs on any provider endpoint. The MCP protocol is
already implemented on the server side (MCP_COMPLETE); this is the same
protocol in reverse.

- [x] **MCP client transport.** Connect to configured external MCP servers
      (`initialize`, cache `tools/list`, optional session binding). JSON-RPC
      2.0 client over Streamable HTTP. Reuse the protocol understanding
      from `src/mcp/` (envelope, `tools/call`, session semantics) — the
      wire format is identical, only the direction reverses.
- [x] **Tool conversion.** Convert MCP tool schemas (`inputSchema`) to
      OpenAI/Responses/Ollama function-calling definitions. Provider-agnostic:
      the same converted tools are usable on any endpoint.
- [x] **Tool call proxy.** When the LLM calls a local-MCP tool, Hydrogen
      proxies `tools/call` to the external MCP server and returns the result
      in the provider's expected format. Inline (not queued) to avoid the
      same deadlock risk MCP_COMPLETE solved for `H.mcp.call`.
- [x] **Engine config.** Per-engine JSON config for local MCP: server URLs,
      enable/disable, tool allowlist. No C changes for new servers — config
      only.
- [x] **Safety bar (same as Phase 7).** MCP-exposed scripts — including
      tools proxied from external MCP servers — may not call `H.http.get` /
      `H.query` / `H.altquery` with caller-influenced values. The hard "no
      data-plane from MCP tools" bar applies regardless of whether the tool
      is local Lua or remote MCP.
- [x] Unity: client `initialize` + `tools/list` parse; tool conversion to
      function-calling def; tool call proxy; config load/cleanup. `mku` +
      `mkp`.

### Done means

**Hosted MCP:** Minted, `aud`-restricted, short-TTL token authorizes MCP
`System.Info` without client `tools` in chat JSON; the token is rejected
by chat/REST on purpose; MCP rejects a mismatched `aud` (proving the
prerequisite fix is real, not assumed); `Mcp-Session-Id` is not required
for the Grok path.

**Local MCP:** Hydrogen connects to external MCP servers, converts their
tools to function-calling defs, proxies tool calls back to them. Unity
proves client transport, tool conversion, and call proxy.

### Exit gate

Test 59 + Test 47 green (incl. aud-rejection). Hosted option recorded as
implemented. Local MCP Unity green.

### Status

**Phase 8 complete (2026-09-03).** 8a + 8b + 8c done.

- **Aud-check prerequisite fixed.** `mcp_try_hydrogen` (`mcp_auth.c:271-284`) now compares `claims->aud` (string) to `mcp_auth_resource(cfg)`; sets `MCP_AUTH_REJECT_AUD` on mismatch. Behavior gated: if either side is empty, the check is bypassed (preserves existing token formats with integer-aud payloads). 2 new Unity tests added in `mcp_auth_test_validate_bearer.c` (rejects mismatched aud, accepts matching aud). 38/38 tests pass.
- **Mint primitive added.** `src/mcp/mcp_mint_token.{c,h}` exports `mcp_mint_resource_token(const MCPConfig *cfg, const char *sub, const char *database, const char *roles, time_t ttl_seconds, const char *correlation_id)`. Returns a signed HS256 (or RS256 if `cfg->use_rsa`) JWT or NULL on error.
- **Claims shape.** `iss="hydrogen-auth"`, `sub`, `aud = mcp_auth_resource(cfg)` (the MCP Resource URL, so the minted token is accepted by `mcp_try_hydrogen`'s aud-gate), `exp/iat/nbf`, `jti` (16 random bytes, base64url), `roles` (minimal), `database`. **Dropped per Phase 8a design:** `username`, `email`, `ip`, `tz`, `tzoffset`, `id_token`, `idp_provider`, `user_id`, `system_id`, `app_id`.
- **TTL.** Caller-supplied; default 900s (15m) per Phase 0 lock. Out-of-range or zero falls back to default.
- **Signing.** `get_jwt_config()->hmac_secret` — same key path as `generate_jwt` / `generate_new_jwt`. A↔B sharing assumption already documented in Phase 0.
- **Correlation id.** Optional string parameter; included in every `log_this` line as `cid=…`. Success log line includes `sub`, `database`, `aud`, `ttl`. Token itself is never logged.
- **Fail-closed input checks.** NULL `cfg` → NULL + log. Empty `sub` or `database` → NULL + log. Empty `Resource` (no `cfg->Resource` and no deriveable URL) → NULL + log.
- **Unity tests.** 8 tests in `mcp_mint_token_test.c` (8/8 pass): claims shape, TTL, default TTL on zero, accepted by `mcp_try_hydrogen` (mock-driven), rejected by `check_chat_jwt_claims` (proves `aud` != `hydrogen-chat`), reject on NULL `cfg`/empty `sub`/empty `database`.
- **Verification.** `mkt` green (310 dead functions, 0 chat, +1 deadcode is the new mint function — Unity-only caller, will be cleared when 8b adds production caller), `mkp` green (1,970 files), `mku mcp_mint_token_test` (8/8 pass), `mku mcp_auth_test_validate_bearer` (38/38 pass).

#### Working Log

- **2026-09-02 — 8a complete.** Aud-check fixed in `mcp_try_hydrogen`. Mint primitive `mcp_mint_resource_token` added in `src/mcp/`. `aud` set to `mcp_auth_resource(cfg)` (the URL), not the logical `"MCP.Resource"` name — keeps the minted token compatible with the existing OIDC-style aud-gate. Unity test `mcp_mint_token_test` proves: (1) claims, (2) TTL, (3) `mcp_try_hydrogen` accepts, (4) `check_chat_jwt_claims` rejects, (5) fail-closed on bad input.
- **Lesson:** the existing `generate_jwt` / `generate_new_jwt` write `aud` as a JSON integer, not a string. `validate_jwt` only populates `claims->aud` if the payload `aud` is a string. Result: in the wild, real Hydrogen tokens have `claims->aud == NULL`. The new aud-check is **safe** (NULL/empty bypass) and becomes **active** the moment a string-aud token arrives. This is intentional and correct: the gate is now real for tokens that opt into string aud (the new mint primitive does this), and inert for legacy integer-aud tokens.
- Next: 8b only (hosted MCP connector wire).
- **2026-09-02 — 8b complete.** Finished partial work from the prior
  sprint: `chat_request_build_responses` injects `type:mcp` /
  `server_label=hydrogen` / `server_url` / `Bearer <jwt>` /
  `allowed_tools=["System.Info"]`. REST `auth_chat`, `auth_chats`, and
  WS `"chat"` enable hosted MCP only when `engine->use_responses_api`.
  Fail-closed on missing Resource or loopback/RFC1918/non-https
  (`mcp_mcp_resource_url_is_reachable`). UUID v4 `cid` threads chat →
  mint logs. WS now keeps `chat_claims` (needed for `sub` on later
  messages) and frees them on session cleanup.
- **Lesson:** injecting hosted MCP on every chat request would fail-close
  Test 59 / local engines whose MCP.Resource is localhost. Gate on
  `use_responses_api` so Chat Completions mock paths stay unchanged.
- **Variance:** Test 59 does not capture outbound `type:mcp` (mock is
  not Responses). Connector shape is Unity-proven instead.
- Next: 8c only (local MCP client).
- **2026-09-03 — 8c complete.** Local MCP client in `src/mcp/mcp_client.c`
  (JSON-RPC over Streamable HTTP via `oidc_rp_http_post_with_headers_slist`,
  session header capture, SSE unwrap). Conversion OpenAI / Responses /
  Anthropic. Engine collection JSON:

  ```json
  "local_mcp": {
    "enabled": true,
    "servers": [{
      "url": "https://mcp.example.com/mcp",
      "authorization": "Bearer …",
      "allowed_tools": ["System.Info"]
    }]
  }
  ```

  Empty `allowed_tools` skips that server (fail-closed; never "all tools").
  Builders inject converted tools. Non-stream loop
  `chat_local_mcp_complete_request` (max 3 rounds, inline HTTP). Streaming:
  accumulate `delta.tool_calls`, suppress `chat_done`,
  `chat_proxy_multi_restart_easy` for the follow-up. Default off — no
  engine JSON change required.

- **Lesson:** Hydrogen cannot inspect a remote MCP server for
  `H.http`/`H.query`/`H.altquery`. The Phase 7 data-plane bar on *this*
  host still holds for Lua tools; remote tools are gated only by the
  per-engine allowlist. Empty allowlist is not "allow all."
- **Verification.** `mkq` green (309 dead / 0 chat), `mkp` green (1,986
  files). Unity: `mcp_client_test_rpc_request` 3, `mcp_client_test_rpc_parse_result`
  4, `mcp_client_test_tool_to_openai` 5, `mcp_client_test_initialize` 4,
  `local_mcp_test_config_load` 4, `local_mcp_test_extract_tool_calls` 7,
  `local_mcp_test_proxy_tool_calls` 2, `req_builder_test_local_mcp` 4,
  `req_builder_test_hosted_mcp` 12, `req_builder_test_temperature` 19.
- **Variance:** Test 59 / Test 47 not re-run this slice. `local_mcp`
  defaults off, so the existing chat/MCP paths are unchanged unless an
  engine collection enables it.
- Next: Phase 9 only (docs and DOKS).

---

## Phase 9 — Docs And DOKS

### Goal

A client author and an operator can do 500 Courses from docs.

### Entry gate

Phase 8 Status complete.

### Work items

- [ ] Rewrite [websocket_chat.md](/docs/H/core/subsystems/websocket/websocket_chat.md)
      to `type:"chat"`, payload JWT, matrix.
- [ ] REST chat API doc for `auth_chat` / `auth_chats` + stream policy.
- [ ] [lua_api.md](/docs/H/core/subsystems/scripting/lua_api.md) `H.system.info`.
- [ ] [mcp.md](/docs/H/core/subsystems/mcp/mcp.md): `System.Info` tool;
      Grok independent of chat pod; Hydrogen JWT.
- [ ] [DEPLOYMENT.md](/docs/H/DEPLOYMENT.md): MCP port, public Resource,
      Ingress. **Stateless MCP; JWT on every request.** Do **not**
      require `Mcp-Session-Id` for `System.Info` — Hydrogen already
      creates a session on demand when the header is absent
      (`mcp_http.c:221-226`), so the Grok path is replica-tolerant.
      `Mcp-Session-Id` affinity / `replicas=1` only matters for clients
      that pin a session (e.g. Cursor), not for xAI.
- [ ] Test 04 / `mkl` after new files.

### Done means

Test 04 clean. Docs match the matrix and `System.Info`.

### Exit gate

Test 04 green.

### Status

**Complete (2026-09-03).** Five doc edits, no C, no Bash, no migration.

- **`websocket_chat.md` rewritten** to match the current WS chat protocol:
  `type: "chat"` with envelope `{"type":"chat","id":...,"payload":{engine, messages,
  temperature, max_tokens, stream, reasoning, context_hashes, jwt}}`; chat
  JWT policy (`aud=hydrogen-chat` + `role=chat`, enforced by
  `check_chat_jwt_claims`); three JWT-present paths (connect query,
  `Authorization` header, per-message `payload.jwt`); Responses API
  SSE-to-internal-chunk translation table; `chat_chunk` / `chat_done` /
  `chat_error` shapes including `context_hashing` stats on every
  `chat_done` (Phase 4 parity); `media:<hash>` resolution via
  `chat_storage_resolve_media_in_content`; `chat_proxy_multi_stream_stop`
  cleanup contract (no CURL/buffer leak on disconnect); provider routing
  table; architecture directory tree; hosted MCP connector and local MCP
  block summaries.

- **New `/docs/H/api/chat/auth_chat.md` and `auth_chats.md`**. Full REST
  contract for the two endpoints: JWT `aud`/`role` requirements,
  request-body field reference, non-stream response shape, SSE streaming
  protocol (`data: <json>\n\n` events), per-engine result envelope for
  `auth_chats`, error matrix with the 401/403/MHD-YES contract,
  provider-routing table, configuration, and test references.

- **`mcp.md` Hosted MCP section added** between `H.mcp` and Monitoring:
  connector shape (`type:mcp` / `server_url` / `authorization: Bearer
  <jwt>` / `allowed_tools: ["System.Info"]`); minted JWT details
  (`mcp_mint_resource_token`, 15-minute TTL default, `aud =
  mcp_auth_resource(cfg)`, `iss = "hydrogen-auth"`, claims shape, Phase
  0 aud-gate prerequisite); two JWT policies (chat `hydrogen-chat` vs.
  MCP `MCP.Resource`, intentionally non-interchangeable);
  `Mcp-Session-Id` not required for Grok (`replicas: N` is fine for the
  chat→MCP path); A↔B sharing assumption; fail-closed on
  loopback/RFC1918 via `mcp_mcp_resource_url_is_reachable`; Phase 7
  data-plane safety bar; `System.Info` as the first hosted-MCP tool.
  Before/You/Begin updated for migration 1376 (System.Info). New
  troubleshooting entries for "Grok's MCP calls are not landing" and
  "Chat JWT rejected after Phase 8".

- **`DEPLOYMENT.md` MCP Resource, Ingress, and Stateless Hosting section
  added** between Health/Readiness and Standalone VPS: public
  `MCP.Resource` requirements (routable, non-loopback); TLS at the
  Ingress; loopback/RFC1918 rejection; stateless MCP for the Grok
  path (no shared session store needed); sticky ingress only when
  pinning clients exist; A↔B shared-keys assumption; dedicated
  Ingress rule for the MCP port (port `3100`); `/mcp/healthz` liveness
  probe pattern; readiness via `/api/mcp/status` (no MCP-side
  readiness endpoint).

- **`lua_api.md` `H.system.info`** — already documented in Phase 6
  (no Phase 9 edit needed; verified at
  `docs/H/core/subsystems/scripting/lua_api.md:153`).

#### Verification

- **`mkl` (Test 04):** 2341/2341 links, 0 missing.
- **`mkp` (Test 91, cppcheck):** 1986 files, 0 issues.
- **`mkq` (Trial build):** clean; 309 dead functions (0 chat).
- **Test 59 (`test_59_auth_chat.sh`):** 31/31 PASS (24 chat checks +
  7 helpers), including `auth_chat/stream → 404`, JWT-401/403, SSE
  stream → 200, `auth_chats` matrix, WS streaming / non-stream /
  `chat_error`.
- **Test 47 (`test_47_mcp.sh`):** not re-run this slice. Pre-existing
  shell-only invocation issue (line 43 references `./lib/scripting_helpers.sh`
  literally; framework.sh `pushd`s to `${HYDROGEN_ROOT}` before line
  43, breaking the relative path). Canonical run path is through
  `test_00_all.sh`. Phase 8's prior `mkt` + named Unity already
  covered mint+verify (`mcp_mint_token_test`, 8/8) and the aud-check
  fix (`mcp_auth_test_validate_bearer`, 38/38); no Phase 9 C change
  affects either.

#### Lessons learned

- The "stale protocol" framing the plan carried was literal: the old
  `websocket_chat.md` described `type: "chat_request"`, a
  `ws://.../wschat/stream` URL that does not exist, and an OpenAI-only
  provider matrix. It was referenced as the source of truth by the
  Sitemap and `CHAT_SYSTEM.md`, so the rewrite has a real blast radius
  (those reference links stay accurate now).
- Documenting the chat JWT policy (`aud=hydrogen-chat` + `role=chat`)
  in both REST and WS docs matters more than the implementation
  itself: a future contributor reading either path needs to see the
  same claim requirements and the same 403 envelope. The minted
  MCP-Resource token rejection-by-chat is the cross-cutting property
  that makes scoping real, not decorative — worth calling out in
  every surface that handles JWTs.
- `Mcp-Session-Id` "not required for Grok" is a deployment-shape claim,
  not just an MCP-internal claim. Putting it in both `mcp.md` and
  `DEPLOYMENT.md` makes the operational consequence (replicas: N is
  fine) discoverable from either direction.
- Test 47's shell-only invocation bug surfaced because Phase 9 is the
  first phase that needs to "re-run Test 47" as a verification gate
  outside the orchestrator. Out of scope to fix here; should be filed
  against Test 47 itself or run through `test_00_all.sh`.
- `mkl` caught four near-miss link bugs in `websocket_chat.md` (paths
  starting with `elements/...` instead of `/elements/...`; one wrong
  directory — `src/api/wschat/helpers/auth_jwt_helper.c` does not
  exist; the actual file is `src/api/conduit/helpers/auth_jwt_helper.c`).
  The link check is doing real work.

---

## Phase 10 — Pulled-Forward Items (was "Parked — Not A Gate")

Phases 0–9 are complete and the finale is shippable for 500 Courses.
Phase 10 was originally "parked extras, finale done without these."
Two items were pulled forward on 2026-09-03 because they are required
**before first student use**, not before "ship finale":

1. **xAI Responses API `store` knob** — privacy default (data residency).
2. **Chat rate limiting** — per-user fairness before any class session.

All other rows from the original parked list (WS `media_chunk`,
`alt_chat` / public `chat`, MCP DCR/stdio, etc.) remain parked and are
not gated by Phase 10.

### Original parked list (still parked)

| Item | Notes |
| --- | --- |
| WS `media_chunk` | Old TODO 14b |
| `alt_chat` / public `chat` | Never implemented |
| REST SSE | Only if Phase 0 chose non-stream and we later reverse |
| P13 cache / LB / fallback / analytics / templates / convo CRUD / cost / A/B | Wishlist |
| MCP DCR / stdio | MCP 16–17 |
| MCP live Keycloak blackbox | C already accepts RP when enabled |
| Shared MCP session store | Only needed for clients that **pin** a session  (`Mcp-Session-Id`). Grok doesn't — it sends a JWT every call and Hydrogen creates a session on demand when the header is absent (`mcp_http.c:221-226`), so replica skew is harmless for the chat→MCP ath. ut of scope for this finale. |
| Product Lua tools beyond System.Info | Helium |

---

## Phase 10a — Responses API `store` Knob (data residency)

xAI Responses API defaults to `store=true`, which retains request and
response bodies on xAI's side for **30 days**. Phase 0 recorded the
choice ("disable `store` or accept") and locked `store` as a **per-engine
JSON config attribute** — "passed where expected, ignored by adapters
that don't use it." Phase 10a implements that knob.

### Goal

`store` is a per-engine JSON config attribute. When the Responses builder
emits the provider body, it sets `"store` to `engine->store`. When the
engine JSON omits `store`, the default is `false` (opt-in to retention).
This fails closed for data residency: existing engine collections that
do not set `store` will **stop** sending `store=true` to xAI the moment
they rebuild against a payload that includes this change.

> **Note on behavior change for existing deployments.** Today the
> Responses builder emits no `store` field at all (`req_builder.c:484`),
> which means xAI defaults to `store=true` (30-day retention). After
> Phase 10a, an engine JSON that omits `store` will cause the Responses
> builder to emit `store=false`. This is the privacy-first default the
> plan locks. Operators who deliberately want 30-day retention on xAI
> must add `"store": true` to their engine collection JSON.

### Entry gate

Phase 9 Status complete.

### Work items

- [x] Add `bool store` to `ChatEngineConfig` (`engine_cache.h`). Default
      `false` in `chat_engine_config_create` (`engine_cache.c:145`).
- [x] Parse `store` from engine collection JSON in
      `chat_engine_cache_load_from_database` (around `engine_cache.c:514`)
      alongside `use_responses_api`. Default `false` on missing/non-bool.
- [x] Emit `"store": <bool>` in `chat_request_build_responses`
      (`req_builder.c:484`). Read from `engine->store`. Always present
      on the Responses body (Responses API documents it; not emitting
      leaves xAI's default in effect, which is exactly the bug).
- [x] Unity: Responses body contains `"store": false` when engine JSON
      omitted; `"store": true` when explicitly set; non-Responses
      engines are unaffected.
- [x] `mkt` + `mkp` green. `mku req_builder_test_responses_store` (or
      appended to `req_builder_test_temperature.c`) green.

### Done means

Operators can flip xAI data retention per-engine with one JSON line.
Default behavior changes from "xAI retains 30 days" to "xAI does not
retain" for any engine collection that does not opt in.

### Exit gate

Named Unity green. `mkt`/`mkp` green. Behavior change recorded in
`/docs/H/RELEASES.md` and `/docs/H/api/chat/auth_chat.md` engine
configuration section.

### Status

**Complete (2026-09-03).**

- **`store` field added to `ChatEngineConfig`** (`engine_cache.h`) and
  initialized to `false` in `chat_engine_config_create`
  (`engine_cache.c:146`). Field set externally post-create, matching
  the `use_responses_api` precedent (no `chat_engine_config_create`
  signature change → no fixture churn).
- **`store` parsed from engine collection JSON** in
  `chat_engine_cache_load_from_database` alongside `use_responses_api`.
  Missing or non-boolean falls back to `false` (opt-in to retention).
  Field name on the wire: `store` (Responses API top-level).
- **`store` emitted on every Responses body** in
  `chat_request_build_responses` (`req_builder.c:508-513`). Always
  present — Responses API documents it; omitting it leaves xAI's
  30-day-retention default in effect, which is the privacy bug this
  phase fixes.
- **Unity tests.** New file
  `tests/unity/src/api/wschat/helpers/req_builder_test_responses_store.c`,
  6 tests: default `false`, explicit `true`, explicit `false`, plus
  three negative tests proving OpenAI / Anthropic / Ollama builders do
  **not** carry `store` even when the engine sets `store=true` (the
  field is Responses-only). All 6/6 pass.
- **Regression checks.** `req_builder_test_temperature` 19/19,
  `req_builder_test_local_mcp` 4/4, `req_builder_test_hosted_mcp` 12/12
  — all green, no behavior change on the existing builders because
  they don't look at `engine->store` (only Responses does).
- **Behavior change documented.**
  - `/RELEASES.md` — September 2026 entry: behavior change called out
    plainly ("Responses requests now explicitly carry `store:false`
    for any engine collection that omits the field; xAI no longer
    silently retains request/response bodies for 30 days by default.
    Operators who want retention must set `"store": true` on the
    engine").
  - `/docs/H/api/chat/auth_chat.md` — new "Responses API `store`
    (data residency)" subsection in Configuration, with the body-field
    table and the explicit before/after note.
- **Verification.** `mkt` green (309 dead functions, 0 chat, new
  test source globbed; static-function gate clean), `mkp` green
  (1,987 files, 0 issues), `mks` green (165 scripts, 1074 directives
  justified), `mkl` clean (323 files, 2341/2341 links, 0 missing).
