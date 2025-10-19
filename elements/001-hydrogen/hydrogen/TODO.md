# Hydrogen Project TODO List

## CRITICAL INSTRUCTIONS - PLEASE FOLLOW FIRST

Hydrogen project. Use elements/001-hydrogen/hydrogen as the base for the project and everything we're doing here.

Review RECIPE.md
Review tests/README.md
Review tests/UNITY.md
Review docs/plans/DATABASE_PLAN.md

## Purpose

This TODO.md file serves as a central location to track plenty of incomplete items in the Hydrogen project. Use this file to prioritize and manage development tasks as they are identified and addressed.

### Status Folder (4 TODOs)

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/status/status_process.c:297 | Implement message counting for logging metrics |
| 2 | Pending | src/status/status_process.c:302 | Implement request tracking for webserver metrics |
| 3 | Pending | src/status/status_process.c:322 | Implement discovery counting for mDNS metrics |
| 4 | Pending | src/status/status_process.c:327 | Implement job counting for print metrics |

### Print Folder (1 TODO)

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/print/print_queue_manager.c:66 | Implement actual print job processing |

### API Folder (2 TODO)

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/api/api_utils.c:364 | Implement actual JWT creation with the provided secret |
| 2 | Pending | src/api/api_utils.c:364 | Remove static flag from create_jwt_token function |

### OIDC Folder (5 TODOs)

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/oidc/oidc_service.c:202 | Implement actual authorization flow |
| 2 | Pending | src/oidc/oidc_service.c:247 | Implement actual token request handling |
| 3 | Pending | src/oidc/oidc_service.c:279 | Implement actual userinfo request handling |
| 4 | Pending | src/oidc/oidc_service.c:316 | Implement actual introspection request handling |
| 5 | Pending | src/oidc/oidc_service.c:353 | Implement actual revocation request handling |

### Launch Folder (3 TODOs)

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/launch/launch_notify.c:201 | Add actual notification service initialization |
| 2 | Pending | src/launch/launch_mail_relay.c:162 | Add proper mail relay initialization |
| 3 | Pending | src/launch/launch_mdns_client.c:188 | Implement actual mDNS client initialization |

### Database Folder (18 TODOs)

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/database/postgresql/transaction.c:106 | Generate unique transaction IDs for PostgreSQL |
| 3 | Pending | src/database/postgresql/transaction.c:106 | Generate unique transaction IDs for PostgreSQL |
| 4 | Pending | src/database/database_engine.c:304 | Implement dynamic resizing for database engine |
| 5 | Pending | src/database/database.c:236 | Implement clean shutdown of queue manager and connections |
| 6 | Pending | src/database/database.c:369 | Implement database removal logic |
| 7 | Pending | src/database/database.c:399 | Implement comprehensive health check |
| 8 | Pending | src/database/database.c:417 | Implement query submission to queue system |
| 9 | Pending | src/database/database.c:428 | Implement query status checking |
| 10 | Pending | src/database/database.c:438 | Implement result retrieval |
| 11 | Pending | src/database/database.c:448 | Implement query cancellation |
| 12 | Pending | src/database/database.c:462 | Implement configuration reload |
| 13 | Pending | src/database/database.c:473 | Implement connection testing |
| 14 | Pending | src/database/database.c:506 | Implement API query processing |
| 15 | Pending | src/database/database.c:522 | Implement more comprehensive validation |
| 16 | Pending | src/database/database.c:532 | Implement parameter escaping |
| 17 | Pending | src/database/database.c:542 | Implement query age tracking |
| 18 | Pending | src/database/database.c:552 | Implement result cleanup |
| 19 | Pending | src/database/dbqueue/process.c:113 | Implement actual database query execution (Phase 2) |
| 21 | Pending | src/database/dbqueue/process.c:113 | Implement actual database query execution (Phase 2) |
| 22 | Pending | src/database/dbqueue/process.c:160 | Implement intelligent queue management |

