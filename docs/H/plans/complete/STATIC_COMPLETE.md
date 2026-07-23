# Static functions cleanup — hydrogen/src (mailrelay + scripting + oidc_rp + wschat/helpers)

Scope: `elements/001-hydrogen/hydrogen/src/mailrelay`, `.../src/scripting`,
`.../src/api/auth/oidc_rp`, and `.../src/api/wschat/helpers`.
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
| **oidc_rp** (`src/api/auth/oidc_rp`) | 96 (in 15 files) | ✅ **COMPLETE** |
| **wschat/helpers** (`src/api/wschat/helpers`) | 28 (in 9 files) | ✅ **COMPLETE** |
| **utils** (`src/utils`) | 10 (in 1 file) | ✅ **COMPLETE** |
| **logging** (`src/logging`) | 11 (in 1 file) | ✅ **COMPLETE** |
| **database** (`src/database`) | 26 (in 13 files) | ✅ **COMPLETE** |
| **config** (`src/config`) | 6 (in 1 file) | ✅ **COMPLETE** |
| **status** (`src/status`) | 5 (in 2 files) | ✅ **COMPLETE** |
| **launch** (`src/launch`) | 4 (in 2 files) | ✅ **COMPLETE** |
| **api** (`src/api`, 8 files) | 16 | ✅ **COMPLETE** |
| **handlers** (`src/handlers`) | 3 (in 1 file) | ✅ **COMPLETE** |
| **terminal / websocket / webserver** (last 3) | 3 (1 each) | ✅ **COMPLETE** |

All three conversions build clean under the `Unity`, `Regular`, and
`Coverage` build configs, pass cppcheck, and all existing subsystem Unity
tests pass. The automated static-function gate in `mkt` (see
`docs/H/INSTRUCTIONS.md`) prevents new `static` helpers from being added.

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

### What was done with scripting

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

### Header → function mapping (where the declarations went) - scripting

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

## oidc_rp — COMPLETE

### What was done - OIDC RP

- All 96 `static` function definitions across 15 `src/api/auth/oidc_rp/*.c`
  files were converted to non-`static`.
- A matching declaration was added to the appropriate existing header for
  each function (every `.c` already had a matching `.h`), marked with a
  "NOT part of the stable public API / exposed for Unity tests" comment
  block.
- **5 real cross-file collisions were resolved by renaming FIRST** (see the
  oidc_rp Collision detail at the bottom). Unlike mailrelay/scripting
  (which had 0 true collisions), oidc_rp had helper names duplicated across
  different `.c` files with different signatures. Each was given a
  module-scoped prefix before de-`static`'ing so the linker namespace stays
  unique.
- Remaining `static` in `src/api/auth/oidc_rp` is **intentional and
  correct**: file-scope state only — cache singletons (`g_cache`,
  `g_store`, `g_handoff_store`), test-seam flags
  (`g_test_disable_sweeper`, `g_handoff_test_disable_sweeper`), the HTTP
  test-fixture queue (`g_test_queue*`, `g_test_fixture_lock`), the roles
  test-seam fn pointer (`g_roles_query_fn`), `hex_digits[]`, and the
  service `pthread_once_t` init guards.

### Header → function mapping (where the declarations went) - OIDC RP

| Header | Functions exposed |
|--------|-------------------|
| `oidc_rp_callback.h` | `callback_truncated_state`, `build_spa_error_url`, `build_spa_success_url`, `redirect_with_error`, `callback_scrub_free` |
| `oidc_rp_debug_inject.h` | `send_bad_request` |
| `oidc_rp_discovery.h` | `doc_clear_fields`, `build_discovery_url`, `extract_required_string`, `extract_optional_string`, `discovery_find_slot_locked`, `discovery_find_empty_slot_locked`, `discovery_entry_expired`, `replace_slot_locked` |
| `oidc_rp_handoff.h` | `send_handoff_invalid` |
| `oidc_rp_handoff_store.h` | `handoff_fnv1a_hash`, `handoff_bucket_for`, `handoff_opt_strdup`, `handoff_scrub_free`, `handoff_record_free_fields`, `handoff_entry_expired`, `handoff_detach_entry`, `handoff_free_entry`, `handoff_remove_locked`, `handoff_sweep_expired_locked`, `handoff_inline_sweep_locked`, `handoff_sweeper_loop` |
| `oidc_rp_http.h` | `response_alloc`, `response_set_error`, `response_clear_headers`, `header_list_append`, `write_callback`, `header_callback`, `url_matches`, `take_fixture`, `preflight_request`, `apply_common_curl_opts`, `perform_and_finalize`, `resolve_max_body`, `resolve_request_timeout` |
| `oidc_rp_idtoken.h` | `scrub_and_free`, `scrub_bytes_and_free`, `split_jws`, `parse_header`, `alg_is_allowed`, `verify_signature`, `copy_string_field`, `aud_contains`, `copy_realm_roles`, `parse_payload_and_check` |
| `oidc_rp_jwks.h` | `jwk_clear_fields`, `slot_clear_locked`, `copy_optional_string`, `copy_required_string`, `jwks_find_slot_locked`, `jwks_find_empty_slot_locked`, `jwks_entry_expired`, `find_kid_in_slot_locked` |
| `oidc_rp_link_default.h` | `domains_equal_ci_def`, `email_domain_def`, `email_domain_allowed_def` |
| `oidc_rp_link_provision.h` | `domains_equal_ci`, `email_domain`, `email_domain_allowed` |
| `oidc_rp_roles.h` | `run_query`, `roles_buf_init`, `roles_buf_append`, `roles_buf_steal`, `roles_buf_free`, `roles_from_idp`, `split_roles`, `free_roles_array`, `roles_merge` |
| `oidc_rp_service.h` | `append_param`, `oidc_rp_runtime_init_impl` |
| `oidc_rp_start.h` | `send_internal_error`, `start_truncated_state` |
| `oidc_rp_state.h` | `fnv1a_hash`, `bucket_for`, `opt_strdup`, `state_scrub_free`, `record_free_fields`, `state_entry_expired`, `detach_entry`, `free_entry`, `remove_locked`, `sweep_expired_locked`, `inline_sweep_locked`, `sweeper_loop` |
| `oidc_rp_token.h` | `token_scrub_free`, `append_form_param`, `build_token_body`, `build_basic_auth_header`, `map_oauth_error_code`, `parse_error_body`, `parse_success_body` |

