/*
 * Unity Test File: Logging Subsystem Launch Tests
 * This file contains unit tests for launch_logging_subsystem function
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

// Forward declarations for functions being tested
int launch_logging_subsystem(void);

// External variable from launch_logging.c
extern volatile sig_atomic_t logging_stopping;

// Direct declaration to avoid implicit function declaration
void mock_launch_set_get_subsystem_id_result(int result);

// Forward declarations for test functions
void test_launch_logging_subsystem_successful_launch(void);
void test_launch_logging_subsystem_failed_subsystem_lookup(void);

void setUp(void) {
    // Reset global state before each test
    logging_stopping = 0;
}

void tearDown(void) {
    // Clean up after each test
}

// Test functions
void test_launch_logging_subsystem_successful_launch(void) {
    // Arrange: Mock successful subsystem lookup
    mock_launch_set_get_subsystem_id_result(1);

    // Act
    int result = launch_logging_subsystem();

    // Assert
    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_EQUAL(0, logging_stopping);
}

void test_launch_logging_subsystem_failed_subsystem_lookup(void) {
    // Arrange: Mock failed subsystem lookup
    mock_launch_set_get_subsystem_id_result(-1);

    // Act
    int result = launch_logging_subsystem();

    // Assert
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(0, logging_stopping);
}

int main(void) {
    UNITY_BEGIN();

    if (0) RUN_TEST(test_launch_logging_subsystem_successful_launch);
    RUN_TEST(test_launch_logging_subsystem_failed_subsystem_lookup);

    return UNITY_END();
}