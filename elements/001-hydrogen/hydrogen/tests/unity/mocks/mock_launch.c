/*
 * Mock launch functions implementation for unit testing
 */

#include "mock_launch.h"
#include <string.h>
#include <stdlib.h>

// Function prototypes to avoid missing prototype warnings
int mock_get_subsystem_id_by_name(const char* name);
void mock_update_subsystem_state(int subsystem_id, int state);
void mock_add_launch_message(const char*** messages, size_t* count, size_t* capacity, char* message);
void mock_finalize_launch_messages(const char*** messages, size_t* count, size_t* capacity);
const char* mock_config_logging_get_level_name(const void* config, int level);
void mock_launch_set_get_subsystem_id_result(int result);
void mock_launch_reset_all(void);

// Mock state variables
static int mock_get_subsystem_id_result = 1; // Default to success

// Mock implementations
int mock_get_subsystem_id_by_name(const char* name) {
    (void)name; // Suppress unused parameter warning
    return mock_get_subsystem_id_result;
}

void mock_update_subsystem_state(int subsystem_id, int state) {
    (void)subsystem_id; // Suppress unused parameter warning
    (void)state; // Suppress unused parameter warning
    // Mock implementation - do nothing
}

void mock_add_launch_message(const char*** messages, size_t* count, size_t* capacity, char* message) {
    (void)messages; // Suppress unused parameter warning
    (void)count; // Suppress unused parameter warning
    (void)capacity; // Suppress unused parameter warning
    (void)message; // Suppress unused parameter warning
    // Mock implementation - do nothing
}

void mock_finalize_launch_messages(const char*** messages, size_t* count, size_t* capacity) {
    (void)messages; // Suppress unused parameter warning
    (void)count; // Suppress unused parameter warning
    (void)capacity; // Suppress unused parameter warning
    // Mock implementation - do nothing
}

const char* mock_config_logging_get_level_name(const void* config, int level) {
    (void)config; // Suppress unused parameter warning

    // Return mock level names based on level value
    switch (level) {
        case 0: return "TRACE";
        case 1: return "DEBUG";
        case 2: return "STATE";
        case 3: return "ALERT";
        case 4: return "ERROR";
        case 5: return "FATAL";
        case 6: return "QUIET";
        default: return "UNKNOWN";
    }
}

// Mock control functions
void mock_launch_set_get_subsystem_id_result(int result) {
    mock_get_subsystem_id_result = result;
}

void mock_launch_reset_all(void) {
    mock_get_subsystem_id_result = 1;
}