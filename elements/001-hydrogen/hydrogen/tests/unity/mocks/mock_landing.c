/*
 * Mock landing functions implementation for unit testing
 * This file contains mock implementations of common functions used by landing modules
 */

// System includes
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// Project includes for constants
#include <src/globals.h>
#include <src/state/state_types.h>
#include <src/threads/threads.h>
#include <src/registry/registry.h>
#include <pthread.h>
#include <signal.h>

// Forward declarations for mock functions
bool is_subsystem_running_by_name(const char* subsystem);
void log_this(const char* subsystem, const char* format, int priority, int num_args, ...);

// External registry
extern SubsystemRegistry subsystem_registry;
bool log_is_in_logging_operation(void);
void log_group_begin(void);
void log_group_end(void);
char* log_get_messages(const char* subsystem);
char* log_get_last_n(size_t count);
size_t count_format_specifiers(const char* format);
const char* get_fallback_priority_label(int priority);
int get_subsystem_id_by_name(const char* name);
void update_subsystem_state(int subsystem_id, SubsystemState state);
const char* subsystem_state_to_string(SubsystemState state);
bool add_subsystem_dependency(int subsystem_id, const char* dependency_name);
SubsystemState get_subsystem_state(int subsystem_id);
int register_subsystem(const char* name, ServiceThreads* threads, pthread_t* main_thread, volatile sig_atomic_t* shutdown_flag, int (*init_function)(void), void (*shutdown_function)(void));
void cleanup_api_endpoints(void);
void update_subsystem_on_shutdown(const char* subsystem_name);
void mock_landing_set_api_running(bool running);
void mock_landing_set_webserver_running(bool running);
void mock_landing_set_mdns_client_running(bool running);
void mock_landing_set_network_running(bool running);
void mock_landing_set_logging_running(bool running);
void mock_landing_set_database_running(bool running);
void mock_landing_set_mdns_server_running(bool running);
void mock_landing_set_notify_running(bool running);
void mock_landing_set_oidc_running(bool running);
void mock_landing_set_payload_running(bool running);
void mock_landing_set_print_running(bool running);
void mock_landing_set_registry_running(bool running);
void mock_landing_reset_all(void);

// Mock state variables
static bool mock_api_running = true;
static bool mock_webserver_running = true;
static bool mock_mdns_client_running = true;
static bool mock_network_running = true;
static bool mock_logging_running = true;
static bool mock_database_running = true;
static bool mock_mdns_server_running = true;
static bool mock_notify_running = true;
static bool mock_oidc_running = true;
static bool mock_payload_running = true;
static bool mock_print_running = true;
static bool mock_registry_running = true;

// Mock implementations

// Mock implementation of is_subsystem_running_by_name
__attribute__((weak))
bool is_subsystem_running_by_name(const char* name) {
    if (strcmp(name, SR_API) == 0) {
        return mock_api_running;
    } else if (strcmp(name, "WebServer") == 0) {
        return mock_webserver_running;
    } else if (strcmp(name, SR_MDNS_CLIENT) == 0) {
        return mock_mdns_client_running;
    } else if (strcmp(name, "Network") == 0) {
        return mock_network_running;
    } else if (strcmp(name, "Logging") == 0) {
        return mock_logging_running;
    } else if (strcmp(name, SR_DATABASE) == 0) {
        return mock_database_running;
    } else if (strcmp(name, SR_MDNS_SERVER) == 0) {
        return mock_mdns_server_running;
    } else if (strcmp(name, "Notify") == 0) {
        return mock_notify_running;
    } else if (strcmp(name, "OIDC") == 0) {
        return mock_oidc_running;
    } else if (strcmp(name, "Payload") == 0) {
        return mock_payload_running;
    } else if (strcmp(name, SR_PRINT) == 0) {
        return mock_print_running;
    } else if (strcmp(name, SR_REGISTRY) == 0) {
        return mock_registry_running;
    }
    // For unknown subsystems, check the registry directly
    pthread_mutex_lock(&subsystem_registry.mutex);
    bool running = false;
    for (int i = 0; i < subsystem_registry.count; i++) {
        if (strcmp(subsystem_registry.subsystems[i].name, name) == 0) {
            running = (subsystem_registry.subsystems[i].state == SUBSYSTEM_RUNNING);
            break;
        }
    }
    pthread_mutex_unlock(&subsystem_registry.mutex);
    return running;
}

