# Unity Cleanup — Disabled Tests

Catalogue of Unity unit tests that are currently disabled via `if (0) RUN_TEST(...)`. Each entry records the test function, its file/line, and any inline reason. This file is the canonical to-do list for reviewing and either re-enabling, fixing, or removing these tests.

Generated: 2026-07-26

Total disabled tests: 43 (all reviewed — cleanup complete)

---

## Logging — DONE (re-enabled, all pass)

| # | File | Line | Test Function | Resolution |
| --- | ------ | ------ | --------------- | -------- |
| 1 | `logging_test_basic.c` | — | `test_count_format_specifiers_single_specifier` | Re-enabled; matches `count_format_specifiers` |
| 2 | `logging_test_basic.c` | — | `test_count_format_specifiers_multiple_specifiers` | Re-enabled |
| 3 | `logging_test_basic.c` | — | `test_count_format_specifiers_mixed` | Re-enabled |
| 4 | `logging_test_basic.c` | — | `test_get_fallback_priority_label_valid_priorities` | Re-enabled; matches fallback labels |
| 5 | `logging_test_basic.c` | — | `test_get_fallback_priority_label_invalid_priority` | Re-enabled; invalid → STATE |

---

## Landing — DONE

| # | File | Test Function | Resolution |
| --- | ------ | --------------- | -------- |
| 6 | `landing_api_test_readiness.c` | `both_running` | Re-enabled; mock_landing works |
| 7–8 | `landing_mdns_client_test_readiness.c` | network/logging not running | Re-enabled |
| 9 | `landing_payload_…` | `memory_allocation_failure` | **Removed** — stub TEST_IGNORE only; malloc not interceptable in prebuilt landing_*.o |
| 10 | `landing_swagger_…` | `webserver_not_running` | Re-enabled; fixed expectations to match early-exit messages |
| 11 | `landing_terminal_…` | `malloc_failure` | **Removed** — same as #9 |
| 12–13 | `landing_test_check_all_landing_readiness.c` | shutdown/restart success | Re-enabled; real registry + `restart_requested`; `UNITY_TEST_MODE` skips `exit(0)`; weak `startup_hydrogen` |
| 14–18 | `landing_test_land_approved_subsystems.c` | single/multi/skip cases | Re-enabled via real `init_registry`/`register_subsystem`; assert states after land |

Source: `landing.c` (`UNITY_TEST_MODE` return before process teardown/exit); `launch.c` (`startup_hydrogen` weak under `UNITY_TEST_MODE`).

---

## Launch — DONE

| # | File | Test Function | Resolution |
| --- | ------ | --------------- | -------- |
| 19 | `launch_database_test_launch_subsystem.c` | `basic_functionality` | Re-enabled; mock_strdup already NULL-safe |
| 20 | same | `null_config` | Re-enabled; NULL guard in `launch_database_subsystem` |
| 21–23 | `launch_logging_test_check_logging_launch_readiness.c` | console/file disabled, successful | Real `init_registry` + register Logging |
| 24 | `launch_logging_test_launch_logging_subsystem.c` | successful launch | Real registry register Logging |
| 25 | `launch_oidc_…_with_registry.c` | disabled with registry | Real register Registry (launchable once registered) |
| 26 | `launch_swagger_test_validation.c` | valid_configuration | Calls `validate_swagger_configuration` (full readiness needs API/Payload/payload binary) |

Source: `launch_database.c` NULL `app_config` guard.

---

## WebServer — DONE

| # | File | Test Function | Resolution |
| --- | ------ | --------------- | -------- |
| 27 | `web_server_upload_test_handle_upload_request.c` | `file_upload_completed` | Re-enabled; fixed `mkstemps` suffix len `.gcode` → 6 |

---

## Terminal — DONE

| # | File | Test Function | Resolution |
| --- | ------ | --------------- | -------- |
| 28 | `terminal_shell_test_mock_failures.c` | `pty_spawn_shell_fork_failure` | Re-enabled; `UNITY_TEST_MODE` skips real `fork()` when force-fail is set |

Source: `terminal_shell.c` — force-fork-failure before calling `fork()`.

---

## Scripting — DONE

| # | File | Test Function | Resolution |
| --- | ------ | --------------- | -------- |
| 29 | `scripting_handle_test_lifecycle.c` | `handle_gc_frees_handle` | Re-enabled; passes with current `__gc` / `H_Handle_release` |

---

## API — Auth / Renew — DONE

| # | File | Test Function | Resolution |
| --- | ------ | --------------- | -------- |
| 30 | `renew_utils_test_validate_token_and_extract_claims.c` | success | Re-enabled via `mock_auth_service_jwt`; `validate_jwt_token` alias + include in `renew_utils.c` |

---

## API — OIDC — DONE

| # | File | Test Function | Resolution |
| --- | ------ | --------------- | -------- |
| 31–32 | `oidc_service_test_register_oidc_endpoints.c` | full endpoint table | Re-enabled; stable `g_filler_prefixes[]` (was stack pointers) |

---

## API — Conduit — DONE

| # | File | Test Function | Resolution |
| --- | ------ | --------------- | -------- |
| 33 | `alt_queries_test_execute_single_alt_query.c` | `with_params` | **Removed** — never implemented body; needs full DB queue |

---

## Database — Connection / Pool — DONE

| # | File | Test Function | Resolution |
| --- | ------ | --------------- | -------- |
| 34 | `database_connstring_test_global_pool.c` | init malloc failure | Re-enabled; runs first (static global) |
| 35–37 | `database_connstring_test_pool.c` | create malloc/strdup/connections fail | Re-enabled; `USE_MOCK_SYSTEM` works |
| 38–39 | `database_connstring_test_pool_manager.c` | manager malloc fails | Re-enabled |

---

## Database — MySQL — DONE

| # | File | Test Function | Resolution |
| --- | ------ | --------------- | -------- |
| 40 | `query_test_edge_cases_mysql.c` | prepared execution failure | Re-enabled; `mock_libmysqlclient_set_mysql_stmt_execute_result` |
| 41 | same | null column data | **Removed** — TEST_IGNORE stub only |
| 42 | `query_test_coverage_mysql.c` | prepared with result set | **Removed** — TEST_IGNORE stub only |

---

## Database — DBQueue — DONE

| # | File | Test Function | Resolution |
| --- | ------ | --------------- | -------- |
| 43 | `lead_load_test_execute_migration_load.c` | null queue | Re-enabled; NULL guard in `lead_load.c` |

---

## Summary by subsystem

| Subsystem | Count |
| ----------- | ------- |
| Logging | 5 |
| Landing | 13 |
| Launch | 8 |
| WebServer | 1 |
| Terminal | 1 |
| Scripting | 1 |
| API — Auth / Renew | 1 |
| API — OIDC | 2 |
| API — Conduit | 1 |
| Database — Connection / Pool | 6 |
| Database — MySQL | 3 |
| Database — DBQueue | 1 |