### Database Migrations (20 TODOs)

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/database/dbqueue/lead.c | Analyze current bootstrap query failure behavior when database is empty |
| 2 | Pending | src/database/dbqueue/lead.c | Modify bootstrap query to return multiple fields including empty database detection |
| 3 | Pending | src/database/dbqueue/lead.c | Update logging to differentiate between connection failures and empty database scenarios |
| 4 | Pending | src/database/dbqueue/lead.c | Implement graceful fallback when bootstrap fails but AutoMigration is enabled |
| 5 | Pending | src/database/dbqueue/lead.c | Design bootstrap query to return multiple fields (available_migrations, installed_migrations, etc.) |
| 6 | Pending | src/database/migration/ | Implement migration tracking table structure for available vs installed migrations |
| 7 | Pending | src/database/dbqueue/lead.c | Update bootstrap query to query both migration states from database |
| 8 | Pending | src/database/dbqueue/lead.c | Modify lead.c to interpret bootstrap results for migration decision making |
| 9 | Pending | src/database/dbqueue/lead.c | Separate migration loading from execution in AutoMigration process |
| 10 | Pending | src/database/dbqueue/lead.c | Implement two-phase AutoMigration: load phase completes before execute phase |
| 11 | Pending | src/database/dbqueue/lead.c | Add bootstrap re-execution after loading to verify loaded > installed migrations |
| 12 | Pending | src/database/dbqueue/lead.c | Update lead.c migration workflow to handle both phases as transactions with error stopping |
| 13 | Pending | src/database/dbqueue/lead.c | Design TestMigration mechanism for rollback functionality with built-in safety |
| 14 | Pending | src/database/dbqueue/lead.c | Implement reverse migration execution that stops on SQL errors like other phases |
| 15 | Pending | src/database/migration/ | Add validation that migrations only delete records they added and won't drop non-empty tables |
| 16 | Pending | src/database/dbqueue/lead.c | Update lead.c to handle TestMigration as separate operation mode |

### Queue Folder (2 TODO)

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/queue/queue.c:182 | Initialize queue with attributes |
| 2 | Pending | src/queue/queue.c:26 | Remove static flag from hash function |

### Logging Folder (2 TODOs)

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/logging/log_queue_manager.c:155 | Implement database logging when needed |
| 2 | Pending | src/logging/log_queue_manager.c:168 | Call SMTP send function for notification subsystem |

### Mail Relay Folder

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/launch/launch_mail_relay.c:162 | Add proper mail relay initialization (already included in Launch folder) |

### MDNS Client Folder

| # | Status | File | Description |
|---|--------|------|-------------|
| 1 | Pending | src/launch/launch_mdns_client.c:161 | Add proper mDNS client initialization (already included in Launch folder) |

### Remove Static Function Flags

#### API Folder (1 TODO)

| # | Status | File | Description |
|---|---|------|-------------|

#### Config Folder (29 TODOs)

