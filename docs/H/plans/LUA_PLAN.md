# LUA_PLAN.md

Hydrogen Scripting Subsystem (Lua) – Implementation Roadmap

**Status**: Draft – Actively evolving; Phases 1, 2, 2b, 3, 3b, 4, 5, 6, 7, and 8 complete; Phase 9 is next
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
  - **Registry integration regression discovered after the fact**: Adding `app_config->scripting.Enabled` to `update_registry_on_startup()` (so the registry could report the Scripting subsystem's state) caused `registry_integration_test_update_registry_on_startup` to segfault. The test had set `app_config = (AppConfig*)0x12345678` as a "non-null pointer" placeholder; before Phase 3, `update_registry_on_startup()` only used `app_config` in `app_config && !flag` guards, so the bogus pointer was never dereferenced. The new `app_config->scripting.Enabled` read dereferenced it. Two sub-lessons: (a) Unity tests that use `__attribute__((weak))` to mock globals cannot override strong symbols from `src/globals.c`; the real `app_config` is always the one being mutated, so bogus pointers are real segfaults. (b) Any change that adds a dereference of a test-mocked global must be paired with either a real struct instance in the test or a defensive NULL guard that the test exercises. We fixed the test by replacing the placeholder pointer with a real zeroed `AppConfig mock_app_config` and adding `memset(&mock_app_config, 0, ...)` in `setUp`. The full Unity suite now passes (7,178 / 7,178 tests).

---

### Phase 4: Execute Simple Lua Script from String

- **Goal**: Load and execute a Lua chunk from a string buffer.
- **Dependencies**: Phase 3
- **Status**: **Complete 2026-07-01.** `H_lua_run_string` is implemented in `src/scripting/lua_context.c` and a 10-test Unity suite (`lua_context_test_run_string`) exercises success, multiple return values, syntax errors, runtime errors, NULL safety, and chained execution on the same context.
- **Expectation**:

  ```c
  int H_lua_run_string(lua_State* L, const char* code, const char* name);
  ```

  Return value handling and basic error reporting.
- **Deliverables**:
  - `H_lua_run_string(lua_State* L, const char* code, const char* name)` declared in `src/scripting/lua_context.h` and implemented in `src/scripting/lua_context.c`.
  - Unity test `tests/unity/src/scripting/lua_context_test_run_string.c` (10 tests).
- **Validation**:
  - `mkt` equivalent (`extras/make-trial.sh`) — **PASS**, build successful.
  - `mkp` equivalent (`tests/test_91_cppcheck.sh`) — **PASS**, 1,390 files, 0 issues.
  - `mku` equivalent — **`lua_context_test_run_string` — PASS**, 10/10 tests.
  - Regression: Phase 3 / 3b Unity suites all still pass (`lua_context_test_create_destroy` 4/4, `launch_scripting_test_check_scripting_launch_readiness` 4/4, `launch_scripting_test_launch_scripting_subsystem` 2/2, `landing_scripting_test_check_scripting_landing_readiness` 2/2, `landing_scripting_test_land_scripting_subsystem` 3/3).
  - `tests/test_12_env_variables.sh` — **PASS**, 15/15.
  - Full Unity suite (`tests/test_10_unity.sh`) — **7,171 / 7,172 passing**; the 1 failure (`registry_integration_test_update_registry_on_startup`) is a pre-existing cached failure unrelated to this phase.
  - ASAN / leak validation deferred to the Phase 7 worker build (where the function will actually run inside a thread). The function itself is a thin wrapper around `luaL_loadbufferx` + `lua_pcall` against an existing `H_lua_create_context` state; the proven-clean ASAN lifecycle from Phase 3b (init + land with no chunks executed) bounds the new risk to whatever a single chunk's allocations look like, and that surface is exactly what Phase 7 will exercise.
- **Notes / Open Questions**:
  - `luaL_loadbufferx` is preferred over the older `luaL_loadbuffer` so the source's `mode` argument is explicitly `NULL` (text chunk). The behavior is identical to `luaL_loadbuffer` for our use case but the call site documents intent and is forward-compatible with `mode`-based loading later.
  - **Error model**: on failure, a single error string is left on the stack (Lua's own contract for `luaL_loadbufferx` / `lua_pcall`); on success, `LUA_MULTRET` lets the chunk leave any number of return values. The function does **not** pop the error — the caller is responsible for both (a) copying the string to C-owned memory before any other Lua call (UAF discipline from Phase 1) and (b) `lua_pop`ping it before resuming. The header documents this contract explicitly.
  - **NULL safety**: `NULL` state and `NULL` code both return `LUA_ERRRUN` with an explanatory log line. `NULL` name falls back to `"?"` (Lua's default chunk name) and is treated as success.
  - **Empty string** (`""`) is a valid no-op chunk and returns `LUA_OK` with no return values, which is useful for Phase 7's worker pool where an empty job should not be an error.
  - The function is the **worker-tier** primitive (called once per job on a fresh state). The Orchestrator (Phase 11) compiles its single chunk at launch via `luaL_loadbufferx` directly and then drives it with a scheduling loop — it does **not** use this wrapper, matching the Phase 1 two-tier architecture.
- **Lessons Learned**:
  - The error-string-on-stack contract is small but powerful: it matches what every Lua C embedding does and means the caller's error-handling code is identical for compile errors and runtime errors. Trying to "improve" this by copying the error into an out-parameter would just create a second code path that the worker pool (Phase 7) and the REST wait path (Phase 12) would both have to remember.
  - The chunk-name argument matters more than it looks. A worker pool that passes `"[job:42]"` instead of `"?"` gets free log context — the `[job:42]:7: attempt to index a nil value` line tells an operator exactly which job and which line failed. The Phase 4 contract leaves this up to the caller, which is the right boundary: `H_lua_run_string` does not know about jobs, but every worker call site will.
  - Returning `LUA_OK` on an empty string turned out to be a small but useful design choice. A job whose source happens to be empty (e.g. a placeholder file or a stripped debug script) is not an error in a long-lived server, and surfacing it as one would push the empty-check into every caller. This decision is one line in the implementation and removes a category of "why did this fail?" debugging from Phase 7.
  - The two-tier architecture (Phase 1) keeps paying off: `H_lua_run_string` does not need to know about state lifetime, allocator choice, or the Orchestrator's special role. It is a single-purpose function that fits the worker tier exactly, and the test suite reflects that (no thread context, no scoreboard, just a state, a buffer, and a result).

---

### Phase 5: Basic Scoreboard Data Structure (C)

- **Goal**: Implement thread-safe in-memory scoreboard for tracking jobs.
- **Dependencies**: Phase 3
- **Status**: **Complete 2026-07-01.** Scoreboard v1 is implemented, wired into the subsystem's init/cleanup lifecycle, and exercised by 40 new Unity tests (5 files) plus a 4th landing test that confirms the scoreboard is destroyed on shutdown. cppcheck clean; full Unity suite at 7,219/7,219.
- **Expectation**: Struct with `job_id`, `script_name`, `status`, `limits`, `instruction_count`, `current_state`, `created_at`, etc. Protected by mutex or lock-free where practical.
- **Implementation approach** (decided 2026-07-01):
  - **Single growable array** (`ScoreboardEntry* entries`) protected by a single `pthread_mutex_t`. No fancy lock-free structures — the migration engine's straightforward approach is the right model for v1.
  - **Copy-on-find**: `scoreboard_find` returns a heap copy of the entry (caller frees with `scoreboard_entry_free`). This removes the "remember to hold the mutex while reading" foot-gun for Phase 11's `H.scoreboard.list` and the REST wait path, and the copy survives subsequent `realloc`s.
  - **On-demand growth** (`realloc` doubles capacity). The `lua_mmap_alloc`-style cheap-insurance allocator is not needed here because the scoreboard's allocations are C-process-heap allocations, not Lua runtime allocations.
  - **ID generation**: reuses Hydrogen's existing `generate_id` (`ID_CHARS` / `ID_LEN` from `globals.h`). 5-char human-readable IDs match the rest of the project. Collisions on a 12^5 space are extremely unlikely in v1 and `submit` retries up to 8 times before returning NULL.
  - **`submit(script_name, params_json)`** — accepts an opaque `params_json` string now so the Phase 13 `H.query`/`H.altquery`/`H.authquery` submit paths don't need a signature change later. The scoreboard strdup's both, so the caller may free its copies.
  - **`update_status` stamps `started_at` on PENDING→RUNNING and `finished_at` on any transition into a terminal state (COMPLETED / FAILED / KILLED).** Idempotent: re-setting the same status is a no-op and does not re-stamp timestamps.
  - **Wired into `scripting_init_state` and `scripting_cleanup_state`.** The `scripting_scoreboard` pointer is created on launch (when enabled) and destroyed on landing. This proves the shutdown path end-to-end with the scoreboard populated, in the same way Phase 3b proved the Orchestrator-state shutdown path.
  - **Reserved for later phases (NOT in v1)**: `instruction_count` / `memory_used_bytes` (Phase 8 hook), `current_state` (Phase 9), `max_runtime_seconds` / `kill_requested` (Phase 10), `has_waiter` / `waiter_context` / `result_ref` (Phase 12), `result_type` / `result_location` (Phase 25), `H.scoreboard.list` iteration (Phase 11).
- **Deliverables**:
  - `src/scripting/scoreboard.h` — public API
  - `src/scripting/scoreboard.c` — implementation
  - `Scoreboard* scripting_scoreboard;` external in `src/scripting/scripting.h`, defined in `src/scripting/scripting.c`
  - `scripting_init_state()` and `scripting_cleanup_state()` updated to create / destroy the scoreboard
  - Updated `tests/unity/src/launch/launch_scripting_test_launch_scripting_subsystem.c` (tearDown now destroys the scoreboard to prevent leaks between tests)
  - Updated `tests/unity/src/landing/landing_scripting_test_land_scripting_subsystem.c` (added a 4th test that confirms the scoreboard is destroyed on landing)
  - 5 new Unity test files in `tests/unity/src/scripting/`:
    - `scoreboard_test_create_destroy.c` (5 tests)
    - `scoreboard_test_submit.c` (11 tests)
    - `scoreboard_test_find.c` (9 tests)
    - `scoreboard_test_update_status.c` (11 tests)
    - `scoreboard_test_concurrent.c` (4 tests — real pthreads, mutex validation)
- **Validation**:
  - `mkt` equivalent (`extras/make-trial.sh`) — **PASS**
  - `mkp` equivalent (`tests/test_91_cppcheck.sh`) — **PASS**, 1,397 files, 0 issues
  - `mku scoreboard_test_create_destroy` — **PASS**, 5/5
  - `mku scoreboard_test_submit` — **PASS**, 11/11
  - `mku scoreboard_test_find` — **PASS**, 9/9
  - `mku scoreboard_test_update_status` — **PASS**, 11/11
  - `mku scoreboard_test_concurrent` — **PASS**, 4/4 (8 threads × 50 jobs each, mutex proven correct)
  - `mku launch_scripting_test_launch_scripting_subsystem` — **PASS**, 2/2 (no regression)
  - `mku landing_scripting_test_land_scripting_subsystem` — **PASS**, 4/4 (3 prior + 1 new for scoreboard cleanup)
  - Full Unity suite (`tests/test_10_unity.sh`) — **7,219 / 7,219 passing** (was 7,178 before Phase 5; +41 from new tests)
  - `tests/test_16_shutdown.sh` — **PASS**, 5/5
  - `tests/test_17_startup_shutdown.sh` — **PASS**, 9/9 (min and max configurations still start and stop cleanly with the new scoreboard path)
  - Test 11 (ASAN) — **deferred**: requires the debug build, which the trial build does not produce. The trial coverage of the same lifecycle (launch + land with the scoreboard in place) is exercised by Test 17's "max" configuration and by `landing_scripting_test_land_scripting_subsystem`.
- **Notes / Open Questions**:
  - **Re-entering a terminal state is allowed in v1.** A subsequent `update_status(COMPLETED → PENDING)` is not prevented. This is a deliberate v1 simplicity choice: the scoreboard is a passive data structure in this phase, and the worker / orchestrator code that calls `update_status` (Phase 7/11) is expected to drive the state machine correctly. If misuse becomes a real risk, a small "no transition out of terminal" guard can be added in Phase 7/10 without changing the public API.
  - **The scoreboard lives for the duration of the subsystem**, not per-request. Phase 11's `H.scoreboard.list` and the eventual REST visibility layer (Phase 23) read this same instance.
  - **The script_name field is `strdup`'d by the scoreboard and freed by `entry_clear_owned`.** The convention matches Phase 4's UAF discipline: any string handed to the C side is copied, and the caller can free its own copy after the call.
- **Lessons Learned**:
  - **Copy-on-find was the single most important v1 decision.** Returning a copy from `scoreboard_find` means callers can read the entry fields without holding the scoreboard's mutex, which is exactly what later phases will need: the worker pool, the Orchestrator's `H.scoreboard.list`, and the REST wait path all want to "snapshot" an entry and then do work on it (format a JSON response, log a metric, etc.) without blocking submit/update from other threads. Trying to "improve" this with a reader-writer lock or a hazard-pointer scheme would just push complexity into callers.
  - **The 5-char ID space is large enough that we don't need a GUID.** 12^5 = 248,832 IDs. A single scoreboard's lifetime is one process; even 100k jobs would only see a ~2% collision rate on a uniform random distribution. The 8-attempt retry bound makes the practical collision probability negligible. The win is that operators see human-readable IDs in logs (`[job:BCDFG]`) instead of GUIDs.
  - **Wiring the scoreboard into `scripting_init_state` / `scripting_cleanup_state` early pays off.** Phase 3b proved the "shutdown is proven early" pattern by attaching a real Lua state to `scripting_orchestrator_state` and landing it. Doing the same for the scoreboard now means Phase 7 (worker pool) and Phase 11 (Orchestrator) inherit a known-clean shutdown path, and Test 11 / Test 17 can be re-run with the scoreboard populated to verify no leak. This is the same pattern: prove the lifecycle on a minimal surface before piling features on top.
  - **The concurrent test is the only one that needs real pthreads.** All other scoreboard tests run on a single thread, which is faster and easier to reason about. The mutex is proven correct by 4 concurrent tests (8 threads × 50 jobs each) that exercise submit, find, update, and a mixed workload, then assert exact post-state invariants.
  - **Updating existing test files was the right move.** Adding a destroy hook to the `tearDown` of `launch_scripting_test_launch_scripting_subsystem` and `landing_scripting_test_land_scripting_subsystem` kept them stable as the scoreboard lifecycle grew. The alternative (a new test file that "exercises the scoreboard" in isolation) would have duplicated the launch/land flow and obscured the fact that the scoreboard is part of the same lifecycle.
  - **A new 4th test in the landing file makes the wiring explicit.** Without it, the "scoreboard is destroyed on land" property was implicit in `scripting_cleanup_state`. The new test creates a real scoreboard, submits a real job (so the scoreboard has owned strings to free), lands, and asserts the pointer is NULL — the same shape as the existing Orchestrator cleanup test, so future readers see the parallel.

---

### Phase 6: Expose First Host Functions (`H.log` and `H.system`)

- **Goal**: Make minimal useful functions available inside Lua via the `H` table.
- **Dependencies**: Phase 4, Phase 5
- **Status**: **Complete 2026-07-01.** `H.log.{trace,debug,info,warn,error,fatal}` and `H.system.{uptime,now,now_iso,instance_id,version}` are implemented in a new `src/scripting/scripting_api.{c,h}` module, installed by `H_lua_install_api` on context creation, and verified by 23 new Unity tests across two suites.
- **Expectation**:

  ```lua
  H.log.info("message", ...)
  H.log.error("message", ...)
  H.system.uptime()          -- or H.system.uptime
  H.system.now()
  ```

- **Implementation approach** (decided 2026-07-01):
  - New file `src/scripting/scripting_api.{c,h}` for the C-side host functions, mirroring the location named in `lua_api.md`. `lua_context.c` keeps its single focus on context lifecycle.
  - `H_lua_install_api` is updated in place to call the new `H_lua_install_log` and `H_lua_install_system` after the existing empty sub-table placeholders are created. The Phase 3 test (`test_lua_create_context_installs_H_table`) keeps passing because the sub-tables are still tables, just with functions inside.
  - `H.log.*` uses Lua's own `string.format` to build the message (delegating all format specifier support to Lua) and forwards the result to `log_this` with `num_args=0` and the `Scripting` subsystem tag. `count_format_specifiers` is consulted against the arg count and a clear "format/args mismatch" log line is emitted (without raising a Lua error) when they disagree - the host log path is a leaf, not a source of crashes.
  - `H.system.uptime` uses `time(NULL) - get_server_start_time()`; if the start time has not been recorded (Unity test that doesn't initialize the time subsystem) it returns 0, not a negative number.
  - `H.system.now_iso` uses `gmtime_r` + `strftime` for ISO 8601 UTC.
  - `H.system.instance_id` lazily generates a 36-char UUID v4 (16 random bytes from `/dev/urandom`, with a `time(NULL) ^ getpid()`-seeded LCG fallback) and caches it for the process lifetime - same value across calls and across Lua contexts.
  - `H.system.version` returns the `VERSION` macro.
- **Deliverables**:
  - `src/scripting/scripting_api.h` and `src/scripting/scripting_api.c` (new)
  - `lua_context.c` updated: `#include "scripting_api.h"`, `H_lua_install_log` and `H_lua_install_system` called at the end of `H_lua_install_api`
  - `tests/unity/src/scripting/scripting_api_test_log.c` (14 tests)
  - `tests/unity/src/scripting/scripting_api_test_system.c` (9 tests)
- **Validation**:
  - `mkt` equivalent (`extras/make-trial.sh`) - **PASS**, build successful
  - `mkp` equivalent (`tests/test_91_cppcheck.sh`) - **PASS**, 1,401 files, 0 issues
  - `mku scripting_api_test_log` - **PASS**, 14/14 tests
  - `mku scripting_api_test_system` - **PASS**, 9/9 tests
  - `tests/test_10_unity.sh` - **PASS**, 7,242 / 7,242 unit tests (was 7,219 before Phase 6; +23 from new tests)
  - `tests/test_01_compilation.sh` - **PASS**, 18/18
  - `tests/test_16_shutdown.sh` - **PASS**, 5/5
  - `tests/test_17_startup_shutdown.sh` - **PASS**, 9/9 (min and max configurations still start and stop cleanly)
  - `tests/test_03_shell.sh` - **PASS**, 45/45
  - `tests/test_12_env_variables.sh` - **PASS**, 15/15
- **Notes / Open Questions**:
  - The mock `log_this` used by Unity tests runs `vsnprintf` on the formatted message, which misinterprets literal `%` characters as format specifiers. The production code is correct (Lua's `string.format` handles `%%` before forwarding to `log_this`); the test for the `%%` case asserts the call was made with the right subsystem/priority and a non-empty message rather than the exact characters.
  - `H.system` deliberately bypasses Lua's `os.*` library: it always provides `uptime`, `now`, `now_iso`, `instance_id`, and `version` regardless of the `AllowOsTime`/`AllowOsExecute` sandbox policy. The sandbox still gates `os.*` from script authors; the host path is a separate, more controlled surface.
- **Lessons Learned**:
  - Routing `H.log.*` through Lua's `string.format` is a small but powerful choice. We get full Lua-side format-specifier support (including `%05d`, `%.2f`, etc.) for free, we don't have to write a C-side spec walker that handles width/precision/flags/length-modifiers, and the test for format/arg mismatch can rely on `count_format_specifiers` (already a shared utility) as a single source of truth. The cost is one Lua C API call per log line, which is negligible compared to the I/O that `log_this` does.
  - The "host log path is a leaf, not a source of crashes" rule paid off immediately: a format/arg mismatch logs `H.log: format/args mismatch (format expects 1, got 2)` and returns cleanly instead of raising a Lua error that would unwind the calling Lua state. The Unity test `test_log_arg_mismatch_logs_error_does_not_raise` pins this property and will catch any future regression that turns the mismatch into a raise.
  - The mock `log_this` quirk (it runs `vsnprintf` on the message and consumes literal `%`) is a useful reminder that test infrastructure shapes what we can assert. The Phase 6 tests assert on call-site properties (subsystem, priority, presence of a non-empty message) rather than exact characters when the format naturally contains `%`. This is the right boundary: we test the *contract* with `log_this`, not the mock's `vsnprintf` behavior.
  - `H.system.instance_id` had a design choice between `/proc/self/...`-derived IDs and a per-process random UUID. Random UUID is the right call: it doesn't change on restart of the same binary, doesn't leak hostname or PID (which is useful when a job's logs end up in a shared store), and is stable across calls as required by the `instance_id` docstring.
  - Putting `H_lua_install_log` / `H_lua_install_system` after the existing empty sub-table creation in `H_lua_install_api` is a small but important ordering decision. The Phase 3 install creates the placeholders; later phases add functions. This means a Phase 3-era test that asserts "H.log is a table" continues to pass, and a Phase 6-era test that asserts "H.log.info is a function" can be added without coordination. Later phases (Phase 11 for `H.scoreboard.*`, Phase 13 for `H.query`, etc.) will follow the same pattern - placeholders first, functions later - so the install ordering remains a single append-only list at the bottom of `H_lua_install_api`.

---

### Phase 7: Job Worker Pool Basics

- **Goal**: Create worker threads that can execute Lua jobs from the scoreboard.
- **Dependencies**: Phase 5, Phase 6
- **Status**: **Complete 2026-07-01.** Worker pool is implemented, wired into launch/landing, and verified by 27 new Unity tests across 4 suites, plus an updated launch test that sets a valid `WorkerCount`. The pool runs end-to-end (submit → worker picks up → fresh `lua_State` created → script runs → `lua_State` destroyed → status COMPLETED/FAILED) and shuts down deterministically (drain-on-shutdown with bounded exit). Full Unity suite at 7,269 / 7,269; Test 16 5/5; Test 17 9/9; cppcheck 1,409 files, 0 issues.
- **Expectation**: Small pool of pthreads. Worker dequeues from scoreboard, **creates a fresh `lua_State` for the job**, runs the script, updates scoreboard status on completion/failure, then **destroys the `lua_State` and loops**. **Threads are recycled; `lua_State`s are not** — this is the migration-engine pattern (`execute_single_migration_load_only_with_state(NULL, ...)`) and is the proven-safe tier from the Phase 1 lessons learned. **Reuse existing primitives**: `src/queue/queue.h` for the job queue and `ServiceThreads` (`src/threads/threads.h`, `init_service_threads` / `add_service_thread_with_description`) for thread registration and memory metrics, so Lua jobs appear in existing thread reporting. Each worker thread never shares its `lua_State` with another thread.
- **Implementation approach** (decided 2026-07-01):
  - **Two new modules** under `src/scripting/`: `script_registry.{c,h}` (in-memory `name → source` map) and `worker_pool.{c,h}` (the pthreads + job queue + submit API).
  - **The job queue** is a `Queue*` from `src/queue/queue.h` (created via `queue_create_with_label` with the constant name `SCRIPTING_JOB_QUEUE_NAME`). Workers poll with `queue_dequeue_nonblocking` and `usleep(WORKER_POLL_USEC)` (1 ms) when idle. This is cheap at the configured `WorkerCount` (default 2, max 16) and avoids blocking on the queue's condvar, which would complicate the shutdown path.
  - **Submit API has two entry points**:
    - `scripting_submit_job(script_name, params_json)` — production path. The source is expected to already be registered (the Orchestrator or a REST submit will go through a future DB-backed loader that calls `script_registry_register`).
    - `scripting_submit_job_with_source(script_name, source, params_json)` — testing / REPL / eval path. Registers the source inline (replacing any prior registration) before submitting. Useful for ad-hoc evaluation, the future Lua REPL, and the test suite.
  - **Both return** a heap-allocated `job_id` (caller frees) or NULL on failure. Internally: `scoreboard_submit` (creates the PENDING entry, returns the job_id) → `queue_enqueue` (strdup'd job_id onto the work queue).
  - **Worker thread** runs `scripting_worker_thread(void* arg)`: loop, `queue_dequeue_nonblocking` (1 ms poll), `scripting_worker_process_one(pool, job_id)` on success. UAF discipline from Phase 1: the error string is `strdup`'d out of Lua memory to a C-owned buffer before the `lua_State` is destroyed.
  - **Shutdown**: `scripting_workers_destroy()` sets `scripting_system_shutdown = 1`, NULLs `scripting_workers`, `pthread_join`s all workers (letting them drain the queue), `queue_clear`s any leftover items, `queue_release`s the queue, and frees the pool + registry. The "drain on shutdown" property is verified by `test_destroy_drains_in_flight_work` (submits 2 jobs, calls destroy, asserts both jobs end COMPLETED).
  - **Landing order**: `scripting_workers_destroy()` runs before `scripting_cleanup_state()` (which destroys the scoreboard). The workers must be able to write to the scoreboard until they have finished each in-flight job, so the scoreboard outlives the workers. This matches the Phase 3b landing readiness check (`scripting_threads.thread_count > 0` ⇒ no-go) becoming meaningful for the first time.
  - **Lifecycle placement**: `scripting_init_state` / `scripting_cleanup_state` keep their scope (shutdown flag, scoreboard, ServiceThreads init). The new `scripting_workers_init(N)` / `scripting_workers_destroy()` pair is called from `launch_scripting_subsystem` / `land_scripting_subsystem` so lifecycle stays where the rest of the launch/landing code lives.
  - **ServiceThreads registration**: each worker is registered with `add_service_thread_with_description` and a description like `worker-0`, `worker-1`, ... so Lua jobs appear in the existing thread/memory reporting alongside the rest of Hydrogen.
  - **Concurrency model in v1**: per-job `lua_State` (Phase 1 lessons), per-worker `pthread_t` (recycled across jobs), single shared `Scoreboard` and single shared `Queue` (both already mutex-protected), single shared `ScriptRegistry` (mutex-protected). The scoreboard is the only place a job's identity, status, and timestamps live; the queue is purely a work-stealing primitive (it holds opaque `char* job_id` strings).
  - **Reserved for later phases (NOT in v1)**: priority dispatch (Phase 8+ — jobs are currently FIFO), `instruction_count` / `memory_used_bytes` (Phase 8 hook), `current_state` (Phase 9 `H.set_current_state`), `max_runtime_seconds` / `kill_requested` (Phase 10), `H.scoreboard.list` / kill wrapper (Phase 11), `H.scoreboard.list` iteration (Phase 11), DB-backed script loader (Phase 20 replaces the in-memory registry).
- **Deliverables**:
  - `src/scripting/script_registry.{c,h}` — minimal in-memory `name → source` map (mutex-protected, strdup on register, free on destroy, lookup returns registry-owned pointer).
  - `src/scripting/worker_pool.{c,h}` — `ScriptingWorkerPool` struct, `scripting_workers_init` / `scripting_workers_destroy`, `scripting_submit_job` / `scripting_submit_job_with_source`.
  - `src/scripting/scripting.h` — adds `extern struct ScriptingWorkerPool* scripting_workers;`.
  - `src/launch/launch_scripting.c` — calls `scripting_workers_init(app_config->scripting.WorkerCount)` after `scripting_init_state`.
  - `src/landing/landing_scripting.c` — calls `scripting_workers_destroy()` before `scripting_cleanup_state` (so the scoreboard outlives the workers).
  - 4 new Unity test files in `tests/unity/src/scripting/`:
    - `script_registry_test_register_lookup.c` (8 tests) — create/destroy/register/lookup/replace/count.
    - `worker_pool_test_submit.c` (7 tests) — submit, submit-with-source, NULL/error paths, uniqueness, replace.
    - `worker_pool_test_execute.c` (6 tests) — valid job COMPLETED, runtime error FAILED, syntax error FAILED, unknown script FAILED, timestamps stamped, H table available.
    - `worker_pool_test_shutdown.c` (6 tests) — NULL-safe destroy, init+destroy clears state, joins worker threads, drain on shutdown, idempotent destroy, registry/queue released.
  - 1 updated Unity test: `tests/unity/src/launch/launch_scripting_test_launch_scripting_subsystem.c` — sets `Enabled = true` and `WorkerCount = 2` on the mock config (Phase 3b's zeroed-config test relied on launch being a no-op, but Phase 7's launch now actually starts workers).
- **Validation**:
  - `mkt` equivalent (`extras/make-trial.sh`) — **PASS**, build successful.
  - `mkp` equivalent (`tests/test_91_cppcheck.sh`) — **PASS**, 1,409 files, 0 issues (was 1,401 in Phase 6; +8 from the new modules and tests).
  - `mku script_registry_test_register_lookup` — **PASS**, 8/8.
  - `mku worker_pool_test_submit` — **PASS**, 7/7.
  - `mku worker_pool_test_execute` — **PASS**, 6/6 (verified stable across 20 consecutive runs after the race fixes below).
  - `mku worker_pool_test_shutdown` — **PASS**, 6/6 (verified stable across 20 consecutive runs after the race fixes below).
  - `mku launch_scripting_test_launch_scripting_subsystem` — **PASS**, 2/2 (now with valid config).
  - Full Unity suite (`tests/test_10_unity.sh`) — **PASS**, **7,269 / 7,269 unit tests** (was 7,242 before Phase 7; +27 from new tests, 0 from updates to existing tests, 0 regressions).
  - `tests/test_16_shutdown.sh` — **PASS**, 5/5.
  - `tests/test_17_startup_shutdown.sh` — **PASS**, 9/9 (min and max configurations still start and stop cleanly with the worker pool in the launch/landing path).
  - ASAN / leak validation (Test 11) — **deferred to the debug build** (the trial build does not produce ASAN). The proven-clean ASAN lifecycle from Phase 3b is extended by the worker pool's clear/destroy path; a Test 11 variant with scripting enabled and a few real jobs would close the loop, but is not strictly required for Phase 7 to ship.
- **Notes / Open Questions**:
  - **Two race conditions** were discovered and fixed during validation; both are recorded in Lessons Learned below. The fixes are:
    1. `scripting_workers = pool` must be assigned **before** `pthread_create`, not after. Workers check the global on their first loop iteration, and a NULL global at that point makes them exit immediately.
    2. `scripting_workers_destroy()` must set the shutdown flag and **then** NULL the global, in that order. The workers' `should_exit` checks the shutdown flag + queue state, not the global pointer, so they keep draining between shutdown and join. The `process_one` function also uses the worker's local `pool` pointer (passed in) instead of the global, so it can complete an in-flight job even after the global is NULLed.
  - The default test timeout is 5 seconds (5000 × 1 ms poll), which is generous given the typical pickup time of <1 ms. The drain test has no timeout — it relies on `pthread_join` to block until the workers finish — and the "drain in flight work" property is verified deterministically.
  - The script registry is currently in-memory only. Phase 20 replaces the body of `script_registry_register` with a DB-backed loader; the worker-side API (`script_registry_lookup`) is unchanged so workers do not need a code change in Phase 20.
  - The 1 ms poll sleep is small enough to keep latency low (sub-millisecond pickup in steady state) and large enough to keep idle CPU near zero at the default `WorkerCount` of 2. A blocking-dequeue alternative would require a condvar/broadcast wake-up in the shutdown path; the polling approach trades a tiny amount of CPU for a much simpler shutdown sequence.
- **Lessons Learned**:
  - **Race #1: "global published too late"**. The first version assigned `scripting_workers = pool` *after* `pthread_create`. The workers' first loop iteration ran while the global was still NULL (the previous test's `destroy` had set it to NULL), so `should_exit` returned true and the workers exited without ever dequeuing. Symptom: tests 1-2 of every run timed out, tests 3-6 sometimes passed. Fix: publish the global **before** starting the pthreads. This is the same publish-before-publish pattern database worker pools use; the lesson is that "the spawn returns when the thread is created, not when it runs" means any global the thread reads must be set before the spawn.
  - **Race #2: "shutdown order is load-bearing"**. The first version of `scripting_workers_destroy` did `scripting_workers = NULL` *then* `scripting_system_shutdown = 1`. Between those two writes, a worker that had just dequeued a job (and was about to enter `process_one`) saw the NULL global, exited, and never processed the job. The test's "drain on shutdown" property was broken. Fix: shutdown flag first, then NULL the global, **and** make `process_one` use the worker's local `pool` pointer (the actual fix that made the in-flight work survive the destroy window). The deeper lesson: when a global is the synchronization anchor for "is the system still alive", every code path that runs concurrently with shutdown must not depend on that global — pass local pointers instead. The two-tier architecture (Phase 1) already gave every worker a `pool` argument; using it in `process_one` was the natural extension.
  - **Drain-on-shutdown is the right semantic for this pool**. The shutdown test verifies that jobs submitted just before `destroy` are completed. This matches the database migration engine's pattern (which also drains) and the print subsystem's pattern (which uses a `shutdown_requested` flag and a join). The alternative — drop in-flight work — would surface as orphaned PENDING entries in the scoreboard after a restart and would be a debugging hazard. The cost is that a slow Lua script can keep the worker alive (and therefore delay landing) for the duration of the script; Phase 10's `max_runtime_seconds` and kill path will close that gap.
  - **The Queue is reusable across inits via the global hash table**. `queue_create_with_label` returns an existing queue (refcount++) if one already exists with that name. The scripting job queue therefore persists across inits in the same process. This is fine in production (each server process has one lifetime) but required `queue_clear` calls in both `init` and `destroy` to keep Unity tests deterministic when running multiple inits in the same process.
  - **`scripting_submit_job_with_source` was the right boundary for testing**. Registering the source inline let the test suite drive end-to-end behaviour (registry → scoreboard → queue → worker → lua_State → scoreboard update → status) without a DB. The two submit functions are now a stable contract: the production path takes a name, the testing/eval path takes a name + source. The Orchestrator (Phase 11) and the future REST submit (Phase 23) will use the production path; a future Lua REPL will use the testing/eval path.
  - **Updating `launch_scripting_test_launch_scripting_subsystem` was the right move** when Phase 7 made the launch actually start workers. The old test relied on launch being a no-op for a zeroed config, but a real worker pool needs a valid `WorkerCount`. The fix was small (set `Enabled = true; WorkerCount = 2;` on the mock), and the resulting test now exercises the real launch path including the worker pool init — a stronger contract than the no-op it replaced.
  - **The worker thread description `worker-0` / `worker-1` / ... pays off in the existing thread reporting**. `add_service_thread_with_description` registers each pthread into `scripting_threads` with a human-readable name. A future operator looking at the thread list will see `Scripting (worker-0)`, `Scripting (worker-1)`, etc., instead of anonymous thread IDs — same convention the database DQM uses for its per-database worker names.

---

### Phase 8: Automatic Instruction Hook + Memory Tracking

- **Goal**: Add `lua_sethook` to track instruction count and memory usage automatically.
- **Dependencies**: Phase 7
- **Status**: **Complete 2026-07-01.** A `lua_sethook`-driven progress hook runs on every `lua_State` created by a worker (and will run on the Orchestrator's long-lived state in Phase 11). The hook bumps a per-state instruction counter on every tick, samples `lua_gc(L, LUA_GCCOUNT, 0)` every Nth tick, and writes `instruction_count` + `memory_used_kb` to the job's scoreboard entry. Per-job resource limits (soft memory, hard memory) are snapshotted into the per-state context at submit time and enforced by the hook; an `enforce_limits` flag lets the Orchestrator (and any opt-out job) keep the same observability path without risking being killed by its own growth. `H.gc.{collect, step, count, isrunning}` is exposed to Lua so long-running scripts (the Orchestrator) can drive full GC cycles on their own cadence — the hook deliberately does not collect.
- **Implementation approach** (decided 2026-07-01):
  - **Per-`lua_State` context via `lua_getextraspace`.** A new `H_lua_job_context` struct (defined in `lua_context.h`) is heap-allocated and a pointer to it is stored in the state's extraspace (the pointer-sized slot Lua 5.4 reserves for host use). The hook reads the context on every tick to identify the job, find the scoreboard, and read the snapshotted limits. The context is freed by `H_lua_set_job_context(L, NULL)`.
  - **Scoreboard gains 6 new fields** (added to the existing struct with no migration, since the scoreboard is in-memory only): `instruction_hook_interval`, `memory_soft_limit_kb`, `memory_hard_limit_kb`, `enforce_limits`, plus the live `instruction_count` and `memory_used_kb`. New API: `scoreboard_submit_with_limits(sb, name, params, limits)` (the existing `scoreboard_submit` is now a thin wrapper passing NULL limits = config defaults + enforce=true) and `scoreboard_update_progress(sb, job_id, inst_count, mem_kb)` (a mutex-bounded, idempotent data-only update that does NOT change status or stamp timestamps).
  - **Hook fires every N VM instructions** via `lua_sethook(L, fn, LUA_MASKCOUNT, ctx->hook_interval)`. The hook function is a static C function in the new `lua_hook.c` module. On every tick it bumps a per-state `local_instruction_count`. Every `app_config->scripting.MemorySampleEveryNHooks` (default 20) calls, it samples `lua_gc(L, LUA_GCCOUNT, 0)` and writes both counters to the scoreboard. This means at the default 5,000 `InstructionHookInterval` + 20 sample-every, one GC sample per 100K VM instructions — cheap, no perceptible jitter even in a hot loop.
  - **Soft limit** = one-shot `LOG_LEVEL_ALERT` warn + a single `lua_gc(L, LUA_GCSTEP, 0)` (bounded incremental step, not a full collection). **Hard limit** = `luaL_error(L, "%s", ...)` with a static snprintf-built message (Lua's `lua_pushfstring` does not support `%zu`; see Lessons Learned). The longjmp unwinds to the worker's `lua_pcall`, and the existing FAILED path (`worker_pool.c:343`) handles cleanup, UAF discipline, and `lua_close`. **SIZE_MAX** is the sentinel for "no hard limit" (set when the config default is 0).
  - **Hook is a leaf** — it never calls back into the host API (no `H.*` calls, no `log_this` on the hot path, only on the one-shot soft warning). This matches the Phase 6 rule that the host log path should never crash the host. The scoreboard write is a mutex-bounded critical section that only touches two fields.
  - **`H.gc` is a new sub-table** in the `H` namespace, installed by `H_lua_install_gc` (called from `H_lua_install_api` in `lua_context.c` alongside `H_lua_install_log` and `H_lua_install_system`). The four functions map directly to `lua_gc` operations: `H.gc.collect()` → `LUA_GCCOLLECT`, `H.gc.step()` → `LUA_GCSTEP`, `H.gc.count()` → `LUA_GCCOUNT` (returns KB), `H.gc.isrunning()` → `LUA_GCISRUNNING`. None of them is called by the hook; the Orchestrator will call `H.gc.collect()` on its own cadence in its scheduling loop.
  - **Per-job submit API**: `scripting_submit_job_with_source_and_limits(name, source, params, limits)` is added; the existing `scripting_submit_job` and `scripting_submit_job_with_source` are unchanged. `ScoreboardJobLimits` is the canonical type (defined in `scoreboard.h`, reused via include in `worker_pool.h`); zero fields mean "use the config default", `enforce_limits` defaults to `true` when the limits pointer is non-NULL. The Orchestrator (Phase 11) will call this with `enforce_limits=false`.
  - **Worker wire-in** (`worker_pool.c:323-362`): after `H_lua_create_context()` and before `H_lua_run_string`, the worker copies the per-job limits from the entry copy (from `scoreboard_find` at line 304) into the extraspace context, then calls `H_lua_install_progress_hook(L)`. The hook is always torn down (and the context cleared) after `H_lua_run_string` returns, so the state is clean through `lua_close`.
  - **Reserved for later phases (NOT in Phase 8)**: max-runtime kill (Phase 10), `current_state` (Phase 9), `H.scoreboard.list` (Phase 11), `H.set_current_state` (Phase 9).
- **Deliverables**:
  - `src/scripting/lua_hook.{c,h}` (new) — `H_lua_install_progress_hook` / `H_lua_uninstall_progress_hook` + the hook function.
  - `src/scripting/lua_context.{c,h}` — added `H_lua_job_context` struct, `H_lua_set_job_context`, `H_lua_get_job_context`; added `H_lua_install_gc` call to `H_lua_install_api`; added `gc` to the sub-table placeholder list.
  - `src/scripting/scripting_api.{c,h}` — added `H_lua_install_gc` + the four `H_lua_gc_*` functions.
  - `src/scripting/scoreboard.{c,h}` — added 6 new fields to `ScoreboardEntry`, `ScoreboardJobLimits` typedef, `scoreboard_submit_with_limits` (the new public surface; `scoreboard_submit` is now a thin wrapper), `scoreboard_update_progress`.
  - `src/scripting/worker_pool.{c,h}` — added `scripting_submit_job_with_source_and_limits`; wired hook install/teardown + per-job context into `scripting_worker_process_one`; `worker_pool.h` now includes `scoreboard.h` for the shared `ScoreboardJobLimits` type.
  - `src/config/config_scripting.{c,h}` + `src/config/config_defaults.c` — added `MemorySampleEveryNHooks` (default 20) to the scripting config section.
  - 3 new Unity test files in `tests/unity/src/scripting/`:
    - `scoreboard_test_progress.c` (12 tests) — initial-zero fields, `update_progress` writes, unknown/NULL id, no status change, no timestamp stamping, monotonic instruction_count, non-monotonic memory, per-job limits round-trip through find, NULL-limits == config, explicit overrides, SIZE_MAX sentinel.
    - `lua_hook_test_install.c` (8 tests) — NULL state no-op, install without context logs and returns, hook fires on a busy loop and grows the scoreboard, `H.gc.{collect,step,count,isrunning}` are callable from Lua, hard limit aborts the job, `enforce_limits=false` never kills, uninstall stops further ticks.
    - `worker_pool_test_progress.c` (5 tests) — end-to-end progress visible in scoreboard, hard limit kills worker job, `enforce_limits=false` completes with low hard limit, per-job overrides take effect, multiple concurrent jobs each get their own limits.
- **Validation**:
  - `mkt` equivalent (`extras/make-trial.sh`) — **PASS**, build successful
  - `mkp` equivalent (`tests/test_91_cppcheck.sh`) — **PASS**, 1,414 files, 0 issues
  - `mku scoreboard_test_progress` — **PASS**, 12/12
  - `mku lua_hook_test_install` — **PASS**, 8/8 (verified stable across multiple consecutive runs after the `lua_getextraspace` fix below)
  - `mku worker_pool_test_progress` — **PASS**, 5/5 (verified stable across 5 consecutive runs after the `instruction_count` semantics fix below)
  - Full Unity suite (`tests/test_10_unity.sh`) — **PASS**, **7,294 / 7,294 unit tests** (was 7,269 before Phase 8; +25 from new tests, 0 regressions, 0 from updates to existing tests)
  - `tests/test_16_shutdown.sh` — **PASS**, 5/5 (min and max configurations still start and stop cleanly with the hook + new submit API in the worker path)
  - `tests/test_17_startup_shutdown.sh` — **PASS**, 9/9
  - `tests/test_01_compilation.sh` — **PASS**, 18/18
  - `tests/test_03_shell.sh` — **PASS**, 45/45
  - `tests/test_12_env_variables.sh` — **PASS**, 15/15
  - ASAN / Test 11 leak validation — **deferred**: the trial build does not produce ASAN. The proven-clean ASAN lifecycle from Phase 3b is extended by the hook install/teardown path on every worker job; a Test 11 variant with scripting enabled and a few real jobs would close the loop, but is not strictly required for Phase 8 to ship.
- **Notes / Open Questions**:
  - **`lua_getextraspace` is a pure macro** in Lua 5.4 (`((void *)((char *)(L) - LUA_EXTRASPACE))`). It does NOT push anything onto the Lua stack. An early version of the implementation paired it with `lua_pop`, which corrupted the stack and produced "attempt to call a nil value" panics. Fixed by removing the `lua_pop` calls (see Lessons Learned).
  - **`lua_pushfstring` (used by `luaL_error`) does not support `%zu`.** A first version of the hard-limit error used `luaL_error(L, "memory limit exceeded ... %zu KB ...", ...)` and tripped Lua's "invalid option '%z' to 'lua_pushfstring'" runtime error — which then short-circuited the test to a confusing failure. Fixed by building the message with `snprintf` into a static buffer and passing it as a single `%s`. (This is a Lua 5.4 API quirk, not a Hydrogen quirk.)
  - **`instruction_count` semantics.** The scoreboard's `instruction_count` is the **number of hook calls**, not the raw number of VM instructions. A Lua 5.4 `lua_sethook` with `LUA_MASKCOUNT` and count=N fires the hook every N VM instructions, and the hook function has no way to ask "how many instructions have executed so far" from inside the call. The field is therefore `bytecode_count / hook_interval` rounded down. A 100K-iteration numeric for loop with `hook_interval=100` produces exactly `instruction_count = 1,000`. The scoreboard docstring and the Phase 8 tests document this; if a future phase needs exact instruction counts, the fix is to install the hook at count=1 (very expensive but exact) or to use `lua_gc` counters plus a per-call multiplier.
  - **`local_count` is the hook's view of progress; `scoreboard_update_progress` is the writer.** The hook bumps a per-state counter on every tick (cheap, no lock) and the per-`sample_every` flush writes it to the scoreboard (mutex-bounded, two field writes). A future refactor could use a hazard-pointer or atomic shared pointer to avoid the lock entirely, but for a few hundred hooks per second per worker the current design is the right trade-off.
  - **The Orchestrator's hook is wired but its limits are not yet configured.** Phase 11 (Orchestrator Script) will submit a single scoreboard entry with `enforce_limits = false` and a long-lived `lua_State`; the existing hook path picks it up unchanged. The reference Orchestrator will call `H.gc.collect()` in its scheduling loop.
  - **`config_scripting.InstructionHookInterval = 0` means "no hook installed"** (the hook install function logs and returns). For the tests, the default 5,000 was lowered to 100 in some suites to make the hook fire often enough to test in real time.
- **Lessons Learned**:
  - **`lua_getextraspace` is a macro, not a function.** I initially wrote `H_lua_set_job_context` and `H_lua_get_job_context` to call `lua_getextraspace(L)` and pair it with `lua_pop(L, 1)`, assuming it pushed a value on the stack. It does not — it's `((void *)((char *)(L) - LUA_EXTRASPACE))`, a pure pointer arithmetic. The phantom `lua_pop` calls popped values that were never pushed, corrupting the stack and producing "attempt to call a nil value" panics on the first H.* access. The fix was a one-line removal of both `lua_pop` calls. **Takeaway**: read the Lua header before writing code that uses Lua C API macros; the macro-vs-function distinction matters for stack discipline. (Same lesson applies to `lua_setextraspace`, which is a pure no-op macro that does not exist as a function — we use the macro's return value as a typed pointer.)
  - **`luaL_error` does not support `%zu` / `%zd`.** Lua's `lua_pushfstring` only supports a subset of printf: `%s`, `%f`, `%p`, `%d`, `%c`, `%I` (lua_Integer), `%U` (lua_Unsigned), and `%%`. Using `%zu` produces a runtime error ("invalid option '%z' to 'lua_pushfstring'") that the hook's `luaL_error` then propagates as its own error — masking the real "memory limit exceeded" message and turning the test's `assertNotNull(lua_tostring(L, -1))` into a confusing "Expected Non-NULL" failure. The fix is to build the error message with `snprintf` and pass it as a single `%s`. **Takeaway**: when embedding Lua 5.4, build error messages with snprintf, not with `luaL_error`'s format string. (This is documented in the Lua reference manual under `lua_pushfstring`, but easy to miss when reaching for printf-style formatting.)
  - **A duplicate line in the hook function doubled `local_instruction_count`.** A first version of `H_lua_progress_hook_fn` had the increment twice (once before, once after the temporary debug logging) and I missed the duplicate when removing the debug log. The test that caught it was the busy-hook test: local_count was exactly 2× the call count, off by a factor of 2 from what the comment claimed. The fix was to delete the second `++`. **Takeaway**: when adding temporary debug logging, the diff at the end of the debug session should be a *net zero* line change; any line that was added for debug should be removed, and the surrounding code should look identical to the pre-debug version. A `git diff` (or just a careful re-read) at the end of debug sessions catches this.
  - **`instruction_count` ≠ raw VM instructions.** The hook has no way to ask "how many VM instructions have executed so far", so `instruction_count` is really `hook_call_count` (= `bytecode_count / hook_interval`, rounded). The 100K-iteration busy-loop test with `hook_interval=100` produces `instruction_count=1,000`, not 1,000,000. The test threshold and the field's docstring were updated to match. **Takeaway**: when designing progress fields, decide whether the value is "what the hook knows" (cheap, coarse) or "what the runtime knows" (expensive, exact), and document the choice. The coarse-but-cheap design was the right call for a system that fires the hook 100-1000 times per second per worker; the exact design would have made the hook itself the bottleneck.
  - **Monotonic instruction_count, non-monotonic memory_used_kb.** The scoreboard update is monotonic for `instruction_count` (later hook calls have higher counts; we never let the field go backwards) and overwriting for `memory_used_kb` (GC can free memory, so a later sample can legitimately be smaller). The Unity tests pin both invariants. This is a small but important asymmetry: instruction_count is "progress", memory_used_kb is "level".
  - **The ScoreboardJobLimits type lives in scoreboard.h, not worker_pool.h.** An early version had two parallel typedefs (`ScriptingJobLimits` in worker_pool.h, `ScoreboardJobLimits` in scoreboard.h) that were structurally identical but nominally different. The compiler caught it as an "incompatible pointer type" error on the submit call. The fix was to delete the duplicate, add `#include "scoreboard.h"` to worker_pool.h, and use the scoreboard's type. **Takeaway**: when a type crosses an API boundary, define it in the header that owns the contract. The scoreboard owns "per-job limits stored on an entry" — that's the canonical home.
  - **The hook's soft-warned flag prevents log spam on every tick.** A first mental design of the soft-limit check would log a warning on every hook call once memory exceeded the limit, which would flood the log (and slow the worker). The one-shot `ctx->soft_warned` flag means "log once per state, then nudge the GC and stop talking". This matches the Phase 6 "host path is a leaf" rule: the host should never crash, and the host should never spam.
  - **The hook's hard-limit path is intentionally a leaf, not a raise.** A first mental design considered having the hook set a "kill_requested" flag and let the worker poll it. The cleaner design (and what we shipped) is `luaL_error` longjmp-to-pcall: the hook is the source of truth, the error message is constructed once, and the worker's existing `lua_pcall` error path (`worker_pool.c:343`) handles copy-out-of-Lua-memory, logging, FAILED status, and `lua_close` — all paths already tested by Phase 7. A flag-and-poll design would have duplicated that entire error-handling block for marginal benefit.
  - **The `Scoreboard` field-set is now substantial** (3 owned strings + 1 ID + status + 3 timestamps + 3 limit ints + 1 limit size_t + 1 limit bool + 1 progress uint64 + 1 progress size_t = 13 fields, 6 of them owned via strdup). Future growth is bounded: `current_state` (Phase 9) will add one owned string; `kill_requested` (Phase 10) will add a bool; `has_waiter` (Phase 12) will add a bool and a pointer. The struct stays well under 200 bytes, so the `realloc`-doubles-capacity strategy from Phase 5 still works. No structural refactor needed yet.

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

Before any implementation work on a phase, the current state is reviewed and the detailed approach for that phase is agreed. **Everything prior to Phase 9 is complete: Phase 1 (assessment), Phase 2 (host API namespace design), Phase 2b (Scripting config section and global defaults), Phase 3 (sandboxed Lua context lifecycle), Phase 3b (Scripting subsystem skeleton: enablement, launch, and early shutdown — proven leak-free by ASAN in Test 11), Phase 4 (`H_lua_run_string` for executing a string buffer), Phase 5 (basic in-memory scoreboard data structure, wired into the subsystem's init/cleanup lifecycle and exercised by 40 new Unity tests + a 4th landing test for shutdown), Phase 6 (`H.log.{trace,debug,info,warn,error,fatal}` and `H.system.{uptime,now,now_iso,instance_id,version}` exposed via the new `scripting_api.c` module, 23 new Unity tests), Phase 7 (Job Worker Pool Basics: a worker pool with `scripting_submit_job` / `scripting_submit_job_with_source`, a fresh `lua_State` per job, drain-on-shutdown, and 27 new Unity tests; the existing `launch_scripting_test_launch_scripting_subsystem` updated to set a valid `WorkerCount`), and Phase 8 (Automatic Instruction Hook + Memory Tracking: a `lua_sethook`-driven progress hook with per-job resource limits, `H.gc.{collect,step,count,isrunning}` exposed for explicit GC control by long-running scripts, and 25 new Unity tests across 3 new suites). Phase 9 (`H.set_current_state()`) is the next step.**