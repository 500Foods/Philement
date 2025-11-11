/*
 * Unity Test File: Logging Subsystem Shutdown Tests
 * This file contains unit tests for shutdown_logging function
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/launch/launch.h>

// Mock includes
#include <unity/mocks/mock_launch.h>

// Enable mocks
// USE_MOCK_LAUNCH defined by CMake

// Forward declarations for functions being tested
void shutdown_logging(void);

// External variable from launch_logging.c
extern volatile sig_atomic_t logging_stopping;

// Forward declarations for test functions
void test_shutdown_logging_first_call(void);
void test_shutdown_logging_already_stopping(void);

void setUp(void) {
    // Reset global state before each test
    logging_stopping = 0;
}

void tearDown(void) {
    // Clean up after each test
}

// Test functions
void test_shutdown_logging_first_call(void) {
    // Arrange: logging_stopping should be 0 initially

    // Act
    shutdown_logging();

    // Assert
    TEST_ASSERT_EQUAL(1, logging_stopping);
}

void test_shutdown_logging_already_stopping(void) {
    // Arrange: Set logging_stopping to 1
    logging_stopping = 1;

    // Act
    shutdown_logging();

    // Assert: Should remain 1 (no change)
    TEST_ASSERT_EQUAL(1, logging_stopping);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_shutdown_logging_first_call);
    RUN_TEST(test_shutdown_logging_already_stopping);

    return UNITY_END();
}