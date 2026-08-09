# LUA_CLIENT — Client-Facing Script Invoke Plan

## Purpose

Close the gap left by [LUA_PLAN_COMPLETE.md](/docs/H/plans/complete/LUA_PLAN_COMPLETE.md):
Hydrogen can run Lua jobs with `H.http`, `H.query`, scoreboard, and Orchestrator,
but **clients cannot invoke a named script over HTTP the way they invoke SQL
through Conduit**.

This plan delivers a **conduit-style script surface**: the browser or SPA posts
a script name + JSON params (with JWT when required); Hydrogen loads trusted
DB-backed Lua, runs it on a worker, and returns status + payload. No Canvas-,
Stripe-, or product-named C endpoints. External systems stay tenant Lua +
`H.http`, mirroring Conduit’s indifference to SQL content.

Primary consumer today: 500 Courses Reception
(`FL-49b` free-enroll, future catalog/proxy paths). Design is product-agnostic.

## How To Use This Document

- Work **one phase at a time**, top to bottom.
- Each phase has: **Goal**, **Dependencies**, **Entry gate**, **Work items**,
  **Exit gate / validation**, **Status**, **Lessons learned**.
- Mark work items `[x]` only when verification actually passed.
- Defer with `[~]` plus one-line rationale and target phase.
- After each phase: fill **Status**, append reusable discoveries to
  **Working Log**, then stop for review before the next phase.
- Build aliases: `zsh -ic 'mkt'`, `mku <base>`, `mkp`, `mka` (C); `mks` (shell).
  See [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md).

### Testing policy (Unity + blackbox)

| Layer | When | What |
|-------|------|------|
| **Unity** | Every engine/API phase (1–8, etc.) | Decision paths, seams, no live multi-engine DB unless already mocked |
| **Blackbox** | Phase 9 (required for plan Done) | Real hydrogen + JWT + POST/GET invoke + fixture script; docs in `tests/test_XX_*.md` |
| **Both required** | Definition of Done | No “Unity only” ship for client-facing REST |

Phases must not skip Unity when adding C; Phase 9 must not be waived without explicit Status variance.

## Resuming Work

**CURRENT PAUSE POINT (as of 2026-08-08):** Phases **0–9 complete**. Next:
**Phase 10** — Documentation + Reception handoff.

### Session checklist

1. Read **CURRENT PAUSE POINT** and last **Working Log** entries.
2. Confirm previous phase **Status** is complete.
3. Re-read next phase Goal + Exit gate only.
4. Implement → verify gates → update this doc → stop.

---

## Goals

1. **Client invoke** — Authenticated (and optional public allowlist) REST to run
   a named DB script and optionally wait for completion.
2. **Conduit parity** — Same mental model as `/api/conduit/auth_query`: package
   request in, package result out; C does not interpret business logic.
3. **Return a real body** — Not only `H.set_result(type, location)` metadata;
   callers need JSON (or text) suitable for SPA `fetch`.
4. **Trust model** — Scripts are DB-sourced and operator-controlled; clients
   choose among allowlisted names, not arbitrary code upload.
5. **No WebSocket requirement** for v1 — HTTP is enough; WS progress is optional
   later.
6. **Unblock Reception** — Free-enroll and similar paths call one generic
   endpoint; path names in SPA config map to script names, not C routes.

### Non-goals (v1)

- WebSocket job streaming or REPL.
- Uploading Lua source from the client.
- Canvas-/product-specific C modules or `/api/enroll/*` in Hydrogen.
- Durable workflow engine or multi-step human gates (existing scoreboard is enough).
- Changing Orchestrator poll patterns already used for provision (Phase 30 B).

---

## Problem Statement (Why This Was Missed)

[LUA_PLAN_COMPLETE.md](/docs/H/plans/complete/LUA_PLAN_COMPLETE.md) Goal list
included “REST-triggered Lua execution with waiting for results.” Implementation
delivered the **engine** (workers, `scripting_submit_job`, waiters hooks,
`H.set_result`) and explicitly deferred **REST job submission** in
[lua_api.md](/docs/H/core/subsystems/scripting/lua_api.md):

| Status | Feature |
|--------|---------|
| ⏭️ Deferred | … **REST job submission**, durable workflow engine, … |

In-process submit paths exist; **no** `src/api/` handler calls
`scripting_submit_job` for external clients. The only REST touchpoint is
authenticated `GET /api/system/info` scoreboard visibility.

Reception therefore has Lua + Canvas via Orchestrator poll, but **no** SPA-callable
script conduit for interactive actions (`FL-49b`).

---

## Current Observed State (2026-08-08)

### Present and reusable