### Header include / forward-declaration fixes required

Several helpers take pointers to **file-private struct types** that live in
the `.c`, not the header. Each such type was **forward-declared as opaque**
in the header (`typedef struct X X;`) so the pointer-taking prototype
compiles without pulling the full definition into unrelated TUs. C17 permits
the same typedef to be repeated, so the `.c`'s
`typedef struct X { ... } X;` (which must use a **named tag**) coexists with
the header's opaque forward declaration.

- `oidc_rp_discovery.h` — added `<time.h>`, `<jansson.h>`; forward-declares
  `DiscoveryEntry`.
- `oidc_rp_jwks.h` — added `<time.h>`, `<jansson.h>`; forward-declares
  `JwksEntry`.
- `oidc_rp_handoff_store.h` — added `<stdint.h>` (for `uint64_t`);
  forward-declares `HandoffEntry`.
- `oidc_rp_state.h` — added `<stdint.h>`; forward-declares `StateEntry`.
- `oidc_rp_http.h` — added `<curl/curl.h>` (for `CURL`); forward-declares
  `HeaderList`, `ResponseBuffer`.
- `oidc_rp_idtoken.h` — added `<jansson.h>` (helpers take `json_t*`).
- `oidc_rp_roles.h` — forward-declares `RolesBuf`; the `.c`'s previously
  **anonymous** `typedef struct { ... } RolesBuf;` was promoted to a named
  tag `typedef struct RolesBuf { ... } RolesBuf;` so it matches (see
  lesson #10 from scripting).
- `oidc_rp_callback.h` — added `<src/config/config_oidc_rp.h>` (for
  `OIDCRPProviderConfig`).

### Lessons learned specific to oidc_rp (in addition to all prior lessons)

1. **Check for TRUE cross-file collisions BEFORE de-`static`'ing.** Unlike
   mailrelay/scripting, oidc_rp had 5 helper names defined in ≥2 different
   `.c` files (`scrub_free`, `truncated_state`, `entry_expired`,
   `find_slot_locked`, `find_empty_slot_locked`). Removing `static` without
   renaming would have caused duplicate-symbol linker errors. Detect them
   by counting distinct *files* per name; rename with a module prefix
   (`state_`, `handoff_`, `discovery_`, `jwks_`, `callback_`, `start_`,
   `token_`) FIRST, then de-`static`.
2. **File-private structs need opaque forward declarations in the header.**
   Many helpers take `DiscoveryEntry*` / `JwksEntry*` / `StateEntry*` /
   `HandoffEntry*` / `HeaderList*` / `ResponseBuffer*` / `RolesBuf*` — all
   defined only in the `.c`. Forward-declare each as
   `typedef struct X X;` in the header. This keeps the struct internals
   private while making the prototype compile. Confirm the `.c` uses a
   **named** struct tag (not anonymous) or C will reject the mismatch.
3. **Automated signature extraction is fragile with one-line function
   bodies.** These files heavily use single-line bodies and preceding
   comment blocks. A regex extractor kept capturing comment prose or body
   text as the "return type". The reliable recipe: locate the definition
   by *known function name*, capture params via paren-depth matching to the
   `)` that precedes `{`, then strip `/* */` and `//` comments AND trim any
   leading prose down to the trailing valid-C type tokens. Then let the
   compiler (`-Werror=missing-prototypes`) be the final judge.
4. **The `mkt` static-function gate does not block de-`static`'ing.**
   Removing `static` only ever *shrinks* the set of static names, so the
   baseline comparison (`comm -23`) never flags it. Renamed-then-de-static
   functions become non-static and are likewise never flagged. No baseline
   regeneration was needed.
5. **`static` file-scope state is the ONLY intentional `static` here.**
    Cache singletons, test-seam flags, the HTTP fixture queue, the roles
    test-seam fn pointer, and `pthread_once_t`/`hex_digits[]` all stay
    `static` — they are module-private *state*, not callable helpers.

---

## wschat/helpers — COMPLETE

### What was done - wschat

- All 28 `static` function definitions across 9 `src/api/wschat/helpers/*.c`
  files were converted to non-`static`.
- A matching declaration was added to the appropriate existing header for
  each function (every `.c` already had a matching `.h`), marked with a "NOT
  part of the stable public API / exposed for Unity tests" comment block.
- **Generic helper names were renamed with a `chat_*`-module prefix BEFORE
  de-`static`'ing** to avoid colliding, at link time or via conflicting
  prototypes, with other modules that already declare/define the same
  generic name (e.g. `hash_string`, `save_metadata`, `load_metadata`,
  `free_cache_entry`, `lru_remove`, `lru_add_front`, `evict_lru_entries`,
  `ensure_directory_exists`, `get_segment_path`, `get_metadata_path`,
  `get_cache_base_dir`, `sync_thread_func`, `stream_write_callback`,
  `stream_debug_callback`, `stream_worker_thread`, `cleanup_completed_streams`,
  `is_hex_digit`, `get_or_create_cache`, `multi_worker_thread`,
  `get_metric_entry`). None of these were true cross-file collisions in the
  linker sense (only one definition existed), but they shared very generic
  names with prototypes/definitions elsewhere, so prefixing keeps the global
  namespace safe and consistent with the wider codebase.
- **One name clashed with an EXISTING public function**: the internal helper
  `lru_remove` was initially renamed to `chat_lru_cache_remove`, but
  `lru_cache.h` already declares a public `chat_lru_cache_remove(ChatLRUCache*,
  const char* segment_hash)`. The helper was instead named
  `chat_lru_cache_remove_entry(ChatLRUCache*, ChatLRUCacheEntry*)` so the
  public API is untouched.
- `metrics.h` forward-declares `ChatMetricEntry` as opaque
  (`typedef struct ChatMetricEntry ChatMetricEntry;`) because the struct is
  defined only in `metrics.c` and `get_metric_entry` only returns a pointer.
- Remaining `static` in `src/api/wschat/helpers` is now **zero** (no
  module-private state needed conversion here).

### Header → function mapping (where the declarations went) - wschat

| Header | Functions exposed |
|--------|-------------------|
| `lru_cache.h` | `chat_lru_cache_get_base_dir`, `chat_lru_cache_hash_string`, `chat_lru_cache_ensure_directory_exists`, `chat_lru_cache_get_segment_path`, `chat_lru_cache_get_metadata_path`, `chat_lru_cache_free_entry`, `chat_lru_cache_remove_entry`, `chat_lru_cache_add_front`, `chat_lru_cache_evict_lru_entries`, `chat_lru_cache_save_metadata`, `chat_lru_cache_load_metadata`, `chat_lru_cache_sync_thread_func` |
| `proxy.h` | `chat_proxy_write_callback`, `chat_proxy_stream_write_callback`, `chat_proxy_stream_debug_callback`, `chat_proxy_stream_worker_thread`, `chat_proxy_cleanup_completed_streams` |
| `req_builder.h` | `chat_request_convert_openai_content_to_anthropic`, `chat_request_message_count_images`, `chat_request_count_all_images` |
| `health.h` | `chat_health_check_openai`, `chat_health_check_anthropic`, `chat_health_check_ollama` |
| `storage_hex.h` | `chat_storage_is_hex_digit` |
| `storage.h` | `chat_storage_get_or_create_cache` |
| `proxy_multi.h` | `chat_proxy_multi_worker_thread` |
| `metrics.h` (opaque `ChatMetricEntry`) | `chat_metrics_get_metric_entry` |
| `engine_cache.h` | `chat_engine_cache_add_engine_locked` |

### Lessons learned specific to wschat/helpers (in addition to all prior lessons)

1. **Check for prototype/definition collisions with the EXISTING public API
   in the same header.** `lru_remove` → `chat_lru_cache_remove` collided with
   the already-public `chat_lru_cache_remove(ChatLRUCache*, const char*
   segment_hash)`. Always grep the destination header for the chosen name
   before committing to a rename; pick a distinct suffix (`_entry`).
2. **Generic helper names warrant a module prefix even without a linker
   collision.** The earlier collision check only catches ≥2 *definitions*;
   it misses a generic name that is *declared* (prototype) elsewhere with a
   different signature. Prefix uniformly (`chat_lru_cache_`, `chat_proxy_`,
   `chat_request_`, `chat_storage_`, `chat_metrics_`) to keep the global
   namespace clean and avoid redeclaration warnings under `-Werror`.
3. **Beware the python double-prefix bug.** When renaming via
   `re.sub(...'static ...<old>('...)` then `s.replace('<old>','<prefix><old>')`,
   the regex-replaced definition line already contains the prefix, so the
   subsequent `replace` adds it twice (`chat_proxy_chat_proxy_...`). Fix by
   reverting the affected def/forward-decl lines, or do the rename purely
   with `replace` and drop the `static` keyword separately.
4. **`mkt` exercises Regular + Unity + Coverage configs.** The build passed
   all three; cppcheck (mkp) clean; the 6 existing wschat helper Unity tests
   (`storage_hex_test_binary_to_hex`, `storage_compress_test_compress_message`,
   `storage_hash_test_generate_hash`, `storage_media_test_resolve_media`,
   `storage_media_test_store_media`, `storage_test_utility_functions`) all
    pass with 0 failures.

---

## utils + logging — COMPLETE

### What was done - utils

- **utils** (`src/utils/utils_dependency.c`): 10 `static` function definitions
  were converted to non-`static`. The module-init `init_utils` (marked
  `__attribute__((constructor))`) was intentionally left `static` — it is
  file-scope startup state, not a testable helper.
- **logging** (`src/logging/victoria_logs.c`): 11 `static` function definitions
  were converted to non-`static`.
- A matching declaration was added to each module's existing header
  (`utils_dependency.h`, `victoria_logs.h`), marked with the standard "NOT part
  of the stable public API / exposed for Unity tests" comment block.
- **Generic helper names were prefixed** (`utils_dependency_`, `victoria_logs_`)
  to avoid clashing with prototypes/definitions elsewhere that share generic
  names (`parse_url`, `send_http_post`, `get_version`, `log_status`,
  `get_status_string`, `add_to_batch`, `reset_long_timer`, `clear_batch`,
  `flush_batch_internal`, `vl_queue_*`). No true linker collisions existed
  (only one definition each), but prefixing keeps the global namespace clean
  and prevents redeclaration warnings under `-Werror`.

### utils_dependency.c specifics

- `DatabaseDependencyConfig` and `LibConfig` were **anonymous** struct typedefs
  defined only in `utils_dependency.c`. The helpers `get_database_version()`
  and `get_version()` take pointers to them, so the structs were promoted to
  **named tags** (`typedef struct DatabaseDependencyConfig { ... }
  DatabaseDependencyConfig;`, same for `LibConfig`) and forward-declared opaque
  in `utils_dependency.h` (`typedef struct DatabaseDependencyConfig
  DatabaseDependencyConfig;`). C17 permits the same typedef to be repeated, so
  the header's opaque forward declaration coexists with the `.c` definition.
- Renamed functions: `get_status_string`→`utils_dependency_get_status_string`,
  `determine_status`→`utils_dependency_determine_status`,
  `log_status`→`utils_dependency_log_status`, `parse_db2_version`→
  `utils_dependency_parse_db2_version`, `parse_postgresql_version`→
  `utils_dependency_parse_postgresql_version`, `parse_mysql_version`→
  `utils_dependency_parse_mysql_version`, `parse_sqlite_version`→
  `utils_dependency_parse_sqlite_version`, `get_database_version`→
  `utils_dependency_get_database_version`, `check_database_thread`→
  `utils_dependency_check_database_thread`, `get_version`→
  `utils_dependency_get_version`.

### victoria_logs.c specifics

- The internal helper `clear_batch` already had a public wrapper
  `victoria_logs_clear_batch()` (declared in `victoria_logs.h`). To avoid
  shadowing the public API, the internal helper was renamed
  `victoria_logs_clear_batch_impl` and the wrapper now calls it. (Do NOT also
  name the helper `victoria_logs_clear_batch_internal` — that substring
  contains `clear_batch` and a blanket `s.replace` will double-rename the
  already-renamed identifier.)
- Renamed functions: `parse_url`→`victoria_logs_parse_url`,
  `send_http_post`→`victoria_logs_send_http_post`, `vl_queue_init`→
  `victoria_logs_queue_init`, `vl_queue_cleanup`→`victoria_logs_queue_cleanup`,
  `vl_queue_enqueue`→`victoria_logs_queue_enqueue`, `vl_queue_dequeue`→
  `victoria_logs_queue_dequeue`, `add_to_batch`→`victoria_logs_add_to_batch`,
  `reset_long_timer`→`victoria_logs_reset_long_timer`,
  `flush_batch_internal`→`victoria_logs_flush_batch_internal`,
  `victoria_logs_worker` kept (already prefixed).

### Header → function mapping

| Header | Functions exposed |
|--------|-------------------|
| `utils_dependency.h` (opaque `DatabaseDependencyConfig`, `LibConfig`) | `utils_dependency_get_status_string`, `utils_dependency_determine_status`, `utils_dependency_log_status`, `utils_dependency_parse_db2_version`, `utils_dependency_parse_postgresql_version`, `utils_dependency_parse_mysql_version`, `utils_dependency_parse_sqlite_version`, `utils_dependency_get_database_version`, `utils_dependency_check_database_thread`, `utils_dependency_get_version` |
| `victoria_logs.h` | `victoria_logs_parse_url`, `victoria_logs_send_http_post`, `victoria_logs_queue_init`, `victoria_logs_queue_cleanup`, `victoria_logs_queue_enqueue`, `victoria_logs_queue_dequeue`, `victoria_logs_add_to_batch`, `victoria_logs_reset_long_timer`, `victoria_logs_clear_batch_impl`, `victoria_logs_flush_batch_internal`, `victoria_logs_worker` |

### Lessons learned specific to utils + logging

1. **Watch the `s.replace` double-rename bug with the wrapper pattern.** When a
   public wrapper `victoria_logs_clear_batch()` already exists and the internal
   helper is `clear_batch`, a blanket `s.replace("clear_batch", "...")` will also
   rewrite the wrapper name and any already-renamed identifier whose text
   *contains* `clear_batch`. Strip `static` with a regex that renames only the
   definition line, then rename call sites individually, OR pick an internal
   suffix that does NOT contain the original base name
   (`_impl` instead of `_internal`).
2. **Promote anonymous file-private structs to named tags before de-static'ing
   functions that take them by pointer.** Forward-declare opaque in the header.
3. **`mkt` passes Regular + Unity + Coverage; `mkp` (cppcheck) clean;** the
   existing `utils_dependency_test_*` and `victoria_logs_test_*` Unity tests
   all pass with 0 failures (sampled: `utils_dependency_test_database_version`,
    `utils_dependency_test_parsing_functions`, `victoria_logs_test_batch`,
    `victoria_logs_test_send`, `victoria_logs_test_init`,
    `utils_logging_test_get_priority_label`).

---

## database — COMPLETE

### What was done - database

- All 26 `static` function definitions across 13 `src/database/**/*.c` files
  were converted to non-`static`.
- A matching declaration was added to the appropriate existing header for each
  function (every `.c` already had a matching `.h`, except the migration
  helper which went into the existing `migration/migration.h`), marked with the
  standard "NOT part of the stable public API / exposed for Unity tests"
  comment block.
- **One real cross-file collision** (`sqlite_trim_trailing_whitespace`) was
  defined in BOTH `sqlite/query.c` and `sqlite/query_helpers.c`. The helper in
  `query_helpers.c` (the canonical helper module) kept the name; the copy in
  `query.c` was renamed `sqlite_query_trim_trailing_whitespace` and declared in
  `sqlite/query.h`.
- Generic names were prefixed to keep the global namespace clean
  (`database_params_is_inside_string_literal`, `database_cache_add_entry_locked`,
  `database_connstring_extract_db2_value`, `database_watchdog_effective_timeout`,
  `database_watchdog_entry_free`, `migration_extract_ref_from_filename`). The
  already-prefixed engine helpers (`db2_*`, `mysql_*`, `postgresql_*`,
  `query_result_cache_*`) were left as-is.

### Header include fix required (Coverage config)

- `sqlite/query.h` declares `sqlite_bind_single_parameter(void* stmt, int,
  TypedParameter*, ...)` but did not include the header defining `TypedParameter`.
  Added `#include <src/database/database_params.h>` to `sqlite/query.h`. The
  Regular/Unity builds masked this via transitive includes, but the Coverage
  config (different `-I` exposure) failed until the include was added. This
  confirms the standing rule: every header declaring a function using type `T`
  must itself `#include` (directly or transitively) the header defining `T`.

### Header → function mapping - database

| Header | Functions exposed |
|--------|-------------------|
| `database_params.h` | `database_params_is_inside_string_literal` |
| `database_cache.h` | `database_cache_add_entry_locked` |
| `database_connstring.h` | `database_connstring_extract_db2_value` |
| `database_watchdog.h` | `database_watchdog_effective_timeout`, `database_watchdog_entry_free` |
| `migration/migration.h` | `migration_extract_ref_from_filename` |
| `dbqueue/query_result_cache.h` | `query_result_cache_hash_string`, `query_result_cache_compare_string_pointers`, `query_result_cache_normalize_json`, `query_result_cache_compute_template_hash`, `query_result_cache_compute_param_hash`, `query_result_cache_build_key`, `query_result_cache_bucket_index` |
| `db2/query.h` | `db2_trim_trailing_whitespace`, `db2_format_datetime_string`, `db2_format_timestamp_string`, `db2_cleanup_bound_values` |
| `mysql/query_helpers.h` | `mysql_trim_trailing_whitespace` |
| `mysql/query.h` | `mysql_cleanup_bound_values` |
| `sqlite/query_helpers.h` | `sqlite_trim_trailing_whitespace` |
| `sqlite/query.h` | `sqlite_query_trim_trailing_whitespace`, `sqlite_bind_single_parameter` |
| `postgresql/query.h` | `postgresql_is_numeric_type`, `postgresql_is_datetime_type`, `postgresql_format_timestamp_string`, `postgresql_trim_trailing_whitespace` |

### Lessons learned specific to database

1. **`sqlite/query.h` needed a `database_params.h` include** because the newly
   exposed `sqlite_bind_single_parameter` references `TypedParameter`. Same
   root cause as the mailrelay/scripting/oidc_rp Coverage-config surprises:
   a header must be self-contained for the types in its declarations.
2. **`mkt` passes Regular + Unity + Coverage; `mkp` (cppcheck) clean;** 19
   existing database Unity tests pass with 0 failures (sampled across
   `database_cache_test`, `database_connstring_test_parse_connection_string`,
   `database_params_test`, `database_watchdog_test`, `query_result_cache_test`,
   `query_test_sqlite/postgresql/mysql/db2`,
   `query_helpers_test_sqlite/mysql`, `interface_test_*`,
    `execute_load_test_execute_load_migrations`, `files_test_sort_migration_files`).

---

## config — COMPLETE

### What was done - config

- All 6 `static` function definitions in `src/config/config_oidc_rp.c` were
  converted to non-`static`.
- A matching declaration was added to `src/config/config_oidc_rp.h`, marked with
  the standard "NOT part of the stable public API / exposed for Unity tests"
  comment block.
- The names (`take_string_or_null`, `take_bool_or_default`,
  `take_int_or_default`, `provider_apply_defaults`, `provider_cleanup`,
  `provider_load_from_json`) are generic, so each was prefixed
  `config_oidc_rp_` to keep the global namespace clean and avoid any future
  redeclaration clash. No true definition collisions existed (only one
  definition each), but prefixing is consistent with the rest of the effort.
- `OIDCRPProviderConfig` is fully defined in `config_oidc_rp.h` and `json_t`
  (jansson) is available, so no extra include was required.

### Header → function mapping - config

| Header | Functions exposed |
|--------|-------------------|
| `config_oidc_rp.h` | `config_oidc_rp_take_string_or_null`, `config_oidc_rp_take_bool_or_default`, `config_oidc_rp_take_int_or_default`, `config_oidc_rp_provider_apply_defaults`, `config_oidc_rp_provider_cleanup`, `config_oidc_rp_provider_load_from_json` |

### Lessons learned specific to config

1. **Small, self-contained batch.** `config_oidc_rp.c` already `#include`s
   `config_oidc_rp.h`, and the types in the new declarations were already
   visible there, so no header-include fix was needed — unlike the database
   (`sqlite/query.h`) and oidc_rp (opaque forwards) batches.
2. **`mkt` passes Regular + Unity + Coverage; `mkp` (cppcheck) clean;** 5
   existing config Unity tests pass with 0 failures (sampled:
   `config_oidc_rp_test_load_oidc_rp_config`, `config_test_load_config`,
   `config_databases_test_load_database_config`,
    `config_mail_relay_test_load_mailrelay_config`,
    `config_utils_test_process_config_value`).

---

## status — COMPLETE

### What was done - status

- All 5 `static` function definitions across 2 `src/status/*.c` files were
  converted to non-`static`.
- Declarations added to the modules' existing headers (`status_core.h`,
  `status_process.h`), marked with the standard "NOT part of the stable public
  API / exposed for Unity tests" comment block.
