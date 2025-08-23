// registry_integration_test_register_subsystem_from_launch.c
// Unity tests for register_subsystem_from_launch function

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Test globals
extern SubsystemRegistry subsystem_registry;

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
    return;
}

// Test function prototypes
void test_register_subsystem_from_launch_basic(void);
void test_register_subsystem_from_launch_multiple(void);
void test_register_subsystem_from_launch_duplicate(void);
void test_register_subsystem_from_launch_null_name(void);
void test_register_subsystem_from_launch_empty_name(void);
void test_register_subsystem_from_launch_null_init(void);
void test_register_subsystem_from_launch_null_shutdown(void);
void test_register_subsystem_from_launch_init_failure(void);

void setUp(void) {
    // Initialize registry for each test
    init_registry();
}

void tearDown(void) {
    // Clean up registry after each test
    init_registry();
}

// Test registering a basic subsystem from launch
void test_register_subsystem_from_launch_basic(void) {
    int result = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                              mock_init_success, mock_shutdown_function);
    
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1, subsystem_registry.count);
    TEST_ASSERT_EQUAL_STRING("test_subsystem", subsystem_registry.subsystems[0].name);
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_INACTIVE, subsystem_registry.subsystems[0].state);
}

// Test registering multiple subsystems from launch
void test_register_subsystem_from_launch_multiple(void) {
    int result1 = register_subsystem_from_launch("subsystem_1", NULL, NULL, NULL,
                                                mock_init_success, mock_shutdown_function);
    int result2 = register_subsystem_from_launch("subsystem_2", NULL, NULL, NULL,
                                                mock_init_success, mock_shutdown_function);
    
    TEST_ASSERT_EQUAL_INT(0, result1);
    TEST_ASSERT_EQUAL_INT(1, result2);
    TEST_ASSERT_EQUAL_INT(2, subsystem_registry.count);
}

// Test registering duplicate subsystem from launch
void test_register_subsystem_from_launch_duplicate(void) {
    int result1 = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                                mock_init_success, mock_shutdown_function);
    int result2 = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                                mock_init_success, mock_shutdown_function);
    
    TEST_ASSERT_EQUAL_INT(0, result1);
    TEST_ASSERT_NOT_EQUAL(0, result2);  // Should fail on duplicate
    TEST_ASSERT_EQUAL_INT(1, subsystem_registry.count);  // Should still be 1
}

// Test registering with NULL name
void test_register_subsystem_from_launch_null_name(void) {
    int result = register_subsystem_from_launch(NULL, NULL, NULL, NULL,
                                              mock_init_success, mock_shutdown_function);
    
    TEST_ASSERT_NOT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_INT(0, subsystem_registry.count);
}

// Test registering with empty name
void test_register_subsystem_from_launch_empty_name(void) {
    int result = register_subsystem_from_launch("", NULL, NULL, NULL,
                                              mock_init_success, mock_shutdown_function);
    
    TEST_ASSERT_NOT_EQUAL(0, result);  // Empty name should be rejected
    TEST_ASSERT_EQUAL_INT(0, subsystem_registry.count);
}

// Test registering with NULL init function
void test_register_subsystem_from_launch_null_init(void) {
    int result = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                              NULL, mock_shutdown_function);
    
    TEST_ASSERT_EQUAL_INT(0, result);  // NULL init function should be allowed
    TEST_ASSERT_EQUAL_INT(1, subsystem_registry.count);
}

// Test registering with NULL shutdown function
void test_register_subsystem_from_launch_null_shutdown(void) {
    int result = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                              mock_init_success, NULL);
    
    TEST_ASSERT_EQUAL_INT(0, result);  // NULL shutdown function should be allowed
    TEST_ASSERT_EQUAL_INT(1, subsystem_registry.count);
}

// Test registering with init function that fails
void test_register_subsystem_from_launch_init_failure(void) {
    int result = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                              mock_init_failure, mock_shutdown_function);
    
    TEST_ASSERT_EQUAL_INT(0, result);  // Registration should succeed even with failing init
    TEST_ASSERT_EQUAL_INT(1, subsystem_registry.count);
    
    // Test that the init function is properly stored
    TEST_ASSERT_NOT_NULL(subsystem_registry.subsystems[0].init_function);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_register_subsystem_from_launch_basic);
    RUN_TEST(test_register_subsystem_from_launch_multiple);
    RUN_TEST(test_register_subsystem_from_launch_duplicate);
    RUN_TEST(test_register_subsystem_from_launch_null_name);
    RUN_TEST(test_register_subsystem_from_launch_empty_name);
    RUN_TEST(test_register_subsystem_from_launch_null_init);
    RUN_TEST(test_register_subsystem_from_launch_null_shutdown);
    RUN_TEST(test_register_subsystem_from_launch_init_failure);

    return UNITY_END();
}
