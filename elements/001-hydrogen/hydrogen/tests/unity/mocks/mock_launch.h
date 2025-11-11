/*
 * Mock launch functions for unit testing
 *
 * This file provides mock implementations of launch subsystem functions
 * used in launch tests to enable unit testing without external dependencies.
 */

#ifndef MOCK_LAUNCH_H
#define MOCK_LAUNCH_H

#include <stddef.h>
#include <stdbool.h>
#include <src/registry/registry.h>
#include <src/payload/payload_cache.h>

// Forward declarations
struct LaunchReadiness;
struct WebServerEndpoint;

// Mock function declarations - these will override the real ones when USE_MOCK_LAUNCH is defined
#ifdef USE_MOCK_LAUNCH

// Override specific functions with our mocks
#define get_subsystem_id_by_name mock_get_subsystem_id_by_name
#define add_dependency_from_launch mock_add_dependency_from_launch
#define is_subsystem_running_by_name mock_is_subsystem_running_by_name
#define is_subsystem_launchable_by_name mock_is_subsystem_launchable_by_name
#define register_subsystem_from_launch mock_register_subsystem_from_launch
#define get_subsystem_state mock_get_subsystem_state
#define add_subsystem_dependency mock_add_subsystem_dependency
#define update_subsystem_on_startup mock_update_subsystem_on_startup
#define subsystem_state_to_string mock_subsystem_state_to_string
#define init_terminal_support mock_init_terminal_support
#define register_web_endpoint mock_register_web_endpoint

#endif // USE_MOCK_LAUNCH

// Always declare mock function prototypes for the .c file
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

// Mock control functions for tests
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

#endif // MOCK_LAUNCH_H