- Generic names were prefixed `status_` (`status_free_cpu_metrics`,
  `status_free_network_metrics`, `status_free_service_thread_metrics`,
  `status_allocate_system_metrics`, `status_collect_process_memory`). No true
  definition collisions existed (only one definition each).
- `CpuMetrics`/`NetworkMetrics`/`ServiceThreadMetrics`/`SystemMetrics` are
  defined in `status_core.h`, which `status_process.h` already includes, so no
  extra include was required.

### Header → function mapping - status

| Header | Functions exposed |
|--------|-------------------|
| `status_core.h` | `status_free_cpu_metrics`, `status_free_network_metrics`, `status_free_service_thread_metrics`, `status_allocate_system_metrics` |
| `status_process.h` | `status_collect_process_memory` |

### Lessons learned specific to status

1. **Small, self-contained batch** — same pattern as `config`: the `.c` files
   already include their matching headers and the types were visible, so no
   header-include fix was needed. Prefixing generic names (`free_*`,
   `allocate_*`, `collect_*`) keeps the global namespace clean.
2. **`mkt` passes Regular + Unity + Coverage; `mkp` (cppcheck) clean;** 6
   existing status Unity tests pass with 0 failures (sampled:
   `status_core_test_free_system_metrics`, `status_core_test_init`,
   `status_process_test_collect_file_descriptors`,
   `status_system_test_collect_cpu_metrics`,
   `status_test_get_system_status_json`,
    `status_test_get_system_status_prometheus`).