| Piece | Location / notes |
|-------|------------------|
| Worker submit | `scripting_submit_job` / `_with_source` — `src/scripting/worker_pool.{c,h}` |
| DB script fetch | `scripting_fetch_script_source(group, script, db, timeout)` — `orchestrator.c` |
| Registry | `script_registry_*` — workers load source by name |
| Scoreboard + status | `scoreboard.h` — PENDING→RUNNING→COMPLETED/FAILED/KILLED |
| Waiter slots | `scoreboard_attach_waiter` / `claim_waiter` — fields exist; worker TRACE-only wake (not full condvar client API) |
| Artifact metadata | `H.set_result(type, location)` → `result_type` / `result_location` |
| HTTP from Lua | `H.http.get/post` (+ sync) — Canvas/external REST |
| JWT helper | Conduit `auth_jwt_helper` used by auth_query / mailrelay / system info |
| Conduit service pattern | `src/api/conduit/*` + swagger annotations + blackbox tests 50–55 |
| Scripting blackbox | `tests/test_43_scripting.sh` — lifecycle, not REST invoke |

### Missing

| Gap | Impact |
|-----|--------|
| REST (or any HTTP) script run endpoint | SPA cannot trigger jobs |
| Inline **result payload** on scoreboard | `set_result` is type+location only; no `result_json` |
| Production **name → registry** path for REST | `scripting_submit_job` assumes source already registered; DB load is Orchestrator/`require` path |
| Allowlist / role gate for invokable scripts | Open name → RCE-equivalent if any DB script is callable |
| Client wait with timeout + HTTP response mapping | Waiter not wired for MHD thread |
| Docs + swagger + blackbox for invoke | Discovery gap |

### Design principles (locked unless Phase 0 revises)

1. **Ambivalent C** — C knows `script`, `params`, `wait`, auth; not Canvas IDs semantics.
2. **Named scripts only** — `group.script` or `group/script` consistent with Orchestrator config.
3. **JWT default** — Mirror `auth_query`; optional public allowlist is a later phase if needed.
4. **Sync wait first** — `wait: true` blocks until terminal or timeout; async submit+poll is phase-two.
5. **HTTP not WS** for v1 client access.
6. **Path under conduit or scripting** — Prefer `/api/conduit/script` for discoverability next to query; accept `/api/scripting/run` if conduit ownership is wrong. **Phase 0 decides.**

---

## Phase Groups

| Group | Phases | Theme |
|-------|--------|--------|
| A | 0 | Design lock |
| B | 1–3 | Result payload + production submit path |
| C | 4–6 | REST endpoint, auth, sync wait |
| D | 7–8 | Allowlist, errors, observability |
| E | 9–10 | Blackbox, docs, Reception handoff |
| F | 11+ | Optional async, public scripts, WS progress |

---

## Phases

### Phase 0: Design Lock

- **Goal:** Agree API contract, auth, naming, result shape, and path prefix before code.
- **Dependencies:** None.
- **Entry gate:** This document exists; LUA_PLAN_COMPLETE is reference-only.

#### Work items

- [x] **0.1 Path and methods** — locked below.
- [x] **0.2 Request JSON schema** — locked below.
- [x] **0.3 Response JSON schema** — locked below.
- [x] **0.4 Result payload Lua contract** — `H.set_result_json(table)`.
- [x] **0.5 Auth** — JWT always; `_hydrogen` inject; reject client `_hydrogen`.
- [x] **0.6 Allowlist strategy** — `scripts.invokable` DEFAULT false.
- [x] **0.7 Out of scope confirmation** — No WS; no source upload; no product C routes.

#### Locked decisions (Phase 0)

| Topic | Decision |
|-------|----------|
| Paths | `POST /api/conduit/script`; `GET /api/conduit/script/{job_id}` |
| Script name | Canonical `Group.Name` (dot) |
| Auth | JWT required always (v1) |
| Identity | Inject full filtered claims bag as `params._hydrogen`; **400** if client sent `_hydrogen` |
| GET authz | Only submitting `sub` (store on scoreboard at submit) |
| Result API | `H.set_result_json(table)` — C encodes with jansson |
| Missing result_json | HTTP `result` = `{}` when completed |
| Business errors | Prefer **COMPLETED** + `result: { ok:false, code, message }`; Lua crash → `status:failed` |
| Failed HTTP status | **200** + body `status:failed` (transport OK) |
| Default wait | `wait=true` if omitted |
| wait:false | Supported; pair with GET status in first shippable slice |
| Timeouts | Default **15s**, max **60s** (clamp); on HTTP timeout → body `status:timeout`, **request kill**, GET may still see later terminal if race |
| Allowlist | DB column `scripts.invokable` DEFAULT **false**; 404 for unknown **and** non-invokable (no existence leak) |
| Caps | params **256 KiB**, result **1 MiB** |
| Scripting off | **503** + clear JSON |
| Content-Type | `application/json` only |
| Echo fixture | Yes for blackbox (`Api.Echo`, invokable=true) despite no product seeds |

