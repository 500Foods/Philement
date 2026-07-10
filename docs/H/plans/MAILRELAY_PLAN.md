# Mail Relay Subsystem Implementation Plan

## Purpose

Define a gated, phase-by-phase, operational plan for implementing Hydrogen's Mail Relay subsystem from its current skeleton state to production-ready outbound mail, templated mail, authenticated API support, OTP/MFA support, queue persistence, observability, and optional inbound relay support.

This document is the working document for the implementation. It is meant to be edited as we go. Each phase is numbered, focused, and gated. Do not start a phase until the previous phase's exit gate is green. Record learnings in the Working Log so later phases benefit from earlier discoveries.

## How To Use This Document

- Work one phase at a time, top to bottom. Each phase has an Objective, an Entry Gate, numbered work items, and an Exit Gate.
- Each work item has an explicit verification (a `mkt` build, a named `mku <base>` Unity test, a named blackbox test, or a `mkp`/lint run).
- Mark an item `[x]` only when its verification has actually passed. If an item is deferred, change it to `[~]` and add a one-line rationale and the phase it moves to.
- After each phase, fill in the phase Status block (date, result, variances) and append any reusable discovery to the Working Log.
- Never mark a phase complete with a failing or skipped gate unless explicitly recorded as deferred.

## Resuming Work on This Plan

CURRENT PAUSE POINT (as of 2026-07-10): **Phase 7 complete.** Next work is **Phase 7A** (Lua `H.mail` / `H.notify` backfill). **First chunk locked: 7A.1 + 7A.2 together** — implement when resuming (decisions already reviewed; no re-plan required unless you change them).

### Resume here next session

1. Read this pause point + **Phase 7A implementation vector** (below) + Working Log decisions dated 2026-07-10 (7A contract).
2. Baseline if touching C: `zsh -ic 'mkt'` then optional
   `zsh -ic 'mku mailrelay_send_test && mku mailrelay_preview_test'`.
3. Implement **7A.1 + 7A.2 only** (contract, `H.wait` MAIL/NOTIFY wiring, real `H.mail.send` via `mailrelay_send_template`). Do **not** start 7A.3–7A.6 in the same chunk.
4. Verify with `mkt` + focused Unity (`scripting_api_mail_test` or similar). Update this plan Status/checkboxes. Stop for review.
5. Swagger already done (tag **"Mail Service"**; `/mailrelay/*` in `payloads/swagger.json`).

### Session checklist (every Mail Relay return)

1. Confirm the latest completed phase via **Phase Status** blocks; active phase = first not complete.
2. Re-read **Working Log** decisions/surprises that affect the next chunk.
3. `zsh -ic 'mkt'`; relevant `mku` targets; inspect `src/mailrelay/` and `tests/unity/src/mailrelay/`.
4. One small chunk: ask qualifying questions → present plan → implement → verify → update this plan → stop for review.

Build aliases: `zsh -ic '<alias>'` (`mkt`, `mka`, `mku <base>`, `mkp`, `mks`).

## Scope And Repo Areas

Primary repo area: `/elements/001-hydrogen/hydrogen`

Related repo areas:

- `/elements/002-helium/acuranzo/migrations` for application database migrations and QueryRefs.
- `/elements/003-lithium/src/managers/mail-manager` for the future Mail Manager UI.
- `/elements/003-lithium/src/managers/profile-manager/pages/email` for user profile email settings.
- `/elements/001-hydrogen/hydrogen/src/scripting` for the existing Lua `H.mail` / `H.notify` stubs that must be backfilled once Mail Relay has a real producer API.

Date of snapshot: 2026-07-06 prep review update; original snapshot 2026-06-29

---

## Current Observed State

Hydrogen has a Mail Relay subsystem that is a complete config/launch/landing skeleton with thorough config validation, but the actual relay engine is entirely stubbed. There is no SMTP sender, no worker threads, no queue producer/consumer, no templates, no API, and no database tables.

### Existing Hydrogen source pieces (verified)

- Config struct: `MailRelayConfig` in `/elements/001-hydrogen/hydrogen/src/config/config_mail_relay.h` (struct at lines 43-54). Fields: `Enabled`, `ListenPort`, `Workers`, `QueueSettings Queue` (`MaxQueueSize`, `RetryAttempts`, `RetryDelaySeconds`), `OutboundServerCount`, `OutboundServer Servers[MAX_OUTBOUND_SERVERS]`.
- `OutboundServer` struct (`config_mail_relay.h` lines 27-33): `char* Host`, `char* Port` (string for env-var support), `char* Username`, `char* Password`, `bool UseTLS`. There is no per-server TLS mode, CA path, auth-mode, or timeout field yet.
- Config loader/dump/cleanup: `load_mailrelay_config`, `dump_mailrelay_config` (masks password as `*****`), `cleanup_mailrelay_config`, `cleanup_server` in `/elements/001-hydrogen/hydrogen/src/config/config_mail_relay.c`.
- `MailRelayConfig mail_relay` is a member of `struct AppConfig` at `/elements/001-hydrogen/hydrogen/src/hydrogen.h:142` (config section "L. Mail Relay").
- Defaults function `initialize_config_defaults_mail_relay()` at `/elements/001-hydrogen/hydrogen/src/config/config_defaults.c:361-387`, called from the master initializer at line 66.
- Launch handler: `/elements/001-hydrogen/hydrogen/src/launch/launch_mail_relay.c`. `check_mail_relay_launch_readiness()` (lines 20-151) is real and validates network dependency, enabled flag, port, workers, queue settings, server count, and each server's host/port/username/password. `launch_mail_relay_subsystem()` (lines 153-164) only resets the shutdown flag, logs, and returns `1`. TODO marker at line 162.
- Landing handler: `/elements/001-hydrogen/hydrogen/src/landing/landing_mail_relay.c`. `check_mail_relay_landing_readiness()` always returns ready with "System under development" messages. `land_mail_relay_subsystem()` (lines 57-68) only sets `mail_relay_system_shutdown = 1` and logs. No thread join, no queue drain.
- Shutdown flag: `volatile sig_atomic_t mail_relay_system_shutdown` defined at `/elements/001-hydrogen/hydrogen/src/state/state.c:50`; externed in `state.h:46`, `launch/launch.h:80`, `registry/registry_integration.h:112`. Read in `registry/registry_integration.c:99`.
- Name constant: `SR_MAIL_RELAY "MailRelay"` at `/elements/001-hydrogen/hydrogen/src/globals.h:92`.
- Limits: `MAX_OUTBOUND_SERVERS 5` at `/elements/001-hydrogen/hydrogen/src/globals.h:282`. Adjacent SMTP bounds at `globals.h:284-289`: `MIN_SMTP_PORT 1`, `MAX_SMTP_PORT 65535`, `MIN_SMTP_TIMEOUT 1`, `MAX_SMTP_TIMEOUT 300`, `MIN_SMTP_RETRIES 0`, `MAX_SMTP_RETRIES 10`.
- Queue memory metrics: `QueueMemoryMetrics mail_relay_queue_memory` defined at `/elements/001-hydrogen/hydrogen/src/utils/utils_queue.c:17`, externed in `utils_queue.h:46`, initialized in `utils.c:24`, configured in `utils.c:51`, reported in `launch.c:463` and `launch.c:470`, exported in `status/status_process.c:412-416`. This is plumbed but unused by any relay logic.
- Registry decls already present: `extern int init_mail_relay_subsystem(void);` and `extern void shutdown_mail_relay(void);` at `/elements/001-hydrogen/hydrogen/src/registry/registry_integration.h:132` (implementations do not exist).
- Runtime directory `/elements/001-hydrogen/hydrogen/src/mailrelay/` exists but is empty.
- libcurl is already a Hydrogen dependency. Global init in `launch.c:324` (`curl_global_init`), global cleanup in `state.c:149`. It supports SMTP/SMTPS and is the first outbound transport candidate.
- OpenSSL, jansson, libmicrohttpd, libwebsockets, Lua, and libcurl are already detected by CMake.
- Current CMake uses recursive source discovery for `../src/*.c` in `cmake/CMakeLists-init.cmake`, so new `src/mailrelay/*.c` files should be discovered automatically; verify during Phase 2 rather than assuming manual CMake registration is needed.

### Confirmed defaults discrepancy (must reconcile in Phase 0)

Two divergent default sets exist:

- `initialize_config_defaults_mail_relay()` (`config_defaults.c:361`): `Enabled = false`, `ListenPort = 25`, `Workers = 2`, `OutboundServerCount = 1` (host `localhost`, port `587`, no creds, `UseTLS = true`).
- Inline defaults inside `load_mailrelay_config()` (`config_mail_relay.c:38-118`): `Enabled = true`, `ListenPort = 587`, `Workers = 2`, and if no servers configured it creates `OutboundServerCount = 2` env-var placeholder servers (`${env.SMTP_SERVER1_*}`, `${env.SMTP_SERVER2_*}`).

The existing Unity config tests assert the loader's values (enabled, 587, 2 servers). Reconciling these will require updating those tests. Recommended target: disabled by default, no required SMTP env vars at startup.

### Existing Notify SMTP config (parallel, separate)

- `NotifyConfig notify` is a member of `struct AppConfig` at `/elements/001-hydrogen/hydrogen/src/hydrogen.h:147` ("P. Notify").
- `SMTPConfig` (`/elements/001-hydrogen/hydrogen/src/config/config_notify.h:16-25`): `char* host`, `int port`, `char* username`, `char* password`, `bool use_tls`, `int timeout`, `int max_retries`, `char* from_address`. Note: different field naming (snake_case) and typing (`int port`) than MailRelay's `OutboundServer`.
- `NotifyConfig` (`config_notify.h:28-32`): `bool enabled`, `char* notifier`, `SMTPConfig smtp`.

This is a duplicate SMTP intent. Phase 0 must decide whether Notify becomes a producer that calls Mail Relay, or whether the two coexist.

### Existing Unity tests for Mail Relay (verified)

- `/elements/001-hydrogen/hydrogen/tests/unity/src/config/config_mail_relay_test_load_mailrelay_config.c` (load/cleanup/dump; asserts loader defaults enabled/587/2 servers).
- `/elements/001-hydrogen/hydrogen/tests/unity/src/launch/launch_mail_relay_test_check_mail_relay_launch_readiness.c` (basic readiness, no mocks).
- `/elements/001-hydrogen/hydrogen/tests/unity/src/launch/launch_mail_relay_test_comprehensive_coverage.c` (uses `mock_launch` + `mock_system`; covers valid/null/disabled/invalid-port/invalid-workers/invalid-queue/invalid-retry/invalid-server-count/missing host/port/username/password, plus launch success and shutdown-flag).
- `/elements/001-hydrogen/hydrogen/tests/unity/src/landing/landing_mail_relay_test_land.c` and `.../landing_mail_relay_test_readiness.c`.

Run any of these with `mku <base name without .c>`, for example: `mku config_mail_relay_test_load_mailrelay_config`.

### Existing database state (verified)

- Migrations are Lua, located in `/elements/002-helium/acuranzo/migrations/`, named `acuranzo_NNNN.lua` (sequence from `1000`; highest existing is `1200`).
- A generic polymorphic `templates` table exists: `/elements/002-helium/acuranzo/migrations/acuranzo_1020.lua` (`template_id`, `entity_type_a9`, `status_a12`, `entity_id`, `name`, `summary`, `collection`). Do not overload it for mail bodies.
- The `queries` table (bootstrap migration `acuranzo_1000.lua`) holds QueryRefs (named/numbered SQL retrieved at runtime by reference).
- No mail/SMTP/relay/inbox/outbound table or QueryRef exists. The DB side is greenfield.

### Existing client/UI pieces

- Lithium has a placeholder `MailManager` that renders "under development".
- Profile email settings are stubbed and do not load/save via API.

### Existing Lua scripting mail/notify pieces (verified 2026-07-06)

- Lua plan status is complete in `/docs/H/plans/LUA_PLAN_COMPLETE.md`; Phase 19 intentionally installed stable `H.mail` / `H.notify` stubs because Mail Relay did not exist yet.
- Stub implementation: `/elements/001-hydrogen/hydrogen/src/scripting/scripting_api_mail_notify.c`. `H.mail.send`, `H.mail.send_sync`, `H.notify.send`, and `H.notify.send_sync` currently return handles/errors with `"mail: not implemented"` / `"notify: not implemented"`.
- `H_lua_install_mail_notify(L)` is called from `/elements/001-hydrogen/hydrogen/src/scripting/lua_context.c`, so the Lua API surface already exists and must be backfilled rather than newly designed.
- Handle support already exists in `/elements/001-hydrogen/hydrogen/src/scripting/scripting_handle.{c,h}` via `H_HK_MAIL`, `H_HK_NOTIFY`, `mail_error`, and `notify_error` fields.
- Backfill target: once Mail Relay has a stable internal enqueue/producer API, replace `H.mail` stubs with a templated-mail queue producer and make `H.notify` either call Mail Relay through notification rules or remain an explicit compatibility shim until the Notify subsystem is retired.

### Confirmed test slots and ports (verified)

- Blackbox test numbers `57`, `58`, `59` are free (highest in the 50s is `test_56_cap_query.sh`; nothing in 57-69 except `test_60_performance.sh`).
- Port scheme is `5<TT>x` with a 10-port range per test. Proposed allocations: Test 57 -> 5570-5576, Test 58 -> 5580-5586, Test 59 -> 5590-5596.

### First design decisions to confirm during Phase 0

- Mail Relay is a Hydrogen subsystem, not a Conduit query feature.
- Outbound mail should use libcurl SMTP/SMTPS first unless a later gate proves it insufficient.
- Mail templates and durable mail state should use new mail-specific tables, not the generic `templates` table.
- Inbound SMTP relay is later-phase work and must not be enabled before outbound, auth, rate limits, and anti-open-relay gates are green.

---

## Reference Conventions

These are the verified, established patterns to follow. Match them exactly so new code passes cppcheck and integrates with the launch/landing/registry/thread machinery.

### Subsystem source layout

Follow the `src/mdns/` split (cleanest "service with threads" example): a public `<name>.h`, a private `<name>_internal.h`, a lifecycle `<name>.c`, a `<name>_threads.c` (worker loops), a `<name>_shutdown.c` (drain + free), and feature files such as `<name>_smtp.c`. Each `.c` begins with `#include <src/hydrogen.h>` first, then grouped local includes. Use `<src/folder/...>` include style, never `"../../.."`.

### Launch / landing handlers

Per launch file (see `/elements/001-hydrogen/hydrogen/src/launch/launch_websocket.c` and `launch_print.c`):

- `LaunchReadiness check_<name>_launch_readiness(void)` builds a dynamic message array with `add_launch_message(&messages, &count, &capacity, strdup(...))`, first message is the subsystem name (`SR_MAIL_RELAY`), then `"  Go:      ..."` / `"  No-Go:   ..."` lines, final `"  Decide:  Go For Launch of ..."`, then `finalize_launch_messages(...)`, returns `(LaunchReadiness){ .subsystem, .ready, .messages }`.
- `int launch_<name>_subsystem(void)` returns `1` for success, `0` for failure. Real launchers call `init_service_threads(...)`, spawn threads with `pthread_create` + `add_service_thread(...)`, then `update_subsystem_on_startup(SR_..., true)` and verify `get_subsystem_state(...) == SUBSYSTEM_RUNNING`.
- Launch dispatch by name is in `/elements/001-hydrogen/hydrogen/src/launch/launch.c:174-190` (already dispatches `SR_MAIL_RELAY`). Readiness registration is in `launch_readiness.c`.

Per landing file (see `/elements/001-hydrogen/hydrogen/src/landing/landing_websocket.c` and the drain loop in `src/mdns/mdns_server_shutdown.c:48-70`):

- `check_<name>_landing_readiness(void)` and `int land_<name>_subsystem(void)`.
- Drain pattern: set the `volatile sig_atomic_t` shutdown flag, then poll `update_service_thread_metrics(&<name>_threads)` until `thread_count == 0` or timeout, then `remove_service_thread(...)` / `init_service_threads(...)` to reset tracking and free resources.

### Thread tracking API

From `/elements/001-hydrogen/hydrogen/src/threads/threads.h`:

- Declare a global `ServiceThreads mailrelay_threads;` (define it in `src/state/state.c`, extern in `threads.h`, count it in `report_thread_status`).
- `init_service_threads(&threads, SR_MAIL_RELAY)`, `add_service_thread(&threads, pthread_self())` at worker entry, `remove_service_thread(&threads, pthread_self())` at worker exit, `update_service_thread_metrics(&threads)` to reap and count.
- `MAX_SERVICE_THREADS` is 1024 (`globals.h:29`).

### Registry and state

- State enum (`src/state/state_types.h:18`): `SUBSYSTEM_INACTIVE/_READY/_STARTING/_RUNNING/_STOPPING/_ERROR`.
- `LaunchReadiness` struct (`state_types.h:28`): `{ const char* subsystem; bool ready; const char** messages; }`.
- `register_subsystem(name, threads, main_thread, shutdown_flag, init_fn, shutdown_fn)` (`registry.h:77`).
- Helpers (`registry_integration.h`): `add_dependency_from_launch`, `update_subsystem_on_startup`, `update_subsystem_on_shutdown`, `get_subsystem_id_by_name`, `is_subsystem_running_by_name`, `get_subsystem_state`.
- Limits: `MAX_SUBSYSTEMS 19` (already includes Mail Relay and Scripting), `MAX_DEPENDENCIES 20` (`globals.h:117-118` in current tree).

### API endpoint module

Layout: `src/api/<service>/<endpoint>/<endpoint>.c/.h` (model on `src/api/conduit/`). Flow:

- Register endpoints in `/elements/001-hydrogen/hydrogen/src/api/api_service.c:117` (`register_api_endpoints`) and add string dispatch in `handle_api_request` (around `api_service.c:709-726`).
- Handler signature: `enum MHD_Result handle_<x>_request(struct MHD_Connection*, const char* url, const char* method, const char* upload_data, size_t* upload_data_size, void** con_cls)`.
- POST buffering: `api_buffer_post_data(...)` -> `ApiBufferResult` (`api_utils.h:226`), free with `api_free_post_buffer`.
- Method validation: `handle_method_validation(connection, method)`.
- JSON parse: `api_parse_json_body(ApiPostBuffer*)` -> `json_t*` (jansson).
- JWT: `extract_and_validate_jwt(auth_header, &result)` (`src/api/auth/auth_service_jwt.h`) after checking the `Authorization` header for a `Bearer` token prefix; or `api_extract_jwt_claims(connection, jwt_secret)`.
- Send: `api_send_json_response(connection, json_t*, status_code)` (takes ownership).
- Error envelope: builders in `src/api/conduit/helpers/error_handling.c` (`success:false`, `error`, `message`) and `api_send_error_and_cleanup(connection, con_cls, msg, http_status)`.

### libcurl pattern (thread-safe)

Per-request easy handle (model on `src/api/conduit/helpers/cap_verify.c:70-134`): `curl_easy_init`, set `CURLOPT_*`, always set `CURLOPT_NOSIGNAL = 1L` (required for worker threads), set `CURLOPT_CONNECTTIMEOUT` / `CURLOPT_TIMEOUT`, `curl_easy_perform`, read `CURLINFO_RESPONSE_CODE`, `curl_slist_free_all`, `curl_easy_cleanup`. For SMTP use `smtp://host:port` + STARTTLS or `smtps://host:port`, set `CURLOPT_MAIL_FROM`, `CURLOPT_MAIL_RCPT` (a `curl_slist`), `CURLOPT_USERNAME`/`CURLOPT_PASSWORD`, and `CURLOPT_READFUNCTION`/`CURLOPT_READDATA`/`CURLOPT_UPLOAD = 1L` to stream the RFC 5322 payload. A reusable wrapper like `oidc_rp_http.c`'s `apply_common_curl_opts()` is a good model.

### Logging

`void log_this(const char* subsystem, const char* format, int priority, int num_args, ...)` (`src/logging/logging.h:27`). `num_args` must equal the number of `%` specifiers. Subsystem is always an `SR_*` constant; use `SR_MAIL_RELAY`. Levels (`globals.h:132-138`): `LOG_LEVEL_TRACE 0`, `DEBUG 1`, `STATE 2`, `ALERT 3`, `ERROR 4`, `FATAL 5`, `QUIET 6`. `LOG_LINE_BREAK` is a separator. Never log credentials, OTP plaintext, or full message bodies except under an explicit debug/test-only flag.

### Database migrations and QueryRefs

In `/elements/002-helium/acuranzo/migrations/`:

- Add new migrations as `acuranzo_NNNN.lua` after `1200`. Each returns `function(engine, design_name, schema_name, cfg)` and typically inserts forward, reverse, and diagram rows into `queries`.
- Use cross-engine type macros (resolved per engine in `database_postgresql.lua`, `database_mysql.lua`, `database_sqlite.lua`, `database_db2.lua`): `${INTEGER}`, `${INTEGER_SMALL}`, `${INTEGER_BIG}`, `${TEXT}`, `${TEXT_BIG}`, `${VARCHAR_50/100/500}`, `${JSON}`, `${TIMESTAMP_TZ}`, `${NOW}`, `${PRIMARY}`, `${UNIQUE}`, `${SERIAL}`.
- Inject audit columns with `${COMMON_CREATE}` (`valid_after`, `valid_until`, `created_id/at`, `updated_id/at`) and use the companion `${COMMON_INSERT}` / `${COMMON_FIELDS}` / `${COMMON_VALUES}` / `${COMMON_DIAGRAM}` macros.
- Lookup-typed columns use the `_aNN` suffix referencing Lookup #NN (for example `status_a12`), flagged `"lookup": true` in the diagram JSON.
- QueryRefs are rows in `queries` (see `acuranzo_1198.lua` for an INSERT QueryRef example using `:PARAM` binds and `${INSERT_KEY_*}` engine-abstracted returning). Internal mail QueryRefs must use `internal_sql`/`system_sql`/`system_ddl` types, never `public`/`protected`, so they are not reachable via Conduit endpoints.
- Every Lua file needs the header block with `-- luacheck:` directives and a dated `-- CHANGELOG`. Run `tests/test_98_luacheck.sh` on Lua changes.

