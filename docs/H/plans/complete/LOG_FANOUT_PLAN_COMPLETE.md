# LOG FANOUT PLAN

## Context

`src/logging/log_queue_manager.c` was effectively dead code: it described a
SystemLog queue consumer thread (`log_queue_manager`) that was **never spawned**,
so every entry `log_this()` enqueued onto the `SystemLog` queue had no reader and
was silently dropped. Console output during steady-state operation was instead
produced inline in `logging.c` (`console_log`), and the two halves of the design
(queued entries vs. inline console) were never reconciled.

A transition to a real fan-out consumer was started and then dropped midway:

- `log_queue_manager.c` / `.h` were **deleted** and replaced by
  `src/logging/log_fanout.c` / `.h`.
- `log_fanout.c` implements a proper consumer thread (`log_fanout_thread`) that
  drains `SystemLog` and fans each entry out to **console**, **file**, and
  **mailrelay** (via `mailrelay_send_direct`).
- `init_log_fanout()` is called from `launch.c` (line 392), so the thread is
  actually spawned now.

But the work was never finished and the project no longer builds:

1. **Build gate failure.** The trial build (`mkt`) runs a *static-function gate*
   that rejects any **new** `static` callable function in `src/` (see
   `/docs/H/INSTRUCTIONS.md`, "NO `static` FUNCTIONS" rule). `log_fanout.c`
   declares five callable helpers as `static`:
   `dest_enabled`, `fanout_entry`, `fanout_priority_label`, `fanout_to_mailrelay`,
   `log_fanout_thread`. The gate aborts before compilation.
2. **Orphaned Unity test.** `tests/unity/src/logging/log_queue_manager_test_should_log_to_console.c`
   referenced the deleted API (`should_log_to_console/file/database/notify`,
   `process_log_message`, `init_file_logging`, `close_file_logging`,
   `cleanup_log_queue_manager`) and was deleted, leaving no tests for the new
   module.
3. **Shutdown not wired.** `shutdown_log_fanout()` exists but is never called
   from the logging landing path (`landing_logging.c`), so the consumer thread is
   never joined on shutdown.
4. **Stale references.** `landing_logging.c` still joins a `log_thread` that no
   longer exists (the consumer now lives entirely inside `log_fanout.c`).

### Decision summary (from clarifying questions)

- **Scope:** Design a *coherent* fan-out framework (not just de-static to pass the
  gate). The five helpers become public, testable API. Console/file/mailrelay are
  delivered as concrete destinations now; the design leaves a clean seam for a
  future database destination.
- **Tests:** Drop the old `should_log_to_*` API and write fresh Unity tests
  against the new non-static API.
- **Wiring:** Wire `shutdown_log_fanout()` into the logging landing path; fix the
  stale `log_thread` join.

## Goals

