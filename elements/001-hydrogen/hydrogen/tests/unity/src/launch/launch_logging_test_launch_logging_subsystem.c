/*
 * Unity Test File: Logging Subsystem Launch Tests
 * This file contains unit tests for launch_logging_subsystem function
 */

// Mock includes MUST come before source headers (USE_MOCK_LAUNCH defined by CMake)
#include <unity/mocks/mock_launch.h>

// Unity Framework header
#include <unity.h>

// Standard project header
#include <src/hydrogen.h>

// Include necessary headers for the module being tested
#include <src/launch/launch.h>
#include <src/registry/registry.h>

// Forward declarations for functions being tested
int launch_logging_subsystem(void);

// External variable from launch_logging.c
extern volatile sig_atomic_t logging_stopping;

// Forward declarations for test functions
void test_launch_logging_subsystem_successful_launch(void);
void test_launch_logging_subsystem_failed_subsystem_lookup(void);

void setUp(void) {
    logging_stopping = 0;
    init_registry();
}

void tearDown(void) {
    init_registry();
}

void test_launch_logging_subsystem_successful_launch(void) {
    int id = register_subsystem(SR_LOGGING, NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_TRUE(id >= 0);

    int result = launch_logging_subsystem();

    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_EQUAL(0, logging_stopping);
    TEST_ASSERT_EQUAL(SUBSYSTEM_RUNNING, get_subsystem_state(id));
}

void test_launch_logging_subsystem_failed_subsystem_lookup(void) {
    /* Empty registry — lookup fails */
    int result = launch_logging_subsystem();

    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(0, logging_stopping);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_launch_logging_subsystem_successful_launch);
    RUN_TEST(test_launch_logging_subsystem_failed_subsystem_lookup);

    return UNITY_END();
}