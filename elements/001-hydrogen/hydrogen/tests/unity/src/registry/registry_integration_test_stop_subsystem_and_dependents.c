/*
 * Unity Test File: registry_integration_test_stop_subsystem_and_dependents.c
 * 
 * Tests for stop_subsystem_and_dependents function from registry_integration.c
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Function prototypes to avoid -Werror=missing-prototypes
void test_stop_subsystem_and_dependents_no_dependents(void);
void test_stop_subsystem_and_dependents_with_dependents(void);
void test_stop_subsystem_and_dependents_already_stopped(void);
void test_stop_subsystem_and_dependents_invalid_id(void);
void test_stop_subsystem_and_dependents_chain_dependencies(void);

// Include the module under test
extern void initialize_registry(void);
extern bool stop_subsystem_and_dependents(int subsystem_id);
extern bool add_subsystem_dependency(int subsystem_id, const char* dependency_name);
extern int get_subsystem_id_by_name(const char* name);

static pthread_t dummy_thread = 0;
static volatile sig_atomic_t dummy_shutdown_flag = 0;
static int shutdown_call_count = 0;

static int dummy_init(void) { return 1; }

static void counting_shutdown(void) { 
    shutdown_call_count++;
}

static void dummy_shutdown(void) __attribute__((unused)); 
static void dummy_shutdown(void) { }

void setUp(void) {
    initialize_registry();
    shutdown_call_count = 0;
}

void tearDown(void) {
    // Registry will be reinitialized in setUp
}

void test_stop_subsystem_and_dependents_no_dependents(void) {
    // Register a simple subsystem with no dependents
    int subsystem_id = register_subsystem("standalone_subsystem", NULL, &dummy_thread, 
                                         &dummy_shutdown_flag, dummy_init, counting_shutdown);
    TEST_ASSERT_TRUE(subsystem_id >= 0);
    
    // Set it to running
    update_subsystem_state(subsystem_id, SUBSYSTEM_RUNNING);
    
    // Stop it
    bool result = stop_subsystem_and_dependents(subsystem_id);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, shutdown_call_count);  // Should have called shutdown once
}

void test_stop_subsystem_and_dependents_with_dependents(void) {
    // Register a base subsystem
    int base_id = register_subsystem("base_subsystem", NULL, &dummy_thread,
                                    &dummy_shutdown_flag, dummy_init, counting_shutdown);
    TEST_ASSERT_TRUE(base_id >= 0);
    
    // Register a dependent subsystem  
    int dependent_id = register_subsystem("dependent_subsystem", NULL, &dummy_thread,
                                         &dummy_shutdown_flag, dummy_init, counting_shutdown);
    TEST_ASSERT_TRUE(dependent_id >= 0);
    
    // Add dependency: dependent depends on base
    bool dep_result = add_subsystem_dependency(dependent_id, "base_subsystem");
    TEST_ASSERT_TRUE(dep_result);
    
    // Set both to running
    update_subsystem_state(base_id, SUBSYSTEM_RUNNING);
    update_subsystem_state(dependent_id, SUBSYSTEM_RUNNING);
    
    // Try to stop the base - should stop dependent first, then base
    bool result = stop_subsystem_and_dependents(base_id);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, shutdown_call_count);  // Should have called shutdown twice
}

void test_stop_subsystem_and_dependents_already_stopped(void) {
    // Register a subsystem
    int subsystem_id = register_subsystem("stopped_subsystem", NULL, &dummy_thread,
                                         &dummy_shutdown_flag, dummy_init, counting_shutdown);
    TEST_ASSERT_TRUE(subsystem_id >= 0);
    
    // Leave it in INACTIVE state (default)
    
    // Try to stop it
    bool result = stop_subsystem_and_dependents(subsystem_id);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, shutdown_call_count);  // Should not have called shutdown
}

void test_stop_subsystem_and_dependents_invalid_id(void) {
    // Try to stop non-existent subsystem
    stop_subsystem_and_dependents(999);
    
    // Should handle gracefully (implementation dependent, but shouldn't crash)
    TEST_ASSERT_EQUAL(0, shutdown_call_count);  // Should not have called shutdown
}

void test_stop_subsystem_and_dependents_chain_dependencies(void) {
    // Create a chain: A <- B <- C (C depends on B, B depends on A)
    int a_id = register_subsystem("subsystem_a", NULL, &dummy_thread,
                                 &dummy_shutdown_flag, dummy_init, counting_shutdown);
    int b_id = register_subsystem("subsystem_b", NULL, &dummy_thread,
                                 &dummy_shutdown_flag, dummy_init, counting_shutdown);
    int c_id = register_subsystem("subsystem_c", NULL, &dummy_thread,
                                 &dummy_shutdown_flag, dummy_init, counting_shutdown);
    
    TEST_ASSERT_TRUE(a_id >= 0);
    TEST_ASSERT_TRUE(b_id >= 0);
    TEST_ASSERT_TRUE(c_id >= 0);
    
    // Set up dependencies
    add_subsystem_dependency(b_id, "subsystem_a");  // B depends on A
    add_subsystem_dependency(c_id, "subsystem_b");  // C depends on B
    
    // Set all to running
    update_subsystem_state(a_id, SUBSYSTEM_RUNNING);
    update_subsystem_state(b_id, SUBSYSTEM_RUNNING);
    update_subsystem_state(c_id, SUBSYSTEM_RUNNING);
    
    // Stop A - should cascade stop C, then B, then A
    bool result = stop_subsystem_and_dependents(a_id);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(3, shutdown_call_count);  // Should have called shutdown three times
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_stop_subsystem_and_dependents_no_dependents);
    RUN_TEST(test_stop_subsystem_and_dependents_with_dependents);
    RUN_TEST(test_stop_subsystem_and_dependents_already_stopped);
    RUN_TEST(test_stop_subsystem_and_dependents_invalid_id);
    RUN_TEST(test_stop_subsystem_and_dependents_chain_dependencies);
    
    return UNITY_END();
}
