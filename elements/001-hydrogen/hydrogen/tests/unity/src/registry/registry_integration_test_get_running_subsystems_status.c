// registry_integration_test_get_running_subsystems_status.c
// Unity tests for get_running_subsystems_status function

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

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
void test_get_running_subsystems_status_empty_registry(void);
void test_get_running_subsystems_status_single_running(void);
void test_get_running_subsystems_status_multiple_running(void);
void test_get_running_subsystems_status_mixed_states(void);
void test_get_running_subsystems_status_stopping_subsystem(void);
void test_get_running_subsystems_status_null_buffer(void);
void test_get_running_subsystems_status_with_dependencies(void);
void test_get_running_subsystems_status_state_changes(void);
void test_get_running_subsystems_status_many_subsystems(void);
void test_get_running_subsystems_status_buffer_validity(void);

void setUp(void) {
    // Initialize registry for each test
    init_registry();
}

void tearDown(void) {
    // Clean up registry after each test
    init_registry();
}

// Test getting status with no subsystems registered
void test_get_running_subsystems_status_empty_registry(void) {
    char* status_buffer = NULL;
    
    bool result = get_running_subsystems_status(&status_buffer);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(status_buffer);
    TEST_ASSERT_TRUE(strlen(status_buffer) > 0); // Should have some content even if empty
    
    // Clean up
    free(status_buffer);
}

// Test getting status with one running subsystem
void test_get_running_subsystems_status_single_running(void) {
    // Register and start a subsystem
    int id = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);
    
    update_subsystem_on_startup("test_subsystem", true);
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(id));
    
    char* status_buffer = NULL;
    bool result = get_running_subsystems_status(&status_buffer);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(status_buffer);
    TEST_ASSERT_TRUE(strlen(status_buffer) > 0);
    
    // Should contain the subsystem name
    TEST_ASSERT_NOT_NULL(strstr(status_buffer, "test_subsystem"));
    
    // Clean up
    free(status_buffer);
}

// Test getting status with multiple running subsystems
void test_get_running_subsystems_status_multiple_running(void) {
    // Register and start multiple subsystems
    int id1 = register_subsystem_from_launch("subsystem_1", NULL, NULL, NULL,
                                            mock_init_success, mock_shutdown_function);
    int id2 = register_subsystem_from_launch("subsystem_2", NULL, NULL, NULL,
                                            mock_init_success, mock_shutdown_function);
    int id3 = register_subsystem_from_launch("subsystem_3", NULL, NULL, NULL,
                                            mock_init_success, mock_shutdown_function);
    
    TEST_ASSERT_EQUAL_INT(0, id1);
    TEST_ASSERT_EQUAL_INT(1, id2);
    TEST_ASSERT_EQUAL_INT(2, id3);
    
    // Start all subsystems
    update_subsystem_on_startup("subsystem_1", true);
    update_subsystem_on_startup("subsystem_2", true);
    update_subsystem_on_startup("subsystem_3", true);
    
    char* status_buffer = NULL;
    bool result = get_running_subsystems_status(&status_buffer);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(status_buffer);
    TEST_ASSERT_TRUE(strlen(status_buffer) > 0);
    
    // Should contain all subsystem names
    TEST_ASSERT_NOT_NULL(strstr(status_buffer, "subsystem_1"));
    TEST_ASSERT_NOT_NULL(strstr(status_buffer, "subsystem_2"));
    TEST_ASSERT_NOT_NULL(strstr(status_buffer, "subsystem_3"));
    
    // Clean up
    free(status_buffer);
}

// Test getting status with mixed subsystem states
void test_get_running_subsystems_status_mixed_states(void) {
    // Register multiple subsystems in different states
    int id1 = register_subsystem_from_launch("running_subsystem", NULL, NULL, NULL,
                                            mock_init_success, mock_shutdown_function);
    int id2 = register_subsystem_from_launch("inactive_subsystem", NULL, NULL, NULL,
                                            mock_init_success, mock_shutdown_function);
    int id3 = register_subsystem_from_launch("error_subsystem", NULL, NULL, NULL,
                                            mock_init_success, mock_shutdown_function);
    
    TEST_ASSERT_EQUAL_INT(0, id1);
    TEST_ASSERT_EQUAL_INT(1, id2);
    TEST_ASSERT_EQUAL_INT(2, id3);
    
    // Set different states
    update_subsystem_on_startup("running_subsystem", true);  // RUNNING
    // inactive_subsystem remains INACTIVE
    update_subsystem_on_startup("error_subsystem", false);   // ERROR
    
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(id1));
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_INACTIVE, get_subsystem_state(id2));
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_ERROR, get_subsystem_state(id3));
    
    char* status_buffer = NULL;
    bool result = get_running_subsystems_status(&status_buffer);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(status_buffer);
    TEST_ASSERT_TRUE(strlen(status_buffer) > 0);
    
    // Should contain the running subsystem
    TEST_ASSERT_NOT_NULL(strstr(status_buffer, "running_subsystem"));
    
    // May or may not contain non-running subsystems depending on implementation
    // The function name suggests it should only show running ones
    
    // Clean up
    free(status_buffer);
}

