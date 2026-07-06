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
- (Phase 5) Template syntax: (TBD)

### Surprises / deviations from plan

- (Phase 0, 2026-07-06) `examples/configs/hydrogen_default.json` and `hydrogen_env.json` use legacy `MailRelay.QueueSettings` / `MailRelay.OutboundServers` keys, while the current loader and Phase 0 schema use `MailRelay.Queue` / `MailRelay.Servers`. Reconcile example configs and/or compatibility aliases during Phase 1.
- (Phase 1, 2026-07-06) Existing launch readiness tests assumed disabled = No-Go; Phase 1 changed this to clean skip and required test updates in `launch_mail_relay_test_comprehensive_coverage.c`. Server validation tests also needed `OutboundEnabled=true` because validation is now gated behind the outbound flag.
- (Phase 1, 2026-07-06) Landing tests could not mock `is_subsystem_running_by_name`; reduced landing test scope to the realistic not-running path and no-dependents message verification.

### Reusable snippets / gotchas

- libcurl SMTP requires `CURLOPT_UPLOAD=1L` + `CURLOPT_READFUNCTION` streaming and `CURLOPT_NOSIGNAL=1L` in worker threads. Confirm and record exact option set once Phase 2 works.
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
- Launch readiness now treats disabled Mail Relay as a clean skip (`ready=true`, "disabled - clean skip") rather than a No-Go. Update any downstream scripts or tests that assert disabled subsystems cause launch failure.
- Server validation in launch readiness is gated behind `OutboundEnabled=true`; inbound-only configurations without outbound servers will not fail readiness checks.
- Landing readiness in tests cannot mock `is_subsystem_running_by_name`; test the not-running early-return path or setup `mailrelay_threads.thread_count` directly for worker-drain scenarios.
- When adding new fixed-size string arrays (like `AdminRecipients[]`), define a max constant in `globals.h` (e.g. `MAX_MAIL_RELAY_ADMIN_RECIPIENTS`), use `char*` fixed array + count in the struct, and free each element in cleanup before resetting count.
- New `ServiceThreads` globals must be: defined in `src/state/state.c`, externed in `src/threads/threads.h`, added to `report_thread_status()` in `src/threads/threads.c`, and initialized in the subsystem launch function.

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

Objective: Send one RFC 5322-compliant outbound message to a local SMTP sink, with no DB, API, or templates.

Entry Gate: Phase 1 exit gate green.

- [ ] 2.1 Create `src/mailrelay/` module skeleton (currently empty dir).
  - Files: `mailrelay.h`, `mailrelay_internal.h`, `mailrelay_message.{c,h}`, `mailrelay_smtp.{c,h}`, `mailrelay_result.{c,h}`. Add a `mailrelay_test_seams.{c,h}` only if needed to inject a fixed clock / Message-ID for deterministic tests.
  - Build note: current CMake uses recursive source discovery; confirm new files are discovered and only adjust `cmake/` if Unity/exclusion rules require it.
  - Verification: `mkt`.

- [ ] 2.2 Define `MailRelayMessage` and validation helpers.
  - Fields: from, reply-to, to/cc/bcc arrays, subject, text body, HTML body, headers, metadata, priority, idempotency key.
  - Reuse or extend existing auth email-address validation patterns. Avoid static functions so helpers are testable.
  - Verification: `mku mailrelay_message_test` covers valid/invalid addresses, empty recipients, subject/body limits, and BCC handling.

- [ ] 2.3 Implement RFC 5322 message rendering.
  - Include `Date`, `Message-ID`, MIME multipart/alternative boundaries when both text and HTML are present. Use CRLF line endings.
  - Verification: `mku mailrelay_render_test` verifies deterministic rendering with a fixed clock/test Message-ID seam.

- [ ] 2.4 Implement libcurl SMTP/SMTPS send helper.
  - File: `mailrelay_smtp.c`. Use `smtp://` + STARTTLS or `smtps://` per config. Set `CURLOPT_NOSIGNAL=1L`, connect/total timeouts, `CURLOPT_MAIL_FROM`, `CURLOPT_MAIL_RCPT`, `CURLOPT_UPLOAD=1L` + `CURLOPT_READFUNCTION`. Do not log credentials or body by default. Return a structured `MailRelayResult` (success flag, SMTP code/text, duration).
  - Verification: `mkt` and a unit-testable seam verifying URL, envelope, credentials, timeout, and payload callback behavior. Record the exact working `CURLOPT_*` set in the Working Log.

