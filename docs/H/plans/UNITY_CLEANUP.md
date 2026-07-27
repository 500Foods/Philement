# Unity Cleanup — Disabled Tests

Catalogue of Unity unit tests that are currently disabled via `if (0) RUN_TEST(...)`. Each entry records the test function, its file/line, and any inline reason. This file is the canonical to-do list for reviewing and either re-enabling, fixing, or removing these tests.

Generated: 2026-07-26

Total disabled tests: 43

---

## Logging

| # | File | Line | Test Function | Reason |
| --- | ------ | ------ | --------------- | -------- |
| 1 | `/elements/001-hydrogen/hydrogen/tests/unity/src/logging/logging_test_basic.c` | 126 | `test_count_format_specifiers_single_specifier` | — |
| 2 | `/elements/001-hydrogen/hydrogen/tests/unity/src/logging/logging_test_basic.c` | 127 | `test_count_format_specifiers_multiple_specifiers` | — |
| 3 | `/elements/001-hydrogen/hydrogen/tests/unity/src/logging/logging_test_basic.c` | 129 | `test_count_format_specifiers_mixed` | — |
| 4 | `/elements/001-hydrogen/hydrogen/tests/unity/src/logging/logging_test_basic.c` | 132 | `test_get_fallback_priority_label_valid_priorities` | — |
| 5 | `/elements/001-hydrogen/hydrogen/tests/unity/src/logging/logging_test_basic.c` | 133 | `test_get_fallback_priority_label_invalid_priority` | — |

---

## Landing

| # | File | Line | Test Function | Reason |
| --- | ------ | ------ | --------------- | -------- |
| 6 | `/elements/001-hydrogen/hydrogen/tests/unity/src/landing/landing_api_test_readiness.c` | 100 | `test_check_api_landing_readiness_both_running` | — |
| 7 | `/elements/001-hydrogen/hydrogen/tests/unity/src/landing/landing_mdns_client_test_readiness.c` | 142 | `test_check_mdns_client_landing_readiness_network_not_running` | — |
| 8 | `/elements/001-hydrogen/hydrogen/tests/unity/src/landing/landing_mdns_client_test_readiness.c` | 143 | `test_check_mdns_client_landing_readiness_logging_not_running` | — |
| 9 | `/elements/001-hydrogen/hydrogen/tests/unity/src/landing/landing_payload_test_check_payload_landing_readiness.c` | 102 | `test_check_payload_landing_readiness_memory_allocation_failure` | — |
| 10 | `/elements/001-hydrogen/hydrogen/tests/unity/src/landing/landing_swagger_test_check_swagger_landing_readiness.c` | 163 | `test_check_swagger_landing_readiness_webserver_not_running` | — |
| 11 | `/elements/001-hydrogen/hydrogen/tests/unity/src/landing/landing_terminal_test_check_terminal_landing_readiness.c` | 180 | `test_check_terminal_landing_readiness_malloc_failure` | — |
| 12 | `/elements/001-hydrogen/hydrogen/tests/unity/src/landing/landing_test_check_all_landing_readiness.c` | 238 | `test_check_all_landing_readiness_shutdown_success` | — |
| 13 | `/elements/001-hydrogen/hydrogen/tests/unity/src/landing/landing_test_check_all_landing_readiness.c` | 239 | `test_check_all_landing_readiness_restart_success` | — |
| 14 | `/elements/001-hydrogen/hydrogen/tests/unity/src/landing/landing_test_land_approved_subsystems.c` | 224 | `test_land_approved_subsystems_single_ready_subsystem` | — |
| 15 | `/elements/001-hydrogen/hydrogen/tests/unity/src/landing/landing_test_land_approved_subsystems.c` | 225 | `test_land_approved_subsystems_multiple_ready_subsystems` | — |
| 16 | `/elements/001-hydrogen/hydrogen/tests/unity/src/landing/landing_test_land_approved_subsystems.c` | 226 | `test_land_approved_subsystems_registry_skipped` | — |
| 17 | `/elements/001-hydrogen/hydrogen/tests/unity/src/landing/landing_test_land_approved_subsystems.c` | 227 | `test_land_approved_subsystems_not_ready_subsystems_skipped` | — |
| 18 | `/elements/001-hydrogen/hydrogen/tests/unity/src/landing/landing_test_land_approved_subsystems.c` | 228 | `test_land_approved_subsystems_unknown_subsystem_skipped` | — |

---

## Launch

| # | File | Line | Test Function | Reason |
| --- | ------ | ------ | --------------- | -------- |
| 19 | `/elements/001-hydrogen/hydrogen/tests/unity/src/launch/launch_database_test_launch_subsystem.c` | 176 | `test_launch_database_subsystem_basic_functionality` | Disabled: SEGFAULT – mock_strdup(NULL) issue |
| 20 | `/elements/001-hydrogen/hydrogen/tests/unity/src/launch/launch_database_test_launch_subsystem.c` | 180 | `test_launch_database_subsystem_null_config` | Disabled: needs NULL check in source |
| 21 | `/elements/001-hydrogen/hydrogen/tests/unity/src/launch/launch_logging_test_check_logging_launch_readiness.c` | 286 | `test_check_logging_launch_readiness_console_disabled` | Disabled: Mock registry interaction not working |
| 22 | `/elements/001-hydrogen/hydrogen/tests/unity/src/launch/launch_logging_test_check_logging_launch_readiness.c` | 287 | `test_check_logging_launch_readiness_file_disabled` | Disabled: Mock registry interaction not working |
| 23 | `/elements/001-hydrogen/hydrogen/tests/unity/src/launch/launch_logging_test_check_logging_launch_readiness.c` | 290 | `test_check_logging_launch_readiness_successful` | Disabled: Mock registry interaction not working |
| 24 | `/elements/001-hydrogen/hydrogen/tests/unity/src/launch/launch_logging_test_launch_logging_subsystem.c` | 74 | `test_launch_logging_subsystem_successful_launch` | Disabled: Mock registry interaction not working |
| 25 | `/elements/001-hydrogen/hydrogen/tests/unity/src/launch/launch_oidc_test_check_oidc_launch_readiness_with_registry.c` | 80 | `test_check_oidc_launch_readiness_disabled_with_registry_mock` | Disabled: Mock registry interaction not working |
| 26 | `/elements/001-hydrogen/hydrogen/tests/unity/src/launch/launch_swagger_test_validation.c` | 358 | `test_check_swagger_launch_readiness_valid_configuration` | Disabled: Mock registry interaction not working |