---

## launch — COMPLETE

### What was done - launch

- All 4 `static` function definitions across 2 `src/launch/*.c` files were
  converted to non-`static`.
- `register_payload` (in `launch_payload.c`) was renamed
  `launch_register_payload` and declared in the existing `launch.h`.
- `register_mail_relay_for_launch`, `launch_fail_transport`,
  `launch_wait_for_mailrelay_otp_cache` (in `launch_mail_relay.c`) are already
  `launch_`-prefixed and unique, so they kept their names and were declared in
  a **new header** `src/launch/launch_mail_relay.h` (the `.c` previously had no
  header of its own). `launch_mail_relay.c` was updated to `#include
  "launch_mail_relay.h"` so `-Werror=missing-prototypes` is satisfied.
- `launch_mail_relay.h` includes `mailrelay_smtp.h` and `mailrelay_result.h`
  (for `MailRelaySmtpRequest` / `MailRelayResult`), which were already
  transitively visible in the `.c` but are now explicit for self-containment.
- No true definition collisions existed (only one definition each).

### Header → function mapping - launch

| Header | Functions exposed |
|--------|-------------------|
| `launch.h` | `launch_register_payload` |
| `launch_mail_relay.h` (new) | `register_mail_relay_for_launch`, `launch_fail_transport`, `launch_wait_for_mailrelay_otp_cache` |