**Request (POST):**
```json
{
  "script": "Enroll.FreeCourse",
  "params": { "canvas_course_id": 1002 },
  "wait": true,
  "timeout_seconds": 15
}
```

**Response (always HTTP 200 for job outcomes including failed/timeout unless auth/validation/routing errors):**
```json
{
  "status": "completed",
  "job_id": "ABCDE",
  "script": "Enroll.FreeCourse",
  "result": { "ok": true },
  "result_type": "json",
  "result_location": null,
  "error": null,
  "elapsed_ms": 123
}
```
`status`: `completed` | `failed` | `killed` | `timeout` | (async) `pending`/`running`.

#### Exit gate / validation

- [x] Written decisions in Status block for 0.1–0.6.
- [x] Human Q&A design lock complete (this session).

#### Status

- **State:** complete
- **Date:** 2026-08-08
- **Result:** Design locked via clarifying questions; recorded above.
- **Variances:** GET job status pulled into v1 (not deferred to Phase 11 only). Business failures prefer COMPLETED+ok:false; runtime failures use status:failed with HTTP 200.

#### Lessons learned

- jansson is C-only — do not ask Lua authors to encode JSON strings; host accepts tables.
- Prefer existence-hiding 404 for non-invokable scripts.
- Sync default 15s is stricter than first draft 30s; pair with kill-on-timeout.

---

### Phase 1: Scoreboard Result Payload

- **Goal:** Jobs can attach a JSON (or string) **body** the HTTP layer can return, not only type/location metadata.
- **Dependencies:** Phase 0 (result contract).
- **Entry gate:** Phase 0 Status complete.

#### Work items

- [x] **1.1** `ScoreboardEntry.result_json` + `SCOREBOARD_RESULT_JSON_MAX` (1 MiB).
- [x] **1.2** `scoreboard_update_result_json`; free in `entry_clear_owned`; find/list copy.
- [x] **1.3** `H.set_result_json(table)` + `H_lua_table_to_json_string` / `H_lua_value_to_json`.
- [x] **1.4** `H.scoreboard.get` exposes `result_json`; system info snapshot **omits** body.
- [x] **1.5** Unity: `scoreboard_test_result_json` (10), `scripting_api_system_test_set_result_json` (7), install assert in `lua_context_test_create_destroy`.

#### Exit gate / validation

- [x] `mkt` green.
- [x] `mku scoreboard_test_result_json` / `scripting_api_system_test_set_result_json` / `lua_context_test_create_destroy` green.
- [x] `mkp` green.
- [x] Worker e2e sets JSON via `scripting_submit_job_with_source`.

#### Status

- **State:** complete
- **Date:** 2026-08-08
- **Result:** Payload path landed; ready for Phase 2 submit_from_db.
- **Variances:** None material.

#### Lessons learned

- Array vs object detection: pure 1..n integer keys → JSON array; empty `{}` is object.
- Auto-set `result_type="json"` on successful `set_result_json` for observers.
- Omit `result_json` from `/api/system/info` snapshot to avoid multi-MiB status payloads.

---

### Phase 2: Production Submit — Name to Source

- **Goal:** One C function REST will call: resolve DB script by name, register, submit job, return `job_id`.
- **Dependencies:** Phase 1 optional but preferred after; can parallelize with Phase 1 if careful.
- **Entry gate:** Phase 0 complete; understand `scripting_fetch_script_source` + registry.

#### Work items

- [x] **2.1** `src/scripting/scripting_invoke.{c,h}` — `scripting_submit_job_from_db`.
  - Parse **Group.Name only** (slash rejected).
  - Load → `scripting_submit_job_with_source` / `_and_limits`.
- [x] **2.2** `ScriptingInvokeError` + `scripting_invoke_error_name` for HTTP mapping.
- [x] **2.3** `source_cache` hit before `scripting_fetch_script_source`; put on miss.
- [x] **2.4** Unity `scripting_invoke_test_submit_from_db` (11 tests) via load-source hook.
- [x] **2.5** Orchestrator paths untouched (no call-site changes outside invoke module).

#### Exit gate / validation

- [x] `mkt` + `mku scripting_invoke_test_submit_from_db` green; `mkp` green.
- [x] REST will call only `scripting_submit_job_from_db` (not raw worker submit).

#### Status

