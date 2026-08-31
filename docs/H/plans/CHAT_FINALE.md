<!-- markdownlint-disable MD007 MD024 -->
# Chat Finale

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

This document is the **only** active chat plan. History:
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

**CURRENT PAUSE POINT (as of 2026-08-30):** Plan written. No phase started.
Next: **Phase 0**.

### Resume here next session

1. Confirm the latest completed phase via Status blocks.
2. Re-read only the next phase Goal + Done means + Exit gate.
3. `zsh -ic 'mkt'`; relevant `mku`; Test 59 / 47 as named in that phase.
4. Implement that phase only → verify Exit gate → update this doc → stop.

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
2. Session with **Grok** (xAI CEC; wire format — Chat Completions vs
   Responses — locked by the Phase 0 spike).
3. Questions return with **streaming** (per matrix), **temperature**,
   **reasoning**, the provider's max-tokens knob, and other obvious
   provider knobs — field names as locked by Phase 0 (endpoint spike).
4. Grok can answer by calling **Hydrogen MCP** on the **public** MCP
   URL, possibly a **different instance** than chat (DOKS). Intended.
5. An MCP **Lua tool** returns the same JSON as authenticated
   `GET /api/system/info`, by **reusing** the C collectors — not by
   HTTP-calling the REST endpoint, not by a second implementation.
6. Shared parse/build/proxy where sharing is real. Published support matrix.
7. Chat-related `mkt` dead functions driven to **zero**. Do not baseline them.
8. **Observability.** One opaque correlation/request id threads the chat →
   mint → MCP three-hop path (logs, not a shared store) so a missing MCP
   call is distinguishable from a silent storage failure at 1 a.m. Locked
   by Phase 0; verified by Phase 5 (logged failures) and Phase 8 (mint/MCP
   trace).

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
500 Courses client
  ├─ REST  POST /api/conduit/auth_chat[s]   Bearer <Hydrogen JWT>
  └─ WS    type:"chat"  payload.jwt
            │
            v
     Hydrogen instance A (chat proxy)  ──►  xAI Grok
                                                 │
                                                 │ independent MCP
                                                 v
     Public MCP URL (Streamable HTTP)
            Authorization: Bearer <user-scoped JWT>
            │
            v
     Hydrogen instance B (maybe)
            Lua tools, including System.Info
            (same JSON as JWT GET /api/system/info)