### Blackbox test conventions

In `/elements/001-hydrogen/hydrogen/tests/`:

- New script `test_NN_descriptive_name.sh` with header: shebang, title/description, `# FUNCTIONS`, `# CHANGELOG` (newest first), `set -euo pipefail`, then the config block (`TEST_NAME`, `TEST_ABBR`, `TEST_NUMBER`, `TEST_COUNTER=0`, `TEST_VERSION`), then `source .../lib/framework.sh` and `setup_test_environment`.
- Config files live at `tests/configs/hydrogen_test_<NN>_<name>.json`.
- Lifecycle helpers (`tests/lib/lifecycle.sh`): `find_hydrogen_binary`, `validate_config_file`, `start_hydrogen_with_pid` (waits for `"STARTUP COMPLETE"`), `stop_hydrogen` (SIGINT + `monitor_shutdown`), `check_time_wait_sockets`.
- Assertions/reporting (`framework.sh` / `log_output.sh`): `print_subtest`, `print_command`, `print_result <TEST_NUMBER> <TEST_COUNTER> <0pass|1fail> "msg"`, `print_test_completion`. Use the framework `${GREP}`/`jq` wrappers; use `jq` for all JSON.
- Update `CHANGELOG`/`TEST_VERSION` at the top of any script you change. Adding new tests in the free 57-59 slots avoids the renumbering checklist. Add a matching `docs/H/tests/test_NN_*.md` and run `test_04_check_links.sh` after adding docs.

### Unity test conventions

In `/elements/001-hydrogen/hydrogen/tests/unity/src/` (mirrors `src/`):

- File naming `<source>_test_<function>.c`, one function under test per file, unique filenames.
- Skeleton: `#include <src/hydrogen.h>`, `#include <unity.h>`, module header, forward declarations of functions and tests, `setUp`/`tearDown`, `test_*`, `main` with `UNITY_BEGIN` / `RUN_TEST` / `UNITY_END`.
- No `.c` includes in tests/mocks (header-only). Tests are auto-discovered by CMake (no manual registration). Keep tests fast (no `sleep`).
- Globally predefined mocks (do not redefine): `USE_MOCK_LOGGING`, `USE_MOCK_LIBMICROHTTPD`, `USE_MOCK_SYSTEM`. Per-file mocks enabled via `#define USE_MOCK_X` and reset in `setUp`/`tearDown`: `mock_network`, `mock_libwebsockets`, `mock_launch`, `mock_landing`, `mock_status`, `mock_info`.
- Run with `mku <base>`; the base maps 1:1 to the file basename without `.c`. Avoid static functions so they remain testable.

---

## Gate Philosophy

- One file change or one logical behavior per item where practical.
- After every C change: run `mkt`. Then `mka` once `mkt` is clean.
- When adding or changing a unit-testable component: run the matching `mku <base>`.
- After each phase: run `mkp` (cppcheck), the relevant `mku` set, and the relevant blackbox slice.
- For database migration changes: run `test_98_luacheck.sh` and at least one engine migration path before marking the gate complete.
- No SMTP credential, OTP plaintext, JWT, or mail body should appear in normal logs, config dumps, test artifacts, or failure output.
- Mark a phase complete only when every listed gate has passed, or the item is explicitly deferred with rationale.

### Proposed new blackbox tests (slots confirmed free)

- `tests/test_57_mailrelay_outbound.sh` (ports 5570-5576) - local SMTP sink + outbound send/queue tests.
- `tests/test_58_mailrelay_api.sh` (ports 5580-5586) - authenticated API, preview, template send, status.
- `tests/test_59_mailrelay_inbound.sh` (ports 5590-5596) - inbound listener/routing/no-open-relay tests.

Only create these when their phase reaches its blackbox item (per project rule: only create test scripts when needed by the work at hand).

---

## Working Log (cross-phase memory)

Append discoveries, surprises, and decisions here as we move through phases. Earlier-phase learnings that affect later phases must be recorded so they are not lost.

### Decisions log

