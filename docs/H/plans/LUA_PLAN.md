# LUA_PLAN.md

**Hydrogen Lua Scripting & Execution Environment – Implementation Roadmap**

**Status**: Draft – Actively evolving  
**Last Updated**: 2026-06-30  
**Owner**: Andrew + Grok

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
- Good observability and logging integration (VictoriaLogs + VictoriaMetrics)
- Safe handling of state transitions across multiple server instances
- Automatic monitoring and progress reporting with minimal boilerplate from Lua scripts

---

## Implementation Philosophy

This plan follows a few important principles:

1. **Small, Focused Phases**  
   We prefer many small, simple phases over fewer large ones. Each phase should deliver something coherent and testable. We expect the total number of phases to be high.

2. **Interactive Development**  
   Implementation is **not** "write the plan then execute blindly." For each phase we will:
   - Assess the current state of the relevant code
   - Propose a concrete implementation approach for that phase
   - Review and confirm the approach together before significant work begins
   - After implementation, review validation results together
   - Explicitly capture lessons learned (technical and process) with input from both sides

3. **Validation Required**  
   No phase is considered complete until its validation criteria are met and reviewed.

4. **Flexibility & Learning**  
   We expect to learn things we haven't anticipated. Phases may be split (e.g. 7a / 7b), reordered, or adjusted. The plan is a living document.

5. **Lessons Learned**  
   After each validated phase we will record what was learned so future phases benefit from that knowledge.

---

## Phase Structure

Each phase follows this template:

**Phase N: Short Title**

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
- **Expectation**: Review existing Lua usage (migrations, any execution paths), identify entry points, libraries linked, and how `lua_State` is currently managed.
- **Deliverables**: Documented findings and list of relevant files.
- **Validation**: Shared understanding confirmed between both parties.
- **Notes / Open Questions**:
- **Lessons Learned**:

---

### Phase 2: Define `H` Namespace and Calling Conventions

- **Goal**: Agree on the high-level shape and naming of the host API exposed to Lua.
- **Dependencies**: Phase 1
- **Expectation**: Define `H.db`, `H.http`, `H.log`, `H.system`, `H.user`, `H.scoreboard`, `H.set_current_state`, etc. Decide on consistent patterns (e.g. `H.db.query(sql, params)` vs object style). Provide example usage for common operations.
- **Deliverables**: Written namespace proposal + example code snippets.
- **Validation**: Agreement on the namespace design before implementation.
- **Notes / Open Questions**:
- **Lessons Learned**:

---

### Phase 3: Basic Sandboxed Lua Context Creation

- **Goal**: Create and destroy sandboxed `lua_State` instances safely.
- **Dependencies**: Phase 1
- **Expectation**:
  ```c
  lua_State* H_lua_create_context(void);
  void H_lua_destroy_context(lua_State* L);
  ```
  Basic sandbox: limited standard libraries, no `io`, controlled `os`, etc.
- **Deliverables**: Context creation/destruction functions + basic sandbox setup.
- **Validation**: Unity test creates context, runs minimal script, destroys cleanly.
- **Notes / Open Questions**:
- **Lessons Learned**:

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
- **Expectation**: Small pool of pthreads. Worker dequeues from scoreboard, creates context, runs script, updates status on completion/failure.
- **Deliverables**: Basic worker pool + job execution loop.
- **Validation**: End-to-end: create scoreboard entry → worker runs it → status becomes completed.
- **Notes / Open Questions**:
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

### Phase 11: Master Poller Script Skeleton

- **Goal**: Create the initial long-running master polling Lua script.
- **Dependencies**: Phase 7, Phase 9
- **Expectation**: `poller.lua` runs in its own context, has its own timing loops, can read scoreboard, and can create new scoreboard entries for work.
- **Deliverables**: Basic `poller.lua` + ability to launch it as a special long-running job.
- **Validation**: Poller successfully creates and observes jobs.
- **Notes / Open Questions**:
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

### Phase 13: Basic `H.db.query` Exposure

- **Goal**: Expose synchronous database query capability to Lua.
- **Dependencies**: Phase 6
- **Expectation**:
  ```lua
  local rows = H.db.query("SELECT * FROM foo WHERE id = ?", {123})
  ```
  Uses existing database abstraction layer.
- **Deliverables**: `H.db.query` implementation with parameter binding.
- **Validation**: Lua script can run queries and receive results as tables.
- **Notes / Open Questions**:
- **Lessons Learned**:

---

### Phase 14: `H.db.transaction` Support

- **Goal**: Allow Lua scripts to run multiple queries inside a transaction.
- **Dependencies**: Phase 13
- **Expectation**:
  ```lua
  H.db.transaction(function(tx)
      tx.query(...)
      tx.query(...)
  end)
  ```
- **Deliverables**: Transaction wrapper that manages begin/commit/rollback.
- **Validation**: Transaction test (success path + error/rollback path).
- **Notes / Open Questions**:
- **Lessons Learned**:

---

### Phase 15: Async-style DB Query with Handle

- **Goal**: Allow issuing a query without immediately blocking, returning a handle.
- **Dependencies**: Phase 13
- **Expectation**:
  ```lua
  local handle = H.db.query_async("SELECT ...", params)
  -- later
  local result = H.db.wait_for_result(handle)  -- or check status + get
  ```
- **Deliverables**: Async query issuance + handle + wait mechanism.
- **Validation**: Script can issue query, do other work, then wait for result.
- **Notes / Open Questions**:
- **Lessons Learned**:

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

### Phase 18: `H.llm.call` Convenience Wrapper

- **Goal**: Provide a clean way to call registered LLMs from Lua.
- **Dependencies**: Phase 17
- **Expectation**:
  ```lua
  local response = H.llm.call({
      model = "grok-light",
      prompt = "Is this spam?",
      max_tokens = 200
  })
  ```
  Uses existing LLM registry + direct (non-streaming) path.
- **Deliverables**: `H.llm.call` implementation.
- **Validation**: Lua can call different models and get structured responses.
- **Notes / Open Questions**:
- **Lessons Learned**:

---

### Phase 19: Message Sending (`H.mail` / Notifications)

- **Goal**: Expose ability to send email (and later SMS) from Lua.
- **Dependencies**: Phase 6
- **Expectation**:
  ```lua
  H.mail.send({
      to = "...",
      subject = "...",
      body = "...",
      template = "report_ready"
  })
  ```
- **Deliverables**: Basic message sending host API.
- **Validation**: Lua script can trigger an email send.
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
- **Expectation**: Simple stage script that claims work via transaction, does something, sends a notification, and updates state.
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

## Summary

This plan currently contains **28 focused phases**. More phases can (and likely will) be added as we proceed and discover additional needs. The structure is designed to support the interactive, learning-oriented development process we agreed upon.

We are now ready to begin **Phase 1** (assessment) whenever you are. Before doing any implementation work on a phase, we will review the current state and agree on the detailed approach for that phase. 

Ready to start with Phase 1 assessment? Or would you like any adjustments to the plan first?