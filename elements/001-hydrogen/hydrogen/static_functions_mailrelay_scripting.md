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
| **scripting** (`src/scripting`) | 54 (in 13 files) | ✅ **COMPLETE** |

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

## scripting — COMPLETE

### What was done

- All 54 `static` function definitions across 13 scripting `.c` files were
  converted to non-`static`. (The original estimate of ~85 was high; the
  live scan found 54 — the rest had already been converted in prior work.)
- A matching declaration was added to the appropriate header for each
  function (see mapping below), marked with a "NOT part of the stable
  public API / exposed for Unity tests" comment block.
- Three new headers were created for `.c` files that previously had none:
  - `scripting_api_mail_notify.h` — owns the previously-private
    `MailLuaParse` struct and the 7 parse/helper declarations.
  - `scripting_api_llm.h` — owns the 3 LLM resolve/build helper
    declarations.
  - `scripting_api_scoreboard.h` — owns `bytecode_dump_writer`. The
    previously-anonymous `BytecodeDumpBuffer` struct was promoted to a
    named `struct BytecodeDumpBuffer` so the header's opaque forward
    declaration matches the definition.
- Remaining `static` in `src/scripting` is **intentional and correct**:
  file-scope globals / string literals (e.g. `NOTIFY_DEFERRED_ERROR`,
  the `SourceCacheEntry`/`ScriptRegistryEntry` struct definitions, the
  `*_INITIAL_CAPACITY` macros).
- **No functions were renamed.** All 14 repeated names were
  prototype+definition pairs in the same file (see Collision detail at the
  bottom); there were 0 true cross-file name collisions, and 0 collisions
  with the rest of `hydrogen/src`.

### Header → function mapping (where the declarations went)

| Header | Functions exposed |
|--------|-------------------|
| `worker_pool.h` | `scripting_worker_thread`, `scripting_worker_process_one`, `scripting_worker_should_exit`, `scripting_signal_waiter_if_present` |
| `http_pool.h` | `serialize_headers`, `scripting_http_worker_thread`, `scripting_http_worker_process_one`, `scripting_http_worker_should_exit` |
| `http_client.h` | `try_test_injection`, `resolve_timeout` |
| `orchestrator.h` | `orchestrator_thread_main`, `orchestrator_set_shutdown_and_join`, `orchestrator_load_configured_blocking`, `orchestrator_loader_main`, `orchestrator_resolve_database`, `orchestrator_build_params_json`, `orchestrator_extract_code_from_result` |
| `scoreboard.h` | `is_terminal_status`, `entry_clear_owned`, `timespec_clear`, `timespec_now`, `entries_grow_if_needed`, `generate_unique_id` |
| `scripting_api_mail_notify.h` (new) | `free_mail_parse`, `mail_parse_init`, `parse_recipient_field`, `parse_params_table`, `parse_optional_string_field`, `parse_mail_message`, `status_to_mail_error` |
| `scripting_api_llm.h` (new) | `resolve_llm_db_queue`, `resolve_llm_engine`, `build_llm_request_json` |
| `lua_context.h` | `H_lua_open_sandboxed_libraries`, `H_lua_panic` |
| `scripting_handle.h` | `H_Handle_gc` |
| `source_cache.h` | `source_cache_grow_if_needed` |
| `script_registry.h` | `registry_grow_if_needed` |
| `lua_hook.h` | `H_lua_progress_hook_fn` |
| `scripting_api_scoreboard.h` (new) | `bytecode_dump_writer` |

### Header include fixes required

When you add a function prototype that references a type not already
visible in that header, the header must include the header that defines
the type, otherwise a test that includes only this header fails to
compile. For scripting these were:

- `http_pool.h` adds `#include <src/api/auth/oidc_rp/oidc_rp_http.h>`
  (for `OidcRpHttpResponse`, used by `serialize_headers`).
- `scripting_api_mail_notify.h` adds `src/utils/utils_uuid.h`
  (`UUID_STR_LEN`), `src/mailrelay/mailrelay.h` (`MailRelayTemplateParams`,
  `MailRelayStatus`), and `src/mailrelay/mailrelay_template.h`
  (`mailrelay_template_params_*`). The header now owns the `MailLuaParse`
  struct, so `scripting_api_mail_notify.c` no longer defines it (it
  includes the new header instead).