- **State:** complete
- **Date:** 2026-08-08
- **Result:** Production submit path ready for wait + HTTP layers.
- **Variances:** `db_timeout` enum exists but fetch currently collapses timeout/missing into `not_found` until fetch API differentiates (HTTP can still map both to 404).

#### Lessons learned

- Load-source **hook** keeps Unity free of live DB/QTC (same pattern as OIDC HTTP injection).
- Reuse `orchestrator_resolve_database()` for DefaultDatabase / single-DB fallback.
- Registry key = full `Group.Name` string (not group alone) so workers look up what the client sent.
- Do not accept slash names even if Orchestrator config historically used slash for its own field.

---

### Phase 3: Sync Wait Helper for Callers

- **Goal:** Blocking wait on `job_id` until terminal status or timeout, suitable for MHD handler thread.
- **Dependencies:** Phase 2; waiter fields on scoreboard.
- **Entry gate:** Phase 2 submit returns live `job_id`.

#### Work items

- [x] **3.1** `scripting_wait_job` — timed poll of live `scoreboard_find` (10 ms).
  - Condvar not required; worker claim remains TRACE-only (documented).
- [x] **3.2** `[~]` Full condvar wake deferred — poll is correct and simpler for MHD; optional later optimization.
- [x] **3.3** Returns full `ScoreboardEntry` copy (includes `result_json`, errors, artifacts).
- [x] **3.4** Unity `scripting_invoke_test_wait_job` (7): names, args, not found, pre-complete, worker success, fail, timeout+kill.

#### Exit gate / validation

- [x] Terminal outcomes + timeout covered in Unity.
- [x] `mkt` / `mkp` green.
- [x] Wait loop respects `scripting_system_shutdown`.

#### Status

- **State:** complete
- **Date:** 2026-08-08
- **Result:** Sync wait ready for REST Phase 4–5.
- **Variances:** No condvar wake (3.2 deferred); timeout requests kill per Phase 0.

#### Lessons learned

- Poll is enough; attaching waiter without wake adds complexity without latency win at 10 ms.
- No `static` helpers in Hydrogen `src/` (Unity + baseline gate) — expose small helpers in header.
- `switch` on status enums must handle all cases under `-Wswitch-enum`.

---

### Phase 4: REST Handler Skeleton

- **Goal:** MHD route registered, swagger stub, disabled/404/503 behavior when scripting off.
- **Dependencies:** Phase 0 path decision.
- **Entry gate:** Phase 0 path locked.

#### Work items

- [x] **4.1** New endpoint dir under `src/api/conduit/script/` (Phase 0 path).
- [x] **4.2** Wire into `api_service.c` dispatch + JWT/JSON middleware lists (`conduit/script`, prefix `conduit/script/`).
- [x] **4.3** Swagger annotations on `script.h` for POST and GET `{job_id}`.
- [x] **4.4** `Scripting.Enabled=false` → **503** `scripting_disabled`.
- [x] **4.5** POST + GET job path; other methods **405**.
- [x] **4.6** Unity: `script_test_parse_post_json` (8), `script_test_handle_request` (7).

#### Exit gate / validation

- [x] `mkt` / `mkp` green.
- [x] Handler wired; valid POST returns skeleton envelope until Phase 5 submit.
- [x] Swagger annotations in header (payload regen deferred to docs phase if needed).

#### Status

- **State:** complete
- **Date:** 2026-08-08
- **Result:** REST skeleton live; parse/method/503/405 covered by Unity; no Lua submit yet.
- **Variances:** Happy-path POST returns `status:accepted_skeleton` (not real job) until Phase 5.

#### Lessons learned

- `api_send_json_response` takes ownership of the `json_t*` — do not `json_decref` after send.
- Unity `libhydrogen_unity` compiles conduit TUs without `USE_MOCK_API_UTILS`; `script.c` enables mock redirect under `UNITY_TEST_MODE` so handler tests can capture status.

---

### Phase 5: Auth + Params Merge + Invoke

- **Goal:** JWT validation, identity injection, call Phase 2 submit + Phase 3 wait, map to HTTP.
- **Dependencies:** Phases 1–4.
- **Entry gate:** Skeleton returns structured errors; wait + payload APIs exist.

#### Work items

- [x] **5.1** JWT via `auth_jwt_helper` + Unity jwt hook seam.
- [x] **5.2** Merge params + `params._hydrogen` (filtered claims); reject client `_hydrogen` (400).
- [x] **5.3** `wait=true`: submit_from_db → wait_job → Phase 0 body (HTTP 200 for job outcomes).
- [x] **5.4** `wait=false`: **202** + `status:pending` + `job_id`.
- [x] **5.5** Map: not_found 404; disabled 503; failed/timeout/killed body status with **200** (Phase 0 lock). GET job + `submitted_by` authz.
- [x] **5.6** Unity hooks for submit/wait/jwt; 11 + 10 tests green.

