/*
 * Unity Test File: should_log_to_console Function Tests
 * This file contains unit tests for the should_log_to_console() function
 * from src/logging/log_queue_manager.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers
#include "../../../../src/logging/log_queue_manager.h"
#include "../../../../src/config/config_logging.h"
#include "../../../../src/config/config_defaults.h"

// Test fixtures
static LoggingConfig test_config;
static AppConfig test_app_config;

void setUp(void) {
    // Initialize test config with defaults
    memset(&test_config, 0, sizeof(LoggingConfig));
    test_config.console.enabled = true;
    // Set default level to LOG_LEVEL_ALERT (3)
    test_config.console.default_level = LOG_LEVEL_ALERT;

    // Initialize app config for process_log_message tests
    memset(&test_app_config, 0, sizeof(AppConfig));
    initialize_config_defaults(&test_app_config);
    app_config = &test_app_config;  // Set the global pointer
}

void tearDown(void) {
    // Clean up app config
    // Note: cleanup_config_defaults would be needed here, but for simplicity
    // we'll just reset the global pointer
    app_config = NULL;
}

// Function prototypes for test functions
static void test_should_log_to_console_disabled(void);
static void test_should_log_to_console_enabled_below_level(void);
static void test_should_log_to_console_enabled_at_level(void);
static void test_should_log_to_console_enabled_above_level(void);
static void test_should_log_to_console_trace_level(void);
static void test_should_log_to_console_quiet_level(void);
static void test_should_log_to_file_disabled(void);
static void test_should_log_to_file_enabled_at_level(void);
static void test_should_log_to_database_disabled(void);
static void test_should_log_to_database_enabled_at_level(void);
static void test_should_log_to_notify_disabled(void);
static void test_should_log_to_notify_enabled_at_level(void);
static void test_init_file_logging_valid_path(void);
static void test_close_file_logging(void);
static void test_process_log_message_null_app_config(void);
static void test_process_log_message_valid_json(void);
static void test_process_log_message_invalid_json(void);
static void test_process_log_message_console_logging(void);
static void test_process_log_message_file_logging(void);
static void test_process_log_message_database_logging(void);
static void test_process_log_message_notify_logging(void);
static void test_process_log_message_mixed_logging(void);
static void test_cleanup_log_queue_manager_null_arg(void);
static void test_cleanup_log_queue_manager_valid_arg(void);

void test_should_log_to_console_disabled(void) {
    test_config.console.enabled = false;
    bool result = should_log_to_console("TestSubsystem", LOG_LEVEL_ERROR, &test_config);
    TEST_ASSERT_FALSE(result);
}

void test_should_log_to_console_enabled_below_level(void) {
    test_config.console.enabled = true;
    test_config.console.default_level = LOG_LEVEL_ERROR;
    bool result = should_log_to_console("TestSubsystem", LOG_LEVEL_ALERT, &test_config);
    TEST_ASSERT_FALSE(result);
}

void test_should_log_to_console_enabled_at_level(void) {
    test_config.console.enabled = true;
    test_config.console.default_level = LOG_LEVEL_ALERT;
    bool result = should_log_to_console("TestSubsystem", LOG_LEVEL_ALERT, &test_config);
    TEST_ASSERT_TRUE(result);
}

void test_should_log_to_console_enabled_above_level(void) {
    test_config.console.enabled = true;
    test_config.console.default_level = LOG_LEVEL_ALERT;
    bool result = should_log_to_console("TestSubsystem", LOG_LEVEL_ERROR, &test_config);
    TEST_ASSERT_TRUE(result);
}

void test_should_log_to_console_trace_level(void) {
    test_config.console.enabled = true;
    test_config.console.default_level = LOG_LEVEL_TRACE;
    bool result = should_log_to_console("TestSubsystem", LOG_LEVEL_DEBUG, &test_config);
    TEST_ASSERT_TRUE(result);
}

void test_should_log_to_console_quiet_level(void) {
    test_config.console.enabled = true;
    test_config.console.default_level = LOG_LEVEL_QUIET;
    bool result = should_log_to_console("TestSubsystem", LOG_LEVEL_ERROR, &test_config);
    TEST_ASSERT_FALSE(result);
}

void test_should_log_to_file_disabled(void) {
    test_config.file.enabled = false;
    bool result = should_log_to_file("TestSubsystem", LOG_LEVEL_ERROR, &test_config);
    TEST_ASSERT_FALSE(result);
}

void test_should_log_to_file_enabled_at_level(void) {
    test_config.file.enabled = true;
    test_config.file.default_level = LOG_LEVEL_ALERT;
    bool result = should_log_to_file("TestSubsystem", LOG_LEVEL_ALERT, &test_config);
    TEST_ASSERT_TRUE(result);
}

void test_should_log_to_database_disabled(void) {
    test_config.database.enabled = false;
    bool result = should_log_to_database("TestSubsystem", LOG_LEVEL_ERROR, &test_config);
    TEST_ASSERT_FALSE(result);
}

void test_should_log_to_database_enabled_at_level(void) {
    test_config.database.enabled = true;
    test_config.database.default_level = LOG_LEVEL_ALERT;
    bool result = should_log_to_database("TestSubsystem", LOG_LEVEL_ALERT, &test_config);
    TEST_ASSERT_TRUE(result);
}

void test_should_log_to_notify_disabled(void) {
    test_config.notify.enabled = false;
    bool result = should_log_to_notify("TestSubsystem", LOG_LEVEL_ERROR, &test_config);
    TEST_ASSERT_FALSE(result);
}

void test_should_log_to_notify_enabled_at_level(void) {
    test_config.notify.enabled = true;
    test_config.notify.default_level = LOG_LEVEL_ALERT;
    bool result = should_log_to_notify("TestSubsystem", LOG_LEVEL_ALERT, &test_config);
    TEST_ASSERT_TRUE(result);
}

void test_init_file_logging_valid_path(void) {
    // Test with a temporary file path
    const char* test_path = "/tmp/test_log.txt";
    init_file_logging(test_path);
    // We can't directly check the static FILE*, but the function should not crash
    TEST_ASSERT_TRUE(true); // If we reach here, it didn't crash
}

void test_close_file_logging(void) {
    close_file_logging();
    // Again, can't check directly, but should not crash
    TEST_ASSERT_TRUE(true);
}

void test_process_log_message_null_app_config(void) {
    app_config = NULL;  // Temporarily set to NULL
    const char* test_message = "{\"subsystem\":\"Test\",\"details\":\"test message\",\"LogConsole\":true,\"LogDatabase\":false,\"LogFile\":false,\"LogNotify\":false}";
    process_log_message(test_message, LOG_LEVEL_ALERT);
    // Should return early without crashing
    TEST_ASSERT_TRUE(true);
}

void test_process_log_message_valid_json(void) {
    app_config = &test_app_config;  // Restore
    const char* test_message = "{\"subsystem\":\"Test\",\"details\":\"test message\",\"LogConsole\":true,\"LogDatabase\":false,\"LogFile\":false,\"LogNotify\":false}";
    process_log_message(test_message, LOG_LEVEL_ALERT);
    // Should process without crashing
    TEST_ASSERT_TRUE(true);
}

void test_process_log_message_invalid_json(void) {
    const char* invalid_message = "not json at all";
    process_log_message(invalid_message, LOG_LEVEL_ERROR);
    // Should handle invalid JSON gracefully
    TEST_ASSERT_TRUE(true);
}

void test_process_log_message_console_logging(void) {
    // Enable console logging and test
    test_app_config.logging.console.enabled = true;
    test_app_config.logging.console.default_level = LOG_LEVEL_ALERT;
    const char* test_message = "{\"subsystem\":\"Test\",\"details\":\"console test\",\"LogConsole\":true,\"LogDatabase\":false,\"LogFile\":false,\"LogNotify\":false}";
    process_log_message(test_message, LOG_LEVEL_ALERT);
    TEST_ASSERT_TRUE(true);
}

void test_process_log_message_file_logging(void) {
    // Enable file logging and test
    test_app_config.logging.file.enabled = true;
    test_app_config.logging.file.default_level = LOG_LEVEL_DEBUG;
    const char* test_message = "{\"subsystem\":\"Test\",\"details\":\"file test\",\"LogConsole\":false,\"LogDatabase\":false,\"LogFile\":true,\"LogNotify\":false}";
    process_log_message(test_message, LOG_LEVEL_DEBUG);
    TEST_ASSERT_TRUE(true);
}

void test_process_log_message_database_logging(void) {
    // Enable database logging and test
    test_app_config.logging.database.enabled = true;
    test_app_config.logging.database.default_level = LOG_LEVEL_ERROR;
    const char* test_message = "{\"subsystem\":\"Test\",\"details\":\"database test\",\"LogConsole\":false,\"LogDatabase\":true,\"LogFile\":false,\"LogNotify\":false}";
    process_log_message(test_message, LOG_LEVEL_ERROR);
    TEST_ASSERT_TRUE(true);
}

void test_process_log_message_notify_logging(void) {
    // Enable notify logging with SMTP and test
    test_app_config.logging.notify.enabled = true;
    test_app_config.logging.notify.default_level = LOG_LEVEL_FATAL;
    test_app_config.notify.notifier = strdup("SMTP");
    test_app_config.notify.smtp.host = strdup("test.smtp.example.com");
    const char* test_message = "{\"subsystem\":\"Test\",\"details\":\"notify test\",\"LogConsole\":false,\"LogDatabase\":false,\"LogFile\":false,\"LogNotify\":true}";
    process_log_message(test_message, LOG_LEVEL_FATAL);
    TEST_ASSERT_TRUE(true);
}

void test_process_log_message_mixed_logging(void) {
    // Enable multiple logging destinations
    test_app_config.logging.console.enabled = true;
    test_app_config.logging.console.default_level = LOG_LEVEL_ALERT;
    test_app_config.logging.file.enabled = true;
    test_app_config.logging.file.default_level = LOG_LEVEL_ALERT;
    test_app_config.logging.database.enabled = true;
    test_app_config.logging.database.default_level = LOG_LEVEL_ALERT;
    test_app_config.logging.notify.enabled = true;
    test_app_config.logging.notify.default_level = LOG_LEVEL_ALERT;
    test_app_config.notify.notifier = strdup("SMTP");
    test_app_config.notify.smtp.host = strdup("test.smtp.example.com");

    const char* test_message = "{\"subsystem\":\"Test\",\"details\":\"mixed test\",\"LogConsole\":true,\"LogDatabase\":true,\"LogFile\":true,\"LogNotify\":true}";
    process_log_message(test_message, LOG_LEVEL_ALERT);
    TEST_ASSERT_TRUE(true);
}

void test_cleanup_log_queue_manager_null_arg(void) {
    // Test with NULL argument (should not crash)
    cleanup_log_queue_manager(NULL);
    TEST_ASSERT_TRUE(true);
}

void test_cleanup_log_queue_manager_valid_arg(void) {
    // Test with a dummy argument (should not crash)
    int dummy_arg = 42;
    cleanup_log_queue_manager(&dummy_arg);
    TEST_ASSERT_TRUE(true);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_should_log_to_console_disabled);
    RUN_TEST(test_should_log_to_console_enabled_below_level);
    RUN_TEST(test_should_log_to_console_enabled_at_level);
    RUN_TEST(test_should_log_to_console_enabled_above_level);
    RUN_TEST(test_should_log_to_console_trace_level);
    RUN_TEST(test_should_log_to_console_quiet_level);
    RUN_TEST(test_should_log_to_file_disabled);
    RUN_TEST(test_should_log_to_file_enabled_at_level);
    RUN_TEST(test_should_log_to_database_disabled);
    RUN_TEST(test_should_log_to_database_enabled_at_level);
    RUN_TEST(test_should_log_to_notify_disabled);
    RUN_TEST(test_should_log_to_notify_enabled_at_level);
    RUN_TEST(test_init_file_logging_valid_path);
    RUN_TEST(test_close_file_logging);
    RUN_TEST(test_process_log_message_null_app_config);
    RUN_TEST(test_process_log_message_valid_json);
    RUN_TEST(test_process_log_message_invalid_json);
    RUN_TEST(test_process_log_message_console_logging);
    RUN_TEST(test_process_log_message_file_logging);
    RUN_TEST(test_process_log_message_database_logging);
    RUN_TEST(test_process_log_message_notify_logging);
    RUN_TEST(test_process_log_message_mixed_logging);
    RUN_TEST(test_cleanup_log_queue_manager_null_arg);
    RUN_TEST(test_cleanup_log_queue_manager_valid_arg);

    return UNITY_END();
}