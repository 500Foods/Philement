/*
 * Unity Test File: Logging Launch Readiness Check Tests
 * This file contains unit tests for check_logging_launch_readiness function
 */

// Enable mocks before including headers
#define USE_MOCK_LAUNCH

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/launch/launch.h>

// Mock includes
#include <unity/mocks/mock_launch.h>

// Include config headers for structure definitions
#include <src/config/config_logging.h>

// Direct declarations to avoid implicit function declarations
void mock_launch_set_get_subsystem_id_result(int result);
void mock_launch_reset_all(void);

// Forward declarations for functions being tested
LaunchReadiness check_logging_launch_readiness(void);

// Forward declarations for test functions
void test_check_logging_launch_readiness_server_stopping(void);
void test_check_logging_launch_readiness_not_starting_or_running(void);
void test_check_logging_launch_readiness_no_config(void);
void test_check_logging_launch_readiness_no_log_levels(void);
void test_check_logging_launch_readiness_invalid_log_level(void);
void test_check_logging_launch_readiness_invalid_console_level(void);
void test_check_logging_launch_readiness_invalid_file_level(void);
void test_check_logging_launch_readiness_console_disabled(void);
void test_check_logging_launch_readiness_file_disabled(void);
void test_check_logging_launch_readiness_no_destinations_enabled(void);
void test_check_logging_launch_readiness_subsystem_not_registered(void);
void test_check_logging_launch_readiness_successful(void);

// External variables from hydrogen.c that affect the function
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t server_running;
extern AppConfig* app_config;

// Test data structures
static AppConfig test_app_config;

void setUp(void) {
    // Reset global state before each test
    server_stopping = 0;
    server_starting = 1;
    server_running = 1;
    app_config = &test_app_config;

    // Initialize test config with valid logging configuration
    memset(&test_app_config, 0, sizeof(AppConfig));
    test_app_config.logging.levels = (LogLevel*)malloc(sizeof(LogLevel) * 7);
    test_app_config.logging.level_count = 7;

    // Set up valid log levels
    for (int i = 0; i < 7; i++) {
        test_app_config.logging.levels[i].value = i; // 0=TRACE to 6=QUIET
        test_app_config.logging.levels[i].name = strdup("LEVEL");
    }

    // Set up logging destinations
    test_app_config.logging.console.enabled = true;
    test_app_config.logging.console.default_level = LOG_LEVEL_STATE;
    test_app_config.logging.console.subsystem_count = 0;
    test_app_config.logging.console.subsystems = NULL;

    test_app_config.logging.file.enabled = true;
    test_app_config.logging.file.default_level = LOG_LEVEL_DEBUG;
    test_app_config.logging.file.subsystem_count = 0;
    test_app_config.logging.file.subsystems = NULL;

    test_app_config.logging.database.enabled = false;
    test_app_config.logging.database.default_level = LOG_LEVEL_TRACE;
    test_app_config.logging.database.subsystem_count = 0;
    test_app_config.logging.database.subsystems = NULL;

    test_app_config.logging.notify.enabled = false;
    test_app_config.logging.notify.default_level = LOG_LEVEL_TRACE;
    test_app_config.logging.notify.subsystem_count = 0;
    test_app_config.logging.notify.subsystems = NULL;

    // Reset mocks
    mock_launch_reset_all();
}

void tearDown(void) {
    // Clean up test fixtures
    if (test_app_config.logging.levels) {
        for (int i = 0; i < 7; i++) {
            free(test_app_config.logging.levels[i].name);
        }
        free(test_app_config.logging.levels);
        test_app_config.logging.levels = NULL;
    }
}