#### Exit gate / validation

- [x] Auth failure paths use conduit JWT helpers.
- [x] Happy path with submit/wait test doubles (params include `_hydrogen`).
- [x] `mkt` / `mkp` green.

#### Status

- **State:** complete
- **Date:** 2026-08-08
- **Result:** REST invoke wired end-to-end (engine + auth); fixture seed still Phase 6.
- **Variances:** Phase 5.5 draft HTTP codes superseded by Phase 0 (200 for failed/timeout).

#### Lessons learned

- Scoreboard needed `submitted_by` + `scoreboard_set_submitted_by` for GET authz.
- Omit `id_token` from `_hydrogen` bag (logout-sensitive).

---

### Phase 6: End-to-End Internal Fixture Script

- **Goal:** One seedable script that proves proxy-style and transform-style behavior without Canvas.
- **Dependencies:** Phase 5.
- **Entry gate:** REST invoke works in Unity or manual curl against trial hydrogen.

#### Work items

- [x] **6.1** Helium migration `acuranzo_1296.lua`: `Api.Echo`:
  - Echo: `H.set_result_json` of global `params` (shallow copy).
  - Optional: `params.probe_health == true` → `H.http.get_sync` to
    `$HYDROGEN_PROBE_BASE` or `http://127.0.0.1:5000` + `/api/system/health`.
- [x] **6.2** Document script name + example curl in Working Log (below).
- [x] **6.3** Worker injects `params` from `params_json`; Unity e2e
      `test_set_result_json_echo_params_e2e` proves completed + merged params.
      Full live JWT curl against migrated DB deferred to Phase 9 blackbox
      (same as plan blackbox gate).

#### Exit gate / validation

- [x] Invoke Echo path returns merged params + completed (Unity worker e2e).
- [x] Optional HTTP probe coded in fixture (`probe` object in result).
- [x] Migration follows 1289 seed pattern (VALUES + reverse DELETE); luacheck OK.

#### Status

- **State:** complete
- **Date:** 2026-08-08
- **Result:** `H_lua_inject_job_params` + `Api.Echo` seed; Unity 9/9; mkt/mkp green.
- **Variances:** Live curl e2e not run this session (needs migrated DB + JWT);
  covered by Unity + Phase 9 blackbox.

#### Lessons learned

- `params_json` was scoreboard-only until Phase 6 — workers never exposed it to
  Lua. Global `params` table is the client-script contract.

---

### Phase 7: Allowlist / Authorization

- **Goal:** Not every DB script is client-invokable.
- **Dependencies:** Phase 5; Phase 0.6 decision.
- **Entry gate:** Echo works; harden before public docs.

#### Work items

- [x] **7.1** DB column `scripts.invokable` DEFAULT 0 (`acuranzo_1297`) +
      QueryRef **#149** invokable-only load (`acuranzo_1298`).
- [x] **7.2** Non-invokable / missing → `SCRIPTING_INVOKE_ERR_NOT_FOUND` →
      HTTP **404** `script_not_found` (Phase 0 existence-hiding; not 403).
- [~] **7.3** Role requirement per script — deferred (not v1).
- [x] **7.4** Unity allow/deny matrix (Api.Echo allow; Orchestrator/unknown deny).
- [x] **7.5** Only `Api.Echo` set invokable=1; all other seeds stay 0.

#### Exit gate / validation

- [x] Deny for `Orchestrators.Orchestrator` via REST load path (Unity).
- [x] Allow for fixture Echo (Unity).
- [x] `mkt` / `mkp` green.

#### Status

- **State:** complete
- **Date:** 2026-08-08
- **Result:** Client load uses `scripting_fetch_invokable_script_source`;
  skips source_cache for allowlist decision.
- **Variances:** Phase 7 draft said 403; Phase 0 lock wins → **404**.

#### Lessons learned

- Cache-before-allowlist would let `require`/Orchestrator-loaded scripts
  become REST-callable; client path must not short-circuit on source_cache.

---

### Phase 8: Errors, Limits, Observability

- **Goal:** Production-safe limits and operable logs/metrics.
- **Dependencies:** Phases 5–7.
- **Entry gate:** Allowlist on.

#### Work items

- [x] **8.1** Config knobs (defaults = Phase 0):
      `Scripting.ClientInvokeDefaultTimeout` (15),
      `ClientInvokeMaxTimeout` (60),
      `ClientInvokeMaxParamsBytes` (256 KiB),
      `ClientInvokeMaxResultBytes` (1 MiB; clamps scoreboard store).
