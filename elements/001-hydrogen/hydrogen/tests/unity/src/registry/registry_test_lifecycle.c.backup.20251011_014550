/*
 * Unity Test File: Registry Lifecycle and Dependency Tests
 * This file contains unit tests for registry lifecycle management and dependency checking
 * These tests focus on start_subsystem, stop_subsystem, and dependency validation
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Mock init function for testing
static int mock_init_success(void) {
    return 1; // Success
}

// Mock init function that fails
static int mock_init_failure(void) {
    return 0; // Failure
}

// Mock shutdown function for testing
static void mock_shutdown_function(void) {
    // Simple shutdown function
    return;
}

// Test function prototypes
void test_start_subsystem_basic(void);
void test_start_subsystem_already_running(void);
void test_start_subsystem_invalid_id(void);
void test_start_subsystem_init_failure(void);
void test_start_subsystem_with_dependency(void);
void test_start_subsystem_missing_dependency(void);
void test_stop_subsystem_basic(void);
void test_stop_subsystem_not_running(void);
void test_stop_subsystem_invalid_id(void);
void test_stop_subsystem_dependency_violation(void);
void test_check_subsystem_dependencies(void);
void test_check_subsystem_dependencies_no_dependencies(void);
void test_check_subsystem_dependencies_invalid_id(void);
void test_registry_growth(void);
void test_registry_cleanup_verification(void);
void test_multiple_subsystem_registration(void);

void setUp(void) {
    // Initialize the registry before each test
    init_registry();

    // Verify the registry is clean
    TEST_ASSERT_EQUAL(0, subsystem_registry.count);
    TEST_ASSERT_EQUAL(0, subsystem_registry.capacity);
    TEST_ASSERT_NULL(subsystem_registry.subsystems);
}

void tearDown(void) {
    // Clean up the registry after each test
    init_registry();

    // Verify the registry is clean
    TEST_ASSERT_EQUAL(0, subsystem_registry.count);
    TEST_ASSERT_EQUAL(0, subsystem_registry.capacity);
    TEST_ASSERT_NULL(subsystem_registry.subsystems);
}

// Test basic subsystem start
void test_start_subsystem_basic(void) {
    int id = register_subsystem("test_subsystem", NULL, NULL, NULL,
                               mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL(0, id);

    // Start the subsystem
    bool result = start_subsystem(id);
    TEST_ASSERT_TRUE(result);

    // Verify it started
    TEST_ASSERT_EQUAL(SUBSYSTEM_RUNNING, get_subsystem_state(id));
    TEST_ASSERT_TRUE(is_subsystem_running(id));
}

// Test starting already running subsystem
void test_start_subsystem_already_running(void) {
    int id = register_subsystem("test_subsystem", NULL, NULL, NULL,
                               mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL(0, id);

    // Start once
    bool result1 = start_subsystem(id);
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_EQUAL(SUBSYSTEM_RUNNING, get_subsystem_state(id));

    // Try to start again - should succeed but remain running
    bool result2 = start_subsystem(id);
    TEST_ASSERT_TRUE(result2);
    TEST_ASSERT_EQUAL(SUBSYSTEM_RUNNING, get_subsystem_state(id));
}

// Test starting subsystem with invalid ID
void test_start_subsystem_invalid_id(void) {
    bool result = start_subsystem(999);
    TEST_ASSERT_FALSE(result);
}

// Test starting subsystem when init function fails
void test_start_subsystem_init_failure(void) {
    int id = register_subsystem("test_subsystem", NULL, NULL, NULL,
                               mock_init_failure, mock_shutdown_function);
    TEST_ASSERT_EQUAL(0, id);

    // Try to start - should fail
    bool result = start_subsystem(id);
    TEST_ASSERT_FALSE(result);

    // Verify it ended up in error state
    TEST_ASSERT_EQUAL(SUBSYSTEM_ERROR, get_subsystem_state(id));
    TEST_ASSERT_FALSE(is_subsystem_running(id));
}

// Test starting subsystem with dependency
void test_start_subsystem_with_dependency(void) {
    // Register dependency first
    int dep_id = register_subsystem("dependency", NULL, NULL, NULL,
                                   mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL(0, dep_id);

    // Register dependent subsystem
    int id = register_subsystem("dependent", NULL, NULL, NULL,
                               mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL(1, id);

    // Add dependency
    bool dep_result = add_subsystem_dependency(id, "dependency");
    TEST_ASSERT_TRUE(dep_result);

    // Start dependency first
    bool start_dep = start_subsystem(dep_id);
    TEST_ASSERT_TRUE(start_dep);
    TEST_ASSERT_EQUAL(SUBSYSTEM_RUNNING, get_subsystem_state(dep_id));

    // Now start dependent subsystem - should succeed
    bool result = start_subsystem(id);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(SUBSYSTEM_RUNNING, get_subsystem_state(id));
}

// Test starting subsystem with missing dependency
void test_start_subsystem_missing_dependency(void) {
    // Register dependent subsystem but not its dependency
    int id = register_subsystem("dependent", NULL, NULL, NULL,
                               mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL(0, id);

    // Add dependency that doesn't exist
    bool dep_result = add_subsystem_dependency(id, "missing_dependency");
    TEST_ASSERT_TRUE(dep_result);

    // Try to start - should fail due to missing dependency
    bool result = start_subsystem(id);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(SUBSYSTEM_INACTIVE, get_subsystem_state(id));
}

// Test basic subsystem stop
void test_stop_subsystem_basic(void) {
    int id = register_subsystem("test_subsystem", NULL, NULL, NULL,
                               mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL(0, id);

    // Start first
    bool start_result = start_subsystem(id);
    TEST_ASSERT_TRUE(start_result);
    TEST_ASSERT_EQUAL(SUBSYSTEM_RUNNING, get_subsystem_state(id));

    // Stop it
    bool stop_result = stop_subsystem(id);
    TEST_ASSERT_TRUE(stop_result);
    TEST_ASSERT_EQUAL(SUBSYSTEM_INACTIVE, get_subsystem_state(id));
    TEST_ASSERT_FALSE(is_subsystem_running(id));
}

// Test stopping subsystem that's not running
void test_stop_subsystem_not_running(void) {
    int id = register_subsystem("test_subsystem", NULL, NULL, NULL,
                               mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL(0, id);

    // Try to stop without starting - should succeed
    bool result = stop_subsystem(id);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(SUBSYSTEM_INACTIVE, get_subsystem_state(id));
}

// Test stopping subsystem with invalid ID
void test_stop_subsystem_invalid_id(void) {
    bool result = stop_subsystem(999);
    TEST_ASSERT_FALSE(result);
}

// Test stopping subsystem when others depend on it
void test_stop_subsystem_dependency_violation(void) {
    // Register two subsystems
    int dep_id = register_subsystem("dependency", NULL, NULL, NULL,
                                   mock_init_success, mock_shutdown_function);
    int id = register_subsystem("dependent", NULL, NULL, NULL,
                               mock_init_success, mock_shutdown_function);

    TEST_ASSERT_EQUAL(0, dep_id);
    TEST_ASSERT_EQUAL(1, id);

    // Add dependency and start both
    bool dep_result = add_subsystem_dependency(id, "dependency");
    TEST_ASSERT_TRUE(dep_result);

    bool start_dep = start_subsystem(dep_id);
    TEST_ASSERT_TRUE(start_dep);

    bool start_id = start_subsystem(id);
    TEST_ASSERT_TRUE(start_id);

    // Try to stop dependency - should fail because dependent needs it
    bool stop_result = stop_subsystem(dep_id);
    TEST_ASSERT_FALSE(stop_result);
    TEST_ASSERT_EQUAL(SUBSYSTEM_RUNNING, get_subsystem_state(dep_id)); // Should still be running
}

// Test dependency checking
void test_check_subsystem_dependencies(void) {
    // Register two subsystems
    int dep_id = register_subsystem("dependency", NULL, NULL, NULL,
                                   mock_init_success, mock_shutdown_function);
    int id = register_subsystem("dependent", NULL, NULL, NULL,
                               mock_init_success, mock_shutdown_function);

    TEST_ASSERT_EQUAL(0, dep_id);
    TEST_ASSERT_EQUAL(1, id);

    // Add dependency
    bool dep_result = add_subsystem_dependency(id, "dependency");
    TEST_ASSERT_TRUE(dep_result);

    // Initially dependencies not met
    bool deps_met = check_subsystem_dependencies(id);
    TEST_ASSERT_FALSE(deps_met);

    // Start dependency
    bool start_dep = start_subsystem(dep_id);
    TEST_ASSERT_TRUE(start_dep);

    // Now dependencies should be met
    deps_met = check_subsystem_dependencies(id);
    TEST_ASSERT_TRUE(deps_met);
}

// Test dependency checking with no dependencies
void test_check_subsystem_dependencies_no_dependencies(void) {
    int id = register_subsystem("test_subsystem", NULL, NULL, NULL,
                               mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL(0, id);

    // No dependencies - should always be met
    bool deps_met = check_subsystem_dependencies(id);
    TEST_ASSERT_TRUE(deps_met);
}

// Test dependency checking with invalid ID
void test_check_subsystem_dependencies_invalid_id(void) {
    bool deps_met = check_subsystem_dependencies(999);
    TEST_ASSERT_FALSE(deps_met); // Should return false for invalid IDs
    deps_met = check_subsystem_dependencies(-1);
    TEST_ASSERT_FALSE(deps_met); // Should return false for invalid IDs
}

// Test registry capacity growth
void test_registry_growth(void) {
    // Register multiple subsystems to trigger growth
    int initial_capacity = subsystem_registry.capacity;

    for (int i = 0; i < 10; i++) {
        char name[32];
        snprintf(name, sizeof(name), "subsystem_%d", i);
        int id = register_subsystem(name, NULL, NULL, NULL, NULL, NULL);
        TEST_ASSERT_NOT_EQUAL(-1, id); // Ensure registration succeeded
        TEST_ASSERT_EQUAL(i, id); // IDs start from 0
    }

    // Capacity should have grown
    TEST_ASSERT_TRUE(subsystem_registry.capacity > initial_capacity); // cppcheck-suppress knownConditionTrueFalse
    TEST_ASSERT_EQUAL(10, subsystem_registry.count);
    TEST_ASSERT_NOT_NULL(subsystem_registry.subsystems);
}

// Test that registry cleanup works properly between tests
void test_registry_cleanup_verification(void) {
    // This test verifies that init_registry() properly cleans up all state
    // If this test passes but others fail, the issue is elsewhere

    // First, register a test subsystem to dirty the registry
    int id1 = register_subsystem("cleanup_test_subsystem", NULL, NULL, NULL, mock_init_success, mock_shutdown_function);
    TEST_ASSERT_NOT_EQUAL(-1, id1);
    TEST_ASSERT_EQUAL(1, subsystem_registry.count);

    // Now call init_registry() and verify it's clean
    init_registry();

    // Verify registry is clean
    TEST_ASSERT_EQUAL(0, subsystem_registry.count);
    TEST_ASSERT_EQUAL(0, subsystem_registry.capacity);
    TEST_ASSERT_NULL(subsystem_registry.subsystems);

    // Try to register the same subsystem again - should succeed
    int id2 = register_subsystem("cleanup_test_subsystem", NULL, NULL, NULL, mock_init_success, mock_shutdown_function);
    TEST_ASSERT_NOT_EQUAL(-1, id2);
    TEST_ASSERT_EQUAL(0, id2); // Should get ID 0 in clean registry
    TEST_ASSERT_EQUAL(1, subsystem_registry.count);
}

// Test multiple subsystem operations
void test_multiple_subsystem_registration(void) {
    // Register multiple subsystems
    int ids[5];
    for (int i = 0; i < 5; i++) {
        char name[32];
        snprintf(name, sizeof(name), "multi_subsystem_%d", i); // Use unique names for this test
        ids[i] = register_subsystem(name, NULL, NULL, NULL, mock_init_success, mock_shutdown_function);
        TEST_ASSERT_NOT_EQUAL(-1, ids[i]); // Ensure registration succeeded
        TEST_ASSERT_EQUAL(i, ids[i]);
    }

    // Verify all are registered
    TEST_ASSERT_EQUAL(5, subsystem_registry.count);

    // Test lookups by name
    for (int i = 0; i < 5; i++) {
        char name[32];
        snprintf(name, sizeof(name), "multi_subsystem_%d", i);
        int found_id = get_subsystem_id_by_name(name);
        TEST_ASSERT_EQUAL(ids[i], found_id);
    }

    // Start a few subsystems
    bool start_result1 = start_subsystem(ids[0]);
    bool start_result2 = start_subsystem(ids[2]);
    TEST_ASSERT_TRUE(start_result1);
    TEST_ASSERT_TRUE(start_result2);

    // Verify states
    TEST_ASSERT_EQUAL(SUBSYSTEM_RUNNING, get_subsystem_state(ids[0]));
    TEST_ASSERT_EQUAL(SUBSYSTEM_RUNNING, get_subsystem_state(ids[2]));
    TEST_ASSERT_EQUAL(SUBSYSTEM_INACTIVE, get_subsystem_state(ids[1])); // Not started

    // Test running checks
    TEST_ASSERT_TRUE(is_subsystem_running(ids[0]));
    TEST_ASSERT_TRUE(is_subsystem_running_by_name("multi_subsystem_2"));
    TEST_ASSERT_FALSE(is_subsystem_running(ids[1]));
    TEST_ASSERT_FALSE(is_subsystem_running_by_name("multi_subsystem_1"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_start_subsystem_basic);
    RUN_TEST(test_start_subsystem_already_running);
    RUN_TEST(test_start_subsystem_invalid_id);
    RUN_TEST(test_start_subsystem_init_failure);
    RUN_TEST(test_start_subsystem_with_dependency);
    RUN_TEST(test_start_subsystem_missing_dependency);
    RUN_TEST(test_stop_subsystem_basic);
    RUN_TEST(test_stop_subsystem_not_running);
    RUN_TEST(test_stop_subsystem_invalid_id);
    RUN_TEST(test_stop_subsystem_dependency_violation);
    RUN_TEST(test_check_subsystem_dependencies);
    RUN_TEST(test_check_subsystem_dependencies_no_dependencies);
    RUN_TEST(test_check_subsystem_dependencies_invalid_id);
    RUN_TEST(test_registry_growth);
    RUN_TEST(test_registry_cleanup_verification);
    RUN_TEST(test_multiple_subsystem_registration);

    return UNITY_END();
}
