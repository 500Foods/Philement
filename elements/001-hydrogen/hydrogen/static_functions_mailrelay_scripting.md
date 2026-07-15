# Static functions cleanup — hydrogen/src (mailrelay + scripting)

Scope: `elements/001-hydrogen/hydrogen/src/mailrelay` and `.../src/scripting`.
Excludes `examples/` and the benign `static` in `hydrogen.h`.

This doc tracks the work to remove `static` from internal helper functions so
they can be called directly from Unity Framework unit tests. The goal is
**testability**, not just removing the keyword: every converted function must
also get a visible declaration in a header.

---

## Status

| Module group | `static` functions | Status |
|--------------|-------------------|--------|
| **mailrelay** (`src/mailrelay`) | 146 (in 12 files) | ✅ **COMPLETE** |
| **scripting** (`src/scripting`) | 85 (in ~16 files) | ⏳ NEXT BATCH (see below) |

The mailrelay conversion is done and builds clean under the `Unity`,
`Regular`, and `Coverage` build configs, and all existing mailrelay Unity
tests pass.

---

## mailrelay — COMPLETE

### What was done

- All 146 `static` function declarations across the 12 mailrelay `.c` files
  were converted to non-`static`.
- A matching declaration was added to the appropriate header for each
  function (see mapping below), marked with a "not part of the stable public
  API / exposed for Unity tests" comment block.
- Remaining `static` in `src/mailrelay` is **intentional and correct**:
  - `mailrelay_test_seams.c` — the two `static` callbacks are already behind
    the seam pattern and are not called by tests.
  - File-scope globals / string literals (test-seam state vars, the
    `MAILRELAY_EVENT_*_SCRIPT` built-in Lua sources, etc.) — module-private
    by design.
- **No functions were renamed.** There were 0 true cross-file name collisions
  (every repeated name was a same-file prototype+definition pair; see
  Collision detail at the bottom).

### Header → function mapping (where the declarations went)

| Header | Functions exposed |
|--------|-------------------|
| `mailrelay_internal.h` | `recovery_callback`, `insert_callback`, `mailrelay_persist_message` |
| `mailrelay_debounce.h` | `create_entry`, `mailrelay_debounce_thread` |
| `mailrelay_events.h` | `mailrelay_event_check_rate_limit`, `mailrelay_event_free_rate_limits`, `mailrelay_event_push_event_table`, `mailrelay_event_read_string_array`, `mailrelay_event_read_string`, `mailrelay_event_read_int`, `mailrelay_event_read_params`, `mailrelay_event_free_string_array`, `mailrelay_event_dispatch_request`, `mailrelay_event_run_handler`, `mailrelay_event_find_rule`, `mailrelay_event_resolve_source` |
| `mailrelay_message.h` | `add_recipient` |
| `mailrelay_otp.h` | `otp_insert_callback`, `otp_resolve_digits`, `otp_resolve_expiry`, `otp_resolve_max_attempts`, `otp_purpose_valid`, `otp_json_int64`, `otp_json_copy_string`, `otp_get_active_callback`, `otp_write_callback`, `otp_parse_iso_time`, `otp_set_err` |
| `mailrelay.h` (no dedicated producer header) | `idempotency_callback`, `status_from_a63`, `add_recipients_arrays`, `producer_check_runtime`, `producer_try_idempotency`, `producer_resolve_from_reply`, `producer_enqueue_message`, `mailrelay_send_template_default`, `mailrelay_send_direct_default` |
| `mailrelay_queue.h` | `timespec_compare`, `item_is_due`, `find_first_due`, `find_earliest`, `remove_item` |
| `mailrelay_render.h` | `render_grow`, `render_mem`, `render_str`, `render_header`, `render_header_list`, `render_boundary`, `render_mime_part`, `render_rfc822_date` |
| `mailrelay_repository.h` | `mailrelay_repo_resolve_database`, `mailrelay_repo_invoke_callback`, `mailrelay_repo_default_execute`, `repo_params_new`, `repo_add_string`, `repo_add_int`, `repo_add_int64`, `repo_execute_json`, `repo_execute_empty` |
| `mailrelay_smtp.h` | `resolve_tls_mode`, `build_request`, `smtp_read_cb`, `smtp_write_cb`, `parse_smtp_code` |
| `mailrelay_template.h` | `is_macro_name_char`, `output_append`, `output_append_char`, `output_free`, `resolve_builtin`, `parse_macro`, `is_valid_macro_name`, `preview_render_callback` |
| `mailrelay_workers.h` | `mailrelay_worker_thread` |