- [x] **8.2** `log_this(SR_API, …)` invoke start/end: script, wait, job_id,
      status, elapsed_ms — no params body.
- [x] **8.3** Response `error` = `error_message` only; traceback never
      included (admin include deferred).
- [x] **8.4** `scripting_wait_job` → `SCRIPTING_WAIT_SHUTDOWN` → HTTP 503
      `scripting_shutdown`.
- [x] **8.5** Unity: params_too_large, config timeout clamp, wait names,
      no-traceback response, shutdown wait.

#### Exit gate / validation

- [x] Oversize params → **413** `params_too_large`; result oversize rejected
      at scoreboard (config-aware cap).
- [~] Test 16/17 full blackbox not re-run this phase (shutdown unit covered;
      smoke via mkt shutdown test).
- [x] `mkt` / `mkp` green.

#### Status

- **State:** complete
- **Date:** 2026-08-08
- **Result:** Config + logs + 413 + clean shutdown wait.
- **Variances:** Test 16/17 full suite deferred; Unity covers shutdown path.

#### Lessons learned

- Prefer config-backed limits with Phase 0 compile-time defaults as fallback
  when `app_config` is NULL (Unity / early init).

---

### Phase 9: Blackbox Test

- **Goal:** `test_XX` (number: next free blackbox slot — check suite before claiming) covers script invoke.
- **Dependencies:** Phases 6–8.
- **Entry gate:** Fixture script + allowlist + REST stable.

#### Work items

- [x] **9.1** Chose **test_46** (`test_46_conduit_script.sh`) — free slot near scripting 43.
- [x] **9.2** Parallel 7 engines: scripting + API/JWT + AutoMigration; login; POST `Api.Echo` wait:true → 200 completed + params + `_hydrogen.sub`.
- [x] **9.3** Cases: no auth → 401; unknown → 404; non-invokable → 404 (Phase 0 hide; not 403); reserved `_hydrogen` → 400; wait:false → 202 + GET poll. Timeout case skipped (no slow fixture; not flaky).
- [x] **9.4** Docs: `docs/H/tests/test_46_conduit_script.md` + TESTING.md / SITEMAP / INSTRUCTIONS / STRUCTURE.
- [x] **9.5** `mks` (shellcheck) on new test script.

#### Exit gate / validation

- [x] Blackbox test green in isolation (or document engine skips).
- [x] Linked from TESTING.md / SITEMAP as required by project norms.
- [x] No flaky timing without bounded waits.

#### Status

- **State:** complete
- **Date:** 2026-08-08
- **Result:** test_46 green: SQLite full invoke path; other engines skip until live DBs have 1296–1298.
- **Variances:** Non-invokable is **404** (Phase 0), not 403; wait-timeout deferred; live engines soft-skip when QueryRef 149 missing; SQLite seeds fixture (AutoMigration blocked by APPLY hole at 1283 + 1293 SQLite SQL error).

#### Lessons learned

- Payload must include Helium migrations through 1298 (`payload-generate.sh` + rebuild) before AutoMigration can see them.
- Baseline `hydrodemo.sqlite` LOAD/APPLY gap (missing 1283) blocks APPLY; blackbox seeds `Api.Echo` + `invokable` + QueryRef **#149** on an isolated SQLite copy instead.
- JWT login uses connection name `Acuranzo`; reuse `scripting_helpers` start/shutdown.

---

### Phase 10: Documentation + Reception Handoff

- **Goal:** Operators and SPA authors know the contract; Reception can point config at real path.
- **Dependencies:** Phase 9.
- **Entry gate:** Blackbox green.

#### Work items

- [ ] **10.1** API doc: `docs/H/api/conduit/script.md` (or scripting equivalent) — request/response, auth, errors, examples.
- [ ] **10.2** Update [lua_api.md](/docs/H/core/subsystems/scripting/lua_api.md): move REST job submission from Deferred → Implemented; document `H.set_result_json`.
- [ ] **10.3** Update [scripting/README.md](/docs/H/core/subsystems/scripting/README.md) job execution section (REST submit).
- [ ] **10.4** Cross-link LUA_GUIDE, EXAMPLES if needed; RELEASES note when shipping.
- [ ] **10.5** Reception note (Working Log + optional FINISHLINE pointer): `enroll.freeEnrollPath` / script name `Enroll.FreeCourse` is product work **after** this Hydrogen surface exists — not part of this plan’s code.
- [ ] **10.6** TODO.md / plans README: mark progress; when fully done move this file to `complete/LUA_CLIENT_COMPLETE.md`.

