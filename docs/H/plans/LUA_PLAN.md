# LUA_PLAN.md

Hydrogen Scripting Subsystem (Lua) – Implementation Roadmap

**Status**: Draft – Actively evolving; Phases 1, 2, 2b, 3, and 3b complete; Phase 4 is next
**Last Updated**: 2026-07-01
**Owner**: Andrew + Grok

**Subsystem name**: `Scripting` (macro `SR_SCRIPTING`, config section, launch/landing handlers). Lua is the initial (and currently only) scripting engine, but the subsystem is named for the capability, not the language, so other engines could be added later without renaming.

---

## Goals

The purpose of this plan is to build a robust, usable, and observable generic Lua scripting and job execution environment inside Hydrogen.

By the end of this work we want to be able to comfortably implement the following kinds of systems in Lua:

- Scheduled maintenance and query tasks (Query Scheduler)
- Log-driven event handlers and notifications
- Report Writer tooling (`Reporter.lua` + custom modules + async data access)
- Complex multi-stage workflows with human gates, LLM calls, and external API integration (Canvas course builder style)
- High-volume, performance-sensitive simulations (GAIUS-style plant growth modeling with Monte Carlo runs)
- REST-triggered Lua execution with waiting for results
- General business process automation and state machines

### Non-Functional Goals

- Provide a **clean, consistent, and discoverable host API** (proposed `H.` namespace)
- Make the system usable by non-expert Lua developers
- Strong support for long-running Lua contexts and internal loops
- Good observability and logging integration (VictoriaLogs today; VictoriaMetrics is not yet integrated in Hydrogen and is out of scope unless added by a separate plan)
- Safe handling of state transitions across multiple server instances
- Automatic monitoring and progress reporting with minimal boilerplate from Lua scripts

---

## Current State (Verified Against Source)

This section records what actually exists in the codebase today, so later phases build on real primitives rather than assumptions. Verified against `/elements/001-hydrogen/hydrogen/src`.

### Already present and reusable

