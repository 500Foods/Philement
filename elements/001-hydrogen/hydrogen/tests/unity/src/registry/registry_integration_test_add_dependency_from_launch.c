// registry_integration_test_add_dependency_from_launch.c
// Unity tests for add_dependency_from_launch function

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Test globals
extern SubsystemRegistry subsystem_registry;

// Mock init function for testing
static int mock_init_success(void) {
    return 1; // Success
}

// Mock shutdown function for testing
static void mock_shutdown_function(void) {
    return;
}

// Test function prototypes
void test_add_dependency_from_launch_basic(void);
void test_add_dependency_from_launch_multiple(void);
void test_add_dependency_from_launch_invalid_id(void);
void test_add_dependency_from_launch_null_name(void);
void test_add_dependency_from_launch_empty_name(void);
void test_add_dependency_from_launch_duplicate(void);
void test_add_dependency_from_launch_nonexistent_dependency(void);
void test_add_dependency_from_launch_negative_id(void);
void test_add_dependency_from_launch_max_capacity(void);

void setUp(void) {
    // Initialize registry for each test
    init_registry();
}

void tearDown(void) {
    // Clean up registry after each test
    init_registry();
}

// Test adding dependency from launch configuration
void test_add_dependency_from_launch_basic(void) {
    // First register a dependency subsystem
    int dep_id = register_subsystem_from_launch("dependency", NULL, NULL, NULL,
                                              mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, dep_id);

    // Register a dependent subsystem
    int id = register_subsystem_from_launch("dependent", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(1, id);

    // Add dependency using the function under test
    bool result = add_dependency_from_launch(id, "dependency");
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, get_subsystem_dependency_count(id));
    TEST_ASSERT_EQUAL_STRING("dependency", get_subsystem_dependency(id, 0));
}

// Test adding multiple dependencies
void test_add_dependency_from_launch_multiple(void) {
    // Register dependency subsystems
    int dep1_id = register_subsystem_from_launch("dependency1", NULL, NULL, NULL,
                                                mock_init_success, mock_shutdown_function);
    int dep2_id = register_subsystem_from_launch("dependency2", NULL, NULL, NULL,
                                                mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, dep1_id);
    TEST_ASSERT_EQUAL_INT(1, dep2_id);

    // Register dependent subsystem
    int id = register_subsystem_from_launch("dependent", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(2, id);

    // Add both dependencies
    bool result1 = add_dependency_from_launch(id, "dependency1");
    bool result2 = add_dependency_from_launch(id, "dependency2");
    
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_TRUE(result2);
    TEST_ASSERT_EQUAL_INT(2, get_subsystem_dependency_count(id));
    TEST_ASSERT_EQUAL_STRING("dependency1", get_subsystem_dependency(id, 0));
    TEST_ASSERT_EQUAL_STRING("dependency2", get_subsystem_dependency(id, 1));
}

// Test adding dependency with invalid subsystem ID
void test_add_dependency_from_launch_invalid_id(void) {
    bool result = add_dependency_from_launch(999, "some_dependency");
    
    TEST_ASSERT_FALSE(result);
}

// Test adding dependency with NULL dependency name
void test_add_dependency_from_launch_null_name(void) {
    int id = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);

    bool result = add_dependency_from_launch(id, NULL);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(0, get_subsystem_dependency_count(id));
}

// Test adding dependency with empty name
void test_add_dependency_from_launch_empty_name(void) {
    int id = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);

    bool result = add_dependency_from_launch(id, "");
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(0, get_subsystem_dependency_count(id));
}

// Test adding duplicate dependency
void test_add_dependency_from_launch_duplicate(void) {
    // Register dependency subsystem
    int dep_id = register_subsystem_from_launch("dependency", NULL, NULL, NULL,
                                              mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, dep_id);

    // Register dependent subsystem
    int id = register_subsystem_from_launch("dependent", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(1, id);

    // Add dependency twice
    bool result1 = add_dependency_from_launch(id, "dependency");
    bool result2 = add_dependency_from_launch(id, "dependency");
    
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_TRUE(result2);  // Core function returns true for duplicates but doesn't add them
    TEST_ASSERT_EQUAL_INT(1, get_subsystem_dependency_count(id));  // Should still be 1
}

// Test adding dependency that doesn't exist in registry
void test_add_dependency_from_launch_nonexistent_dependency(void) {
    int id = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);

    // Try to add dependency that doesn't exist
    bool result = add_dependency_from_launch(id, "nonexistent_dependency");
    
    TEST_ASSERT_TRUE(result);  // Should succeed - dependency validation happens later
    TEST_ASSERT_EQUAL_INT(1, get_subsystem_dependency_count(id));
    TEST_ASSERT_EQUAL_STRING("nonexistent_dependency", get_subsystem_dependency(id, 0));
}

// Test adding dependency with negative ID
void test_add_dependency_from_launch_negative_id(void) {
    bool result = add_dependency_from_launch(-1, "some_dependency");
    
    TEST_ASSERT_FALSE(result);
}

// Test adding dependencies up to maximum capacity
void test_add_dependency_from_launch_max_capacity(void) {
    int id = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);

    // Add dependencies up to MAX_DEPENDENCIES limit
    bool last_result = true;
    int dependencies_added = 0;
    
    for (int i = 0; i < 20 && last_result; i++) {  // Try to add many dependencies
        char dep_name[32];
        snprintf(dep_name, sizeof(dep_name), "dependency_%d", i);
        last_result = add_dependency_from_launch(id, dep_name);
        if (last_result) {
            dependencies_added++;
        }
    }
    
    // Should have added some dependencies but may have hit a limit
    TEST_ASSERT_TRUE(dependencies_added > 0);
    TEST_ASSERT_EQUAL_INT(dependencies_added, get_subsystem_dependency_count(id));
    
    // Try to add one more after hitting limit (if we hit it)
    if (!last_result) {
        bool overflow_result = add_dependency_from_launch(id, "overflow_dependency");
        TEST_ASSERT_FALSE(overflow_result);
        TEST_ASSERT_EQUAL_INT(dependencies_added, get_subsystem_dependency_count(id)); // Should remain same
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_add_dependency_from_launch_basic);
    RUN_TEST(test_add_dependency_from_launch_multiple);
    RUN_TEST(test_add_dependency_from_launch_invalid_id);
    RUN_TEST(test_add_dependency_from_launch_null_name);
    RUN_TEST(test_add_dependency_from_launch_empty_name);
    RUN_TEST(test_add_dependency_from_launch_duplicate);
    RUN_TEST(test_add_dependency_from_launch_nonexistent_dependency);
    RUN_TEST(test_add_dependency_from_launch_negative_id);
    RUN_TEST(test_add_dependency_from_launch_max_capacity);

    return UNITY_END();
}