---

## WebServer

| # | File | Line | Test Function | Reason |
| --- | ------ | ------ | --------------- | -------- |
| 27 | `/elements/001-hydrogen/hydrogen/tests/unity/src/webserver/web_server_upload_test_handle_upload_request.c` | 171 | `test_handle_upload_request_file_upload_completed` | — |

---

## Terminal

| # | File | Line | Test Function | Reason |
| --- | ------ | ------ | --------------- | -------- |
| 28 | `/elements/001-hydrogen/hydrogen/tests/unity/src/terminal/terminal_shell_test_mock_failures.c` | 224 | `test_pty_spawn_shell_fork_failure` | — |

---

## Scripting

| # | File | Line | Test Function | Reason |
| --- | ------ | ------ | --------------- | -------- |
| 29 | `/elements/001-hydrogen/hydrogen/tests/unity/src/scripting/scripting_handle_test_lifecycle.c` | 192 | `test_handle_gc_frees_handle` | GC test deferred |

---

## API — Auth / Renew

| # | File | Line | Test Function | Reason |
| --- | ------ | ------ | --------------- | -------- |
| 30 | `/elements/001-hydrogen/hydrogen/tests/unity/src/api/auth/renew/renew_utils_test_validate_token_and_extract_claims.c` | 137 | `test_validate_token_and_extract_claims_success` | — |

---

## API — OIDC

| # | File | Line | Test Function | Reason |
| --- | ------ | ------ | --------------- | -------- |
| 31 | `/elements/001-hydrogen/hydrogen/tests/unity/src/api/oidc/oidc_service_test_register_oidc_endpoints.c` | 139 | `test_register_well_known_fails_when_full` | — |
| 32 | `/elements/001-hydrogen/hydrogen/tests/unity/src/api/oidc/oidc_service_test_register_oidc_endpoints.c` | 140 | `test_register_oauth_fails_when_full` | — |

---

## API — Conduit

| # | File | Line | Test Function | Reason |
| --- | ------ | ------ | --------------- | -------- |
| 33 | `/elements/001-hydrogen/hydrogen/tests/unity/src/api/conduit/alt_queries/alt_queries_test_execute_single_alt_query.c` | 115 | `test_execute_single_alt_query_with_params` | SKIPPED – requires full database queue setup |

---

## Database — Connection / Pool

| # | File | Line | Test Function | Reason |
| --- | ------ | ------ | --------------- | -------- |
| 34 | `/elements/001-hydrogen/hydrogen/tests/unity/src/database/database_connstring_test_global_pool.c` | 172 | `test_connection_pool_system_init_malloc_failure` | Disabled: global state already initialized |
| 35 | `/elements/001-hydrogen/hydrogen/tests/unity/src/database/database_connstring_test_pool.c` | 228 | `test_connection_pool_create_malloc_failure` | Disabled: unreliable due to system allocations |
| 36 | `/elements/001-hydrogen/hydrogen/tests/unity/src/database/database_connstring_test_pool.c` | 229 | `test_connection_pool_create_strdup_failure` | Disabled: unreliable due to system allocations |
| 37 | `/elements/001-hydrogen/hydrogen/tests/unity/src/database/database_connstring_test_pool.c` | 230 | `test_connection_pool_create_connections_malloc_failure` | Disabled: unreliable due to system allocations |
| 38 | `/elements/001-hydrogen/hydrogen/tests/unity/src/database/database_connstring_test_pool_manager.c` | 242 | `test_connection_pool_manager_create_malloc_failure` | Disabled: unreliable due to system allocations |
| 39 | `/elements/001-hydrogen/hydrogen/tests/unity/src/database/database_connstring_test_pool_manager.c` | 243 | `test_connection_pool_manager_create_pools_malloc_failure` | Disabled: unreliable due to system allocations |

---

## Database — MySQL

| # | File | Line | Test Function | Reason |
| --- | ------ | ------ | --------------- | -------- |
| 40 | `/elements/001-hydrogen/hydrogen/tests/unity/src/database/mysql/query_test_edge_cases_mysql.c` | 425 | `test_mysql_execute_prepared_execution_failure` | — |
| 41 | `/elements/001-hydrogen/hydrogen/tests/unity/src/database/mysql/query_test_edge_cases_mysql.c` | 428 | `test_mysql_execute_prepared_null_column_data` | — |
| 42 | `/elements/001-hydrogen/hydrogen/tests/unity/src/database/mysql/query_test_coverage_mysql.c` | 644 | `test_mysql_execute_prepared_with_result_set` | Skipped – mock limitation |

---

## Database — DBQueue

| # | File | Line | Test Function | Reason |
| --- | ------ | ------ | --------------- | -------- |
| 43 | `/elements/001-hydrogen/hydrogen/tests/unity/src/database/dbqueue/lead_load_test_execute_migration_load.c` | 83 | `test_database_queue_lead_execute_migration_load_null_queue` | — |

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