### Lessons learned specific to launch

1. **A new header was required** (`launch_mail_relay.h`) because the `.c` had
   no matching header. This mirrors the scripting batch where new headers were
   created for `.c` files lacking one. The `.c` MUST `#include` the new header
   or `-Werror=missing-prototypes` fails the build.
2. **`mkt` passes Regular + Unity + Coverage; `mkp` (cppcheck) clean;** 5
   existing launch Unity tests pass with 0 failures (sampled:
   `launch_payload_test_check_payload_launch_readiness`,
   `launch_mail_relay_test_check_mail_relay_launch_readiness`,
   `launch_mail_relay_test_comprehensive_coverage`,
   `launch_database_test_launch_subsystem`,
    `launch_logging_test_launch_logging_subsystem`).

---

## api — COMPLETE

### What was done - api

- All 16 `static` function definitions across 8 `src/api/**/*.c` files were
  converted to non-`static`.
- A matching declaration was added to the appropriate header for each function
  (every file already had a header except `conduit/helpers/database_operations.c`,
  for which a new `conduit/helpers/database_operations.h` was created and
  `#include`d by the `.c`).
- **One real cross-file collision**: `has_valid_jwt` was defined in BOTH
  `api/system/info/info.c` and `api/conduit/status/status.c`. Each was renamed
  with a module prefix — `system_info_has_valid_jwt` and
  `conduit_status_has_valid_jwt` — before de-`static`'ing (mirroring the
  oidc_rp collision handling).