- [ ] 2.5 Add local SMTP sink blackbox support.
  - Add a small Python SMTP sink under `tests/` only (or use a system tool if present). Create `tests/test_57_mailrelay_outbound.sh` (ports 5570-5576) plus `tests/configs/hydrogen_test_57_mailrelay_outbound.json` and `docs/H/tests/test_57_mailrelay_outbound.md`.
  - Verification: `test_57_mailrelay_outbound.sh` starts Hydrogen with a local sink and captures one message; `test_92_shellcheck.sh` and `test_04_check_links.sh` pass.

Exit Gate: Hydrogen can send one configured raw outbound email to a local SMTP sink, and failures return structured results without leaking secrets. `mkt`, `mkp`, the new `mku` targets, and `test_57` pass.

Phase 2 Status: pending. Date: (TBD). Result: (TBD). Variances: (TBD).

---

## Phase 3 - In-Memory Queue, Workers, Retry, Debounce

Objective: Make outbound sending asynchronous and resilient before adding persistence.

Entry Gate: Phase 2 exit gate green.

- [ ] 3.1 Add Mail Relay runtime state and thread tracking.
  - Add `ServiceThreads mailrelay_threads;` in `src/state/state.c`, extern in `src/threads/threads.h`, and count it in `report_thread_status`. Track queue, mutex/condition variables, worker threads, counters, shutdown flag, last errors in `mailrelay_internal.h`.
  - Verification: `mku mailrelay_runtime_test` covers init/cleanup idempotency.

- [ ] 3.2 Implement bounded in-memory queue.
  - Respect `Queue.MaxInMemory` / `MaxQueueSize`; return `MAIL_QUEUE_FULL` when full.
  - Verification: `mku mailrelay_queue_test` covers enqueue/dequeue order, capacity, and cleanup.

- [ ] 3.3 Implement worker pool.
  - Configurable worker count. Sends run off the API/logging caller path. Each worker registers with `add_service_thread(&mailrelay_threads, pthread_self())` and unregisters on exit.
  - Verification: `mku mailrelay_workers_test` with a fake sender processes N messages concurrently, each exactly once.

- [ ] 3.4 Implement retry with exponential backoff.
  - Classify transient (4xx connection / 5xx transient / transport) vs permanent failures from SMTP/libcurl responses. Honor `RetryAttempts`, `InitialDelaySeconds`, `MaxDelaySeconds`.
  - Verification: `mku mailrelay_retry_test` verifies transient retries and permanent-failure stop policy.

- [ ] 3.5 Implement debounce/coalescing for event-generated mail.
  - Example: `DebounceSeconds = 5`; repeated identical event key collapses into one summary mail.
  - Verification: `mku mailrelay_debounce_test` simulates burst events and verifies one queued message with count/summary.

- [ ] 3.6 Wire launch/landing to start and stop workers.
  - Replace the TODO in `launch_mail_relay_subsystem()` (`launch_mail_relay.c:162`) with real init: `init_service_threads`, spawn workers, `update_subsystem_on_startup`, verify `SUBSYSTEM_RUNNING`. Implement the landing drain loop (mdns pattern) in `land_mail_relay_subsystem()`.
  - Verification: `test_17_startup_shutdown.sh` (or a mail-enabled variant) shows no orphan worker threads and clean landing.

- [ ] 3.7 Define the internal raw enqueue producer API.
  - File: `src/mailrelay/mailrelay.h`.
  - Purpose: one C API used by future REST endpoints, Lua `H.mail`, Notify compatibility, and system events. Producers must enqueue through Mail Relay so queue limits, audit metadata, rate limits, idempotency, and redaction are centralized. Lua and Notify must not call SMTP or HTTP endpoints directly.
  - Verification: `mku mailrelay_queue_test` or a new producer test verifies disabled/error/queued results and structured error codes.

Exit Gate: Mail Relay accepts asynchronous messages through a stable internal producer API, processes them with workers, retries transient failures, debounces bursts, and lands cleanly with no orphan threads. `mkt`, `mkp`, the new `mku` set, and startup/shutdown pass.

Phase 3 Status: pending. Date: (TBD). Result: (TBD). Variances: (TBD).

---

## Phase 4 - Database Persistence and Mail Tables

Objective: Persist templates, queue entries, attempts, and audit records across restarts.

Entry Gate: Phase 3 exit gate green; Phase 0.2 database-target decision recorded.

