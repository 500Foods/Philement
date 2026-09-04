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

### 1. Auth Finale — OIDC, Keycloak, register, MFA, IdP leftovers

| | |
| --- | --- |
| **Plan** | [`AUTH_FINALE.md`](/docs/H/plans/AUTH_FINALE.md) |
| **Effort** | XL |
| **Done** | Password login/renew/logout; OIDC RP Phases 1–26 + multi-provider; IdP Phases 0–16 + Test 45; Mail Relay OTP primitives |
| **Remaining** | Gated Phases 0–12 (+8b, +10b): register email, DefaultRoles, client-role parse, RP health/backchannel, terminal WS auth, login MFA, password reset, IdP durability, optional IdP post-MVP, self-service session revoke, real-Keycloak E2E (Phase 11, ops-gated), docs |
| **Why now** | One plan for remaining auth, same pattern as Chat Finale. Production SSO is coded; register/provision still lie; live Keycloak unsigned (OTP blocker) |
| **Note** | History: [`OIDC-PLAN_COMPLETE.md`](/docs/H/plans/complete/OIDC-PLAN_COMPLETE.md), [`KEYCLOAK_PLAN_COMPLETE.md`](/docs/H/plans/complete/KEYCLOAK_PLAN_COMPLETE.md), [`OIDC_IDP_COMPLETE.md`](/docs/H/plans/complete/OIDC_IDP_COMPLETE.md), [`AUTH_PLAN_COMPLETE.md`](/docs/H/plans/complete/AUTH_PLAN_COMPLETE.md). Chat JWT mint: [`CHAT_FINALE_COMPLETE.md`](/docs/H/plans/complete/CHAT_FINALE_COMPLETE.md) |

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

### 12d. Mail Relay Persist — MySQL/MariaDB SEGV on QueryRef 93

| | |
| --- | --- |
| **Code** | `src/database/mysql/query.c` (prepared bind/execute) · QueryRef **093** `acuranzo_1223.lua` · `mailrelay_repo_queue_insert` · workers Persist path |
| **Effort** | M |
| **Done** | INTEGER binds use `MYSQL_TYPE_LONGLONG` for 8-byte values; hand-rolled `MYSQL_BIND` aligned to MariaDB (`error` as `my_bool*`, `flags`, function-pointer arity). `test_58` Persist enabled for all engines including MySQL/MariaDB. |
| **Remaining** | Confirm `test_58` MySQL/MariaDB Persist live (not run this session). |
| **Why now** | Persist was shielded off those engines; bind ABI was the likely SEGV. |
| **Note** | Distinct from item 12e (pkey race). |

### 12e. App-generated `MAX+1` PKs — clients must confirm insert + retry on conflict

| | |
| --- | --- |
| **Design** | **Intentional:** prefer integer PKs via `COALESCE(MAX(id),0)+1` + `INSERT_KEY_*` / `RETURNING` over UUIDs everywhere. Migrations that emit this pattern are fine; the duty is on **callers**, not on rewriting schema to sequences/UUIDs. |
| **Code (example)** | QueryRef **093** `acuranzo_1223.lua` (`mail_queue`); Hydrogen `mailrelay_persist_message` / `insert_callback`. Same SQL shape is **widespread** in Acuranzo migrations (`INSERT_KEY_START` / `WITH next_*_id AS (SELECT COALESCE(MAX…)+1)`). |
| **Effort** | M (audit clients + add retry where concurrent inserts are possible) |
| **Done** | Convention works single-threaded; `RETURNING` / result row gives the new id **only after a successful insert** |
| **Remaining** | (1) **Audit** C (and any other) clients of `INSERT_KEY_*` / `MAX+1` QueryRefs for concurrent use. (2) On unique/duplicate PK (or missing returned id): **retry** the insert (bounded), then treat returned id as success. (3) Mail Relay Persist: lifecycle debounce + API send can collide today → flaky `mail_queue_pkey` under PG; `mailrelay_persist_message` fails once with no retry. (4) Document the contract: success = insert OK **and** new id in result; failure = retry or surface error — never assume `MAX+1` alone is race-free. |
| **Why now** | Concurrent Persist/workers will hit this; silent one-shot failure is worse than a short retry loop. |
| **Note** | Orthogonal to 12d (MySQL SEGV). Do **not** “fix” only by serializing tests. Search migrations for `COALESCE(MAX(` / `INSERT_KEY_` to find the surface area; fix **call sites**, not the migration style. |

