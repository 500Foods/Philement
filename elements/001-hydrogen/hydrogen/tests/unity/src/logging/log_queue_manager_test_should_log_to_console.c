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

// Test fixtures
static LoggingConfig test_config;

void setUp(void) {
    // Initialize test config with defaults
    memset(&test_config, 0, sizeof(LoggingConfig));
    test_config.console.enabled = true;
    // Set default level to LOG_LEVEL_ALERT (3)
    test_config.console.default_level = LOG_LEVEL_ALERT;
}

void tearDown(void) {
    // Clean up if needed
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

    return UNITY_END();
}