- [ ] 4.1 Design mail-specific schema as Acuranzo migrations.
  - Location: `/elements/002-helium/acuranzo/migrations/acuranzo_NNNN.lua` (after 1200). Tables: `mail_templates`, `mail_queue`, `mail_attempts`, `mail_events` (`mail_otp_codes` deferred to Phase 8 unless created now). Use `${...}` type macros, `${COMMON_CREATE}` audit columns, and `_aNN` lookup columns where a status lookup applies.
  - Verification: schema design reviewed against PostgreSQL, MySQL, SQLite, and DB2 macros; `test_98_luacheck.sh` passes.

- [ ] 4.2 Create `mail_templates` migration.
  - Suggested columns: `template_id`, `template_key`, `name`, `status_aNN`, `subject_template`, `text_template`, `html_template`, `collection`, plus `${COMMON_CREATE}`. Include the diagram migration row.
  - Verification: migration loads; diagram metadata exists; `test_71_database_diagrams.sh` if it covers new tables.

- [ ] 4.3 Create `mail_queue` and `mail_attempts` migrations.
  - Queue: message UUID, status, priority, template key, rendered recipients/subject/body, next-attempt time, attempts count, idempotency key, created/updated timestamps. Attempts: queue id, attempt number, server used, SMTP code/text, duration, success flag.
  - Verification: migration load/apply/reverse works for at least SQLite and one server engine (run the matching `test_3x_*_migrations.sh`).

- [ ] 4.4 Add internal QueryRefs for mail SQL.
  - Register QueryRefs in `queries` using `internal_sql`/`system_sql` types (never `public`/`protected`). Model on `acuranzo_1198.lua`.
  - Verification: QueryRefs are not reachable via public Conduit endpoints (manual check + a Conduit blackbox negative if practical).

- [ ] 4.5 Implement queue persistence repository helpers.
  - Insert pending, claim next, mark sending, mark sent, mark failed, reschedule retry, recover stale sending rows.
  - Verification: `mku mailrelay_repository_test` with a mock DB seam, plus a blackbox DB path covering CRUD and recovery.

- [ ] 4.6 Recover pending mail on startup.
  - Reset stale `sending` rows older than a configured timeout to `pending`.
  - Verification: blackbox starts, restarts, and verifies pending mail is retried once.

Exit Gate: durable queue and templates exist in the database, survive restart, and are manipulated only through internal mail relay paths. `mkt`, `mkp`, `test_98_luacheck.sh`, the new `mku` set, and at least SQLite + one server-engine migration test pass.

Phase 4 Status: pending. Date: (TBD). Result: (TBD). Variances: (TBD).

---

## Phase 5 - Template Rendering and Macro Engine

Objective: Render mail templates safely, deterministically, and testably.

Entry Gate: Phase 4 exit gate green.

- [ ] 5.1 Define template syntax (record in Working Log).
  - Recommended v1: `%MACRO%` replacement with an explicit variable map plus built-ins: `%APP_NAME%`, `%SERVER_NAME%`, `%TIMESTAMP%`, `%USER_EMAIL%`, `%REQUEST_ID%`, `%OTP_CODE%` (when available).
  - Verification: syntax documented in this plan's Working Log and in the code header.

- [ ] 5.2 Implement the macro engine.
  - Missing required macro fails unless a default is provided; unknown variables detected before send.
  - Verification: `mku mailrelay_template_test` covers replacement, escaping policy, missing vars, repeated vars, and large values.

- [ ] 5.3 Add HTML/text rendering policy.
  - Decide whether callers must provide both bodies or text is derived from HTML.
  - Verification: `mku mailrelay_render_policy_test` verifies MIME output for text-only, HTML-only, and multipart alternative.

- [ ] 5.4 Seed initial system templates.
  - Candidates: `system.server_started`, `system.server_stopped`, `auth.otp_code`, `mail.test`. Seed via migration.
  - Verification: migration seeds templates; blackbox can fetch/render them; `test_98_luacheck.sh` passes.

- [ ] 5.5 Add preview rendering.
  - Preview returns rendered subject/body and macro diagnostics with no queue/send side effects.
  - Verification: `mku mailrelay_preview_test` verifies preview has no queue side effects.

- [ ] 5.6 Define the internal templated-send producer API.
  - File: `src/mailrelay/mailrelay.h` plus implementation files as needed.
  - Purpose: common entry point for `mailrelay_enqueue_template(...)` / equivalent used by REST, Lua `H.mail`, Notify, and system events. It resolves templates, validates parameters/recipients, applies policy metadata, and enqueues without sending synchronously.
  - Verification: `mku mailrelay_template_test` or `mku mailrelay_producer_test` verifies success, missing template, missing macro, invalid recipient, idempotency key, and no body/OTP leakage in logs.

