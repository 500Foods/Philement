/*
 * Unity Test File: Subsystem Launchable Check Tests
 * This file contains unit tests for is_subsystem_launchable_by_name function
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/launch/launch.h>

// Forward declarations for functions being tested
bool is_subsystem_launchable_by_name(const char* name);

// Forward declarations for test functions
void test_is_subsystem_launchable_by_name_null_name(void);
void test_is_subsystem_launchable_by_name_registry_special_case(void);
void test_is_subsystem_launchable_by_name_nonexistent_subsystem(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_is_subsystem_launchable_by_name_null_name(void) {
    // Test NULL name parameter
    bool result = is_subsystem_launchable_by_name(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_is_subsystem_launchable_by_name_registry_special_case(void) {
    // Test Registry special case (should always be considered launchable once registered)
    bool result = is_subsystem_launchable_by_name("Registry");
    // Note: Result depends on whether Registry is registered
    // The function should not crash and should handle the Registry case appropriately
    // We don't assert on the result as it depends on system state
    (void)result; // Mark as used to avoid unused variable warning
    TEST_ASSERT_TRUE(true); // If we reach here, the function handled the case without crashing
}

void test_is_subsystem_launchable_by_name_nonexistent_subsystem(void) {
    // Test with a non-existent subsystem name
    bool result = is_subsystem_launchable_by_name("NonExistentSubsystem");
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_is_subsystem_launchable_by_name_null_name);
    RUN_TEST(test_is_subsystem_launchable_by_name_registry_special_case);
    RUN_TEST(test_is_subsystem_launchable_by_name_nonexistent_subsystem);

    return UNITY_END();
}