| # | Status | File | Description |
|---|---|------|-------------|
| 1 | Pending | src/config/config_oidc.c:15 | Remove static flag from construct_endpoint_path function |
| 2 | Pending | src/config/config_logging.c:15 | Remove static flag from init_subsystems function |
| 3 | Pending | src/config/config_logging.c:30 | Remove static flag from process_subsystems function |
| 4 | Pending | src/config/config_logging.c:120 | Remove static flag from dump_destination function |
| 5 | Pending | src/config/config_logging.c:269 | Remove static flag from get_subsystem_level_internal function |
| 6 | Pending | src/config/config_mdns_server.c:14 | Remove static flag from cleanup_service function |
| 7 | Pending | src/config/config_mdns_server.c:30 | Remove static flag from cleanup_services function |
| 8 | Pending | src/config/config_mdns_server.c:40 | Remove static flag from process_txt_records function |
| 9 | Pending | src/config/config_print.c:116 | Remove static flag from dump_priorities function |
| 10 | Pending | src/config/config_print.c:125 | Remove static flag from dump_timeouts function |
| 11 | Pending | src/config/config_print.c:132 | Remove static flag from dump_buffers function |
| 12 | Pending | src/config/config_print.c:139 | Remove static flag from dump_motion function |
| 13 | Pending | src/config/config_network.c:34 | Remove static flag from is_port_in_range function |
| 14 | Pending | src/config/config_network.c:39 | Remove static flag from compare_interface_names function |
| 15 | Pending | src/config/config_network.c:53 | Remove static flag from sort_available_interfaces function |
| 16 | Pending | src/config/config.c:52 | Remove static flag from clean_app_config function |
| 17 | Pending | src/config/config.c:259 | Remove static flag from utf8_char_count function |
| 18 | Pending | src/config/config.c:271 | Remove static flag from utf8_truncate function |
| 19 | Pending | src/config/config.c:289 | Remove static flag from format_section_header function |
| 20 | Pending | src/config/config_utils.c:24 | Remove static flag from get_top_level_section function |
| 21 | Pending | src/config/config_utils.c:28 | Remove static flag from get_indent function |
| 22 | Pending | src/config/config_utils.c:108 | Remove static flag from is_env_var_ref function |
| 23 | Pending | src/config/config_utils.c:113 | Remove static flag from get_env_var_name function |
| 24 | Pending | src/config/config_utils.c:130 | Remove static flag from get_indent function |
| 25 | Pending | src/config/config_utils.c:156 | Remove static flag from format_sensitive function |
| 26 | Pending | src/config/config_utils.c:288 | Remove static flag from log_value function |
| 27 | Pending | src/config/config_utils.c:562 | Remove static flag from format_int_array function |
| 28 | Pending | src/config/config_utils.c:597 | Remove static flag from format_string_array function |
| 29 | Pending | src/config/config_mail_relay.c:11 | Remove static flag from cleanup_server function |

#### Database Folder (17 TODOs)

| # | Status | File | Description |
|---|---|------|-------------|
| 1 | Pending | src/database/dbqueue/lead.c:32 | Remove static flag from calc_elapsed_time function |
| 2 | Pending | src/database/dbqueue/lead.c:84 | Remove static flag from database_queue_lead_determine_migration_action function |
| 3 | Pending | src/database/dbqueue/lead.c:116 | Remove static flag from database_queue_lead_log_migration_status function |
| 4 | Pending | src/database/dbqueue/lead.c:139 | Remove static flag from database_queue_lead_validate_migrations function |
| 5 | Pending | src/database/dbqueue/lead.c:164 | Remove static flag from database_queue_lead_execute_migration_load function |
| 6 | Pending | src/database/dbqueue/lead.c:191 | Remove static flag from database_queue_lead_execute_migration_apply function |
| 7 | Pending | src/database/dbqueue/lead.c:210 | Remove static flag from database_queue_lead_rerun_bootstrap function |
| 8 | Pending | src/database/dbqueue/lead.c:237 | Remove static flag from database_queue_lead_is_auto_migration_enabled function |
| 9 | Pending | src/database/dbqueue/lead.c:254 | Remove static flag from database_queue_lead_acquire_migration_connection function |
| 10 | Pending | src/database/dbqueue/lead.c:273 | Remove static flag from database_queue_lead_release_migration_connection function |
| 11 | Pending | src/database/dbqueue/lead.c:280 | Remove static flag from database_queue_lead_execute_migration_cycle function |
| 12 | Pending | src/database/dbqueue/submit.c:17 | Remove static flag from serialize_query_to_json function |
| 13 | Pending | src/database/dbqueue/submit.c:31 | Remove static flag from deserialize_query_from_json function |
| 14 | Pending | src/database/dbqueue/heartbeat.c:106 | Remove static flag from database_queue_handle_connection_success function |
| 15 | Pending | src/database/dbqueue/heartbeat.c:174 | Remove static flag from database_queue_perform_connection_attempt function |
| 16 | Pending | src/database/database_engine.c:261 | Remove static flag from find_prepared_statement function |
| 17 | Pending | src/database/database_engine.c:277 | Remove static flag from store_prepared_statement function |

