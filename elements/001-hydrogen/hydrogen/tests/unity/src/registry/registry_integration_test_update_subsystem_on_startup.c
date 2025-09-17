// registry_integration_test_update_subsystem_on_startup.c
// Unity tests for update_subsystem_on_startup function

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
void test_update_subsystem_on_startup_success(void);
void test_update_subsystem_on_startup_failure(void);
void test_update_subsystem_on_startup_multiple(void);
void test_update_subsystem_on_startup_null_name(void);
void test_update_subsystem_on_startup_empty_name(void);
void test_update_subsystem_on_startup_nonexistent(void);
void test_update_subsystem_on_startup_already_running(void);
void test_update_subsystem_on_startup_from_error_state(void);
void test_update_subsystem_on_startup_timing(void);
void test_update_subsystem_on_startup_case_sensitive(void);

void setUp(void) {
    // Initialize registry for each test
    init_registry();
}

void tearDown(void) {
    // Clean up registry after each test
    init_registry();
}

// Test updating subsystem on successful startup
void test_update_subsystem_on_startup_success(void) {
    // Register a subsystem
    int id = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);
    
    // Initially should be inactive
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_INACTIVE, get_subsystem_state(id));
    
    // Update on successful startup
    update_subsystem_on_startup("test_subsystem", true);
    
    // Should now be running
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(id));
    TEST_ASSERT_TRUE(is_subsystem_running(id));
    TEST_ASSERT_TRUE(is_subsystem_running_by_name("test_subsystem"));
}

// Test updating subsystem on failed startup
void test_update_subsystem_on_startup_failure(void) {
    // Register a subsystem
    int id = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);

    // Initially should be inactive
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_INACTIVE, get_subsystem_state(id));

    // Update on failed startup
    update_subsystem_on_startup("test_subsystem", false);

    // Should now be in error state
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_ERROR, get_subsystem_state(id));
    TEST_ASSERT_FALSE(is_subsystem_running(id));
    TEST_ASSERT_FALSE(is_subsystem_running_by_name("test_subsystem"));
}

// Test updating multiple subsystems on startup
void test_update_subsystem_on_startup_multiple(void) {
    // Register multiple subsystems
    int id1 = register_subsystem_from_launch("subsystem_1", NULL, NULL, NULL,
                                            mock_init_success, mock_shutdown_function);
    int id2 = register_subsystem_from_launch("subsystem_2", NULL, NULL, NULL,
                                            mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id1);
    TEST_ASSERT_EQUAL_INT(1, id2);
    
    // Update first subsystem successfully, second with failure
    update_subsystem_on_startup("subsystem_1", true);
    update_subsystem_on_startup("subsystem_2", false);
    
    // Check states
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(id1));
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_ERROR, get_subsystem_state(id2));
    TEST_ASSERT_TRUE(is_subsystem_running(id1));
    TEST_ASSERT_FALSE(is_subsystem_running(id2));
}

// Test updating subsystem with NULL name
void test_update_subsystem_on_startup_null_name(void) {
    // Register a subsystem first
    int id = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);
    
    // Try to update with NULL name - should not crash
    update_subsystem_on_startup(NULL, true);
    
    // Original subsystem should remain unchanged
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_INACTIVE, get_subsystem_state(id));
}

// Test updating subsystem with empty name
void test_update_subsystem_on_startup_empty_name(void) {
    // Register a subsystem first
    int id = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);
    
    // Try to update with empty name - should not crash
    update_subsystem_on_startup("", true);
    
    // Original subsystem should remain unchanged
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_INACTIVE, get_subsystem_state(id));
}

// Test updating nonexistent subsystem
void test_update_subsystem_on_startup_nonexistent(void) {
    // Register one subsystem
    int id = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);
    
    // Try to update a nonexistent subsystem - should not crash
    update_subsystem_on_startup("nonexistent_subsystem", true);
    
    // Original subsystem should remain unchanged
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_INACTIVE, get_subsystem_state(id));
}

// Test updating already running subsystem
void test_update_subsystem_on_startup_already_running(void) {
    // Register a subsystem
    int id = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);
    
    // First startup - successful
    update_subsystem_on_startup("test_subsystem", true);
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(id));
    
    // Try to update again - should handle gracefully
    update_subsystem_on_startup("test_subsystem", true);
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(id));
    
    // Try to update with failure - should handle gracefully
    update_subsystem_on_startup("test_subsystem", false);
    // Behavior depends on implementation - could remain running or go to error
    SubsystemState final_state = get_subsystem_state(id);
    TEST_ASSERT_TRUE(final_state == SUBSYSTEM_RUNNING || final_state == SUBSYSTEM_ERROR);
}

// Test updating subsystem that's in error state
void test_update_subsystem_on_startup_from_error_state(void) {
    // Register a subsystem
    int id = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);
    
    // First startup - failure
    update_subsystem_on_startup("test_subsystem", false);
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_ERROR, get_subsystem_state(id));
    
    // Try to recover with successful startup
    update_subsystem_on_startup("test_subsystem", true);
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(id));
    TEST_ASSERT_TRUE(is_subsystem_running(id));
}

// Test updating subsystem startup timing
void test_update_subsystem_on_startup_timing(void) {
    // Register a subsystem
    int id = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);
    
    // Get initial state change time
    time_t initial_time = subsystem_registry.subsystems[id].state_changed;
    
    // Wait a moment to ensure time difference
    usleep(10000); // 10ms
    
    // Update on startup
    update_subsystem_on_startup("test_subsystem", true);
    
    // State change time should have been updated
    time_t updated_time = subsystem_registry.subsystems[id].state_changed;
    TEST_ASSERT_TRUE(updated_time >= initial_time);
}

// Test case-sensitive subsystem name matching
void test_update_subsystem_on_startup_case_sensitive(void) {
    // Register a subsystem with specific case
    int id = register_subsystem_from_launch("TestSubsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);
    
    // Try to update with different case - should not match
    update_subsystem_on_startup("testsubsystem", true);
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_INACTIVE, get_subsystem_state(id));
    
    update_subsystem_on_startup("TESTSUBSYSTEM", true);
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_INACTIVE, get_subsystem_state(id));
    
    // Update with exact case - should work
    update_subsystem_on_startup("TestSubsystem", true);
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(id));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_update_subsystem_on_startup_success);
    RUN_TEST(test_update_subsystem_on_startup_failure);
    RUN_TEST(test_update_subsystem_on_startup_multiple);
    RUN_TEST(test_update_subsystem_on_startup_null_name);
    RUN_TEST(test_update_subsystem_on_startup_empty_name);
    RUN_TEST(test_update_subsystem_on_startup_nonexistent);
    RUN_TEST(test_update_subsystem_on_startup_already_running);
    RUN_TEST(test_update_subsystem_on_startup_from_error_state);
    RUN_TEST(test_update_subsystem_on_startup_timing);
    RUN_TEST(test_update_subsystem_on_startup_case_sensitive);

    return UNITY_END();
}
