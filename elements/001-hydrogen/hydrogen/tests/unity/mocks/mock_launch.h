/*
 * Mock launch functions for unit testing
 *
 * This file provides mock implementations of launch subsystem functions
 * used in launch_logging.c to enable unit testing without external dependencies.
 */

#ifndef MOCK_LAUNCH_H
#define MOCK_LAUNCH_H

#include <stddef.h>

// Forward declarations
struct LaunchReadiness;

// Mock function declarations - these will override the real ones when USE_MOCK_LAUNCH is defined
#ifdef USE_MOCK_LAUNCH

// Override specific functions with our mocks
#define get_subsystem_id_by_name mock_get_subsystem_id_by_name
#define update_subsystem_state mock_update_subsystem_state
#define add_launch_message mock_add_launch_message
#define finalize_launch_messages mock_finalize_launch_messages
#define config_logging_get_level_name mock_config_logging_get_level_name

#endif // USE_MOCK_LAUNCH

// Always declare mock function prototypes for the .c file
int mock_get_subsystem_id_by_name(const char* name);
void mock_update_subsystem_state(int subsystem_id, int state);
void mock_add_launch_message(const char*** messages, size_t* count, size_t* capacity, char* message);
void mock_finalize_launch_messages(const char*** messages, size_t* count, size_t* capacity);
const char* mock_config_logging_get_level_name(const void* config, int level);

// Mock control functions for tests
void mock_launch_set_get_subsystem_id_result(int result);
void mock_launch_reset_all(void);

#endif // MOCK_LAUNCH_H