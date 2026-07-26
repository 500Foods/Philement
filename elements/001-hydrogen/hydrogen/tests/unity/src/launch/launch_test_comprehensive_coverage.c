/*
 * Unity Test File: Launch Comprehensive Coverage Tests
 * This file contains unit tests for launch.c functions to improve coverage
 * of uncovered lines in get_current_config_path, check_all_launch_readiness,
 * startup_hydrogen (server_stopping path), finalize_launch_messages,
 * and launch_approved_subsystems.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/launch/launch.h>

// Forward declarations for functions being tested
char* get_current_config_path(void);
void finalize_launch_messages(const char*** messages, const size_t* count, size_t* capacity);
bool launch_approved_subsystems(ReadinessResults* results);
bool check_all_launch_readiness(void);
int startup_hydrogen(const char* config_path);

// Forward declarations for test functions
void test_get_current_config_path_returns_null_initially(void);
void test_finalize_launch_messages_with_existing_capacity(void);
void test_finalize_launch_messages_needs_expansion(void);
void test_finalize_launch_messages_count_zero(void);
void test_launch_approved_subsystems_null_results(void);
void test_launch_approved_subsystems_empty_results(void);
void test_launch_approved_subsystems_all_not_ready(void);
void test_launch_approved_subsystems_only_registry(void);
void test_launch_approved_subsystems_single_not_ready(void);
void test_check_all_launch_readiness(void);
void test_startup_hydrogen_server_stopping(void);

void setUp(void) {
    // Reset server_stopping to ensure clean state for each test
    server_stopping = 0;
}

void tearDown(void) {
    // Ensure server_stopping is reset after each test
    server_stopping = 0;
}

// Test: get_current_config_path returns NULL when nothing has been set
void test_get_current_config_path_returns_null_initially(void) {
    char* path = get_current_config_path();
    TEST_ASSERT_NULL(path);
}

// Test: finalize_launch_messages with existing capacity (no realloc needed)
void test_finalize_launch_messages_with_existing_capacity(void) {
    const char** messages = malloc(4 * sizeof(char*));
    TEST_ASSERT_NOT_NULL(messages);
    messages[0] = "msg1";
    messages[1] = "msg2";
    size_t count = 2;
    size_t capacity = 4;

    finalize_launch_messages(&messages, &count, &capacity);

    TEST_ASSERT_NULL(messages[2]);
    TEST_ASSERT_EQUAL(4, capacity);

    free(messages);
}

// Test: finalize_launch_messages when count >= capacity (triggers realloc)
void test_finalize_launch_messages_needs_expansion(void) {
    const char** messages = malloc(2 * sizeof(char*));
    TEST_ASSERT_NOT_NULL(messages);
    messages[0] = "msg1";
    messages[1] = "msg2";
    size_t count = 2;
    size_t capacity = 2;

    finalize_launch_messages(&messages, &count, &capacity);

    TEST_ASSERT_NULL(messages[2]);
    TEST_ASSERT_EQUAL(3, capacity);

    free(messages);
}

// Test: finalize_launch_messages with count=0 and capacity=0 (triggers realloc)
void test_finalize_launch_messages_count_zero(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;

    finalize_launch_messages(&messages, &count, &capacity);

    TEST_ASSERT_NOT_NULL(messages);
    TEST_ASSERT_NULL(messages[0]);
    TEST_ASSERT_EQUAL(1, capacity);

    free(messages);
}

// Test: launch_approved_subsystems with NULL results
void test_launch_approved_subsystems_null_results(void) {
    bool result = launch_approved_subsystems(NULL);
    TEST_ASSERT_FALSE(result);
}

// Test: launch_approved_subsystems with empty results (no subsystems checked)
void test_launch_approved_subsystems_empty_results(void) {
    ReadinessResults results = {0};

    bool result = launch_approved_subsystems(&results);
    TEST_ASSERT_TRUE(result);
}

// Test: launch_approved_subsystems with all subsystems not ready
void test_launch_approved_subsystems_all_not_ready(void) {
    ReadinessResults results = {
        .results = {
            {"Registry", false},
            {"Payload", false},
            {"Threads", false}
        },
        .total_checked = 3,
        .total_ready = 0,
        .total_not_ready = 3,
        .any_ready = false
    };

    bool result = launch_approved_subsystems(&results);
    TEST_ASSERT_TRUE(result);
}

// Test: launch_approved_subsystems with only Registry (should be skipped)
void test_launch_approved_subsystems_only_registry(void) {
    ReadinessResults results = {
        .results = {
            {"Registry", true}
        },
        .total_checked = 1,
        .total_ready = 1,
        .total_not_ready = 0,
        .any_ready = true
    };

    bool result = launch_approved_subsystems(&results);
    TEST_ASSERT_TRUE(result);
}

// Test: launch_approved_subsystems with single not-ready subsystem
void test_launch_approved_subsystems_single_not_ready(void) {
    ReadinessResults results = {
        .results = {
            {"Registry", true},
            {"Payload", false}
        },
        .total_checked = 2,
        .total_ready = 1,
        .total_not_ready = 1,
        .any_ready = true
    };

    bool result = launch_approved_subsystems(&results);
    TEST_ASSERT_TRUE(result);
}

// Test: check_all_launch_readiness - exercises the full coordination function
// This calls handle_readiness_checks, handle_launch_plan, launch_approved_subsystems,
// and handle_launch_review in sequence.
void test_check_all_launch_readiness(void) {
    bool result = check_all_launch_readiness();
    // In a clean test environment, no subsystems are ready, so the launch plan
    // will fail and the function should return false.
    TEST_ASSERT_FALSE(result);
}

// Test: startup_hydrogen with server_stopping set - should return 0 immediately
// This covers the early return path at the top of startup_hydrogen.
void test_startup_hydrogen_server_stopping(void) {
    server_stopping = 1;

    int result = startup_hydrogen("test_config.json");
    TEST_ASSERT_EQUAL(0, result);

    server_stopping = 0;
}

int main(void) {
    UNITY_BEGIN();

    // get_current_config_path tests
    RUN_TEST(test_get_current_config_path_returns_null_initially);

    // finalize_launch_messages tests
    RUN_TEST(test_finalize_launch_messages_with_existing_capacity);
    RUN_TEST(test_finalize_launch_messages_needs_expansion);
    RUN_TEST(test_finalize_launch_messages_count_zero);

    // launch_approved_subsystems tests
    RUN_TEST(test_launch_approved_subsystems_null_results);
    RUN_TEST(test_launch_approved_subsystems_empty_results);
    RUN_TEST(test_launch_approved_subsystems_all_not_ready);
    RUN_TEST(test_launch_approved_subsystems_only_registry);
    RUN_TEST(test_launch_approved_subsystems_single_not_ready);

    // check_all_launch_readiness test
    RUN_TEST(test_check_all_launch_readiness);

    // startup_hydrogen server_stopping test
    RUN_TEST(test_startup_hydrogen_server_stopping);

    return UNITY_END();
}