#### Exit gate / validation

- [ ] Docs lint (`test_90` / project markdown norms) clean for touched files.
- [ ] No remaining “REST job submission deferred” contradiction in lua_api.md.

#### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

#### Lessons learned

- (fill after phase)

---

### Phase 11 (Optional): Async Status Endpoint

- **Goal:** `wait:false` clients can poll `GET .../script/{job_id}` (authz: only caller or admin).
- **Dependencies:** Phase 5+.
- **Entry gate:** Product need for long jobs (> HTTP proxy timeouts).

#### Work items

- [ ] **11.1** GET status by job_id; return same result envelope when terminal.
- [ ] **11.2** Authz model for reading another job’s result (default: deny).
- [ ] **11.3** Blackbox case for async submit + poll.
- [ ] **11.4** TTL / scoreboard retention note (jobs disappear — document).

#### Exit gate / validation

- [ ] Documented and tested; or explicitly deferred forever with rationale.

#### Status

- **State:** pending (optional)
- **Date:**
- **Result:**
- **Variances:**

#### Lessons learned

- (fill after phase)

---

### Phase 12 (Optional): Public / Cap-Gated Scripts

- **Goal:** Limited no-JWT or Cap-token invoke for public brochure actions if ever required.
- **Dependencies:** Phase 7 allowlist must be strict.
- **Entry gate:** Explicit product request; do not build speculatively.

#### Work items

- [ ] **12.1** Separate allowlist tier: public vs authenticated.
- [ ] **12.2** Cap or rate limit (reuse cap_query patterns if applicable).
- [ ] **12.3** Never expose privileged scripts on public tier.

#### Exit gate / validation

- [ ] Security review note in Working Log.
- [ ] Blackbox public deny-by-default.

#### Status

- **State:** deferred until requested
- **Date:**
- **Result:**
- **Variances:**

#### Lessons learned

- (fill after phase)

---

### Phase 13 (Optional): WebSocket Progress

- **Goal:** Stream `current_state` for long jobs — **not** a substitute for Phase 5 HTTP invoke.
- **Dependencies:** HTTP invoke stable; real WS product need.
- **Entry gate:** Explicit request.

#### Work items

- [ ] **13.1** Design only first: channel, auth, job_id subscription.
- [ ] **13.2** Implement only after design sign-off.

#### Exit gate / validation

- [ ] Design doc subsection or explicit “won’t do”.

#### Status

- **State:** deferred
- **Date:**
- **Result:**
- **Variances:**

#### Lessons learned

- (fill after phase)

---

## Risks

| Risk | Mitigation |
|------|------------|
| MHD thread blocked on long Lua/HTTP | Timeouts; optional async Phase 11; keep default timeout modest (30s) |
| Result JSON memory blowup | Hard cap Phase 8; reject oversize in `H.set_result_json` |
| Client invokes internal scripts | Allowlist Phase 7 before advertising API |
| Waiter race (attach after complete) | Documented scoreboard rules; wait helper checks terminal after attach |
| DB fetch latency on every invoke | source_cache / registry; Phase 2.3 |
| Identity spoofing via params | Server-injected `_hydrogen` claims Phase 0/5 |
| Scope creep into Canvas C | Refuse product routes; Reception scripts stay Helium Lua |

---

## Suggested Implementation Order (ROI)

1. Phase 0 (half session)
2. Phases 1 → 2 → 3 (engine completion)
3. Phases 4 → 5 → 6 (first curl success)
4. Phase 7 (before any internet exposure)
5. Phases 8 → 9 → 10 (ship)
6. 11–13 only on demand

---

## Working Log

### 2026-08-08 — Plan created

- Gap confirmed: LUA_PLAN deferred REST job submission; engine complete; no `src/api` invoke.
- Reception `FL-49b` needs client invoke for free-enroll; provision uses Orchestrator poll only.
- Decision lean (pre–Phase 0): HTTP `POST` script run + sync wait; not WS-first; conduit-adjacent path preferred.
- Scoreboard has `result_type`/`result_location` and waiter POD fields; worker wake for clients still incomplete; **no `result_json`**.
- `scripting_submit_job` does not load DB source; REST needs `submit_from_db` composition with `scripting_fetch_script_source`.

### 2026-08-08 — Phase 0 locked + Phase 1 implemented

- Phase 0 Q&A: see Locked decisions table under Phase 0.
- Phase 1 code: `result_json`, `scoreboard_update_result_json`, `H.set_result_json`, Unity suites green; `mkt` green.
- Lessons carried: table→JSON array detection; omit result_json from system info; auto result_type=json.

### 2026-08-08 — Phase 2 submit_from_db

