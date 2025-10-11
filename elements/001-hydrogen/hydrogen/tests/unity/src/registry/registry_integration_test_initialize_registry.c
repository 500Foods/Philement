/*
 * Unity Test File: Registry Integration Initialize Registry Tests
 * This file contains unit tests for initialize_registry function
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Test function prototypes
void test_initialize_registry_basic(void);
void test_initialize_registry_multiple_calls(void);

void setUp(void) {
    // Clean start for each test
}

void tearDown(void) {
    // Clean up after each test
    initialize_registry();
}

// Test basic registry initialization
void test_initialize_registry_basic(void) {
    // Initialize registry
    initialize_registry();
    
    // Verify the registry is clean
    TEST_ASSERT_EQUAL(0, subsystem_registry.count);
    TEST_ASSERT_EQUAL(0, subsystem_registry.capacity);
    TEST_ASSERT_NULL(subsystem_registry.subsystems);
}

// Test multiple initializations
void test_initialize_registry_multiple_calls(void) {
    initialize_registry();
    TEST_ASSERT_EQUAL(0, subsystem_registry.count);
    
    // Initialize again - should still be clean
    initialize_registry();
    TEST_ASSERT_EQUAL(0, subsystem_registry.count);
    TEST_ASSERT_EQUAL(0, subsystem_registry.capacity);
    TEST_ASSERT_NULL(subsystem_registry.subsystems);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_initialize_registry_basic);
    RUN_TEST(test_initialize_registry_multiple_calls);

    return UNITY_END();
}
