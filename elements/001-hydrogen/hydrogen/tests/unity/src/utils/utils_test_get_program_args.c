/*
 * Unity Test File: get_program_args Function Tests
 * This file contains comprehensive unit tests for the get_program_args() function
 * from src/utils/utils.c
 *
 * Coverage Goals:
 * - Test that get_program_args returns NULL before store_program_args is called
 * - Test that get_program_args returns the stored argv after store_program_args is called
 * - Test that the returned pointer matches the stored argv
 * - Test NULL argv retrieval after storing NULL
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Forward declarations for the functions being tested
void store_program_args(int argc, char* argv[]);
char** get_program_args(void);

// Unity framework requires these functions to be externally visible
extern void setUp(void);
extern void tearDown(void);

void setUp(void) {
    // Reset stored args before each test
    store_program_args(0, NULL);
}

void tearDown(void) {
    // Clean up after each test
    store_program_args(0, NULL);
}

// Function prototypes for test functions
void test_get_program_args_returns_null_initially(void);
void test_get_program_args_returns_null_after_null_store(void);
void test_get_program_args_returns_stored_argv(void);
void test_get_program_args_returns_correct_pointer(void);
void test_get_program_args_returns_argv_after_overwrite(void);
void test_get_program_args_content_integrity(void);

//=============================================================================
// Initial State Tests
//=============================================================================

void test_get_program_args_returns_null_initially(void) {
    // Before any store_program_args call, get_program_args should return NULL
    // (static variables are zero-initialized)
    char** result = get_program_args();
    TEST_ASSERT_NULL(result);
}

void test_get_program_args_returns_null_after_null_store(void) {
    // Store NULL argv
    store_program_args(0, NULL);

    char** result = get_program_args();
    TEST_ASSERT_NULL(result);
}

//=============================================================================
// Stored Value Retrieval Tests
//=============================================================================

void test_get_program_args_returns_stored_argv(void) {
    char arg0[] = "hydrogen";
    char arg1[] = "--config=test.json";
    char* test_argv[] = {arg0, arg1, NULL};

    store_program_args(2, test_argv);

    char** retrieved = get_program_args();
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_PTR(arg0, retrieved[0]);
    TEST_ASSERT_EQUAL_STRING("hydrogen", retrieved[0]);
    TEST_ASSERT_EQUAL_STRING("--config=test.json", retrieved[1]);
}

void test_get_program_args_returns_correct_pointer(void) {
    char arg0[] = "test_program";
    char* test_argv[] = {arg0, NULL};

    store_program_args(1, test_argv);

    char** retrieved = get_program_args();
    TEST_ASSERT_NOT_NULL(retrieved);
    // The returned pointer should be the exact same pointer we stored
    TEST_ASSERT_EQUAL_PTR(test_argv, retrieved);
}

void test_get_program_args_returns_argv_after_overwrite(void) {
    // Store first set of args
    char first_arg[] = "first";
    char* first_argv[] = {first_arg, NULL};
    store_program_args(1, first_argv);

    // Verify first set
    char** retrieved = get_program_args();
    TEST_ASSERT_EQUAL_PTR(first_argv, retrieved);

    // Store second set of args (overwrite)
    char second_arg[] = "second";
    char* second_argv[] = {second_arg, NULL};
    store_program_args(1, second_argv);

    // Verify second set is now returned
    retrieved = get_program_args();
    TEST_ASSERT_EQUAL_PTR(second_argv, retrieved);
    TEST_ASSERT_EQUAL_PTR(second_arg, retrieved[0]);
}

void test_get_program_args_content_integrity(void) {
    char arg0[] = "hydrogen_server";
    char arg1[] = "--port=8080";
    char arg2[] = "--verbose";
    char* test_argv[] = {arg0, arg1, arg2, NULL};

    store_program_args(3, test_argv);

    char** retrieved = get_program_args();
    TEST_ASSERT_NOT_NULL(retrieved);

    // Verify content integrity
    TEST_ASSERT_EQUAL_STRING("hydrogen_server", retrieved[0]);
    TEST_ASSERT_EQUAL_STRING("--port=8080", retrieved[1]);
    TEST_ASSERT_EQUAL_STRING("--verbose", retrieved[2]);
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();

    // Initial state tests
    RUN_TEST(test_get_program_args_returns_null_initially);
    RUN_TEST(test_get_program_args_returns_null_after_null_store);

    // Stored value retrieval tests
    RUN_TEST(test_get_program_args_returns_stored_argv);
    RUN_TEST(test_get_program_args_returns_correct_pointer);
    RUN_TEST(test_get_program_args_returns_argv_after_overwrite);
    RUN_TEST(test_get_program_args_content_integrity);

    return UNITY_END();
}
