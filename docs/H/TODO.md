# Hydrogen TODO

Actionable incomplete work only. Completed plans live in
[`/docs/H/plans/complete/`](/docs/H/plans/complete/). Active plan bodies live in
[`/docs/H/plans/`](/docs/H/plans/). Plan index:
[`/docs/H/plans/README.md`](/docs/H/plans/README.md).

## How to use this file

- Sorted by **immediate ROI vs effort** (quick wins first within each band).
- Items range from small code gaps to multi-phase plans — one list to prioritize from.
- Each item carries rough metrics:
  - **Effort** — S / M / L / XL (session-scale estimate to finish remaining work)
  - **Done** — portion already shipped (rough %)
  - **Remaining** — what still blocks “done”
  - **Code** — primary paths when there is no dedicated plan
- When a plan finishes: move it to `plans/complete/` with a `_COMPLETE` suffix
  (if missing), update links, remove it from this file, run `mkl`.
- Prefer fixing stale comments when closing an item so the next scan stays clean.

**Scan note (2026-07-28):** production `src/` has **no** classic `TODO`/`FIXME`
markers. Deferred work is stubs, intentional 501s, or plan leftovers. Scripting
`Phase N` file banners are historical LUA_PLAN provenance (mostly complete) —
not open work unless listed below.

---

## P0 — Close the loop (high ROI, low–medium effort)

### 1. Keycloak / OIDC RP real-IdP sign-off

| | |
| --- | --- |
| **Plan** | [`KEYCLOAK_PLAN.md`](/docs/H/plans/KEYCLOAK_PLAN.md) (Phase 5) · checklist [`OIDC_E2E_LOG.md`](/docs/H/plans/OIDC_E2E_LOG.md) · history [`OIDC-PLAN.md`](/docs/H/plans/OIDC-PLAN.md) |
| **Effort** | S–M (ops/manual; blocked on test-user MFA/OTP, not code) |
| **Done** | ~90% — Phases 0–4 complete; mock Test 42 88/88; production config wired |
| **Remaining** | Manual E2E against real Keycloak (password + OIDC login, provision, renew, logout, managers); optional Phase 26 last-method polish |
| **Why now** | Unblocks production SSO. Implementation is done; only verification remains. |

### 2. Database parameter support — verification & docs closeout

| | |
| --- | --- |
| **Plan** | [`DATABASE_UPDATE_PLAN.md`](/docs/H/plans/DATABASE_UPDATE_PLAN.md) |
| **Effort** | S |
| **Done** | ~75% — Phases 1–4 implementation complete |
| **Remaining** | Phase 5 test runs (coverage/cppcheck/shellcheck/leaks) and Phase 6 comment/docs tidy; mark complete |
| **Why now** | Code is in; finishing checkboxes shrinks the backlog with almost no design risk. |

### 3. OIDC RP — provision DefaultRoles → `account_roles`

