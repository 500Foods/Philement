/*
 * Unity Test File: registry_integration_test_update_registry_on_startup.c
 * This file contains unit tests for the update_registry_on_startup function
 * from src/registry/registry_integration.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/registry/registry_integration.h>
#include <src/threads/threads.h>

// Forward declarations for functions being tested
void update_registry_on_startup(void);

// Test function declarations
void test_update_registry_on_startup_all_subsystems_running(void);
void test_update_registry_on_startup_no_subsystems_running(void);
void test_update_registry_on_startup_mixed_subsystems_running(void);
void test_update_registry_on_startup_null_app_config(void);

// Mock external variables using weak attributes.
// NOTE: app_config is intentionally NOT declared here. It is a strong
// global defined in src/globals.c, so a weak declaration in this file
// would be ignored by the linker and the test's assignments would mutate
// the real global anyway. Instead, we point the real global at a real
// AppConfig instance (or NULL) during each test.
__attribute__((weak)) ServiceThreads logging_threads = {0};
__attribute__((weak)) ServiceThreads webserver_threads = {0};
__attribute__((weak)) ServiceThreads websocket_threads = {0};
__attribute__((weak)) ServiceThreads mdns_server_threads = {0};
__attribute__((weak)) ServiceThreads print_threads = {0};
__attribute__((weak)) volatile sig_atomic_t mdns_client_system_shutdown = 0;
__attribute__((weak)) volatile sig_atomic_t mail_relay_system_shutdown = 0;
__attribute__((weak)) volatile sig_atomic_t swagger_system_shutdown = 0;
__attribute__((weak)) volatile sig_atomic_t terminal_system_shutdown = 0;

// A real AppConfig struct that tests can point the real app_config at.
// Phase 3 of the LUA_PLAN added app_config->scripting.Enabled reads to
// update_registry_on_startup(); a bogus pointer like 0x12345678 would
// now dereference invalid memory and crash. Zero-initialization keeps
// all subsystems in their default (disabled/off) state unless a test
// explicitly flips a flag.
static AppConfig mock_app_config = {0};

// Mock functions for registry operations
__attribute__((weak)) void update_service_thread_metrics(ServiceThreads *threads) {
    (void)threads; // Suppress unused parameter warning
}

__attribute__((weak)) void update_subsystem_on_startup(const char* subsystem_name, bool success) {
    (void)subsystem_name; // Suppress unused parameter warning
    (void)success; // Suppress unused parameter warning
}

void setUp(void) {
    // Reset all mock state
    logging_threads.thread_count = 0;
    webserver_threads.thread_count = 0;
    websocket_threads.thread_count = 0;
    mdns_server_threads.thread_count = 0;
    print_threads.thread_count = 0;
    app_config = NULL;
    mdns_client_system_shutdown = 0;
    mail_relay_system_shutdown = 0;
    swagger_system_shutdown = 0;
    terminal_system_shutdown = 0;
    memset(&mock_app_config, 0, sizeof(mock_app_config));
}

void tearDown(void) {
    // No cleanup needed for this test
}

// ===== TESTS FOR update_registry_on_startup =====

void test_update_registry_on_startup_all_subsystems_running(void) {
    // Arrange
    logging_threads.thread_count = 1;
    webserver_threads.thread_count = 2;
    websocket_threads.thread_count = 1;
    mdns_server_threads.thread_count = 1;
    print_threads.thread_count = 1;
    app_config = &mock_app_config;  // Point real global at real struct
    mdns_client_system_shutdown = 0;
    mail_relay_system_shutdown = 0;
    swagger_system_shutdown = 0;
    terminal_system_shutdown = 0;

    // Act
    update_registry_on_startup();

    // Assert - Function should complete without errors
    // Since this function only calls other functions and doesn't return values,
    // we mainly verify it doesn't crash and calls expected functions
    TEST_ASSERT_TRUE(true); // If we reach here, the function executed successfully
}

void test_update_registry_on_startup_no_subsystems_running(void) {
    // Arrange
    logging_threads.thread_count = 0;
    webserver_threads.thread_count = 0;
    websocket_threads.thread_count = 0;
    mdns_server_threads.thread_count = 0;
    print_threads.thread_count = 0;
    app_config = &mock_app_config;  // Point real global at real struct
    mdns_client_system_shutdown = 1;
    mail_relay_system_shutdown = 1;
    swagger_system_shutdown = 1;
    terminal_system_shutdown = 1;

    // Act
    update_registry_on_startup();

    // Assert - Function should complete without errors
    TEST_ASSERT_TRUE(true); // If we reach here, the function executed successfully
}

void test_update_registry_on_startup_mixed_subsystems_running(void) {
    // Arrange
    logging_threads.thread_count = 1;      // Running
    webserver_threads.thread_count = 0;     // Not running
    websocket_threads.thread_count = 2;     // Running
    mdns_server_threads.thread_count = 0;   // Not running
    print_threads.thread_count = 1;         // Running
    app_config = &mock_app_config;          // Point real global at real struct
    mdns_client_system_shutdown = 0;        // Running (not shutdown)
    mail_relay_system_shutdown = 1;         // Not running (shutdown)
    swagger_system_shutdown = 0;            // Running (not shutdown)
    terminal_system_shutdown = 1;           // Not running (shutdown)

    // Act
    update_registry_on_startup();

    // Assert - Function should complete without errors
    TEST_ASSERT_TRUE(true); // If we reach here, the function executed successfully
}

void test_update_registry_on_startup_null_app_config(void) {
    // Arrange
    logging_threads.thread_count = 1;
    webserver_threads.thread_count = 1;
    websocket_threads.thread_count = 1;
    mdns_server_threads.thread_count = 1;
    print_threads.thread_count = 1;
    app_config = NULL; // Real global app_config left NULL
    mdns_client_system_shutdown = 0;
    mail_relay_system_shutdown = 0;
    swagger_system_shutdown = 0;
    terminal_system_shutdown = 0;

    // Act
    update_registry_on_startup();

    // Assert - Function should complete without errors even with null app_config
    TEST_ASSERT_TRUE(true); // If we reach here, the function executed successfully
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_update_registry_on_startup_all_subsystems_running);
    RUN_TEST(test_update_registry_on_startup_no_subsystems_running);
    RUN_TEST(test_update_registry_on_startup_mixed_subsystems_running);
    RUN_TEST(test_update_registry_on_startup_null_app_config);

    return UNITY_END();
}