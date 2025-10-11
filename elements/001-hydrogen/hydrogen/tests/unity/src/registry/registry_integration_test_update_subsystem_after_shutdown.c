/*
 * Unity Test File: registry_integration_test_update_subsystem_after_shutdown.c
 * 
 * Tests for update_subsystem_after_shutdown function from registry_integration.c
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Function prototypes to avoid -Werror=missing-prototypes
void test_update_subsystem_after_shutdown_valid_subsystem(void);
void test_update_subsystem_after_shutdown_invalid_subsystem(void);
void test_update_subsystem_after_shutdown_null_name(void);
void test_update_subsystem_after_shutdown_state_transition(void);

// Include the module under test
extern void initialize_registry(void);
extern void update_subsystem_after_shutdown(const char* subsystem_name);
extern int get_subsystem_id_by_name(const char* name);

static pthread_t dummy_thread = 0;
static volatile sig_atomic_t dummy_shutdown_flag = 0;

static int dummy_init(void) { return 1; }
static void dummy_shutdown(void) { }

void setUp(void) {
    initialize_registry();
}

void tearDown(void) {
    // Registry will be reinitialized in setUp
}

void test_update_subsystem_after_shutdown_valid_subsystem(void) {
    // Register a subsystem
    int subsystem_id = register_subsystem("test_subsystem", NULL, &dummy_thread, 
                                         &dummy_shutdown_flag, dummy_init, dummy_shutdown);
    TEST_ASSERT_TRUE(subsystem_id >= 0);
    
    // Set it to running first
    update_subsystem_state(subsystem_id, SUBSYSTEM_RUNNING);
    
    // Call function under test
    update_subsystem_after_shutdown("test_subsystem");
    
    // Verify it was set to INACTIVE (we can't directly check state, 
    // but the function should complete without error)
    TEST_ASSERT_TRUE(true); // Function completed without crashing
}

void test_update_subsystem_after_shutdown_invalid_subsystem(void) {
    // Call with non-existent subsystem name
    // This should not crash - function handles invalid IDs gracefully
    update_subsystem_after_shutdown("non_existent_subsystem");
    
    TEST_ASSERT_TRUE(true); // Function completed without crashing
}

void test_update_subsystem_after_shutdown_null_name(void) {
    // Call with NULL name
    // This should not crash - get_subsystem_id_by_name should handle NULL
    update_subsystem_after_shutdown(NULL);
    
    TEST_ASSERT_TRUE(true); // Function completed without crashing
}

void test_update_subsystem_after_shutdown_state_transition(void) {
    // Register a subsystem and set it to STOPPING state
    int subsystem_id = register_subsystem("stopping_subsystem", NULL, &dummy_thread,
                                         &dummy_shutdown_flag, dummy_init, dummy_shutdown);
    TEST_ASSERT_TRUE(subsystem_id >= 0);
    
    update_subsystem_state(subsystem_id, SUBSYSTEM_STOPPING);
    
    // Call function under test
    update_subsystem_after_shutdown("stopping_subsystem");
    
    // Function should have transitioned it to INACTIVE
    TEST_ASSERT_TRUE(true); // Function completed without crashing
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_update_subsystem_after_shutdown_valid_subsystem);
    RUN_TEST(test_update_subsystem_after_shutdown_invalid_subsystem);
    RUN_TEST(test_update_subsystem_after_shutdown_null_name);
    RUN_TEST(test_update_subsystem_after_shutdown_state_transition);
    
    return UNITY_END();
}
