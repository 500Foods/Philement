/*
 * Unity Test File: registry_integration_test_stop_all_subsystems_in_dependency_order.c
 * 
 * Tests for stop_all_subsystems_in_dependency_order function from registry_integration.c
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Function prototypes to avoid -Werror=missing-prototypes
void test_stop_all_subsystems_empty_registry(void);
void test_stop_all_subsystems_no_dependencies(void);
void test_stop_all_subsystems_with_dependencies(void);
void test_stop_all_subsystems_complex_dependency_tree(void);
void test_stop_all_subsystems_partial_running(void);

// Include the module under test
extern void initialize_registry(void);
extern size_t stop_all_subsystems_in_dependency_order(void);
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

void test_stop_all_subsystems_empty_registry(void) {
    // Call with empty registry
    size_t stopped_count = stop_all_subsystems_in_dependency_order();
    
    TEST_ASSERT_EQUAL(0, stopped_count);
    TEST_ASSERT_EQUAL(0, shutdown_call_count);
}

void test_stop_all_subsystems_no_dependencies(void) {
    // Register multiple independent subsystems
    int sub1_id = register_subsystem("independent1", NULL, &dummy_thread,
                                     &dummy_shutdown_flag, dummy_init, counting_shutdown);
    int sub2_id = register_subsystem("independent2", NULL, &dummy_thread,
                                     &dummy_shutdown_flag, dummy_init, counting_shutdown);
    int sub3_id = register_subsystem("independent3", NULL, &dummy_thread,
                                     &dummy_shutdown_flag, dummy_init, counting_shutdown);
    
    TEST_ASSERT_TRUE(sub1_id >= 0);
    TEST_ASSERT_TRUE(sub2_id >= 0);
    TEST_ASSERT_TRUE(sub3_id >= 0);
    
    // Set all to running
    update_subsystem_state(sub1_id, SUBSYSTEM_RUNNING);
    update_subsystem_state(sub2_id, SUBSYSTEM_RUNNING);
    update_subsystem_state(sub3_id, SUBSYSTEM_RUNNING);
    
    // Stop all
    size_t stopped_count = stop_all_subsystems_in_dependency_order();
    
    TEST_ASSERT_EQUAL(3, stopped_count);
    TEST_ASSERT_EQUAL(3, shutdown_call_count);
}

void test_stop_all_subsystems_with_dependencies(void) {
    // Create dependency chain: base <- dependent1 <- dependent2
    int base_id = register_subsystem("base", NULL, &dummy_thread,
                                     &dummy_shutdown_flag, dummy_init, counting_shutdown);
    int dep1_id = register_subsystem("dependent1", NULL, &dummy_thread,
                                     &dummy_shutdown_flag, dummy_init, counting_shutdown);
    int dep2_id = register_subsystem("dependent2", NULL, &dummy_thread,
                                     &dummy_shutdown_flag, dummy_init, counting_shutdown);
    
    TEST_ASSERT_TRUE(base_id >= 0);
    TEST_ASSERT_TRUE(dep1_id >= 0);
    TEST_ASSERT_TRUE(dep2_id >= 0);
    
    // Set up dependencies
    add_subsystem_dependency(dep1_id, "base");        // dependent1 depends on base
    add_subsystem_dependency(dep2_id, "dependent1");  // dependent2 depends on dependent1
    
    // Set all to running
    update_subsystem_state(base_id, SUBSYSTEM_RUNNING);
    update_subsystem_state(dep1_id, SUBSYSTEM_RUNNING);
    update_subsystem_state(dep2_id, SUBSYSTEM_RUNNING);
    
    // Stop all - should stop in dependency order: dependent2, then dependent1, then base
    size_t stopped_count = stop_all_subsystems_in_dependency_order();
    
    TEST_ASSERT_EQUAL(3, stopped_count);
    TEST_ASSERT_EQUAL(3, shutdown_call_count);
}

void test_stop_all_subsystems_complex_dependency_tree(void) {
    /* Create complex dependency tree:
     *     root
     *    /    \
     *  child1  child2
     *    |       |
     *  leaf1   leaf2
     */
    
    int root_id = register_subsystem("root", NULL, &dummy_thread,
                                     &dummy_shutdown_flag, dummy_init, counting_shutdown);
    int child1_id = register_subsystem("child1", NULL, &dummy_thread,
                                       &dummy_shutdown_flag, dummy_init, counting_shutdown);
    int child2_id = register_subsystem("child2", NULL, &dummy_thread,
                                       &dummy_shutdown_flag, dummy_init, counting_shutdown);
    int leaf1_id = register_subsystem("leaf1", NULL, &dummy_thread,
                                      &dummy_shutdown_flag, dummy_init, counting_shutdown);
    int leaf2_id = register_subsystem("leaf2", NULL, &dummy_thread,
                                      &dummy_shutdown_flag, dummy_init, counting_shutdown);
    
    TEST_ASSERT_TRUE(root_id >= 0);
    TEST_ASSERT_TRUE(child1_id >= 0);
    TEST_ASSERT_TRUE(child2_id >= 0);
    TEST_ASSERT_TRUE(leaf1_id >= 0);
    TEST_ASSERT_TRUE(leaf2_id >= 0);
    
    // Set up dependencies
    add_subsystem_dependency(child1_id, "root");    // child1 depends on root
    add_subsystem_dependency(child2_id, "root");    // child2 depends on root
    add_subsystem_dependency(leaf1_id, "child1");   // leaf1 depends on child1
    add_subsystem_dependency(leaf2_id, "child2");   // leaf2 depends on child2
    
    // Set all to running
    update_subsystem_state(root_id, SUBSYSTEM_RUNNING);
    update_subsystem_state(child1_id, SUBSYSTEM_RUNNING);
    update_subsystem_state(child2_id, SUBSYSTEM_RUNNING);
    update_subsystem_state(leaf1_id, SUBSYSTEM_RUNNING);
    update_subsystem_state(leaf2_id, SUBSYSTEM_RUNNING);
    
    // Stop all - should stop leaves first, then children, then root
    size_t stopped_count = stop_all_subsystems_in_dependency_order();
    
    TEST_ASSERT_EQUAL(5, stopped_count);
    TEST_ASSERT_EQUAL(5, shutdown_call_count);
}

