// registry_integration_test_update_subsystem_on_shutdown.c
// Unity tests for update_subsystem_on_shutdown function

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
void test_update_subsystem_on_shutdown_from_running(void);
void test_update_subsystem_on_shutdown_from_inactive(void);
void test_update_subsystem_on_shutdown_from_error(void);
void test_update_subsystem_on_shutdown_multiple(void);
void test_update_subsystem_on_shutdown_null_name(void);
void test_update_subsystem_on_shutdown_empty_name(void);
void test_update_subsystem_on_shutdown_nonexistent(void);
void test_update_subsystem_on_shutdown_already_stopping(void);
void test_update_subsystem_on_shutdown_timing(void);
void test_update_subsystem_on_shutdown_case_sensitive(void);
void test_update_subsystem_on_shutdown_with_dependencies(void);

void setUp(void) {
    // Initialize registry for each test
    init_registry();
}

void tearDown(void) {
    // Clean up registry after each test
    init_registry();
}

// Test updating subsystem on shutdown from running state
void test_update_subsystem_on_shutdown_from_running(void) {
    // Register and start a subsystem
    int id = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);

    // Start the subsystem
    update_subsystem_on_startup("test_subsystem", true);
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(id));

    // Update on shutdown
    update_subsystem_on_shutdown("test_subsystem");

    // Should now be stopping
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_STOPPING, get_subsystem_state(id));
    TEST_ASSERT_FALSE(is_subsystem_running(id));
    TEST_ASSERT_FALSE(is_subsystem_running_by_name("test_subsystem"));
}

// Test updating subsystem on shutdown from inactive state
void test_update_subsystem_on_shutdown_from_inactive(void) {
    // Register a subsystem but don't start it
    int id = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);
    
    // Should be inactive initially
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_INACTIVE, get_subsystem_state(id));
    
    // Update on shutdown - should handle gracefully
    update_subsystem_on_shutdown("test_subsystem");
    
    // State should remain inactive or go to stopping
    SubsystemState final_state = get_subsystem_state(id);
    TEST_ASSERT_TRUE(final_state == SUBSYSTEM_INACTIVE || final_state == SUBSYSTEM_STOPPING);
}

// Test updating subsystem on shutdown from error state
void test_update_subsystem_on_shutdown_from_error(void) {
    // Register a subsystem and put it in error state
    int id = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);
    
    // Put in error state
    update_subsystem_on_startup("test_subsystem", false);
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_ERROR, get_subsystem_state(id));
    
    // Update on shutdown - should handle gracefully
    update_subsystem_on_shutdown("test_subsystem");
    
    // State should go to stopping or remain in error
    SubsystemState final_state = get_subsystem_state(id);
    TEST_ASSERT_TRUE(final_state == SUBSYSTEM_STOPPING || final_state == SUBSYSTEM_ERROR);
}

// Test updating multiple subsystems on shutdown
void test_update_subsystem_on_shutdown_multiple(void) {
    // Register multiple subsystems and start them
    int id1 = register_subsystem_from_launch("subsystem_1", NULL, NULL, NULL,
                                            mock_init_success, mock_shutdown_function);
    int id2 = register_subsystem_from_launch("subsystem_2", NULL, NULL, NULL,
                                            mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id1);
    TEST_ASSERT_EQUAL_INT(1, id2);
    
    // Start both subsystems
    update_subsystem_on_startup("subsystem_1", true);
    update_subsystem_on_startup("subsystem_2", true);
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(id1));
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(id2));
    
    // Shutdown both subsystems
    update_subsystem_on_shutdown("subsystem_1");
    update_subsystem_on_shutdown("subsystem_2");
    
    // Both should be stopping
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_STOPPING, get_subsystem_state(id1));
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_STOPPING, get_subsystem_state(id2));
    TEST_ASSERT_FALSE(is_subsystem_running(id1));
    TEST_ASSERT_FALSE(is_subsystem_running(id2));
}

// Test updating subsystem with NULL name
void test_update_subsystem_on_shutdown_null_name(void) {
    // Register a subsystem first
    int id = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);
    
    // Start the subsystem
    update_subsystem_on_startup("test_subsystem", true);
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(id));
    
    // Try to update with NULL name - should not crash
    update_subsystem_on_shutdown(NULL);
    
    // Original subsystem should remain unchanged
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(id));
}