#### Landing Folder (2 TODOs)

| # | Status | File | Description |
|---|---|------|-------------|
| 1 | Pending | src/landing/landing_oidc.c:52 | Remove static flag from free_oidc_resources function |
| 2 | Pending | src/landing/landing_notify.c:51 | Remove static flag from free_notify_resources function |

#### Launch Folder (13 TODOs)

| # | Status | File | Description |
|---|---|------|-------------|
| 1 | Pending | src/launch/launch.c:114 | Remove static flag from log_early_info function |
| 2 | Pending | src/launch/launch.c:117 | Remove static flag from get_uppercase_name function |
| 3 | Pending | src/launch/launch.c:136 | Remove static flag from launch_approved_subsystems function |
| 4 | Pending | src/launch/launch.c:201 | Remove static flag from log_early_info function |
| 5 | Pending | src/launch/launch_network.c:31 | Remove static flag from register_network function |
| 6 | Pending | src/launch/launch_webserver.c:31 | Remove static flag from register_webserver function |
| 7 | Pending | src/launch/launch_mdns_server.c:34 | Remove static flag from register_mdns_server_for_launch function |
| 8 | Pending | src/launch/launch_mdns_client.c:29 | Remove static flag from register_mdns_client_for_launch function |
| 9 | Pending | src/launch/launch_readiness.c:56 | Remove static flag from parse_major_version function |
| 10 | Pending | src/launch/launch_readiness.c:63 | Remove static flag from is_rfc1918_or_local_ip function |
| 11 | Pending | src/launch/launch_readiness.c:76 | Remove static flag from is_plausible_db2_version function |
| 12 | Pending | src/launch/launch_readiness.c:84 | Remove static flag from has_db2_keywords_nearby function |
| 13 | Pending | src/launch/launch_readiness.c:94 | Remove static flag from score_db2_version function |

#### Logging Folder (11 TODOs)

| # | Status | File | Description |
|---|---|------|-------------|
| 1 | Pending | src/logging/logging.c:44 | Remove static flag from free_logging_flag function |
| 2 | Pending | src/logging/logging.c:49 | Remove static flag from init_tls_keys function |
| 3 | Pending | src/logging/logging.c:59 | Remove static flag from get_logging_operation_flag function |
| 4 | Pending | src/logging/logging.c:79 | Remove static flag from set_logging_operation_flag function |
| 5 | Pending | src/logging/logging.c:87 | Remove static flag from get_mutex_operation_flag function |
| 6 | Pending | src/logging/logging.c:102 | Remove static flag from set_mutex_operation_flag function |
| 7 | Pending | src/logging/logging.c:109 | Remove static flag from get_log_group_flag function |
| 8 | Pending | src/logging/logging.c:124 | Remove static flag from set_log_group_flag function |
| 9 | Pending | src/logging/logging.c:287 | Remove static flag from console_log function |
| 10 | Pending | src/logging/log_queue_manager.c:155 | Remove static flag from database logging implementation |
| 11 | Pending | src/logging/log_queue_manager.c:168 | Remove static flag from SMTP send function call |

#### Mutex Folder (8 TODOs)

