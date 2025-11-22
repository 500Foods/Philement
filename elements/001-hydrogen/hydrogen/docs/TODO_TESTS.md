# TODO: Tests

These are tetst that have been created but have been excluded, likely because they are either failing or causing a segfault.

At some point, we should review these all and either...

- Fix the test so that the test doesn't fail/crash
- Fix the code so that the test doesn't fail/crash
- Remove the test entirely if it isn't testing anything useful

## Instructions

This is for the AI Model to use to resolve these failing tests.

### Review Background information

- Review INSTRUCTIONS.md
- Review tests/README.md
- Review tests/UNITY.md

NOTE: Some tests are old and might have been difficult to get working when they were first created.
Since then, many additional mock libraries have been added, and thousands of additional tests are
now available and working that we can look at for examples of how to write far more complex tests.

NOTE: Most tests are well-contained, meaning that the result of one test is not in any way dependeent
on the presence or success/failure of another test, certainly in other files but also within the same
file. The files are used primarily to group the tests and the dependencies. But this isn't always so.

### Examine one test at a time

- Run the test with `mku <testname>` to confirm that all the other tests in the file are currently all passing
- Review what the test is trying to accomplish and the resources it needs to work effectively (mocks and so on)
- Make a determination about whether the failing test is valuable enough to keep in the project (default to yes if undecided)

### Remove invaluable tests

- Remove the `RUN_TEST` line and the test function that it is calling
- Run the test again with `mku <testname>` to ensure the removal didn't break other tests

### Fix failing tests

- Remove the `if (0)` from the beginning of the individual test line
- Run the test again with `mku <testname>` to confirm either a FAIL result for the individual test, or a segfault
- Fix the test
- Run the test again with `mku <testname>` to confirm a working test
- Do not leave the test in a non-buildable state or with any tests failing or segfaulting

### After fixing or removing test

- Update TODO_TESTS.md to remove the test that has been corrected or removed
- Start again with the next test in the section being worked on
- Stop when that section is complete

## Launch

NOTE: Launch tests require registry/subsystem mocking which has architectural limitations.
Tests that verify readiness with mock registry state remain disabled until better mock integration is available.

- tests/unity/src/launch/launch_logging_test_check_logging_launch_readiness.c:286:    if (0) RUN_TEST(test_check_logging_launch_readiness_console_disabled);  
- tests/unity/src/launch/launch_logging_test_check_logging_launch_readiness.c:287:    if (0) RUN_TEST(test_check_logging_launch_readiness_file_disabled);
- tests/unity/src/launch/launch_logging_test_check_logging_launch_readiness.c:290:    if (0) RUN_TEST(test_check_logging_launch_readiness_successful);
- tests/unity/src/launch/launch_logging_test_launch_logging_subsystem.c:78:    if (0) RUN_TEST(test_launch_logging_subsystem_successful_launch);
- tests/unity/src/launch/launch_oidc_test_check_oidc_launch_readiness_with_registry.c:80:    if (0) RUN_TEST(test_check_oidc_launch_readiness_disabled_with_registry_mock);
- tests/unity/src/launch/launch_swagger_test_validation.c:358:    if (0) RUN_TEST(test_check_swagger_launch_readiness_valid_configuration);

## Landing

- tests/unity/src/landing/landing_api_test_readiness.c:100:    if (0) RUN_TEST(test_check_api_landing_readiness_both_running);
- tests/unity/src/landing/landing_test_land_approved_subsystems.c:224:    if (0) RUN_TEST(test_land_approved_subsystems_single_ready_subsystem);
- tests/unity/src/landing/landing_test_land_approved_subsystems.c:225:    if (0) RUN_TEST(test_land_approved_subsystems_multiple_ready_subsystems);
- tests/unity/src/landing/landing_test_land_approved_subsystems.c:226:    if (0) RUN_TEST(test_land_approved_subsystems_registry_skipped);
- tests/unity/src/landing/landing_test_land_approved_subsystems.c:227:    if (0) RUN_TEST(test_land_approved_subsystems_not_ready_subsystems_skipped);
- tests/unity/src/landing/landing_test_land_approved_subsystems.c:228:    if (0) RUN_TEST(test_land_approved_subsystems_unknown_subsystem_skipped);
- tests/unity/src/landing/landing_test_check_all_landing_readiness.c:238:    if (0) RUN_TEST(test_check_all_landing_readiness_shutdown_success);
- tests/unity/src/landing/landing_test_check_all_landing_readiness.c:239:    if (0) RUN_TEST(test_check_all_landing_readiness_restart_success);
- tests/unity/src/landing/landing_terminal_test_check_terminal_landing_readiness.c:180:    if (0) RUN_TEST(test_check_terminal_landing_readiness_malloc_failure);
- tests/unity/src/landing/landing_swagger_test_check_swagger_landing_readiness.c:163:    if (0) RUN_TEST(test_check_swagger_landing_readiness_webserver_not_running);
- tests/unity/src/landing/landing_payload_test_check_payload_landing_readiness.c:102:    if (0) RUN_TEST(test_check_payload_landing_readiness_memory_allocation_failure);
- tests/unity/src/landing/landing_mdns_client_test_readiness.c:142:    if (0) RUN_TEST(test_check_mdns_client_landing_readiness_network_not_running);
- tests/unity/src/landing/landing_mdns_client_test_readiness.c:143:    if (0) RUN_TEST(test_check_mdns_client_landing_readiness_logging_not_running);