- (Phase 7A contract, 2026-07-10, LOCKED — not coded yet) First chunk is **7A.1 + 7A.2 together**. (1) Lua `idempotency_key` is **optional**; auto-generate UUID when omitted (REST still requires it). (2) `H.notify` v1 returns a **stable deferred error** (no silent drop, no channel→template map in 7A). (3) `H.mail` is template-first (`template` or `template_key`); reject raw subject/body-only. (4) Success wait result: `{ message_id, status = "queued" }`. (5) Wire `H.wait` for `H_HK_MAIL`/`H_HK_NOTIFY` in the same chunk as real `H.mail.send`. Full vector under "Phase 7A implementation vector".
- (Phase 7.6, 2026-07-10) Swagger public tag is **"Mail Service"** (not "Mail Relay Service"); internal subsystem name/paths remain `mailrelay` / `SR_MAIL_RELAY`. Annotations live on `send.h` / `preview.h` / `status.h` plus `mailrelay_service.h` for the tag description. Document-as-implemented with full stable `MAIL_*` error codes. Regenerated swagger + payload; `test_22_swagger.sh` green.
- (Phase 7.5b, 2026-07-08, PAUSE) Role authorization design locked before the break. (1) The JWT `roles` claim stays canonical: a comma-separated list of **role_id integers** (not role names). (2) The mailrelay API authorizes by the `mail_send` **role_id**, resolved at request time via a **new QueryRef #127 "Get Role By Name"** (migration `acuranzo_1260.lua`) — NOT a hardcoded `"1"` constant. (3) The QueryRef #017 loader (`roles_from_database` in `oidc_rp_roles.c`) becomes a **non-static shared** function reused by the password login path. (4) Work split: Chunk 1 = auth roles fix; Chunk 2 = `test_58` hang hardening; then 7.6 Swagger. NONE of this is coded yet — see the "Phase 7.5b resume plan" under Phase 7.
- (Phase 7.5b Chunk 1, 2026-07-08) Chunk 1 implementation completed. `auth_roles_from_database()` is declared in `auth_service.h` and defined in `oidc_rp_roles.c`; the existing OIDC test seam continues to cover it. The mailrelay role check is implemented in a new shared file `src/api/mailrelay/mailrelay_api_auth.{c,h}` and resolves the role name through the Mail Relay repository seam (QueryRef #127) so endpoint unit tests stay DB-free. Password login now loads roles before JWT generation, matching OIDC semantics.
- (Phase 7.5b Chunk 2 follow-up, 2026-07-09) `test_58_mailrelay_api.sh` hang hardening implemented: replaced migration-completion wait with canonical `READY FOR REQUESTS`, reduced `STARTUP_TIMEOUT` to 15s, added 30s `READY_TIMEOUT`. Lint/link checks pass. However, running the matrix exposes a Hydrogen crash during `/api/auth/login` when multiple engine variants run in parallel. The crash signature is `Query result not found for signaling` immediately followed by `free(): invalid pointer` and a SIGABRT/SIGSEGV in the crash handler. A single Hydrogen instance started manually with the same config logs in successfully and returns a JWT with the correct `roles: "1"` claim, so the crash is concurrency-related (likely in the database pending-result manager or query-result cache) rather than a simple logic bug in `auth_roles_from_database()`. `test_40_auth.sh` shows the same class of segfaults when run in its normal 7-engine parallel mode, confirming the issue is not specific to `test_58`. This crash is the new blocker before 7.5b can be marked `[x]`.
- (Phase 7.5b auth recovery, 2026-07-09) Fixed three auth regressions that blocked Test 40 / Test 58:
  1. **Unity JWT helper contract:** `send_jwt_error_response()` was flipped to `MHD_YES` in commit `8fa9d81f5`. Intermediate helpers treat `MHD_NO` as "stop; error already queued"; top-level handlers return `MHD_YES` to MHD. Reverted helper to `MHD_NO`; mailrelay `send`/`preview`/`status` now call the helper then `return MHD_YES`. All five previously failing Unity suites pass.
  2. **Pending-result race in `execute_auth_query`:** submitted before registering the waiter (unlike conduit/mailrelay/scripting). Fast workers signaled "Query result not found for signaling" and freed orphan results (`free(): invalid pointer`). Fixed to register → submit → wait, matching `mailrelay_repository`.
  3. **`is_token_revoked` polarity:** QueryRef #018 returns a row when the token is *valid*. The old path never set `row_count` so revocation was always false. After copying `row_count`, polarity was inverted and every renew/logout failed with "Token has been revoked". Fixed to `revoked = !has_valid_row` with JSON-array fallback for engines that only return `data_json`.
  4. **`test_40` logout token:** renew deletes the old token; logout must use the renewed JWT. Script now writes `JWT_RENEWED_TOKEN=` and logout uses it.
- (Phase 7.5b CLOSED green, 2026-07-09) User confirmed after schema regen: `test_40`, `test_57`, `test_58` all pass; Unity, shellcheck, cppcheck, Test 90, Test 99 pass. Phase 7.5b is complete. Next item: 7.6 Swagger.
- (Phase 7.5b test_58 fail-soft, 2026-07-09) `test_58_mailrelay_api.sh` v2.2.0: parallel jobs no longer abort the suite under `set -e`. Root cause of "abrupt stop with no console messages" was `run_engine_test` returning 1 on variant failure, then `wait "${pid}"` (and `wait -n` in the job limiter) inheriting that status and exiting the parent before `analyze_engine_results` / `print_test_completion`. Fix: early failure paths in `run_mailrelay_variant` write `VARIANT_*_FAIL` and return 0; `run_engine_test` always returns 0 after writing `ENGINE_TEST_COMPLETE` or `ENGINE_TEST_FAILED`; `wait -n` / `wait pid` use `|| true`. Failures still set `EXIT_CODE=1` via analysis.
- (Phase 7.5b SQLite same-second validity, 2026-07-09) Root cause of remaining SQLite Test 40 renew/logout and Test 58 SQLite-STARTTLS failures: QueryRefs used `valid_after < ${NOW}` / `valid_until > ${NOW}`. SQLite `CURRENT_TIMESTAMP` is second-resolution text, so a token stored and validated in the same second fails the strict `<` check (0 rows → treated as revoked). Confirmed with an in-memory repro (`strict_lt=0`, `lte=1`). Server engines pass because higher-resolution timestamps or extra latency avoid the same tick. **Not a macro/`TRFS` issue** — `${NOW}` only expands the clock expression; the comparison operator is written in each QueryRef. Later migrations (1117/1135/1158) already used `<=`/`>=`. Fix: patch original migration sources (greenfield). Files: `acuranzo_1092` (#001), `1093` (#002), `1094` (#003), `1099` (#008), `1100` (#009), `1102` (#011), `1108` (#017), `1109` (#018 Validate JWT), `1193` (#082). All now use `valid_after <=` / `valid_until >=`. Schema regen required so applied `queries.code` rows pick up the change.
- (Phase 7.2, 2026-07-08) `POST /api/mailrelay/send` auth policy: requires a valid JWT with a database claim AND the `mail_send` role. `GET /api/mailrelay/status` continues to require only a valid JWT with a database claim. This prevents arbitrary JWT holders from using the server as a mail gateway.
- (Phase 7.1, 2026-07-08) API permission model for Phase 7: any valid JWT with a database claim may access `/api/mailrelay/status`; no role check. Raw subject/body send is rejected outright; `template_key` is required. Full status counters (queued, sending, sent, failed, retrying, permanent_failures, last_success, last_failure, worker_count, queue_depth) are implemented now rather than deferred to Phase 10.
- (Phase 7.1, 2026-07-08) Status endpoint method validation: `validate_http_method()` only permits POST, so `handle_mailrelay_status_request()` performs its own GET-only check instead of using `handle_method_validation()`.
- (Phase 6, 2026-07-08) Event rule execution model: `MailRelay.Events.Rules` keeps the existing `event_key → script_name` mapping. Phase 6.1a implements built-in default Lua handlers for `system.server_started` and `system.server_stopped`; custom DB-loaded scripts come in Phase 6.1b. Handlers return a mail-request table (template_key, to/cc/bcc, params, etc.) and C dispatches via `mailrelay_send_template()`.
- (Phase 6, 2026-07-08) Event rate limiting: per-event-key fixed-window counter with `MaxEventsPerInterval` (default 10) and `EventIntervalSeconds` (default 60). Rate-limit state lives in `MailRelayRuntime` and is freed on shutdown.
- (Phase 6, 2026-07-08) Startup/shutdown trigger points: `system.server_started` is emitted after the canonical `READY FOR REQUESTS` log in both the async database path (`database_signal_ready_if_complete`) and the no-database path in `launch.c`. `system.server_stopped` is emitted from `land_mail_relay_subsystem()` before `mailrelay_shutdown()` so Mail Relay can still enqueue.
- (Phase 6, 2026-07-08) Event persistence: events are not written to `mail_events`; they enqueue directly through the existing producer API, per user direction.
- (Phase 0, 2026-07-06) Notify vs MailRelay SMTP ownership: Mail Relay is the only SMTP/mail delivery subsystem. Notify becomes a producer/compatibility layer that calls Mail Relay later; do not build a second active SMTP delivery path in Notify.
- (Phase 0, 2026-07-06) Durable-mail database target policy: add `MailRelay.Database` as the configured Hydrogen database connection for system mail tables, durable queue state, templates, attempts, events, and OTP rows. API/Lua callers may carry JWT/database context for authorization and audit, but durable mail state is owned by `MailRelay.Database`.
- (Phase 0, 2026-07-06) Default enabled/port reconciliation: Mail Relay is disabled by default and must not require SMTP environment variables at startup. Outbound servers are required only when outbound sending is enabled. Submission/listen port settings apply only when inbound relay is enabled.
- (Phase 0, 2026-07-06) Initial API send policy: v1 is template-key send only for external clients. Raw subject/body send is reserved for internal/system test paths or a future explicitly gated admin permission.
- (Phase 0, 2026-07-06) Lua backfill policy: existing `H.mail` / `H.notify` stubs are a stable surface. After Phase 7, replace `H.mail` stubs with a Mail Relay queue producer and make `H.notify` a compatibility shim that routes through Mail Relay rules or returns a documented deferred error.
- (Phase 0, 2026-07-06) Initial role policy: service/internal callers and administrators may send/preview/status initially. User-facing sends are template-scoped and permission-scoped; exact JWT claim names are locked during Phase 7 against the auth code that exists then.
- (Phase 0, 2026-07-06) Initial OTP scope: Phase 8 targets login MFA and email verification first. Password reset can use the same primitives but is not a Phase 8 gate unless the auth owner confirms it before Phase 8 starts.
- (Phase 0, 2026-07-06) Canvas/inbound workflow: inbound SMTP relay waits until outbound/API/OTP foundations are stable. v1 inbound scope is trusted/internal submission only, never public MX/open-relay behavior.
- (Phase 0, 2026-07-06) Events config format: `MailRelay.Events.Rules` is a JSON object mapping event keys to Lua script names. The Lua scripts receive event details and determine recipients/behavior; this defers mail recipient routing logic to Lua rather than hardcoding it in C config.
- (Phase 1, 2026-07-06) Disabled subsystem semantics: disabled Mail Relay returns `ready=true` with a "clean skip" readiness message, not a No-Go. This allows launch planning to proceed without treating disabled subsystems as failures.
- (Phase 1, 2026-07-06) Server validation gating: outbound server host/port/username/password validation happens only when `OutboundEnabled=true`. Inbound-only configurations don't need SMTP credentials at launch.
- (Phase 1, 2026-07-06) Database defaulting: `MailRelay.Database` defaults to the single configured database connection name automatically when only one database exists and no explicit MailRelay.Database is set.
- (Phase 2, 2026-07-06) No Python in the mail test path. The local mail validator is a C program under `extras/mailval/` (long-term reusable multi-protocol fixture for SMTP/IMAP/JMAP), not a Python script. Built with its own standalone CMakeLists (OpenSSL for TLS); it is not part of Hydrogen's recursive source discovery. TLS (STARTTLS + implicit TLS) is included now, not deferred.
- (Phase 2, 2026-07-06) Email validation is a dedicated `mailrelay_is_valid_email()` in the mailrelay module (modeled on the auth `is_valid_email` rules but decoupled from `src/api/auth`); mailrelay must not depend on the auth subsystem for validation.
- (Phase 2, 2026-07-06) Raw send path is seeded as a real internal `mailrelay_send_raw()` API (becomes the Phase 3 producer) and triggered in the blackbox test via a `MailRelay.Test.SendRawOnLaunch` config flag, rather than a throwaway self-test. The sink can be smoke-tested with our own `mailrelay_smtp` helper once 2.4 lands.
- (Phase 2, 2026-07-06) IMAP and JMAP modules are implemented as full, functional servers in Phase 2 (not scaffolds), backed by a shared in-memory mailbox store that SMTP delivery populates. This avoids rework when inbound (Phase 12) and JMAP access land later.
- (Phase 2, 2026-07-06) libcurl SMTP ambiguity: `TLSMode == MAIL_TLS_MODE_NONE` with `UseTLS == true` maps to `smtp://` with STARTTLS (not `smtps://`), because explicit `TLSMode` values take precedence and legacy `UseTLS` is only a backward-compatible hint that defaults to STARTTLS when no explicit mode is set.
- (Phase 2, 2026-07-06) `mailrelay_smtp` owns rendering: callers pass `MailRelayMessage + OutboundServer`, and `mailrelay_smtp` calls `mailrelay_render_message` internally. This keeps the public API minimal and ensures secrets/body redaction, seam injection, and policy are centralized in one place.
- (Phase 5, 2026-07-08) Template syntax locked: `%MACRO%` for variables, `%MACRO|default%` for optional defaults, `%%` for a literal percent sign. Built-in macros: `%APP_NAME%`, `%SERVER_NAME%`, `%TIMESTAMP%`, `%USER_EMAIL%`, `%REQUEST_ID%`, `%OTP_CODE%`. No automatic HTML/text escaping: callers/API/Lua must sanitize values before passing them to the engine. Missing required macros cause a hard `MAIL_PARAM_MISSING` error unless a default is provided. `%OTP_CODE%` is rendered as plaintext into the message body; log redaction policy already prevents secret exposure.

- (Phase 7.5a, 2026-07-08) Preview endpoint and macro tracking: `POST /api/mailrelay/preview` requires the `mail_send` role, accepts `template_key` + optional `params`, and returns rendered `subject`, optional `text_body`/`html_body`, resolved `from`/`reply_to`, and a deduplicated `macros_used` array. The macro engine gained `MailRelayMacroList`, `mailrelay_template_render_with_macros()`, and `mailrelay_template_preview_with_macros()`; the original render/preview functions remain unchanged wrappers so existing callers and tests are unaffected. Preview is rejected when Mail Relay is disabled and performs no queue/SMTP writes.

- (Phase 7.5b data, 2026-07-08) Added Acuranzo migrations 1257–1259 to give the blackbox test an authenticated, authorized user and a renderable template: migration 1257 inserts the `mail_send` role (role_id 1), migration 1258 inserts a `mailadmin` account (account_id 5) with username/email contacts and an `account_roles` mapping, and migration 1259 seeds the `mail.test` template. The `mailadmin` password comes from `HYDROGEN_MAILADMIN_PASS` so the test harness can log in without a production secret. Verified by `helium_update.sh`, `test_98_luacheck.sh`, and `test_34_sqlite_migrations.sh` (257 migrations reversed). The next sub-chunk is the `test_58_mailrelay_api.sh` script itself.

- (Phase 7.5b test design, 2026-07-08) Environment variables for the `mailadmin` account are now part of the test environment: `HYDROGEN_MAILADMIN_NAME="mailadmin"`, `HYDROGEN_MAILADMIN_PASS="thisisonlyatest"`, `HYDROGEN_MAILADMIN_EMAIL="mailadmin@500nodes.com"`. The blackbox script must read these env vars directly and must not hardcode defaults or fallback values. The test should follow the existing multi-engine pattern (`test_30_database.sh` / `test_40_auth.sh`) and cover all seven engines plus both plaintext and STARTTLS SMTP variants against `mailval`. Known test data: role_id=1, account_id=5, contact_id 7 (username) and 8 (email), template_id=1, template_key=`mail.test`. The `mail.test` template renders `%NAME%`, `%APP_NAME%`, `%SERVER_NAME%`, and `%TIMESTAMP%`, so a successful preview/send can assert on both the rendered output and the `macros_used` list.

- (Phase 3.2, 2026-07-06) In-memory queue design: specialized `MailRelayQueue` in `mailrelay_queue.{c,h}` rather than the generic `src/queue/queue.{c,h}` because mail items need deep-copied `MailRelayMessage`, priority, attempts, and next-attempt metadata. Queue is priority-ordered (higher first, FIFO within same priority), bounded by `Queue.MaxInMemory`, supports timed blocking dequeue and shutdown broadcast. Status enum `MailRelayStatus` added with `OK`, `INVALID_ARGS`, `QUEUE_FULL`, `SHUTDOWN`, `TIMEOUT`. `mailrelay_message_copy()` added to support deep-copy enqueue without exposing message internals.

- (Phase 3.3, 2026-07-07) Worker pool and producer API: `mailrelay_workers.{c,h}` spawn detached worker threads that register/unregister with `mailrelay_threads`, dequeue via the bounded queue, send via `mailrelay_send_raw` (server[0] for now), and update counters under the runtime mutex. `mailrelay_enqueue()` is the stable internal producer API; it validates the message, deep-copies into the queue, and returns `MailRelayStatus`. `MailRelayStatus` expanded to include `MAILRELAY_DISABLED`. Launch now starts workers and routes `SendRawOnLaunch` through the async enqueue path instead of a synchronous send. The `test_57_mailrelay_outbound.sh` log assertion was updated from "SendRawOnLaunch success" to "SendRawOnLaunch smoke test enqueued" to match the new async behavior; sink-side delivery verification remains the primary gate.

- (Phase 3.6, 2026-07-07) Registry alignment: launch registers Mail Relay via `register_subsystem_from_launch` with `mailrelay_threads`, `mail_relay_system_shutdown`, `launch_mail_relay_subsystem`, and `land_mail_relay_subsystem`; updates registry to `SUBSYSTEM_RUNNING` and verifies state; failure paths update registry to `SUBSYSTEM_ERROR` and shut down. Landing updates registry through `SUBSYSTEM_STOPPING` to `SUBSYSTEM_INACTIVE` around `mailrelay_shutdown()`. This matches the mDNS server pattern.

- (Phase 3.6, 2026-07-07) Retry/DB coordination note: Phase 3.4 will implement an in-memory retry scheduler (transient classification + exponential backoff) for the current single-instance queue. Durable queue state, retry records, and multi-instance coordination will be handled in Phase 4 (`mail_queue`/`mail_attempts` tables) and Phase 11 (atomic claim-next-pending across instances). The in-memory scheduler is intentionally the v1 path; it will later be backed by DB persistence rather than replaced by it.

- (Phase 3.4, 2026-07-07) Retry implementation: added `MailRelayResult.retryable` so the transport decides retry eligibility once, using the actual libcurl `CURLcode` and SMTP reply code. `mailrelay_retry.{c,h}` centralizes backoff policy (`InitialDelaySeconds * 2^attempt` capped at `MaxDelaySeconds`) and `should_retry` logic. `mailrelay_queue_dequeue` was made time-aware: it scans the priority-ordered list for the highest-priority due item and blocks until the earliest `next_attempt_at` or a new arrival, so future retries do not block due items. This design mirrors the future DB-backed queue (`WHERE next_attempt_at <= now ORDER BY priority DESC`). A new `mailrelay_queue_enqueue_scheduled` carries explicit `attempts` and `next_attempt_at` for re-enqueue. Retry tests use `InitialDelaySeconds=0` so retries are immediate and deterministic.
- (Phase 3.5, 2026-07-07) Debounce/coalescing design: added `MailRelayMessage.debounce_key` as the explicit grouping key. `src/mailrelay/mailrelay_debounce.{c,h}` maintains a bounded in-memory pending map (max `MAILRELAY_DEBOUNCE_MAX_PENDING`) and a dedicated expiry thread. Repeated submissions with the same key within `Queue.DebounceSeconds` update the pending entry's count and refresh the expiry window; when the window closes, a single coalesced message is enqueued with `%COUNT%` replaced by the arrival count and `%SUMMARY%` replaced by a pluralized summary (e.g., "5 events"). `mailrelay_enqueue()` routes messages with a non-empty `debounce_key` through the debounce path when enabled; otherwise they go straight to the queue. Shutdown flushes pending debounce entries so no event mail is lost. The expiry thread is lazily started on the first debounce submission (rather than inside `mailrelay_init()`) so tests and configurations that never use debounce see no extra background thread.
- (Phase 4, 2026-07-07) Schema numbering and design lock: next Acuranzo migration is 1211, next lookup is 063, next QueryRef is 093. All mail-specific enumerated columns get new lookups; no existing lookups are reused. `mail_templates` uses `status_a64`, `mail_queue` uses `status_a63`, `mail_events` uses `status_a65`, `mail_otp_codes` uses `purpose_a66` and `status_a67`, `mail_routes` uses `status_a68`. Tables created in this phase: `mail_templates`, `mail_queue`, `mail_attempts`, `mail_events`, `mail_otp_codes`, `mail_routes`. Queue bodies stored as structured fields only (no duplicate rendered payload). `mail_queue` includes `instance_id` and `claim_token` now to support Phase 11 HA without a later ALTER.

- (Phase 4A, 2026-07-07) Lookups 063–068 created and indexed. Migrations `acuranzo_1211.lua` through `acuranzo_1216.lua` define Mail Queue Status (pending/sending/sent/failed/retrying), Mail Template Status (inactive/active/deprecated), Mail Event Status (pending/queued/sent/failed/suppressed), Mail OTP Purpose (login_mfa/email_verify/password_reset), Mail OTP Status (active/consumed/expired/max_attempts_exceeded), and Mail Route Status (inactive/active). `elements/002-helium/scripts/helium_update.sh` regenerated `elements/002-helium/acuranzo/README.md` and link checks passed; `tests/test_98_luacheck.sh` is green.

- (Phase 4B.1, 2026-07-07) Core mail tables created. `acuranzo_1217.lua` creates `mail_templates` (`template_id` application-generated integer primary key, `template_key` unique, `status_a64`, `subject_template`, `text_template`, `html_template`, `collection` JSON, `${COMMON_CREATE}`). `acuranzo_1218.lua` creates `mail_queue` (`queue_id` application-generated integer primary key, `message_uuid` unique, `status_a63`, structured body fields, `idempotency_key`, `attempts`, `next_attempt_at`, `instance_id`, `claim_token`, indexes on `(status_a63, next_attempt_at)`, `idempotency_key`, and `(instance_id, claim_token)`). Both use `${INTEGER}` primary keys per project convention (no engine autoincrement). Diagram migrations included. Verified by `tests/test_34_sqlite_migrations.sh` (217 migrations reversed), `tests/test_98_luacheck.sh`, and `helium_update.sh`.

- (Phase 4B.2, 2026-07-07) Audit/event tables created. `acuranzo_1219.lua` creates `mail_attempts` (`attempt_id` application-generated integer primary key, `queue_id` logical FK, `attempt_number`, `server_index`, `success`, `smtp_code`, `smtp_text`, `duration_ms`, `error_class`, index on `queue_id`). `acuranzo_1220.lua` creates `mail_events` (`event_id` application-generated integer primary key, `event_key`, `status_a65`, structured body/template fields, `params_json`, `debounce_key`, `idempotency_key`, `priority`, `queue_id`, `processed_at`, indexes on `(status_a65, created_at)` and `event_key`). Both use `${INTEGER}` primary keys and include diagram migrations. Verified by `tests/test_34_sqlite_migrations.sh` (221 migrations reversed), `tests/test_98_luacheck.sh`, and `helium_update.sh`. Note: running the SQLite migration test concurrently with luacheck caused an OOM kill; sequential execution is required.

- (Phase 4B.3, 2026-07-07) OTP and inbound-route tables created. `acuranzo_1221.lua` creates `mail_otp_codes` (`otp_id` application-generated integer primary key, `code_hash`, `email`, `account_id`, `purpose_a66`, `status_a67`, `expiry_at`, `attempts`, `max_attempts`, `consumed_at`, indexes on `(email, purpose_a66, status_a67)` and `(status_a67, expiry_at)`). `acuranzo_1222.lua` creates `mail_routes` (`route_id` application-generated integer primary key, `status_a68`, `source_network`, sender/recipient domain/pattern fields, `auth_required`, `require_tls`, `template_key`, `rewrite_from`, `rewrite_to`, `add_recipients_json`, `priority`, `sort_seq`, indexes on `(status_a68, sender_domain, sort_seq)` and `(status_a68, recipient_domain, sort_seq)`). Both use `${INTEGER}` primary keys and include diagram migrations. With this, all six Phase 4B tables and all Phase 4A lookups are complete. Verified sequentially by `tests/test_98_luacheck.sh` (248 files) and `tests/test_34_sqlite_migrations.sh` (221 migrations reversed).

- (Phase 4C planning, 2026-07-07) Split Phase 4C into four sub-chunks for manageable review: 4C.1 queue/attempts/templates QueryRefs (093–107, migrations 1223–1237), 4C.2 event QueryRefs (108–111, migrations 1238–1241), 4C.3 OTP QueryRefs (112–117, migrations 1242–1247), 4C.4 inbound route QueryRefs (118–122, migrations 1248–1252). Phase 4B validation tests to be completed before starting 4C.1.

- (Phase 4C.1a, 2026-07-07) Queue lifecycle QueryRefs 093–101 created as migrations 1223–1231. All are `query_type_a28 = ${TYPE_INTERNAL_SQL}` (value 0) so they are unreachable from REST/Conduit. Read queries use `${QTC_FAST}` and write/update queries use `${QTC_SLOW}`. QueryRef 096 is a plain `SELECT` for the next pending due row, not an atomic claim, to remain engine-agnostic; the C repository helper will call QueryRef 097 to mark the row `sending`. Application-generated `queue_id` values use the standard `${INSERT_KEY_*}` macros. `helium_update.sh` regenerated the README index and `tests/test_98_luacheck.sh` is green. The SQLite/server-engine migration reverse test is intentionally left for the user to run before the next sub-chunk.

- (Phase 4C.1b, 2026-07-07) Attempts and template QueryRefs 102–107 created as migrations 1232–1237. QueryRef 102 inserts `mail_attempts` rows with application-generated `attempt_id`; QueryRefs 103–104 read `mail_templates` by key or list active templates; QueryRefs 105–107 insert, update, and soft-delete templates. All continue the `TYPE_INTERNAL_SQL` / `QTC_FAST` for reads / `QTC_SLOW` for writes conventions established in 4C.1a. `helium_update.sh` and `tests/test_98_luacheck.sh` are green. Phase 4C.1 is now complete; the remaining 4C sub-chunks are events (4C.2), OTP (4C.3), and inbound routes (4C.4).

- (Phase 4C.2, 2026-07-07) Event QueryRefs 108–111 created as migrations 1238–1241. QueryRef 108 inserts pending `mail_events` rows with application-generated `event_id`; QueryRef 109 lists pending events in creation order; QueryRef 110 marks an event processed/queued with a parameterized `status_a65` and `queue_id`; QueryRef 111 marks an event suppressed (`status_a65 = 4`). Conventions match 4C.1. `helium_update.sh` and `tests/test_98_luacheck.sh` are green. Remaining 4C sub-chunks are OTP (4C.3) and inbound routes (4C.4).

- (Phase 4C.3, 2026-07-07) OTP QueryRefs 112–117 created as migrations 1242–1247. QueryRef 112 inserts `mail_otp_codes` rows with application-generated `otp_id`; QueryRef 113 retrieves the active OTP by email and purpose; QueryRef 114 consumes an active OTP; QueryRef 115 increments the attempt counter; QueryRef 116 expires OTPs past their `expiry_at`; QueryRef 117 fetches an OTP by primary key. Conventions match 4C.1. `helium_update.sh` and `tests/test_98_luacheck.sh` are green. The final 4C sub-chunk is inbound routes (4C.4).

- (Phase 4C.4, 2026-07-07) Inbound route QueryRefs 118–122 created as migrations 1248–1252. QueryRef 118 retrieves active routes by sender domain; QueryRef 119 lists all active routes; QueryRef 120 inserts a route with application-generated `route_id`; QueryRef 121 updates a route; QueryRef 122 soft-deletes a route. Conventions match 4C.1. With this, all Phase 4C core QueryRefs (093–122) are complete. `helium_update.sh` and `tests/test_98_luacheck.sh` are green. The remaining Phase 4 work is operational cleanup QueryRefs (Phase 4D) and repository C code (Phase 4E), after the user verifies migrations and QTC loading.

- (Phase 4D, 2026-07-07) Operational cleanup QueryRefs 123–126 created as migrations 1253–1256. QueryRef 123 deletes terminal-status (`sent`/`failed`) `mail_queue` rows older than `:CUTOFF_AT`; QueryRef 124 deletes terminal-status (`sent`/`failed`/`suppressed`) `mail_events` rows older than `:CUTOFF_AT`; QueryRef 125 deletes `mail_attempts` rows older than `:CUTOFF_AT`; QueryRef 126 deletes terminal-status (`consumed`/`expired`/`max_attempts_exceeded`) `mail_otp_codes` rows older than `:CUTOFF_AT`. All are `query_type_a28 = ${TYPE_INTERNAL_SQL}` and `QTC_SLOW`. Using `:CUTOFF_AT` instead of `:RETENTION_DAYS` keeps the SQL engine-agnostic because the existing macro set does not provide portable date-interval arithmetic; the C repository layer will compute the cutoff timestamp from the configured retention period. `helium_update.sh` regenerated the README index; `tests/test_98_luacheck.sh` (282 files) and `tests/test_34_sqlite_migrations.sh` (253 migrations reversed) are green.

- (Phase 4E.1, 2026-07-07) Implemented `src/mailrelay/mailrelay_repository.{c,h}` with callback-based helpers for all QueryRefs 093–126. The default executor resolves the Mail Relay database (explicit `MailRelay.Database` or single-configured database fallback), looks up the QueryRef in the QTC via `query_cache_lookup` (internal queries are not blocked), submits the query through the database queue, waits synchronously on the pending result manager, and invokes the caller's callback. The API is callback-based (callers provide a `mailrelay_repo_callback_fn`), but the default executor is synchronous because the existing database infrastructure is synchronous from the caller's perspective; this satisfies the async-callback API requirement while remaining compatible with the current DQM. A swappable `mailrelay_repo_execute_fn` seam lets unit tests mock execution without a live database. `mku mailrelay_repository_test` has 11 passing tests covering the seam, representative helper parameter JSON, and NULL-argument rejection. `mkt` and `mkp` are green. The next chunks are 4E.2 (startup recovery) and 4E.3 (enqueue persistence).
- (Phase 4E.2, 2026-07-07) Startup recovery implemented in `src/mailrelay/mailrelay.c`. `mailrelay_recover_stale_sending_rows()` runs from `mailrelay_init()` when `Queue.Persist=true` and a database target is configured, calling QueryRef 101 with a `:STALE_BEFORE` cutoff computed as `now - Queue.StaleTimeoutSeconds`. A new `MailRelay.Queue.StaleTimeoutSeconds` config field was added with a default of 300 seconds. The repo helper parameter was renamed from `STALE_CUTOFF_AT` to `STALE_BEFORE` to match migration `acuranzo_1231.lua`. Verified by `mku mailrelay_persistence_test`.
- (Phase 4E.3, 2026-07-07) Enqueue persistence implemented. `mailrelay_enqueue()` writes a pending `mail_queue` row via QueryRef 093 before placing the message in the in-memory queue. An internal `mailrelay_enqueue_to_queue()` helper is used by both the direct enqueue path and the debounce flush path so coalesced messages are also persisted. `MailRelayMessage` was extended with `template_key` and `headers` fields to support future template sends. If persistence fails, the function returns `MAILRELAY_PERSIST_FAILED` and the message is not enqueued. Verified by `mku mailrelay_persistence_test` (6 tests) and existing queue/debounce tests.
- (Phase 4E utility cleanup, 2026-07-07) Moved `generate_uuid()` from `src/webserver/web_server_upload.{c,h}` to a new common utility `src/utils/utils_uuid.{c,h}` so Mail Relay and other subsystems can use it without depending on webserver internals. Exposed `format_iso_time()` in `src/utils/utils_time.h` for ISO 8601 timestamp generation in the repository and recovery code. All existing callers were updated and the build/tests remain green.

### Surprises / deviations from plan

- (Phase 7.5b closeout, 2026-07-09) Lessons learned while making Test 40/58 fully green (carry these forward for Phase 7A/8/12 and any future JWT-gated API):
  1. **JWT `roles` claim is role_id integers** (e.g. `"1,3"`), NOT role names. Gate by resolving name → `role_id` (QueryRef #127) then matching the id. Same trap will hit Lithium role UI, OTP endpoints, inbound relay auth if they compare names.
  2. **Password login ≠ OIDC login for claims.** OIDC loads roles via QueryRef #017 before `generate_jwt()`; password login did not until Chunk 1. Always test auth-dependent features through the real login path the blackbox uses, not only OIDC or hand-crafted Unity JWTs.
  3. **Unity fixtures must mirror production claim format.** Fixtures with `"mail_send"` as a role string passed while e2e failed. Prefer role_id strings and mock QueryRef #127.
  4. **Sync DB helpers: register-pending → submit → wait.** `execute_auth_query` submitted first; under parallel multi-engine load, workers signaled "Query result not found for signaling" and freed orphans (`free(): invalid pointer`). Match `mailrelay_repository` / conduit order.
  5. **MHD return polarity is layered.** Intermediate JWT helpers return `MHD_NO` ("stop; error already queued"); top-level handlers return `MHD_YES` so MHD delivers the response. Flipping the helper to `MHD_YES` broke five Unity suites and made callers continue with NULL JWT state.
  6. **QueryRef #018 polarity:** a returned row means the token is *valid* (present, not expired). After fixing `row_count` propagation, inverted logic made every renew/logout look revoked. Rule: `revoked = !has_valid_row` (with `data_json` array fallback when engines omit `row_count`).
  7. **Renew deletes the old token.** Logout (and any follow-on call) must use the *renewed* JWT. Blackbox scripts must capture `JWT_RENEWED_TOKEN` from renew response, not reuse the login token.
  8. **SQLite second-resolution timestamps + strict `<`/`>` on `valid_after`/`valid_until`.** Same-second store+validate returns 0 rows → "revoked". Prefer `<=` / `>=` (already used in later migrations). **Macros (`${NOW}`, `TRFS`) do not fix operators** — they only expand the clock expression. Applied QueryRefs store pre-expanded SQL; greenfield can patch original migrations; production would need a rewrite migration.
  9. **Parallel blackbox under `set -e`:** background jobs that `return 1` make `wait` / `wait -n` kill the parent mid-suite with little console output. Encode pass/fail in result files and always `return 0` from parallel workers; analyze results in the parent. Use `wait … || true`.
  10. **Readiness wait:** prefer canonical log line `READY FOR REQUESTS` over long migration-completion polls; keep per-variant timeouts short so missing engines fail fast.
  11. **Migration numbering:** always re-derive next-free migration/QueryRef from disk (`ls migrations`, Acuranzo README), not from plan prose. As of 7.5b close: highest migration used for mail/auth work is **1260** (QueryRef #127); next free is **1261** / next free QueryRef depends on README after any later inserts.
  12. **SQLite isolation:** copy baseline DB + WAL/SHM sidecars into a per-variant temp path; never share one SQLite file across parallel Hydrogen instances.
- (Phase 0, 2026-07-06) `examples/configs/hydrogen_default.json` and `hydrogen_env.json` use legacy `MailRelay.QueueSettings` / `MailRelay.OutboundServers` keys, while the current loader and Phase 0 schema use `MailRelay.Queue` / `MailRelay.Servers`. Reconcile example configs and/or compatibility aliases during Phase 1.
- (Phase 1, 2026-07-06) Existing launch readiness tests assumed disabled = No-Go; Phase 1 changed this to clean skip and required test updates in `launch_mail_relay_test_comprehensive_coverage.c`. Server validation tests also needed `OutboundEnabled=true` because validation is now gated behind the outbound flag.
- (Phase 1, 2026-07-06) Landing tests could not mock `is_subsystem_running_by_name`; reduced landing test scope to the realistic not-running path and no-dependents message verification.
- (Phase 2, 2026-07-06) `extras/mailval/` build/debug lessons (all now fixed in the tool): (1) a network server MUST `signal(SIGPIPE, SIG_IGN)` or a single write to a client-closed socket kills the whole process; (2) the shared store mutex must be recursive because callers lock it and then call `store_*` helpers that also lock; (3) the per-session capture mutex must be unlocked on every path (a missing `pthread_mutex_unlock` deadlocks the next write on that session); (4) TLS reads must retry on `SSL_ERROR_WANT_READ`/`WANT_WRITE` rather than treating them as EOF; (5) `ncat` piped stdin does not always deliver an entire multi-command script before the server finishes responding — drive multi-step protocol tests with a real client (a throwaway Python script is acceptable for local validation; the committed blackbox tests must use our own C tool, per the no-Python rule).
- (Phase 2, 2026-07-06) `mailval.pem` / `mailval.key` are generated locally by `gen_cert.sh` and must never be committed or logged (the private key is secret).
- (Phase 2.4, 2026-07-06) libcurl SMTP delivery: the server's protocol replies (220/250/354/221) arrive via `CURLOPT_HEADERFUNCTION`, NOT the body `CURLOPT_WRITEFUNCTION` — the message body still uploads through `CURLOPT_READFUNCTION`. Pointing both header and write callbacks at the same capture buffer is what lets `mailrelay_smtp` read the final SMTP code; with only the write callback, `resp.len` stays 0 and the send is misreported as failed even though the message was accepted. Flat `smtp_write_cb` returning `realsize` works for both.
- (Phase 2.4, 2026-07-06) `mailrelay_smtp` seam design: the wrapper renders internally and resolves config into a `MailRelaySmtpRequest` (url scheme, use_ssl, envelope from, to+cc+bcc recipient list, creds, timeout, auth_mode, rendered payload) handed to a swappable `mailrelay_smtp_transport_fn`. Unit tests assert on the request without the network; the default transport is real libcurl. The live test (`mailrelay_smtp_live_test.c`, gated by `MAILRELAY_LIVE_SMTP`) exercises the real transport so the `CURLOPT_*` set stays honest. cppcheck is green after using a const-compatible response callback signature.
- (Phase 3.1, 2026-07-06) Runtime ownership: `MailRelayRuntime` is a heap-allocated global owned by `mailrelay_init()`/`mailrelay_shutdown()` and declared in `mailrelay_internal.h`. This keeps the runtime internal to the mailrelay module while still allowing unit tests to inspect it via the internal header, matching the mDNS instance pattern without exposing a subsystem pointer in `state.c`.
- (Phase 3.1, 2026-07-06) Async producer contract: `mailrelay_send_raw` remains the direct SMTP transport implementation used by worker threads. The public producer API will be an explicit `mailrelay_enqueue` (Phase 3.7) that accepts the same message shape and returns a queued status, so callers never block on SMTP. The `SendRawOnLaunch` test trigger will be rerouted through the queue once it exists.
- (Phase 3.1, 2026-07-06) Launch/landing lifecycle convention: `launch_mail_relay_subsystem()` calls `mailrelay_init()` and `land_mail_relay_subsystem()` calls `mailrelay_shutdown()`. Registration via `register_subsystem_from_launch` and `update_subsystem_on_startup` verification will be aligned with the mDNS pattern in Phase 3.6.
- (Phase 2.6, 2026-07-06) `test_57_mailrelay_outbound.sh` is now the committed blackbox gate for raw outbound delivery. It runs plaintext SMTP on 5570 and STARTTLS SMTP on 5571 against `extras/mailval`, then verifies `SendRawOnLaunch success` and a stored SMTP transcript with matching subject.
- (Phase 2.6, 2026-07-06) `mailval` EHLO multiline capability order matters: `STARTTLS` must be advertised as a continued `250-STARTTLS` line before the final `250 AUTH ...` line, or libcurl treats EHLO as complete and never upgrades.

### Reusable snippets / gotchas

- **Auth / JWT (Phase 7.5b):**
  - Role gates: resolve name → role_id (QueryRef #127 / repository seam), then match id in JWT `roles` claim (`"1,3"` style).
  - Password login must call `auth_roles_from_database()` before `generate_jwt()` (same as OIDC).
  - Sync query pattern: `pending_result_register` → `database_queue_submit_query` → wait (never submit-first).
  - `is_token_revoked`: QueryRef #018 row present ⇒ valid; `revoked = !has_valid_row`.
  - Temporal validity SQL: `valid_after <= ${NOW}` and `valid_until >= ${NOW}` (not strict inequalities).
  - MHD: helpers return `MHD_NO` on queued errors; top-level handlers return `MHD_YES`.
- **Parallel blackbox scripts:** workers write PASS/FAIL to result files and return 0; parent `wait … || true` then analyzes; use `READY FOR REQUESTS` for readiness.
- A standalone C test tool (like `mailval`) builds cleanly with its own `CMakeLists.txt` (OpenSSL + jansson + pthreads) and is exercised by: (1) build it, (2) `bash gen_cert.sh`, (3) run it in the background on test ports, (4) drive a real client, (5) assert on the per-session JSON transcripts it writes. This avoids Python in the committed path while still giving deterministic, inspectable validation.
- libcurl SMTP requires `CURLOPT_UPLOAD=1L` + `CURLOPT_READFUNCTION` streaming and `CURLOPT_NOSIGNAL=1L` in worker threads. Confirm and record exact option set once Phase 2 works.
- Verified libcurl `CURLOPT_*` set for `mailrelay_smtp` (Phase 2.4, against `mailval` plaintext → code 250): `CURLOPT_URL` = `smtp://host:port` (or `smtps://` for `MAIL_TLS_MODE_SMTPS`); `CURLOPT_MAIL_FROM`; `CURLOPT_MAIL_RCPT` (curl_slist of to+cc+bcc); `CURLOPT_USERNAME`/`CURLOPT_PASSWORD` when set; `CURLOPT_LOGIN_OPTIONS` = `CRAM-MD5`/`PLAIN`/`NTLM`/`XOAUTH2` per `AuthMode`; `CURLOPT_USE_SSL` = `CURLUSESSL_TRY` (STARTTLS) or `CURLUSESSL_ALL` (STARTTLS_REQUIRED); `CURLOPT_CAINFO` when `CAPath` set; `CURLOPT_READFUNCTION`+`CURLOPT_READDATA`+`CURLOPT_UPLOAD=1L`+`CURLOPT_INFILESIZE` streaming the rendered RFC 5322 payload; `CURLOPT_CONNECTTIMEOUT`/`CURLOPT_TIMEOUT` = `TimeoutSeconds`; `CURLOPT_NOSIGNAL=1L`; and BOTH `CURLOPT_HEADERFUNCTION`+`CURLOPT_HEADERDATA` and `CURLOPT_WRITEFUNCTION`+`CURLOPT_WRITEDATA` pointed at the same response-capture buffer. The SMTP reply code is parsed from the captured server responses (last `3xx`/`2xx` line).
- Launch readiness pattern: for subsections with separate enable flags, validate required fields only when the enabling flag is true. This prevents false No-Go for inbound-only or feature-gated configurations.
- Landing readiness pattern: `is_subsystem_running_by_name` is not a mockable function in the current Unity setup; landing tests should focus on the not-running early-return path or test worker-drain messages through direct state setup without relying on the registry mock.
- Config field additions: add to `config_mail_relay.h` struct, `config_mail_relay.c` loader/cleanup/dump, `config_defaults.c`, `hydrogen_config_schema.json`, example configs, and Unity tests in lockstep. Cleanup must free all newly allocated strings/arrays before zeroing the struct.
- Thread tracking: declare `ServiceThreads mailrelay_threads;` in `src/state/state.c`, extern in `src/threads/threads.h`, report in `threads.c` `report_thread_status()`, and initialize in `launch_mail_relay_subsystem()` via `init_service_threads(&mailrelay_threads, SR_MAIL_RELAY)`.
- Schema maintenance: `tests/artifacts/hydrogen_config_schema.json` is hand-maintained; update it in the same change as config field changes. Run `tests/test_93_jsonlint.sh` after schema modifications.

### Phase 1 preparation notes

- Build aliases are defined for interactive shells. In non-interactive automation, run them through `zsh -ic '<alias>'`; direct `mkt` / `mku` shell calls may report `command not found` even though the user environment is correctly configured.
- If `mku <base>` reports `No rule to make target` after a clean or target-discovery change, run `mkt` first to regenerate/configure Unity targets, then rerun the exact `mku <base>` command.
- `mkt` is the lowest-cost required gate after each C change; `mka` and `mkp` should be run once the focused `mkt` / `mku` checks are clean.
- `tests/artifacts/hydrogen_config_schema.json` appears to be maintained directly; no schema generator was found during Phase 0. If Phase 1 adds or renames config fields, update this file in the same change and run `tests/test_93_jsonlint.sh`.
- The JSON schema requires a top-level `Server` object for standalone validation, so representative MailRelay schema checks need at least `{ "Server": {}, "MailRelay": { ... } }`.
- `tests/test_93_jsonlint.sh` schema-validates `tests/configs/hydrogen_test_*.json`, not `examples/configs/*.json`; example config drift can survive JSON lint unless explicitly reviewed.
- Keep `load_mailrelay_config()`, `initialize_config_defaults_mail_relay()`, Unity expectations, `hydrogen_config_schema.json`, and example configs synchronized whenever Phase 1 changes config field names or defaults.
- Existing examples used legacy `MailRelay.QueueSettings` / `MailRelay.OutboundServers` keys; Phase 1 updated all three example configs (`hydrogen_default.json`, `hydrogen.json`, `hydrogen_env.json`) to use the current `MailRelay.Queue` / `MailRelay.Servers` schema.

### Phase 7A implementation vector (locked 2026-07-10 — resume and code)

**Entry:** Phase 7 green. Producer: `mailrelay_send_template()`. Stubs: `src/scripting/scripting_api_mail_notify.c`.

**Decisions locked with user (do not re-ask unless changing them):**

| Topic | Choice |
|---|---|
| First ship | **7A.1 + 7A.2 together** (contract + wait wiring + real `H.mail.send`) |
| `idempotency_key` | **Auto-generate UUID if Lua omits it** (REST still requires explicit key) |
| `H.notify` v1 | **Stable deferred error** (e.g. `notify: deferred to mailrelay rules`); no silent success; no channel→template map in 7A |
| Delivery | Lua → **internal** `mailrelay_send_template` only (never HTTP/SMTP from scripts) |
| `send_sync` | Wait for **queue accept only**, not SMTP (7A.3; out of first chunk) |

**Lua `H.mail.send` contract (template-first; align MAIL_GUIDE, not legacy subject/body docs):**

```lua
H.mail.send({
  template = "mail.test",   -- or template_key (either accepted)
  to = "a@x.com" | { "a@x.com" },
  cc = ..., bcc = ...,      -- at least one of to/cc/bcc required
  params = { NAME = "Ada" },
  idempotency_key = "...",  -- optional; auto UUID if omitted
  priority = 0,             -- optional
}, opts?) -> handle

-- H.wait(handle) success: { message_id = "...", status = "queued" }, nil
-- H.wait(handle) error:   nil, "MAIL_...: ..."  (or mail_error string)
```

- Reject raw-only `{ subject, body }` without `template` / `template_key` (same policy as REST v1).
- Built-in macros (`APP_NAME`, `SERVER_NAME`, `TIMESTAMP`, etc.) filled from app/server context where available; no JWT required for in-process Lua (scripting is trusted host context).

**Code gaps to fix in first chunk:**

1. Stubs always set `mail_error` / `notify_error` to `"mail: not implemented"` / `"notify: not implemented"`.
2. `H_lua_mail_notify_wait_one` is declared in `scripting_api.h` but **not** dispatched from `H_lua_wait_one` in `scripting_api_query.c` — `H_HK_MAIL` / `H_HK_NOTIFY` currently fall through and mis-handle.
3. `H_Handle` only has `mail_error` / `notify_error` for mail/notify; success needs **message_id + status** (extend handle or free after packing into wait result).
4. `lua_api.md` still documents legacy `{to, subject, body, template?}` — update in **7A.6**, not first chunk.

**First chunk (7A.1 + 7A.2) — implement next:**

1. **7A.1** Contract + wait: parse/validation helpers; wire `H.wait` for MAIL/NOTIFY (`mail_error` / success payload); extend handle fields if needed.
2. **7A.2** Real `H.mail.send`: Lua table → `MailRelaySendTemplateRequest` → `mailrelay_send_template()`; auto UUID; success/error handle; **Unity with mocked producer seam** (no live SMTP/DB).

**Files (expected):** `scripting_api_mail_notify.c`, `scripting_api.h`, `scripting_handle.{c,h}`, `scripting_api_query.c` (`H_lua_wait_one`), optional producer test seam, `tests/unity/src/scripting/...` mail API test.

**Out of first chunk:** 7A.3 `send_sync` polish, 7A.4 notify deferred string (stubs may remain `"notify: not implemented"` until 7A.4), 7A.5 safety audit, 7A.6 docs.

**Later chunks:** 7A.3 → 7A.4 (deferred error text) → 7A.5 → 7A.6 (`lua_api.md`, LUA plan cross-ref).

**Verification (first chunk):** `mkt`; `mku` for new mail scripting test; no full `test_58` unless delivery behavior changes.

**Next free migration** after 1260 is **1261** if 7A/8 needs QueryRefs (re-check disk). No migration expected for 7A.1–7A.2.

**Process note:** for future public REST (OTP, inbound), prefer OpenAPI/contract before wiring when design-first docs are wanted.

### Phase 5 preparation notes

- `MailRelayMessage` now carries `template_key` and `headers`. The macro engine should render templates into `subject`, `text_body`, and `html_body`, set `template_key` to the resolved template key, and then call `mailrelay_enqueue()` (or the future `mailrelay_enqueue_template` wrapper) so persistence records the template association.
- Repository helpers for templates are already available: `mailrelay_repo_template_get_by_key` (QueryRef 103) and `mailrelay_repo_template_list_active` (QueryRef 104). The producer API should resolve templates through these before rendering.
- The internal persistence path is `mailrelay_enqueue()` -> `mailrelay_enqueue_to_queue()`. The templated-send producer should validate, render, and then call `mailrelay_enqueue()`; it should not bypass validation or debounce by talking directly to the queue.
- Preview must be side-effect free: render the template into a transient message and return the rendered subject/body without calling `mailrelay_enqueue()` or touching the database queue.
- Seed templates should use the same keys that the API and Lua will reference (e.g., `mail.test`, `system.server_started`, `auth.otp_code`). A migration can insert them via QueryRef 105 or direct SQL; the repository helper is the preferred C path.
- The macro engine should detect unknown/missing macros before rendering and return a stable error that the API maps to `MAIL_PARAM_MISSING`. The API error codes from the draft contract are `MAIL_AUTH_REQUIRED`, `MAIL_TEMPLATE_NOT_FOUND`, `MAIL_PARAM_MISSING`, `MAIL_QUEUE_FULL`, `MAIL_RECIPIENT_INVALID`, `MAIL_DISABLED`, and `MAIL_RATE_LIMITED`.
- Because `mailrelay_enqueue()` is now fail-fast on persistence errors, the templated-send producer must handle `MAILRELAY_PERSIST_FAILED` and return a suitable API error (`MAIL_QUEUE_FULL` or a generic internal error) without leaking database details.
- The live database persistence path has been verified only through the mock executor; a future blackbox test with `MailRelay.Queue.Persist=true` against a real SQLite/server engine database will be needed before claiming full production readiness.
- Launch readiness now treats disabled Mail Relay as a clean skip (`ready=true`, "disabled - clean skip") rather than a No-Go. Update any downstream scripts or tests that assert disabled subsystems cause launch failure.
- Server validation in launch readiness is gated behind `OutboundEnabled=true`; inbound-only configurations without outbound servers will not fail readiness checks.
- Landing readiness in tests cannot mock `is_subsystem_running_by_name`; test the not-running early-return path or setup `mailrelay_threads.thread_count` directly for worker-drain scenarios.
- When adding new fixed-size string arrays (like `AdminRecipients[]`), define a max constant in `globals.h` (e.g. `MAX_MAIL_RELAY_ADMIN_RECIPIENTS`), use `char*` fixed array + count in the struct, and free each element in cleanup before resetting count.
- New `ServiceThreads` globals must be: defined in `src/state/state.c`, externed in `src/threads/threads.h`, added to `report_thread_status()` in `src/threads/threads.c`, and initialized in the subsystem launch function.

### Phase 2 preparation notes

- The mail validator (`extras/mailval/`) is a separate binary built by its own `CMakeLists.txt` (linking OpenSSL); it is intentionally outside Hydrogen's recursive `../src/*.c` discovery so it is not linked into the server. Smoke-test it manually with `./mailval --smtp-port 5570 --tls-cert <pem> --tls-key <pem>` and, after 2.4, send to it with our own `mailrelay_smtp` helper.
- TLS is in scope now: the `tls.c` layer supports both STARTTLS (upgrade after handshake) and implicit TLS (listen on a TLS socket). Generate a self-signed cert/key in `tests/configs/` or `extras/mailval/` for the blackbox run; never log the private key. The validator must accept both plaintext and TLS so tests can exercise `MAIL_TLS_MODE_*` without external tooling.
- Design the protocol modules behind a common interface (`accept_connection`, `handle_session`, `free_session`) so IMAP (Phase 12) and JMAP (future) plug in without restructuring the listener. Capture everything to JSON so blackbox assertions stay tool-agnostic.
- `mailrelay_send_raw()` is the seed of the Phase 3 producer API: keep its signature stable so Phase 3 wraps it with queue/workers/retry rather than rewriting it. Phase 2 leaves the `launch_mail_relay_subsystem()` TODO mostly intact except for the gated `SendRawOnLaunch` call.
- `MailRelay.Test.SendRawOnLaunch` must be off by default and must not allocate workers, open queues, or require a database; it only renders + sends one canned message so production startup is unaffected.
- Deterministic render tests need a seam: `mailrelay_test_seams.{c,h}` should expose an injectable `time()`/Message-ID source (function pointers or a settable override) so `mku mailrelay_render_test` and `mku mailrelay_send_raw_test` are stable.
- Record the verified libcurl `CURLOPT_*` set in the Working Log Reusable snippets once 2.4 works, mirroring the existing libcurl pattern in `src/api/conduit/helpers/cap_verify.c`.
- First chunk learnings (2026-07-06): fixed `mv_xstrdup` → `strdup` in `mailrelay_message.c`; `mailrelay_is_valid_email` aligned to match auth validator exactly without the extra length cap; Unity test `mailrelay_message_test` added with 20 passing tests covering init/free, all recipient types, overflow, and every validation path.

### Working approach: prefer smaller, independently-verifiable chunks

- The Phase 2 effort started as one large drop (the full `mailval` multi-protocol tool plus the Hydrogen `mailrelay_*` modules in the same push). That is too much to review or bisect at once. Going forward, break each remaining phase item into a small chunk that has its own green gate before moving on:
  1. One module + its `mku` unit test, build-verified with `mkt`, before adding the next module.
  2. Config/launch wiring as its own chunk (build + existing `mku`/dump checks) before any new send path.
  3. The blackbox `test_57` as its own chunk after the send path is proven by `mku` and a manual `mailval` run.
- Keep the standalone `mailval` tool as the validation harness; invest in it first (done) so later chunks can validate quickly without re-deriving a fixture.
- After each chunk, update this Working Log (decisions, surprises, status) so the document reflects reality, not just intent.

---

## Phase 0 - Design Lock and Baseline Cleanup

Objective: Turn the existing skeleton into a documented, testable, unambiguous baseline before adding behavior.

Entry Gate: none (starting phase). Confirm `mkt` and `mku config_mail_relay_test_load_mailrelay_config` currently pass on the untouched tree to establish a baseline.

- [x] 0.1 Audit and reconcile `MailRelay` vs `Notify.SMTP` config intent.
  - Files to review: `src/config/config_mail_relay.{c,h}`, `src/config/config_notify.{c,h}`.
  - Deliverable: decision recorded in the Working Log decisions log.
  - Recommended decision: Mail Relay becomes the SMTP/mail delivery subsystem; `Notify` becomes a producer that calls Mail Relay later rather than owning SMTP delivery.
  - Verification: decision recorded; no ambiguous duplicate active SMTP delivery path remains.

- [x] 0.2 Decide the initial database target for durable mail state.
  - Options: a configured `MailRelay.Database` name, the first configured database marked for mail, or app/auth database from JWT for user-triggered sends.
  - Recommended v1: `MailRelay.Database` names a configured Hydrogen database connection for system queues/templates; API sends may carry JWT database context only for authorization/audit.
  - Verification: chosen policy recorded in the Working Log and reflected in Phase 4 and Phase 7 tasks.

- [x] 0.3 Reconcile the two divergent default sets.
  - Files: `src/config/config_defaults.c:361-387` (disabled/port 25/1 server) vs `src/config/config_mail_relay.c:38-118` (enabled/port 587/2 env placeholder servers).
  - Recommended v1: disabled by default; no required SMTP env vars at startup; outbound servers only required when outbound sending is enabled; submission port 587 only when inbound relay is enabled.
  - This will require updating the existing Unity assertions in `config_mail_relay_test_load_mailrelay_config.c` (currently assert enabled/587/2 servers).
  - Verification: `mkt`, then `mku config_mail_relay_test_load_mailrelay_config` passes with the reconciled defaults.

- [x] 0.4 Add complete `MailRelay` JSON schema coverage.
  - Files: `tests/artifacts/hydrogen_config_schema.json` (and schema generation source if one exists).
  - Verification: `tests/test_93_jsonlint.sh` passes and a representative MailRelay config validates against the schema.

- [x] 0.5 Finalize the subsystem file map (lock exact paths before coding).
  - Deliverable: confirm the file list below in the Subsystem File Map section; adjust if 0.1-0.3 change anything.
  - Verification: this document's Subsystem File Map reflects the agreed final paths.

- [x] 0.6 Lock the Lua backfill contract for existing `H.mail` / `H.notify` stubs.
  - Files to review: `src/scripting/scripting_api_mail_notify.c`, `src/scripting/scripting_api.h`, `src/scripting/scripting_handle.{c,h}`, `src/scripting/lua_context.c`, and `/docs/H/plans/LUA_PLAN_COMPLETE.md` Phase 19.
  - Deliverable: confirm that Mail Relay will expose an internal C producer API used by REST, Lua, Notify, and system events, so Lua does not call back through HTTP and does not bypass queue/audit/rate-limit logic.
  - Verification: Phase 7A exists in this plan with concrete work items and tests.

Exit Gate: defaults/behavior unambiguous; schema knows `MailRelay`; a disabled-by-default Hydrogen starts cleanly with no SMTP env vars required; Lua backfill path is locked; baseline Unity tests pass. Fill in the Status block below.

Phase 0 Status: complete. Date: 2026-07-06. Result: decisions, Lua backfill path, defaults reconciliation, schema coverage, and subsystem file map are locked; disabled-by-default startup is verified. Variances: added Phase 7A to backfill completed Lua stubs through Mail Relay; example config key name reconciliation deferred from Phase 0 and completed in Phase 1.

---

## Phase 1 - Config Model and Launch/Landing Readiness

Objective: Make Mail Relay fully configurable and launchable (and cleanly skippable when disabled) without sending mail yet.

Entry Gate: Phase 0 exit gate green.

- [ ] 1.1 Expand `MailRelayConfig` for outbound-only v1.
  - File: `src/config/config_mail_relay.{c,h}`.
  - Fields to add/consider: `Database`, `DefaultFrom`, `DefaultReplyTo`, `AdminRecipients[]`, `Outbound.Enabled`, per-server `TLSMode`/`CAPath`/`AuthMode`/`TimeoutSeconds`, `Queue.MaxInMemory`, `Queue.Persist`, `Queue.InitialDelaySeconds`, `Queue.MaxDelaySeconds`, `Queue.DebounceSeconds`, `Templates.ReloadIntervalSeconds`, `Events.Enabled` + rules. Keep existing field names where tests already depend on them or update tests in lockstep.
  - Verification: `mkt`.

- [ ] 1.2 Add env-var processing for all sensitive/deploy-specific fields.
  - Use existing `PROCESS_SENSITIVE` / `PROCESS_STRING` / `PROCESS_BOOL` / `PROCESS_INT` macros (see current `load_mailrelay_config`).
  - Verification: `mku config_mail_relay_test_load_mailrelay_config` (extended) verifies `${env.SMTP_*}` resolves and missing sensitive values fail only when the feature needing them is enabled.

- [ ] 1.3 Confirm dump behavior with secret redaction.
  - File: `dump_mailrelay_config` already masks password as `*****`; extend masking to any new secret fields.
  - Verification: a Unity dump test (new `config_mail_relay_test_dump_mailrelay_config` or extend existing) shows host/from but never passwords/tokens.

- [ ] 1.4 Strengthen launch readiness.
  - File: `src/launch/launch_mail_relay.c`.
  - Disabled Mail Relay must be a clean skip. Outbound-enabled requires at least one valid outbound server. Queue persistence requires a valid database target. Inbound-enabled requires a valid listen port and no open-relay mode.
  - Verification: `mku launch_mail_relay_test_comprehensive_coverage` extended to cover disabled, outbound-only, missing server, invalid port, bad retry settings, and database-missing cases.

- [ ] 1.5 Strengthen landing readiness.
  - File: `src/landing/landing_mail_relay.c`.
  - Landing must reflect whether workers exist, whether queue drain is pending, and whether forced shutdown is needed (replace the static "under development" messages).
  - Verification: `mku landing_mail_relay_test_readiness` covers no-workers and synthetic-workers cases.

Exit Gate: Mail Relay can be disabled or configured outbound-only without startup surprises; readiness messages explain exactly why launch is allowed or blocked. `mkt`, `mkp`, and the three `mku` targets above pass.

Phase 1 Status: complete. Date: 2026-07-06. Result: Mail Relay config is fully expanded with outbound/inbound/enabled flags, database defaulting, template/event settings, and comprehensive launch/landing readiness. Disabled state is a clean skip in launch planning. All Unity config, launch, and landing tests pass. Example configs and JSON schema are synchronized. Variances: none significant; server validation in readiness moved behind OutboundEnabled so inbound-only deployments don't require SMTP servers unless outbound sending is active. Updated tests to reflect clean skip semantics for disabled subsystem.

---

## Phase 2 - Core Outbound SMTP Sender

Objective: Send one RFC 5322-compliant outbound message to a local SMTP sink, with no DB, API, or templates, using our own C code end-to-end (no Python in the test path).

Entry Gate: Phase 1 exit gate green.

### Phase 2 reordering note (2026-07-06, resolved with user)

The original plan assumed a Python SMTP sink and a throwaway self-test to satisfy the "send one email" exit gate before the queue/API/producer existed. Per user direction we avoid Python and bootstrap with our own code, so Phase 2 is reordered:

- The local SMTP sink is a small **C program under `extras/mailval/`** (a long-term reusable multi-protocol test fixture, not a Python script), built by its own standalone CMakeLists and not part of Hydrogen's recursive source discovery.
- The raw send path is introduced as a **real internal `mailrelay_send_raw()` API** (the seed of the Phase 3 producer API) rather than a throwaway self-test hack. The blackbox test drives a real send via a `MailRelay.Test.SendRawOnLaunch` config flag, with no queue/API required yet.
- Email validation is a **dedicated `mailrelay_is_valid_email()`** in the mailrelay module (modeled on the auth rules but decoupled from the auth subsystem), so mailrelay has no hard dependency on `src/api/auth`.
- The sink can be smoke-tested with our own `mailrelay_smtp` helper once 2.4 lands, reinforcing the "use our own code to bootstrap" approach.

- [x] 2.0 Build a multi-protocol mail validator tool `extras/mailval/` (our own C code, no Python), TLS-capable, designed to validate SMTP now and IMAP/JMAP later from one fixture.
  - Architecture: a standalone `CMakeLists.txt` producing a `mailval` binary (kept out of Hydrogen's recursive source discovery). A generic TLS-capable TCP listener (`listener.c`/`tls.c` using OpenSSL, already a Hydrogen dependency) dispatches each accepted connection to a pluggable protocol module. A shared `capture.c` writes per-session transcripts + parsed key fields (envelope, recipients, auth params, capabilities, JSON for JMAP) to the test artifact directory for blackbox assertions.
  - `proto_smtp.c` (Phase 2, full): `EHLO`/`MAIL FROM`/`RCPT TO`/`DATA`/`QUIT`, `STARTTLS` and implicit `SMTPS` via the TLS layer, capture of AUTH parameters (never store plaintext creds), writes each delivered message (envelope + body) to a file. This is the functional fixture for tests 57/58/59.
  - `proto_imap.c` and `proto_jmap.c` (Phase 2, full): real, functional servers backed by the shared mailbox store (so an SMTP-delivered message is retrievable via IMAP and JMAP). IMAP implements `CAPABILITY`/`STARTTLS`/`LOGIN`/`SELECT`/`EXAMINE`/`CREATE`/`LIST`/`LSUB`/`STATUS`/`APPEND`/`FETCH`/`STORE`/`SEARCH`/`UID` variants/`IDLE`/`LOGOUT` against the store. JMAP implements HTTPS `/jmap/session`, `/jmap/upload`, `/jmap/download`, `mailbox/get|set`, `messages/get|set|query|search`, and blob access against the same store. Both capture full transcripts to JSON for blackbox assertions.
  - Verification: manual `./mailval --smtp-port 5570 --tls-cert <pem> --tls-key <pem>` smoke run accepts a TLS and a plaintext SMTP send (via our own `mailrelay_smtp` helper after 2.4) and writes the captured message; IMAP/JMAP ports accept a connection and log the handshake. `mkp` covers the C sources if in cppcheck scope.

- [x] 2.1 Create `src/mailrelay/` module skeleton (currently empty dir).
  - Files: `mailrelay.h`, `mailrelay_internal.h`, `mailrelay_message.{c,h}`, `mailrelay_render.{c,h}`, `mailrelay_smtp.{c,h}`, `mailrelay_result.{c,h}`, `mailrelay_test_seams.{c,h}` (for injecting a fixed clock / Message-ID for deterministic tests).
  - Note: `mailrelay.c` lifecycle/init is deferred to Phase 3.3 (worker wiring); `mailrelay_queue.*`, `mailrelay_template.*`, `mailrelay_repository.*` are later phases.
  - Build note: current CMake uses recursive source discovery for `../src/*.c`; confirm new files are discovered and only adjust `cmake/` if Unity/exclusion rules require it.
  - Verification: `mkt`.

- [x] 2.2 Define `MailRelayMessage` and a dedicated validation helper.
  - Fields: from, reply-to, to/cc/bcc arrays, subject, text body, HTML body, headers, metadata, priority, idempotency key.
  - Add `bool mailrelay_is_valid_email(const char* email)` in the mailrelay module (dedicated validator; model the rules on `src/api/auth/auth_service_validation.c:219` `is_valid_email` but keep it local to mailrelay). Avoid static functions so helpers are testable.
  - Verification: `mku mailrelay_message_test` covers valid/invalid addresses, empty recipients, subject/body limits, and BCC handling.

- [x] 2.3 Implement RFC 5322 message rendering.
  - File: `mailrelay_render.{c,h}`. Include `Date`, `Message-ID`, MIME multipart/alternative boundaries when both text and HTML are present. Use CRLF line endings. Use `mailrelay_test_seams` to inject a fixed clock / Message-ID for deterministic tests.
  - Verification: `mku mailrelay_render_test` verifies deterministic rendering with a fixed clock/test Message-ID seam.

- [x] 2.4 Implement libcurl SMTP/SMTPS send helper.
  - File: `mailrelay_smtp.c`. Use `smtp://` + STARTTLS or `smtps://` per the server's `TLSMode` (map legacy `UseTLS` to `MAIL_TLS_MODE_STARTTLS` for backward compat). Set `CURLOPT_NOSIGNAL=1L`, connect/total timeouts, `CURLOPT_MAIL_FROM`, `CURLOPT_MAIL_RCPT`, `CURLOPT_UPLOAD=1L` + `CURLOPT_READFUNCTION`/`CURLOPT_READDATA` to stream the RFC 5322 payload. Do not log credentials or body by default. Return a structured `MailRelayResult` (success flag, SMTP code/text, duration).
  - Verification: `mkt` and a unit-testable seam verifying URL, envelope, credentials, timeout, and payload callback behavior. Record the exact working `CURLOPT_*` set in the Working Log.

- [x] 2.5 Define the internal raw send API (`mailrelay_send_raw`) and test trigger.
  - Files: `src/mailrelay/mailrelay.{c,h}` (public API), `src/launch/launch_mail_relay.c` (trigger), `src/config/config_mail_relay.{c,h}` (Test substruct + loader + dump + cleanup + defaults), `tests/unity/src/mailrelay/mailrelay_send_raw_test.c`. `mailrelay_send_raw()` wraps `mailrelay_smtp_send()` as the stable Phase 3 producer seed. `MailRelay.Test.SendRawOnLaunch` + canned fields (`TestFrom`, `TestTo`, `TestSubject`, `TestBody`) are gated off by default; `launch_mail_relay_subsystem()` fires one synchronous send on `Servers[0]` when true, logs the structured `MailRelayResult`, and does not touch queues or workers. Config schema updated.
  - Verification: `mkt` passes, `mku mailrelay_send_raw_test` has 7 passing tests (init, success, null args, transport failure, fallback from), existing `mku config_mail_relay_test_load_mailrelay_config`, `launch_mail_relay_test_comprehensive_coverage`, and `landing_mail_relay_test_readiness` all pass. `mkp` is clean for new files.

- [x] 2.6 Add local SMTP sink blackbox support.
  - Create `tests/test_57_mailrelay_outbound.sh` (ports 5570-5576) that starts the C `mailval` SMTP module, starts Hydrogen with `tests/configs/hydrogen_test_57_mailrelay_outbound.json` (`MailRelay.Test.SendRawOnLaunch=true`, `Servers[0]` pointing at the validator), and captures one message file. Add `docs/H/tests/test_57_mailrelay_outbound.md`.
  - Verification: `test_57_mailrelay_outbound.sh` passes 8/8 with plaintext SMTP and STARTTLS SMTP; `test_92_shellcheck.sh`, `test_04_check_links.sh`, and `test_90_markdownlint.sh` pass.

Exit Gate: Hydrogen can send one configured raw outbound email to a local C SMTP sink, and failures return structured results without leaking secrets. `mkt`, `mkp`, the new `mku` targets, and `test_57` pass.

Phase 2 Status: complete. Date: 2026-07-06. Result: 2.0 complete and verified end-to-end (SMTP delivers into the shared in-memory store; JMAP `messages/get` retrieves it; IMAP works; TLS works). 2.1 complete: module files created (`mailrelay_message.{c,h}`, `mailrelay_result.{c,h}`, `mailrelay_render.{c,h}`, `mailrelay_test_seams.{c,h}`); build verified by `mkt`. 2.2 complete: `MailRelayMessage` model with init/free, `mailrelay_is_valid_email` (aligned to `auth_service_validation.c:219` rules exactly), `mailrelay_validate_message`, `mailrelay_message_add_to/cc/bcc`, `mailrelay_message_recipient_count`; `mailrelay_message_test` has 20 passing tests. 2.3 complete: RFC 5322 rendering via `mailrelay_render_message` with injectable `time()`/Message-ID seams, deterministic MIME boundary, CRLF line endings, stable header order, Bcc exclusion, and `default_from` fallback; `mailrelay_render_test` has 12 passing tests. Build errors fixed: `mv_xstrdup` → `strdup`; `*len++` precedence bug in `render_header` fixed to `(*len)++`. 2.4 complete: `src/mailrelay/mailrelay_smtp.{c,h}` added — renders internally (owns rendering), resolves TLS/Auth into a testable `MailRelaySmtpRequest`, and delivers via libcurl with full `AuthMode` dispatch behind a swappable transport seam. `mailrelay_smtp_test.c` (10 tests) asserts URL/envelope/credentials/timeout/rendered payload across TLS modes plus failure/arg/render paths; `mailrelay_smtp_live_test.c` is env-gated (`MAILRELAY_LIVE_SMTP=host:port`) and performed a real send to `mailval` on :5570 (code 250, `stored_uid=yes`). 2.5 complete: `src/mailrelay/mailrelay.{c,h}` added as the public producer API (`mailrelay_send_raw` + `mailrelay_init`); `MailRelay.Test` config substruct (`SendRawOnLaunch`, `TestFrom`, `TestTo`, `TestSubject`, `TestBody`) wired through loader, cleanup, dump, defaults, and schema; `launch_mail_relay_subsystem()` fires one synchronous `mailrelay_send_raw` to `Servers[0]` when `SendRawOnLaunch=true` and logs the structured result. `mailrelay_send_raw_test.c` has 7 passing tests (init, success, null args, transport failure, fallback from). 2.6 complete: `tests/test_57_mailrelay_outbound.sh`, plaintext/TLS configs, and `docs/H/tests/test_57_mailrelay_outbound.md` added; final `test_57` passes 8/8 and captures stored messages for plaintext SMTP and STARTTLS SMTP. Variances: `mailval` required `signal(SIGPIPE, SIG_IGN)`, a recursive store mutex, capture-mutex unlock on all paths, and TLS WANT_READ retry — all now in the tool. `mailval.pem`/`mailval.key` are generated locally by `gen_cert.sh` and must never be committed. Note: work was done in large chunks this pass; future chunks should be smaller and individually gated (see Working approach note).

---

## Phase 3 - In-Memory Queue, Workers, Retry, Debounce

Objective: Make outbound sending asynchronous and resilient before adding persistence.

Entry Gate: Phase 2 exit gate green.

- [x] 3.1 Add Mail Relay runtime state and thread tracking.
  - Add `ServiceThreads mailrelay_threads;` in `src/state/state.c`, extern in `src/threads/threads.h`, and count it in `report_thread_status`. Track queue, mutex/condition variables, worker threads, counters, shutdown flag, last errors in `mailrelay_internal.h`.
  - Verification: `mku mailrelay_runtime_test` covers init/cleanup idempotency.

- [x] 3.2 Implement bounded in-memory queue.
  - Respect `Queue.MaxInMemory` / `MaxQueueSize`; return `MAIL_QUEUE_FULL` when full.
  - Verification: `mku mailrelay_queue_test` covers enqueue/dequeue order, capacity, and cleanup.

- [x] 3.3 Implement worker pool.
  - Configurable worker count. Sends run off the API/logging caller path. Each worker registers with `add_service_thread(&mailrelay_threads, pthread_self())` and unregisters on exit.
  - Verification: `mku mailrelay_workers_test` with a fake sender processes N messages concurrently, each exactly once.

- [x] 3.4 Implement retry with exponential backoff.
  - Classify transient (4xx connection / 5xx transient / transport) vs permanent failures from SMTP/libcurl responses. Honor `RetryAttempts`, `InitialDelaySeconds`, `MaxDelaySeconds`.
  - Verification: `mku mailrelay_retry_test` verifies transient retries and permanent-failure stop policy.

- [x] 3.5 Implement debounce/coalescing for event-generated mail.
  - Added `debounce_key` to `MailRelayMessage` (init/free/copy/validate).
  - Added `src/mailrelay/mailrelay_debounce.{c,h}` with a bounded in-memory pending map, a dedicated expiry thread, and `%COUNT%`/`%SUMMARY%` placeholder replacement.
  - Wired `mailrelay_enqueue()` to route messages with a non-empty `debounce_key` through the debounce module when `Queue.DebounceSeconds > 0`.
  - The expiry thread is lazily started on the first debounce submission; `mailrelay_shutdown()` stops the thread and flushes pending entries.
  - Verification: `mku mailrelay_debounce_test` passes with 8 tests (no-key passthrough, disabled window, pending creation, coalescing, placeholder replacement, window respect, flush, max-pending limit).

- [x] 3.6 Wire launch/landing to start and stop workers.
  - Replace the TODO in `launch_mail_relay_subsystem()` (`launch_mail_relay.c:162`) with real init: `init_service_threads`, spawn workers, `update_subsystem_on_startup`, verify `SUBSYSTEM_RUNNING`. Implement the landing drain loop (mdns pattern) in `land_mail_relay_subsystem()`.
  - Verification: `test_17_startup_shutdown.sh` (or a mail-enabled variant) shows no orphan worker threads and clean landing.

- [x] 3.7 Define the internal raw enqueue producer API.
  - File: `src/mailrelay/mailrelay.h`.
  - Purpose: one C API used by future REST endpoints, Lua `H.mail`, Notify, and system events. Producers must enqueue through Mail Relay so queue limits, audit metadata, rate limits, idempotency, and redaction are centralized. Lua and Notify must not call SMTP or HTTP endpoints directly.
  - Verification: `mku mailrelay_queue_test` or a new producer test verifies disabled/error/queued results and structured error codes.

Exit Gate: Mail Relay accepts asynchronous messages through a stable internal producer API, processes them with workers, retries transient failures, debounces bursts, and lands cleanly with no orphan threads. `mkt`, `mkp`, the new `mku` set, and startup/shutdown pass.

Phase 3 Status: complete. Date: 2026-07-07. Result: 3.1 complete. Runtime state (`MailRelayRuntime`) lives in a heap-allocated global owned by `mailrelay_init()`/`mailrelay_shutdown()`, initialized from launch and torn down from landing. Thread tracking is wired through the existing `mailrelay_threads` `ServiceThreads`. 3.2 complete: bounded in-memory queue with deep-copy enqueue, timed blocking dequeue, priority ordering, shutdown broadcast, and `MailRelayStatus` codes. 3.3 complete: `mailrelay_workers.{c,h}` spawn configurable worker threads that register with service threads, dequeue messages, send via `mailrelay_send_raw`, update counters, and exit cleanly on shutdown. 3.4 complete: added `mailrelay_retry.{c,h}` with transient/permanent classification, exponential backoff using `InitialDelaySeconds` base and `MaxDelaySeconds` cap, and worker integration that re-enqueues transient failures up to `RetryAttempts`. `MailRelayResult` now carries a `retryable` flag set by the libcurl transport for connection/timeout/TLS errors and by SMTP 4xx codes; 5xx is permanent. `mailrelay_queue_dequeue` is now time-aware: it scans for the highest-priority due item and blocks until the next scheduled `next_attempt_at` or a new arrival, so future retry items do not starve due items. `mailrelay_queue_enqueue_scheduled` was added to support explicit `attempts`/`next_attempt_at` on re-enqueue. `mailrelay_retry_test` has 9 passing tests (classification, SMTP-code fallback, delay computation, should-retry, time-aware dequeue, worker transient-retry-success, permanent no-retry, max-attempts exhaustion). 3.5 complete: added `MailRelayMessage.debounce_key` and `src/mailrelay/mailrelay_debounce.{c,h}`. The debounce module keeps a bounded in-memory map of pending keys, runs a dedicated expiry thread that is lazily started on the first debounce submission, and replaces `%COUNT%`/`%SUMMARY%` placeholders in the coalesced message. `mailrelay_enqueue()` routes debounced messages through this path; `mailrelay_shutdown()` stops the expiry thread and flushes pending entries. `mailrelay_debounce_test` has 8 passing tests. 3.6 complete: launch registers Mail Relay via `register_subsystem_from_launch` with `mailrelay_threads`, `mail_relay_system_shutdown`, `launch_mail_relay_subsystem`, and `land_mail_relay_subsystem`; updates registry to `SUBSYSTEM_RUNNING` and verifies state; failure paths update registry to `SUBSYSTEM_ERROR` and shut down. Landing updates registry through `SUBSYSTEM_STOPPING` to `SUBSYSTEM_INACTIVE` around `mailrelay_shutdown()`. 3.7 complete: `mailrelay_enqueue()` is the stable internal producer API; it validates messages, deep-copies into the queue, and returns `MailRelayStatus`. `SendRawOnLaunch` routes through the async enqueue path. Verification: `mkt`, `mkp`, all listed `mku` targets (including the new `mailrelay_debounce_test`), and startup/shutdown pass. Variances: the debounce expiry thread uses a bounded 1-second maximum `pthread_cond_timedwait` to avoid a lost-shutdown-signal stall, and it is lazily started on first debounce submission so `mailrelay_init()` does not introduce a background thread unless debounce is actually used. 3.7 complete: `mailrelay_enqueue()` is the stable internal producer API used by the launch trigger. `mailrelay_workers_test` has 4 passing tests, all related Unity tests pass, `mkp` is clean, `mka` is clean, and `test_57_mailrelay_outbound.sh` passes 8/8. Variances: 3.7 (producer API) was implemented earlier than originally listed because the async `SendRawOnLaunch` path needed a public enqueue function; the plan now reflects this. Remaining Phase 3 exit-gate item 3.5 (debounce) is the next chunk; the current in-memory queue/worker/retry foundation is stable and ready for it.

---

## Phase 4 - Database Persistence and Mail Tables

Objective: Create every database object (tables, lookups, QueryRefs) needed by the Mail Relay implementation across all phases, so later phases only write C/Lua code against an already-migrated schema.

Entry Gate: Phase 3 exit gate green; Phase 0.2 database-target decision recorded.

Design decisions locked for this phase:

- Schema work is delivered as one coordinated effort, in small independently-verifiable chunks.
- Every table, lookup, and QueryRef gets its own Acuranzo migration file.
- **No existing lookups are reused.** Every mail-specific enumerated column gets a new lookup.
- All database work for any Mail Relay phase is created now; later phases do not add new migrations.
- Lookup numbers are taken from the Acuranzo README index; the next free lookup is **063**.
- The next free QueryRef is **093**.
- The next free migration number is **1211**.
- Queue bodies are stored as structured fields only (no duplicate rendered payload).
- All SQL is engine-agnostic: no partial indexes, no engine-specific `RETURNING` outside the existing `${INSERT_KEY_*}` macros.

### New lookups (all mail-specific)

| Lookup | Name | Values |
|---|---|---|
| 063 | Mail Queue Status | 0=pending, 1=sending, 2=sent, 3=failed, 4=retrying |
| 064 | Mail Template Status | 0=inactive, 1=active, 2=deprecated |
| 065 | Mail Event Status | 0=pending, 1=queued, 2=sent, 3=failed, 4=suppressed |
| 066 | Mail OTP Purpose | 0=login_mfa, 1=email_verify, 2=password_reset |
| 067 | Mail OTP Status | 0=active, 1=consumed, 2=expired, 3=max_attempts_exceeded |
| 068 | Mail Route Status | 0=inactive, 1=active |

### New tables

| Table | Status lookup | Purpose |
|---|---|---|
| `mail_templates` | `status_a64` | Reusable mail templates (Phases 4, 5, 7, 7A) |
| `mail_queue` | `status_a63` | Durable outbound queue (Phases 4, 6, 11) |
| `mail_attempts` | — | Per-message delivery attempts (Phase 4) |
| `mail_events` | `status_a65` | Durable event/audit log for system-generated mail (Phase 6) |
| `mail_otp_codes` | `purpose_a66`, `status_a67` | One-time password storage (Phase 8) |
| `mail_routes` | `status_a68` | Inbound SMTP routing/rewrite rules (Phase 12) |

`mail_queue` includes `instance_id` and `claim_token` columns up-front to support Phase 11 multi-instance atomic claim without a later ALTER.

### QueryRef assignments (all internal/system)

| QueryRef | Migration | Purpose |
|---|---|---|
| 093 | 1223 | Insert pending mail queue row |
| 094 | 1224 | Get mail queue row by `message_uuid` |
| 095 | 1225 | Get mail queue row by `idempotency_key` |
| 096 | 1226 | Claim next pending row |
| 097 | 1227 | Mark mail queue row as `sending` |
| 098 | 1228 | Mark mail queue row as `sent` |
| 099 | 1229 | Mark mail queue row as `failed` |
| 100 | 1230 | Reschedule mail queue row for retry |
| 101 | 1231 | Recover stale `sending` rows |
| 102 | 1232 | Insert mail attempt |
| 103 | 1233 | Get mail template by `template_key` |
| 104 | 1234 | List active mail templates |
| 105 | 1235 | Insert mail template |
| 106 | 1236 | Update mail template |
| 107 | 1237 | Soft-delete mail template |
| 108 | 1238 | Insert mail event |
| 109 | 1239 | List pending mail events |
| 110 | 1240 | Mark mail event processed/queued |
| 111 | 1241 | Mark mail event suppressed |
| 112 | 1242 | Insert OTP code |
| 113 | 1243 | Get active OTP by email + purpose |
| 114 | 1244 | Consume OTP code |
| 115 | 1245 | Increment OTP attempts |
| 116 | 1246 | Expire old OTP codes |
| 117 | 1247 | Get OTP by id |
| 118 | 1248 | Get inbound route by sender domain |
| 119 | 1249 | List active inbound routes |
| 120 | 1250 | Insert inbound route |
| 121 | 1251 | Update inbound route |
| 122 | 1252 | Soft-delete inbound route |
| 123 | 1253 | Cleanup old sent/failed queue rows |
| 124 | 1254 | Cleanup old mail events |
| 125 | 1255 | Cleanup old mail attempts |
| 126 | 1256 | Cleanup old OTP codes |

### Phase 4 chunk 4A — Lookups

Create one lookup per migration, in order, so later table migrations can reference them safely.

- [x] 4A.1 Lookup 063 - Mail Queue Status (`acuranzo_1211.lua`).
- [x] 4A.2 Lookup 064 - Mail Template Status (`acuranzo_1212.lua`).
- [x] 4A.3 Lookup 065 - Mail Event Status (`acuranzo_1213.lua`).
- [x] 4A.4 Lookup 066 - Mail OTP Purpose (`acuranzo_1214.lua`).
- [x] 4A.5 Lookup 067 - Mail OTP Status (`acuranzo_1215.lua`).
- [x] 4A.6 Lookup 068 - Mail Route Status (`acuranzo_1216.lua`).
- [x] 4A.7 Update Acuranzo README index (via `elements/002-helium/scripts/helium_update.sh`).

Exit Gate 4A: Lookups 063–068 apply/reverse cleanly and are present in the `lookups` table; `test_98_luacheck.sh` passes; README is up to date.

### Phase 4 chunk 4B — Tables

- [x] 4B.1 `mail_templates` table (`acuranzo_1217.lua`).
  - Columns: `template_id`, `template_key` (UNIQUE), `name`, `status_a64`, `subject_template`, `text_template`, `html_template`, `collection`, `${COMMON_CREATE}`.
- [x] 4B.2 `mail_queue` table (`acuranzo_1218.lua`).
  - Columns: `queue_id`, `message_uuid` (UNIQUE), `status_a63`, `priority`, `template_key`, `from_addr`, `reply_to`, `recipients_json`, `subject`, `body_text`, `body_html`, `headers_json`, `idempotency_key`, `attempts`, `next_attempt_at`, `last_attempt_at`, `smtp_code`, `smtp_text`, `server_index`, `instance_id`, `claim_token`, `${COMMON_CREATE}`.
  - Indexes: `(status_a63, next_attempt_at)`, `(idempotency_key)`, `(instance_id, claim_token)`.
- [x] 4B.3 `mail_attempts` table (`acuranzo_1219.lua`).
  - Columns: `attempt_id`, `queue_id` (FK to `mail_queue`), `attempt_number`, `server_index`, `success`, `smtp_code`, `smtp_text`, `duration_ms`, `error_class`, `${COMMON_CREATE}`.
  - Index on `queue_id`.
- [x] 4B.4 `mail_events` table (`acuranzo_1220.lua`).
  - Columns: `event_id`, `event_key`, `status_a65`, `template_key`, `from_addr`, `reply_to`, `recipients_json`, `subject`, `body_text`, `body_html`, `headers_json`, `params_json`, `debounce_key`, `idempotency_key`, `priority`, `queue_id`, `processed_at`, `${COMMON_CREATE}`.
  - Indexes: `(status_a65, created_at)`, `(event_key)`.
- [x] 4B.5 `mail_otp_codes` table (`acuranzo_1221.lua`).
  - Columns: `otp_id`, `code_hash`, `email`, `account_id`, `purpose_a66`, `status_a67`, `expiry_at`, `attempts`, `max_attempts`, `consumed_at`, `${COMMON_CREATE}`.
  - Indexes: `(email, purpose_a66, status_a67)`, `(status_a67, expiry_at)`.
- [x] 4B.6 `mail_routes` table (`acuranzo_1222.lua`).
  - Columns: `route_id`, `status_a68`, `source_network`, `sender_domain`, `sender_pattern`, `recipient_domain`, `recipient_pattern`, `auth_required`, `require_tls`, `template_key`, `rewrite_from`, `rewrite_to`, `add_recipients_json`, `priority`, `sort_seq`, `${COMMON_CREATE}`.
  - Indexes: `(status_a68, sender_domain, sort_seq)`, `(status_a68, recipient_domain, sort_seq)`.
- [x] 4B.7 Update Acuranzo README index (via `elements/002-helium/scripts/helium_update.sh`).

Exit Gate 4B: All six tables apply/reverse cleanly on SQLite and at least one server engine; diagram migrations are present; `test_71_database_diagrams.sh` passes; `test_98_luacheck.sh` is green; README is up to date.

### Phase 4 chunk 4C — Core QueryRefs

The QueryRefs needed for queue, attempts, templates, events, OTP, and routes. Each QueryRef is its own migration (1223–1252, mapping to QueryRefs 093–122). This chunk is split into four smaller independently-verifiable sub-chunks:

- [x] 4C.1 Queue, attempts, and template QueryRefs (migrations 1223–1237, QueryRefs 093–107).
  - [x] 4C.1a Queue lifecycle QueryRefs (migrations 1223–1231, QueryRefs 093–101).
    - [x] 093: Insert pending mail queue row (`acuranzo_1223.lua`).
    - [x] 094: Get mail queue row by `message_uuid` (`acuranzo_1224.lua`).
    - [x] 095: Get mail queue row by `idempotency_key` (`acuranzo_1225.lua`).
    - [x] 096: Select next pending row (`acuranzo_1226.lua`).
    - [x] 097: Mark mail queue row as `sending` (`acuranzo_1227.lua`).
    - [x] 098: Mark mail queue row as `sent` (`acuranzo_1228.lua`).
    - [x] 099: Mark mail queue row as `failed` (`acuranzo_1229.lua`).
    - [x] 100: Reschedule mail queue row for retry (`acuranzo_1230.lua`).
    - [x] 101: Recover stale `sending` rows (`acuranzo_1231.lua`).
  - [x] 4C.1b Attempts and template QueryRefs (migrations 1232–1237, QueryRefs 102–107).
    - [x] 102: Insert mail attempt (`acuranzo_1232.lua`).
    - [x] 103: Get mail template by `template_key` (`acuranzo_1233.lua`).
    - [x] 104: List active mail templates (`acuranzo_1234.lua`).
    - [x] 105: Insert mail template (`acuranzo_1235.lua`).
    - [x] 106: Update mail template (`acuranzo_1236.lua`).
    - [x] 107: Soft-delete mail template (`acuranzo_1237.lua`).
- [x] 4C.2 Event QueryRefs (migrations 1238–1241, QueryRefs 108–111).
  - [x] 108: Insert mail event (`acuranzo_1238.lua`).
  - [x] 109: List pending mail events (`acuranzo_1239.lua`).
  - [x] 110: Mark mail event processed/queued (`acuranzo_1240.lua`).
  - [x] 111: Mark mail event suppressed (`acuranzo_1241.lua`).
- [x] 4C.3 OTP QueryRefs (migrations 1242–1247, QueryRefs 112–117).
  - [x] 112: Insert OTP code (`acuranzo_1242.lua`).
  - [x] 113: Get active OTP by email + purpose (`acuranzo_1243.lua`).
  - [x] 114: Consume OTP code (`acuranzo_1244.lua`).
  - [x] 115: Increment OTP attempts (`acuranzo_1245.lua`).
  - [x] 116: Expire old OTP codes (`acuranzo_1246.lua`).
  - [x] 117: Get OTP by id (`acuranzo_1247.lua`).
- [x] 4C.4 Inbound route QueryRefs (migrations 1248–1252, QueryRefs 118–122).
  - [x] 118: Get inbound route by sender domain (`acuranzo_1248.lua`).
  - [x] 119: List active inbound routes (`acuranzo_1249.lua`).
  - [x] 120: Insert inbound route (`acuranzo_1250.lua`).
  - [x] 121: Update inbound route (`acuranzo_1251.lua`).
  - [x] 122: Soft-delete inbound route (`acuranzo_1252.lua`).
- [x] 4C.5 Update Acuranzo README index; run `test_98_luacheck.sh`.
- [ ] 4C.6 Verify QueryRefs 093–122 load into the QTC after migration.

Exit Gate 4C: QueryRefs 093–122 apply/reverse cleanly, are type internal/system, and appear in the QTC after migration.

### Phase 4 chunk 4D — Operational cleanup QueryRefs

- [x] 4D.1 QueryRef #123 — Cleanup old sent/failed queue rows (`acuranzo_1253.lua`).
- [x] 4D.2 QueryRef #124 — Cleanup old mail events (`acuranzo_1254.lua`).
- [x] 4D.3 QueryRef #125 — Cleanup old mail attempts (`acuranzo_1255.lua`).
- [x] 4D.4 QueryRef #126 — Cleanup old OTP codes (`acuranzo_1256.lua`).
- [x] 4D.5 Update Acuranzo README index; run `test_98_luacheck.sh`.

Exit Gate 4D: Cleanup QueryRefs apply/reverse cleanly and are type internal/system.

### Phase 4 chunk 4E — Repository helpers and startup recovery

- [x] 4E.1 Implement `src/mailrelay/mailrelay_repository.{c,h}`.
  - Helpers for every QueryRef above, using a mockable seam.
  - Verification: `mku mailrelay_repository_test`.
- [x] 4E.2 Add startup recovery.
  - On `mailrelay_init()` with persistence enabled, call QueryRef #101 to recover stale `sending` rows.
  - Added `MailRelay.Queue.StaleTimeoutSeconds` (default 300 s) to configure the recovery cutoff.
  - Fixed a parameter mismatch: the repo helper now binds `:STALE_BEFORE` (not `:STALE_CUTOFF_AT`) to match migration `acuranzo_1231.lua`.
  - Verification: `mku mailrelay_persistence_test` covers disabled skip, enabled recovery, and affected-row logging.
- [x] 4E.3 Wire persistence into `mailrelay_enqueue()`.
  - When `Queue.Persist=true` and a DB target is configured, `mailrelay_enqueue()` writes the pending `mail_queue` row before returning `MAILRELAY_OK`.
  - Added `template_key` and `headers` to `MailRelayMessage` so persisted rows can carry them.
  - Added `mailrelay_enqueue_to_queue()` internal helper so the debounce flush path also persists coalesced messages.
  - Persistence failures are fail-fast: `mailrelay_enqueue` returns `MAILRELAY_PERSIST_FAILED` and does not place the message in memory.
  - Verification: `mku mailrelay_persistence_test` (6 tests) and `mku mailrelay_queue_test` both pass.

Exit Gate 4E (Phase 4 final): the durable queue, templates, events, OTP, routes, and cleanup paths are reachable through the repository; `mkt`, `mkp`, `test_98_luacheck.sh`, the new `mku` set, and at least SQLite + one server-engine migration test pass.

Phase 4 Status: complete. Date: 2026-07-07. Result: Phase 4A, 4B, 4C, 4D, and 4E are complete. Lookups 063–068 created as migrations 1211–1216; all six Mail Relay tables (`mail_templates`, `mail_queue`, `mail_attempts`, `mail_events`, `mail_otp_codes`, `mail_routes`) created as migrations 1217–1222; all core QueryRefs 093–122 created as migrations 1223–1252; all cleanup QueryRefs 123–126 created as migrations 1253–1256. `src/mailrelay/mailrelay_repository.{c,h}` created with callback-based helpers for all QueryRefs 093–126 and a mockable execution seam. Startup recovery (`mailrelay_recover_stale_sending_rows`) runs from `mailrelay_init()` when `Queue.Persist=true` and a database target is configured, calling QueryRef 101 with a `STALE_BEFORE` cutoff computed from `Queue.StaleTimeoutSeconds`. Enqueue persistence writes a pending `mail_queue` row via QueryRef 093 before the message enters the in-memory queue; debounced/coalesced messages are also persisted when flushed. `mku mailrelay_repository_test` (12 tests), `mku mailrelay_persistence_test` (6 tests), `mku mailrelay_queue_test`, `mku mailrelay_debounce_test`, `mku mailrelay_message_test`, `mku config_mail_relay_test_load_mailrelay_config`, `mkt`, `mkp`, and `tests/test_93_jsonlint.sh` all pass. README index regenerated by `elements/002-helium/scripts/helium_update.sh`; `tests/test_98_luacheck.sh` passes. The QTC load verification (4C.6) remains pending per-user verification; the live DB persistence path has been validated only through the mock executor so far. Variances: README update performed by `helium_update.sh` rather than manual edit, per project convention; integer primary keys are application-generated rather than using `${SERIAL}` to match project migration convention; logical foreign keys (`queue_id` references) are not enforced by database constraints, matching existing Acuranzo convention; QueryRef 096 is intentionally a non-atomic SELECT-only claim to stay engine-agnostic; cleanup QueryRefs use a `:CUTOFF_AT` timestamp parameter rather than `:RETENTION_DAYS` because engine-agnostic date arithmetic is not available in the existing macro set; the repository API is callback-based but the default executor waits synchronously for the database result before invoking the callback, because the existing database infrastructure (pending result manager) is synchronous from the caller's perspective; `Queue.StaleTimeoutSeconds` is new and defaults to 300 seconds; the recovery QueryRef parameter was renamed from `STALE_CUTOFF_AT` to `STALE_BEFORE` to match migration 1231.

## Phase 4F - Generic Query Result Cache

Objective: Add a server-lifetime, global, query-result cache for any QueryRef whose `query_queue_a58` (Lookup 58) is `cached` (value 3). This is a cross-cutting prerequisite for the Phase 5 macro engine so that lookup-driven macros (e.g., QueryRef 034/035 for Lookup 046) do not hit the database on every render.

Entry Gate: Phase 4 exit gate green.

Design decisions locked for this chunk (2026-07-07):

- Cache is global, keyed by `database_name:sql_template_hash:param_hash`. The SQL template hash makes the cache transparent and database-agnostic; no existing code needs to know about the cache.
- Parameter hash is SHA256 of normalized parameter JSON.
- Only successful query results are cached; failures are re-executed.
- Cache hooks live in the database queue layer (`database_queue_submit_query` and `database_queue_process_single_query`) so both Conduit REST and Mail Relay repository queries benefit transparently.

- [x] 4F.1 Add query-result cache data structure.
  - Files: `src/database/dbqueue/query_result_cache.{c,h}`.
  - Thread-safe hash table storing `data_json`, `row_count`, `column_count`, `affected_rows`, `execution_time_ms`, and `success`.
  - Key includes a SHA256 hash of the SQL template plus a SHA256 hash of the normalized parameter JSON.
  - Verification: `mku query_result_cache_test` passes.

- [x] 4F.2 Hook cache into database queue submission and result processing.
  - In `database_queue_submit_query`: when `queue_type_hint == DB_QUEUE_CACHE` and a cache entry exists for the query template + parameters, signal the already-registered pending result with the cached `QueryResult` and skip the queue.
  - In `database_queue_process_single_query`: after a successful cache-type query, store the result in the cache before signaling the pending result.
  - No changes to existing callers; the cache is transparent.
  - Verification: `mkt` and existing `mku` database/Conduit tests pass.

- [x] 4F.3 Verify end-to-end with a cached query.
  - Rather than modifying a live migration, a new integration test (`query_result_cache_integration_test`) pre-populates the global cache and verifies that `database_queue_submit_query` returns a cache hit without enqueuing and signals the pending result.
  - Verification: `mku query_result_cache_integration_test` passes; `mku` database/Conduit/Mail Relay tests still pass.

Exit Gate: cache hit/miss is verified, no regressions in database/Conduit/Mail Relay unit tests, `mkt` and `mkp` are green.

Phase 4F Status: complete. Date: 2026-07-07. Result: Implemented a global, server-lifetime, query-result cache keyed by `database_name:sql_template_hash:param_hash`. The cache is transparent to all existing callers: cache hits are served in `database_queue_submit_query` and successful cache-type results are stored in `database_queue_process_single_query`. New files: `src/database/dbqueue/query_result_cache.{c,h}`, `tests/unity/src/database/dbqueue/query_result_cache_test.c`, `tests/unity/src/database/dbqueue/query_result_cache_integration_test.c`. Modified files: `src/database/dbqueue/manager.c`, `src/database/dbqueue/submit.c`, `src/database/dbqueue/process.c`. `mku query_result_cache_test` (8 tests), `mku query_result_cache_integration_test` (1 test), `mkt`, and `mkp` all pass. Relevant Mail Relay and database queue unit tests pass. Variances: The original plan proposed adding `query_ref` to `DatabaseQuery` and updating callers; this was abandoned because the SQL template hash provides a transparent, database-agnostic key without requiring any existing caller to know about the cache. The end-to-end verification was done with a unit-test seam instead of changing a migration to mark QueryRef 034 as `QTC_CACHED`.

---

## Phase 5 - Template Rendering and Macro Engine

Objective: Render mail templates safely, deterministically, and testably.

Entry Gate: Phase 4F exit gate green; the generic query-result cache is available for macro lookups.

- [x] 5.1 Define template syntax (record in Working Log).
  - Recommended v1: `%MACRO%` replacement with an explicit variable map plus built-ins: `%APP_NAME%`, `%SERVER_NAME%`, `%TIMESTAMP%`, `%USER_EMAIL%`, `%REQUEST_ID%`, `%OTP_CODE%` (when available).
  - Future expansion candidates include additional date formats, server uptime, and JWT claims when available from a REST/Lua request context.
  - Sharing with Lua: built-in macros will be implemented in C; a C helper or read-only Lua table can be exposed later so both C and Lua resolve macros from the same source (deferred to Phase 7A/13).
  - Verification: syntax documented in this plan's Working Log and in the code header.

- [x] 5.2 Implement the macro engine.
  - Missing required macro fails unless a default is provided; unknown variables detected before send.
  - Verification: `mku mailrelay_template_test` covers replacement, escaping policy, missing vars, repeated vars, and large values.

- [x] 5.3 Add HTML/text rendering policy.
  - Policy: use whatever bodies are provided. Do not derive text from HTML or HTML from text. At least one body (text or HTML) must be present; if neither is provided, rendering fails. The existing RFC 5322 renderer already handles text-only, HTML-only, and multipart alternative correctly.
  - Verification: `mku mailrelay_render_policy_test` verifies MIME output for text-only, HTML-only, and multipart alternative.

- [x] 5.4 Seed initial system templates.
  - Candidates: `system.server_started`, `system.server_stopped`, `auth.otp_code`, `mail.test`. Seed via migration.
  - Verification: migration seeds templates; blackbox can fetch/render them; `test_98_luacheck.sh` passes.

- [x] 5.5 Add preview rendering.
  - Preview returns rendered subject/body and macro diagnostics with no queue/send side effects.
  - Verification: `mku mailrelay_preview_test` verifies preview has no queue side effects.

- [x] 5.6 Define the internal templated-send producer API.
  - File: `src/mailrelay/mailrelay.h` plus implementation files as needed.
  - Purpose: common entry point for `mailrelay_enqueue_template(...)` / equivalent used by REST, Lua `H.mail`, Notify, and system events. It resolves templates, validates parameters/recipients, applies policy metadata, and enqueues without sending synchronously.
  - Verification: `mku mailrelay_template_test` or `mku mailrelay_producer_test` verifies success, missing template, missing macro, invalid recipient, idempotency key, and no body/OTP leakage in logs.

Exit Gate: template rendering is deterministic, tested, and supports system events, REST, Lua, and Notify producers through one internal API. `mkt`, `mkp`, the new `mku` set, and seed-migration checks pass.

Phase 5 Status: complete. Date: 2026-07-08. Result: All Phase 5 items (5.1-5.6) complete. Template syntax locked; macro engine implemented with 22 passing tests; preview rendering implemented with 5 passing tests; internal templated-send producer API (`mailrelay_send_template`) implemented with 6 passing tests. HTML/text policy locked: use whatever bodies are provided, never derive the missing body. Four seed templates created as migrations 1257-1260. `message_id` field added to `MailRelayMessage` so the producer can return stable identifiers. `mkt`, `mka`, `mkp`, `mku mailrelay_template_test`, `mku mailrelay_template_preview_test`, `mku mailrelay_producer_test`, `mku mailrelay_message_test`, and `mku mailrelay_persistence_test` all pass. `tests/test_34_sqlite_migrations.sh` (257 migrations reversed) is green. Variances: Function named `mailrelay_send_template` per user decision, not `mailrelay_enqueue_template`.

---

## Phase 6 - System Event Mail Integration

Objective: Send configured administrative emails for Hydrogen events without blocking critical paths.

Entry Gate: Phase 5 exit gate green.

- [x] 6.1 Define event rule config.
  - Kept existing `event_key → script_name` mapping in `MailRelay.Events.Rules`.
  - Added `MailRelay.Events.MaxEventsPerInterval` and `MailRelay.Events.EventIntervalSeconds` for per-event-key rate limiting.
  - Verification: `mku config_mail_relay_test_load_mailrelay_config` still passes; `tests/test_93_jsonlint.sh` validates the schema.

- [x] 6.2 Add a safe in-process event injection API.
  - Added `mailrelay_event_emit(event_key, params)` in `src/mailrelay/mailrelay_events.{c,h}`.
  - Events are dispatched to Lua handler scripts that return a mail-request table; C enqueues via `mailrelay_send_template()`.
  - Verification: `mku mailrelay_events_test` (8 tests) covers disabled state, unknown events, rate limit, and dispatch.

- [x] 6.3 Implement startup/shutdown admin messages.
  - Emitted `system.server_started` after `READY FOR REQUESTS` in `src/database/database.c` and the no-DB path in `src/launch/launch.c`.
  - Emitted `system.server_stopped` before shutdown in `src/landing/landing_mail_relay.c`.
  - Built-in Lua handlers use the seeded `system.server_started` / `system.server_stopped` templates and `MailRelay.AdminRecipients`.
  - Verification: `mku mailrelay_events_test`; blackbox extension deferred to Phase 6.1b.

- [x] 6.4 Add rate limiting for noisy events.
  - Implemented per-event-key fixed-window rate limiter in `MailRelayRuntime`.
  - Configurable via `MaxEventsPerInterval` / `EventIntervalSeconds`; defaults 10 / 60 seconds.
  - Verification: `mku mailrelay_events_test` verifies burst blocking and independent buckets per key.

Exit Gate: administrative event email works through the queue/templates path, is rate-limited, and does not block startup, logging, or shutdown. `mkt`, `mkp`, `mku mailrelay_events_test`, and `tests/test_93_jsonlint.sh` pass.

Phase 6 Status: partial complete (sub-chunk 6.1a). Date: 2026-07-08. Result: Event emission API implemented with built-in Lua handlers for `system.server_started`/`system.server_stopped`, per-event-key rate limiting, and config/schema updates. All Unity tests and lint pass. Variances: Custom DB-loaded event scripts and the extended `test_57_mailrelay_outbound.sh` blackbox verification are deferred to sub-chunk 6.1b. The original 6.1 wording assumed direct template/recipient mapping; the implemented design uses Lua-script-driven rules as decided by the user.

---

## Phase 7 - REST API: Send, Preview, Status

Objective: Expose authenticated mail operations through the Hydrogen API.

Entry Gate: Phase 5 exit gate green (Phase 6 not required, but recommended). Phase 0.2 DB-target and Phase 0 template-only send decision recorded.

- [x] 7.1 Add the `src/api/mailrelay/` endpoint module.
  - Layout `src/api/mailrelay/<endpoint>/<endpoint>.c/.h` with endpoints: `POST /api/mailrelay/send`, `POST /api/mailrelay/preview`, `GET /api/mailrelay/status`. Registered in `src/api/api_service.c` and logged in the endpoint list.
  - Verification: `mkt`; endpoint log list includes `/api/mailrelay/status` and `/api/mailrelay/send`.

- [x] 7.2 Implement method validation and request buffering with existing API helpers.
  - Used `api_buffer_post_data`, `api_parse_json_body`, and a local POST-only method check.
  - Verification: `mku mailrelay_send_test` rejects wrong method and invalid JSON.

- [x] 7.3 Require authentication for send/preview/status.
  - Reused `extract_and_validate_jwt` / `validate_jwt_claims` and added a `mail_send` role check for `POST /api/mailrelay/send`. `GET /api/mailrelay/status` continues to require only a valid JWT with a database claim.
  - Verification: `mku mailrelay_send_test` rejects missing JWT, invalid JWT, and JWT without `mail_send` role.

- [x] 7.4 Define the send request contract.
  - `POST /api/mailrelay/send` requires `template_key`, `idempotency_key`, and at least one recipient in `to`/`cc`/`bcc`. Optional `params` object and integer `priority`. Raw subject/body send is not exposed; `from`/`reply_to` are forced from `MailRelay.DefaultFrom`/`DefaultReplyTo`.
  - Verification: `mku mailrelay_send_test` validates required fields and rejects malformed recipient arrays.

- [x] 7.5 Wire `POST /api/mailrelay/send` to the internal templated-send producer API.
  - `POST /api/mailrelay/send` calls `mailrelay_send_template()` and returns `{success:true, message_id, status:"queued"}` on success. Producer error strings are mapped to stable API error codes (`MAIL_TEMPLATE_NOT_FOUND`, `MAIL_PARAM_MISSING`, `MAIL_RECIPIENT_INVALID`, `MAIL_QUEUE_FULL`, `MAIL_DISABLED`, etc.) and appropriate HTTP status codes.
  - Verification: `mku mailrelay_send_test` passes (8/8).

- [x] 7.5a Implement `POST /api/mailrelay/preview`.
  - Requires `mail_send` role. Request body contains `template_key` and optional `params`; no recipients or idempotency key are required. Response returns `success`, `template_key`, resolved `from`/`reply_to`, rendered `subject`, optional `text_body`/`html_body`, and a `macros_used` array listing every built-in and user macro that was resolved.
  - Macro tracking is implemented by extending the template engine: `MailRelayMacroList`, `mailrelay_template_render_with_macros()`, and `mailrelay_template_preview_with_macros()` record deduplicated macro names during rendering. The original `mailrelay_template_render()` and `mailrelay_template_preview()` remain unchanged wrappers.
  - Verification: `mku mailrelay_preview_test` passes (9/9); `mku mailrelay_template_preview_test` passes (8/8).

- [x] 7.5b Add blackbox `test_58_mailrelay_api.sh`.
  - Full 7-engine × plaintext/STARTTLS matrix; unique result-file suffixes; SQLite WAL/SHM copies; unique ports; docs in `docs/H/tests/test_58_mailrelay_api.md`.
  - Chunk 1 (roles): password login loads roles; mailrelay authorizes by `mail_send` **role_id** via QueryRef #127; Unity fixtures use role_id claims.
  - Chunk 2 (readiness): `READY FOR REQUESTS`, shorter timeouts.
  - Auth recovery: pending-result register-before-submit; JWT MHD contract; revoke polarity; test_40 logout uses renewed token.
  - SQLite same-second validity: nine original QueryRefs use `valid_after <=` / `valid_until >=`.
  - Fail-soft harness (v2.2.0): parallel workers always return 0; pass/fail in result files; suite always reaches analysis/summary.
  - Verification (user, 2026-07-09): `test_40`, `test_57`, `test_58` green; Unity, shellcheck, cppcheck, Test 90, Test 99 green.

- [x] 7.6 Add Swagger/OpenAPI documentation.
  - Documented `POST /api/mailrelay/send`, `POST /api/mailrelay/preview`, `GET /api/mailrelay/status` via `//@ swagger:*` annotations on endpoint headers; public OpenAPI tag is **"Mail Service"** (`mailrelay_service.h`).
  - Bearer JWT + database claim on all three; `mail_send` role on send/preview; full stable error codes; document-as-implemented request/response shapes.
  - Regenerated `payloads/swagger.json` and encrypted payload; paths appear as `/mailrelay/send`, `/mailrelay/preview`, `/mailrelay/status`.
  - Verification: `swagger-generate.sh` validation passed; `test_22_swagger.sh` 17/17 PASS.

Exit Gate: authenticated clients can preview and queue templated emails; behavior is documented and tested. `mkt`, `mkp`, the new `mku` set, `test_58`, and `test_22_swagger.sh` pass.

Phase 7 Status: complete. Date: 2026-07-10. Result: 7.1–7.6 complete. REST send/preview/status, multi-engine blackbox (`test_58`), auth/role recovery, and Swagger OpenAPI (tag "Mail Service") are green. Next: Phase 7A Lua backfill. Variances: Swagger was documented after handlers/blackbox (document-as-implemented) rather than design-first; public tag is "Mail Service" while internal paths remain `mailrelay`.

### Phase 7.5b diagnosis (2026-07-08) — RESOLVED 2026-07-09

Historical diagnosis kept for context. All defects below are fixed; 7.5b is `[x]`.

`test_58_mailrelay_api.sh` reached `mailadmin` login + `GET /api/mailrelay/status` on all engines, then failed at `preview`/`send`. Root cause was TWO distinct defects (both fixed), plus later auth/SQLite/harness issues also fixed.

Defect 1 — Password login never loads roles (JWT `"roles":""`):

- `lookup_account()` sets `account->roles = NULL` (`src/api/auth/auth_service_database.c:258`); its comment claims roles are populated during password verification, but that never happens.
- `verify_password_and_status()` fills only `account->username`, never roles (`src/api/auth/auth_service_database.c:321-329`).
- `generate_jwt()` serializes `account->roles ? account->roles : ""` (`src/api/auth/auth_service_jwt.c:143`), so a NULL becomes `"roles":""`.
- The OIDC path does it correctly: `oidc_rp_roles_apply()` (QueryRef #017) is called before `generate_jwt()` (`src/api/auth/oidc_rp/oidc_rp_callback.c:481`). The password path has NO equivalent role-loading call between `verify_password_and_status()` and `generate_jwt()` (`src/api/auth/login/login.c:319-334`).

Defect 2 — Format mismatch between the roles claim and the mailrelay role check (this is the part the earlier note missed):

- The canonical JWT `roles` claim is a comma-separated list of **role_id integers**, e.g. `"1,3"` (documented in `src/api/auth/oidc_rp/oidc_rp_roles.c:8-40`; QueryRef #017 returns `role_id` integers, one per row, from `account_roles`).
- `mail_send` is `role_id = 1`, `name = 'mail_send'` (migration `acuranzo_1257.lua`).
- But the mailrelay API's `has_role(roles, "mail_send")` in `src/api/mailrelay/send/send.c:68`/`:428` and `src/api/mailrelay/preview/preview.c:57`/`:310` searches the claim for the literal **name** `"mail_send"`, which will never appear in a role_id-based claim. So even after Defect 1 is fixed, the OIDC-shaped claim contains `"1"` and `has_role(..., "mail_send")` still fails.

Concern 3 — `test_58` apparent hang (separate from auth; Chunk 2):

- The `set -x` trace the user shared actually shows the SQLite-plaintext variant COMPLETING (it wrote `VARIANT_plaintext_FAIL` and returned 0 from `run_engine_test`, then `grep -q VARIANT_plaintext_PASS` returned 1). So that variant did not hang.
- The 14-variant matrix (7 engines × plaintext/STARTTLS) is launched in parallel; `CORES` = `nproc*2` (24 on this box, `tests/lib/framework.sh:139-141`), so all 14 launch at once with no throttle. Variants for engines not running locally (e.g. DB2/CockroachDB/YugabyteDB) or STARTTLS variants likely stall in `wait_for_migrations` (180 s `MIGRATION_TIMEOUT`) or `start_hydrogen_with_pid` (25 s), making the overall run look hung for minutes before `wait "${pid}"` collects them.
- Chunk 2 will make the matrix fail-fast / skip unreachable engines and/or reduce timeouts, mirroring how `test_30`/`test_40` gate engine availability.

### Phase 7.5b resume plan (Chunk 1 / Chunk 2)

Locked decisions (from user, 2026-07-08):

1. Authorization keeps the canonical role_id-based JWT claim; the mailrelay API authorizes by the `mail_send` **role_id**, not the name string.
2. The role-id for `mail_send` is resolved at request time via a **new QueryRef** (name → role_id), NOT a hardcoded `"1"` constant. (User explicitly chose the QueryRef resolver over a magic-number constant.)
3. The QueryRef #017 database role loader (currently `static` in `oidc_rp_roles.c`) is promoted to a **non-static, testable** shared function reused by the password login path.
4. Work is chunked: Chunk 1 = auth roles fix (unblock preview/send); Chunk 2 (separate) = `test_58` multi-engine hang hardening. 7.6 Swagger comes after.

Chunk 1 — Auth roles fix (COMPLETE). Concrete steps:

- Part A — New QueryRef **#127 "Get Role By Name"**, migration **`acuranzo_1260.lua`** (next free migration is 1260; next free QueryRef is #127 — highest existing is #126). Returns `role_id` from `roles` for a bound `:ROLENAME` where the role is active (`status_a34 = 1`). Type internal/system (NOT `public`/`protected`). Model exactly on QueryRef #017 migration `acuranzo_1108.lua`. The `roles` table has `role_id` and `name` columns (`acuranzo_1016.lua:40,44`). Verified by `elements/002-helium/scripts/helium_update.sh`, `tests/test_98_luacheck.sh`, and `tests/test_34_sqlite_migrations.sh` (260 migrations reversed).
- Part B — Promoted `roles_from_database()` in `src/api/auth/oidc_rp/oidc_rp_roles.c:160` to non-static `auth_roles_from_database()`, declared it in `src/api/auth/auth_service.h`, and updated the three OIDC call sites in `oidc_rp_roles.c`.
- Part C — In `src/api/auth/login/login.c`, after `verify_password_and_status()` succeeds and before `generate_jwt()`, `account->roles` is loaded via `auth_roles_from_database(account->id, database)` with `strdup("")` fallback on transport failure.
- Part D — Added shared `src/api/mailrelay/mailrelay_api_auth.{c,h}` with `mailrelay_api_has_role()`. It resolves the `mail_send` role_id via QueryRef #127 through the Mail Relay repository seam, then checks the JWT `roles` claim for that integer id. Both `send.c` and `preview.c` now call this helper and no longer duplicate the static `has_role` logic.
- Test updates: `mku mailrelay_send_test` and `mku mailrelay_preview_test` fixtures now use role-id claim `"1"` and mock `MAILRELAY_QREF_ROLE_GET_BY_NAME` to return `role_id=1`. New focused Unity test `oidc_rp_roles_test_auth_roles_from_database` covers null args, no rows, single/multiple roles, query failure, malformed JSON, and uppercase `ROLE_ID`.
- Chunk 1 verification: `mkt` → `mkp` → `mku mailrelay_send_test && mku mailrelay_preview_test && mku mailrelay_template_preview_test && mku oidc_rp_roles_test_auth_roles_from_database && mku oidc_rp_roles_test_apply` → `mka`; plus `tests/test_98_luacheck.sh` and `tests/test_34_sqlite_migrations.sh`. All green. The full 14-variant `test_58` is intentionally left for Chunk 2.

Chunk 2 — `test_58` hang hardening (IMPLEMENTED, 2026-07-09):

- Replaced `wait_for_migrations()` with `wait_for_ready_for_requests()` keyed off the canonical `READY FOR REQUESTS` log line.
- Reduced `STARTUP_TIMEOUT` from 25s to 15s and replaced the 180s `MIGRATION_TIMEOUT` with a 30s `READY_TIMEOUT` for faster failure on unreachable engines.
- Updated `docs/H/tests/test_58_mailrelay_api.md` to describe the new readiness behavior.
- Verified `test_92_shellcheck.sh`, `test_90_markdownlint.sh`, and `test_04_check_links.sh` are green.
- Running `test_58` now exposes a new blocker: Hydrogen crashes during `/api/auth/login` when multiple engine variants run in parallel. See the Working Log entry "Phase 7.5b Chunk 2 follow-up (2026-07-09)".

Chunk 2 alone did not mark 7.5b complete (login crash + SQLite validity + fail-soft still open then). All three follow-ups are now done; 7.5b is `[x]` as of 2026-07-09 closeout.

Files touched map for Chunk 1 (for quick orientation on resume):

- `elements/002-helium/acuranzo/migrations/acuranzo_1260.lua` (new; QueryRef #127).
- `src/api/auth/auth_service.h` — declare `auth_roles_from_database()`.
- `src/api/auth/oidc_rp/oidc_rp_roles.c` (+ its header) — de-static + rename the loader; update 3 call sites.
- `src/api/auth/login/login.c` — call the loader before `generate_jwt`.
- `src/mailrelay/mailrelay_repository.{c,h}` — add `MAILRELAY_QREF_ROLE_GET_BY_NAME` and `mailrelay_repo_role_get_by_name()`.
- `src/api/mailrelay/mailrelay_api_auth.{c,h}` (new) — shared `mailrelay_api_has_role()` helper.
- `src/api/mailrelay/send/send.c`, `src/api/mailrelay/preview/preview.c` — use shared helper; remove duplicated static `has_role`.
- `tests/unity/src/api/mailrelay/send/mailrelay_send_test.c`, `tests/unity/src/api/mailrelay/preview/mailrelay_preview_test.c` — update role claim format to role_id and mock QueryRef #127.
- `tests/unity/src/api/auth/oidc_rp/oidc_rp_roles_test_auth_roles_from_database.c` (new) — focused tests for the promoted loader.

### Phase 7.5b blackbox test builder notes

- **Credentials**: read `HYDROGEN_MAILADMIN_NAME`, `HYDROGEN_MAILADMIN_PASS`, and `HYDROGEN_MAILADMIN_EMAIL` from the environment only; do not hardcode defaults.
- **Known IDs**: `mail_send` role has `role_id = 1`; `mailadmin` account has `account_id = 5`; contacts use `contact_id = 7` (username) and `contact_id = 8` (email); `mail.test` template has `template_id = 1`.
- **Template**: `mail.test` has subject `"Hello %NAME% from %APP_NAME%"`, text body with `%NAME%`, `%APP_NAME%`, `%SERVER_NAME%`, `%TIMESTAMP%`, and an HTML body with the same macros.
- **Engine matrix**: include PostgreSQL, MySQL, SQLite, DB2, MariaDB, CockroachDB, and YugabyteDB, mirroring the loop structure in `test_30_database.sh` / `test_40_auth.sh`.
- **Transport variants**: for each engine, run one subtest against a plaintext `mailval` sink and one against a STARTTLS `mailval` sink (dedicated ports per engine).
- **API checks per variant**:
  1. `POST /api/auth/login` as `mailadmin` → obtain JWT.
  2. `GET /api/mailrelay/status` → expect `success:true` and counters.
  3. `POST /api/mailrelay/preview` with `{"template_key":"mail.test","params":{"NAME":"MailRelayBlackbox"}}` → expect rendered subject/body, resolved `from`/`reply_to`, and `macros_used` containing `NAME`, `APP_NAME`, `SERVER_NAME`, `TIMESTAMP`.
  4. `POST /api/mailrelay/send` with `template_key`, `idempotency_key`, `to`, and `params` → expect `success:true`, `message_id`, `status:"queued"`.
  5. Poll `mailval` capture for a stored message whose subject contains the rendered `%NAME%` value.
  6. Negative: request without JWT or with a user lacking `mail_send` → expect 401 / `MAIL_AUTH_REQUIRED`.
- **Cleanup**: stop Hydrogen with `SIGINT`, stop `mailval` with `SIGINT`, and clean per-engine SQLite files in the test diagnostic directory to avoid cross-run state.
- **Existing patterns to reuse**: `start_hydrogen_with_pid` / `stop_hydrogen` from `tests/lib/lifecycle.sh`, mailval startup probe from `test_57_mailrelay_outbound.sh`, and config merging with `jq` to inject the MailRelay section into existing `test_40_*` configs.

### Draft API Contract

`POST /api/mailrelay/send`

```json
{
  "template_key": "mail.test",
  "to": ["user@example.com"],
  "cc": [],
  "bcc": [],
  "params": {
    "NAME": "Example User"
  },
  "idempotency_key": "client-generated-unique-key",
  "priority": "normal"
}
```

Success:

```json
{
  "success": true,
  "message_id": "...",
  "status": "queued"
}
```

Errors use Hydrogen's normal API error envelope (`success:false`, `error`, `message`) with stable mail codes: `MAIL_AUTH_REQUIRED`, `MAIL_TEMPLATE_NOT_FOUND`, `MAIL_PARAM_MISSING`, `MAIL_QUEUE_FULL`, `MAIL_RECIPIENT_INVALID`, `MAIL_DISABLED`, `MAIL_RATE_LIMITED`.

---

## Phase 7A - Lua `H.mail` / `H.notify` Backfill

Objective: Replace the completed Lua stub surface with real Mail Relay integration without creating a second mail path.

Entry Gate: Phase 7 exit gate green; Lua Phase 19 stubs verified in `LUA_PLAN_COMPLETE.md`; internal templated-send producer API exists.

**Chunking (locked 2026-07-10):** ship **7A.1 + 7A.2** first as one reviewable chunk; then 7A.3, 7A.4, 7A.5, 7A.6 separately. Full contract and file list: Working Log → "Phase 7A implementation vector".

- [ ] 7A.1 Audit stubs + lock contract + wire `H.wait` for MAIL/NOTIFY.
  - Files: `scripting_api_mail_notify.c`, `scripting_api.h`, `scripting_handle.{c,h}`, `scripting_api_query.c` (`H_lua_wait_one`), `lua_context.c` as needed.
  - Contract: template-first table (`template` or `template_key`); at least one recipient; optional `params` / `priority` / `idempotency_key` (auto UUID if omitted); reject raw subject/body-only; success wait result `{ message_id, status = "queued" }`.
  - Fix: dispatch `H_HK_MAIL` / `H_HK_NOTIFY` in `H_lua_wait_one` (declared `H_lua_mail_notify_wait_one` is not wired today).
  - Verification: Unity covers wait error/success paths for mail handles (with 7A.2).

- [ ] 7A.2 Replace `H.mail.send` with Mail Relay producer wrapper (**same chunk as 7A.1**).
  - Map Lua table → `MailRelaySendTemplateRequest` → `mailrelay_send_template()`; return `H_HK_MAIL` handle with success fields or `mail_error` (`MAIL_*`).
  - Unity with **mocked producer seam** (no live SMTP/DB).
  - Verification: `mku scripting_api_mail_test` (or equivalent) for queued success, missing template, invalid recipients, disabled Mail Relay, auto idempotency key.

- [ ] 7A.3 Replace `H.mail.send_sync` with wait-based behavior (after 7A.1–2).
  - Contract: wait only for **queue acceptance**, not SMTP delivery.
  - Verification: Unity for queued result, invalid template/recipient, disabled, error mapping.

- [ ] 7A.4 Define `H.notify` compatibility behavior (after 7A.3).
  - **Locked v1:** stable deferred error (`notify: deferred to mailrelay rules` or equivalent); **no** silent success; **no** channel→template mapping in 7A. Real rule mapping deferred.
  - Verification: Unity for deferred error on send/send_sync/wait; no accidental enqueue.

- [ ] 7A.5 Preserve Lua safety and audit boundaries.
  - Lua must not call REST or SMTP for mail; same producer as REST/events (validation, rate limits, redaction, idempotency, audit).
  - Verification: no secrets/OTP/JWT/body in normal logs/artifacts.

- [ ] 7A.6 Update scripting docs and Lua completion plan cross-reference.
  - Files: `/docs/H/core/subsystems/scripting/lua_api.md` (replace legacy subject/body shape), `/docs/H/plans/LUA_PLAN_COMPLETE.md`, Working Log if needed.
  - Verification: `test_04_check_links.sh` and `test_90_markdownlint.sh` for touched docs.

Exit Gate: Lua scripts can queue templated mail through Mail Relay via `H.mail`; `H.notify` has explicit deferred compatibility behavior; no parallel mail path. `mkt`, `mkp`, Lua mail Unity tests, and relevant doc/link lints pass.

Phase 7A Status: pending (contract locked 2026-07-10; **not coded**). Date: (TBD). Result: (TBD). Variances: first chunk is 7A.1+7A.2 together; notify is deferred-error only (stricter than optional rule mapping).

---

## Phase 8 - OTP / MFA Mail Support

Objective: Add one-time password generation, storage, sending, and verification for authentication workflows.

Entry Gate: Phase 7 exit gate green.

- [ ] 8.1 Decide OTP relationship to existing auth/OIDC flows (record in Working Log).
  - OTP is a mail feature consumed by auth, not an auth bypass.
  - Verification: decision recorded with exact endpoint and auth ownership.

- [ ] 8.2 Create the `mail_otp_codes` table (Acuranzo migration).
  - Store hashed code, user/account reference, email, purpose, expiry, attempts, consumed timestamp, audit columns. Never store plaintext OTP after send.
  - Verification: migration applies/reverses; `test_98_luacheck.sh` passes; schema docs exist.

- [ ] 8.3 Implement OTP generator.
  - Use existing OpenSSL/random utility patterns. Configurable digits/length and expiry.
  - Verification: `mku mailrelay_otp_generate_test` covers randomness seam, length, expiry, hashing, max attempts.

- [ ] 8.4 Implement the OTP send path.
  - Uses `auth.otp_code` template and the Mail Relay queue.
  - Verification: blackbox sends an OTP mail to the sink; DB stores only the hashed code.

- [ ] 8.5 Implement the verification endpoint/helper.
  - Validate code, expiry, attempts, consumed state, purpose.
  - Verification: `mku mailrelay_otp_verify_test` full generate/send/verify/consume roundtrip and negative cases.

Exit Gate: OTP codes can be generated, mailed, verified once, expire correctly, and never expose plaintext in storage or logs. `mkt`, `mkp`, the new `mku` set, migration checks, and `test_02_secrets.sh` pass.

Phase 8 Status: pending. Date: (TBD). Result: (TBD). Variances: (TBD).

---

## Phase 9 - Lithium Mail Manager and Profile Email Integration

Objective: Replace client placeholders with minimal useful UI backed by the new API.

Entry Gate: Phase 7 exit gate green; Phase 10 status counters available (at least partial).

- [ ] 9.1 Update Lithium `MailManager` from placeholder to a status dashboard.
  - Location: `/elements/003-lithium/src/managers/mail-manager`. Show enabled state, queue counts, recent failures, and a test-send action backed by `/api/mailrelay/status`.
  - Verification: UI unit/build test passes; manual smoke shows live data.

- [ ] 9.2 Add template preview/test-send UI.
  - Verification: preview does not send; test send goes only to a configured/admin recipient.

- [ ] 9.3 Wire profile email settings if needed.
  - Location: `/elements/003-lithium/src/managers/profile-manager/pages/email`. Use existing profile-manager settings patterns; update only via authenticated API.
  - Verification: user can view/update email preferences only through authenticated API.

- [ ] 9.4 Align manager routing/permissions.
  - Ensure the Mail Manager menu item is visible only to appropriate roles if role gating exists.
  - Verification: role/visibility behavior documented and manually verified.

Exit Gate: the placeholder Mail Manager becomes an operational dashboard/test console without exposing unrestricted raw mail sending. UI build/lint passes.

Phase 9 Status: pending. Date: (TBD). Result: (TBD). Variances: (TBD).

---

## Phase 10 - Observability, Metrics, and Operations

Objective: Make Mail Relay observable and supportable.

Entry Gate: Phase 3 and Phase 4 exit gates green.

- [x] 10.1 Add status counters.
  - Counters: queued, sending, sent, failed, retrying, permanent failures, last success, last failure, worker count, queue depth. Implemented in Phase 7.1 and surfaced via `GET /api/mailrelay/status`.
  - Verification: `GET /api/mailrelay/status` returns stable JSON.

- [ ] 10.2 Add Prometheus metrics.
  - Follow the existing `/api/system/prometheus` pattern.
  - Verification: blackbox confirms metrics appear when Mail Relay is enabled.

- [ ] 10.3 Add structured logging with redaction.
  - Log message IDs, templates, result codes, timings via `log_this(SR_MAIL_RELAY, ...)`; never passwords, OTP plaintext, or full bodies unless an explicit debug/test-only flag allows it.
  - Verification: manual log review from a blackbox run shows no secrets.

- [ ] 10.4 Add operational cleanup.
  - Retention policies for sent/failed mail and OTP rows.
  - Verification: a unit or migration test confirms cleanup only removes eligible records.

- [ ] 10.5 Add operator/user documentation.
  - Add docs under `/docs/H/` (configuration example, local SMTP sink testing, troubleshooting). Use absolute links per Markdown rules.
  - Verification: `test_04_check_links.sh` and `test_90_markdownlint.sh` pass.

Exit Gate: operators can see queue health, diagnose failures, and safely retain/clean mail records. `mkt`, `mkp`, relevant `mku`, and doc/link lints pass.

Phase 10 Status: partial complete. Date: 2026-07-08. Result: 10.1 status counters implemented as part of Phase 7.1 and exposed through `/api/mailrelay/status`. Remaining 10.2–10.5 (Prometheus metrics, structured logging, operational cleanup, docs) pending. Variances: Counters were pulled forward from Phase 10 into Phase 7.1 per user direction.

---

## Phase 11 - HA and Multi-Instance Safety

Objective: Allow multiple Hydrogen instances to share a durable mail queue without duplicate sends.

Entry Gate: Phase 4 exit gate green.

- [ ] 11.1 Define claim semantics per database engine.
  - Use portable locking/update patterns or engine-specific QueryRefs.
  - Verification: design verified against PostgreSQL, MySQL, SQLite, and DB2 limitations and recorded in the Working Log.

- [ ] 11.2 Implement atomic claim-next-pending behavior.
  - Avoid two instances sending the same message.
  - Verification: `mku mailrelay_claim_test` (or blackbox) with two synthetic instances claims each message once.

- [ ] 11.3 Implement stale-claim recovery.
  - Messages stuck in `sending` after a crash return to `pending` after timeout.
  - Verification: restart/recovery blackbox proves recovery and no duplicates beyond the accepted retry policy.

- [ ] 11.4 Enforce idempotency.
  - Duplicate API requests with the same idempotency key return the existing queued/sent status.
  - Verification: API test posts a duplicate request and observes one mail delivered.

Exit Gate: multi-instance Hydrogen processes a shared mail queue with no normal duplicate sends and predictable crash recovery.

Phase 11 Status: pending. Date: (TBD). Result: (TBD). Variances: (TBD).

---

## Phase 12 - Inbound SMTP Relay

Objective: Add controlled inbound SMTP submission/routing, only after outbound is reliable and security gates are green.

Entry Gate: Phase 2, Phase 3 green and Phase 14 partial (anti-open-relay rules designed).

- [ ] 12.1 Decide inbound scope and threat model (record in Working Log).
  - Recommended v1: submission-only listener for trusted/internal clients; no public MX/open-relay behavior.
  - Verification: scope and threat model recorded.

- [ ] 12.2 Implement a basic SMTP listener skeleton.
  - Handle connection, EHLO/HELO, MAIL FROM, RCPT TO, DATA, QUIT. STARTTLS/auth may be required before accepting mail. Uses the Network subsystem (already a launch dependency) and a worker thread tracked via `mailrelay_threads`.
  - Verification: `test_59_mailrelay_inbound.sh` (ports 5590-5596) connects and completes a rejected/accepted minimal SMTP conversation. Add config and docs.

- [ ] 12.3 Add relay authorization and anti-open-relay rules.
  - Restrict source networks, authenticated users, sender/recipient domains, and route rules.
  - Verification: security blackbox proves unauthorized external relay attempts are rejected.

- [ ] 12.4 Add From-based filtering and routing.
  - Map allowed sender/from/domain to template/rewrite/outbound recipients (Canvas-style routing).
  - Verification: `mku mailrelay_route_test` accepts/rejects route cases.

- [ ] 12.5 Apply rewrite/template and re-send outbound.
  - An accepted inbound message becomes a normal queued outbound message after policy/filtering.
  - Verification: end-to-end test: inbound to local listener, outbound captured by the SMTP sink.

- [ ] 12.6 Add an LMTP option stub only if a concrete consumer exists.
  - Verification: the config toggle parses and logs "unsupported/deferred" rather than silently doing nothing.

Exit Gate: inbound relay is opt-in, not an open relay, and can rewrite/route a controlled mail flow through the outbound queue. `mkt`, `mkp`, `test_59`, and security negatives pass.

Phase 12 Status: pending. Date: (TBD). Result: (TBD). Variances: (TBD).

---

## Phase 13 - Additional Lua and Extension Hooks

Objective: Add optional mail extension hooks beyond the Phase 7A `H.mail` backfill only after core security is stable.

Entry Gate: Phase 7A and Phase 14 (partial) green.

- [ ] 13.1 Identify any remaining Lua or extension runtimes that need mail access beyond `H.mail`.
  - The primary Lua scripting runtime is handled in Phase 7A. Do not add additional mail APIs unless a concrete non-Phase-7A consumer exists.
  - Verification: consumer and lifecycle documented; if none, mark this phase deferred.

- [ ] 13.2 Add a minimal C wrapper if justified.
  - The API enqueues templated mail only through the same internal Mail Relay producer; no raw unrestricted SMTP from scripts or extensions.
  - Verification: an extension/Lua test queues one templated mail with restricted permissions.

- [ ] 13.3 Add guardrails.
  - Rate limits, allowed templates, allowed recipients, audit records.
  - Verification: security unit tests reject unauthorized template/recipient/script calls.

Exit Gate: any additional extension mail access exists only if needed and is constrained to audited, rate-limited template sends through Mail Relay.

Phase 13 Status: pending. Date: (TBD). Result: (TBD). Variances: (TBD).

---

## Phase 14 - Security, Abuse Controls, and Production Hardening

Objective: Complete the security checklist before enabling production mail.

Entry Gate: all active surfaces from selected phases implemented.

- [ ] 14.1 Secret-handling review.
  - SMTP passwords, OTPs, JWTs, API keys never appear in logs, dumps, queue JSON, attempts, or tests.
  - Verification: `test_02_secrets.sh` plus a manual grep of blackbox logs.

- [ ] 14.2 Recipient and sender policy.
  - Validate sender domains, allowed envelope-from, admin recipients, optional blocklists.
  - Verification: unit tests for allowed/rejected senders and recipients.

- [ ] 14.3 Rate limits.
  - Per user, per IP, per template, per subsystem event, global queue.
  - Verification: API blackbox exceeds a limit and receives a stable `MAIL_RATE_LIMITED` error.

- [ ] 14.4 TLS policy.
  - Configurable minimum TLS for outbound and inbound.
  - Verification: a local/blackbox test confirms TLS-required mode rejects plain SMTP where applicable.

- [ ] 14.5 Header injection and content safety.
  - Reject CR/LF in header fields, validate display names, bound body sizes.
  - Verification: unit tests cover injection attempts.

- [ ] 14.6 Open-relay review.
  - Inbound relay cannot send to arbitrary external recipients from arbitrary sources.
  - Verification: manual checklist plus blackbox negative tests pass.

Exit Gate: Mail Relay passes a security review and can be enabled in a real deployment with documented defaults. `mkt`, `mkp`, `test_02_secrets.sh`, and the security tests pass.

Phase 14 Status: pending. Date: (TBD). Result: (TBD). Variances: (TBD).

---

## Phase 15 - Full Integration and Release Gate

Objective: Validate complete functionality and update documentation.

Entry Gate: all selected prior phases green.

- [ ] 15.1 Run build/test/lint gates.
  - Required: `mkt`, `mka`, all relevant `mku ...`, `mkp`, `mks`. Blackbox: `test_57_mailrelay_outbound.sh`, `test_58_mailrelay_api.sh`, and `test_59_mailrelay_inbound.sh` if inbound is implemented.
  - Verification: all pass, or failures are documented as unrelated baseline issues.

- [ ] 15.2 Run database migration checks.
  - At least SQLite and one server engine initially; all supported engines before final release.
  - Verification: migrations apply/reverse cleanly; `test_98_luacheck.sh` passes.

- [ ] 15.3 Run startup/shutdown with Mail Relay disabled and enabled.
  - Verification: `test_17_startup_shutdown.sh` plus a mail-enabled config variant pass.

- [ ] 15.4 Update plan completion status.
  - Fill in every phase Status block; capture variances and the Working Log. Add completion summaries like the Chat/CAP plans.
  - Verification: this document reflects actual implementation, not just intended work.

- [ ] 15.5 Update operator/user docs and Lithium help text.
  - Verification: `test_04_check_links.sh`, `test_90_markdownlint.sh`, and the UI build pass.

Exit Gate: Mail Relay is implemented, tested, documented, observable, and secure enough for production opt-in.

Phase 15 Status: pending. Date: (TBD). Result: (TBD). Variances: (TBD).

---

## Subsystem File Map (lock in Phase 0.5)

New or modified files expected across the implementation. Confirm/adjust during Phase 0.5.

### Hydrogen C source (`/elements/001-hydrogen/hydrogen/src/`)

- `mailrelay/mailrelay.h`, `mailrelay/mailrelay_internal.h`, `mailrelay/mailrelay.c` (lifecycle/init/shutdown).
- `mailrelay/mailrelay_message.{c,h}` (message model + validation).
- `mailrelay/mailrelay_render.{c,h}` (RFC 5322 + MIME rendering).
- `mailrelay/mailrelay_smtp.{c,h}` (libcurl SMTP/SMTPS sender).
- `mailrelay/mailrelay_queue.{c,h}` (bounded in-memory queue with time-aware dequeue).
- `mailrelay/mailrelay_workers.{c,h}` (worker pool).
- `mailrelay/mailrelay_retry.{c,h}` (retry classification + exponential backoff).
- `mailrelay/mailrelay_debounce.{c,h}` (debounce/coalescing, Phase 3.5).
- `mailrelay/mailrelay_repository.{c,h}` (DB persistence via QueryRefs).
- `mailrelay/mailrelay_template.{c,h}` (macro engine).
- `mailrelay/mailrelay_otp.{c,h}` (Phase 8).
- `mailrelay/mailrelay_smtp_listener.{c,h}` (Phase 12, inbound).
- `mailrelay/mailrelay_result.{c,h}` (structured result type).
- Modify: `config/config_mail_relay.{c,h}`, `config/config_defaults.c`, `launch/launch_mail_relay.c`, `landing/landing_mail_relay.c`, `state/state.c` (add `mailrelay_threads`), `threads/threads.h`, `status/status_process.c`, `api/api_service.c`, `cmake/CMakeLists.txt`.
- API: `api/mailrelay/send/`, `api/mailrelay/preview/`, `api/mailrelay/status/` (Phase 7).
- Lua backfill: modify `scripting/scripting_api_mail_notify.c`, `scripting/scripting_api.h`, `scripting/scripting_handle.{c,h}`, and any wait-dispatch helpers needed to replace `H.mail` / `H.notify` stubs with Mail Relay producer integration (Phase 7A).

### Unity tests (`/elements/001-hydrogen/hydrogen/tests/unity/src/`)

- Extend: `config/config_mail_relay_test_load_mailrelay_config.c`, `launch/launch_mail_relay_test_comprehensive_coverage.c`, `landing/landing_mail_relay_test_readiness.c`.
- New: `mailrelay/mailrelay_message_test.c`, `mailrelay_render_test.c`, `mailrelay_queue_test.c`, `mailrelay_workers_test.c`, `mailrelay_retry_test.c`, `mailrelay_debounce_test.c`, `mailrelay_template_test.c`, `mailrelay_preview_test.c`, `mailrelay_producer_test.c`, `mailrelay_repository_test.c`, `mailrelay_otp_generate_test.c`, `mailrelay_otp_verify_test.c`, `mailrelay_route_test.c`, plus API endpoint tests under `api/mailrelay/` (e.g., `api/mailrelay/status/mailrelay_get_status_test.c`) and Lua backfill tests under `scripting/` such as `scripting_api_mail_test.c`.

### Blackbox tests (`/elements/001-hydrogen/hydrogen/tests/`)

- `test_57_mailrelay_outbound.sh` (+ `configs/hydrogen_test_57_mailrelay_outbound.json`, `docs/H/tests/test_57_mailrelay_outbound.md`).
- `test_58_mailrelay_api.sh` (+ config + doc).
  - `test_59_mailrelay_inbound.sh` (+ config + doc).
  - Optional local SMTP sink helper under `extras/smtp-sink/` (C binary, standalone CMake build).

### Mail validator tool (`/elements/001-hydrogen/hydrogen/extras/mailval/`)

- `CMakeLists.txt` (standalone build, not part of Hydrogen recursive discovery; links OpenSSL for TLS).
- `mailval.c` (arg parse + per-protocol listener threads).
- `listener.c/.h` (generic TLS-capable TCP accept loop, dispatches to a protocol module).
- `tls.c/.h` (OpenSSL context, STARTTLS + implicit TLS support).
- `capture.c/.h` (per-session transcript + parsed-field JSON writer).
- `store.c/.h` (shared in-memory mailbox store; SMTP delivery appends messages here, IMAP and JMAP serve from it).
- `proto_smtp.c/.h` (full SMTP send capture + store delivery; functional for tests 57/58/59).
- `proto_imap.c/.h` (full IMAP server over the store; handshake + capability negotiation + transcript capture).
- `proto_jmap.c/.h` (full JMAP server over HTTPS against the store; session/upload/download/get/set/query + transcript capture).

### Database migrations (`/elements/002-helium/acuranzo/migrations/`)

- `acuranzo_NNNN.lua` for `mail_templates`, `mail_queue`, `mail_attempts`, `mail_events`, `mail_otp_codes`, plus seed-template and internal-QueryRef migrations.

### Lithium UI (`/elements/003-lithium/`)

- `src/managers/mail-manager` (dashboard), `src/managers/profile-manager/pages/email` (settings).

---

## Phase Dependency Summary

| Phase | Depends on | Unlocks |
|---|---:|---|
| 0 Design/Baseline | none | clean implementation start |
| 1 Config/Launch | 0 | runtime subsystem startup |
| 2 SMTP Sender | 1 | actual outbound delivery |
| 3 Queue/Workers | 2 | async and retries |
| 4 DB Persistence | 3 | restart recovery and audit |
| 4F Query Result Cache | 4 | cached query results for Phase 5+ |
| 5 Templates | 4F | reusable system/API messages |
| 6 System Events | 5 | admin notifications |
| 7 REST API | 5 | client-triggered mail |
| 7A Lua Backfill | 7 | Lua `H.mail` / `H.notify` real integration |
| 8 OTP/MFA | 7 | auth mail workflows |
| 9 Lithium UI | 7, 10 partial | user/operator UI |
| 10 Observability | 3, 4 | operations readiness |
| 11 HA Safety | 4 | multi-instance deployment |
| 12 Inbound Relay | 2, 3, 14 partial | Canvas-style relay |
| 13 Additional Lua Hooks | 7A, 14 partial | controlled extension support beyond `H.mail` |
| 14 Security | all active surfaces | production enablement |
| 15 Release Gate | all selected phases | final completion |

---

## Phase 0 Questions Resolved During Prep Review

1. `Notify.SMTP` does not become a second active SMTP delivery path. Notify becomes a producer/compatibility layer that calls Mail Relay when implemented.
2. `MailRelay.Database` owns system mail tables when multiple `Databases[]` entries exist.
3. `/api/mailrelay/send` is template-key only for normal external clients. Raw send is internal/admin-test only behind explicit future gating.
4. Initial permissions: service/internal callers and administrators may send, preview, view status, and manage templates. User-facing send permission is template-scoped and JWT-claim-scoped during Phase 7.
5. Initial OTP use cases are login MFA and email verification. Password reset is compatible future work, not a Phase 8 gate unless explicitly pulled in.
6. Canvas/inbound workflow can wait. Phase 12 v1 is trusted/internal submission only, after outbound/API/OTP foundations are stable and anti-open-relay rules are designed.

---

## Initial Milestone Recommendation

For first usable value, implement Phases 0 through 7A:

1. Fix/lock config and schema (Phase 0-1).
2. Send one email to a local SMTP sink via libcurl (Phase 2).
3. Add in-memory queue/workers/retry and a common internal producer API (Phase 3).
4. Add DB persistence and templates (Phase 4-5).
5. Add admin "Server Started" event mail (Phase 6).
6. Add authenticated preview/send/status API (Phase 7).
7. Backfill existing Lua `H.mail` / `H.notify` stubs through the same producer API (Phase 7A).

Defer OTP, HA, inbound relay, optional Lua extension hooks beyond `H.mail`, and full Lithium management until the outbound queue + API/Lua path is stable.