// Test getting status with stopping subsystem
void test_get_running_subsystems_status_stopping_subsystem(void) {
    // Register and start a subsystem
    int id = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);
    
    update_subsystem_on_startup("test_subsystem", true);
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_RUNNING, get_subsystem_state(id));
    
    // Start shutdown process
    update_subsystem_on_shutdown("test_subsystem");
    TEST_ASSERT_EQUAL_INT(SUBSYSTEM_STOPPING, get_subsystem_state(id));
    
    char* status_buffer = NULL;
    bool result = get_running_subsystems_status(&status_buffer);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(status_buffer);
    
    // Behavior depends on implementation - stopping subsystems may or may not be included
    
    // Clean up
    free(status_buffer);
}

// Test getting status with NULL buffer pointer
void test_get_running_subsystems_status_null_buffer(void) {
    bool result = get_running_subsystems_status(NULL);
    
    TEST_ASSERT_FALSE(result); // Should fail with NULL buffer pointer
}

// Test getting status with subsystem dependencies
void test_get_running_subsystems_status_with_dependencies(void) {
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
    
    char* status_buffer = NULL;
    bool result = get_running_subsystems_status(&status_buffer);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(status_buffer);
    TEST_ASSERT_TRUE(strlen(status_buffer) > 0);
    
    // Should contain both subsystems
    TEST_ASSERT_NOT_NULL(strstr(status_buffer, "dependency"));
    TEST_ASSERT_NOT_NULL(strstr(status_buffer, "dependent"));
    
    // Clean up
    free(status_buffer);
}

// Test getting status after subsystem state changes
void test_get_running_subsystems_status_state_changes(void) {
    // Register a subsystem
    int id = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);
    
    // Initially inactive - get status
    char* status_buffer1 = NULL;
    bool result1 = get_running_subsystems_status(&status_buffer1);
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_NOT_NULL(status_buffer1);
    
    // Start subsystem - get status again
    update_subsystem_on_startup("test_subsystem", true);
    
    char* status_buffer2 = NULL;
    bool result2 = get_running_subsystems_status(&status_buffer2);
    TEST_ASSERT_TRUE(result2);
    TEST_ASSERT_NOT_NULL(status_buffer2);
    
    // Second status should contain the subsystem
    TEST_ASSERT_NOT_NULL(strstr(status_buffer2, "test_subsystem"));
    
    // Stop subsystem - get status again
    update_subsystem_on_shutdown("test_subsystem");
    
    char* status_buffer3 = NULL;
    bool result3 = get_running_subsystems_status(&status_buffer3);
    TEST_ASSERT_TRUE(result3);
    TEST_ASSERT_NOT_NULL(status_buffer3);
    
    // Clean up
    free(status_buffer1);
    free(status_buffer2);
    free(status_buffer3);
}

// Test getting status with large number of subsystems
void test_get_running_subsystems_status_many_subsystems(void) {
    const int num_subsystems = 10;
    
    // Register multiple subsystems
    for (int i = 0; i < num_subsystems; i++) {
        char name[32];
        snprintf(name, sizeof(name), "subsystem_%d", i);
        
        int id = register_subsystem_from_launch(name, NULL, NULL, NULL,
                                               mock_init_success, mock_shutdown_function);
        TEST_ASSERT_EQUAL_INT(i, id);
        
        // Start every other subsystem
        if (i % 2 == 0) {
            update_subsystem_on_startup(name, true);
        }
    }
    
    char* status_buffer = NULL;
    bool result = get_running_subsystems_status(&status_buffer);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(status_buffer);
    TEST_ASSERT_TRUE(strlen(status_buffer) > 0);
    
    // Should contain the running subsystems (even-numbered ones)
    for (int i = 0; i < num_subsystems; i += 2) {
        char name[32];
        snprintf(name, sizeof(name), "subsystem_%d", i);
        TEST_ASSERT_NOT_NULL(strstr(status_buffer, name));
    }
    
    // Clean up
    free(status_buffer);
}

// Test that buffer is properly allocated and contains valid data
void test_get_running_subsystems_status_buffer_validity(void) {
    // Register a subsystem
    int id = register_subsystem_from_launch("test_subsystem", NULL, NULL, NULL,
                                           mock_init_success, mock_shutdown_function);
    TEST_ASSERT_EQUAL_INT(0, id);
    
    update_subsystem_on_startup("test_subsystem", true);
    
    char* status_buffer = NULL;
    bool result = get_running_subsystems_status(&status_buffer);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(status_buffer);
    
    // Buffer should be null-terminated string
    size_t buffer_len = strlen(status_buffer);
    TEST_ASSERT_TRUE(buffer_len > 0);
    TEST_ASSERT_EQUAL_CHAR('\0', status_buffer[buffer_len]);
    
    // Should be valid memory that we can read
    for (size_t i = 0; i < buffer_len; i++) {
        char c = status_buffer[i];
        // Should be printable characters or common whitespace
        TEST_ASSERT_TRUE(c >= 32 || c == '\n' || c == '\r' || c == '\t');
    }
    
    // Clean up
    free(status_buffer);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_get_running_subsystems_status_empty_registry);
    RUN_TEST(test_get_running_subsystems_status_single_running);
    RUN_TEST(test_get_running_subsystems_status_multiple_running);
    RUN_TEST(test_get_running_subsystems_status_mixed_states);
    RUN_TEST(test_get_running_subsystems_status_stopping_subsystem);
    RUN_TEST(test_get_running_subsystems_status_null_buffer);
    RUN_TEST(test_get_running_subsystems_status_with_dependencies);
    RUN_TEST(test_get_running_subsystems_status_state_changes);
    RUN_TEST(test_get_running_subsystems_status_many_subsystems);
    RUN_TEST(test_get_running_subsystems_status_buffer_validity);

    return UNITY_END();
}