// Test updating subsystem with empty name
void test_update_subsystem_on_shutdown_empty_name(void) {
    // Register a subsystem first
    int id = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);
    
    // Start the subsystem
    update_subsystem_on_startup("test_subsystem", true);
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(id));
    
    // Try to update with empty name - should not crash
    update_subsystem_on_shutdown("");
    
    // Original subsystem should remain unchanged
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(id));
}

// Test updating nonexistent subsystem
void test_update_subsystem_on_shutdown_nonexistent(void) {
    // Register one subsystem
    int id = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);
    
    // Start the subsystem
    update_subsystem_on_startup("test_subsystem", true);
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(id));
    
    // Try to shutdown a nonexistent subsystem - should not crash
    update_subsystem_on_shutdown("nonexistent_subsystem");
    
    // Original subsystem should remain unchanged
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(id));
}

// Test updating already stopping subsystem
void test_update_subsystem_on_shutdown_already_stopping(void) {
    // Register a subsystem
    int id = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);
    
    // Start the subsystem
    update_subsystem_on_startup("test_subsystem", true);
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(id));
    
    // First shutdown
    update_subsystem_on_shutdown("test_subsystem");
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_STOPPING, get_subsystem_state(id));
    
    // Try to shutdown again - should handle gracefully
    update_subsystem_on_shutdown("test_subsystem");
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_STOPPING, get_subsystem_state(id));
}

// Test updating subsystem shutdown timing
void test_update_subsystem_on_shutdown_timing(void) {
    // Register a subsystem
    int id = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);
    
    // Start the subsystem
    update_subsystem_on_startup("test_subsystem", true);
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(id));
    
    // Get current state change time
    time_t initial_time = subsystem_registry.subsystems[id].state_changed;
    
    // Wait a moment to ensure time difference
    usleep(10000); // 10ms
    
    // Update on shutdown
    update_subsystem_on_shutdown("test_subsystem");
    
    // State change time should have been updated
    time_t updated_time = subsystem_registry.subsystems[id].state_changed;
    TEST_ASSERT_TRUE(updated_time >= initial_time);
}

// Test case-sensitive subsystem name matching
void test_update_subsystem_on_shutdown_case_sensitive(void) {
    // Register a subsystem with specific case
    int id = register_subsystem_from_launch("TestSubsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);
    
    // Start the subsystem
    update_subsystem_on_startup("TestSubsystem", true);
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(id));
    
    // Try to shutdown with different case - should not match
    update_subsystem_on_shutdown("testsubsystem");
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(id));
    
    update_subsystem_on_shutdown("TESTSUBSYSTEM");
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(id));
    
    // Shutdown with exact case - should work
    update_subsystem_on_shutdown("TestSubsystem");
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_STOPPING, get_subsystem_state(id));
}

// Test subsystem dependencies during shutdown
void test_update_subsystem_on_shutdown_with_dependencies(void) {
    // Register dependency and dependent subsystems
    int dep_id = register_subsystem_from_launch("dependency", NULL, NULL, NULL,
                                              mock_init_success, mock_shutdown_function);
    int id = register_subsystem_from_launch("dependent", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, dep_id);
    TEST_ASSERT_EQUAL_INT(1, id);
    
    // Add dependency
    bool dep_result = add_dependency_from_launch(id, "dependency");
    TEST_ASSERT_TRUE(dep_result);
    
    // Start both subsystems
    update_subsystem_on_startup("dependency", true);
    update_subsystem_on_startup("dependent", true);
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(dep_id));
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(id));
    
    // Shutdown the dependent first - should work
    update_subsystem_on_shutdown("dependent");
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_STOPPING, get_subsystem_state(id));
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(dep_id)); // Dependency still running
    
    // Now shutdown the dependency
    update_subsystem_on_shutdown("dependency");
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_STOPPING, get_subsystem_state(dep_id));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_update_subsystem_on_shutdown_from_running);
    RUN_TEST(test_update_subsystem_on_shutdown_from_inactive);
    RUN_TEST(test_update_subsystem_on_shutdown_from_error);
    RUN_TEST(test_update_subsystem_on_shutdown_multiple);
    RUN_TEST(test_update_subsystem_on_shutdown_null_name);
    RUN_TEST(test_update_subsystem_on_shutdown_empty_name);
    RUN_TEST(test_update_subsystem_on_shutdown_nonexistent);
    RUN_TEST(test_update_subsystem_on_shutdown_already_stopping);
    RUN_TEST(test_update_subsystem_on_shutdown_timing);
    RUN_TEST(test_update_subsystem_on_shutdown_case_sensitive);
    RUN_TEST(test_update_subsystem_on_shutdown_with_dependencies);

    return UNITY_END();
}