| # | Status | File | Description |
|---|---|------|-------------|
| 1 | Pending | src/mutex/mutex.c:32 | Remove static flag from free_mutex_id function |
| 2 | Pending | src/mutex/mutex.c:38 | Remove static flag from init_mutex_tls_keys function |
| 3 | Pending | src/mutex/mutex.c:47 | Remove static flag from get_current_mutex_op_id function |
| 4 | Pending | src/mutex/mutex.c:52 | Remove static flag from set_current_mutex_op_id function |
| 5 | Pending | src/mutex/mutex.c:71 | Remove static flag from get_current_mutex_op_ptr function |
| 6 | Pending | src/mutex/mutex.c:76 | Remove static flag from set_current_mutex_op_ptr function |
| 7 | Pending | src/mutex/mutex.c:83 | Remove static flag from detect_potential_deadlock function |
| 8 | Pending | src/mutex/mutex.c:309 | Remove static flag from detect_potential_deadlock function |

#### Network Folder (2 TODOs)

| # | Status | File | Description |
|---|---|------|-------------|
| 1 | Pending | src/network/network_linux.c:57 | Remove static flag from test_interface function |
| 2 | Pending | src/network/network_linux.c:405 | Remove static flag from find_iface_for_ip function |

#### Payload Folder (1 TODO)

| # | Status | File | Description |
|---|---|------|-------------|
| 1 | Pending | src/payload/payload_cache.c:267 | Remove static flag from compare_files function |

#### Registry Folder (1 TODO)

| # | Status | File | Description |
|---|---|------|-------------|
| 1 | Pending | src/registry/registry.c:41 | Remove static flag from grow_registry function |

#### Status Folder (1 TODO)

| # | Status | File | Description |
|---|---|------|-------------|
| 1 | Pending | src/status/status.c:26 | Remove static flag from collect_all_metrics function |

#### Terminal Folder (3 TODOs)

| # | Status | File | Description |
|---|---|------|-------------|
| 1 | Pending | src/terminal/terminal_websocket_bridge.c:162 | Remove static flag from terminal_io_bridge_thread function |
| 2 | Pending | src/terminal/terminal_shell_spawn.c:70 | Remove static flag from setup_child_process function |
| 3 | Pending | src/terminal/terminal_session.c:40 | Remove static flag from session_cleanup_thread function |

#### Threads Folder (2 TODOs)

| # | Status | File | Description |
|---|---|------|-------------|
| 1 | Pending | src/threads/threads.c:32 | Remove static flag from get_thread_stack_size function |
| 2 | Pending | src/threads/threads.c:107 | Remove static flag from remove_thread_internal function |

#### Utils Folder (9 TODOs)

| # | Status | File | Description |
|---|---|------|-------------|
| 1 | Pending | src/utils/utils.c:12 | Remove static flag from init_all_service_threads function |
| 2 | Pending | src/utils/utils.c:57 | Remove static flag from init_all_service_threads function |
| 3 | Pending | src/utils/utils_hash.c:11 | Remove static flag from djb2_hash_64 function |
| 4 | Pending | src/utils/utils_time.c:34 | Remove static flag from format_iso_time function |
| 5 | Pending | src/utils/utils_time.c:44 | Remove static flag from calc_elapsed_time function |
| 6 | Pending | src/utils/utils_dependency.c:82 | Remove static flag from get_cache_file_path function |
| 7 | Pending | src/utils/utils_dependency.c:95 | Remove static flag from ensure_cache_dir function |
| 8 | Pending | src/utils/utils_dependency.c:125 | Remove static flag from load_cached_version function |
| 9 | Pending | src/utils/utils_dependency.c:156 | Remove static flag from save_cache function |

#### Webserver Folder (1 TODO)

| # | Status | File | Description |
|---|---|------|-------------|
| 1 | Pending | src/webserver/web_server_core.c:113 | Remove static flag from is_port_available function |

#### Websocket Folder (2 TODOs)

| # | Status | File | Description |
|---|---|------|-------------|
| 1 | Pending | src/websocket/websocket_server_pty.c:38 | Remove static flag from pty_bridge_iteration function |
| 2 | Pending | src/websocket/websocket_server.c:303 | Remove static flag from websocket_server_run function |
| 2 | Pending | src/launch/launch_mdns_client.c:188 | Implement actual mDNS client initialization (already included in Launch folder) |
