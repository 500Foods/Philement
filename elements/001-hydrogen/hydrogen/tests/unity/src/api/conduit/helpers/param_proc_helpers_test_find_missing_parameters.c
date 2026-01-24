/*
 * Unity Test File: Find Missing Parameters
 * This file contains unit tests for find_missing_parameters() function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/api/conduit/helpers/param_proc_helpers.h>

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

static void test_find_missing_parameters_basic(void) {
    const char* required[] = {"userId", "username", "email"};
    const char* provided[] = {"userId", "email"};
    
    size_t missing_count = 0;
    char** missing = find_missing_parameters((char**)required, 3, (char**)provided, 2, &missing_count);
    
    TEST_ASSERT_NOT_NULL(missing);
    TEST_ASSERT_EQUAL_INT(1, missing_count);
    TEST_ASSERT_EQUAL_STRING("username", missing[0]);
    
    // Cleanup
    for (size_t i = 0; i < missing_count; i++) {
        free(missing[i]);
    }
    free(missing);
}

static void test_find_missing_parameters_all_provided(void) {
    const char* required[] = {"userId", "username", "email"};
    const char* provided[] = {"userId", "username", "email"};
    
    size_t missing_count = 0;
    char** missing = find_missing_parameters((char**)required, 3, (char**)provided, 3, &missing_count);
    
    TEST_ASSERT_NULL(missing);
    TEST_ASSERT_EQUAL_INT(0, missing_count);
}

static void test_find_missing_parameters_none_provided(void) {
    const char* required[] = {"userId", "username", "email"};
    
    size_t missing_count = 0;
    char** missing = find_missing_parameters((char**)required, 3, NULL, 0, &missing_count);
    
    TEST_ASSERT_NOT_NULL(missing);
    TEST_ASSERT_EQUAL_INT(3, missing_count);
    TEST_ASSERT_EQUAL_STRING("userId", missing[0]);
    TEST_ASSERT_EQUAL_STRING("username", missing[1]);
    TEST_ASSERT_EQUAL_STRING("email", missing[2]);
    
    // Cleanup
    for (size_t i = 0; i < missing_count; i++) {
        free(missing[i]);
    }
    free(missing);
}

static void test_find_missing_parameters_none_required(void) {
    const char* provided[] = {"userId", "username", "email"};
    
    size_t missing_count = 0;
    char** missing = find_missing_parameters(NULL, 0, (char**)provided, 3, &missing_count);
    
    TEST_ASSERT_NULL(missing);
    TEST_ASSERT_EQUAL_INT(0, missing_count);
}

static void test_find_missing_parameters_no_overlap(void) {
    const char* required[] = {"userId", "username"};
    const char* provided[] = {"email", "password"};
    
    size_t missing_count = 0;
    char** missing = find_missing_parameters((char**)required, 2, (char**)provided, 2, &missing_count);
    
    TEST_ASSERT_NOT_NULL(missing);
    TEST_ASSERT_EQUAL_INT(2, missing_count);
    TEST_ASSERT_EQUAL_STRING("userId", missing[0]);
    TEST_ASSERT_EQUAL_STRING("username", missing[1]);
    
    // Cleanup
    for (size_t i = 0; i < missing_count; i++) {
        free(missing[i]);
    }
    free(missing);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_find_missing_parameters_basic);
    RUN_TEST(test_find_missing_parameters_all_provided);
    RUN_TEST(test_find_missing_parameters_none_provided);
    RUN_TEST(test_find_missing_parameters_none_required);
    RUN_TEST(test_find_missing_parameters_no_overlap);
    
    return UNITY_END();
}
