/*
 * Mock landing functions header for unit testing
 * This file contains declarations for mock implementations of common functions used by landing modules
 */

#ifndef MOCK_LANDING_H
#define MOCK_LANDING_H

#include <stdbool.h>

// Mock control functions
void mock_landing_set_api_running(bool running);
void mock_landing_set_webserver_running(bool running);
void mock_landing_set_logging_running(bool running);
void mock_landing_set_mdns_client_running(bool running);
void mock_landing_set_network_running(bool running);
void mock_landing_set_database_running(bool running);
void mock_landing_set_mdns_server_running(bool running);
void mock_landing_set_notify_running(bool running);
void mock_landing_set_oidc_running(bool running);
void mock_landing_set_payload_running(bool running);
void mock_landing_set_print_running(bool running);
void mock_landing_set_registry_running(bool running);
void mock_landing_set_other_subsystems_inactive(bool inactive);
void mock_landing_set_log_thread_valid(bool valid);
void mock_landing_set_app_config_valid(bool valid);
void mock_landing_reset_all(void);

#endif /* MOCK_LANDING_H */