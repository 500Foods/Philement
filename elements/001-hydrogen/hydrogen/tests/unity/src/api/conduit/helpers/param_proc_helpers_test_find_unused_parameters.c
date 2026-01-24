/*
 * Unity Test File: Find Unused Parameters
 * This file contains unit tests for find_unused_parameters() function
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

static void test_find_unused_parameters_basic(void) {
    const char* required[] = {"userId", "email"};
    const char* provided[] = {"userId", "email", "username"};
    
    size_t unused_count = 0;
    char** unused = find_unused_parameters((char**)required, 2, (char**)provided, 3, &unused_count);
    
    TEST_ASSERT_NOT_NULL(unused);
    TEST_ASSERT_EQUAL_INT(1, unused_count);
    TEST_ASSERT_EQUAL_STRING("username", unused[0]);
    
    // Cleanup
    for (size_t i = 0; i < unused_count; i++) {
        free(unused[i]);
    }
    free(unused);
}

static void test_find_unused_parameters_all_used(void) {
    const char* required[] = {"userId", "username", "email"};
    const char* provided[] = {"userId", "username", "email"};
    
    size_t unused_count = 0;
    char** unused = find_unused_parameters((char**)required, 3, (char**)provided, 3, &unused_count);
    
    TEST_ASSERT_NULL(unused);
    TEST_ASSERT_EQUAL_INT(0, unused_count);
}

static void test_find_unused_parameters_no_provided(void) {
    const char* required[] = {"userId", "username", "email"};
    
    size_t unused_count = 0;
    char** unused = find_unused_parameters((char**)required, 3, NULL, 0, &unused_count);
    
    TEST_ASSERT_NULL(unused);
    TEST_ASSERT_EQUAL_INT(0, unused_count);
}

static void test_find_unused_parameters_no_required(void) {
    const char* provided[] = {"userId", "username", "email"};
    
    size_t unused_count = 0;
    char** unused = find_unused_parameters(NULL, 0, (char**)provided, 3, &unused_count);
    
    TEST_ASSERT_NOT_NULL(unused);
    TEST_ASSERT_EQUAL_INT(3, unused_count);
    TEST_ASSERT_EQUAL_STRING("userId", unused[0]);
    TEST_ASSERT_EQUAL_STRING("username", unused[1]);
    TEST_ASSERT_EQUAL_STRING("email", unused[2]);
    
    // Cleanup
    for (size_t i = 0; i < unused_count; i++) {
        free(unused[i]);
    }
    free(unused);
}

static void test_find_unused_parameters_no_overlap(void) {
    const char* required[] = {"userId", "username"};
    const char* provided[] = {"email", "password"};
    
    size_t unused_count = 0;
    char** unused = find_unused_parameters((char**)required, 2, (char**)provided, 2, &unused_count);
    
    TEST_ASSERT_NOT_NULL(unused);
    TEST_ASSERT_EQUAL_INT(2, unused_count);
    TEST_ASSERT_EQUAL_STRING("email", unused[0]);
    TEST_ASSERT_EQUAL_STRING("password", unused[1]);
    
    // Cleanup
    for (size_t i = 0; i < unused_count; i++) {
        free(unused[i]);
    }
    free(unused);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_find_unused_parameters_basic);
    RUN_TEST(test_find_unused_parameters_all_used);
    RUN_TEST(test_find_unused_parameters_no_provided);
    RUN_TEST(test_find_unused_parameters_no_required);
    RUN_TEST(test_find_unused_parameters_no_overlap);
    
    return UNITY_END();
}