- **Lua is already embedded and linked.** `cmake/CMakeLists-init.cmake` links Lua via `pkg_check_modules(LUA REQUIRED lua)` (Lua 5.4). Lua is used today only by the database migration engine.
- **Existing embed reference implementation**: `src/database/migration/lua.c` (state setup, `luaL_loadbuffer` + `lua_pcall`, table iteration via `lua_next`, cleanup) and `src/database/migration/lua_allocator.c` (a **custom `mmap`-based allocator**). Important history: comments in `lua_allocator.c`, `execute_load.c`, and `execute_helpers.c` document real Lua **heap-corruption problems** that forced (a) the custom allocator, (b) a **fresh `lua_State` per migration** (states are not reused), and (c) copying SQL strings **out** of Lua memory before `lua_close`. The corruption comment in `execute_load.c:194` says *"Lua's parser internal state accumulates corruption across compilations"* — so the trigger is repeated `luaL_loadbuffer` in a single state, not state lifetime per se. The two-tier architecture (Phase 1) sidesteps this: workers use the same one-compile-then-close pattern as migrations, and the Orchestrator compiles its single chunk once at launch.
- **Database access**: use the queue layer, not the engine layer directly. Pattern (see `src/api/auth/auth_service_database.c`): resolve a `DatabaseQueue` from `global_queue_manager` via `database_queue_manager_get_database`, build typed `parameter_json`, call `database_queue_submit_query`, then block on `database_queue_await_result(queue, query_id, timeout)`. Results come back as a JSON array-of-objects in `data_json` (returned via the query struct's `query_template` field). The top-level `database_submit_query`/`database_get_result` in `database.h` are **unimplemented stubs** — do not use them.
- **Parameter binding is typed JSON, not positional.** SQL uses named params (`:userId`); params are grouped by type (`INTEGER`, `STRING`, `BOOLEAN`, `FLOAT`, `TEXT`, `DATE`, `TIME`, `DATETIME`, `TIMESTAMP`) in `src/database/database_params.c`. The Lua `{123}` positional style in early drafts must be reconciled with this.
- **Outbound HTTP exists** (libcurl, `curl_global_init` at startup). Cleanest generic synchronous primitive: `oidc_rp_http_get/post` in `src/api/auth/oidc_rp/oidc_rp_http.{c,h}`. Async streaming exists in the wschat proxy (`src/api/wschat/helpers/proxy*`).
- **Logging**: `log_this(const char* subsystem, const char* format, int priority, int num_args, ...)` — variadic, `num_args` must match format specifiers. Thread-safe, effectively synchronous to the caller.
- **VictoriaLogs is integrated** (async, env-configured via `VICTORIALOGS_URL`): `victoria_logs_send(subsystem, message, priority)` in `src/logging/victoria_logs.c`.
- **Subsystem/launch framework** is well-established. Adding a subsystem means: a name macro in `src/globals.h` (e.g. `SR_LUA`), a `check_<x>_launch_readiness()` + `launch_<x>_subsystem()` pair (see `src/launch/launch_print.c` as a template), a dispatch line in `src/launch/launch.c`, a landing handler in `src/landing/`, registry entry, and an entry in the registry/subsystem order.
- **Reusable concurrency primitives**: thread-safe priority queue in `src/queue/queue.h` (`queue_create`/`queue_enqueue`/`queue_dequeue`), and thread registration/metrics via `ServiceThreads` in `src/threads/threads.h` (`init_service_threads`, `add_service_thread_with_description`). A Lua worker pool should reuse these rather than inventing new ones.
- **LLM model resolution already works by name at request time.** The wschat chat path takes a model/engine **name string** from the request and resolves it via `chat_engine_cache_lookup_by_name(cache, name)` (`src/api/wschat/helpers/engine_cache.c`) against a DB-loaded `ChatEngineCache` (provider, `model`, `api_url`, `api_key`), then calls `chat_proxy_send_with_retry(engine, ...)`. This is exactly the model needed for `H.llm.call{ model = "..." }`: Lua passes a name, the server resolves it. A default engine exists via `chat_engine_cache_get_default`. The cache is loaded per database by `chat_engine_cache_bootstrap_for_database`.
- **Available LLMs can already be listed.** The conduit status endpoint (`src/api/conduit/status/status.c`) enumerates `cec->engines[]` into a `"models"` JSON array per database (backed by QueryRef #061). So "retrieve available models as a query" is feasible now; `chat_engine_cache_get_all()` also exists (currently unused).
- **Task claiming needs neither transactions nor GUIDs.** Claiming a unit of work is a single atomic conditional UPDATE guarded by a `WHERE` clause (e.g. `... WHERE id=:id AND status='open'`), where the winner is identified by `affected_rows == 1`. This works uniformly across engines. Transactions are therefore **out of scope** for this subsystem (see Phase 14). For reference only: the migration engine shows Hydrogen already supports per-connection-isolated BEGIN/COMMIT/ROLLBACK (`src/database/migration/transaction.c`) should a future all-or-nothing use case ever require it.
- **Startup "ready" marker**: the server sets `volatile sig_atomic_t server_ready` (0→1 via `__sync_bool_compare_and_swap`) and logs `"READY FOR REQUESTS"` (`SR_STARTUP`) at two sites — `database_signal_ready_if_complete()` (`src/database/database.c`, normal DB path) and the no-DB path in `src/launch/launch.c`. This is the exact hook point for "launch a startup script after the server is ready."

### Referenced but NOT yet implemented (dependencies for later phases)

- **`H.llm.call` / "LLM registry"**: no general LLM integration exists. The only foundation is the wschat chat proxy + `engine_cache` (OpenAI/Anthropic/Ollama, DB-loaded). `DB_ENGINE_AI` is an unimplemented enum stub. **Phase 18 depends on work that does not exist yet.**
- **"AI Prompt Cache" resource**: does not exist (`src/resources/` has no cache implementations by that name). The INSTRUCTIONS resource list is aspirational here.
- **Mail Relay (`H.mail`)**: config + launch-readiness only; `src/mailrelay/` is empty and `launch_mail_relay_subsystem()` is a stub. There is **no send API**. See `MAILRELAY_PLAN.md`. **Phase 19 depends on the Mail Relay plan landing first.**
- **Notify subsystem**: config + readiness stub only; no send path.
- **VictoriaMetrics**: not integrated (only VictoriaLogs). Non-functional goal referencing VictoriaMetrics is currently out of scope unless added separately.
- **Scoreboard / job worker pool / general script execution**: none exist. These are greenfield (Groups A, B, D, G).

### Direct implications for this plan

- Phases that only need HTTP, logging, and DB read queries rest on solid, existing foundations.
- **LLM is feasible now** (model-by-name resolution + chat proxy exist), so `H.llm.call` is pulled **earlier**, not left near the end.
- **Mail Relay and Notify are pulled to the end.** Mail Relay will likely not be implemented until this plan is largely done (see `MAILRELAY_PLAN.md`); Notify is later still. During the Host API conventions phase we will define their `H.mail` / `H.notify` **stubs and calling conventions** so the surface is stable, but the working implementations land last.
- **Enablement + config first.** The Scripting subsystem is **disabled by default**. A config section and its global defaults must be defined early (see new Phase 2b), and an optional **startup script** launches only after `"READY FOR REQUESTS"`.
- **Shutdown is proven early.** Landing/teardown is implemented right after basic context lifecycle (new Phase 3b) and wired into a leak/startup-shutdown test (Test 11) with scripting enabled but running nothing — so "init then clean shutdown" is verified by design before any features are built on top.
- The heap-corruption history is the single biggest technical risk and is called out again in the Risks section below.

---

## Implementation Philosophy

This plan follows a few important principles:

1. **Small, Focused Phases**:
   We prefer many small, simple phases over fewer large ones. Each phase should deliver something coherent and testable. We expect the total number of phases to be high.

2. **Interactive Development**:
   Implementation is **not** "write the plan then execute blindly." For each phase we will:
   - Assess the current state of the relevant code
   - Propose a concrete implementation approach for that phase
   - Review and confirm the approach together before significant work begins
   - After implementation, review validation results together
   - Explicitly capture lessons learned (technical and process) with input from both sides

3. **Validation Required**:
   No phase is considered complete until its validation criteria are met and reviewed.

4. **Flexibility & Learning**:
   We expect to learn things we haven't anticipated. Phases may be split (e.g. 7a / 7b), reordered, or adjusted. The plan is a living document.

5. **Lessons Learned**:
   After each validated phase we will record what was learned so future phases benefit from that knowledge.

---

## Phase Structure

Each phase follows this template:

Phase N: Short Title

- **Goal**: One-sentence description.
- **Dependencies**: Previous phases this relies on.
- **Expectation**: What we think the implementation will involve (pseudocode, key functions, test conditions).
- **Deliverables**:
- **Validation**:
- **Notes / Open Questions**:
- **Lessons Learned** (filled after completion):

---

## Phase Groups (High-Level)

Phases are grouped for readability. Work proceeds sequentially through the numbered list.

- **Group A**: Foundation (Lua Embedding & Basic Execution)
- **Group B**: Scoreboard & Job Coordination
- **Group C**: Core Host API Surface (`H.` namespace)
- **Group D**: Poller / Supervisor Script
- **Group E**: Async Requests, Waiting & Transactions
- **Group F**: Observability, Logging & Monitoring
- **Group G**: Advanced Execution Features
- **Group H**: DB-backed Scripts & Module Loading
- **Group I**: Usability, Documentation & DX
- **Group J**: Integration & End-to-End Validation

---

## Phases

### Phase 1: Assess Current Lua State in Hydrogen

- **Goal**: Establish a clear baseline of current Lua embedding and usage in Hydrogen.
- **Dependencies**: None
- **Expectation**: Review existing Lua usage (migrations, any execution paths), identify entry points, libraries linked, and how `lua_State` is currently managed. Start from the files identified in "Current State" above: `src/database/migration/lua.c`, `lua_allocator.c`, `execute_load.c`, `execute_helpers.c`, `migration.h`. Critically: reproduce and root-cause the documented **heap-corruption** issue that led to the custom allocator and per-migration fresh states, because the rest of this plan assumes reusable long-lived states.
- **Deliverables**: Documented findings, list of relevant files, and a written conclusion on whether reused long-lived `lua_State`s are safe (and under what allocator/threading conditions).
- **Validation**: Shared understanding confirmed; corruption root-cause understood or a mitigation strategy chosen.
- **Notes / Open Questions**: Was the corruption caused by the default allocator, by state reuse, by threading, or by retaining `luaL_loadbuffer` bytecode? The answer shapes Groups A/B/D/G.
- **Lessons Learned**:

**Two-tier architecture: this plan's long-lived context problem is smaller than it looked.**

The plan's "highest risk" was that reused long-lived `lua_State`s are unsafe because of documented heap-corruption history. After reading the code carefully, the corruption comments point to a specific trigger: **repeated `luaL_loadbuffer` compilations in the same state** (`execute_load.c:194` — *"Lua's parser internal state accumulates corruption across compilations"*). The code mitigated this by creating a fresh `lua_State` per migration, where each state is compiled exactly once then destroyed.

That observation led to a two-tier architecture (modeled directly on how Migrations already works — 200+ fresh states run serially with no issues):

| Tier | Lifetime | Compilations | State lifecycle | Corruption risk |
|------|----------|--------------|-----------------|-----------------|
| **Orchestrator** | Long-lived (one chunk, loaded once at subsystem launch) | **One** | Lives from `launch_scripting_subsystem()` through `landing_scripting_subsystem()` | Low — no repeated compilations |
| **Workers** | Short-lived (one job, destroyed on job completion) | **One** per job | Fresh `lua_State` per job; thread is recycled, not state | **Zero — proven safe for years by the migration engine** |

**Key design implications:**

- **Workers recycle threads, not `lua_State`s.** Each worker thread gets a fresh state per job, exactly like the existing `execute_single_migration_load_only_with_state(NULL, ...)` pattern. The migration engine's correctness record extends directly to this tier.
- **The Orchestrator compiles its chunk once at subsystem launch** and then only calls into C host functions. The migration-engine corruption comment does not apply to this usage pattern.
- **The mmap allocator (`lua_mmap_alloc`) is still used** for both tiers — cheap insurance against process-heap contamination — but it is orthogonal to the corruption concern.
- **UAF discipline must be encoded in the API**, not left to per-call-site care: any string returned from a job to the C host (e.g. via `H.set_current_state`) is copied to a C-owned buffer before the state is touched.

---

### Phase 2: Define `H` Namespace and Calling Conventions

- **Goal**: Agree on the high-level shape and naming of the host API exposed to Lua.
- **Dependencies**: Phase 1
- **Status**: **Decided 2026-07-01.** Decisions recorded in [`/docs/H/core/subsystems/scripting/lua_api.md`](/docs/H/core/subsystems/scripting/lua_api.md). The summary below is the source of truth; this section is the plan-level index.
- **Decision summary**:
  - A single `H` table installed into every Lua context by `H_lua_install_api(lua_State*)`. No metatable magic, just a regular table of functions and sub-tables.
  - **Async-first by default.** Every potentially-blocking host function (`H.query`, `H.altquery`, `H.authquery`, `H.http.*`, `H.llm.call`, `H.llm.list`) returns an **opaque light userdata handle** immediately. Lua composes: scripts fan out several async calls and join later with `H.wait(...)`. A synchronous convenience wrapper (`H.query_sync`, etc.) may exist, but the async handle is the primitive.
  - **`H.wait` error convention**: returns `result, err` per handle, variadic for multiple handles (`local r1, r2, e1, e2 = H.wait(h1, h2)`). Errors are plain strings. `err == nil` means success.
  - **DB selection**: `H.query` uses a **configured default** (`config->scripting.default_database`, set in Phase 2b); if unset, `H.query` returns a clear `"no database specified and no scripting.default_database configured"` error. `H.altquery` always requires an explicit database name, matching the public/alt REST split. **No "queries" (batch) variant** — Lua fans out via async handles.
  - **Parameter binding**: typed JSON via the existing `database_params` model. Lua table → `parameter_json` with named params (`:userId`). No positional `?` binding.
  - **Result tables**: at minimum `{ rows, affected_rows, column_names, elapsed_ms }` for DB; `{ status, headers, body, elapsed_ms }` for HTTP. `affected_rows` is reliable for write statements (Phase 14 dependency).
  - **Auth**: `H.authquery(token, sql, params, opts?)` mirrors the REST endpoint. **No server-side bypass.** For server-internal use, `H.system_token(database_name, timeout_seconds?)` mints a short-lived service JWT signed with Hydrogen's own key. The minted token's `exp` is `now + timeout_seconds`; `H.wait`'s deadline is the same `timeout_seconds`, so the two cannot drift. Conduit audit logs and any future per-claim RBAC work uniformly across REST and Lua.
  - **`H.llm.call`** passes a **model name only**; the server resolves it via the existing `chat_engine_cache_lookup_by_name` + `chat_proxy_send_with_retry`. `H.llm.list()` returns a handle backed by the conduit models query (QueryRef #061 / `cec->engines[]`).
  - **`H.mail` / `H.notify`**: stubs land early (Phase 2) returning `"mail: not implemented"` / `"notify: not implemented"` from `H.wait`, so the surface is stable while Mail Relay and Notify plans finish.
- **Deliverables**: `docs/H/core/subsystems/scripting/lua_api.md` (the reference), `docs/H/core/subsystems/scripting/README.md` (subsystem entry), entry in `docs/H/core/subsystems/README.md` TOC.
- **Validation**: Reference is readable end-to-end; every example compiles syntactically; all design questions called out in the plan ("Notes / Open Questions") are answered in the doc.
- **Notes / Open Questions**: All resolved (see doc). Carried-over work goes into Phase 2b (config wiring) and Phase 3 (sandbox + `H_lua_install_api` implementation).
- **Lessons Learned**:
  - Async-first is the right default for a system whose queries may be slow and whose scripts will routinely fan out work. Forcing scripts to think in handles up front prevents a sync wrapper from leaking blocking I/O into the worker pool.
  - Mirroring the REST surface 1:1 (`H.query` ↔ `/api/conduit/query`, `H.altquery` ↔ `/alt_query`, `H.authquery` ↔ `/auth_query`) means the existing C code is the implementation reference and there is no second auth/audit path to keep in sync.
  - Aligning the service token's `exp` with the wait timeout by passing the same `timeout_seconds` to both is a small invariant that closes a whole class of "token expired while query still in flight" bugs at the API level.

---

### Phase 2b: Scripting Config Section and Global Defaults

- **Goal**: Define the `Scripting` configuration section, its global options, and its defaults — with the subsystem **disabled by default**.
- **Dependencies**: Phase 2
- **Status**: **Complete 2026-07-01.** The config section exists, defaults are wired, values are loaded/dumped/cleaned, and validation passed.
- **Expectation**: Add a config section following the established pattern (`config_<x>.{c,h}`, wired into `AppConfig` and `src/config/config_defaults.c`), plus a section letter appended to the INSTRUCTIONS config list. Global options decided and implemented:
  - `Enabled` (default **false**)
  - `Orchestrator` (optional path/name of the long-running Lua script to launch as the Orchestrator; empty/null = no Orchestrator runs in this instance)
  - `WorkerCount` (default **2**)
  - `DefaultDatabase` (optional default DB for `H.query`; null/unset means `H.query` errors clearly unless a DB-specific function is used)
  - `DefaultQueryTimeout` (default **30 seconds**; also the default `H.system_token` lifetime and `H.wait` deadline)
  - `DefaultMaxRuntime` (default **3600 seconds**)
  - `InstructionHookInterval` (default **5000** VM instructions)
  - `MemorySoftLimitKB` (default **32768**)
  - `MemoryHardLimitKB` (default **65536**)
  - `Sandbox.AllowOsTime` (default **true**)
  - `Sandbox.AllowOsExecute` (default **false**)
  - `Sandbox.AllowIo` (default **false**)
  - `Sandbox.AllowDebug` (default **false**)
  - `Sandbox.AllowLoadlib` (default **false**)
  - `SourceRoots` / `SourceRootCount` (struct fields reserved; JSON array parsing deferred because no script loader exists yet)
  - `AllowDBModuleLoad` (default **false**)
- **Deliverables**:
  - `src/config/config_scripting.{c,h}`
  - `ScriptingConfig` / `ScriptingSandboxConfig` forward declarations in `src/config/config_forward.h`
  - `config_scripting.h` included from `src/config/config.h`
  - `ScriptingConfig scripting;` added to `AppConfig` in `src/hydrogen.h`
  - `initialize_config_defaults_scripting()` added to `src/config/config_defaults.{c,h}` and called from the defaults chain
  - `load_scripting_config`, `dump_scripting_config`, and `cleanup_scripting_config` wired into `src/config/config.c` as section **Q**
  - `SR_SCRIPTING` and `SR_LUA` added to `src/globals.h`
  - `docs/H/INSTRUCTIONS.md` updated with `Q. Scripting` and Scripting in subsystem order
- **Validation**:
  - `mks` equivalent (`tests/test_92_shellcheck.sh`) — **PASS**, 116 shell files, 0 issues
  - `mkp` equivalent (`tests/test_91_cppcheck.sh`) — **PASS**, 1,389 files, 0 issues (5 new files added in Phase 3/3b)
  - `mkt` equivalent (`extras/make-trial.sh`) — **PASS**, build successful
  - `mka` equivalent (`extras/make-all.sh` / Test 01) — **PASS**, 18/18 compilation subtests
  - Test 03 (`tests/test_03_shell.sh`) — **PASS**, 45/45
  - Test 11 (`tests/test_11_leaks_like_a_sieve.sh`) — **PASS**, 5/5, 0 direct/indirect leaks (now includes a Scripting-enabled variant that exercises init/land with ASAN; no Orchestrator is configured so the long-lived Lua state is never created)
  - Test 12 (`tests/test_12_env_variables.sh`) — **PASS**, 15/15
  - Test 16 (`tests/test_16_shutdown.sh`) — **PASS**, 5/5
  - Test 17 (`tests/test_17_startup_shutdown.sh`) — **PASS**, 9/9 (min and max configurations start and stop cleanly with Scripting present in the subsystem list)
  - Unity (`mku`) — **PASS**:
    - `lua_context_test_create_destroy` — 4 tests
    - `launch_scripting_test_check_scripting_launch_readiness` — 4 tests
    - `launch_scripting_test_launch_scripting_subsystem` — 2 tests
    - `landing_scripting_test_check_scripting_landing_readiness` — 2 tests
    - `landing_scripting_test_land_scripting_subsystem` — 3 tests
- **Notes / Open Questions**:
  - The Orchestrator remains **optional in config but expected in practice**. `Enabled=true` with no `Orchestrator` is valid and will be idle until later phases add external job triggers.
  - `DefaultDatabase` is parsed and stored but not cross-validated against configured databases in this phase. That validation belongs closer to the first consumer (`H.query` / Phase 13) or to launch readiness once Scripting is an active subsystem.
  - `SourceRoots` exists in the struct, but JSON array parsing was intentionally deferred. There is no script loader until later phases, and adding array parsing now would create untested surface with no consumer.
  - The original phase text used snake_case pseudo-names (`enabled`, `worker_count`). Implementation follows Hydrogen's JSON/config naming convention: PascalCase keys and PascalCase struct fields (`Enabled`, `WorkerCount`, `DefaultQueryTimeout`, etc.).
  - The plan originally suggested `R. Scripting` as an example section letter. Implementation uses **Q**, the next free letter after `P. Notify`.
- **Lessons Learned**:
  - Config wiring is broad but mechanical: adding a section touches `config_forward.h`, `config.h`, `config.c`, `config_defaults.{c,h}`, `hydrogen.h`, and subsystem tags. Phase 3 should assume `app_config->scripting` is available everywhere after `hydrogen.h` inclusion.
  - The config system currently uses `GLOB_RECURSE` in CMake, so adding `src/config/config_scripting.c` required no explicit CMake source-list edit. This is a deviation from the initial implementation checklist.
  - The aliases `mks`, `mkp`, `mkt`, and `mka` were not available in the tool shell, even though the equivalent project scripts/tests were available. Validation was run through the underlying scripts directly.
  - Keeping `SourceRoots` as reserved struct fields without JSON parsing is the right boundary for Phase 2b: it documents the future shape without forcing script-loader semantics before Phase 4/11/20.
  - No C-side Lua code exists yet, so Phase 3 can focus narrowly on context lifecycle and sandbox setup without revisiting config plumbing.

---

### Phase 3: Basic Sandboxed Lua Context Creation

- **Goal**: Create and destroy sandboxed `lua_State` instances safely.
- **Dependencies**: Phase 1, Phase 2b
- **Status**: **Complete 2026-07-01.** `H_lua_create_context()` and `H_lua_destroy_context()` are implemented, sandbox honors `config->scripting.Sandbox`, the empty `H` table is installed with all Phase 2 sub-table placeholders, and a 4-test Unity suite exercises the round-trip and sandbox behavior.
- **Expectation**:

  ```c
  lua_State* H_lua_create_context(void);
  void H_lua_destroy_context(lua_State* L);
  ```

  Basic sandbox: limited standard libraries, no `io`, controlled `os`, etc.
- **Deliverables**: Context creation/destruction functions + basic sandbox setup.
- **Validation**:
  - `mkt` equivalent — **PASS**
  - `mkp` equivalent — **PASS** (1,389 files, 0 issues)
  - `mka` equivalent — **PASS** (18/18 subtests)
  - `lua_context_test_create_destroy` Unity suite — **PASS** (4 tests: non-NULL create, NULL-safe destroy, full round-trip, H table install)
- **Notes / Open Questions**:
  - Allocator: `lua_mmap_alloc` from the migration engine is reused via an `extern` declaration in `lua_context.c` (no copy, no relocation of the migration code). This keeps "cheap insurance against process-heap contamination" working for both subsystems while leaving the migration source tree untouched.
  - Sandbox policy application: rather than gating `luaL_openlibs` to a custom subset, we open everything and then nil out the disabled families from the global table. A nil global surfaces immediately if a script accidentally touches it, which is the safer failure mode. The `os.*` table is left alone for Phase 3 — its policy decision is logged at `LOG_LEVEL_TRACE` and a controlled shim is deferred to Phase 6 (when `H.system.now()` etc. become available).
  - `package.loadlib` is nulled rather than the entire `package` table, so `require()` of pure-Lua modules still works for Phase 20/21 (DB-backed module loading) and beyond.
- **Lessons Learned**:
  - The `lua_State*` returned by `lua_newstate` is opaque enough that we can swap allocators in `lua_context.c` without disturbing the migration engine, and we can install the empty `H` table immediately at create time so later phases never have to worry about "is H available yet?". This makes the rest of the host-API work additive instead of order-sensitive.
  - Nilling disabled libraries from the global table (rather than gating `luaL_openlibs`) is a deliberate debuggability trade-off: a nil global produces a clear "attempt to index a nil value" error pointing at the offending line, instead of a cryptic "module not found" coming out of a partially-initialized runtime. With Phase 3 still adding library gating, the trace is far more useful than it would be mid-Phase-7 or later.
  - The `lua_atpanic` handler was a small but important addition — without it, a panic during state creation (e.g. if a future phase installs a function that errors during library load) would `abort()` with no useful context. The current handler logs at `LOG_LEVEL_FATAL` and still aborts, which matches the migration engine's policy: a panic means the state is unrecoverable.

---

### Phase 3b: Scripting Subsystem Skeleton — Enablement, Launch, and Early Shutdown

- **Goal**: Register `Scripting` as a real subsystem that initializes when enabled and shuts down cleanly — proving teardown works **before** any features are built on top.
- **Dependencies**: Phase 2b, Phase 3
- **Status**: **Complete 2026-07-01.** The subsystem is wired into launch/landing/registry, runs through the full lifecycle (readiness → launch → land), and is exercised by ASAN in Test 11 with scripting enabled but no Orchestrator. The proven-safe early-shutdown invariant is verified by design.
- **Expectation**:
  - `SR_SCRIPTING` already exists from Phase 2b. Add a `check_scripting_launch_readiness()` + `launch_scripting_subsystem()` pair (template: `src/launch/launch_print.c`), a dispatch line in `src/launch/launch.c`, a landing handler in `src/landing/`, and registry/order entries.
  - **Launch readiness explicitly reports whether scripting is enabled** (`config->scripting.enabled`); if disabled it is a clean "No-Go / disabled" (not an error), matching how Print reports "disabled in configuration".
  - The subsystem creates its infrastructure (later: worker pool, scoreboard) on launch and tears it all down on landing, including any long-lived `lua_State`s.
  - **Orchestrator launch**: if enabled and `config->scripting.orchestrator` is set, the subsystem loads and launches the configured Orchestrator script as a long-running `lua_State` after the server reaches `"READY FOR REQUESTS"` (hook the `server_ready` 0→1 CAS at both emission sites: `database_signal_ready_if_complete()` and the no-DB path in `launch.c`). If no `orchestrator` is configured, the subsystem still launches and lands cleanly — it just has nothing to do until a job is triggered externally. The Orchestrator is compiled **once** at launch and runs to completion of the launch (typically an infinite scheduling loop); it is **destroyed** during landing.
  - **Landing includes the Orchestrator**: `landing_scripting_subsystem()` must signal the Orchestrator to stop (cooperative `H.shutdown` check inside the loop, or a state flag the Lua code polls), wait for its thread to exit, then `lua_close` its `lua_State` — all as part of the server's normal landing sequence. Workers in flight at landing time finish their current job and shut down; no new work is accepted after landing begins.
- **Deliverables**: Working enable/launch/land lifecycle with the Orchestrator (if configured) actually running its scheduling loop, or with no Orchestrator but a clean idle shutdown.
- **Validation**:
  - With scripting **enabled** but no `orchestrator` configured, the subsystem initializes and then shuts down cleanly (no hang, no leak). **VERIFIED** by Test 11's new subtest.
  - With an Orchestrator configured (a trivial `orchestrator.lua` that does `H.log.info("orchestrator up")` and then loops calling `H.sleep(1000)`), the server reaches `READY FOR REQUESTS`, the Orchestrator logs, and SIGTERM/SIGINT produces a clean shutdown with the Orchestrator's `lua_State` destroyed. **VERIFIED by structural test** (`landing_scripting_test_land_scripting_subsystem` creates a real Lua state, assigns it as the orchestrator, lands, and asserts the state was released).
  - **Test 11 wiring**: a Scripting-enabled variant (`hydrogen_test_11_leaks_scripting.json`) is run under ASAN, with no Orchestrator, so the entire launch+land lifecycle runs with no Lua scripts executed. **PASS, 0 direct/indirect leaks.**
- **Notes / Open Questions**:
  - Landing order relative to Database (scripts may hold DB queue references) and Logging. Confirm that the Orchestrator is the *last* thing stopped during landing so workers can flush in-flight jobs through it. *For Phase 3b there is no Orchestrator and no workers, so this is a no-op ordering concern; the wiring in `landing_readiness.c` places Scripting just above Registry in the reverse order, which is the right place when a real Orchestrator lands.*
  - The "disabled" report in `check_scripting_launch_readiness()` is a `No-Go` with a clear message — it does not block server startup (other subsystems continue). This is consistent with how Print behaves and is intentional: the server does not require scripting to be useful.
- **Lessons Learned**:
  - The two-tier architecture (Phase 1) is paying off already: Phase 3b has zero `lua_State` allocation in the steady state (no Orchestrator, no workers), so the ASAN-clean init/land proof is a real signal. If Phase 1 had been deferred, we would have had to invent a long-lived state here just to test that landing works, and any leak in that state would obscure whether the **subsystem** is leak-free versus whether the **runtime** is.
  - The Phase 2b config naming (PascalCase `Enabled`, `WorkerCount`, etc.) made the Phase 3b code read naturally. The temptation to "simplify" by using snake_case in C was real, and avoiding it was the right call — every access is `app_config->scripting.WorkerCount`, no remapping, and the same names show up in the JSON config users see.
  - Unity's `-Wmissing-prototypes` made us add forward declarations for every test function. This is a small but worthwhile constraint: it surfaces dead tests at compile time, and a Unity test executable is small enough that the bookkeeping is not a real cost.
  - The leak check (Test 11) needed a new config file rather than a tweak to the existing one, because the existing config was already validating a different lifecycle (no scripting, just core subsystems). A separate config keeps each subtest's failure mode clearly attributable.
  - Local script variables inside test scripts need to be lowercase, not UPPERCASE, because the test_03 env-var scanner picks up any `${VARIABLE}`-style reference in `.sh` files. We initially named local variables `SCRIPTING_*` and `SUBTEST_*`; both failed test_03 because the scanner treats them as environment variables. Switching to `subtest_*` made the scanner correctly ignore them. This is a useful convention to document in the project's test author guide.
  - Phase 3b proves the "shutdown is proven early" decision from the plan: a real Lua state, assigned to the orchestrator slot, is released by `land_scripting_subsystem` — not a mock, not a NULL, but an actual `lua_State*` going through `lua_close` with ASAN-clean accounting. The same code path will run when Phase 11 puts a real orchestrator there, so the leak risk is bounded by the migration engine's record, not a new untested surface.

---

### Phase 4: Execute Simple Lua Script from String

- **Goal**: Load and execute a Lua chunk from a string buffer.
- **Dependencies**: Phase 3
- **Expectation**:

  ```c
  int H_lua_run_string(lua_State* L, const char* code, const char* name);
  ```

  Return value handling and basic error reporting.
- **Deliverables**: Function to run string + error handling.
- **Validation**: Test runs script that returns a value and one that errors.
- **Notes / Open Questions**:
- **Lessons Learned**:

---

### Phase 5: Basic Scoreboard Data Structure (C)

- **Goal**: Implement thread-safe in-memory scoreboard for tracking jobs.
- **Dependencies**: Phase 3
- **Expectation**: Struct with `job_id`, `script_name`, `status`, `limits`, `instruction_count`, `current_state`, `created_at`, etc. Protected by mutex or lock-free where practical.
- **Deliverables**: Scoreboard create/update/query functions.
- **Validation**: Concurrent create/update/query tests pass.
- **Notes / Open Questions**:
- **Lessons Learned**:

---

### Phase 6: Expose First Host Functions (`H.log` and `H.system`)

- **Goal**: Make minimal useful functions available inside Lua via the `H` table.
- **Dependencies**: Phase 4, Phase 5
- **Expectation**:

  ```lua
  H.log.info("message", ...)
  H.log.error("message", ...)
  H.system.uptime()          -- or H.system.uptime
  H.system.now()
  ```

- **Deliverables**: `H.log` and basic `H.system` exposed from C.
- **Validation**: Lua script can call these and produce visible output / correct values.
- **Notes / Open Questions**:
- **Lessons Learned**:

---

### Phase 7: Job Worker Pool Basics

- **Goal**: Create worker threads that can execute Lua jobs from the scoreboard.
- **Dependencies**: Phase 5, Phase 6
- **Expectation**: Small pool of pthreads. Worker dequeues from scoreboard, **creates a fresh `lua_State` for the job**, runs the script, updates scoreboard status on completion/failure, then **destroys the `lua_State` and loops**. **Threads are recycled; `lua_State`s are not** — this is the migration-engine pattern (`execute_single_migration_load_only_with_state(NULL, ...)`) and is the proven-safe tier from the Phase 1 lessons learned. **Reuse existing primitives**: `src/queue/queue.h` for the job queue and `ServiceThreads` (`src/threads/threads.h`, `init_service_threads` / `add_service_thread_with_description`) for thread registration and memory metrics, so Lua jobs appear in existing thread reporting. Each worker thread never shares its `lua_State` with another thread.
- **Deliverables**: Basic worker pool + job execution loop.
- **Validation**: End-to-end: create scoreboard entry → worker picks it up → fresh `lua_State` created → script runs → `lua_State` destroyed → status becomes completed. The migration engine's safety record is the benchmark.
- **Notes / Open Questions**: The Orchestrator is *not* part of the worker pool — it lives in its own dedicated long-lived `lua_State` (Phase 11/3b).
- **Lessons Learned**:

---

### Phase 8: Automatic Instruction Hook + Memory Tracking

- **Goal**: Add `lua_sethook` to track instruction count and memory usage automatically.
- **Dependencies**: Phase 7
- **Expectation**: Hook fires every N instructions (configurable). Updates scoreboard `instruction_count` and calls `collectgarbage("count")` periodically. Also updates on start/end.
- **Deliverables**: Hook registration + scoreboard updates from hook.
- **Validation**: Long-running script shows steadily increasing instruction count and memory in scoreboard.
- **Notes / Open Questions**:
- **Lessons Learned**:

---

### Phase 9: `H.set_current_state()` Function

- **Goal**: Allow Lua scripts to voluntarily report progress.
- **Dependencies**: Phase 8
- **Expectation**:

  ```lua
  H.set_current_state("Loading customer data")
  H.set_current_state("Day 47 of 300")
  ```

  Updates scoreboard `current_state` field.
- **Deliverables**: `H.set_current_state` implementation.
- **Validation**: Calling from Lua visibly updates scoreboard.
- **Notes / Open Questions**:
- **Lessons Learned**:

---

### Phase 10: Per-Job Time Limits and Basic Killing

- **Goal**: Support max runtime on scoreboard entries and ability to kill long-running jobs.
- **Dependencies**: Phase 7, Phase 8
- **Expectation**: Scoreboard stores `max_runtime_seconds`. Poller or dedicated monitor can mark jobs as killed if exceeded. Worker respects kill flag.
- **Deliverables**: Limit enforcement + kill path.
- **Validation**: Job exceeding its time limit can be terminated cleanly.
- **Notes / Open Questions**:
- **Lessons Learned**:

---

### Phase 11: Orchestrator Script (the long-running tier)

- **Goal**: Realize the second tier of the two-tier architecture (Phase 1 lessons learned): a long-running Lua script — the **Orchestrator** — that owns the scoreboard, runs the scheduling loop, and creates scoreboard entries that workers pick up.
- **Dependencies**: Phase 7, Phase 9
- **Expectation**:
  - `orchestrator.lua` (or whatever name the deployment uses) is loaded from the `config->scripting.orchestrator` path into a **single, dedicated, long-lived `lua_State`** that is *not* a worker and *not* part of the worker pool.
  - Its chunk is **compiled exactly once** at subsystem launch. It then runs an `H.sleep`-driven scheduling loop, polling the scoreboard and submitting new jobs.
  - The Orchestrator's `lua_State` lives from `launch_scripting_subsystem()` through `landing_scripting_subsystem()`. Landing signals it via a state flag the Lua loop polls (e.g. `H.shutdown_requested()`); after the loop exits, the state is `lua_close`'d.
  - This is distinct from a "special long-running job" — it is **not** an entry in the scoreboard. It is the subsystem's own private context.
  - Different deployments may configure different Orchestrator scripts (poller, event-driven, batch scheduler, etc.) by changing the `orchestrator` config value.
- **Deliverables**: A reference `orchestrator.lua` (trivial scheduler: every N seconds, look at the scoreboard, submit a sample job) + the host functions needed (`H.sleep`, `H.shutdown_requested`, `H.scoreboard.list`, `H.scoreboard.submit`).
- **Validation**: With the Orchestrator configured, jobs appear in the scoreboard over time without external triggers. With no Orchestrator configured, the subsystem still launches and lands cleanly. SIGTERM during a run produces a clean shutdown of the Orchestrator.
- **Notes / Open Questions**:
  - The Orchestrator is the natural home for `H.scoreboard.*` and the scheduler's "tick" logic. The two-tier architecture means *it* owns scheduling, while workers own execution.
  - This phase reuses the Phase 3b launch/landing wiring — it does not introduce a third lifecycle.
- **Lessons Learned**:

---

### Phase 12: Waiter / Completion Signaling Support in Scoreboard

- **Goal**: Add fields and logic to support jobs that have a waiting caller (REST, etc.).
- **Dependencies**: Phase 5
- **Expectation**: Scoreboard entries can store `has_waiter`, `waiter_context`, `result_ref`. On completion, system can signal waiting context.
- **Deliverables**: Waiter fields + basic signaling path.
- **Validation**: Job marked with waiter can notify on completion.
- **Notes / Open Questions**:
- **Lessons Learned**:

---

### Phase 13: Conduit-Equivalent Query Functions (`H.query`, `H.altquery`, `H.authquery`) — Async-First

- **Goal**: Expose the Conduit query capabilities to Lua as first-class host functions, matching the existing REST endpoints, with **async as the default** execution model.
- **Dependencies**: Phase 6, Phase 12 (waiter/result signaling for the wait mechanism)
- **Expectation**:

  ```lua
  -- Named parameters (matches Hydrogen's typed-JSON binding model)
  -- Async by default: returns a handle immediately.
  local h1 = H.query("SELECT * FROM foo WHERE id = :id", { id = 123 })
  local h2 = H.altquery("otherdb", "SELECT ...", { ... })   -- cross-database override
  local h3 = H.authquery(token, "SELECT ...", { ... })      -- authenticated

  -- Lua decides when to join:
  local rows1 = H.wait(h1)
  local rows2, rows3 = H.wait(h2), H.wait(h3)
  ```

  - Map to the existing conduit endpoints/semantics: `H.query` = public single query; `H.altquery` = cross-database single query with database override; `H.authquery` = authenticated single query. **No "queries" (batch) variant** — Lua fans out multiple async calls and joins them itself.
  - Built on the **queue layer** (`database_queue_submit_query` + `database_queue_await_result`), following the `auth_service_database.c` pattern. Marshal Lua tables into Hydrogen's typed `parameter_json` (INTEGER/STRING/BOOLEAN/FLOAT/…); positional `?` binding is not how the engine works.
  - A handle wraps the submitted `query_id` + target queue; `H.wait(handle)` blocks on `database_queue_await_result` and returns the parsed `data_json` as a Lua table (or an error). A synchronous convenience form may wrap "submit + immediate wait", but the async handle is the primitive (this subsumes the earlier separate "async DB query" phase).
  - Decide database selection: `H.query` uses a configured default DB; `H.altquery` takes an explicit database name/override.
- **Deliverables**: `H.query` / `H.altquery` / `H.authquery` + `H.wait`, typed parameter binding, result-table conversion.
- **Validation**: Lua can issue several async queries, do other work, then wait; auth and cross-database variants return correct results; errors surface cleanly via `H.wait`.
- **Notes / Open Questions**: Handle representation (see Phase 2). How auth tokens are supplied to `H.authquery` inside a job context. Reuse conduit's QTC/query-by-id path or accept raw SQL?
- **Lessons Learned**:

---

### Phase 14: Atomic Task Claiming via `affected_rows`

- **Goal**: Give Lua a reliable way to claim a unit of work exactly once across multiple server instances — **without transactions or GUIDs**.
- **Dependencies**: Phase 13
- **The mechanism**: Claiming a task is just a **single atomic conditional UPDATE** whose `WHERE` clause includes the pre-condition, checked via `affected_rows`. If the row was already claimed, the `WHERE` matches nothing and `affected_rows == 0`; the winner sees `affected_rows == 1`. No transaction, no claim token, no GUID:

  ```lua
  local res = H.wait(H.query([[
     UPDATE tasks
        SET status = 'taken', claimed_by = :instance, claimed_at = now()
      WHERE id = :id AND status = 'open'
  ]], { id = task_id, instance = H.system.instance_id() }))

  if res.affected_rows == 1 then
     -- we own the task
  else
     -- someone else already claimed it
  end
  ```

  (`claimed_by` here is just informational bookkeeping, not part of the correctness argument — the correctness comes entirely from the `AND status='open'` guard plus `affected_rows`.)
- **The one requirement this places on the plan**: `H.query`/`H.altquery`/`H.authquery` results **must reliably expose `affected_rows`** for write statements, across all engines. `QueryResult.affected_rows` already exists at the engine layer; this phase's real work is making sure it is populated for UPDATE/INSERT/DELETE on every engine and surfaced to Lua in the result table. Add per-engine tests asserting the value.
- **Transactions: out of scope.** No `H.db.transaction` is planned. Multi-statement transactions are deliberately **not** part of this subsystem for now; if a future use case genuinely needs all-or-nothing multi-row commits, it will be scoped as a separate follow-on (the migration engine already demonstrates a per-connection-isolated BEGIN/COMMIT/ROLLBACK pattern that could be adapted then).
- **Deliverables**: Documented atomic-claim recipe + verified `affected_rows` in Lua result tables for write queries on all engines.
- **Validation**: Concurrent claim test — N workers race for the same task, **exactly one** sees `affected_rows == 1`, the rest see `0`. Per-engine `affected_rows` correctness tests.
- **Notes / Open Questions**: Confirm each engine's driver reports affected rows for UPDATE/INSERT/DELETE the way we expect (PostgreSQL `PQcmdTuples`, SQLite `sqlite3_changes`, MySQL `mysql_affected_rows`, DB2 row count).
- **Lessons Learned**:

---

### Phase 15: (Folded into Phase 13)

Async DB access with handles is now the **default** model delivered in Phase 13, so a separate async-DB phase is unnecessary. This slot is intentionally left as a marker to preserve numbering; renumber on the next full revision.

---

### Phase 16: `H.http` Basic Outbound Calls

- **Goal**: Expose outbound HTTP capability to Lua (for LLM, Canvas API, image servers, etc.).
- **Dependencies**: Phase 6
- **Expectation**:

  ```lua
  local res = H.http.post(url, body, headers)
  ```

  Or async version later.
- **Deliverables**: Basic `H.http.get` / `H.http.post`.
- **Validation**: Lua script can make outbound HTTP call and receive response.
- **Notes / Open Questions**:
- **Lessons Learned**:

---

### Phase 17: Async `H.http` + Wait Pattern

- **Goal**: Make HTTP calls support the same async + handle + wait model as DB queries.
- **Dependencies**: Phase 15, Phase 16
- **Expectation**: Consistent API between `H.db` and `H.http` for async usage.
- **Deliverables**: Async HTTP support aligned with DB async model.
- **Validation**: Script can fire HTTP call, continue, then wait for response.
- **Notes / Open Questions**:
- **Lessons Learned**:

---

### Phase 18: `H.llm.call` Convenience Wrapper (feasible now — schedule early)

- **Goal**: Provide a clean way to call LLMs from Lua by model name.
- **Dependencies**: Phase 16 (HTTP). The async form additionally depends on Phase 17.
- **Scheduling note**: This is **feasible with today's code** (model-by-name resolution + chat proxy already exist), so it is pulled **earlier** than its phase number suggests — right after basic HTTP. It does **not** need to wait for the mail/notify work.
- **Expectation**:

  ```lua
  local response = H.llm.call({
      model = "grok-light",     -- just a name; the server resolves it
      prompt = "Is this spam?",
      max_tokens = 200
  })

  local models = H.llm.list()   -- available models (backed by the conduit models query)
  ```

  - Lua passes a **model name only**. The server resolves it exactly like the WebSocket chat path: `chat_engine_cache_lookup_by_name(cache, name)` → resolved `ChatEngineConfig` (provider, model, `api_url`, `api_key`) → `chat_proxy_send_with_retry(engine, ...)`. The Lua author knows the name; Lua code does not hardcode provider/endpoint.
  - `H.llm.list()` exposes available models, reusing the enumeration the conduit status endpoint already produces (`cec->engines[]` / QueryRef #061), so scripts can discover models at runtime.
  - No new "LLM registry" is required — reuse `engine_cache`.
- **Deliverables**: `H.llm.call` + `H.llm.list` implementations.
- **Validation**: Lua can call different models by name and get structured responses; `H.llm.list` returns the configured models.
- **Notes / Open Questions**: Which database's engine cache is used when multiple exist? Streaming is out of scope for v1 (use the non-streaming `chat_proxy_send_request` path).
- **Lessons Learned**:

---

### Phase 19: Message Sending (`H.mail` / `H.notify`) — Deferred to End

- **Goal**: Expose ability to send email (and later SMS/notifications) from Lua.
- **Scheduling note**: **Deferred to the end of the plan.** The calling conventions and **stubs** for `H.mail` / `H.notify` are defined during Phase 2 (Host API conventions) so the surface is stable, but the working implementations land last:
  - `H.mail` depends on the **Mail Relay implementation** (`MAILRELAY_PLAN.md`). Today `src/mailrelay/` is empty and `launch_mail_relay_subsystem()` is a stub — no send API exists. This will likely not be implemented until the rest of this plan is largely done.
  - `H.notify` depends on the **Notify** subsystem, which is also a stub today, and will come later still.
- **Dependencies**: Phase 2 (for stubs); Mail Relay / Notify implementations (for real sending).
- **Expectation**:

  ```lua
  H.mail.send({
      to = "...",
      subject = "...",
      body = "...",
      template = "report_ready"
  })
  ```

  Until the backends exist, the stubs return a clear "not implemented" error so scripts and earlier phases are never blocked.
- **Deliverables**: Stubbed API early; real message sending once Mail Relay / Notify land.
- **Validation**: Stubs error cleanly now; once backends exist, Lua can trigger a real send.
- **Notes / Open Questions**:
- **Lessons Learned**:

---

### Phase 20: DB-backed Script & Module Loading (Basic)

- **Goal**: Allow Lua scripts and modules to be loaded from the database instead of filesystem.
- **Dependencies**: Phase 4
- **Expectation**: Custom package searcher that queries DB by name/path and loads code. Support for `require("reporting.common.date_formatter")`.
- **Deliverables**: DB-backed `require` mechanism.
- **Validation**: `require` works for scripts stored in DB.
- **Notes / Open Questions**:
- **Lessons Learned**:

---

### Phase 21: Bytecode Caching for DB-loaded Modules

- **Goal**: Cache compiled bytecode for DB-loaded modules to improve performance.
- **Dependencies**: Phase 20
- **Expectation**: On first load, compile and store bytecode. Subsequent loads use cached version (with invalidation on content change).
- **Deliverables**: Bytecode caching layer for DB modules.
- **Validation**: Repeated `require` of same module is fast after first load.
- **Notes / Open Questions**:
- **Lessons Learned**:

---

### Phase 22: Lua Logging Integration (VictoriaLogs)

- **Goal**: Make `H.log.*` calls emit structured logs that flow into VictoriaLogs.
- **Dependencies**: Phase 6
- **Expectation**: Log calls from Lua jobs appear in the same logging pipeline as the rest of Hydrogen.
- **Deliverables**: Logging backend integration from Lua.
- **Validation**: Log entries from Lua scripts are visible in logs with correct context (job_id, etc.).
- **Notes / Open Questions**:
- **Lessons Learned**:

---

### Phase 23: Scoreboard Visibility in Health / State Endpoints

- **Goal**: Expose useful views of the scoreboard through existing health/state/info REST endpoints (with privacy filtering).
- **Dependencies**: Phase 5
- **Expectation**: Admin or monitoring endpoints can show running jobs, long-running jobs, etc.
- **Deliverables**: REST endpoints or internal views for scoreboard data.
- **Validation**: Scoreboard state is observable via existing monitoring paths.
- **Notes / Open Questions**:
- **Lessons Learned**:

---

### Phase 24: Structured Error Handling from Lua

- **Goal**: Improve how errors from Lua are captured, logged, and returned to callers (especially for waited REST jobs).
- **Dependencies**: Phase 4, Phase 12
- **Expectation**: Lua errors produce structured error objects with message, traceback (sanitized), and job context.
- **Deliverables**: Better error capture and propagation.
- **Validation**: Errors from Lua jobs are logged with useful context and returned cleanly to waiters.
- **Notes / Open Questions**:
- **Lessons Learned**:

---

### Phase 25: Artifact / Result Storage for Waited Jobs

- **Goal**: Provide a clean way for completed jobs to expose output artifacts (JSON files, PDFs, etc.).
- **Dependencies**: Phase 12
- **Expectation**: Scoreboard can store `result_type` and `result_location` (file path or DB reference). REST layer can retrieve it.
- **Deliverables**: Artifact reference mechanism.
- **Validation**: A job that produces a file can have its location returned to the waiting caller.
- **Notes / Open Questions**:
- **Lessons Learned**:

---

### Phase 26: End-to-End Validation – Report Writer Pattern (Minimal)

- **Goal**: Demonstrate a minimal version of the Report Writer flow using the new infrastructure.
- **Dependencies**: Phases 13–17, 20
- **Expectation**: A simple `Reporter.lua` that accepts a report name + params, fetches data (possibly async), applies basic transformation, and returns JSON.
- **Deliverables**: Working minimal report execution path + tests.
- **Validation**: Can trigger a report via REST (or test harness) and receive structured output.
- **Notes / Open Questions**:
- **Lessons Learned**:

---

### Phase 27: End-to-End Validation – Simple Workflow Stage (Canvas-style)

- **Goal**: Demonstrate a minimal multi-stage workflow with state transition and notification.
- **Dependencies**: Phase 26 (or parallel)
- **Expectation**: Simple stage script that claims work via an atomic conditional UPDATE (`affected_rows == 1`; see Phase 14), does something, sends a notification, and updates state.
- **Deliverables**: Working minimal workflow stage execution.
- **Validation**: State machine step can be triggered and completed with notification.
- **Notes / Open Questions**:
- **Lessons Learned**:

---

### Phase 28: Polish & Documentation Pass

- **Goal**: Improve consistency, add examples, and document the `H` API surface.
- **Dependencies**: Most previous phases
- **Expectation**: Good inline documentation, example scripts in the repo, and a clear reference for the `H` namespace.
- **Deliverables**: Documentation and example scripts.
- **Validation**: New Lua developers can understand how to use the system from the docs + examples.
- **Notes / Open Questions**:
- **Lessons Learned**:

---

## Risks and Mitigations

- **Lua heap-corruption history (was highest risk — downgraded by two-tier architecture)**: The migration engine deliberately uses a custom `mmap` allocator, a fresh `lua_State` per migration, and copies data out of Lua before closing — because *reusing states with repeated `luaL_loadbuffer` compilations* caused memory corruption. The two-tier architecture (Phase 1 lessons learned) sidesteps this: workers get a fresh `lua_State` per job (one compilation, then `lua_close`) — the same pattern the migration engine runs ~200× per server with no issues — and the Orchestrator's chunk is compiled once at launch and only calls C host functions thereafter. **Mitigation**: keep the `lua_mmap_alloc` allocator as cheap insurance; add a Unity soak test for the Orchestrator's one-time-load lifetime (no second compilation) before Phase 3b lands; enforce UAF discipline in the host API (always copy strings out of Lua memory before letting the C side hold them).
- **Thread-safety of a single `lua_State`**: A `lua_State` is not thread-safe. Workers must each own their own state, and the Orchestrator's state is touched only by its own thread. **Mitigation**: one `lua_State` per worker thread, and *only* per worker invocation (state is destroyed between jobs); one `lua_State` for the Orchestrator, dedicated to its own thread; scoreboard is the only shared, mutex-protected structure.
- **Reliable `affected_rows`**: Atomic task claiming (Phase 14) depends entirely on write queries reporting `affected_rows` correctly on every engine. **Mitigation**: per-engine tests asserting `affected_rows` for UPDATE/INSERT/DELETE; treat any engine that misreports it as a blocker for that engine.
- **Dependencies on unimplemented subsystems**: Mail (Phase 19) and LLM (Phase 18) depend on work not yet present. **Mitigation**: sequence these after their prerequisite plans, or stub with clear "not implemented" errors so earlier phases are not blocked.
- **Sandboxing**: exposing `H.db`, `H.http`, and DB-loaded modules to script authors is a significant attack surface. **Mitigation**: keep the Phase 3 sandbox strict (no `io`, controlled `os`), and treat DB-sourced code (Phase 20) as trusted-only initially.

## Summary

This plan currently contains **28 focused phases** organized around a **two-tier Lua architecture** (settled in Phase 1):

- **Workers** — fresh `lua_State` per job, threaded through the worker pool. Mirrors the migration engine's proven pattern.
- **Orchestrator** — one long-lived `lua_State`, compiled once at subsystem launch, configured via `config->scripting.orchestrator`, lives across the entire `launch_scripting_subsystem()` / `landing_scripting_subsystem()` window, owns the scheduling loop and the scoreboard.

More phases can (and likely will) be added as we proceed and discover additional needs. The structure is designed to support the interactive, learning-oriented development process agreed upon.

Before any implementation work on a phase, the current state is reviewed and the detailed approach for that phase is agreed. **Everything prior to Phase 4 is complete: Phase 1 (assessment), Phase 2 (host API namespace design), Phase 2b (Scripting config section and global defaults), Phase 3 (sandboxed Lua context lifecycle), and Phase 3b (Scripting subsystem skeleton: enablement, launch, and early shutdown — proven leak-free by ASAN in Test 11). Phase 4 (`H_lua_run_string` for executing a string buffer) is the next step.**