```

---

## Current Observed State (2026-08-30)

### Works

| Surface | Status |
| --- | --- |
| REST `auth_chat` / `auth_chats` non-stream | JWT, CEC, proxy, storage, hashes, `media:` |
| WS `"chat"` stream and non-stream | multi_curl + `chat_done` |
| WS `"media_upload"` | Single-frame |
| CEC `xai` | Maps to OpenAI-compatible |
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
| Scoped MCP token minting | Does not exist. `H.system_token` / `sub=hydrogen-scripting` referenced below as "don't use this" is **not present in the codebase at all** — there is no minting primitive to avoid or to use yet |
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
| `max_tokens` | yes | yes | yes |
| Reasoning request | add | add | add |
| Reasoning inbound | n/a if no REST stream | n/a | chunks |
| `context_hashes` | yes | as today | parity or drop |
| `media:` | yes | as today | parity |
| Persist | yes | as today | parity |
| Client `tools` | **no** | **no** | **no** |
| Grok MCP | Phase 8 | Phase 8 | Phase 8 |

**Lock notes for Phase 0 (from independent review):**

- **Streaming error contract** is locked as **400 + stable JSON, use WS** —
  not a `200` body that is only a single SSE error event.
- **Reasoning inbound** (`reasoning_content`) lives on WS chunks **only if**
  Phase 0's spike confirms Grok chat stays on the Chat Completions wire
  format; on xAI's Responses API, reasoning is a different field entirely.
  Do not assert `reasoning_content` in Unity until the endpoint is locked.
- **Grok may not even reach `/v1/chat/completions` with MCP.** The spike
  probes that first. If MCP only works on `/v1/responses`, Phases 1–4
  target the Responses wire format.

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
| 0 | Decisions written in Status (endpoint spike, streaming policy + error
contract, aud-check outcome, System.Info scope, cost cap); no C |
| 1 | Provider JSON uses client/engine temperature, not `1.0` |
| 2 | Reasoning knobs round-trip; inbound chunks consistent with locked endpoint |
| 3 | REST matches matrix; no fake 501/200 SSE |
| 4 | WS matches matrix |
| 5 | Zero chat names in `dead_functions.txt` |
| 6 | `H.system.info()` === JWT `/api/system/info` collectors |
| 7 | MCP tool `System.Info` callable; Test 47 proves it |
| 8 | Grok (or mock) calls public MCP with a minted, short-TTL
`aud=MCP.Resource` JWT; `aud`-gate proven real; mint+verify (8a) before
wire (8b); `Mcp-Session-Id` not required for the Grok path |
| 9 | Docs and DOKS notes match code |
| 10 | Parked extras — not a gate |

---

## Phase 0 — Contract Lock

### Goal

Write the decisions later phases implement. No C.

### Entry gate

This document exists.

### Work items

- [ ] **Spike: live CEC endpoint probe.** `curl` the actual Grok engine
      URL / CEC config with a dummy `type:mcp` tool body on the **same
      endpoint** (`/v1/chat/completions`) CEC already uses. xAI documents
      Remote MCP on the native SDK / **Responses API**, not Chat
      Completions. If `/v1/chat/completions` ignores or rejects the
      `type:mcp` tool, lock `/v1/responses` **before** Phases 1–4 write
      any Unity asserting Chat Completions knobs (temperature,
      `reasoning_content`, streaming) — otherwise the wire format is
      wrong under the hood. Record result + endpoint in Status.
- [ ] **Lock whether Grok chat moves to the Responses API.** If it does
      (per the spike): streaming = SSE `response.*` (not chat deltas),
      reasoning is a Responses field (not `reasoning_content` in Chat
      Completions deltas), `max_output_tokens` (not `max_tokens`), and
      default `store=true` retains request/response 30 days on xAI's
      side. This rewrites the Phase 1–3 meaning of "temperature,"
      "reasoning," and "stream." Encode a data-residency Non-Goal.
- [ ] Fill Support Matrix **Decision** column.
- [ ] Choose REST streaming: request/response **or** real SSE via multi_curl.
- [ ] Lock the exact error contract for the losing choice (status code +
      JSON error shape) so the 500 Courses client can code against it
      before Phase 3 lands — **400 + stable JSON, use WS** (do not ship a
      200-with-one-SSE-error-event body that returns 200 and is only an
      error event).
- [ ] Choose Grok MCP option A / B / C (default A).
- [ ] Name 500 Courses Grok engine in CEC and the public MCP URL shape.
- [ ] Lock the MCP `authorization` header form: `"Bearer <jwt>"`, **not**
      the raw JWT — xAI Remote MCP third-party examples require the
      `Bearer ` prefix. Lock `allowed_tools` to `["System.Info"]` only
      (empty = every `mcp_access=1` tool Hydrogen ever grows).
- [ ] Decide chat JWT policy: does chat require a specific `aud` and/or
      role beyond a non-empty `database` claim (today it does not check
      either)? Record the decision even if it is "no change." **Record
      two policies**: chat (no extra scoping for 500 Courses) and MCP
      (`aud=MCP.Resource` enforced) — do not pretend Phase 8 scoping is
      real on the chat side.
- [ ] Decide the Phase 8 token-handoff shape: what exactly gets minted
      and sent to xAI as `authorization` (claims, TTL, which tools it
      authorizes). Do not defer this to Phase 8 as an implementation
      detail — it's a credential-exfiltration surface to a third party
      and needs an explicit decision now. See Security & Safety. **Treat
      the minted token as already leaked to xAI the moment it leaves
      Hydrogen**: short TTL + minimal role/`aud` that chat/REST **reject
      on purpose** are the only controls (no revocation exists).
- [ ] **Split token work (process).** Mint + verify the narrow token in
      Unity (its own change, its own review) **before** wiring the xAI MCP
      connector — do not do both in one phase. See Phase 8 restructure.
- [ ] **Aud-check fix (ideal Pre-Phase-0 land).** Strengthen
      `mcp_try_hydrogen` (`mcp_auth.c:253-284`) to call
      `mcp_auth_aud_contains` like the OIDC IdP/RP paths. Land + Unity-
      prove a mismatched-`aud` JWT is rejected **before** Phase 0 Status
      is marked complete if possible; at minimum before Phase 8.
- [ ] **Deployment assumption: A↔B sharing.** If chat runs on instance A
      and MCP on instance B, both must share signing keys, aligned clocks,
      and the same `sub`/`database` claim space. Record as a Phase 0
      assumption, not a Phase 9 afterthought.
- [ ] **Fail-closed for `MCP.Resource`, beyond localhost.** Lock rejection
      of localhost **and** internal ClusterIP/DNS that xAI cannot reach —
      a loopback "Grok called MCP" smoke test otherwise lies.
- [ ] **Stateless first tool path.** Lock that the first MCP tool call
      tolerates a missing/rotating `Mcp-Session-Id` (initialize +
      tools/call on one POST, or sessionless) — xAI session affinity on
      that header is not guaranteed; sticky ingress is a hope, not a
      dependency.
- [ ] **System.Info scope.** Decide whether the authenticated system-info
      JSON includes the `scripting` scoreboard (operational inventory) or
      only `version`/`status`/`system`. Lock as a product choice; Grok
      must be permitted to see the chosen set (reuse of collectors is
      engineering; reuse of the *authenticated* view is a product choice).
- [ ] **One-line cost cap for 500 Courses.** Record a cap story
      (`max_output_tokens` ceiling / per-session token budget / per-user
      rate limit) even if enforcement stays in Phase 10.
- [ ] **Correlation id across the three-hop path.** Lock one opaque
      request/correlation id that threads chat (incoming request) → mint
      log → MCP tool log, so "Grok didn't call MCP" is distinguishable from
      "MCP call failed" and storage failures surface with context (not just
      "not found"). Logged by all three hops, not a shared store. Required
      by Goal 8.
- [ ] Run `zsh -ic 'mkt'` once to (re)generate
      `build/deadcode/dead_functions.txt` (it is not persisted between
      checkouts), then snapshot current chat names into Working Log (do
      not baseline).

### Done means

Status block records matrix, stream policy + error contract (400 + stable
JSON, use WS), MCP option + engine name, `authorization` form
(`Bearer <jwt>`) + `allowed_tools` whitelist, chat JWT policy (two
policies), endpoint spike result (Chat Completions vs Responses), the
aud-check fix outcome, System.Info scope, fail-closed shape, stateless
Mcp-Session-Id stance, and the 500 Courses cost-cap line.

### Exit gate

Status complete. Review stop.

### Status

Not started.

---

## Phase 1 — Temperature Reaches The Provider

### Goal

`ChatRequestParams.temperature` is what OpenAI-compatible and Ollama
builders emit. Stop hardcoding `1.0`.

> **Depends on Phase 0 spike.** Temperature round-trips identically on
> either wire format, but confirm the chosen Grok endpoint before
> asserting any provider-specific field shapes elsewhere (see Phase 2).

### Entry gate

Phase 0 Status complete.

### Work items

- [ ] `chat_request_build_openai` / `_ollama`: use `params->temperature`
      when `>= 0`, else engine default (currently both hardcode `1.0`,
      `req_builder.c:127-128,356-357`).
- [ ] `chat_request_build_anthropic` currently emits **no** `temperature`
      field at all — decide and implement one behavior: either start
      emitting it the same way as the other two builders, or explicitly
      document why Anthropic stays field-absent. Do not leave it
      ambiguous between "not yet done" and "intentionally different."
- [ ] REST and WS already resolve via `auth_chat_resolve_request_params`
      (or equivalent) — keep one resolver.
- [ ] Unity: omitted → engine default; `0.2` → `0.2` in JSON; `1.0` → `1.0`
      for OpenAI/Ollama. Add an explicit Anthropic assertion matching
      whichever behavior was chosen above (field present-with-value vs.
      confirmed absent) so a future change can't silently regress it.
- [ ] `mkt` + `mkp`.

### Done means

Unity proves provider JSON temperature is not a hardcoded `1.0`.

### Exit gate

Named Unity green. `mkt`/`mkp` green.

### Status

Not started.

---

## Phase 2 — Reasoning Knobs And Inbound Chunks

### Goal

Request-side reasoning that Grok/xAI actually accept. Inbound
`reasoning_content` on every WS chunk path.

> **Gated on Phase 0 endpoint spike.** xAI documents `reasoning_content`
> as a Responses-API behavior, not a Chat Completions delta. If the spike
> locks `/v1/responses`, the inbound assertion must target the Responses
> reasoning field, not chat `delta.reasoning_content`. The "Confirm
> current xAI field names" work item below is the lock point.

### Entry gate

Phase 1 Status complete.

### Work items

- [ ] Confirm current xAI field names (`reasoning`, `reasoning_effort`,
      or extra body). Parse from client JSON; pass through builder
      (`additional_params` or explicit fields). Do not invent names.
- [ ] `proxy_multi.c` final chunk copies `reasoning_content` like
      `proxy_mc.c`.
- [ ] Unity for parse + emit + chunk parse.
- [ ] `mkt` + `mkp`.

### Done means

A request with the documented reasoning field appears on the provider
body; a mock SSE `delta.reasoning_content` appears on WS chunks.

### Exit gate

Unity green. `mkt`/`mkp` green.

### Status

Not started.

---

## Phase 3 — REST Pathway

### Goal

REST is complete per the Phase 0 matrix. No 501-as-success. No SSE 200
that is only an error event unless SSE is real.

### Entry gate

Phase 2 Status complete.

### Work items

- [ ] Implement the exact error contract locked in Phase 0:
      - Non-stream REST: `stream:true` → **400** (not the current 501)
        with the Phase-0-locked message pointing at WS. Unregister stub
        `/auth_chat/stream` (`auth_stream.c`).
      - Or real SSE: MHD incremental + `chat_proxy_multi_*`; then
        `stream:true` works.
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

Test 59 REST matches the matrix. Stub SSE gone.

### Exit gate

Test 59 REST green. `mkt`/`mkp` green.

### Status

Not started.

---

## Phase 4 — WebSocket Pathway

### Goal

WS `"chat"` matches the matrix: knobs, stream, non-stream, JWT.

### Entry gate

Phase 3 Status complete.

### Work items

- [ ] Verify WS builds the same provider JSON as REST for temperature
      and reasoning.
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

Not started.

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

Not started.

---

## Phase 6 — Host API: Authenticated System Info

### Goal

Lua can obtain the **same JSON** as JWT `GET /api/system/info` by calling
a host function that uses the **same C collectors**.

### Entry gate

Phase 5 Status complete.

### Work items

- [ ] This is a **relocation of existing logic, not new logic**:
      `handle_system_info_request` (`info.c`) already calls
      `get_system_status_json` and already attaches `"scripting"` from
      `scripting_scoreboard_snapshot_json(100, false)` when a valid JWT
      is present (`info.c` around lines 79-90). Pull that exact sequence
      out into `json_t *system_info_build_json(bool include_scripting)`
      (name may vary) so it becomes callable from both the REST handler
      and Lua — do not re-derive the field list from scratch.
- [ ] `handle_system_info_request` uses that helper (`include_scripting`
      iff valid JWT). Behavior of Test 21 unchanged.
- [ ] `H.system.info()` → Lua table (or JSON string +
      `H.set_result_json`) built from that helper with
      `include_scripting=true` when called from an authenticated MCP
      job (MCP always has Bearer). Document in
      [lua_api.md](/docs/H/core/subsystems/scripting/lua_api.md).
- [ ] Do **not** implement this as `H.http.get` to `/api/system/info`.
- [ ] Unity: helper with/without scripting; `H.system.info` install +
      shape (version/system/status keys); REST handler still uses helper.
- [ ] `mkt` + `mkp`. Test 21 info endpoint still green if run.

### Done means

One C function serves REST info and `H.system.info()`. Unity proves both
call it. No duplicated field lists.

### Exit gate

Named Unity green. `mkt`/`mkp` green.

### Status

Not started.

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

Not started.

---

## Phase 8 — Grok Calls MCP With The User JWT

### Goal

A chat completion through Hydrogen can result in Grok calling the
**public** MCP URL with a **user-scoped** JWT. Chat pod and MCP pod may
differ. First useful call: `System.Info`.

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

- [ ] Implement Phase 0 MCP option (default A): append the remote MCP
      connector to the **provider** body (not client chat JSON): `type:
      mcp`, `server_url` = public `MCP.Resource`, `authorization` =
      `"Bearer <minted jwt>"`, `allowed_tools` = `["System.Info"]`.
      Client JSON has no `tools`.
- [ ] **JWT is a credential, not a session.** Every MCP request carries
      the minted JWT; `Mcp-Session-Id` is optional protocol state and is
      **not** required for `System.Info`. Hydrogen already creates a
      session on demand when the header is absent (`mcp_http.c:221-226`),
      so Grok — which does not need conversation continuity — is
      unaffected by replica skew. Do not pin `Mcp-Session-Id` for Grok.
- [ ] **Provider-side tool loop.** xAI emits tool-loop output only if MCP
      call output is opted into; Hydrogen's chat path is built around
      chat deltas, not server-side tool orchestration (especially on
      `/v1/responses`). Account for the field shape locked in Phase 0
      when parsing the provider response.
- [ ] Document plainly (this doc + Phase 9) that the token is held by xAI
      for the request duration and cannot be revoked — short TTL is the
      only real control.
- [ ] Blackbox: mock LLM captures outbound body (option A); independent
      MCP POST with that JWT runs `System.Info` (other-instance
      simulation); a second POST with a mismatched `aud` is rejected.
- [ ] **Observability: trace the token (Goal 8).** The mint log and the MCP
      tool invocation log both carry the Phase 0 correlation id, so
      chat → mint → MCP is traceable across pods (logs, not a shared
      store). xAI is in the middle and you do not own it; you must still
      be able to tell whether a tool call happened.
- [ ] Test 59 + Test 47 green (incl. new `aud`-rejection case). `mkt` +
      `mkp`.

### Done means

Minted, `aud`-restricted, short-TTL token authorizes MCP `System.Info`
without client `tools` in chat JSON; the token is rejected by chat/REST
on purpose; MCP rejects a mismatched `aud` (proving the prerequisite fix
is real, not assumed); `Mcp-Session-Id` is not required for the Grok path.

### Exit gate

Test 59 + Test 47 green (incl. aud-rejection). Option recorded as
implemented.

### Status

Not started.

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

Not started.

---

## Phase 10 — Parked (Not A Gate)

Pull forward only if product asks. Finale is complete without these.

| Item | Notes |
| --- | --- |
| WS `media_chunk` | Old TODO 14b |
| `alt_chat` / public `chat` | Never implemented |
| REST SSE | Only if Phase 0 chose non-stream and we later reverse |
| P13 cache / LB / fallback / analytics / templates / convo CRUD / cost / A/B | Wishlist |
| MCP DCR / stdio | MCP 16–17 |
| MCP live Keycloak blackbox | C already accepts RP when enabled |
| Shared MCP session store | Only needed for clients that **pin** a session
  (`Mcp-Session-Id`). Grok doesn't — it sends a JWT every call and Hydrogen
  creates a session on demand when the header is absent (`mcp_http.c:221-226`),
  so replica skew is harmless for the chat→MCP path. Out of scope for this
  finale. |
| Product Lua tools beyond System.Info | Helium |
| Chat rate limiting | No per-user/per-connection throttle today (REST or WS),
  only a global connection cap. Login, mailrelay, and conduit already have
  `rate_limit` precedent to copy. **Phase 0 records a one-line cost-cap
  story; a crude per-user `rate_limit` copy belongs before first student
  use**, not before "public non-500-Courses deploy" — flag to product. Out
  of scope for code in this finale. |
| Responses API data residency | If Phase 0 spike locks `/v1/responses`,
  xAI default `store=true` retains 30-day request/response. Non-Goal;
  Phase 0 records the decision to disable `store` (or accept). |

---

## Completion Criteria

1. Phases 0–9 Status complete, Exit gates green in order.
2. Support matrix true in Test 59.
3. 500 Courses can use REST **or** WS against Grok with temperature,
   reasoning, and the chosen stream policy.
4. Grok can call public MCP with user JWT; `System.Info` returns JWT
   `/api/system/info` JSON via shared C.
5. No chat functions in `dead_functions.txt`.
6. Phase 10 is parked, not silent stubs.
7. MCP `aud` check is real (`mcp_try_hydrogen` fixed); Phase 8's token
   handoff to xAI is a minted, short-TTL, scope-limited token rejected by
   chat/REST on purpose — not the full account JWT.
8. **Observability gate.** Phase 0 correlation id threads chat → mint →
   MCP logs; storage-failure logs carry it (no silent "not found") and
   the mint/MCP logs prove a tool call traceable across pods.

---

## Completion is a beachhead

This plan ships a working 500 Courses chat+MCP surface. It is **not**
the final platform for arbitrary future clients/tools — that is the
sequel, with its own token/scoping review. Completion here means "trust
this in production for the known one-tool case," not "hand the keys to
the next feature."

---

## Verification Commands

```bash
zsh -ic 'mkt'
zsh -ic 'mkp'
zsh -ic 'mku <phase-named-unity>'