// Mock implementation of log_this
__attribute__((weak))
void log_this(const char* subsystem, const char* format, int priority, int num_args, ...) {
    (void)subsystem; (void)format; (void)priority; (void)num_args;
    // Do nothing - suppress logging in tests
}

// Mock implementation of cleanup_api_endpoints
__attribute__((weak))
void cleanup_api_endpoints(void) {
    // Do nothing - mock cleanup
}

// Mock implementation of update_subsystem_on_shutdown
__attribute__((weak))
void update_subsystem_on_shutdown(const char* subsystem_name) {
    (void)subsystem_name; // Suppress unused parameter warning
    // Do nothing - mock registry update
}

// Additional logging function mocks
__attribute__((weak))
bool log_is_in_logging_operation(void) {
    return false;
}

__attribute__((weak))
void log_group_begin(void) {
    // Do nothing
}

__attribute__((weak))
void log_group_end(void) {
    // Do nothing
}

__attribute__((weak))
char* log_get_messages(const char* subsystem) {
    (void)subsystem;
    return NULL;
}

__attribute__((weak))
char* log_get_last_n(size_t count) {
    (void)count;
    return NULL;
}

// Additional logging function mocks
__attribute__((weak))
size_t count_format_specifiers(const char* format) {
    (void)format;
    return 0;
}

__attribute__((weak))
const char* get_fallback_priority_label(int priority) {
    (void)priority;
    return "MOCK";
}

// Registry function mocks
// Note: Removed generic get_subsystem_id_by_name mock to avoid conflicts
// with test-specific mocks that need different behavior

__attribute__((weak))
void update_subsystem_state(int subsystem_id, SubsystemState state) {
    (void)subsystem_id; (void)state;
    // Do nothing - mock implementation
}

__attribute__((weak))
const char* subsystem_state_to_string(SubsystemState state) {
    (void)state;
    return "MOCK_STATE";
}

// Additional registry function mocks
__attribute__((weak))
bool add_subsystem_dependency(int subsystem_id, const char* dependency_name) {
    (void)subsystem_id; (void)dependency_name;
    return true;
}

__attribute__((weak))
SubsystemState get_subsystem_state(int subsystem_id) {
    (void)subsystem_id;
    return SUBSYSTEM_RUNNING;
}

__attribute__((weak))
int register_subsystem(const char* name, ServiceThreads* threads, pthread_t* main_thread, volatile sig_atomic_t* shutdown_flag, int (*init_function)(void), void (*shutdown_function)(void)) {
    (void)name; (void)threads; (void)main_thread; (void)shutdown_flag; (void)init_function; (void)shutdown_function;
    return 1;
}

// Note: Removed malloc and strdup mocks as they can cause system instability
// when using weak linkage. For testing memory allocation failures, consider
// using dependency injection or other testing patterns.

// Mock control functions

void mock_landing_set_api_running(bool running) {
    mock_api_running = running;
}

void mock_landing_set_webserver_running(bool running) {
    mock_webserver_running = running;
}

void mock_landing_set_mdns_client_running(bool running) {
    mock_mdns_client_running = running;
}

void mock_landing_set_network_running(bool running) {
    mock_network_running = running;
}

void mock_landing_set_logging_running(bool running) {
    mock_logging_running = running;
}

void mock_landing_set_database_running(bool running) {
    mock_database_running = running;
}

void mock_landing_set_mdns_server_running(bool running) {
    mock_mdns_server_running = running;
}

void mock_landing_set_notify_running(bool running) {
    mock_notify_running = running;
}

void mock_landing_set_oidc_running(bool running) {
    mock_oidc_running = running;
}

void mock_landing_set_payload_running(bool running) {
    mock_payload_running = running;
}

void mock_landing_set_print_running(bool running) {
    mock_print_running = running;
}

void mock_landing_set_registry_running(bool running) {
    mock_registry_running = running;
}

void mock_landing_reset_all(void) {
    mock_api_running = true;
    mock_webserver_running = true;
    mock_mdns_client_running = true;
    mock_network_running = true;
    mock_logging_running = true;
    mock_database_running = true;
    mock_mdns_server_running = true;
    mock_notify_running = true;
    mock_oidc_running = true;
    mock_payload_running = true;
    mock_print_running = true;
    mock_registry_running = true;
}