- Other generic names were prefixed for namespace safety:
  `api_service_endpoint_requires_auth/_expects_json`,
  `auth_stream_send_sse_response_headers`, `auth_chats_generate_broadcast_id`,
  `conduit_status_check_database_readiness/_get_migration_status`,
  `mailrelay_api_role_lookup_callback/_has_role_id`,
  `conduit_dbops_skip_sql_whitespace_and_comments`,
  `cap_verify_is_valid_http_url/_hard_fail/_fallback`.

### Header → function mapping - api

| Header | Functions exposed |
|--------|-------------------|
| `api/api_service.h` | `api_service_endpoint_requires_auth`, `api_service_endpoint_expects_json` |
| `api/wschat/auth_stream/auth_stream.h` | `auth_stream_send_sse_response_headers` |
| `api/system/info/info.h` | `system_info_has_valid_jwt` |
| `api/wschat/auth_chats/auth_chats.h` | `auth_chats_generate_broadcast_id` |
| `api/conduit/status/status.h` | `conduit_status_check_database_readiness`, `conduit_status_get_migration_status`, `conduit_status_has_valid_jwt` |
| `api/mailrelay/mailrelay_api_auth.h` | `mailrelay_api_role_lookup_callback`, `mailrelay_api_has_role_id` |
| `api/conduit/helpers/cap_verify.h` | `cap_verify_is_valid_http_url`, `cap_verify_hard_fail`, `cap_verify_fallback` |
| `api/conduit/helpers/database_operations.h` (new) | `conduit_dbops_skip_sql_whitespace_and_comments` |