## Logging

- tests/unity/src/logging/logging_test_basic.c:126:    if (0) RUN_TEST(test_count_format_specifiers_single_specifier);
- tests/unity/src/logging/logging_test_basic.c:127:    if (0) RUN_TEST(test_count_format_specifiers_multiple_specifiers);
- tests/unity/src/logging/logging_test_basic.c:129:    if (0) RUN_TEST(test_count_format_specifiers_mixed);
- tests/unity/src/logging/logging_test_basic.c:132:    if (0) RUN_TEST(test_get_fallback_priority_label_valid_priorities);
- tests/unity/src/logging/logging_test_basic.c:133:    if (0) RUN_TEST(test_get_fallback_priority_label_invalid_priority);

## mDNS Server

- tests/unity/src/mdns/mdns_server_init_test.c:814:    if (0) RUN_TEST(test_mdns_server_init_null_services_array); // Disabled due to double free issues - needs investigation
- tests/unity/src/mdns/mdns_server_init_test.c:815:    if (0) RUN_TEST(test_mdns_server_init_network_failure);
- tests/unity/src/mdns/mdns_server_init_test.c:816:    if (0) RUN_TEST(test_mdns_server_init_malloc_failure);
- tests/unity/src/mdns/mdns_server_init_test.c:817:    if (0) RUN_TEST(test_mdns_server_init_socket_failure);
- tests/unity/src/mdns/mdns_server_init_test.c:818:    if (0) RUN_TEST(test_mdns_server_init_hostname_failure);
- tests/unity/src/mdns/mdns_server_init_test_setup_hostname.c:91:    if (0) RUN_TEST(test_mdns_server_setup_hostname_gethostname_failure);
- tests/unity/src/mdns/mdns_server_init_test_setup_hostname.c:92:    if (0) RUN_TEST(test_mdns_server_setup_hostname_malloc_failure);
- tests/unity/src/mdns/mdns_server_init_test_allocate.c:64:    if (0) RUN_TEST(test_mdns_server_allocate_malloc_failure);

## Database_Engine

- tests/unity/src/database/database_engine_test_comprehensive.c:685:    if (0) RUN_TEST(test_database_engine_register_basic);
- tests/unity/src/database/database_engine_test_comprehensive.c:686:    if (0) RUN_TEST(test_database_engine_register_null_engine);
- tests/unity/src/database/database_engine_test_comprehensive.c:687:    if (0) RUN_TEST(test_database_engine_register_invalid_type);
- tests/unity/src/database/database_engine_test_comprehensive.c:688:    if (0) RUN_TEST(test_database_engine_register_already_registered);
- tests/unity/src/database/database_engine_test_comprehensive.c:689:    if (0) RUN_TEST(test_database_engine_register_already_registered_independent);
- tests/unity/src/database/database_engine_test_transaction.c:429:    if (0) RUN_TEST(test_database_engine_begin_transaction_no_engine);
- tests/unity/src/database/database_engine_test_transaction.c:433:    if (0) RUN_TEST(test_database_engine_commit_transaction_null_transaction);
- tests/unity/src/database/database_engine_test_transaction.c:434:    if (0) RUN_TEST(test_database_engine_commit_transaction_no_engine);
- tests/unity/src/database/database_engine_test_transaction.c:436:    if (0) RUN_TEST(test_database_engine_rollback_transaction_basic);
- tests/unity/src/database/database_engine_test_transaction.c:438:    if (0) RUN_TEST(test_database_engine_rollback_transaction_null_transaction);
- tests/unity/src/database/database_engine_test_transaction.c:439:    if (0) RUN_TEST(test_database_engine_rollback_transaction_no_engine);
- tests/unity/src/database/database_engine_test_connect.c:300:    if (0) RUN_TEST(test_database_engine_connect_with_designator_basic);
- tests/unity/src/database/database_engine_test_connect.c:301:    if (0) RUN_TEST(test_database_engine_connect_with_designator_null_designator);