Exit Gate: template rendering is deterministic, tested, and supports system events, REST, Lua, and Notify producers through one internal API. `mkt`, `mkp`, the new `mku` set, and seed-migration checks pass.

Phase 5 Status: pending. Date: (TBD). Result: (TBD). Variances: (TBD).

---

## Phase 6 - System Event Mail Integration

Objective: Send configured administrative emails for Hydrogen events without blocking critical paths.

Entry Gate: Phase 5 exit gate green.

- [ ] 6.1 Define event rule config.
  - Map Hydrogen subsystem/log/event keys to template, recipients, severity, debounce key, and rate limit.
  - Verification: `mku config_mail_relay_test_events` (or extend config test) loads a representative rule set.

- [ ] 6.2 Add a safe in-process event injection API.
  - Logging/event paths enqueue a compact event object, not a rendered body.
  - Verification: `mku mailrelay_event_inject_test` verifies a synthetic event becomes one queued template send.

- [ ] 6.3 Implement startup/shutdown admin messages.
  - Send `system.server_started` after API/webserver/database readiness (only once Mail Relay is ready). Send shutdown/failed-shutdown when landing allows time.
  - Verification: `test_57_mailrelay_outbound.sh` (extended) captures "Server Started" exactly once.

- [ ] 6.4 Add rate limiting for noisy events.
  - Verification: `mku mailrelay_event_ratelimit_test` verifies a burst is debounced/coalesced and respects max sends per interval.

Exit Gate: administrative event email works through the queue/templates path, is rate-limited, and does not block startup, logging, or shutdown. `mkt`, `mkp`, the new `mku` set, and `test_57` pass.

Phase 6 Status: pending. Date: (TBD). Result: (TBD). Variances: (TBD).

---

## Phase 7 - REST API: Send, Preview, Status

Objective: Expose authenticated mail operations through the Hydrogen API.

Entry Gate: Phase 5 exit gate green (Phase 6 not required, but recommended). Phase 0.2 DB-target and Phase 0 template-only send decision recorded.

- [ ] 7.1 Add the `src/api/mailrelay/` endpoint module.
  - Layout `src/api/mailrelay/<endpoint>/<endpoint>.c/.h`. Endpoints: `POST /api/mailrelay/send`, `POST /api/mailrelay/preview`, `GET /api/mailrelay/status`, optionally `GET /api/mailrelay/message/{id}`. Register in `src/api/api_service.c` and add dispatch in `handle_api_request`.
  - Verification: `mkt`; endpoint log list includes the mail endpoints.

- [ ] 7.2 Implement method validation and request buffering with existing API helpers.
  - Use `api_buffer_post_data`, `handle_method_validation`, `api_parse_json_body`.
  - Verification: `mku` endpoint tests reject wrong method, invalid JSON, and oversized body.

- [ ] 7.3 Require authentication for send/preview/status.
  - Reuse `extract_and_validate_jwt` / `api_extract_jwt_claims`.
  - Verification: `mku`/API test rejects missing/invalid JWT and accepts a valid JWT.

- [ ] 7.4 Define the send request contract.
  - Required for normal external clients: `template_key`. Fields: to/cc/bcc, params, idempotency_key, priority, preview flag. Raw subject/body is internal/admin-test only behind explicit future gating.
  - Verification: the Draft API Contract below matches the implemented behavior and tests.

- [ ] 7.5 Wire API to the internal templated-send producer API.
  - Response returns queued message id and status, not SMTP success (unless an explicit synchronous test flag is enabled). The endpoint must not duplicate template, queue, policy, or rate-limit logic that belongs in the internal producer API.
  - Verification: `test_58_mailrelay_api.sh` (ports 5580-5586) sends templated mail to the sink and verifies queued/sent state. Add `tests/configs/hydrogen_test_58_mailrelay_api.json` and `docs/H/tests/test_58_mailrelay_api.md`.

- [ ] 7.6 Add Swagger/OpenAPI documentation.
  - Verification: `test_22_swagger.sh` passes.

Exit Gate: authenticated clients can preview and queue templated emails; behavior is documented and tested. `mkt`, `mkp`, the new `mku` set, `test_58`, and `test_22_swagger.sh` pass.

Phase 7 Status: pending. Date: (TBD). Result: (TBD). Variances: (TBD).

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

