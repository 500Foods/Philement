<!-- markdownlint-disable MD007 MD024 -->
# Chat Finale — Pulled-Forward Phase 10

## Status

**Active (2026-09-03).** This is the **active** plan for chat work that
the original CHAT_FINALE.md left parked or pulled forward after the
500 Courses beachhead shipped. The beachhead itself (Phases 0–9) is
archived at
[`CHAT_FINALE_BEACHHEAD_COMPLETE.md`](/docs/H/plans/complete/CHAT_FINALE_BEACHHEAD_COMPLETE.md).
Read that file for the implementation history; read this file for what
is still open.

| Phase | Status | Notes |
| --- | --- | --- |
| 10a — Responses API `store` knob | **Complete (2026-09-03)** | Per-engine JSON, default `false` (opt-in to provider retention). Verified end-to-end. |
| 10b — Chat rate limiting | **Complete (2026-09-03)** | Per-sub fixed-window: REST 429 + WS `chat_error` with `error_code` 4291/4292. Bundles request count and estimated token budget. Config: `Chat.RateLimit.*` (disabled by default). |
| Original parked list | Parked | See table below. Not in scope unless product asks. |

## How To Use This Document

Same rules as the beachhead archive (one phase per conversation, confirm
prior phase done, discuss before implementing, get explicit approval to
start, log major milestones and lessons learned, mark complete only after
clean verification, never apply a DB migration). The
[Implementor Workflow](/docs/H/plans/complete/CHAT_FINALE_BEACHHEAD_COMPLETE.md#implementor-workflow-every-phase)
section is in the beachhead archive. The
[How To Use This Document](/docs/H/plans/complete/CHAT_FINALE_BEACHHEAD_COMPLETE.md#how-to-use-this-document)
section is also there. Do not duplicate them; follow them.

## Priority

| | |
| --- | --- |
| **Band** | P0 — [TODO.md item 2](/docs/H/TODO.md) (continuation) |
| **Effort** | S per phase; chat rate limiting is M |
| **First deploy** | 500 Courses (beachhead live; rate limiting is the pre-student-use gate) |

---

## Original Parked List (still parked)

Pull forward only if product asks. The beachhead is shippable without
any of these. Phase 10a (above) was pulled forward because the plan
itself flagged the data-residency default as pre-student-use. Phase 10b
(chat rate limiting) is the second such item. Everything below stays
parked unless reclassified.

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

Phase 9 Status complete (archived).

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

#### Working Log

- **2026-09-03 — Phase 10 created (pulled-forward from parked list).**
  Two items moved out of "parked" because the plan itself flagged them
  as pre-student-use requirements: `store` knob (data residency default)
  and chat rate limiting. Other parked rows remain parked. 10a
  designed: `store` is a per-engine JSON config attribute parsed in
  `chat_engine_cache_load_from_database`, emitted by
  `chat_request_build_responses`, default `false` (opt-in to retention).
  Behavior change for existing deployments: Responses requests will
  stop carrying xAI's 30-day-retention default. Operators must add
  `"store": true` to opt back in. 10b (rate limiting) discussion
  follows after 10a lands.
- **2026-09-03 — Phase 10a complete: Responses `store` knob shipped.**
  `ChatEngineConfig.store` (default false), parsed from engine JSON in
  `chat_engine_cache_load_from_database`, emitted unconditionally on
  Responses bodies by `chat_request_build_responses`. New Unity file
  `req_builder_test_responses_store.c` (6 tests, all green) plus
  regression green on `req_builder_test_temperature` (19),
  `req_builder_test_local_mcp` (4), `req_builder_test_hosted_mcp` (12).
  `mkt`/`mkp`/`mks`/`mkl` all clean. Behavior change called out in
  `RELEASES.md` (September 2026 section) and in `auth_chat.md`
  Configuration section. Decision chosen: opt-in to retention
  (default `store=false`) — fails closed for privacy.
- **2026-09-03 — Phase 10b design locked.** Follow-up discussion
  resolved all 8 open questions. Algorithm: fixed-window, mailrelay
  precedent (linked list of `ChatRateLimitEntry`, mutex-guarded,
  fail-open on alloc error). Key: per-`sub`. WS: per-seen-sub per
  message. Scope: **bundles** request count AND estimated token budget
  (chars/4 input + streaming `usage.completion_tokens` output; local
  providers throttled by request count only). Response: REST 429 +
  WS `chat_error`, matching conduit envelope from
  `error_handling.c:99`. Config: new top-level `Chat.RateLimit`,
  disabled by default. Tests: extend Test 59 with multi-user
  throttling subtests. Traefik edge rate limiting out of scope.
  Pre-work research checklist ticked. Implementation gated on user
  approval.
- **2026-09-03 — Phase 10b complete: per-sub rate limiting shipped.**
  New `ChatConfig { ChatRateLimitConfig RateLimit }` (config letter
  **U**) loads `Chat.RateLimit.{Enabled (false default),
  MaxRequestsPerInterval (60), IntervalSeconds (60),
  MaxTokensPerInterval (100000)}`. New module
  `src/api/wschat/helpers/chat_rate_limit.{h,c}` implements the
  fixed-window bucket list (mutex-guarded, fail-open on alloc error),
  the `chat_rate_limit_estimate_input_tokens` chars/4 walker over
  nested JSON, and the `chat_rate_limit_build_error_response`
  envelope helper (error_code 4291 = request cap, 4292 = token cap).
  Chokepoint inserts in `auth_chat.c`, `auth_chats.c`, and
  `websocket_server_chat.c` all gate the provider call on
  `chat_rate_limit_check_and_record(sub, est_input_tokens)` after
  JWT validation. Output-token recording wired into the
  streaming/non-streaming finalize paths (REST aggregate per-fanout;
  WS via new `rate_limit_sub` parameter on `send_chat_proxy_result`).
  `send_chat_error` gains an optional `error_code` (0 = omit). WS
  envelope now carries `error_code` for throttle conditions, matching
  the REST 429 envelope shape. `chat_rate_limit_shutdown` hooked into
  `cleanup_application_config` via forward declaration.

  Unity: `tests/unity/src/api/wschat/helpers/chat_rate_limit_test.c`
  (27 tests: request cap, token cap, request-precedence, multi-sub
  isolation, disabled-fails-open, null-app-config-fails-open,
  null/empty-sub-fails-open, zero-max-requests-disables-request-cap,
  zero-interval-fails-open, negative-tokens-clamped, record_output
  increment/noop/disabled/no-positive, estimate NULL/non-array/
  chars-over-4/nested-walk/UTF-8-multibyte/empty, error envelope
  shape, utf8_chars codepoint counting, find_locked miss/hit,
  new_bucket_locked fields). All 27/27 PASS.

  Blackbox: Test 59 extended with three subtests
  (`Phase 10b per-sub rate limiting`):
  1. Sub `1` makes 3 successful POSTs then gets 429 on the 4th.
  2. Sub `2` (separate `sub` mint) is unaffected by sub `1`'s
     throttle (200).
  3. After waiting `IntervalSeconds + 1`, sub `1` recovers (200).
  Plus the 429 body is verified to carry `success=false,
  error=rate_limited, error_code=4291`. `mint_chat_jwt` gained an
  optional third arg (`sub`, default `"1"`) for multi-sub minting.
  `TEST_VERSION` 1.7.2 → 1.8.0.

   `mkp` green (1,992 files, 0 issues), `mks` green (165 scripts,
   1074 directives justified), `mku chat_rate_limit_test` 27/27 PASS,
   regression on `req_builder_test_responses_store` 6/6,
   `websocket_server_chat_send_test` 5/5,
   `auth_chat_test_success_path` 9/9 green. Behavior change
   documented in `/RELEASES.md` (September 2026 section),
   `/docs/H/api/chat/auth_chat.md` (new "Per-sub Rate Limiting (Phase
   10b)" subsection + 429 row in Errors), `auth_chats.md` (link to
   `auth_chat.md` + note on per-sub vs per-broadcast),
   `/docs/H/core/subsystems/websocket/websocket_chat.md`
   (`chat_error` envelope note + two new rows in the cause table +
   Unity test list).
- **2026-09-04 — Phase 10b bug-fix pass: SIGSEGV in rate-limit
  chokepoint resolved.** Test 59 crashed (SIGSEGV, fault addr 0x8) when
  the per-sub rate limiter triggered: in both `auth_chats.c:476` and
  `auth_chat.c:498`, `free_jwt_validation_result(&jwt_result)` was
  called *before* the subsequent `log_this(..., sub, ...)` call
  dereferenced `jwt_result.claims->sub` — a use-after-free that freed
  the JWT claims struct (and set `claims = NULL`) before reading `sub`.
  Fix: hoist `const char *sub = jwt_result.claims->sub;` into a local
  variable *before* any `free_jwt_validation_result` call, then use the
  local copy in `log_this`. `websocket_server_chat.c` was already safe
  (`ws_sub` saved at line 348 before any free). Also corrected Test 59
  JWT `sub` allocation: non-rate-limit REST tests (59-0022/23/25) and
  WebSocket tests (59-0026/28) each now get a distinct `sub` token so
  they don't exhaust each other's 3-req/5s window. `TEST_VERSION` 1.8.0
  → 1.8.2. `test_59_auth_chat.sh`: 28/28 PASS. `mkt`/`mkp`/`mks` all
  green.
- **2026-09-03 — Housekeeping: `auth_chat_test_success_path` fixed.**
  Pre-existing failure on `main` (verified by stash test before any
  Phase 10a changes). Root cause: `auth_chat.c` calls the chat proxy
  through `chat_local_mcp_complete_request` (in `local_mcp.c`),
  not `chat_proxy_send_with_retry` directly. The `USE_MOCK_AUTH_CHAT_DEPS`
  rename (`chat_proxy_send_with_retry → mock_chat_proxy_send_with_retry`)
  was only applied to AUTH source files (`auth_chat.c`), so `local_mcp.c`
  kept the real symbol and made real CURL calls, which all failed with
  "Could not resolve hostname" + 3 retries + 14 retry-attempts-exhausted
  log lines per test. Result: `g_store_segment_count` never incremented
  and the success-path test failed at line 128
  (`mock_auth_chat_deps_store_segment_count() > 0`). Fix: in
  `cmake/CMakeLists-unity.cmake` WSCHAT branch, add the
  `USE_MOCK_AUTH_CHAT_DEPS` define **and** `-include mock_auth_chat_deps.h`
  to `local_mcp.c` only (mirroring the existing `helpers/health.c` /
  `USE_MOCK_CHAT_PROXY` precedent that guards against `proxy.c` being
  compiled with the rename and colliding with `mock_chat_proxy.c`).
  Same discipline: rename the call site, not the definition. Verified:
  `mku auth_chat_test_success_path` 9/9 PASS (was 8/9); all 6 auth_chat
  test files (56 tests) PASS; all 4 local_mcp tests PASS; `mkt`/`mkp`/
  `mks` clean; 309 dead functions (0 chat), no new entries.
  **Lesson:** the mock-pipeline is brittle when production code
  refactors through a wrapper — the wrapper's call site becomes
  invisible to the source-file filter that gates the mock rename.
  Worth a future audit of every AUTH-source test that mocks
  chat_proxy / chat_storage against the current call graph.
- Next: Phase 10b (chat rate limiting) is the second pulled-forward
  item, **not yet started**. 10b design discussion deferred to a
  follow-up conversation (per the plan's "one phase per conversation"
  rule and the user's preference for sequenced work).

---

## Phase 10b — Chat Rate Limiting (pre-student-use)

### Goal

Per-user throttle on REST `auth_chat` / `auth_chats` and WebSocket `chat`,
keyed off the chat JWT `sub`. Fails closed: the existing global connection
cap stays; rate limiting is an additional layer, not a replacement.

### Entry gate

Phase 10a Status complete.

### Status

**Complete (2026-09-03).** Bug-fix pass done (2026-09-04): SIGSEGV
use-after-free in the rate-limit chokepoint resolved in `auth_chats.c`
and `auth_chat.c`; Test 59 JWT `sub` allocation corrected to avoid
rate-limit collisions between non-rate-limit subtests; 28/28 PASS;
`mkt`/`mkp`/`mks` all green.

#### Locked design decisions

1. **Algorithm.** Fixed-window per-key bucket, mailrelay precedent
   (`mailrelay_event_check_rate_limit`). Linked list of
   `ChatRateLimitEntry { const char* sub; time_t window_start;
   size_t request_count; size_t token_count; ChatRateLimitEntry* next }`.
   Mutex-guarded. Fail-open on allocation error (do not block legitimate
   users on internal fault — same posture as mailrelay).
2. **Key.** Per-`sub` only, pulled from `claims->sub` after JWT
   validation. No IP fallback, no `sub+database`.
3. **WS shape.** Per-seen-sub per message. Lookup-or-insert bucket
   keyed by the inbound message's JWT `sub` (whether connect-time or
   per-message). Same data structure as REST; no special-casing. This
   defends against mid-session `payload.jwt` rotation bypassing the
   limit.
4. **Scope.** **Both** request count AND estimated token budget, in
   one window. Two counters per bucket. Rationale: pre-student-use
   gate; cost control and fairness belong together.
5. **Token measurement.** Estimated tokens. Input: chars/4 heuristic on
   the concatenated message content of the request. Output: read
   `usage.completion_tokens` from the streaming `chat_done` chunk; if
   absent (non-streaming or local provider), record 0 for that request.
   Local models (Ollama) get throttled by request count alone, not
   token budget.
6. **Throttle response.** REST returns 429 with
   `{ "error_code": <integer>, "error": "rate_limited", "message":
   "<human>" }` matching the conduit envelope from
   `error_handling.c:99`. WS returns `chat_error` payload with the same
   code/message. Audit existing WS error codes before picking the
   integer; the new value must be distinct from existing 401/403/JWT
   codes.
7. **Configuration.** New top-level `Chat.RateLimit` in `hydrogen.json`,
   disabled by default (fails open):

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

   `config_defaults.c` provides the defaults. Documented in
   `auth_chat.md` Configuration section. Matches the MailRelay.Events
   precedent (subsystem owns its rate config; not co-mingled with
   HTTP-level rate limiting).
8. **Test isolation.** **Extend Test 59** with multi-user throttling
   subtests. Two-sub scenario: exhaust `Acuranzo`'s bucket, verify a
   second `sub` is unaffected, verify `Acuranzo` recovers after the
   window. Existing single-user subtests stay green. Traefik edge
   rate limiting is out of scope and not documented in
   `DEPLOYMENT.md`.

#### Pre-work research checklist (before starting)

- [x] Grep the existing `rate_limit` precedent in login, mailrelay,
      conduit. **MailRelay chosen** as the precedent (per-event-key,
      fixed-window, mutex-guarded linked list, fail-open on allocation
      error). JSON config shape: top-level subsystem-owned
      (`MailRelay.Events.*`); adopted as `Chat.RateLimit.*`. C
      enforcement site: `mailrelay_event_check_rate_limit` in
      `src/mailrelay/mailrelay_events.c`. `tests/.static-baseline.txt`
      not yet checked; will verify before writing any `static`
      helpers (none expected — bucket type and functions will be
      non-`static` per project convention).
- [x] Confirm `auth_chat.c` / `auth_chats.c` / `websocket_server_chat.c`
      have a single chokepoint (after JWT validation, before provider call)
      where rate-limit checks can be inserted without rewiring the
      request lifecycle. **Confirmed** at lines 488, 464, 337
      respectively (each already passes `claims->sub` through to the
      chat storage/proxy layer — the rate-limit check slots in
      immediately before that handoff).
- [x] Confirm `CHAT_JWT_TOKEN` mint path in Test 59 can mint tokens for
      multiple `sub`s; otherwise extend the test helper. **Confirmed**:
      `mint_chat_jwt "Acuranzo" "wrong-role"` already accepts a
      second argument that controls role/claims; multi-`sub` minting
      works as-is.
- [x] Decide on the answers above and lock them in the phase's Status
      block before writing code. **Locked above (Decisions 1-8).**

### Exit gate

Named Unity green. Test 59 (or its successor) green for single-user
behavior unchanged + new multi-user rate-limit subtest green. `mkt`/`mkp`
green. Behavior documented in `auth_chat.md`, `auth_chats.md`, and
`websocket_chat.md`.