void test_stop_all_subsystems_partial_running(void) {
    // Register subsystems but only set some to running
    int sub1_id = register_subsystem("running1", NULL, &dummy_thread,
                                     &dummy_shutdown_flag, dummy_init, counting_shutdown);
    int sub2_id = register_subsystem("stopped2", NULL, &dummy_thread,
                                     &dummy_shutdown_flag, dummy_init, counting_shutdown);
    int sub3_id = register_subsystem("running3", NULL, &dummy_thread,
                                     &dummy_shutdown_flag, dummy_init, counting_shutdown);
    
    TEST_ASSERT_TRUE(sub1_id >= 0);
    TEST_ASSERT_TRUE(sub2_id >= 0);
    TEST_ASSERT_TRUE(sub3_id >= 0);
    
    // Set only some to running (sub2 stays INACTIVE)
    update_subsystem_state(sub1_id, SUBSYSTEM_RUNNING);
    update_subsystem_state(sub3_id, SUBSYSTEM_RUNNING);
    
    // Stop all
    size_t stopped_count = stop_all_subsystems_in_dependency_order();
    
    // Should only stop the running ones
    TEST_ASSERT_EQUAL(2, stopped_count);
    TEST_ASSERT_EQUAL(2, shutdown_call_count);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_stop_all_subsystems_empty_registry);
    RUN_TEST(test_stop_all_subsystems_no_dependencies);
    RUN_TEST(test_stop_all_subsystems_with_dependencies);
    RUN_TEST(test_stop_all_subsystems_complex_dependency_tree);
    RUN_TEST(test_stop_all_subsystems_partial_running);
    
    return UNITY_END();
}