## Database_Bootstrap

- tests/unity/src/database/database_bootstrap_test_execute_bootstrap_query.c:72:    if (0) RUN_TEST(test_database_queue_execute_bootstrap_query_lead_queue_no_connection);
- tests/unity/src/database/database_bootstrap_test_execute_bootstrap_query_full.c:867:    if (0) RUN_TEST(test_lead_queue_no_connection);
- tests/unity/src/database/database_bootstrap_test_execute_bootstrap_query_full.c:869:    if (0) RUN_TEST(test_query_id_allocation_failure);
- tests/unity/src/database/database_bootstrap_test_execute_bootstrap_query_full.c:870:    if (0) RUN_TEST(test_sql_template_allocation_failure);
- tests/unity/src/database/database_bootstrap_test_execute_bootstrap_query_full.c:871:    if (0) RUN_TEST(test_parameters_json_allocation_failure);
- tests/unity/src/database/database_bootstrap_test_execute_bootstrap_query_full.c:872:    if (0) RUN_TEST(test_query_execution_failure);
- tests/unity/src/database/database_bootstrap_test_execute_bootstrap_query_full.c:873:    if (0) RUN_TEST(test_successful_execution_no_qtc);
- tests/unity/src/database/database_bootstrap_test_execute_bootstrap_query_full.c:874:    if (0) RUN_TEST(test_successful_execution_with_qtc);
- tests/unity/src/database/database_bootstrap_test_execute_bootstrap_query_full.c:875:    if (0) RUN_TEST(test_qtc_creation_failure);
- tests/unity/src/database/database_bootstrap_test_execute_bootstrap_query_full.c:876:    if (0) RUN_TEST(test_qtc_entry_creation_failure);
- tests/unity/src/database/database_bootstrap_test_execute_bootstrap_query_full.c:877:    if (0) RUN_TEST(test_migration_tracking_available);
- tests/unity/src/database/database_bootstrap_test_execute_bootstrap_query_full.c:878:    if (0) RUN_TEST(test_migration_tracking_installed);
- tests/unity/src/database/database_bootstrap_test_execute_bootstrap_query_full.c:879:    if (0) RUN_TEST(test_migration_tracking_mixed);
- tests/unity/src/database/database_bootstrap_test_execute_bootstrap_query_full.c:880:    if (0) RUN_TEST(test_bootstrap_completion_signaling);
- tests/unity/src/database/database_bootstrap_test_execute_bootstrap_query_full.c:881:    if (0) RUN_TEST(test_empty_database_detection);

## Database/DBQueue

- tests/unity/src/database/dbqueue/process_test_missing_coverage.c:398:    if (0) RUN_TEST(test_database_queue_process_single_query_no_connection);
- tests/unity/src/database/dbqueue/process_test_missing_coverage.c:400:    if (0) RUN_TEST(test_database_queue_manage_child_queues_scaling_down); // Disabled due to segfault in queue destruction
- tests/unity/src/database/dbqueue/process_test_missing_coverage.c:404:    if (0) RUN_TEST(test_database_queue_worker_thread_main_loop_processing);
- tests/unity/src/database/dbqueue/heartbeat_test_perform_heartbeat.c:118:    if (0) RUN_TEST(test_database_queue_perform_heartbeat_with_connection);
- tests/unity/src/database/dbqueue/heartbeat_test_perform_heartbeat.c:119:    if (0) RUN_TEST(test_database_queue_perform_heartbeat_connection_states);
- tests/unity/src/database/dbqueue/heartbeat_test_check_connection.c:144:    if (0) RUN_TEST(test_database_queue_check_connection_sqlite_format);

## Database/Migration

