/*
 * Unity Test File: Registry Core Functions Tests
 * This file contains unit tests for the core registry functions in registry.c
 * These tests call the actual registry functions to provide real code coverage.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Test function prototypes
void test_registry_initialization(void);
void test_init_registry_functionality(void);
void test_init_registry_with_dependency_inspection(void);
void test_subsystem_registration_basic(void);
void test_subsystem_registration_null_name(void);
void test_subsystem_registration_duplicate_name(void);
void test_subsystem_state_update(void);
void test_subsystem_state_update_invalid_id(void);
void test_is_subsystem_running_basic(void);
void test_is_subsystem_running_invalid_id(void);
void test_is_subsystem_running_by_name(void);
void test_is_subsystem_running_by_name_not_found(void);
void test_get_subsystem_state_basic(void);
void test_get_subsystem_state_invalid_id(void);
void test_get_subsystem_id_by_name(void);
void test_get_subsystem_id_by_name_not_found(void);
void test_get_subsystem_id_by_name_null_name(void);
void test_subsystem_state_to_string(void);
void test_subsystem_state_to_string_invalid(void);
void test_registry_readiness_check(void);
void test_add_subsystem_dependency_basic(void);
void test_add_subsystem_dependency_null_name(void);
void test_add_subsystem_dependency_invalid_id(void);
void test_add_subsystem_dependency_max_dependencies(void);
void test_add_subsystem_dependency_duplicate(void);

void setUp(void) {
    // Initialize the registry before each test
    init_registry();
}

void tearDown(void) {
    // Clean up the registry after each test
    init_registry();
}

// Test registry initialization
void test_registry_initialization(void) {
    // Registry should be initialized with empty state
    TEST_ASSERT_EQUAL(0, subsystem_registry.count);
    TEST_ASSERT_EQUAL(0, subsystem_registry.capacity);
    TEST_ASSERT_NULL(subsystem_registry.subsystems);

    // Should be able to call init again safely
    init_registry();
    TEST_ASSERT_EQUAL(0, subsystem_registry.count);
    TEST_ASSERT_EQUAL(0, subsystem_registry.capacity);
    TEST_ASSERT_NULL(subsystem_registry.subsystems);
}

// Test init_registry functionality - what does it actually do?
void test_init_registry_functionality(void) {
    // First, let's add some subsystems to see if init_registry cleans them up
    int id1 = register_subsystem("test_sub1", NULL, NULL, NULL, NULL, NULL);
    int id2 = register_subsystem("test_sub2", NULL, NULL, NULL, NULL, NULL);

    TEST_ASSERT_EQUAL(0, id1);
    TEST_ASSERT_EQUAL(1, id2);
    TEST_ASSERT_EQUAL(2, subsystem_registry.count);
    TEST_ASSERT_NOT_NULL(subsystem_registry.subsystems);

    // Now call init_registry and see what happens
    init_registry();

    // Check what the registry state is after init_registry
    // This will tell us what init_registry actually does
    TEST_ASSERT_EQUAL(0, subsystem_registry.count);
    TEST_ASSERT_EQUAL(0, subsystem_registry.capacity);
    TEST_ASSERT_NULL(subsystem_registry.subsystems);

    // Let's try to register the same subsystems again
    // This will tell us if init_registry properly cleaned up the mutex
    int id3 = register_subsystem("test_sub1", NULL, NULL, NULL, NULL, NULL);
    int id4 = register_subsystem("test_sub2", NULL, NULL, NULL, NULL, NULL);

    // If init_registry worked properly, these should succeed
    TEST_ASSERT_EQUAL(0, id3);
    TEST_ASSERT_EQUAL(1, id4);
    TEST_ASSERT_EQUAL(2, subsystem_registry.count);
}

// Test basic subsystem registration
void test_subsystem_registration_basic(void) {
    int id = register_subsystem("test_subsystem", NULL, NULL, NULL, NULL, NULL);

    TEST_ASSERT_EQUAL(0, id); // First subsystem should get ID 0
    TEST_ASSERT_EQUAL(1, subsystem_registry.count);
    TEST_ASSERT_NOT_NULL(subsystem_registry.subsystems);

    // Verify the subsystem was registered correctly
    TEST_ASSERT_NOT_NULL(subsystem_registry.subsystems[0].name);
    TEST_ASSERT_EQUAL_STRING("test_subsystem", subsystem_registry.subsystems[0].name);
    TEST_ASSERT_EQUAL(SUBSYSTEM_INACTIVE, subsystem_registry.subsystems[0].state);
    TEST_ASSERT_EQUAL(0, subsystem_registry.subsystems[0].dependency_count);
}

// Test subsystem registration with NULL name
void test_subsystem_registration_null_name(void) {
    int id = register_subsystem(NULL, NULL, NULL, NULL, NULL, NULL);

    TEST_ASSERT_EQUAL(-1, id); // Should fail
    TEST_ASSERT_EQUAL(0, subsystem_registry.count); // No subsystems should be registered
}

// Test subsystem registration with duplicate name
void test_subsystem_registration_duplicate_name(void) {
    // Register first subsystem
    int id1 = register_subsystem("test_subsystem", NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(0, id1);
    TEST_ASSERT_EQUAL(1, subsystem_registry.count);

    // Try to register duplicate - should fail
    int id2 = register_subsystem("test_subsystem", NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(-1, id2);
    TEST_ASSERT_EQUAL(1, subsystem_registry.count); // Count should remain the same
}

// Test subsystem state update
void test_subsystem_state_update(void) {
    // Register a subsystem first
    int id = register_subsystem("test_subsystem", NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(0, id);

    // Update its state
    update_subsystem_state(id, SUBSYSTEM_RUNNING);

    // Verify the state was updated
    TEST_ASSERT_EQUAL(SUBSYSTEM_RUNNING, subsystem_registry.subsystems[id].state);
    TEST_ASSERT_TRUE(subsystem_registry.subsystems[id].state_changed > 0);
}

// Test subsystem state update with invalid ID
void test_subsystem_state_update_invalid_id(void) {
    // Try to update a non-existent subsystem - should not crash
    update_subsystem_state(999, SUBSYSTEM_RUNNING);

    // Registry should remain unchanged
    TEST_ASSERT_EQUAL(0, subsystem_registry.count);
}

// Test is_subsystem_running function
void test_is_subsystem_running_basic(void) {
    // Register a subsystem
    int id = register_subsystem("test_subsystem", NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(0, id);

    // Initially should not be running
    TEST_ASSERT_FALSE(is_subsystem_running(id));

    // Update to running state
    update_subsystem_state(id, SUBSYSTEM_RUNNING);
    TEST_ASSERT_TRUE(is_subsystem_running(id));

    // Update to stopped state
    update_subsystem_state(id, SUBSYSTEM_INACTIVE);
    TEST_ASSERT_FALSE(is_subsystem_running(id));
}

// Test is_subsystem_running with invalid ID
void test_is_subsystem_running_invalid_id(void) {
    TEST_ASSERT_FALSE(is_subsystem_running(999));
    TEST_ASSERT_FALSE(is_subsystem_running(-1));
}

// Test is_subsystem_running_by_name function
void test_is_subsystem_running_by_name(void) {
    // Register a subsystem
    int id = register_subsystem("test_subsystem", NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(0, id);

    // Initially should not be running
    TEST_ASSERT_FALSE(is_subsystem_running_by_name("test_subsystem"));

    // Update to running state
    update_subsystem_state(id, SUBSYSTEM_RUNNING);
    TEST_ASSERT_TRUE(is_subsystem_running_by_name("test_subsystem"));

    // Update to stopped state
    update_subsystem_state(id, SUBSYSTEM_INACTIVE);
    TEST_ASSERT_FALSE(is_subsystem_running_by_name("test_subsystem"));
}

// Test is_subsystem_running_by_name with non-existent subsystem
void test_is_subsystem_running_by_name_not_found(void) {
    TEST_ASSERT_FALSE(is_subsystem_running_by_name("nonexistent"));
}

// Test get_subsystem_state function
void test_get_subsystem_state_basic(void) {
    // Register a subsystem
    int id = register_subsystem("test_subsystem", NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(0, id);

    // Initially should be inactive
    TEST_ASSERT_EQUAL(SUBSYSTEM_INACTIVE, get_subsystem_state(id));

    // Update state and verify
    update_subsystem_state(id, SUBSYSTEM_RUNNING);
    TEST_ASSERT_EQUAL(SUBSYSTEM_RUNNING, get_subsystem_state(id));
}

// Test get_subsystem_state with invalid ID
void test_get_subsystem_state_invalid_id(void) {
    TEST_ASSERT_EQUAL(SUBSYSTEM_INACTIVE, get_subsystem_state(999));
    TEST_ASSERT_EQUAL(SUBSYSTEM_INACTIVE, get_subsystem_state(-1));
}

// Test get_subsystem_id_by_name function
void test_get_subsystem_id_by_name(void) {
    // Register a subsystem
    int id = register_subsystem("test_subsystem", NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(0, id);

    // Find it by name
    int found_id = get_subsystem_id_by_name("test_subsystem");
    TEST_ASSERT_EQUAL(id, found_id);
}

// Test get_subsystem_id_by_name with non-existent subsystem
void test_get_subsystem_id_by_name_not_found(void) {
    int found_id = get_subsystem_id_by_name("nonexistent");
    TEST_ASSERT_EQUAL(-1, found_id);
}

// Test get_subsystem_id_by_name with NULL name
void test_get_subsystem_id_by_name_null_name(void) {
    int found_id = get_subsystem_id_by_name(NULL);
    TEST_ASSERT_EQUAL(-1, found_id);
}

// Test subsystem_state_to_string function
void test_subsystem_state_to_string(void) {
    TEST_ASSERT_EQUAL_STRING("Inactive", subsystem_state_to_string(SUBSYSTEM_INACTIVE));
    TEST_ASSERT_EQUAL_STRING("Starting", subsystem_state_to_string(SUBSYSTEM_STARTING));
    TEST_ASSERT_EQUAL_STRING("Running", subsystem_state_to_string(SUBSYSTEM_RUNNING));
    TEST_ASSERT_EQUAL_STRING("Stopping", subsystem_state_to_string(SUBSYSTEM_STOPPING));
    TEST_ASSERT_EQUAL_STRING("Error", subsystem_state_to_string(SUBSYSTEM_ERROR));
}

// Test subsystem_state_to_string with invalid state
void test_subsystem_state_to_string_invalid(void) {
    TEST_ASSERT_EQUAL_STRING("Unknown", subsystem_state_to_string(999));
    TEST_ASSERT_EQUAL_STRING("Unknown", subsystem_state_to_string(-1));
}

// Test registry readiness check
void test_registry_readiness_check(void) {
    LaunchReadiness readiness = check_registry_readiness();

    // Registry should be ready after initialization
    TEST_ASSERT_TRUE(readiness.ready);
    TEST_ASSERT_NOT_NULL(readiness.messages);
    TEST_ASSERT_NOT_NULL(readiness.messages[0]);
    TEST_ASSERT_EQUAL_STRING("Registry", readiness.messages[0]);

    // Note: Cannot free const strings from LaunchReadiness
    // The cleanup should be handled by the check_registry_readiness function itself
    // or the strings are static and don't need freeing
}

// Test adding subsystem dependency
void test_add_subsystem_dependency_basic(void) {
    // Register two subsystems
    int id1 = register_subsystem("subsystem1", NULL, NULL, NULL, NULL, NULL);
    int id2 = register_subsystem("subsystem2", NULL, NULL, NULL, NULL, NULL);

    TEST_ASSERT_EQUAL(0, id1);
    TEST_ASSERT_EQUAL(1, id2);

    // Add dependency
    bool result = add_subsystem_dependency(id2, "subsystem1");
    TEST_ASSERT_TRUE(result);

    // Verify dependency was added
    TEST_ASSERT_EQUAL(1, subsystem_registry.subsystems[id2].dependency_count);
    TEST_ASSERT_EQUAL_STRING("subsystem1", subsystem_registry.subsystems[id2].dependencies[0]);
}

// Test adding dependency with NULL name
void test_add_subsystem_dependency_null_name(void) {
    int id = register_subsystem("test_subsystem", NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(0, id);

    bool result = add_subsystem_dependency(id, NULL);
    TEST_ASSERT_FALSE(result);
}

// Test adding dependency with invalid subsystem ID
void test_add_subsystem_dependency_invalid_id(void) {
    bool result = add_subsystem_dependency(999, "dependency");
    TEST_ASSERT_FALSE(result);
}

// Test adding dependency when maximum dependencies reached
void test_add_subsystem_dependency_max_dependencies(void) {
    // Use a completely unique approach - include current time and random number
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand((unsigned int)ts.tv_nsec); // Seed with nanoseconds
    long unique_seed = ts.tv_nsec + rand();

    char main_name[128];
    snprintf(main_name, sizeof(main_name), "main_%ld", unique_seed);

    // setUp() already calls init_registry(), so registry is clean
    // First register the main subsystem
    int id = register_subsystem(main_name, NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(0, id);
    TEST_ASSERT_EQUAL(1, subsystem_registry.count); // Verify main subsystem was registered

    // Register dependency subsystems first with unique names
    char dep_names[20][128];
    for (int i = 0; i < 20; i++) {
        snprintf(dep_names[i], sizeof(dep_names[i]), "dep%d_%ld", i, unique_seed + i);
        int dep_id = register_subsystem(dep_names[i], NULL, NULL, NULL, NULL, NULL);
        TEST_ASSERT_NOT_EQUAL(-1, dep_id); // Ensure registration succeeded
        TEST_ASSERT_EQUAL(i + 1, dep_id); // Verify expected ID
    }

    // Verify we have 21 total subsystems (main + 20 deps)
    TEST_ASSERT_EQUAL(21, subsystem_registry.count);

    // Now add dependencies (should succeed for all 20)
    for (int i = 0; i < 20; i++) {
        bool result = add_subsystem_dependency(id, dep_names[i]);
        TEST_ASSERT_TRUE(result);
    }

    // Verify we've reached the maximum using the new function
    int dep_count = get_subsystem_dependency_count(id);
    TEST_ASSERT_EQUAL(20, dep_count);

    // Try to add one more - should fail
    char too_many_name[128];
    snprintf(too_many_name, sizeof(too_many_name), "too_many_%ld", unique_seed + 1000);
    bool result = add_subsystem_dependency(id, too_many_name);
    TEST_ASSERT_FALSE(result);

    // Verify count didn't change
    dep_count = get_subsystem_dependency_count(id);
    TEST_ASSERT_EQUAL(20, dep_count);
}

// Test init_registry functionality with dependency inspection
void test_init_registry_with_dependency_inspection(void) {
    // Use unique names for this test
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    long unique_seed = ts.tv_nsec;

    char main_name[128];
    snprintf(main_name, sizeof(main_name), "test_main_%ld", unique_seed);

    // setUp() already calls init_registry(), so registry is clean
    // Register a main subsystem
    int id = register_subsystem(main_name, NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(0, id);

    // Register some dependency subsystems
    char dep1_name[128], dep2_name[128];
    snprintf(dep1_name, sizeof(dep1_name), "test_dep1_%ld", unique_seed);
    snprintf(dep2_name, sizeof(dep2_name), "test_dep2_%ld", unique_seed);

    int dep1_id = register_subsystem(dep1_name, NULL, NULL, NULL, NULL, NULL);
    int dep2_id = register_subsystem(dep2_name, NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(1, dep1_id);
    TEST_ASSERT_EQUAL(2, dep2_id);

    // Add dependencies to the main subsystem
    bool result1 = add_subsystem_dependency(id, dep1_name);
    bool result2 = add_subsystem_dependency(id, dep2_name);
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_TRUE(result2);

    // Verify dependencies were added using the new functions
    int dep_count = get_subsystem_dependency_count(id);
    TEST_ASSERT_EQUAL(2, dep_count);

    const char* dep1 = get_subsystem_dependency(id, 0);
    const char* dep2 = get_subsystem_dependency(id, 1);
    TEST_ASSERT_NOT_NULL(dep1);
    TEST_ASSERT_NOT_NULL(dep2);
    TEST_ASSERT_EQUAL_STRING(dep1_name, dep1);
    TEST_ASSERT_EQUAL_STRING(dep2_name, dep2);

    // Verify registry state before init_registry
    TEST_ASSERT_EQUAL(3, subsystem_registry.count); // main + 2 deps
    TEST_ASSERT_NOT_NULL(subsystem_registry.subsystems);

    // Now call init_registry and see what happens
    init_registry();

    // Check what the registry state is after init_registry using new functions
    int after_dep_count = get_subsystem_dependency_count(id);
    TEST_ASSERT_EQUAL(-1, after_dep_count); // Should be -1 because id is no longer valid

    const char* after_dep1 = get_subsystem_dependency(id, 0);
    TEST_ASSERT_NULL(after_dep1); // Should be NULL because id is no longer valid

    // Check overall registry state
    TEST_ASSERT_EQUAL(0, subsystem_registry.count);
    TEST_ASSERT_EQUAL(0, subsystem_registry.capacity);
    TEST_ASSERT_NULL(subsystem_registry.subsystems);

    // Verify we can register new subsystems after init_registry
    int new_id = register_subsystem("new_subsystem", NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(0, new_id); // Should get ID 0 in clean registry
}

// Test adding duplicate dependency
void test_add_subsystem_dependency_duplicate(void) {
    int id = register_subsystem("test_subsystem", NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(0, id);

    // Add dependency
    bool result1 = add_subsystem_dependency(id, "dependency1");
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_EQUAL(1, subsystem_registry.subsystems[id].dependency_count);

    // Add same dependency again - should succeed but not duplicate
    bool result2 = add_subsystem_dependency(id, "dependency1");
    TEST_ASSERT_TRUE(result2);
    TEST_ASSERT_EQUAL(1, subsystem_registry.subsystems[id].dependency_count); // Count should remain the same
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_registry_initialization);
    RUN_TEST(test_init_registry_functionality);
    RUN_TEST(test_init_registry_with_dependency_inspection);
    RUN_TEST(test_subsystem_registration_basic);
    RUN_TEST(test_subsystem_registration_null_name);
    RUN_TEST(test_subsystem_registration_duplicate_name);
    RUN_TEST(test_subsystem_state_update);
    RUN_TEST(test_subsystem_state_update_invalid_id);
    RUN_TEST(test_is_subsystem_running_basic);
    RUN_TEST(test_is_subsystem_running_invalid_id);
    RUN_TEST(test_is_subsystem_running_by_name);
    RUN_TEST(test_is_subsystem_running_by_name_not_found);
    RUN_TEST(test_get_subsystem_state_basic);
    RUN_TEST(test_get_subsystem_state_invalid_id);
    RUN_TEST(test_get_subsystem_id_by_name);
    RUN_TEST(test_get_subsystem_id_by_name_not_found);
    RUN_TEST(test_get_subsystem_id_by_name_null_name);
    RUN_TEST(test_subsystem_state_to_string);
    
    RUN_TEST(test_subsystem_state_to_string_invalid);
    RUN_TEST(test_registry_readiness_check);
    RUN_TEST(test_add_subsystem_dependency_basic);
    RUN_TEST(test_add_subsystem_dependency_null_name);
    RUN_TEST(test_add_subsystem_dependency_invalid_id);
    RUN_TEST(test_add_subsystem_dependency_max_dependencies);
    RUN_TEST(test_add_subsystem_dependency_duplicate);

    return UNITY_END();
}