- `scripting_invoke.c`: parse, load (cache+fetch), submit; error enum; Unity hook seam.
- Testing policy clarified: Unity every engine phase; blackbox Phase 9 required for Done.

### 2026-08-08 — Phase 3 wait_job

- `scripting_wait_job` poll + kill-on-timeout; Unity 7/7; mkt/mkp green.
- Lesson: no new statics in src/; full enum switches.
- Next: Phase 4 REST skeleton.

### 2026-08-08 — Phase 4 REST skeleton

- `src/api/conduit/script/{script.c,h}`: POST parse, GET job_id path, 503/405/400.
- `api_service.c`: route + auth + JSON middleware for `conduit/script` and prefix.
- Unity 15 tests green; mkt/mkp green. Submit/JWT = Phase 5.
- Next: Phase 5 auth + `_hydrogen` merge + submit/wait mapping.

### 2026-08-08 — Phase 5 auth + invoke

- JWT + `_hydrogen` merge; submit/wait hooks; `submitted_by` on scoreboard.
- wait true → 200 job body; wait false → 202 pending; script missing → 404.
- Unity 21 tests; mkt/mkp green. Next: Phase 6 Api.Echo fixture.

### 2026-08-08 — Phase 6 Api.Echo fixture

- **Gap fixed:** worker now calls `H_lua_inject_job_params` → global `params`.
- Seed: Helium `acuranzo_1296` → `Api.Echo` (group `Api`, name `Echo`).
- Unity: inject unit test + echo params e2e via worker pool.
- Example curl (after migrations + JWT from test_40/auth flow):

```bash
# TOKEN=... from /api/oidc or test harness
curl -sS -X POST "http://127.0.0.1:${PORT}/api/conduit/script" \
  -H "Authorization: Bearer ${TOKEN}" \
  -H "Content-Type: application/json" \
  -d '{"script":"Api.Echo","params":{"hello":"world"},"wait":true,"timeout_seconds":15}'
# Expect HTTP 200, status=completed, result.hello=world, result._hydrogen.sub=...
# Optional outbound probe:
# -d '{"script":"Api.Echo","params":{"probe_health":true},"wait":true}'
```

- Next: Phase 7 allowlist / `scripts.invokable`.

### 2026-08-08 — Phase 7 allowlist

- `scripts.invokable` + QueryRef 149; `Api.Echo` invokable=1.
- REST load: `scripting_fetch_invokable_script_source` (no cache bypass).
- Non-invokable → same 404 as missing. Unity matrix green.
- Next: Phase 8 errors/limits/observability.

### 2026-08-08 — Phase 8 limits + observability

- Config: ClientInvoke* defaults 15s / 60s / 256KiB / 1MiB.
- Logs start/end without params; 413 on oversize params; no traceback in body.
- Wait shutdown → 503. Unity expanded. Next: Phase 9 blackbox.

### 2026-08-08 — Phase 9 blackbox

- `tests/test_46_conduit_script.sh` + 7 configs (`15460–15466`) + `docs/H/tests/test_46_conduit_script.md`.
- Cases: JWT Echo wait, 401, 404×2, reserved 400, async 202+GET.
- SQLite: isolated DB + seed fixture; live engines skip until 1296–1298 applied.
- Regenerated `payload.tar.br.enc` (migrations through 1298). Indexes updated.
- Next: Phase 10 docs handoff.

---

## Related Documents

| Doc | Role |
|-----|------|
| [LUA_PLAN_COMPLETE.md](/docs/H/plans/complete/LUA_PLAN_COMPLETE.md) | Original scripting implementation (complete) |
| [lua_api.md](/docs/H/core/subsystems/scripting/lua_api.md) | Host API; deferred REST note |
| [scripting/README.md](/docs/H/core/subsystems/scripting/README.md) | Subsystem overview |
| [LUA_GUIDE.md](/docs/H/LUA_GUIDE.md) | Author guide |
| [CONDUIT_COMPLETE.md](/docs/H/plans/complete/CONDUIT_COMPLETE.md) | Conduit patterns to mirror |
| [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md) | Build/test aliases |
| [TODO.md](/docs/H/TODO.md) | Prioritized backlog entry |
| Reception FINISHLINE | Product consumer (`FL-30-lua`, `FL-49b`) |

---

## Definition of Done (Plan Complete)

- [ ] Phases 0–10 Status complete with green exit gates.
- [ ] Blackbox test in suite; docs updated; lua_api deferred line removed.
- [ ] No Canvas-specific C; generic invoke only.
- [ ] File moved to `docs/H/plans/complete/LUA_CLIENT_COMPLETE.md`; plans README + TODO updated; `mkl` run.
