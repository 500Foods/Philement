<!-- markdownlint-disable MD007 MD024 -->
# Chat Finale

## Purpose

One gated plan to finish Hydrogen chat for **500 Courses** and every later
client: dual REST + WebSocket paths, real provider knobs, independent MCP
with user credentials, and no leftover dead chat functions.

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
2. Session with **Grok** (xAI, OpenAI-compatible CEC).
3. Questions return with **streaming** (per matrix), **temperature**,
   **reasoning**, `max_tokens`, and other obvious provider knobs.
4. Grok can answer by calling **Hydrogen MCP** on the **public** MCP
   URL, possibly a **different instance** than chat (DOKS). Intended.
5. An MCP **Lua tool** returns the same JSON as authenticated
   `GET /api/system/info`, by **reusing** the C collectors — not by
   HTTP-calling the REST endpoint, not by a second implementation.
6. Shared parse/build/proxy where sharing is real. Published support matrix.
7. Chat-related `mkt` dead functions driven to **zero**. Do not baseline them.

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
  client by design; stated as a contract, not implemented here.

---

## Security & Safety

This finale hands an autonomous third-party model (Grok) the ability to
call tools on Hydrogen's behalf. That is a materially different risk
than "chat is a REST/WS feature" — it is reviewed here as its own
category, separate from routine infrastructure hardening (timeouts,
leaks, rate limiting), which is tracked inline in the phases above.

| Risk | Current reality | Handled by |
| --- | --- | --- |
| Token handoff to xAI | Phase 8 puts a Hydrogen JWT in the provider body; xAI holds and replays it to public MCP. No infra exists today to mint a narrow, short-TTL, tool-scoped token — only full-account JWTs (1h, full `roles`) or OIDC-client tokens exist | Phase 8 (new work item: build minimal minting) |
| MCP `aud` bypass | `mcp_try_hydrogen` (`mcp_auth.c:253-284`) never calls `mcp_auth_aud_contains` — only the OIDC IdP/RP paths check `aud`. Any Hydrogen JWT for any audience is accepted by MCP today, which undermines the `aud=MCP.Resource` scoping Phase 8 depends on | Phase 8 entry gate (blocking prerequisite fix, filed against MCP core even though MCP 0-15 is otherwise "done") |
| No per-tool MCP authorization | Any JWT that clears MCP auth can invoke any `mcp_access=1` tool; scope/role enforcement, if any, is left to each Lua script | Phase 7/8 (policy statement; no framework fix in this finale) |
| No revocation | Hydrogen JWTs and OIDC access tokens cannot be revoked in real time (`oidc_service_revoke.c` only revokes refresh tokens); security is TTL-only | Documented risk; short TTL on the Phase 8 minted token is the only mitigation available |
| Indirect prompt injection | Tool-role and user-role content are inserted into the provider `messages` array identically (`req_builder.c:120-121`) — no framing marks tool output as data rather than instructions | Documented risk; no generic fix. Kept low today because `System.Info` takes no arguments and returns fixed-shape internal data |
| SSRF / arbitrary SQL via scripting | `H.http.get`/`H.query` take caller-influenced URLs/SQL with no allowlist; `mcp_access` is a load-time gate, not a capability sandbox | Phase 7 policy: no MCP-exposed script may pass model- or caller-influenced values into `H.http.get`/`H.query` without an explicit allowlist. `System.Info` complies (no args, internal collectors only) |
| Chat data at rest | Conversation content is stored compressed but **not encrypted**; no retention/deletion path exists | Non-Goal (flagged for product/compliance decision, not solved here) |
| LLM output handling | No sanitization of model output before storage/return | Non-Goal (client's responsibility, stated as contract) |

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

---

## Grok MCP Credentials (lock option in Phase 0, implement Phase 8)

xAI Remote MCP Tools (provider body, **not** client chat JSON):

```json
{
  "type": "mcp",
  "server_url": "https://<public>/mcp",
  "server_label": "hydrogen",
  "authorization": "<user-scoped JWT>"
}
```

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
| 0 | Decisions written in Status; no C |
| 1 | Provider JSON uses client/engine temperature, not `1.0` |
| 2 | Reasoning knobs round-trip; inbound chunks consistent |
| 3 | REST matches matrix; no fake 501/200 SSE |
| 4 | WS matches matrix |
| 5 | Zero chat names in `dead_functions.txt` |
| 6 | `H.system.info()` === JWT `/api/system/info` collectors |
| 7 | MCP tool `System.Info` callable; Test 47 proves it |
| 8 | Grok (or mock) calls public MCP with user JWT |
| 9 | Docs and DOKS notes match code |
| 10 | Parked extras — not a gate |

---

## Phase 0 — Contract Lock

### Goal

Write the decisions later phases implement. No C.

### Entry gate

This document exists.

### Work items

- [ ] Fill Support Matrix **Decision** column.
- [ ] Choose REST streaming: request/response **or** real SSE via multi_curl.
- [ ] Lock the exact error contract for the losing choice (status code +
      JSON error shape) so the 500 Courses client can code against it
      before Phase 3 lands — not just "a stable message."
- [ ] Choose Grok MCP option A / B / C (default A).
- [ ] Name 500 Courses Grok engine in CEC and the public MCP URL shape.
- [ ] Decide chat JWT policy: does chat require a specific `aud` and/or
      role beyond a non-empty `database` claim (today it does not check
      either)? Record the decision even if it is "no change."
- [ ] Decide the Phase 8 token-handoff shape: what exactly gets minted
      and sent to xAI as `authorization` (claims, TTL, which tools it
      authorizes). Do not defer this to Phase 8 as an implementation
      detail — it's a credential-exfiltration surface to a third party
      and needs an explicit decision now. See Security & Safety.
- [ ] Record that Phase 8 has a blocking prerequisite: `mcp_try_hydrogen`
      (`mcp_auth.c:253-284`) does not check `aud`, so `aud=MCP.Resource`
      scoping is not real yet. Either fix this before Phase 8 or record
      why it's acceptable to proceed without it (it is not recommended
      to proceed without it).