---

## P2 — Active product subsystems (larger, clear value)

### 13. Mail Relay — finish remaining phases

| | |
| --- | --- |
| **Plan** | [`MAILRELAY_PLAN.md`](/docs/H/plans/MAILRELAY_PLAN.md) |
| **Effort** | L–XL |
| **Done** | ~75% — Phases 0–5, 7–8, 7A/7B done; templates/events seeds 1280–1282; Test 57/58 seams; Persist retry in C; 12d bind fix |
| **Remaining** | Phase 6.1b custom DB event scripts if still wanted; Phase 9 Lithium UI (placeholder); Phase 10.2–10.5; Phases 11.1–11.3 HA claim; 12–15 inbound/hardening/release. Login MFA → [`AUTH_FINALE.md`](/docs/H/plans/AUTH_FINALE.md) Phase 8. Do not use plan pause “migration 1263” — that number is QueryRef 129. |
| **Why next** | Core send/API/Lua/OTP stack works (`test_57`/`test_58`). Remaining is product surface and ops polish. |
| **Note** | Parallel session may complete subsets — re-check plan/tests before starting. |

### 25. SchemaHelper — interactive SchemaTool front-end

| | |
| --- | --- |
| **Plan** | [`SCHEMAHELPER.md`](/docs/H/plans/SCHEMAHELPER.md) |
| **Guide** | [`/docs/H/tools/SCHEMAHELPER.md`](/docs/H/tools/SCHEMAHELPER.md) |
| **Effort** | L |
| **Done** | v1 (Phases 0–4 + 6). Phase 5 first slice: one-field metadata apply. |
| **Remaining** | Orphan/anomaly DELETE, live `[r]`, dashboard count wording |
| **Why later** | SchemaTool already audits; v1 review + packets shipped. Not a Hydrogen subsystem. |
| **Note** | Lua TUI in `extras/schematool/`. SchemaTool stays read-only. Packets, not full `design_NNNN.lua`. |

### 24. `H.externaldb` — ad-hoc external database connections from Lua scripts