| | |
| --- | --- |
| **Plan** | Follow-on to OIDC RP (role **mapping** at JWT time is live; this is DB seed on provision) |
| **Code** | `src/api/auth/oidc_rp/oidc_rp_link_provision.c` · `oidc_rp_link_default.c` · config `ProvisionDefaults.DefaultRoles` |
| **Effort** | S–M (QueryRef INSERT + call after #083/#081) |
| **Done** | ~30% — config parsed; logs count when non-empty; no row writes |
| **Remaining** | QueryRef (or reuse) to insert each default role_id into `account_roles`; call from both provision paths; Unity + Test 42 provision cases with DefaultRoles set |
| **Why now** | Operators can configure DefaultRoles today and get only a log line; DATABASE role source then yields empty JWT roles for new users. |
| **Note** | Distinct from `oidc_rp_roles.c` (JWT mapping). Distinct from IdP client-role parse (item 16). |

---

## P1 — Quality / safety gates (medium effort, durable ROI)

### 4. Unity ASAN variant

| | |
| --- | --- |
| **Plan** | [`UNITY_ASAN_PLAN.md`](/docs/H/plans/UNITY_ASAN_PLAN.md) |
| **Effort** | M |
| **Done** | 0% — plan only |
| **Remaining** | `unity_asan` CMake tree, harness test, first-run triage (`detect_leaks=0`) |
| **Why now** | Catches UAF/double-free on unit-only paths blackbox never hits. Separate build; does not touch gcov. |

### 5. Unity disabled-test cleanup

| | |
| --- | --- |
| **Plan** | [`UNITY_CLEANUP.md`](/docs/H/plans/UNITY_CLEANUP.md) |
| **Effort** | M |
| **Done** | ~0% — catalogue complete (43 tests) |
| **Remaining** | Review each `if (0) RUN_TEST(...)` entry: fix and re-enable, mark ignored with `TEST_IGNORE_MESSAGE`, or remove if redundant/broken |
| **Why now** | 43 disabled tests are dead weight; several segfault or have weak justification. |

### 6. Auth register — persist email on `account_contacts`

| | |
| --- | --- |
| **Plan** | No dedicated plan yet (auth suite complete; this is a follow-on gap) |
| **Code** | `src/api/auth/auth_service_database.c` (`create_account_record`) · `src/api/auth/register/register.c` |
| **Effort** | S–M (needs Acuranzo QueryRef + call site) |
| **Done** | ~40% — account row via #051; password hash via #052 wired on register; email accepted by API but not stored |
| **Remaining** | New migration QueryRef to `INSERT` email into `account_contacts` (contact type email); call from `create_account_record` or register; Unity + blackbox register coverage |
| **Why now** | Register returns 201 with email in JSON but login-by-email / contact lookups stay empty unless fixtures seed contacts. |
| **Note** | Do **not** reuse QueryRef #052 — that is password-hash storage only. |

### 7. WebSocket server heartbeat — blackbox coverage

| | |
| --- | --- |
| **Plan** | No dedicated plan (WebSocket stack otherwise live) |
| **Code** | `src/websocket/websocket_server_heartbeat.c` · `websocket_server_dispatch.c` · `websocket_server_connection.c` |
| **Effort** | S |
| **Done** | ~90% — timer arm on connect, `TIMER` → health/ping-due, writable sends ping, pong handler live; config `WebSocketServer.Heartbeat.*` |
| **Remaining** | Confirm gcov hits `ws_send_ping` / timer path under `test_23` (short `PingIntervalSeconds`); stale-connection close assertions if missing |
| **Why now** | Implementation looks wired; prior backlog assumed zero callers. Validate coverage, then drop or shrink this item. |
| **Note** | Distinct from libwebsockets client `--ping-interval` used in tests today. |

### 8. VictoriaLogs fanout — TLS (`https://`)

| | |
| --- | --- |
| **Code** | `src/logging/victoria_logs.c` (`victoria_logs_send_http_post`, `victoria_logs_parse_url`) |
| **Effort** | M |
| **Done** | ~70% — URL parse sets `use_ssl`; POST path is plain TCP and ignores the flag |
| **Remaining** | TLS connect (OpenSSL or reuse existing crypto/curl patterns); fail closed if `https` requested and TLS unavailable; blackbox against TLS sink or documented http-only constraint |
| **Why now** | Production log sinks are often HTTPS; silent downgrade/ignore is a footgun. |

### 9. DB queue health check — live connection probe

| | |
| --- | --- |
| **Code** | `src/database/dbqueue/dbqueue.c` (`database_queue_health_check`) |
| **Effort** | S–M |
| **Done** | ~40% — shutdown flag + depth watermark only |
| **Remaining** | Optional lightweight connectivity/liveness check consistent with engine handles; avoid false negatives during migrate |
| **Why now** | Health endpoints/registry may report healthy with a dead backend. |

### 9a. Config-driven engine health SQL + bootstrap orphan DROP

| | |
| --- | --- |
| **Plan** | None (fits “no business SQL in C” hygiene; small design choice) |
| **Code** | Health: `src/database/{postgresql,mysql,sqlite,db2}/connection.c` (`*_health_check`) · Orphan drop: `src/database/database_bootstrap.c` (~zero-row bootstrap path builds `DROP TABLE IF EXISTS %s`) |
| **Effort** | S–M |
| **Done** | Hard-coded probes today: PG/MySQL/SQLite `"SELECT 1"`; DB2 `"SELECT 1 FROM SYSIBM.SYSDUMMY1"`. Bootstrap orphan path parses table name from configured bootstrap SQL via `FROM …` and runs a C-built `DROP TABLE IF EXISTS`. |
| **Remaining** | **Health:** per-database (or per-engine) config string for the liveness statement; engines execute that instead of literals (sensible defaults matching current strings). **Orphan DROP:** document why it exists (empty bootstrap result ⇒ treat target table as orphaned and drop before retry/recreate); replace C-built DDL with a config template or explicit QueryRef/DDL hook; verify dialect safety (`IF EXISTS`, schema-qualified names, quoting) across all engines — works today largely because Acuranzo bootstrap table names are simple unquoted identifiers, not because the helper is portable SQL. |
| **Why** | Operators may want a different health probe; keeping DROP/DDL out of C matches the QueryRef model and avoids silent multi-engine edge cases. |
| **Note** | Migrations still supply their own SQL from payloads (out of scope). Item 9 is queue-depth health; this is the **engine** probe SQL. |

### 10. Payload / webserver path hygiene

| | |
| --- | --- |
| **Code** | `src/payload/payload_cache.c` · `src/webserver/web_server_core.c` (`get_payload_subdirectory_path`) |
| **Effort** | S–M |
| **Done** | Live cache uses `process_payload_tar_cache_from_data`; swagger/terminal use cache APIs |
| **Remaining** | Either implement `get_payload_subdirectory_path` against payload cache **or** remove/stop calling the synthetic `/payload/...` helper; delete or implement unused `process_payload_tar_cache` / `list_tar_contents` symbols |
| **Why now** | Dead API surface and a misleading path resolver confuse coverage and future webroot work. |

### 11. Dead API cleanup — `oidc_generate_refresh_token`

| | |
| --- | --- |
| **Code** | `src/oidc/oidc_tokens.c` · live path `src/oidc/oidc_refresh_tokens.c` (`oidc_refresh_issue`) |
| **Effort** | S |
| **Done** | Real refresh issue/rotate/validate live on IdP token path |
| **Remaining** | Remove legacy `oidc_generate_refresh_token` (+ header/Unity) **or** thin-wrapper to `oidc_refresh_issue` if anything external still links it |
| **Why now** | No-op symbol that looks like unfinished Phase 11 work. |

### 12. Database fault tolerance — crash / transient outage while running

| | |
| --- | --- |
| **Plan** | None yet (spin a short plan when starting; touches DQM + engines + API error surface) |
| **Code** | `src/database/dbqueue/heartbeat.c` · `database_watchdog.*` · `dbqueue.c` health · engine connection paths · Conduit/auth submit error paths |
| **Effort** | L (design + multi-engine; not a one-file fix) |
| **Done** | Partial foundations only — Lead DQM heartbeat already discards corrupted connections and retries (“connection lost - will retry”), bootstrap re-run on Lead reconnect; query **watchdog is log/ALERT only** (no cancel/retry hooks yet per its own header); queue `health_check` is depth/shutdown, not live probe (see item 9) |
| **Remaining** | Define desired behavior when a backend dies mid-flight or flaps: (1) detect lost TCP/session across engines, (2) drain/fail in-flight queries with clear errors (not hang), (3) reconnect + optional Lead bootstrap without full process restart, (4) back-pressure / 503 vs queue forever, (5) caller-visible retry policy (or explicit no-retry), (6) blackbox: kill/restart DB under load and assert recovery; document operator expectations; (7) MySQL cancel via dedicated admin connection so `mysql_kill` cannot wedge the watchdog on a dead socket |
| **Why now** | Long-running multi-DB deployments will hit restarts and network blips; today resilience is heartbeat-centric and uneven, and stuck queries are observed more than cancelled. |
| **Note** | Orthogonal to migration/startup readiness. Mail Relay already has transport retry — do not invent a second pattern without aligning semantics. Item 9 (health probe) is a small slice of this. |

### 12a. DQM child-queue auto-scale (optional)

| | |
| --- | --- |
| **Code** | `src/database/dbqueue/process.c` (`database_queue_manage_child_queues`) |
| **Effort** | L |
| **Done** | Intentionally no-op — children spawned at Lead startup from config; live until shutdown |
| **Remaining** | Only if product needs dynamic scale: refcounted child lifetime + safe scale-down under concurrent submit |
| **Note** | Not a defect. Prior scale-down caused UAF; static children are the supported model. |

### 12b. Scoreboard job waiter → real wake (optional)

| | |
| --- | --- |
| **Code** | `src/scripting/worker_pool.c` (`scripting_signal_waiter_if_present`) · `scoreboard_attach_waiter` / `claim_waiter` |
| **Effort** | M |
| **Done** | Attach/claim one-shot semantics work; worker claims at terminal and TRACE-logs; jobs observed via `H.scoreboard.get` / list poll |
| **Remaining** | If blocking wait on scoreboard jobs is required: condvar/H_Handle signal using claimed `waiter_handle` (query/HTTP already use other wait paths) |
| **Note** | Incomplete condvar path, not broken polling. Capture so “would signal” comments do not reappear as mystery debt. |

### 12c. Dead API — `database_queue_lead_process_queries`

| | |
| --- | --- |
| **Code** | `src/database/dbqueue/lead.c` · Unity `lead_test_process_queries` |
| **Effort** | S |
| **Done** | Symbol exists; **no production caller** — dequeues and frees without engine execute |
| **Remaining** | Delete function + tests, or rewire to `process_single_query` if a Lead-specific path is needed |
| **Why** | Looks like unfinished Lead processing; is Unity-only surface. |

---

## P2 — Active product subsystems (larger, clear value)

### 13. Mail Relay — finish remaining phases

| | |
| --- | --- |
| **Plan** | [`MAILRELAY_PLAN.md`](/docs/H/plans/MAILRELAY_PLAN.md) |
| **Effort** | L–XL |
| **Done** | ~70% — Phases 0–5, 7–8 done; Phase 6/10 partial; pause after 7B/8 |
| **Remaining** | System template seeds, Phase 9 Lithium UI, Phase 10 ops remainder, Phases 11–15 (inbound/rewrite/security/docs as scoped), auth MFA wiring via OTP; blackbox depth (events/persist/debounce) may land in parallel sessions |
| **Why next** | Core send/API/Lua/OTP stack works (`test_57`/`test_58`). Remaining is product surface and ops polish. |
| **Note** | Parallel session may complete subsets — re-check plan/tests before starting. |

### 14. Chat — Phase 13 advanced features (+ known gaps)

| | |
| --- | --- |
| **Plan** | [`CHAT_PLAN_PHASE_13.md`](/docs/H/plans/CHAT_PLAN_PHASE_13.md) · index [`CHAT_PLAN_SUMMARY.md`](/docs/H/plans/CHAT_PLAN_SUMMARY.md) |
| **Effort** | XL |
| **Done** | Phases 1–12 complete; WS streaming + media single-upload + non-stream `chat_done` blackbox live; Phase 13 feature list mostly open |
| **Remaining (Phase 13 wishlist)** | Function calling, response cache, key load-balance, fallback engines, analytics, templates, convo APIs, cost tracking, A/B, tests |
| **Remaining (concrete gaps)** | Sub-items 14a–14c |
| **Why later** | Large wishlist on top of a working chat proxy. Prefer discrete bullets when product needs them. |

#### 14a. REST `/api/conduit/auth_chat` SSE streaming

| | |
| --- | --- |
| **Code** | `src/api/wschat/auth_stream/auth_stream.c` · `src/api/wschat/auth_chat/auth_chat.c` |
| **Effort** | L (MHD callback/SSE + reuse multi_curl proxy path) |
| **Done** | ~20% — non-stream REST works; endpoint returns intentional **501** / SSE error event; WS streaming fully works |
| **Remaining** | MHD incremental SSE response; drive `chat_proxy_*` multi-stream into SSE frames; flip `stream:true` off 501; update `test_59` (today asserts 501) and Unity stubs |
| **Note** | Interactive streaming is already on WebSocket. REST SSE is parity for HTTP-only clients. |

#### 14b. WebSocket chunked media upload

| | |
| --- | --- |
| **Code** | `src/websocket/websocket_server_media.c` (`handle_media_chunk_message`) |
| **Effort** | M |
| **Done** | ~70% — single-message `media_upload` path complete (hash, store #071, blackbox) |
| **Remaining** | Session buffers for `media_chunk` (upload_id / index / total); assemble → store; bounds/concurrency; cleanup on disconnect |
| **Note** | Stub returns -1 by design until multi-frame uploads are required. |

#### 14c. Legacy chat send helpers cleanup

| | |
| --- | --- |
| **Code** | `src/websocket/websocket_server_chat_send.c` (`send_chat_chunk` / unused `send_stream_*` if any) |
| **Effort** | S |
| **Done** | `websocket_server_chat_stream.c` already removed; live path is multi_curl (`proxy_multi` / `proxy_mc`) |
| **Remaining** | Drop dead send helpers **or** rewire if a product path needs them; adjust Unity if any |
| **Note** | Coverage noise only — not a runtime bug. |

### 15. Terminal WebSocket authentication

| | |
| --- | --- |
| **Code** | `src/terminal/terminal_websocket.c` (`terminal_websocket_requires_auth`) |
| **Effort** | M |
| **Done** | ~10% — hook always returns false |
| **Remaining** | Product decision (JWT/session cookie vs stay open on trusted nets); implement gate + blackbox |
| **Why later** | Fine for lab; risk on exposed deployments. |

### 16. H.notify — real delivery or permanent deprecation

| | |
| --- | --- |
| **Code** | `src/scripting/scripting_api_mail_notify.c` · docs `MAIL_GUIDE.md` |
| **Effort** | S (docs/deprecate) or M–L (implement channel→template map) |
| **Done** | Stable deferred error `"notify: deferred to mailrelay rules"`; `H.mail` is the real path |
| **Remaining** | Either implement notify→mailrelay rules mapping **or** document as permanent shim and fold “Notify subsystem” (item 21) into Mail Relay only |
| **Note** | Do not leave both notify launch scaffold and Lua notify ambiguous. |

### 17. OIDC RP — parse IdP client roles (`resource_access`)

| | |
| --- | --- |
| **Code** | `src/api/auth/oidc_rp/oidc_rp_idtoken.c` · `oidc_rp_roles.c` (`IDP_CLIENT_ROLES`) |
| **Effort** | S–M |
| **Done** | Config enum exists; mode falls back to realm roles |
| **Remaining** | Parse `resource_access.<client>.roles` into claims; use in IDP_CLIENT_ROLES / MERGE; tests |
| **Why later** | Only needed when operators choose client-scoped Keycloak roles. |

---

## P3 — Greenfield / deferred (keep visible, do not start casually)

### 18. Hydrogen as OIDC Identity Provider (post-MVP)

| | |
| --- | --- |
| **Plan** | [`OIDC_IDP.md`](/docs/H/plans/OIDC_IDP.md) |
| **Effort** | M–L remaining (was XL) |
| **Done** | ~90% — Phases 0–15 complete (protocol + Test 45 + hardening); Phase 16 docs; refresh via `oidc_refresh_tokens.c` |
| **Remaining** | Phase 17 optional: end-session (`api/oidc/end_session` → 501), dynamic registration (`api/oidc/registration` → 501), client credentials, consent UI; DB-backed codes/refresh for multi-process; optional userinfo accounts-DB profile merge (`oidc_service_userinfo.c`); retire or replace scaffold `oidc_users.c` (test_user dummy) |
| **Docs** | [oidc_endpoints.md](/docs/H/api/oidc/oidc_endpoints.md) · [OIDC_IDP_OPERATOR.md](/docs/H/api/oidc/OIDC_IDP_OPERATOR.md) |
| **Note** | Separate from OIDC **RP** (Keycloak). MVP IdP is usable behind kill switch; HA needs durable stores. |

### 19. Print subsystem — job → device / Beryllium handoff

| | |
| --- | --- |
| **Plan** | None (print queue + Beryllium analyze exist as pieces) |
| **Code** | `src/print/print_queue_manager.c` · `src/print/beryllium.c` · upload path in webserver |
| **Effort** | L–XL (needs printer I/O product definition) |
| **Done** | ~30% — queue create/dequeue/thread; job JSON parse/log; Beryllium G-code analysis library present |
| **Remaining** | Define device interface; wire `process_print_job` → open file → optional Beryllium analyze → printer driver/status; blackbox when hardware or mock exists |
| **Note** | Launch marks print running; processing is log-only until a real sink exists. |

### 20. mDNS client runtime (browse / discover)

| | |
| --- | --- |
| **Plan** | None (server is complete under `src/mdns/`) |
| **Code** | `src/launch/launch_mdns_client.c` · config `mdns_client` · no `src/mdns_client/` worker |
| **Effort** | L–XL |
| **Done** | ~25% — config validation, launch/landing registry, readiness checks |
| **Remaining** | Browse PTR/SRV/TXT for configured service types; result cache/API; real init beyond “register RUNNING”; tests |
| **Note** | Launch is a scaffold that marks the subsystem running without discovery. mDNS **server** announcements are separate and working (`test_25`). |

### 21. Notify subsystem — outbound SMTP runtime

| | |
| --- | --- |
| **Plan** | None (or fold into Mail Relay if product chooses one path) |
| **Code** | `src/launch/launch_notify.c` · config `notify.smtp` · no `src/notify/` send path |
| **Effort** | M–L |
| **Done** | ~25% — config + launch validates SMTP notifier settings and registers subsystem |
| **Remaining** | Decide with item 16: implement notify SMTP client **or** officially route events through Mail Relay; then wire auth/system events; tests |
| **Note** | Mail Relay is the production mail stack today. Notify launch is config-only placeholder. |

### 22. Mirage distributed proxy

| | |
| --- | --- |
| **Plan** | [`MIRAGE_PLAN.md`](/docs/H/plans/MIRAGE_PLAN.md) |
| **Effort** | XL |
| **Done** | 0% — architecture sketch only |
| **Remaining** | Full design → phased implementation (not yet broken into gates) |
| **Note** | Deferred. Do not treat as near-term backlog. |

### 23. Enum / struct reservations (no work unless product needs them)

| | |
| --- | --- |
| **Code** | `DB_ENGINE_AI` in `database_types.h` · `is_write_lock` in `mutex.h` · similar “future” fields |
| **Effort** | n/a |
| **Done** | Reserved identifiers only |
| **Remaining** | Implement only if a real engine/rwlock design appears; otherwise leave as reserved |
| **Note** | Not actionable backlog — listed so scans do not rediscover them as gaps. |

---

## Outside this tree (track elsewhere)

| Item | Where | Note |
| --- | --- | --- |
| Lithium table refactor Phases 12–19 | [`/docs/Li/LITHIUM-TAB-PLAN.md`](/docs/Li/LITHIUM-TAB-PLAN.md) | Frontend; not a Hydrogen `plans/` item |
| Cap master coordination | external `CAP_PLAN.md` | Hydrogen Phase 2 query work is in [`complete/CAP_PLAN_QUERY-COMPLETE.md`](/docs/H/plans/complete/CAP_PLAN_QUERY-COMPLETE.md); deferred 2.8 cache work noted there |

---

## Recently completed (do not re-open)

Moved under [`plans/complete/`](/docs/H/plans/complete/) in this cleanup, including:

Auth suite, Conduit (+ fix/diagrams), Database subsystem, Terminal, Migrations, Chat Phases 1–12, Lua scripting, Cap query, Mail Relay blackbox, Static-function purge, Log fanout, FilterLogs, Test 40 debug archaeology, and the old code-level `plans/TODO.md`.

**2026-07 non-OIDC code cleanup** (not plan moves, but closed as code debt):

- Database façade TODOs wired (`database_execute`, shutdown/health/reload/remove/validate)
- Register password persist via QueryRef #052
- `api_create_jwt` real HS256 implementation
- Chat LRU write-through dirty-flag fix (`chat_lru_cache_flush` clears residual dirty only)
- Stale conduit swagger 501 / config `DUMP_NOT_IMPLEMENTED` removed
- Intentional stubs documented in-code (REST SSE 501, media_chunk -1, print log-only, mDNS/Notify launch scaffold)
- `deserialize_query_from_json` implemented (stale “placeholder” comment removed)

**2026-07-28 comment hygiene** (minimal behavior: REST SSE error string wording only):

- OIDC RP provision DefaultRoles comments no longer claim “Phase 22 deferred”
- `oidc_generate_refresh_token` documented as legacy no-op vs `oidc_refresh_issue`
- IdP end-session/registration 501s tagged Phase 17 optional
- Payload/webserver/VictoriaLogs/terminal/dbqueue health comments aligned with reality
- Cleared “for now” / “will be implemented” / “stub implementation” phrasing where work is done or intentional
- Scoreboard waiter docs match claim+TRACE (no false Phase 13 promise)
- New TODO slices: 12a auto-scale, 12b waiter wake, 12c lead_process_queries

---

## Status snapshot

| # | Item | Effort left | Done | Priority |
| --- | ------ | ------------- | ------ | ---------- |
| 1 | Keycloak / OIDC RP E2E | S–M | ~90% | P0 |
| 2 | Database params closeout | S | ~75% | P0 |
| 3 | Provision DefaultRoles → account_roles | S–M | ~30% | P0 |
| 4 | Unity ASAN | M | 0% | P1 |
| 5 | Unity disabled-test cleanup | M | ~0% | P1 |
| 6 | Register email → account_contacts | S–M | ~40% | P1 |
| 7 | WS heartbeat blackbox | S | ~90% | P1 |
| 8 | VictoriaLogs TLS | M | ~70% | P1 |
| 9 | DB queue health probe | S–M | ~40% | P1 |
| 9a | Config health SQL + bootstrap orphan DROP | S–M | hard-coded | P1 |
| 10 | Payload/webserver path hygiene | S–M | partial | P1 |
| 11 | Drop legacy oidc_generate_refresh_token | S | n/a | P1 |
| 12 | DB fault tolerance (crash/transient) | L | partial | P1 |
| 12a | DQM child auto-scale (optional) | L | no-op by design | P1 |
| 12b | Scoreboard waiter condvar wake | M | poll only | P1 |
| 12c | lead_process_queries dead API | S | Unity-only | P1 |
| 13 | Mail Relay remainder | L–XL | ~70% | P2 |
| 14 | Chat Phase 13 (+ 14a–14c) | XL | ~15% of P13 | P2 |
| 14a | REST auth_chat SSE streaming | L | ~20% | P2 |
| 14b | WS chunked media upload | M | ~70% | P2 |
| 14c | Legacy chat_send helpers | S | n/a | P2 |
| 15 | Terminal WS auth | M | ~10% | P2 |
| 16 | H.notify real or deprecate | S–L | shim | P2 |
| 17 | OIDC RP client-role parse | S–M | fallback | P2 |
| 18 | OIDC IdP post-MVP | M–L | ~90% | P3 |
| 19 | Print job → device / Beryllium | L–XL | ~30% | P3 |
| 20 | mDNS client runtime | L–XL | ~25% | P3 |
| 21 | Notify SMTP runtime | M–L | ~25% | P3 |
| 22 | Mirage | XL | 0% | P3 |
| 23 | Reserved enums/fields | n/a | n/a | P3 |

(End of file)
