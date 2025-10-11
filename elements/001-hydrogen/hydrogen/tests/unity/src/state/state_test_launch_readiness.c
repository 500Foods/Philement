/*
 * Unity Test File: state_test_launch_readiness.c
 *
 * Tests for LaunchReadiness structure validation and usage patterns from state_types.h and state.c
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include state_types.h for LaunchReadiness structure
#include <src/state/state_types.h>

// Function prototypes for test functions
void test_launch_readiness_structure_initialization(void);
void test_launch_readiness_structure_assignment(void);
void test_launch_readiness_structure_validation(void);
void test_launch_readiness_structure_with_messages(void);
void test_launch_readiness_structure_cleanup(void);
void test_launch_readiness_structure_edge_cases(void);
void test_launch_readiness_structure_memory_safety(void);

void setUp(void) {
    // No special setup needed for structure tests
}

void tearDown(void) {
    // No special cleanup needed for structure tests
}

// Tests for LaunchReadiness structure
void test_launch_readiness_structure_initialization(void) {
    // Test structure initialization with zeros
    LaunchReadiness readiness = {0};

    // Verify default values
    TEST_ASSERT_NULL(readiness.subsystem);  // NULL, not empty string
    TEST_ASSERT_FALSE(readiness.ready);
    TEST_ASSERT_NULL(readiness.messages);

    // Should not crash
    TEST_PASS();
}

void test_launch_readiness_structure_assignment(void) {
    // Test structure field assignment
    LaunchReadiness readiness;

    // Assign values
    readiness.subsystem = "test_subsystem";
    readiness.ready = true;
    readiness.messages = NULL;

    // Verify assignments
    TEST_ASSERT_EQUAL_STRING("test_subsystem", readiness.subsystem);
    TEST_ASSERT_TRUE(readiness.ready);
    TEST_ASSERT_NULL(readiness.messages);

    // Should not crash
    TEST_PASS();
}

void test_launch_readiness_structure_validation(void) {
    // Test structure with valid data
    LaunchReadiness readiness = {
        .subsystem = "logging",
        .ready = true,
        .messages = NULL
    };

    // Verify structure integrity
    TEST_ASSERT_EQUAL_STRING("logging", readiness.subsystem);
    TEST_ASSERT_TRUE(readiness.ready);
    TEST_ASSERT_NULL(readiness.messages);

    // Test with different values
    readiness.ready = false;
    readiness.subsystem = "database";

    TEST_ASSERT_EQUAL_STRING("database", readiness.subsystem);
    TEST_ASSERT_FALSE(readiness.ready);

    // Should not crash
    TEST_PASS();
}

void test_launch_readiness_structure_with_messages(void) {
    // Test structure with messages array
    const char** messages = calloc(3, sizeof(const char*));
    TEST_ASSERT_NOT_NULL(messages);
    messages[0] = "Message 1";
    messages[1] = "Message 2";
    messages[2] = NULL; // NULL-terminated

    LaunchReadiness readiness = {
        .subsystem = "test",
        .ready = false,
        .messages = messages
    };

    // Verify structure
    TEST_ASSERT_EQUAL_STRING("test", readiness.subsystem);
    TEST_ASSERT_FALSE(readiness.ready);
    TEST_ASSERT_NOT_NULL(readiness.messages);
    TEST_ASSERT_EQUAL_STRING("Message 1", readiness.messages[0]);
    TEST_ASSERT_EQUAL_STRING("Message 2", readiness.messages[1]);
    TEST_ASSERT_NULL(readiness.messages[2]);

    // Cleanup
    free(messages);

    // Should not crash
    TEST_PASS();
}

void test_launch_readiness_structure_cleanup(void) {
    // Test proper cleanup patterns for LaunchReadiness
    LaunchReadiness readiness = {0};

    // Simulate cleanup by setting messages to NULL (as free_readiness_messages does)
    readiness.messages = NULL;

    // Verify cleanup state
    TEST_ASSERT_NULL(readiness.messages);

    // Test with populated structure
    readiness.subsystem = "cleanup_test";
    readiness.ready = true;

    // These fields don't need cleanup in the structure itself
    TEST_ASSERT_EQUAL_STRING("cleanup_test", readiness.subsystem);
    TEST_ASSERT_TRUE(readiness.ready);

    // Should not crash
    TEST_PASS();
}

void test_launch_readiness_structure_edge_cases(void) {
    // Test edge cases for LaunchReadiness structure

    // Test with empty subsystem name
    LaunchReadiness readiness1 = {
        .subsystem = "",
        .ready = true,
        .messages = NULL
    };
    TEST_ASSERT_EQUAL_STRING("", readiness1.subsystem);

    // Test with very long subsystem name (conceptual test)
    const char* long_name = "very_long_subsystem_name_that_might_be_used_in_real_systems";
    LaunchReadiness readiness2 = {
        .subsystem = long_name,
        .ready = false,
        .messages = NULL
    };
    TEST_ASSERT_EQUAL_STRING(long_name, readiness2.subsystem);

    // Test boolean edge cases
    LaunchReadiness readiness3 = {.ready = false};
    TEST_ASSERT_FALSE(readiness3.ready);

    readiness3.ready = true;
    TEST_ASSERT_TRUE(readiness3.ready);

    // Should not crash
    TEST_PASS();
}

void test_launch_readiness_structure_memory_safety(void) {
    // Test memory safety aspects of LaunchReadiness usage

    // Test that we can safely create and destroy structures
    LaunchReadiness* readiness_ptr = calloc(1, sizeof(LaunchReadiness));
    TEST_ASSERT_NOT_NULL(readiness_ptr);

    // Initialize
    readiness_ptr->subsystem = "memory_test";
    readiness_ptr->ready = true;
    readiness_ptr->messages = NULL;

    // Verify
    TEST_ASSERT_EQUAL_STRING("memory_test", readiness_ptr->subsystem);
    TEST_ASSERT_TRUE(readiness_ptr->ready);

    // Cleanup
    free(readiness_ptr);

    // Test multiple allocations
    for (int i = 0; i < 10; i++) {
        LaunchReadiness* temp = calloc(1, sizeof(LaunchReadiness));
        TEST_ASSERT_NOT_NULL(temp);
        temp->subsystem = "temp";
        free(temp);
    }

    // Should not crash
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_launch_readiness_structure_initialization);
    RUN_TEST(test_launch_readiness_structure_assignment);
    RUN_TEST(test_launch_readiness_structure_validation);
    RUN_TEST(test_launch_readiness_structure_with_messages);
    RUN_TEST(test_launch_readiness_structure_cleanup);
    RUN_TEST(test_launch_readiness_structure_edge_cases);
    RUN_TEST(test_launch_readiness_structure_memory_safety);

    return UNITY_END();
}