- tests/unity/src/database/migration/transaction_test_execute_transaction.c:495:    if (0) RUN_TEST(test_parse_sql_statements_strdup_failure);  // Disabled: malloc mocking not supported
- tests/unity/src/database/migration/transaction_test_execute_transaction.c:496:    if (0) RUN_TEST(test_parse_sql_statements_realloc_failure);  // Disabled: malloc mocking not supported
- tests/unity/src/database/migration/transaction_test_execute_transaction.c:503:    if (0) RUN_TEST(test_execute_db2_migration_calloc_failure);  // Disabled: malloc mocking not supported
- tests/unity/src/database/migration/transaction_test_execute_transaction.c:508:    if (0) RUN_TEST(test_execute_postgresql_migration_calloc_failure);  // Disabled: malloc mocking not supported
- tests/unity/src/database/migration/transaction_test_execute_transaction.c:529:    if (0) RUN_TEST(test_database_migrations_execute_transaction_parse_failure);  // Disabled: malloc mocking not supported
- tests/unity/src/database/migration/lua_test_load_database_module.c:669:    if (0) RUN_TEST(test_database_migrations_lua_extract_queries_table_success);
- tests/unity/src/database/migration/lua_test_load_database_module.c:679:    if (0) RUN_TEST(test_database_migrations_lua_execute_run_migration_success);
- tests/unity/src/database/migration/execute_test_execute_auto.c:553:    if (0) RUN_TEST(test_database_migrations_execute_single_migration_with_mocks);

## Database/MySQL

- tests/unity/src/database/mysql/query_test_coverage_mysql.c:509:    if (0) RUN_TEST(test_mysql_execute_query_memory_allocation_failure); // Skipped - requires mock_system
- tests/unity/src/database/mysql/query_test_coverage_mysql.c:516:    if (0) RUN_TEST(test_mysql_execute_prepared_affected_rows_fallback); // Targets line 520
- tests/unity/src/database/mysql/query_test_coverage_mysql.c:518:    if (0) RUN_TEST(test_mysql_execute_prepared_stmt_execute_unavailable); // Skipped - mock limitation
- tests/unity/src/database/mysql/query_test_coverage_mysql.c:519:    if (0) RUN_TEST(test_mysql_execute_prepared_memory_allocation_failure); // Skipped - requires mock_system
- tests/unity/src/database/mysql/query_test_coverage_mysql.c:520:    if (0) RUN_TEST(test_mysql_execute_prepared_with_result_set); // Skipped - mock limitation
- tests/unity/src/database/mysql/query_test_edge_cases_mysql.c:415:    if (0) RUN_TEST(test_mysql_execute_prepared_execution_failure);
- tests/unity/src/database/mysql/query_test_edge_cases_mysql.c:418:    if (0) RUN_TEST(test_mysql_execute_prepared_null_column_data);

## Database/PostgreSQL

- tests/unity/src/database/postgresql/prepared_test_postgresql_force_failures.c:218:    if (0) RUN_TEST(test_force_cache_initialization_failure);
- tests/unity/src/database/postgresql/prepared_test_postgresql_force_failures.c:219:    if (0) RUN_TEST(test_force_lru_eviction_failure);

## Database/SQLite

- tests/unity/src/database/sqlite/query_test_sqlite.c:787:    if (0) RUN_TEST(test_sqlite_execute_query_memory_allocation_failure);
- tests/unity/src/database/sqlite/query_test_sqlite.c:809:    if (0) RUN_TEST(test_sqlite_execute_prepared_memory_allocation_failure);

## WebServer