# Blackbox from hydrogen tests/
./test_59_auth_chat.sh
./test_47_mcp.sh
./test_21_system_endpoints.sh   # after Phase 6

# Dead code after mkt
# elements/001-hydrogen/hydrogen/build/deadcode/dead_functions.txt
```

---

## Working Log

### 2026-08-30 — Plan created; phases gated; System.Info added

- Consolidated TODO 14/14a/14b, Phase 13, stale Phase 3/4/11 claims, MCP
  leftovers, dead-code pipeline, temperature hardcode, REST 501.
- MCP 0–15 remains done. Finale MCP work: shared status collectors,
  Lua `System.Info`, user JWT handoff, DOKS.
- Next: Phase 0 only.

### 2026-08-31 — Independent code review; robustness gaps folded in

- Verified all "Current Observed State" claims against source; all held
  up. Tightened Phase 1 (Anthropic has no temperature field at all, not
  just hardcoded), Phase 5 (pinned exact dead-function list + added
  silent-logging fixes), Phase 6 (clarified it's extracting existing
  `info.c` logic, not writing new logic), Phase 7 (seed-ID recheck is now
  a checkbox), Phase 0 (added stream error-contract lock + chat JWT
  policy decision).
- New findings folded into Phase 3/4: WS chat never resolves `media:`
  refs (REST does); `context_hashing` stats only reported on non-stream
  REST; chat JWT `aud`/roles parsed but never checked; multi_curl
  streaming timeouts are hardcoded literals bypassing config, with no
  idle/low-speed detection; `chat_proxy_multi_stream_stop` leaks the CURL
  handle/buffers/context on every client disconnect mid-stream (real bug,
  not just a finale nicety — the write-callback abort path already does
  this correctly and is being defeated by an explicit
  `curl_multi_remove_handle()` call).
- Chat rate limiting (none exists) parked in Phase 10 with an explicit
  flag to revisit before any public, non-500-Courses chat deploy.
- Added "Implementor Workflow" section: one phase per conversation,
  confirm prior phase done, discuss/research before implementing, get
  explicit approval to start, ask questions as they arise, log major
  milestones and lessons learned, mark complete only after clean
  verification, never apply DB migrations (hand packets to the user),
  follow existing Unity/blackbox/tooling norms.
- Next: Phase 0 only.

### 2026-08-31 (later) — Security & Safety review; token-handoff risk found

- Focused second pass specifically on safety/security beyond
  infrastructure hardening, since Phase 8 hands tool-calling authority
  to a third-party model. Added a dedicated "Security & Safety" section.
- Real finding: `mcp_try_hydrogen` never checks `aud`
  (`mcp_auth.c:253-284`) — only the OIDC IdP/RP MCP paths do. This
  undermines the `aud=MCP.Resource` scoping Phase 8 was going to rely on.
  Made it a blocking prerequisite of Phase 8, not a footnote.
- Corrected a factual error in the plan itself: `H.system_token` /
  `sub=hydrogen-scripting` does **not exist** anywhere in `src/` (grepped
  and confirmed absent) — the "do not use it" guidance implied prior art
  that isn't there. Phase 8 must build the minting primitive from
  scratch: short-TTL, `aud`-restricted, minimal-role token — not a copy
  of the full 1-hour account JWT.
- Confirmed no per-tool MCP authorization exists (any accepted JWT can
  call any `mcp_access=1` tool) and no isolation between trusted
  system/user content and untrusted tool-result content in the
  provider `messages` array — both documented as accepted risk today,
  bounded by `System.Info` taking no arguments.
- Confirmed `H.http.get`/`H.query` have no allowlist (SSRF / arbitrary
  SQL from any script capable of reaching them) — added an explicit
  safety-bar work item to Phase 7 requiring future MCP tools not pass
  caller/model-influenced values into either without an allowlist.
- Confirmed no encryption at rest and no retention/deletion for stored
  chat history, and no LLM output sanitization. Both added as explicit,
  named Non-Goals (flagged for a product/compliance decision) rather
  than left silently unaddressed.
- Next: Phase 0 only.

### 2026-08-31 (review follow-up) — Endpoint, token, and session contract review

- **Biggest blast radius:** xAI documents Remote MCP on the native SDK /
  **Responses API**, not Chat Completions. Hydrogen's CEC path is
  OpenAI-compatible (`/v1/chat/completions`). Added a Phase 0 spike
  requirement: `curl` the live Grok CEC endpoint with a dummy
  `type:mcp` tool to confirm which endpoint honors MCP **before**
  Phases 1–4 write Unity asserting Chat Completions knobs. If it only
  works on `/v1/responses`, the streaming/reasoning/`max_tokens` meaning
  of Phases 1–3 all move. Locked the streaming error contract as
  **400 + stable JSON, use WS** (no 200-with-one-error-EVENT body).
- **Token = credential, not session.** Verified in source: a lone
  authenticated `tools/call` already succeeds sessionless —
  `mcp_http.c:221-226` sets `allow_create = (session_hdr == NULL || …)`
  and `mcp_session_resolve` creates a session on demand when
  `Mcp-Session-Id` is absent. Grok does not need session continuity, so
  `Mcp-Session-Id` pinning is **not** required for the Grok path;
  `replicas=1`/sticky ingress is only for pinning clients. Phase 9 DOKS
  rewritten accordingly.
- **Hardened Phase 7 safety bar** to "MCP-exposed scripts may not call
  `H.http.get`/`H.query` at all" (no allowlist exception — allowlists
  rot), with future data-access tools parked behind a Phase 10 security
  review. `System.Info` complies by construction.
- **Split Phase 8** into 8a (mint + verify the narrow token in Unity —
  done *before* 8b) and 8b (wire the xAI MCP connector). The minting
  primitive does not exist today; bundling it with the connector would
  merge two security projects. Minted token is now documented as
  **treated as already leaked to xAI** on departure (short TTL + a role/
  `aud` chat/REST reject on purpose are the only controls).
- **Reinforced Phase 0 locks:** `authorization` = `"Bearer <jwt>"`
  (prefix required), `allowed_tools` = `["System.Info"]` (never empty),
  fail-closed on localhost **and** internal ClusterIP xAI can't reach,
  A↔B shared-key/clock/`sub` deployment assumption, and a one-line 500-
  Courses cost-cap story (code may stay Phase 10).
- **Process:** flagged Phase 5 as hygiene-only, not a merge blocker for
  Phases 6–8 (deleting Unity-only `chat_proxy_init` while MCP is vapor
  is self-inflicted).
- Next: Phase 0 only.

### 2026-08-31 (verdict) — Beachhead scope + observability gate

- **Scoped the plan to a beachhead.** Rewrote Purpose as "ship Hydrogen
  chat for 500 Courses" — dual REST+WS, real knobs, sessionless MCP, one
  read-only tool, minted narrow JWT, zero dead chat functions. Explicitly
  *not* the "every later client" platform: a second MCP tool that can
  write/query or a non-500-Courses client is a sequel with a different
  security review. The implementation should survive first contact because
  the tool surface stays tiny and the resource/cost bugs already have line
  numbers.
- **Made observability a gate (Goal 8).** Added a Phase 0 lock for one
  opaque correlation id threading chat → mint → MCP **logs** (not a shared
  store), so "Grok didn't call MCP" is distinguishable from "MCP call
  failed" and the silent storage `return false` paths are traceable.
  Phase 5 storage-failure logs now must carry that id; Phase 8b mint/MCP
  logs carry it too. The plan's admission of lying 501s, the real CURL
  leak, hardcoded 10/600s, and decorative `aud` are now the things that
  get fixed before dead-function purity matters.
- **Reaffirmed the four production contracts** Phase 0 must lock: spike
  Responses vs Chat Completions on the live endpoint; `authorization` is
  `Bearer <jwt>` + `allowed_tools=["System.Info"]` (never empty); the
  minted token is useless for chat/REST on purpose; crude per-user
  rate/l cost cap lands before first student. With those, "trust this in
  production for 500 Courses" holds — "every later client" does not.
- Next: Phase 0 only.
