/*
 * Unity Test File: Registry Print Subsystem Status Tests
 * This file contains unit tests for the print_subsystem_status() function
 * in src/registry/registry.c.
 *
 * print_subsystem_status() walks the subsystem registry and emits a status
 * report for every registered subsystem via log_this (remapped to
 * mock_log_this in Unity builds). Each test inspects the mock log call count
 * and message history to verify the correct branches execute for empty
 * registries, each subsystem state, dependencies, thread-tracking
 * structures, and multi-subsystem summaries.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Module under test and its dependencies
#include <src/registry/registry.h>
#include <src/threads/threads.h>

// Mock logging introspection API.
// USE_MOCK_LOGGING and -Dlog_this=mock_log_this are defined globally by the
// CMake build for every Unity test target, so log_this() in the code under
// test is redirected to mock_log_this().
#include <tests/unity/mocks/mock_logging.h>

// Test function prototypes
void test_print_subsystem_status_empty_registry(void);
void test_print_subsystem_status_single_inactive_subsystem(void);
void test_print_subsystem_status_running_subsystem(void);
void test_print_subsystem_status_ready_state(void);
void test_print_subsystem_status_starting_state(void);
void test_print_subsystem_status_error_state(void);
void test_print_subsystem_status_stopping_state(void);
void test_print_subsystem_status_with_dependencies(void);
void test_print_subsystem_status_with_threads(void);
void test_print_subsystem_status_multiple_subsystems(void);

void setUp(void) {
    // Initialize the registry before each test
    init_registry();
    // Clear captured log calls so assertions count only print_subsystem_status output
    mock_logging_reset_all();
}

void tearDown(void) {
    // Clean up the registry after each test
    init_registry();
    mock_logging_reset_all();
}

// Test print_subsystem_status with an empty registry (no subsystems).
void test_print_subsystem_status_empty_registry(void) {
    print_subsystem_status();

    // 3 header lines + 3 summary lines
    TEST_ASSERT_EQUAL(6, mock_logging_get_call_count());
    TEST_ASSERT_TRUE(mock_logging_message_contains("SUBSYSTEM STATUS REPORT"));
    TEST_ASSERT_TRUE(mock_logging_message_contains("Total subsystems: 0 - Running: 0"));
}

// Test print_subsystem_status with a single inactive subsystem.
void test_print_subsystem_status_single_inactive_subsystem(void) {
    register_subsystem("inactive_sub", NULL, NULL, NULL, NULL, NULL);
    mock_logging_reset_all();

    print_subsystem_status();

    // 3 header + 1 subsystem + 3 summary
    TEST_ASSERT_EQUAL(7, mock_logging_get_call_count());
    TEST_ASSERT_TRUE(mock_logging_message_contains("Subsystem: inactive_sub - State: Inactive"));
    TEST_ASSERT_TRUE(mock_logging_message_contains("Total subsystems: 1 - Running: 0"));
}

// Test print_subsystem_status with a running subsystem (running_count increments).
void test_print_subsystem_status_running_subsystem(void) {
    int id = register_subsystem("active_sub", NULL, NULL, NULL, NULL, NULL);
    update_subsystem_state(id, SUBSYSTEM_RUNNING);
    mock_logging_reset_all();

    print_subsystem_status();

    // 3 header + 1 subsystem + 3 summary
    TEST_ASSERT_EQUAL(7, mock_logging_get_call_count());
    TEST_ASSERT_TRUE(mock_logging_message_contains("Subsystem: active_sub - State: Running"));
    TEST_ASSERT_TRUE(mock_logging_message_contains("Total subsystems: 1 - Running: 1"));
}

// Test print_subsystem_status reflects the SUBSYSTEM_READY state string.
void test_print_subsystem_status_ready_state(void) {
    int id = register_subsystem("ready_sub", NULL, NULL, NULL, NULL, NULL);
    update_subsystem_state(id, SUBSYSTEM_READY);
    mock_logging_reset_all();

    print_subsystem_status();

    TEST_ASSERT_EQUAL(7, mock_logging_get_call_count());
    TEST_ASSERT_TRUE(mock_logging_message_contains("Subsystem: ready_sub - State: Ready"));
}

// Test print_subsystem_status reflects the SUBSYSTEM_STARTING state string.
void test_print_subsystem_status_starting_state(void) {
    int id = register_subsystem("starting_sub", NULL, NULL, NULL, NULL, NULL);
    update_subsystem_state(id, SUBSYSTEM_STARTING);
    mock_logging_reset_all();

    print_subsystem_status();

    TEST_ASSERT_EQUAL(7, mock_logging_get_call_count());
    TEST_ASSERT_TRUE(mock_logging_message_contains("Subsystem: starting_sub - State: Starting"));
}

// Test print_subsystem_status renders the ERROR state (which selects
// LOG_LEVEL_ERROR priority for that subsystem's status line).
void test_print_subsystem_status_error_state(void) {
    int id = register_subsystem("error_sub", NULL, NULL, NULL, NULL, NULL);
    update_subsystem_state(id, SUBSYSTEM_ERROR);
    mock_logging_reset_all();

    print_subsystem_status();

    TEST_ASSERT_EQUAL(7, mock_logging_get_call_count());
    TEST_ASSERT_TRUE(mock_logging_message_contains("Subsystem: error_sub - State: Error"));
    TEST_ASSERT_TRUE(mock_logging_message_contains("Total subsystems: 1 - Running: 0"));
}

// Test print_subsystem_status renders the STOPPING state (which selects
// LOG_LEVEL_ALERT priority for that subsystem's status line).
void test_print_subsystem_status_stopping_state(void) {
    int id = register_subsystem("stopping_sub", NULL, NULL, NULL, NULL, NULL);
    update_subsystem_state(id, SUBSYSTEM_STOPPING);
    mock_logging_reset_all();

    print_subsystem_status();

    TEST_ASSERT_EQUAL(7, mock_logging_get_call_count());
    TEST_ASSERT_TRUE(mock_logging_message_contains("Subsystem: stopping_sub - State: Stopping"));
    TEST_ASSERT_TRUE(mock_logging_message_contains("Total subsystems: 1 - Running: 0"));
}

// Test print_subsystem_status logs dependency names when present.
void test_print_subsystem_status_with_dependencies(void) {
    int id = register_subsystem("dep_sub", NULL, NULL, NULL, NULL, NULL);
    add_subsystem_dependency(id, "dep_one");
    add_subsystem_dependency(id, "dep_two");
    mock_logging_reset_all();

    print_subsystem_status();

    // 3 header + 1 subsystem + 1 dependencies line + 3 summary
    TEST_ASSERT_EQUAL(8, mock_logging_get_call_count());
    TEST_ASSERT_TRUE(mock_logging_message_contains("Subsystem: dep_sub - State: Inactive"));
    TEST_ASSERT_TRUE(mock_logging_message_contains("Dependencies: dep_one, dep_two"));
    TEST_ASSERT_TRUE(mock_logging_message_contains("Total subsystems: 1 - Running: 0"));
}

// Test print_subsystem_status logs thread metrics when a ServiceThreads
// pointer is registered. update_service_thread_metrics() is the real
// implementation (threads.c); with thread_count == 0 it is a safe no-op
// that zeros the memory fields, so no /proc work or kill() calls occur.
// Call count is not asserted exactly here because the mutex unlock trace
// logs emitted by update_service_thread_metrics() are unrelated to the
// behaviour under test; the content assertions below are stable.
void test_print_subsystem_status_with_threads(void) {
    ServiceThreads fake_threads = {0};
    fake_threads.thread_count = 0;
    int id = register_subsystem("threaded_sub", &fake_threads, NULL, NULL, NULL, NULL);
    update_subsystem_state(id, SUBSYSTEM_RUNNING);
    mock_logging_reset_all();

    print_subsystem_status();

    TEST_ASSERT_TRUE(mock_logging_message_contains("Subsystem: threaded_sub - State: Running"));
    TEST_ASSERT_TRUE(mock_logging_message_contains("Threads: 0 - Memory: 0 bytes"));
    TEST_ASSERT_TRUE(mock_logging_message_contains("Total subsystems: 1 - Running: 1"));
}

// Test print_subsystem_status with multiple subsystems and a mixed running
// count across two distinct entries.
void test_print_subsystem_status_multiple_subsystems(void) {
    int id1 = register_subsystem("multi_a", NULL, NULL, NULL, NULL, NULL);
    update_subsystem_state(id1, SUBSYSTEM_RUNNING);
    register_subsystem("multi_b", NULL, NULL, NULL, NULL, NULL);
    mock_logging_reset_all();

    print_subsystem_status();

    // 3 header + 2 subsystems + 3 summary
    TEST_ASSERT_EQUAL(8, mock_logging_get_call_count());
    TEST_ASSERT_TRUE(mock_logging_message_contains("Subsystem: multi_a - State: Running"));
    TEST_ASSERT_TRUE(mock_logging_message_contains("Subsystem: multi_b - State: Inactive"));
    TEST_ASSERT_TRUE(mock_logging_message_contains("Total subsystems: 2 - Running: 1"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_print_subsystem_status_empty_registry);
    RUN_TEST(test_print_subsystem_status_single_inactive_subsystem);
    RUN_TEST(test_print_subsystem_status_running_subsystem);
    RUN_TEST(test_print_subsystem_status_ready_state);
    RUN_TEST(test_print_subsystem_status_starting_state);
    RUN_TEST(test_print_subsystem_status_error_state);
    RUN_TEST(test_print_subsystem_status_stopping_state);
    RUN_TEST(test_print_subsystem_status_with_dependencies);
    RUN_TEST(test_print_subsystem_status_with_threads);
    RUN_TEST(test_print_subsystem_status_multiple_subsystems);

    return UNITY_END();
}