- `scripting_api_llm.h` adds `jansson.h`, `src/database/dbqueue/dbqueue.h`
  (`DatabaseQueue`), and `src/api/wschat/helpers/engine_cache.h`
  (`ChatEngineConfig`).
- `scripting_api_scoreboard.h` forward-declares `lua_State` and
  `struct BytecodeDumpBuffer` (opaque), so it does not pull Lua headers
  into unrelated TUs.
- `lua_context.h`, `lua_hook.h`, `scripting_handle.h`, `source_cache.h`,
  `script_registry.h` already had `lua_State` / `Scoreboard` /
  `SourceCache` / `ScriptRegistry` visible via existing includes or
  forward declarations — no extra includes needed.

### Lessons learned (apply to every future batch)

1. **Remove `static` AND add a header declaration — two steps, both required.**
   Removing `static` alone makes the symbol linkable but a test TU still
   can't *see* it without a declaration. Tests do `#include <src/scripting/...>`.
2. **Make every touched header self-contained.** The build configs are NOT
   identical: `mkt` runs `Unity`, `Regular`, AND `Coverage`. A header
   compiling in one config does not guarantee it compiles standalone (a
   test TU that includes only this header under a different config). Every
   header declaring a function using type `T` must itself `#include` (directly
   or transitively) the header defining `T`.
3. **The `.c` file MUST `#include` the header that holds its new
   declarations.** With `-Werror=missing-prototypes`, a non-`static`
   function with no visible prototype is a hard error. Three new-header
   `.c` files (`scripting_api_llm.c`, `scripting_api_scoreboard.c`, and the
   `MailLuaParse`-owning `scripting_api_mail_notify.c`) needed explicit
   `#include` lines added — the build failed on the first trial until fixed.
4. **Check for same-file prototype+definition pairs first.** All 14
   "duplicate" names were just `static foo(...);` forward-decl followed by
   `static foo(...) {...}` in the same `.c`. These are not real collisions.
   The reliable way to detect TRUE collisions is: count distinct *files*
   per name; a name in ≥2 different `.c` files is a real linker collision.
   Verified: 0 true cross-file collisions within scripting, and 0
   collisions with the rest of `hydrogen/src`.
5. **Don't rename unless there's a real cross-file collision.** Within a
   module, function names are unique across files, so no renaming is
   needed. (Confirmed: scripting needed 0 renames.)
6. **Verify against ALL build configs, not just one.** Minimum bar:
   - `cmake --build build --target hydrogen_unity` (library)
   - `cmake --build build --target unity_tests` (full test suite)
   - `cmake --build build --target hydrogen` (regular daemon)
   - `cmake --build build --target hydrogen_coverage` + `coverage`
   The `Unity` library build passing is NOT sufficient proof — include
   gaps surface in `Regular`/`Coverage` because of `-Werror=missing-prototypes`.
7. **Run a sample of the affected tests** after editing to confirm runtime
   behavior is unchanged. Scripting: scoreboard (submit), worker_pool
   (submit), orchestrator (load_run), http_pool (lifecycle), lua_context
   (run_string), source_cache (put_get), script_registry
   (register_lookup), lua_hook (install), scripting_handle (lifecycle),
   scripting_api (mail, llm) — all 0 failures.
8. **Keep genuinely module-private `static`.** File-scope globals, string
   literals, and private struct definitions stay `static` — removing them
   buys nothing and risks exposing internal state.
9. **Prefer `python3` in-place replace for many occurrences** of
   `static` → non-static in one file; it's faster and less error-prone
   than many individual `edit` calls. Always re-read the file after to
   confirm counts.
10. **Watch for header self-definition collisions.** If a header
    forward-declares `typedef struct Foo Foo;` but the `.c` defines
    `typedef struct { ... } Foo;` (anonymous tag), the two conflict. Fix
    by giving the `.c` struct a named tag
    (`typedef struct Foo { ... } Foo;`).
11. **No CMake changes are needed.** Sources are collected via
    `file(GLOB_RECURSE HYDROGEN_SOURCES "../src/*.c")` in
    `cmake/CMakeLists-init.cmake`, so adding/removing `static` and editing
    headers (or adding new headers) does not require touching build files.

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