- tests/unity/src/webserver/web_server_core_test_resolve_filesystem_path.c:292:    if (0) RUN_TEST(test_resolve_filesystem_path_absolute_path);
- tests/unity/src/webserver/web_server_core_test_resolve_filesystem_path.c:293:    if (0) RUN_TEST(test_resolve_filesystem_path_relative_path_with_webroot);
- tests/unity/src/webserver/web_server_core_test_resolve_filesystem_path.c:294:    if (0) RUN_TEST(test_resolve_filesystem_path_relative_path_no_webroot);
- tests/unity/src/webserver/web_server_core_test_resolve_filesystem_path.c:297:    if (0) RUN_TEST(test_resolve_filesystem_path_with_parent_directory);
- tests/unity/src/webserver/web_server_core_test_resolve_filesystem_path.c:298:    if (0) RUN_TEST(test_resolve_filesystem_path_with_tilde);
- tests/unity/src/webserver/web_server_core_test_shutdown.c:178:    if (0) RUN_TEST(test_shutdown_web_server_null_daemon);
- tests/unity/src/webserver/web_server_core_test_shutdown.c:179:    if (0) RUN_TEST(test_shutdown_web_server_with_running_daemon);
- tests/unity/src/webserver/web_server_core_test_shutdown.c:180:    if (0) RUN_TEST(test_shutdown_web_server_already_shutdown);
- tests/unity/src/webserver/web_server_core_test_shutdown.c:181:    if (0) RUN_TEST(test_shutdown_web_server_atomic_failure);
- tests/unity/src/webserver/web_server_core_test_run_web_server.c:292:    if (0) RUN_TEST(test_run_web_server_getifaddrs_failure);
- tests/unity/src/webserver/web_server_core_test_run_web_server.c:293:    if (0) RUN_TEST(test_run_web_server_mhd_start_daemon_failure);
- tests/unity/src/webserver/web_server_core_test_run_web_server.c:294:    if (0) RUN_TEST(test_run_web_server_mhd_get_daemon_info_failure);
- tests/unity/src/webserver/web_server_core_test_run_web_server.c:295:    if (0) RUN_TEST(test_run_web_server_successful_startup);
- tests/unity/src/webserver/web_server_core_test_init_web_server.c:192:    if (0) RUN_TEST(test_init_web_server_null_config);
- tests/unity/src/webserver/web_server_request_test_handle_request.c:87:    if (0) RUN_TEST(test_handle_request_null_connection);
- tests/unity/src/webserver/web_server_request_test_handle_request.c:88:    if (0) RUN_TEST(test_handle_request_null_url);
- tests/unity/src/webserver/web_server_request_test_handle_request.c:89:    if (0) RUN_TEST(test_handle_request_null_method);
- tests/unity/src/webserver/web_server_request_test_handle_request.c:90:    if (0) RUN_TEST(test_handle_request_unsupported_method);
- tests/unity/src/webserver/web_server_request_test_handle_request.c:91:    if (0) RUN_TEST(test_handle_request_options_method);
- tests/unity/src/webserver/web_server_request_test_handle_request.c:92:    if (0) RUN_TEST(test_handle_request_get_method_root_path);
- tests/unity/src/webserver/web_server_request_test_handle_request.c:93:    if (0) RUN_TEST(test_handle_request_get_method_api_endpoint);
- tests/unity/src/webserver/web_server_request_test_handle_request.c:94:    if (0) RUN_TEST(test_handle_request_get_method_static_file);
- tests/unity/src/webserver/web_server_request_test_handle_request.c:95:    if (0) RUN_TEST(test_handle_request_post_method_without_endpoint);
- tests/unity/src/webserver/web_server_request_test_handle_request.c:96:    if (0) RUN_TEST(test_handle_request_post_method_with_registered_endpoint);
- tests/unity/src/webserver/web_server_request_test_handle_request.c:97:    if (0) RUN_TEST(test_handle_request_connection_thread_registration);
- tests/unity/src/webserver/web_server_request_test_handle_request.c:98:    if (0) RUN_TEST(test_handle_request_swagger_request_handling);
- tests/unity/src/webserver/web_server_upload_test_handle_upload_data.c:210:    if (0) RUN_TEST(test_handle_upload_data_null_connection_info);
- tests/unity/src/webserver/web_server_core_test_add_cors_headers.c:79:    if (0) RUN_TEST(test_add_cors_headers_with_various_null_combinations);
- tests/unity/src/webserver/web_server_core_test_resolve_webroot_path.c:213:    if (0) RUN_TEST(test_resolve_webroot_path_payload_prefix);
- tests/unity/src/webserver/web_server_core_test_resolve_webroot_path.c:214:    if (0) RUN_TEST(test_resolve_webroot_path_payload_prefix_no_slash);
- tests/unity/src/webserver/web_server_core_test_resolve_webroot_path.c:216:    if (0) RUN_TEST(test_resolve_webroot_path_filesystem_relative);
- tests/unity/src/webserver/web_server_core_test_resolve_webroot_path.c:217:    if (0) RUN_TEST(test_resolve_webroot_path_empty_payload_path);
- tests/unity/src/webserver/web_server_core_test_resolve_webroot_path.c:219:    if (0) RUN_TEST(test_resolve_webroot_path_null_subdir_for_payload_prefix);
- tests/unity/src/webserver/web_server_core_test_resolve_webroot_path.c:220:    if (0) RUN_TEST(test_resolve_webroot_path_empty_string);
- tests/unity/src/webserver/web_server_request_test_system_endpoints.c:98:    if (0) RUN_TEST(test_handle_system_info_request_function_exists);
- tests/unity/src/webserver/web_server_request_test_system_endpoints.c:99:    if (0) RUN_TEST(test_handle_system_health_request_function_exists);
- tests/unity/src/webserver/web_server_request_test_system_endpoints.c:100:    if (0) RUN_TEST(test_handle_system_prometheus_request_function_exists);
- tests/unity/src/webserver/web_server_request_test_system_endpoints.c:101:    if (0) RUN_TEST(test_handle_system_test_request_function_exists);
- tests/unity/src/webserver/web_server_request_test_system_endpoints.c:102:    if (0) RUN_TEST(test_handle_system_test_request_with_get_method);
- tests/unity/src/webserver/web_server_request_test_system_endpoints.c:103:    if (0) RUN_TEST(test_handle_system_test_request_with_post_method);
- tests/unity/src/webserver/web_server_request_test_system_endpoints.c:104:    if (0) RUN_TEST(test_handle_system_test_request_null_method);
- tests/unity/src/webserver/web_server_request_test_system_endpoints.c:105:    if (0) RUN_TEST(test_handle_system_test_request_invalid_method);
- tests/unity/src/webserver/web_server_request_test_system_endpoints.c:106:    if (0) RUN_TEST(test_handle_system_test_request_empty_upload_data);
- tests/unity/src/webserver/web_server_request_test_system_endpoints.c:107:    if (0) RUN_TEST(test_handle_system_test_request_with_upload_data);
- tests/unity/src/webserver/web_server_request_test_system_endpoints.c:108:    if (0) RUN_TEST(test_system_endpoints_url_routing);
- tests/unity/src/webserver/web_server_request_test_system_endpoints.c:109:    if (0) RUN_TEST(test_system_endpoints_service_name_extraction);
- tests/unity/src/webserver/web_server_request_test_system_endpoints.c:110:    if (0) RUN_TEST(test_system_endpoints_endpoint_name_extraction);
- tests/unity/src/webserver/web_server_request_test_system_endpoints.c:111:    if (0) RUN_TEST(test_system_endpoints_prefix_validation);
- tests/unity/src/webserver/web_server_request_test_request_completed.c:78:    if (0) RUN_TEST(test_request_completed_null_parameters);
- tests/unity/src/webserver/web_server_request_test_request_completed.c:79:    if (0) RUN_TEST(test_request_completed_null_con_cls);
- tests/unity/src/webserver/web_server_request_test_request_completed.c:80:    if (0) RUN_TEST(test_request_completed_null_con_info_in_con_cls);
- tests/unity/src/webserver/web_server_request_test_request_completed.c:81:    if (0) RUN_TEST(test_request_completed_with_valid_con_info);
- tests/unity/src/webserver/web_server_request_test_request_completed.c:82:    if (0) RUN_TEST(test_request_completed_cleanup_postprocessor);
- tests/unity/src/webserver/web_server_request_test_request_completed.c:83:    if (0) RUN_TEST(test_request_completed_cleanup_file_pointer);
- tests/unity/src/webserver/web_server_request_test_request_completed.c:84:    if (0) RUN_TEST(test_request_completed_cleanup_filenames);
- tests/unity/src/webserver/web_server_request_test_request_completed.c:85:    if (0) RUN_TEST(test_request_completed_thread_cleanup);
- tests/unity/src/webserver/web_server_request_test_request_completed.c:86:    if (0) RUN_TEST(test_request_completed_multiple_calls_safe);
- tests/unity/src/webserver/web_server_request_test_request_completed.c:87:    if (0) RUN_TEST(test_request_completed_termination_codes);
- tests/unity/src/webserver/web_server_request_test_serve_file.c:168:    if (0) RUN_TEST(test_serve_file_system_dependencies);
- tests/unity/src/webserver/web_server_request_test_serve_file.c:169:    if (0) RUN_TEST(test_serve_file_content_type_logic);
- tests/unity/src/webserver/web_server_request_test_serve_file.c:170:    if (0) RUN_TEST(test_serve_file_brotli_logic);
- tests/unity/src/webserver/web_server_request_test_serve_file.c:171:    if (0) RUN_TEST(test_serve_file_cors_headers);
- tests/unity/src/webserver/web_server_request_test_serve_file.c:172:    if (0) RUN_TEST(test_serve_file_error_handling);
- tests/unity/src/webserver/web_server_request_test_brotli_file_exists.c:120:    if (0) RUN_TEST(test_brotli_file_exists_null_file_path);
- tests/unity/src/webserver/web_server_request_test_brotli_file_exists.c:121:    if (0) RUN_TEST(test_brotli_file_exists_null_br_file_path);
- tests/unity/src/webserver/web_server_request_test_brotli_file_exists.c:122:    if (0) RUN_TEST(test_brotli_file_exists_empty_file_path);
- tests/unity/src/webserver/web_server_request_test_brotli_file_exists.c:123:    if (0) RUN_TEST(test_brotli_file_exists_zero_buffer_size);
- tests/unity/src/webserver/web_server_request_test_brotli_file_exists.c:124:    if (0) RUN_TEST(test_brotli_file_exists_nonexistent_file);
- tests/unity/src/webserver/web_server_request_test_brotli_file_exists.c:125:    if (0) RUN_TEST(test_brotli_file_exists_creates_correct_br_path);
- tests/unity/src/webserver/web_server_request_test_brotli_file_exists.c:126:    if (0) RUN_TEST(test_brotli_file_exists_path_construction);
- tests/unity/src/webserver/web_server_request_test_brotli_file_exists.c:127:    if (0) RUN_TEST(test_brotli_file_exists_buffer_overflow_protection);
- tests/unity/src/webserver/web_server_request_test_brotli_file_exists.c:128:    if (0) RUN_TEST(test_brotli_file_exists_special_characters);
- tests/unity/src/webserver/web_server_upload_test_generate_uuid.c:66:    if (0) RUN_TEST(test_generate_uuid_null_buffer);
- tests/unity/src/webserver/web_server_upload_test_generate_uuid.c:67:    if (0) RUN_TEST(test_generate_uuid_basic_functionality);
- tests/unity/src/webserver/web_server_upload_test_generate_uuid.c:68:    if (0) RUN_TEST(test_generate_uuid_multiple_calls);
- tests/unity/src/webserver/web_server_upload_test_generate_uuid.c:69:    if (0) RUN_TEST(test_generate_uuid_uniqueness_over_time);
- tests/unity/src/webserver/web_server_upload_test_generate_uuid.c:70:    if (0) RUN_TEST(test_generate_uuid_no_null_termination);
- tests/unity/src/webserver/web_server_upload_test_generate_uuid.c:71:    if (0) RUN_TEST(test_generate_uuid_consistent_format);
- tests/unity/src/webserver/web_server_upload_test_generate_uuid.c:72:    if (0) RUN_TEST(test_generate_uuid_version_and_variant_bits);