- [ ] Run `zsh -ic 'mkt'` once to (re)generate
      `build/deadcode/dead_functions.txt` (it is not persisted between
      checkouts), then snapshot current chat names into Working Log (do
      not baseline).

### Done means

Status block records matrix, stream policy, stream error contract, MCP
option, engine name, chat JWT policy.

### Exit gate

Status complete. Review stop.

### Status

Not started.

---

## Phase 1 — Temperature Reaches The Provider

### Goal

`ChatRequestParams.temperature` is what OpenAI-compatible and Ollama
builders emit. Stop hardcoding `1.0`.

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
      with no log line, indistinguishable from a normal "not found."
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
      Security & Safety): no argument, and no value derived from the
      caller/model, may reach `H.http.get`/`H.query`/`H.altquery`
      without an explicit allowlist. `System.Info` complies by
      construction (no arguments, calls only internal C collectors) —
      confirm this stays true in code review, not just by intent.
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

Phase 7 Status complete. Test 47 still green.

### Blocking prerequisite

Fix `mcp_try_hydrogen` (`mcp_auth.c:253-284`) to call
`mcp_auth_aud_contains` like the OIDC IdP/RP paths already do. Without
this, `aud=MCP.Resource` scoping on the minted token below is
decorative — MCP currently accepts a Hydrogen JWT for any audience. Land
and verify this fix (Unity + Test 47) before the rest of this phase.

### Work items

- [ ] Land the `aud` check fix above; confirm Test 47 still green and
      add a case proving a JWT minted for a different `aud` is rejected.
- [ ] Implement the Phase 0 MCP option and the Phase 0 token-handoff
      decision.
- [ ] If **A**: build the minting primitive first (it does not exist —
      see Security & Safety correction on `H.system_token`). Mint a
      **short-TTL** (minutes, not the standard 1h), `aud=MCP.Resource`
      token carrying only the chat user's `sub`/`database` and the
      minimum roles needed for the tools this chat session is allowed to
      reach — not a verbatim copy of the full account JWT's `roles`.
      Then append the remote MCP connector to the **provider** body:
      `server_url` = public `MCP.Resource`, `authorization` = that
      minted token. Client JSON has no `tools`.
- [ ] Document plainly (in this doc and in Phase 9's docs) that this
      token is held by xAI for the duration of the request and cannot be
      revoked once issued — short TTL is the only real control.
- [ ] Fail closed if `MCP.Resource` is localhost in production-shaped
      configs.
- [ ] Blackbox: mock LLM captures outbound body (option A); independent
      MCP POST with that JWT runs `System.Info` (other-instance
      simulation); a second POST with a mismatched `aud` is rejected.
- [ ] Test 47 unharmed. `mkt` + `mkp` + Test 59 + Test 47.

### Done means

Minted, `aud`-restricted, short-TTL token authorizes MCP `System.Info`
without client `tools` in chat JSON, and MCP actually rejects a
mismatched `aud` (proving the prerequisite fix is real, not assumed).

### Exit gate

Test 59 + Test 47 green (including the new `aud`-rejection case).
Option recorded as implemented.

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
      Ingress, **session affinity** on `Mcp-Session-Id` (or replicas=1).
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
| Shared MCP session store | Only if MCP replicas > 1 without sticky sessions |
| Product Lua tools beyond System.Info | Helium |
| Chat rate limiting | No per-user/per-connection throttle today (REST or WS), only a global connection cap. Login, mailrelay, and conduit already have `rate_limit` precedent to copy. Flag to product before first public (non-500-Courses) chat deploy — it is a real DoS/cost exposure, just out of scope for this finale |

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
   handoff to xAI is a minted, short-TTL, scope-limited token — not the
   full account JWT.

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