### Header include fix required

- `mailrelay_api_auth.h` declares `mailrelay_api_role_lookup_callback(MailRelayRepoResult*, ...)`
  but did not include the header defining `MailRelayRepoResult`. Other TUs
  (e.g. `mailrelay/preview/preview.c`, `mailrelay/send/send.c`) include
  `mailrelay_api_auth.h` without transitively pulling in `mailrelay_repository.h`,
  so the build failed with "unknown type name 'MailRelayRepoResult'". Added
  `#include <src/mailrelay/mailrelay_repository.h>` to `mailrelay_api_auth.h`.
  This is the same self-containment rule that bit the database batch.

### Lessons learned specific to api

1. **`has_valid_jwt` was a real cross-file collision** — rename with module
   prefixes per file before de-`static`'ing.
2. **New header `database_operations.h`** required for the `.c` that lacked
   one (same pattern as the launch and scripting batches); the `.c` must
   `#include` it for `-Werror=missing-prototypes`.
3. **`mkt` passes Regular + Unity + Coverage; `mkp` (cppcheck) clean;** 8
   existing api Unity tests pass with 0 failures (sampled:
   `api_service_test_is_api_endpoint`, `auth_chats_test_handle_auth_chats_request`,
   `info_test_handle_system_info_request`, `status_test_handle_conduit_status_request`,
   `cap_verify_test`, `database_operations_test`, `conduit_service_test`,
    `mailrelay_get_status_test`).

---

## handlers — COMPLETE

### What was done - handlers

- All 3 `static` function definitions in `src/handlers/handlers.c` were
  converted to non-`static`.
- Declarations added to `src/handlers/handlers.h`, marked with the standard
  "NOT part of the stable public API / exposed for Unity tests" comment block.
- Generic names were prefixed `handlers_` (`handlers_read_proc_maps`,
  `handlers_dump_segment`, `handlers_read_auxv_data`). No true definition
  collisions existed (only one definition each).
- **Anonymous structs promoted to named tags.** `CoreMapping` and `LoadSegment`
  were anonymous typedefs defined only in `handlers.c`; they were promoted to
  `typedef struct CoreMapping { ... } CoreMapping;` (and `LoadSegment`) and
  forward-declared opaque in `handlers.h`. This mirrors the oidc_rp/scripting
  pattern for file-private structs passed by pointer.

### Header → function mapping - handlers

| Header | Functions exposed |
|--------|-------------------|
| `handlers.h` (opaque `CoreMapping`, `LoadSegment`) | `handlers_read_proc_maps`, `handlers_dump_segment`, `handlers_read_auxv_data` |

### Lessons learned specific to handlers

1. **Promote anonymous file-private structs to named tags** when exposing
   pointer-taking helpers, then forward-declare opaque in the header.
2. **`mkt` passes Regular + Unity + Coverage; `mkp` (cppcheck) clean;** 4
   existing handler Unity tests pass with 0 failures (sampled:
   `handlers_test_config_dump_handler`, `landing_test_signal_handlers`,
    `state_test_signal_handler`, `swagger_test_request_handler`).

---

## terminal / websocket / webserver — COMPLETE (final batch)

### What was done - terminal

