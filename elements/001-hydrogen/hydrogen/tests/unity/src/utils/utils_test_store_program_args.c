/*
 * Unity Test File: store_program_args Function Tests
 * This file contains comprehensive unit tests for the store_program_args() function
 * from src/utils/utils.c
 *
 * Coverage Goals:
 * - Test storing valid program arguments
 * - Test storing with argc = 0
 * - Test overwriting previously stored arguments
 * - Verify arguments are correctly retrievable via get_program_args
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
    // Reset stored args before each test by storing NULL
    store_program_args(0, NULL);
}

void tearDown(void) {
    // Clean up after each test
    store_program_args(0, NULL);
}

// Function prototypes for test functions
void test_store_program_args_valid_args(void);
void test_store_program_args_zero_argc(void);
void test_store_program_args_overwrite_previous(void);
void test_store_program_args_single_arg(void);
void test_store_program_args_preserves_argv_content(void);
void test_store_program_args_null_argv(void);
void test_store_program_args_large_argc(void);

//=============================================================================
// Parameter Validation Tests
//=============================================================================

void test_store_program_args_valid_args(void) {
    // Set up test arguments
    char arg0[] = "hydrogen";
    char arg1[] = "--config=test.json";
    char arg2[] = "--port=8080";
    char* test_argv[] = {arg0, arg1, arg2, NULL};
    int test_argc = 3;

    store_program_args(test_argc, test_argv);

    // Verify args were stored by retrieving them
    char** retrieved = get_program_args();
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_PTR(arg0, retrieved[0]);
    TEST_ASSERT_EQUAL_PTR(arg1, retrieved[1]);
    TEST_ASSERT_EQUAL_PTR(arg2, retrieved[2]);
}

void test_store_program_args_zero_argc(void) {
    char* test_argv[] = {NULL};
    store_program_args(0, test_argv);

    char** retrieved = get_program_args();
    TEST_ASSERT_NOT_NULL(retrieved);
    // The argv pointer itself should be stored even with argc=0
    TEST_ASSERT_EQUAL_PTR(test_argv, retrieved);
}

void test_store_program_args_null_argv(void) {
    store_program_args(0, NULL);

    char** retrieved = get_program_args();
    TEST_ASSERT_NULL(retrieved);
}

void test_store_program_args_single_arg(void) {
    char arg0[] = "hydrogen";
    char* test_argv[] = {arg0, NULL};

    store_program_args(1, test_argv);

    char** retrieved = get_program_args();
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_PTR(arg0, retrieved[0]);
}

void test_store_program_args_large_argc(void) {
    char arg0[] = "hydrogen";
    char arg1[] = "--verbose";
    char arg2[] = "--config";
    char arg3[] = "conf.json";
    char arg4[] = "--debug";
    char arg5[] = "--port";
    char arg6[] = "8080";
    char* test_argv[] = {arg0, arg1, arg2, arg3, arg4, arg5, arg6, NULL};

    store_program_args(7, test_argv);

    char** retrieved = get_program_args();
    TEST_ASSERT_NOT_NULL(retrieved);
    for (int i = 0; i < 7; i++) {
        TEST_ASSERT_EQUAL_PTR(test_argv[i], retrieved[i]);
    }
}

//=============================================================================
// Overwrite Tests
//=============================================================================

void test_store_program_args_overwrite_previous(void) {
    // First, store one set of args
    char first_arg[] = "first_program";
    char* first_argv[] = {first_arg, NULL};
    store_program_args(1, first_argv);

    char** retrieved = get_program_args();
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_PTR(first_arg, retrieved[0]);

    // Now overwrite with a different set
    char second_arg[] = "second_program";
    char* second_argv[] = {second_arg, NULL};
    store_program_args(1, second_argv);

    retrieved = get_program_args();
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_PTR(second_arg, retrieved[0]);
    TEST_ASSERT_EQUAL_PTR(second_argv, retrieved);
}

//=============================================================================
// Content Preservation Tests
//=============================================================================

void test_store_program_args_preserves_argv_content(void) {
    char arg0[] = "hydrogen_server";
    char arg1[] = "--config=/etc/hydrogen.json";
    char arg2[] = "--workers=8";
    char* test_argv[] = {arg0, arg1, arg2, NULL};

    store_program_args(3, test_argv);

    char** retrieved = get_program_args();
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_STRING("hydrogen_server", retrieved[0]);
    TEST_ASSERT_EQUAL_STRING("--config=/etc/hydrogen.json", retrieved[1]);
    TEST_ASSERT_EQUAL_STRING("--workers=8", retrieved[2]);
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_store_program_args_valid_args);
    RUN_TEST(test_store_program_args_zero_argc);
    RUN_TEST(test_store_program_args_null_argv);
    RUN_TEST(test_store_program_args_single_arg);
    RUN_TEST(test_store_program_args_large_argc);

    // Overwrite tests
    RUN_TEST(test_store_program_args_overwrite_previous);

    // Content preservation tests
    RUN_TEST(test_store_program_args_preserves_argv_content);

    return UNITY_END();
}