| | |
| --- | --- |
| **Plan** | Extends `LUA_PLAN_COMPLETE.md` (Phase 29) — scripting can already run queries via `H.query` (through the DQM queue system); this adds a parallel direct-connection surface that bypasses DQM entirely |
| **Example** | A Lua script connects to a Canvas LMS PostgreSQL/MySQL database (not managed by Hydrogen's config) and runs a query using a plain connection string and SQL |
| **Code (new)** | `src/scripting/scripting_api_db.c` (mirror `scripting_api_http.c` pattern) · C wrappers around `DatabaseEngineInterface` vtable + `parse_connection_string` |
| **Code (existing infra)** | `database.h` `DatabaseEngineInterface` (connect/disconnect/execute_query) · `database_connstring.c:parse_connection_string` · `QueryRequest`/`QueryResult` · `H_lua_install_api` in `lua_context.c` |
| **Effort** | M |
| **Done** | 0% — planning |
| **Remaining** | (1) `H.externaldb.connect(conn_str)` → userdata handle with metatable for `:query(sql, params?)` and `:close()`; (2) resolve engine from connection string via `database_get_engine_interface` / `database_engine_get`; (3) build `ConnectionConfig` via `parse_connection_string`, call `engine->connect`; (4) `engine->execute_query` + format `QueryResult` as Lua table; (5) `engine->disconnect` + `free_connection_config` on close; (6) GC `__gc` to avoid leaks; (7) register `H_lua_install_db` in `lua_context.c:246`; (8) Unity unit tests (model `scripting_api_http_test.c`); (9) blackbox via Test 43. **Do not** call `database_create_and_start_queue` — it builds a DQM queue (worker threads + queue manager registration), which is the opposite of what an ad-hoc connection needs. |
| **Why now** | Operators increasingly need to query external databases (Canvas, SIS, etc.) from scripts without provisioning them as Hydrogen databases. Reusing the existing `DatabaseEngineInterface` + `parse_connection_string` infra gives multi-engine support and named `QueryRef`-style parsing for free. |

---

## P3 — Greenfield / deferred (keep visible, do not start casually)

### 19. Print subsystem — job → device / Beryllium handoff

| | |
| --- | --- |
| **Plan** | None (print queue + Beryllium analyze exist as pieces) |
| **Code** | `src/print/print_queue_manager.c` · `src/print/beryllium.c` · upload path in webserver |
| **Effort** | L–XL (needs printer I/O product definition) |
| **Done** | ~30% — queue create/dequeue/thread; job JSON parse/log; Beryllium G-code analysis library present |
| **Remaining** | Define device interface; wire `process_print_job` → open file → optional Beryllium analyze → printer driver/status; blackbox when hardware or mock exists |
| **Note** | Launch marks print running; processing is log-only until a real sink exists. |

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
- Intentional stubs documented in-code (REST SSE 501, media_chunk -1, print log-only, Notify launch scaffold). mDNS upgrade is complete ([`MDNS_UPGRADE_COMPLETE.md`](/docs/H/plans/complete/MDNS_UPGRADE_COMPLETE.md)).
- `deserialize_query_from_json` implemented (stale “placeholder” comment removed)

**2026-07-28 comment hygiene** (minimal behavior: REST SSE error string wording only):

- OIDC RP provision DefaultRoles comments no longer claim “Phase 22 deferred”
- IdP end-session/registration 501s tagged Phase 17 optional
- Payload/webserver/VictoriaLogs/terminal/dbqueue health comments aligned with reality
- Cleared “for now” / “will be implemented” / “stub implementation” phrasing where work is done or intentional
- Scoreboard waiter docs match claim+TRACE (no false Phase 13 promise)
- New TODO slices: 12a auto-scale, 12b waiter wake, 12c lead_process_queries

**2026-08-07 H.notify permanent shim (§16) + Notify fold (§21):**

- Decision: `H.notify` stays forever as deferred-error only (`"notify: deferred to mailrelay rules"`); no channel→template map
- Production mail: Mail Relay only (`H.mail`, REST, events, `LogNotify` fanout)
- `launch_notify` / `NotifyConfig` remain config-only scaffold (not a second SMTP runtime)
- Docs: `MAIL_GUIDE.md`, `LUA_GUIDE.md`, `lua_api.md`; comments in `scripting_api_mail_notify.c`, `launch_notify.c`, `landing_notify.c`

**2026-08-07 Unity disabled-test cleanup (§5):**

- All 43 `if (0) RUN_TEST(...)` entries reviewed; plan moved to [`UNITY_CLEANUP_COMPLETE.md`](/docs/H/plans/complete/UNITY_CLEANUP_COMPLETE.md); zero remaining `if (0) RUN_TEST` in Unity sources

**2026-08-07 database params closeout:**

- [`DATABASE_UPDATE_PLAN_COMPLETE.md`](/docs/H/plans/complete/DATABASE_UPDATE_PLAN_COMPLETE.md) — Phases 5–6 verification/docs done; Unity param suites + `mkp`/`mks` green
- Docs: [PARAMETER_TYPES.md](/docs/H/database/PARAMETER_TYPES.md), [PARAMETER_BINDING.md](/docs/H/database/PARAMETER_BINDING.md)

**2026-09-02 AUTH_FINALE consolidation:**

- Remaining OIDC / Keycloak / register / MFA / terminal-WS-auth / IdP post-MVP work gathered into [`AUTH_FINALE.md`](/docs/H/plans/AUTH_FINALE.md)
- Historical plans moved: [`KEYCLOAK_PLAN_COMPLETE.md`](/docs/H/plans/complete/KEYCLOAK_PLAN_COMPLETE.md), [`OIDC-PLAN_COMPLETE.md`](/docs/H/plans/complete/OIDC-PLAN_COMPLETE.md), [`OIDC_IDP_COMPLETE.md`](/docs/H/plans/complete/OIDC_IDP_COMPLETE.md), [`OIDC_E2E_LOG_COMPLETE.md`](/docs/H/plans/complete/OIDC_E2E_LOG_COMPLETE.md)
- TODO items 1, 3, 6, 15, 17, 18 collapsed into item 1 (Auth Finale). Login MFA wiring no longer listed under Mail Relay remainder

**2026-08-07 dead API cleanup:**

- Removed legacy no-op `oidc_generate_refresh_token` (+ header/Unity); live path remains `oidc_refresh_issue`
- Removed dead `database_queue_lead_process_queries` (+ header, `lead_test_process_queries`, coverage-improvement cases); production path remains `database_queue_process_single_query`
- Removed dead `send_chat_chunk` (+ decl/Unity); live WS helpers remain `send_chat_error` / `send_chat_done`; streaming stays multi_curl
- Payload/webserver path hygiene: removed unused `resolve_webroot_path` / `get_payload_subdirectory_path` / `resolve_filesystem_path`, no-op `process_payload_tar_cache` / `list_tar_contents`, and their Unity tests; live cache remains `process_payload_tar_cache_from_data` + swagger/terminal cache APIs
- WS heartbeat coverage closed: `test_59` already blackbox-exercises PING (`PingIntervalSeconds=1`); Unity added for `ws_handle_heartbeat_timer` (healthy/pong-timeout/stale), `ws_arm_heartbeat_timer`, `ws_maybe_send_heartbeat_ping`
- VictoriaLogs HTTPS: `victoria_logs_send_http_post` uses OpenSSL TLS when `use_ssl` (SNI + peer verify); fail closed on handshake/I/O error (no plain-TCP downgrade); Unity asserts HTTPS against plain sink fails

---

## Status snapshot

| # | Item | Effort left | Done | Priority |
| --- | ------ | ------------- | ------ | ---------- |
| 1 | Auth Finale | XL | RP 1–26 + IdP 0–16 | P0 |
| 2 | Chat Finale | XL | Phases 1–12 + MCP 0–15 | P0 |
| 4 | Unity ASAN | M | 0% | P1 |
| 9 | DB queue health probe | S–M | ~40% | P1 |
| 9a | Config health SQL + bootstrap orphan DROP | S–M | hard-coded | P1 |
| 12 | DB fault tolerance (crash/transient) | L | partial | P1 |
| 12a | DQM child auto-scale (optional) | L | no-op by design | P1 |
| 12b | Scoreboard waiter condvar wake | M | poll only | P1 |
| 12d | MailRelay Persist MySQL/MariaDB SEGV | M | bind fix; live test_58 TBD | P1 |
| 12e | MAX+1 PK clients: confirm + retry | M | single-thread OK | P1 |
| 13 | Mail Relay remainder | L–XL | ~75% | P2 |
| 25 | SchemaHelper TUI | L | v1 | P2 |
| 24 | `H.externaldb` — ad-hoc external DB from Lua | M | 0% | P2 |
| 19 | Print job → device / Beryllium | L–XL | ~30% | P3 |
| 22 | Mirage | XL | 0% | P3 |
| 23 | Reserved enums/fields | n/a | n/a | P3 |

(End of file)