1. Make `mkt` (and `mka`) pass again.
2. Reconcile the dead-code history into a working, tested fan-out consumer.
3. Expose the fan-out helpers as non-static, declared-in-header functions so the
   Unity suite can cover them (per the project's static-function rule).
4. Wire the full lifecycle: launch spawns the consumer, landing joins it cleanly.
5. Provide initial Unity coverage for the fan-out logic.

## Current State Inventory

### Files

| File | Status | Notes |
| ---- | ------ | ----- |
| `src/logging/log_queue_manager.c` / `.h` | **Deleted** | Old dead-code consumer. |
| `src/logging/log_fanout.c` / `.h` | **New/In-progress** | Replaces it; has 5 static helpers that break the build. |
| `src/logging/logging.c` | Existing | `log_this()` enqueues JSON onto `SystemLog`; suppresses inline console when enqueue succeeds. |
| `src/launch/launch.c` | Existing | Calls `init_log_fanout()` at line 392. |
| `src/launch/launch_logging.c` | Existing | `launch_logging_subsystem()` does registration only. |
| `src/landing/landing_logging.c` | Existing | Joins stale `log_thread`; never calls `shutdown_log_fanout()`. |
| `tests/unity/src/logging/log_queue_manager_test_should_log_to_console.c` | **Deleted** | Referenced removed API. |
| `tests/unity/src/logging/*` | Existing | VictoriaLogs + basic logging tests; no fan-out tests. |

### Public symbols today (`log_fanout.h`)

- `bool init_log_fanout(void);`
- `void shutdown_log_fanout(void);`

### Internal-but-should-be-public symbols (`log_fanout.c`, currently `static`)

- `fanout_priority_label(int priority)` -> `const char*`
- `dest_enabled(const LoggingDestConfig* dest, int priority)` -> `bool`
- `format_entry(char* out, size_t out_size, const char* subsystem, const char* details, int priority)` -> `void`
- `fanout_to_mailrelay(const char* subsystem, const char* details, int priority)` -> `void`
- `fanout_entry(const char* raw)` -> `void`
- `log_fanout_thread(void* arg)` -> `void*` (thread entry; kept non-static for testability/consistency)

### Dependencies used

- `queue_*` (queue.h): `queue_create`, `queue_find`, `queue_enqueue`,
  `queue_release`, `queue_size`, `queue_dequeue`, `queue_dequeue_nonblocking`.
- `config_logging.h`: `LoggingDestConfig` (`.enabled`, `.default_level`).
- `mailrelay.h`: `mailrelay_send_direct`, `MailRelaySendDirectRequest`.
- Globals: `app_config`, `log_queue_shutdown`, `terminate_cond`, `terminate_mutex`.

## Design

### 1. Public fan-out API (`log_fanout.h`)

Promote the helpers to the public (non-static) surface. Group them under a
clear "internal/testable helpers" comment block in the header, mirroring the
pattern already used in `mailrelay.h` (which exposes producer helpers "not part
of the stable public API" but non-static for Unity).

```c
/* ---- Init / lifecycle ---- */
bool init_log_fanout(void);
void shutdown_log_fanout(void);

/* ---- Testable internal helpers (non-static so Unity can call them) ---- */
const char* fanout_priority_label(int priority);
bool        dest_enabled(const LoggingDestConfig* dest, int priority);
void        format_entry(char* out, size_t out_size,
                         const char* subsystem, const char* details, int priority);
void        fanout_to_mailrelay(const char* subsystem, const char* details, int priority);
void        fanout_entry(const char* raw);
void*       log_fanout_thread(void* arg);
```

File-scope **state** (`g_log_queue`, `g_log_fanout_thread`, `g_log_fanout_running`)
stays `static` — state is explicitly allowed by the rule; only callable
functions must be non-static.

### 2. `log_fanout.c` changes

- Remove `static` from the six helpers above.
- Keep the `mailrelay_message.h` include only if actually used; currently
  `fanout_to_mailrelay` builds a `MailRelaySendDirectRequest` directly and does
  **not** use `MailRelayMessage`, so drop the unused include to keep cppcheck
  clean.
- No behavioral change to the fan-out logic itself — it is correct as written
  (console = stdout, file = append to `app_config->server.log_file`, mailrelay =
  async `mailrelay_send_direct` when `LogNotify` + notify destination enabled).
- Store the `Queue*` returned by `init_log_fanout`'s `queue_create` in
  `g_log_queue` (already done) and reference it from the thread; the thread
  currently also calls `queue_find("SystemLog")` via `log_this`'s enqueue
  path — keep the module self-contained by having `log_fanout_thread` use
  `g_log_queue` consistently.

### 3. Lifecycle wiring

- **Launch** (`launch_logging.c`, `launch_logging_subsystem`): after the
  subsystem is marked `SUBSYSTEM_RUNNING`, call `init_log_fanout()`. This keeps
  the fan-out consumer tied to the logging subsystem's lifecycle and guarantees
  the queue consumer exists once logging is "running" (matching the
  `use_startup_filtering` check in `logging.c` that tests
  `is_subsystem_running_by_name(SR_LOGGING)`).
  - Guard against double-init with `g_log_fanout_running` (already present).
  - On failure, log an error but still return success for the subsystem so we
    don't block the whole launch (queue is best-effort: if it fails, `log_this`
    falls back to inline console anyway).
  - Remove the standalone `init_log_fanout()` call from `launch.c:392` to avoid
    a second, untracked spawn.

- **Landing** (`landing_logging.c`, `land_logging_subsystem`):
  - Replace the stale `log_thread` join (that variable no longer exists) with a
    call to `shutdown_log_fanout()`, which sets `log_queue_shutdown = 1`,
    broadcasts `terminate_cond`, and joins the consumer thread.
  - Order: signal shutdown, join consumer, *then* continue with config/buffer
    cleanup (so in-flight entries can still be written before buffers are freed).
  - Include `<src/logging/log_fanout.h>` in `landing_logging.c`.

### 4. Build gate

The five (six) helpers become non-static + declared, so the static-function gate
passes without regenerating `tests/.static-baseline.txt`. We do **not** widen the
baseline — these are genuinely testable callables, so they belong in the normal
(non-static) namespace per the rule.

### 5. Unity tests (new)

Create `tests/unity/src/logging/log_fanout_test_<fn>.c` files mirroring the
`<source>_test_<function>.c` convention, one per helper, plus an integration
test for `fanout_entry` exercising the JSON parse + destination gating:

- `log_fanout_test_fanout_priority_label.c` — every `LOG_LEVEL_*` maps to the
  right label; default/unknown -> `"STATE"`.
- `log_fanout_test_dest_enabled.c` — NULL dest, disabled dest, below-level drop,
  at/above-level pass; uses `LoggingDestConfig` fixtures (no global config needed).
- `log_fanout_test_format_entry.c` — output contains the formatted timestamp,
  priority bracket, subsystem bracket, and details; verify width padding.
- `log_fanout_test_fanout_to_mailrelay.c` — when `app_config` NULL or
  mail_relay disabled / no admin recipients, returns early (no send); when enabled
  with recipients, calls `mailrelay_send_direct`. Use the `mailrelay_send_direct`
  test seam (`mailrelay_send_direct_set_fn`) to capture the request without
  touching SMTP.
- `log_fanout_test_fanout_entry.c` — feed a JSON string with `LogConsole`/`LogFile`/
  `LogNotify` flags; assert console/file/mailrelay routing via mocks/seams; feed
  invalid JSON -> early return; feed NULL/empty -> early return.
- `log_fanout_test_init_shutdown.c` — `init_log_fanout()` creates the queue and
  spawns (then `shutdown_log_fanout()` joins); safe to call shutdown before init.

Note on mocking: `USE_MOCK_LOGGING` is global, so `log_this` is already stubbed
in tests. `mailrelay_send_direct` is **not** globally mocked — use its
`mailrelay_send_direct_set_fn` test seam (already provided) to intercept.

## Implementation Steps

1. Edit `src/logging/log_fanout.h`: add the six non-static helper declarations
   under a "testable internal helpers" block.
2. Edit `src/logging/log_fanout.c`:
   - Drop `static` from the six helpers.
   - Remove the unused `<src/mailrelay/mailrelay_message.h>` include.
   - (Optional) have `log_fanout_thread` use `g_log_queue` directly for
     consistency.
3. Edit `src/launch/launch_logging.c`: `#include <src/logging/log_fanout.h>` and
   call `init_log_fanout()` after marking the subsystem running.
4. Edit `src/launch/launch.c`: remove the now-redundant `init_log_fanout()` call
   at line 392 (and its header include if no longer used there).
5. Edit `src/landing/landing_logging.c`: replace the stale `log_thread` join with
   `shutdown_log_fanout()` (after signaling), include the fanout header.
6. Run `mkt` — confirm the static gate passes and the project compiles.
7. Add the six Unity test files under `tests/unity/src/logging/`.
8. Run `mku` for each new test; iterate until green.
9. Run `mkp` (cppcheck) and `mks` if any scripts touched.
10. Run `mka` to confirm all targets build.

## Verification

- `mkt` succeeds (static gate + compile).
- `mka` succeeds (all targets).
- Each new Unity test passes via `mku <base>`.
- `mkp` reports no new cppcheck issues in the touched files.
- `shutdown_log_fanout()` is exercised (landing path) — covered by blackbox
  startup/shutdown tests (`test_16_shutdown`, `test_17_startup_shutdown`).

## Risks / Open Questions

- **Database destination:** `LoggingDestConfig database` exists in config but the
  new `fanout_entry` does not implement DB writes (the old code only had a
  `// TODO`). This plan leaves DB as a documented future destination; no DB write
  is added now to avoid scope creep. Flagged here for later.
- **Double console output:** `log_this` already suppresses inline console when the
  enqueue succeeds, so the consumer is the steady-state console sink. Verified in
  `logging.c:558-583`. Keep as-is.
- **Thread handle type:** `g_log_fanout_thread` is `pthread_t`; `shutdown_log_fanout`
  joins it. The stale `log_thread` (old `pthread_t` extern) is removed from
  landing to avoid referencing a non-existent symbol.

## Working Log

- 2026-07-18: Review complete. Confirmed build blocker is the static-function gate
  on `log_fanout.c`'s five helpers. Old test deleted; old API gone. Plan written
  and approved (full framework scope, new tests, wire shutdown). Implementation
  pending.
- 2026-07-18: Implementation complete.
  - `log_fanout.h`: promoted `fanout_priority_label`, `dest_enabled`,
    `format_entry`, `fanout_to_mailrelay`, `fanout_entry`, `log_fanout_thread`
    to non-static, header-declared helpers. `shutdown_log_fanout()` now returns
    `bool`.
  - `log_fanout.c`: removed `static` from the six helpers; dropped unused
    `mailrelay_message.h` include; added `(void)subsystem`; simplified the
    recipient-allocation block (removed always-true guard).
  - `launch.c`: kept the single `init_log_fanout()` call (correct ordering after
    queue system + config + mailrelay launch).
  - `landing_logging.c`: replaced the stale `log_thread` join with
    `shutdown_log_fanout()`; removed the now-unused `log_thread` extern and
    `remove_service_thread` call.
  - Added 6 Unity tests under `tests/unity/src/logging/`
    (`log_fanout_test_*`). All pass.
  - Verification: `mkt`, `mka` (18 targets), `mkp` (cppcheck clean), and all 6
    `mku` tests green. Database destination intentionally left as a documented
    future seam (config field exists; no writer implemented).