### Header include fixes required (CAREFUL — caused the Coverage build to break)

When you add a function prototype that references a type not already visible
in that header, **the header must include the header that defines the type**,
otherwise a test that includes only this header (under a different build
config) fails to compile.

- `mailrelay_otp.h` was edited to add
  `#include <src/mailrelay/mailrelay_repository.h>` (for `MailRelayRepoResult`,
  `json_t`).
- `mailrelay_internal.h` adds `mailrelay_repository.h` (for `MailRelayRepoResult`).
- `mailrelay_template.h` adds `mailrelay_repository.h` (needed for
  `MailRelayRepoResult` in `preview_render_callback`).

### Lessons learned (apply to scripting and every future batch)

1. **Remove `static` AND add a header declaration — two steps, both required.**
   Removing `static` alone makes the symbol linkable but a test TU still can't
   *see* it without a declaration. Tests do `#include <src/mailrelay/...>`.
2. **Make every touched header self-contained.** Before moving on, a header
   that declares a function using type `T` must itself `#include` (directly or
   transitively) the header defining `T`. The build configs are NOT identical:
   `mkt` runs `Unity`, `Regular`, AND `Coverage` (`-DBUILD_TYPE='"Coverage"'`),
   and the Coverage config flags/`-I` path exposed a missing include that the
   Unity build masked via transitive includes. **A header compiling in one
   config does not guarantee it compiles standalone.**
3. **Check for same-file prototype+definition pairs first.** Many "duplicate"
   names are just `static foo(...);` forward-decl followed by `static foo(...) {...}`
   in the same `.c`. These are not real collisions — don't rename them. The
   reliable way to detect TRUE collisions is: count distinct *files* per name;
   a name in ≥2 different `.c` files is a real linker collision.
4. **Don't rename unless there's a real cross-file collision.** Within a
   module, function names are typically unique across files, so no renaming is
   needed. (Confirmed: mailrelay needed 0 renames.)
5. **Verify against ALL build configs, not just one.** Minimum bar:
   - `cmake --build build --target hydrogen_unity` (library)
   - `cmake --build build --target unity_tests` (full test suite, 906/1117 targets)
   - `cmake --build build --target hydrogen` (regular daemon)
   - `cmake --build build --target hydrogen_coverage` + `coverage` (Coverage config)
   The `Unity` library build passing is NOT sufficient proof — the header
   include bug only surfaced in `Coverage`.
6. **Run a sample of the affected tests** after editing, to confirm runtime
   behavior is unchanged (mailrelay: queue 7, template 22, otp 23/33, producer
   33, repository 40 — all 0 failures).
7. **Keep genuinely module-private `static`.** Test-seam callbacks, file-scope
   globals, and string literals stay `static` — removing them buys nothing and
   risks exposing internal state.
8. **Prefer `python3` in-place replace for many occurrences** of `static` →
   non-static in one file (e.g. 10–16 replacements per file); it's faster and
   less error-prone than many individual `edit` calls. Always re-read the file
   after to confirm counts.
9. **Watch for EDIT pitfalls:** the `edit` tool requires exact whitespace;
   when a header's tail has blank lines / `#endif` patterns, match the *exact*
   surrounding lines (use `sed -n 'Np' | cat -A` to see exact bytes). Also note
   `mailrelay_template.h` ends with `MAILRELAY_TEMPLATE_H` (not `MAILRELAY_RENDER_H`),
   and `mailrelay.h` has no separate producer header — producer internals go in
   `mailrelay.h`.
10. **No CMake changes are needed.** Sources are collected via
    `file(GLOB_RECURSE HYDROGEN_SOURCES "../src/*.c")` in
    `cmake/CMakeLists-init.cmake`, so adding/removing `static` and editing
    headers does not require touching build files.

---

## scripting — NEXT BATCH (playbook to start fresh)

Scope: `elements/001-hydrogen/hydrogen/src/scripting`. ~85 `static` functions
across roughly these files (re-derive exact line numbers with the command in
"Reproduce the scan" before starting — they shift as code changes):

- `orchestrator.c` (12)
- `worker_pool.c` (8)
- `scoreboard.c` (6)
- `scripting_api_mail_notify.c` (7)
- `http_pool.c` (7)
- `lua_context.c` (4)
- `scripting_api_llm.c` (3)
- `scripting_handle.c` (1)
- `scripting_api_scoreboard.c` (1)
- `script_registry.c` (1)
- `source_cache.c` (1)
- `http_client.c` (2)
- `lua_hook.c` (1)

