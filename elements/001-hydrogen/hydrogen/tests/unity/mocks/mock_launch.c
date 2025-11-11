/*
 * Mock launch functions implementation for unit testing
 */

#include "mock_launch.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// Function prototypes to avoid missing prototype warnings
int mock_get_subsystem_id_by_name(const char* name);
bool mock_add_dependency_from_launch(int subsystem_id, const char* dependency_name);
bool mock_is_subsystem_running_by_name(const char* name);
bool mock_is_subsystem_launchable_by_name(const char* name);
int mock_register_subsystem_from_launch(const char* name, void* start_fn, void* stop_fn, void* status_fn, void* launch_fn, void* landing_fn);
SubsystemState mock_get_subsystem_state(int id);
bool mock_add_subsystem_dependency(int subsystem_id, const char* dependency_name);
void mock_update_subsystem_on_startup(const char* name, bool success);
const char* mock_subsystem_state_to_string(SubsystemState state);
bool mock_init_terminal_support(struct TerminalConfig* config);
bool mock_register_web_endpoint(const struct WebServerEndpoint* endpoint);

void mock_launch_set_get_subsystem_id_result(int result);
void mock_launch_set_add_dependency_result(bool result);
void mock_launch_set_is_subsystem_running_result(bool result);
void mock_launch_set_is_subsystem_running_by_name_result(const char* name, bool result);
void mock_launch_set_is_subsystem_launchable_result(bool result);
void mock_launch_set_register_subsystem_result(int result);
void mock_launch_set_get_subsystem_state_result(SubsystemState state);
void mock_launch_set_init_terminal_support_result(bool result);
void mock_launch_set_register_web_endpoint_result(bool result);
void mock_launch_reset_all(void);

// Mock state variables
static int mock_get_subsystem_id_result = 1; // Default to success
static bool mock_add_dependency_result = true; // Default to success (true)
static bool mock_is_subsystem_running_result = true; // Default to running (true)
static bool mock_is_subsystem_launchable_result = true; // Default to launchable (true)
static int mock_register_subsystem_result = 1; // Default to success
static SubsystemState mock_get_subsystem_state_result = SUBSYSTEM_RUNNING; // Default to running
static bool mock_init_terminal_support_result = true; // Default to success
static bool mock_register_web_endpoint_result = true; // Default to success

// Per-name state for is_subsystem_running_by_name
#define MAX_MOCK_SUBSYSTEMS 10
static struct {
    char* name;
    bool result;
} mock_subsystem_running_states[MAX_MOCK_SUBSYSTEMS];
static int mock_subsystem_running_states_count = 0;

// Mock implementations
int mock_get_subsystem_id_by_name(const char* name) {
    (void)name; // Suppress unused parameter warning
    return mock_get_subsystem_id_result;
}

bool mock_add_dependency_from_launch(int subsystem_id, const char* dependency_name) {
    (void)subsystem_id; // Suppress unused parameter warning
    (void)dependency_name; // Suppress unused parameter warning
    return mock_add_dependency_result;
}

bool mock_is_subsystem_running_by_name(const char* name) {
    // Check if we have a per-name result
    for (int i = 0; i < mock_subsystem_running_states_count; i++) {
        if (mock_subsystem_running_states[i].name && strcmp(mock_subsystem_running_states[i].name, name) == 0) {
            return mock_subsystem_running_states[i].result;
        }
    }
    // Fall back to global result
    return mock_is_subsystem_running_result;
}

bool mock_is_subsystem_launchable_by_name(const char* name) {
    (void)name; // Suppress unused parameter warning
    return mock_is_subsystem_launchable_result;
}

int mock_register_subsystem_from_launch(const char* name, void* start_fn, void* stop_fn, void* status_fn, void* launch_fn, void* landing_fn) {
    (void)name;
    (void)start_fn;
    (void)stop_fn;
    (void)status_fn;
    (void)launch_fn;
    (void)landing_fn;
    return mock_register_subsystem_result;
}

SubsystemState mock_get_subsystem_state(int id) {
    (void)id;
    return mock_get_subsystem_state_result;
}

bool mock_add_subsystem_dependency(int subsystem_id, const char* dependency_name) {
    (void)subsystem_id;
    (void)dependency_name;
    return mock_add_dependency_result;
}

void mock_update_subsystem_on_startup(const char* name, bool success) {
    (void)name;
    (void)success;
    // No-op for mock
}

const char* mock_subsystem_state_to_string(SubsystemState state) {
    switch (state) {
        case SUBSYSTEM_INACTIVE: return "INACTIVE";
        case SUBSYSTEM_READY: return "READY";
        case SUBSYSTEM_STARTING: return "STARTING";
        case SUBSYSTEM_RUNNING: return "RUNNING";
        case SUBSYSTEM_STOPPING: return "STOPPING";
        case SUBSYSTEM_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

bool mock_init_terminal_support(struct TerminalConfig* config) {
    (void)config;
    return mock_init_terminal_support_result;
}

bool mock_register_web_endpoint(const struct WebServerEndpoint* endpoint) {
    (void)endpoint;
    return mock_register_web_endpoint_result;
}

// Mock control functions
void mock_launch_set_get_subsystem_id_result(int result) {
    mock_get_subsystem_id_result = result;
}

void mock_launch_set_add_dependency_result(bool result) {
    mock_add_dependency_result = result;
}

void mock_launch_set_is_subsystem_running_result(bool result) {
    mock_is_subsystem_running_result = result;
}

void mock_launch_set_is_subsystem_running_by_name_result(const char* name, bool result) {
    // Find existing entry or add new one
    for (int i = 0; i < mock_subsystem_running_states_count; i++) {
        if (mock_subsystem_running_states[i].name && strcmp(mock_subsystem_running_states[i].name, name) == 0) {
            mock_subsystem_running_states[i].result = result;
            return;
        }
    }
    
    // Add new entry
    if (mock_subsystem_running_states_count < MAX_MOCK_SUBSYSTEMS) {
        mock_subsystem_running_states[mock_subsystem_running_states_count].name = strdup(name);
        mock_subsystem_running_states[mock_subsystem_running_states_count].result = result;
        mock_subsystem_running_states_count++;
    }
}

void mock_launch_set_is_subsystem_launchable_result(bool result) {
    mock_is_subsystem_launchable_result = result;
}

void mock_launch_set_register_subsystem_result(int result) {
    mock_register_subsystem_result = result;
}

void mock_launch_set_get_subsystem_state_result(SubsystemState state) {
    mock_get_subsystem_state_result = state;
}

void mock_launch_set_init_terminal_support_result(bool result) {
    mock_init_terminal_support_result = result;
}

void mock_launch_set_register_web_endpoint_result(bool result) {
    mock_register_web_endpoint_result = result;
}

void mock_launch_reset_all(void) {
    mock_get_subsystem_id_result = 1;
    mock_add_dependency_result = true;
    mock_is_subsystem_running_result = true;
    mock_is_subsystem_launchable_result = true;
    mock_register_subsystem_result = 1;
    mock_get_subsystem_state_result = SUBSYSTEM_RUNNING;
    mock_init_terminal_support_result = true;
    mock_register_web_endpoint_result = true;
    
    // Clean up per-name states
    for (int i = 0; i < mock_subsystem_running_states_count; i++) {
        free(mock_subsystem_running_states[i].name);
        mock_subsystem_running_states[i].name = NULL;
        mock_subsystem_running_states[i].result = false;
    }
    mock_subsystem_running_states_count = 0;
}