## WebSocket

- tests/unity/src/websocket/websocket_server_test_coverage_gaps.c:633:    if (0) RUN_TEST(test_start_websocket_server_pthread_create_failure);
- tests/unity/src/websocket/websocket_server_message_test_handle_message_type.c:303:    if (0) RUN_TEST(test_handle_message_type_terminal_message_processing);
- tests/unity/src/websocket/websocket_server_test_run_helpers.c:285:    if (0) RUN_TEST(test_run_service_loop_success);
- tests/unity/src/websocket/websocket_server_test_run_helpers.c:286:    if (0) RUN_TEST(test_run_service_loop_service_error);
- tests/unity/src/websocket/websocket_server_test_run_helpers.c:287:    if (0) RUN_TEST(test_run_service_loop_shutdown);
- tests/unity/src/websocket/websocket_server_test_run_helpers.c:291:    if (0) RUN_TEST(test_handle_shutdown_timeout_with_connections);
- tests/unity/src/websocket/websocket_server_test_run_helpers.c:292:    if (0) RUN_TEST(test_handle_shutdown_timeout_mutex_lock_failure);
- tests/unity/src/websocket/websocket_server_message_test_comprehensive.c:485:    if (0) RUN_TEST(test_ws_handle_receive_message_too_large);
- tests/unity/src/websocket/websocket_server_auth_test.c:452:    if (0) RUN_TEST(test_authentication_edge_cases);
- tests/unity/src/websocket/websocket_server_terminal_test_find_or_create_terminal_session.c:134:    if (0) RUN_TEST(test_find_or_create_terminal_session_null_wsi);
- tests/unity/src/websocket/websocket_server_terminal_test_find_or_create_terminal_session.c:135:    if (0) RUN_TEST(test_find_or_create_terminal_session_null_context);
- tests/unity/src/websocket/websocket_server_terminal_test_find_or_create_terminal_session.c:136:    if (0) RUN_TEST(test_find_or_create_terminal_session_terminal_disabled);
- tests/unity/src/websocket/websocket_server_terminal_test_find_or_create_terminal_session.c:137:    if (0) RUN_TEST(test_find_or_create_terminal_session_reuse_existing);
- tests/unity/src/websocket/websocket_server_terminal_test_find_or_create_terminal_session.c:138:    if (0) RUN_TEST(test_find_or_create_terminal_session_create_new);
- tests/unity/src/websocket/websocket_server_terminal_test_find_or_create_terminal_session.c:139:    if (0) RUN_TEST(test_find_or_create_terminal_session_inactive_existing);
- tests/unity/src/websocket/websocket_server_test_start_server_error_paths.c:138:    if (0) RUN_TEST(test_start_websocket_server_successful_start);
- tests/unity/src/websocket/websocket_server_pty_test_pty_bridge.c:238:    if (0) RUN_TEST(test_setup_pty_select_valid_fd); // Temporarily disabled due to test environment limitations