(NOTE: a couple of scripting functions are Java/other-language namespaced and
may need care — `bytecode_dump_writer`, etc. Verify they're real C functions
in `.c` files first.)

### Step-by-step

1. **Reproduce the scan** to get the exact current list (line numbers move):

   ```bash
   rg -N --no-heading -n '^\s*static\b.*\w+\s*\(' -g '*.c' -g '*.h' \
     elements/001-hydrogen/hydrogen/src/scripting
   ```

   To detect TRUE cross-file collisions only:

   ```bash
   rg -N --no-heading -n '^\s*static\b.*\w+\s*\(' -g '*.c' -g '*.h' \
     elements/001-hydrogen/hydrogen/src/scripting \
   | rg -o 'static\s+[\w\s\*]*?(\w+)\s*\(' -r '$1' \
   | sort | uniq -d      # these "duplicates" are prototype+def pairs — verify they're same-file
   ```

   Then confirm none collide with the rest of `hydrogen/src` (reuse the
   comm-style check from mailrelay).
2. **For each `.c`, strip `static`** from every function definition (and any
   same-file forward declaration). Use `python3` in-place replace per file;
   re-read to confirm counts.
3. **Add a declaration to the right header for each.** Mapping guidance:
   - Most scripting files already have a matching `*.h` — put the declaration
     there.
   - `lua_context.c` → `lua_context.h`; `worker_pool.c` → `worker_pool.h`;
     `scoreboard.c` → `scoreboard.h`; `orchestrator.c` → `orchestrator.h`;
     `http_pool.c`/`http_client.c` → their http headers;
     `scripting_api_*.c` → the matching api header;
     `script_registry.c`/`source_cache.c`/`scripting_handle.c`/`lua_hook.c` →
     their headers. If a file has no header, create one or place the decl in
     the nearest module header (mirror the `mailrelay.h` producer-internals
     approach).
   - For functions touching Lua (`lua_State*`), **forward-declare
     `typedef struct lua_State lua_State;`** in the header instead of pulling
     the Lua headers into a public header (this is what `mailrelay_events.h`
     did).
4. **Make every touched header self-contained** (Lesson #2). scripting headers
   reference types like `ScriptingWorkerPool`, `ScriptingHttpPool`,
   `Scoreboard`, `MailRelayTemplateParams`, `OutboundServer`, `json_t`, etc. —
   each must be available via include. When in doubt, add the specific include
   (e.g. a header declaring `MailRelayTemplateParams` needs
   `mailrelay_template.h`).
5. **Build & verify ALL configs** (Lesson #5):
   `hydrogen_unity`, `unity_tests`, `hydrogen`, `hydrogen_coverage`, `coverage`.
   Pay special attention to `Coverage` — it's where include gaps surface.
6. **Run a sample of scripting tests** (e.g. scoreboard, worker_pool,
   orchestrator) to confirm no behavior change.
7. **Update this doc**: mark scripting COMPLETE with its header→function
   mapping, and append any new lessons.

### Reusable scan/verify one-liner set (run from repo root)

```bash
# 1. count static fns in a module
rg -N --no-heading -c '^\s*static\b.*\w+\s*\(' -g '*.c' -g '*.h' \
  elements/001-hydrogen/hydrogen/src/scripting | grep -v ':0$'
# 2. confirm none left after conversion (expect only test-seam globals)
rg -n '^\s*static\b' elements/001-hydrogen/hydrogen/src/scripting/*.c
```

---

## Collision detail (reference — mailrelay, all safe)

Names declared as both prototype and definition in the same file (NOT real
collisions — safe to un-`static`):

- `build_request`
- `create_entry`
- `H_lua_open_sandboxed_libraries`
- `H_lua_panic`
- `idempotency_callback`
- `is_macro_name_char`
- `is_valid_macro_name`
- `mailrelay_repo_default_execute`
- `orchestrator_load_configured_blocking`
- `orchestrator_loader_main`
- `orchestrator_resolve_database`
- `orchestrator_set_shutdown_and_join`
- `orchestrator_thread_main`
- `output_append`
- `output_append_char`
- `output_free`
- `parse_macro`
- `parse_smtp_code`
- `preview_render_callback`
- `resolve_builtin`
- `resolve_tls_mode`
- `scripting_http_worker_process_one`
- `scripting_http_worker_should_exit`
- `scripting_http_worker_thread`
- `scripting_signal_waiter_if_present`
- `scripting_worker_process_one`
- `scripting_worker_should_exit`
- `scripting_worker_thread`
- `smtp_read_cb`
- `smtp_write_cb`