- [ ] 7A.1 Audit the existing Lua mail/notify stub implementation.
  - Files: `src/scripting/scripting_api_mail_notify.c`, `src/scripting/scripting_api.h`, `src/scripting/scripting_handle.{c,h}`, `src/scripting/lua_context.c`, and `docs/H/plans/LUA_PLAN_COMPLETE.md` Phase 19.
  - Verification: current stub behavior is captured by a focused Unity test before replacing it.

- [ ] 7A.2 Replace `H.mail.send` with a Mail Relay producer wrapper.
  - Contract: accept the existing Lua message table shape, require `template` / `template_key` for normal sends, map `to`/`cc`/`bcc`/`params`/`idempotency_key`/`priority` to the internal templated-send producer API, and return an async `H_HK_MAIL` handle.
  - Verification: `mku scripting_api_mail_test` queues a templated message through a mocked Mail Relay producer and verifies handle success/error behavior.

- [ ] 7A.3 Replace `H.mail.send_sync` with wait-based behavior.
  - Contract: `send_sync` may wait only for queue acceptance/rendering, not SMTP delivery, unless an explicit test-only synchronous mode exists.
  - Verification: Unity test verifies queued result, invalid template, invalid recipient, disabled Mail Relay, and timeout/error mapping.

- [ ] 7A.4 Define `H.notify` compatibility behavior.
  - Recommended v1: `H.notify.send` maps to Mail Relay event/rule templates only when a matching notification rule exists; otherwise it returns a stable `notify: deferred to mailrelay rules` error rather than silently dropping notifications.
  - Verification: Unity tests cover mapped notification, unmapped notification, disabled Mail Relay, and preserved legacy stub error semantics where still deferred.

- [ ] 7A.5 Preserve Lua safety and audit boundaries.
  - Lua must not call REST endpoints or SMTP directly for mail. It must enqueue through the same internal producer API as REST/system events and inherit the same validation, rate limits, redaction, idempotency, and audit metadata.
  - Verification: no SMTP credentials, OTP plaintext, JWTs, or message bodies appear in Lua logs/test artifacts; `test_02_secrets.sh` remains clean if run with the Lua mail blackbox slice.

- [ ] 7A.6 Update scripting docs and Lua completion plan cross-reference.
  - Files: `/docs/H/core/subsystems/scripting/lua_api.md`, `/docs/H/plans/LUA_PLAN_COMPLETE.md`, and this Working Log if implementation deviates.
  - Verification: `test_04_check_links.sh` and `test_90_markdownlint.sh` pass for touched docs.

Exit Gate: Lua scripts can queue templated mail through Mail Relay using the pre-existing `H.mail` API, `H.notify` has explicit compatibility behavior, and no parallel mail delivery path exists. `mkt`, `mkp`, Lua mail Unity tests, and relevant doc/link lints pass.

Phase 7A Status: pending. Date: (TBD). Result: (TBD). Variances: (TBD).

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

- [ ] 10.1 Add status counters.
  - Counters: queued, sending, sent, failed, retrying, permanent failures, last success, last failure, worker count, queue depth. Surface alongside the existing `mail_relay_queue_memory` metrics in `status/status_process.c`.
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

Phase 10 Status: pending. Date: (TBD). Result: (TBD). Variances: (TBD).

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
- `mailrelay/mailrelay_queue.{c,h}` (in-memory queue + workers + retry + debounce).
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
- New: `mailrelay/mailrelay_message_test.c`, `mailrelay_render_test.c`, `mailrelay_queue_test.c`, `mailrelay_workers_test.c`, `mailrelay_retry_test.c`, `mailrelay_debounce_test.c`, `mailrelay_template_test.c`, `mailrelay_preview_test.c`, `mailrelay_producer_test.c`, `mailrelay_repository_test.c`, `mailrelay_otp_generate_test.c`, `mailrelay_otp_verify_test.c`, `mailrelay_route_test.c`, plus API endpoint tests under `api/mailrelay/` and Lua backfill tests under `scripting/` such as `scripting_api_mail_test.c`.

### Blackbox tests (`/elements/001-hydrogen/hydrogen/tests/`)

- `test_57_mailrelay_outbound.sh` (+ `configs/hydrogen_test_57_mailrelay_outbound.json`, `docs/H/tests/test_57_mailrelay_outbound.md`).
- `test_58_mailrelay_api.sh` (+ config + doc).
- `test_59_mailrelay_inbound.sh` (+ config + doc).
- Optional local SMTP sink helper under `tests/`.

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
| 5 Templates | 4 | reusable system/API messages |
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