## Terminal

- tests/unity/src/terminal/terminal_websocket_protocol_test_message_processing.c:265:    if (0) RUN_TEST(test_process_terminal_websocket_message_raw_text_input);
- tests/unity/src/terminal/terminal_websocket_protocol_test_message_processing.c:268:    if (0) RUN_TEST(test_process_terminal_websocket_message_input_command);
- tests/unity/src/terminal/terminal_websocket_protocol_test_message_processing.c:269:    if (0) RUN_TEST(test_process_terminal_websocket_message_resize_command);
- tests/unity/src/terminal/terminal_websocket_protocol_test_message_processing.c:270:    if (0) RUN_TEST(test_process_terminal_websocket_message_ping_command);
- tests/unity/src/terminal/terminal_websocket_protocol_test_message_processing.c:273:    if (0) RUN_TEST(test_process_terminal_websocket_message_invalid_json);
- tests/unity/src/terminal/terminal_websocket_protocol_test_message_processing.c:274:    if (0) RUN_TEST(test_process_terminal_websocket_message_malformed_json);
- tests/unity/src/terminal/terminal_session_test_coverage_improvement.c:630:    if (0) RUN_TEST(test_create_terminal_session_capacity_exceeded);
- tests/unity/src/terminal/terminal_session_test_coverage_improvement.c:631:    if (0) RUN_TEST(test_create_terminal_session_success);
- tests/unity/src/terminal/terminal_session_test_coverage_improvement.c:652:    if (0) RUN_TEST(test_cleanup_expired_sessions_with_expired);
- tests/unity/src/terminal/terminal_session_test_coverage_improvement.c:675:    if (0) RUN_TEST(test_list_active_sessions_with_sessions);
- tests/unity/src/terminal/terminal_session_test_coverage_improvement.c:679:    if (0) RUN_TEST(test_terminate_all_sessions_with_sessions);
- tests/unity/src/terminal/terminal_websocket_validation_test_get_websocket_connection_stats.c:69:    if (0) RUN_TEST(test_get_websocket_connection_stats_null_parameters);
- tests/unity/src/terminal/terminal_websocket_validation_test_get_websocket_connection_stats.c:70:    if (0) RUN_TEST(test_get_websocket_connection_stats_valid_parameters);
- tests/unity/src/terminal/terminal_websocket_validation_test_get_websocket_connection_stats.c:71:    if (0) RUN_TEST(test_get_websocket_connection_stats_connections_only);
- tests/unity/src/terminal/terminal_websocket_validation_test_get_websocket_connection_stats.c:72:    if (0) RUN_TEST(test_get_websocket_connection_stats_max_only);
- tests/unity/src/terminal/terminal_websocket_validation_test_get_websocket_connection_stats.c:73:    if (0) RUN_TEST(test_get_websocket_connection_stats_custom_values);
- tests/unity/src/terminal/terminal_shell_ops_test_coverage_improvement.c:445:    if (0) RUN_TEST(test_pty_spawn_shell_success);
- tests/unity/src/terminal/terminal_shell_ops_test_coverage_improvement.c:452:    if (0) RUN_TEST(test_pty_write_data_success);
- tests/unity/src/terminal/terminal_shell_ops_test_coverage_improvement.c:459:    if (0) RUN_TEST(test_pty_read_data_success);
- tests/unity/src/terminal/terminal_shell_ops_test_coverage_improvement.c:460:    if (0) RUN_TEST(test_pty_read_data_no_data_available);
- tests/unity/src/terminal/terminal_shell_ops_test_coverage_improvement.c:465:    if (0) RUN_TEST(test_pty_set_size_success);
- tests/unity/src/terminal/terminal_test_url_validation.c:55:    if (0) RUN_TEST(test_terminal_url_validator_disabled);
- tests/unity/src/terminal/terminal_test_request_handling.c:59:    if (0) RUN_TEST(test_handle_terminal_request_index_page);
- tests/unity/src/terminal/terminal_test_request_handling.c:71:    if (0) RUN_TEST(test_cleanup_terminal_support_success);