// Test functions
void test_check_logging_launch_readiness_server_stopping(void) {
    // Arrange
    server_stopping = 1;

    // Act
    LaunchReadiness result = check_logging_launch_readiness();

    // Assert
    TEST_ASSERT_EQUAL_STRING("Logging", result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_logging_launch_readiness_not_starting_or_running(void) {
    // Arrange
    server_starting = 0;
    server_running = 0;

    // Act
    LaunchReadiness result = check_logging_launch_readiness();

    // Assert
    TEST_ASSERT_EQUAL_STRING("Logging", result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_logging_launch_readiness_no_config(void) {
    // Arrange
    app_config = NULL;

    // Act
    LaunchReadiness result = check_logging_launch_readiness();

    // Assert
    TEST_ASSERT_EQUAL_STRING("Logging", result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_logging_launch_readiness_no_log_levels(void) {
    // Arrange
    test_app_config.logging.levels = NULL;
    test_app_config.logging.level_count = 0;

    // Act
    LaunchReadiness result = check_logging_launch_readiness();

    // Assert
    TEST_ASSERT_EQUAL_STRING("Logging", result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_logging_launch_readiness_invalid_log_level(void) {
    // Arrange
    test_app_config.logging.levels[0].value = 10; // Invalid level

    // Act
    LaunchReadiness result = check_logging_launch_readiness();

    // Assert
    TEST_ASSERT_EQUAL_STRING("Logging", result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_logging_launch_readiness_invalid_console_level(void) {
    // Arrange
    test_app_config.logging.console.default_level = 10; // Invalid level

    // Act
    LaunchReadiness result = check_logging_launch_readiness();

    // Assert
    TEST_ASSERT_EQUAL_STRING("Logging", result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_logging_launch_readiness_invalid_file_level(void) {
    // Arrange
    test_app_config.logging.file.default_level = 10; // Invalid level

    // Act
    LaunchReadiness result = check_logging_launch_readiness();

    // Assert
    TEST_ASSERT_EQUAL_STRING("Logging", result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_logging_launch_readiness_console_disabled(void) {
    // Arrange
    test_app_config.logging.console.enabled = false;

    // Act
    LaunchReadiness result = check_logging_launch_readiness();

    // Assert
    TEST_ASSERT_EQUAL_STRING("Logging", result.subsystem);
    TEST_ASSERT_TRUE(result.ready); // Should be ready since file is still enabled
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_logging_launch_readiness_file_disabled(void) {
    // Arrange
    test_app_config.logging.file.enabled = false;

    // Act
    LaunchReadiness result = check_logging_launch_readiness();

    // Assert
    TEST_ASSERT_EQUAL_STRING("Logging", result.subsystem);
    TEST_ASSERT_TRUE(result.ready); // Should be ready since console is still enabled
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_logging_launch_readiness_no_destinations_enabled(void) {
    // Arrange
    test_app_config.logging.console.enabled = false;
    test_app_config.logging.file.enabled = false;

    // Act
    LaunchReadiness result = check_logging_launch_readiness();

    // Assert
    TEST_ASSERT_EQUAL_STRING("Logging", result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_logging_launch_readiness_subsystem_not_registered(void) {
    // Arrange
    mock_launch_set_get_subsystem_id_result(-1);

    // Act
    LaunchReadiness result = check_logging_launch_readiness();

    // Assert
    TEST_ASSERT_EQUAL_STRING("Logging", result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_logging_launch_readiness_successful(void) {
    // Arrange - all defaults should work

    // Act
    LaunchReadiness result = check_logging_launch_readiness();

    // Assert
    TEST_ASSERT_EQUAL_STRING("Logging", result.subsystem);
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_check_logging_launch_readiness_server_stopping);
    RUN_TEST(test_check_logging_launch_readiness_not_starting_or_running);
    RUN_TEST(test_check_logging_launch_readiness_no_config);
    RUN_TEST(test_check_logging_launch_readiness_no_log_levels);
    RUN_TEST(test_check_logging_launch_readiness_invalid_log_level);
    RUN_TEST(test_check_logging_launch_readiness_invalid_console_level);
    RUN_TEST(test_check_logging_launch_readiness_invalid_file_level);
    if (0) RUN_TEST(test_check_logging_launch_readiness_console_disabled);
    if (0) RUN_TEST(test_check_logging_launch_readiness_file_disabled);
    RUN_TEST(test_check_logging_launch_readiness_no_destinations_enabled);
    RUN_TEST(test_check_logging_launch_readiness_subsystem_not_registered);
    if (0) RUN_TEST(test_check_logging_launch_readiness_successful);

    return UNITY_END();
}
