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
| 10b — Chat rate limiting | **Not started** | Per-user fairness, before first student use. |
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

Not started.

#### Open design questions (resolve before implementation)

1. **Token bucket vs sliding window.** Login, mailrelay, and conduit
   already have a `rate_limit` precedent. Confirm the existing shape (window,
   max-requests, key derivation) — do not invent a second scheme that
   diverges from the precedent.
2. **Per-sub vs per-`sub+database` vs per-IP-fallback.** A 500 Courses
   student has one `sub`. A future non-500-Courses client may have many
   `sub`s behind one egress IP. Per-`sub` is the simplest correct answer
   for the beachhead; decide whether IP-fallback matters before first
   student use.
3. **WS shape.** WS connection has one `sub` at connect time; per-message
   `payload.jwt` may rotate `sub` mid-session. Decide: rate-limit per
   `sub` seen during the connection lifetime, or per the connect-time
   `sub` only? The connect-time answer is simpler and correct for 500
   Courses (single-tenant JWT); the per-seen-sub answer is correct for
   any future multi-tenant chat.
4. **Cost cap interaction.** Phase 0 recorded a one-line cost-cap story
   (`max_output_tokens` ceiling, enforcement may stay Phase 10). Rate
   limiting is request-count-based; cost cap is token-based. They are
   complementary, not redundant. Decide whether 10b covers request
   count only and Phase 10c covers token budget, or whether 10b bundles
   both.
5. **Failure mode.** What does a throttled user see? 429 on REST,
   `chat_error` on WS with the same error code? Confirm the error
   envelope matches the existing 401/403/JWT envelope from Phase 3.
6. **Configuration.** Where does the per-user limit live in
   `hydrogen.json`? Under `WebServer.RateLimit` or a new top-level
   `Chat.RateLimit`? Match the existing precedent.
7. **Test isolation.** Test 59 today is single-user; multi-user rate
   limiting needs a fixture. Do not break Test 59 with the new check;
   add a separate Test 60-style subtest if needed (or a new test file
   numbered in the next available 6x slot).
8. **Edge rate limiting (Traefik).** Operator-side Traefik
   `rateLimit` middleware sees IP, not `sub`. Useful as a coarse outer
   envelope (cheap, catches NAT'd abuse) but does not replace in-app
   per-`sub` fairness. Document the two layers in `DEPLOYMENT.md` after
   10b lands; do not conflate.

#### Pre-work research checklist (before starting)

- [ ] Grep the existing `rate_limit` precedent in login, mailrelay,
      conduit. Find the JSON config shape, the C enforcement site, the
      Unity test pattern, and the blackbox test pattern. Confirm
      `tests/.static-baseline.txt` does not already whitelist
      `chat_rate_limit_*` static helpers.
- [ ] Confirm `auth_chat.c` / `auth_chats.c` / `websocket_server_chat.c`
      have a single chokepoint (after JWT validation, before provider call)
      where rate-limit checks can be inserted without rewiring the
      request lifecycle.
- [ ] Confirm `CHAT_JWT_TOKEN` mint path in Test 59 can mint tokens for
      multiple `sub`s; otherwise extend the test helper.
- [ ] Decide on the answers above and lock them in the phase's Status
      block before writing code.

### Exit gate

Named Unity green. Test 59 (or its successor) green for single-user
behavior unchanged + new multi-user rate-limit subtest green. `mkt`/`mkp`
green. Behavior documented in `auth_chat.md`, `auth_chats.md`, and
`websocket_chat.md`.

---

## Working Log (Phase 10 only)

See [Phase 10a Working Log](#working-log-1) above for the entries that
moved from the archive. This section is for new entries made against
Phase 10 work going forward.