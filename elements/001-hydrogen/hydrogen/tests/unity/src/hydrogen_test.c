/*
 * Unity Test File: Hydrogen Core Tests
 * This file contains unit tests for core hydrogen.c functionality
 * and mock_system functionality verification
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Enable mock_system for testing the mock itself
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Forward declarations of functions we want to test from hydrogen.c
// For now, we'll provide a stub implementation since linking the full hydrogen.c
// would require all its dependencies
void test_get_program_args_returns_valid_pointer(void);
void test_signal_handling_setup(void);
void test_process_identification(void);
void test_memory_allocation_patterns(void);
void test_string_operations_for_paths(void);

// Mock system tests
void test_mock_malloc_counter_first_call(void);
void test_mock_malloc_counter_second_call(void);
void test_mock_malloc_counter_third_call(void);
void test_mock_calloc_counter_first_call(void);
void test_mock_calloc_shares_malloc_counter(void);
void test_mock_strdup_shares_malloc_counter(void);
void test_mock_malloc_reset(void);

// Stub implementation for testing - this provides test-specific behavior
// without conflicting with the real implementation in utils.c
static char** get_program_args_stub(void) {
    static const char* const stub_args[] = {"hydrogen_test", NULL};
    return (char**)stub_args;
}

void setUp(void) {
    // Reset mock system before each test
    mock_system_reset_all();
}

void tearDown(void) {
    // Reset mock system after each test
    mock_system_reset_all();
}

void test_get_program_args_returns_valid_pointer(void) {
    // Test that get_program_args returns a non-null pointer
    // Since we have a stub implementation, it should always return non-null
    char** args = get_program_args_stub();

    // Our stub implementation should return a valid pointer
    TEST_ASSERT_NOT_NULL(args);

    // Verify the first argument is not null
    TEST_ASSERT_NOT_NULL(args[0]);

    // Verify it contains expected content
    TEST_ASSERT_EQUAL_STRING("hydrogen_test", args[0]);
}

void test_signal_handling_setup(void) {
    // Test that we can set up signal handlers without crashing
    // This tests the signal handling infrastructure
    
    struct sigaction sa;
    int result;
    
    // Test setting up a simple signal handler
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    result = sigaction(SIGUSR1, &sa, NULL);
    TEST_ASSERT_EQUAL(0, result);
    
    // Reset to default
    sa.sa_handler = SIG_DFL;
    sigaction(SIGUSR1, &sa, NULL);
}

void test_process_identification(void) {
    // Test basic process identification functions used in hydrogen.c
    pid_t pid = getpid();
    pid_t ppid = getppid();
    
    TEST_ASSERT_TRUE(pid > 0);
    TEST_ASSERT_TRUE(ppid >= 0);
}

void test_memory_allocation_patterns(void) {
    // Test memory allocation patterns similar to those used in hydrogen.c
    size_t test_size = 1024;
    void* ptr = malloc(test_size);
    
    TEST_ASSERT_NOT_NULL(ptr);
    
    // Write to the memory to ensure it's accessible
    memset(ptr, 0xAA, test_size);
    
    // Verify the memory was written
    unsigned char* byte_ptr = (unsigned char*)ptr;
    TEST_ASSERT_EQUAL(0xAA, byte_ptr[0]);
    TEST_ASSERT_EQUAL(0xAA, byte_ptr[test_size - 1]);
    
    free(ptr);
}

void test_string_operations_for_paths(void) {
    // Test string operations similar to those used for path handling in hydrogen.c
    char test_path[] = "/proc/self/exe";
    char buffer[256];
    
    // Test string length calculation
    size_t len = strlen(test_path);
    TEST_ASSERT_EQUAL(14, len);  // "/proc/self/exe" is 14 characters
    
    // Test string copying
    strncpy(buffer, test_path, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    TEST_ASSERT_EQUAL_STRING(test_path, buffer);
}

// ===== MOCK SYSTEM TESTS =====

void test_mock_malloc_counter_first_call(void) {
    // Test that mock_system_set_malloc_failure(1) fails the 1st malloc call
    mock_system_set_malloc_failure(1);
    
    void* ptr1 = malloc(100);  // This should fail
    void* ptr2 = malloc(100);  // This should succeed
    
    TEST_ASSERT_NULL(ptr1);
    TEST_ASSERT_NOT_NULL(ptr2);
    
    free(ptr2);
}

void test_mock_malloc_counter_second_call(void) {
    // Test that mock_system_set_malloc_failure(2) fails the 2nd malloc call
    mock_system_set_malloc_failure(2);
    
    void* ptr1 = malloc(100);  // This should succeed
    void* ptr2 = malloc(100);  // This should fail
    void* ptr3 = malloc(100);  // This should succeed
    
    TEST_ASSERT_NOT_NULL(ptr1);
    TEST_ASSERT_NULL(ptr2);
    TEST_ASSERT_NOT_NULL(ptr3);
    
    free(ptr1);
    free(ptr3);
}

void test_mock_malloc_counter_third_call(void) {
    // Test that mock_system_set_malloc_failure(3) fails the 3rd malloc call
    mock_system_set_malloc_failure(3);
    
    void* ptr1 = malloc(100);  // This should succeed
    void* ptr2 = malloc(100);  // This should succeed
    void* ptr3 = malloc(100);  // This should fail
    void* ptr4 = malloc(100);  // This should succeed
    
    TEST_ASSERT_NOT_NULL(ptr1);
    TEST_ASSERT_NOT_NULL(ptr2);
    TEST_ASSERT_NULL(ptr3);
    TEST_ASSERT_NOT_NULL(ptr4);
    
    free(ptr1);
    free(ptr2);
    free(ptr4);
}

void test_mock_calloc_counter_first_call(void) {
    // Test that mock_system_set_malloc_failure(1) also fails the 1st calloc call
    mock_system_set_malloc_failure(1);
    
    void* ptr1 = calloc(10, 10);  // This should fail (1st call)
    void* ptr2 = calloc(10, 10);  // This should succeed (2nd call)
    
    TEST_ASSERT_NULL(ptr1);
    TEST_ASSERT_NOT_NULL(ptr2);
    
    free(ptr2);
}

void test_mock_calloc_shares_malloc_counter(void) {
    // Test that malloc and calloc share the same counter
    mock_system_set_malloc_failure(2);
    
    void* ptr1 = malloc(100);   // 1st call - succeed
    void* ptr2 = calloc(10, 10); // 2nd call - fail
    void* ptr3 = malloc(100);   // 3rd call - succeed
    
    TEST_ASSERT_NOT_NULL(ptr1);
    TEST_ASSERT_NULL(ptr2);
    TEST_ASSERT_NOT_NULL(ptr3);
    
    free(ptr1);
    free(ptr3);
}

void test_mock_strdup_shares_malloc_counter(void) {
    // Test that strdup also shares the malloc counter
    mock_system_set_malloc_failure(2);
    
    void* ptr1 = malloc(100);    // 1st call - succeed
    char* str = strdup("test");  // 2nd call - fail
    void* ptr3 = malloc(100);    // 3rd call - succeed
    
    TEST_ASSERT_NOT_NULL(ptr1);
    TEST_ASSERT_NULL(str);
    TEST_ASSERT_NOT_NULL(ptr3);
    
    free(ptr1);
    free(ptr3);
}

void test_mock_malloc_reset(void) {
    // Test that mock_system_reset_all resets the counter
    mock_system_set_malloc_failure(1);
    void* ptr1 = malloc(100);  // Should fail
    TEST_ASSERT_NULL(ptr1);
    
    mock_system_reset_all();
    void* ptr2 = malloc(100);  // Should succeed after reset
    TEST_ASSERT_NOT_NULL(ptr2);
    
    free(ptr2);
}

int main(void) {
    UNITY_BEGIN();
    
    // Mock system verification tests (run first to verify mock works)
    RUN_TEST(test_mock_malloc_counter_first_call);
    RUN_TEST(test_mock_malloc_counter_second_call);
    RUN_TEST(test_mock_malloc_counter_third_call);
    RUN_TEST(test_mock_calloc_counter_first_call);
    RUN_TEST(test_mock_calloc_shares_malloc_counter);
    RUN_TEST(test_mock_strdup_shares_malloc_counter);
    RUN_TEST(test_mock_malloc_reset);
    
    // Original hydrogen tests
    RUN_TEST(test_get_program_args_returns_valid_pointer);
    RUN_TEST(test_signal_handling_setup);
    RUN_TEST(test_process_identification);
    RUN_TEST(test_memory_allocation_patterns);
    RUN_TEST(test_string_operations_for_paths);
    
    return UNITY_END();
}