- The last 3 `static` function definitions were converted to non-`static`:
  - `src/terminal/terminal_websocket.c` → `terminal_io_bridge_thread`
    (declared in `terminal/terminal_websocket.h`, inside the existing
    `extern "C"` block).
  - `src/webserver/web_server_request.c` → `web_server_url_looks_like_file`
    (renamed from `url_looks_like_file`; declared in `web_server_request.h`).
  - `src/websocket/websocket_server_pty.c` → `pty_bridge_iteration` (already
    declared in `websocket_server_pty.h` — just de-`static`'d).

### Two issues found and fixed during this batch

1. **`websocket_server_pty.c` did not `#include "websocket_server_pty.h"`.**
   The header already declared `pty_bridge_iteration` (plus
   `pty_output_bridge_thread` / `start_pty_bridge_thread`), but the TU never
   included it, so de-`static`'ing the definition triggered
   `-Werror=missing-prototypes`. Added the include.
2. **`websocket_server_pty.c` redefined `struct PtyBridgeContext`** (duplicating
   the definition already in the header). This caused a "redefinition of
   'struct PtyBridgeContext'" error once the header was included. Removed the
   duplicate `typedef struct PtyBridgeContext { ... } PtyBridgeContext;` from
   the `.c` (the header's definition is canonical and identical).

### Remaining `static` in `src/` (intentional, NOT to be converted)

After this batch, exactly 4 `static` definitions remain in `src/`, all of
which are module-private *state*, not testable callable helpers:

- `src/utils/utils.c` — `init_utils` (`__attribute__((constructor))` startup
  init, file-scope state)
- `src/hydrogen.h` — `reallocarray` (`static inline`, benign; excluded by the
  original doc scope)
- `src/mailrelay/mailrelay_test_seams.c` — `default_seam_time`,
  `default_seam_message_id` (test-seam callbacks, intentional per the
  mailrelay batch)

These are correct and should be left `static`.

### Lessons learned specific to terminal/websocket/webserver

1. **Before de-`static`'ing, confirm the `.c` actually includes the header
   that holds (or will hold) the declaration.** A pre-existing mismatched
   declaration + missing include is exactly the failure mode `-Werror` catches.
2. **Remove duplicate struct definitions** when adding the include that
   surfaces them.
3. **`mkt` passes Regular + Unity + Coverage; `mkp` (cppcheck) clean;** 6
   terminal/websocket/webserver Unity tests pass with 0 failures (sampled:
   `config_webserver_test_load_webserver_config`,
   `config_websocket_test_load_websocket_config`,
   `config_terminal_test_load_terminal_config`,
   `launch_webserver_test_check_webserver_launch_readiness`,
   `launch_websocket_test_check_websocket_launch_readiness`,
   `terminal_session_test_session_management`).

---

## OVERALL STATUS — COMPLETE

Every testable `static` helper in `src/` has been de-`static`'d and given a
visible header declaration, across:

| Module group | `static` functions | Status |
|--------------|-------------------|--------|
| mailrelay | 146 | ✅ |
| scripting | 54 | ✅ |
| oidc_rp | 96 | ✅ |
| wschat/helpers | 28 | ✅ |
| utils | 10 | ✅ |
| logging | 11 | ✅ |
| database | 26 | ✅ |
| config | 6 | ✅ |
| status | 5 | ✅ |
| launch | 4 | ✅ |
| api | 16 | ✅ |
| handlers | 3 | ✅ |
| terminal / websocket / webserver | 3 | ✅ |

Total: **408 helper functions** converted. The only remaining `static`
definitions are intentional module-private state (constructor, inline util,
test-seam callbacks). The automated static-function gate in `mkt` continues to
enforce the rule for all future code.

The grandfather baseline `tests/.static-baseline.txt` has been shrunk from the
original 228 pre-existing names down to just the **4** remaining intentional
`static` functions (`__attribute__` — the extracted name for `init_utils`'s
`__attribute__((constructor))` form, `reallocarray`, `default_seam_time`,
`default_seam_message_id`). Any new `static` function added to `src/` is now
immediately flagged by the gate; if a future genuinely module-private `static`
is needed, regenerate with the documented command.

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

---

## Collision detail (reference — oidc_rp, REAL cross-file collisions)

These names were defined in ≥2 **different** `.c` files (true linker
collisions once un-`static`'d). They were renamed with a module-scoped
prefix BEFORE de-`static`'ing:

| Original name | Renamed to | File |
|---------------|-----------|------|
| `scrub_free` | `callback_scrub_free` | `oidc_rp_callback.c` |
| `scrub_free` | `state_scrub_free` | `oidc_rp_state.c` |
| `scrub_free` | `token_scrub_free` | `oidc_rp_token.c` |
| `truncated_state` | `callback_truncated_state` | `oidc_rp_callback.c` |
| `truncated_state` | `start_truncated_state` | `oidc_rp_start.c` |
| `entry_expired` | `discovery_entry_expired` | `oidc_rp_discovery.c` |
| `entry_expired` | `jwks_entry_expired` | `oidc_rp_jwks.c` |
| `entry_expired` | `state_entry_expired` | `oidc_rp_state.c` |
| `find_slot_locked` | `discovery_find_slot_locked` | `oidc_rp_discovery.c` |
| `find_slot_locked` | `jwks_find_slot_locked` | `oidc_rp_jwks.c` |
| `find_empty_slot_locked` | `discovery_find_empty_slot_locked` | `oidc_rp_discovery.c` |
| `find_empty_slot_locked` | `jwks_find_empty_slot_locked` | `oidc_rp_jwks.c` |

Note: `oidc_rp_handoff_store.c` helpers were already `handoff_`-prefixed in
the original code, so they did not collide with the `oidc_rp_state.c`
equivalents and needed no rename.
