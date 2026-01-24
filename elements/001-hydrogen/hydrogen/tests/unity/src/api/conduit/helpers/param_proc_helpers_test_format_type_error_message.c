/*
 * Unity Test File: Format Type Error Message
 * This file contains unit tests for format_type_error_message() function
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

static void test_format_type_error_message_basic(void) {
    char* msg = format_type_error_message("userId", "string", "INTEGER", "is not");
    
    TEST_ASSERT_NOT_NULL(msg);
    printf("Actual message: '%s'\n", msg); // Print actual message
    
    // Check if it contains the essential parts
    TEST_ASSERT_TRUE(strstr(msg, "userId") != NULL);
    TEST_ASSERT_TRUE(strstr(msg, "string") != NULL);
    TEST_ASSERT_TRUE(strstr(msg, "is not") != NULL);
    TEST_ASSERT_TRUE(strstr(msg, "INTEGER") != NULL);
    
    free(msg);
}

static void test_format_type_error_message_different_verb(void) {
    char* msg = format_type_error_message("email", "integer", "STRING", "should be");
    
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_TRUE(strstr(msg, "email(integer) should be email(STRING") != NULL);
    
    free(msg);
}

static void test_format_type_error_message_special_chars(void) {
    char* msg = format_type_error_message("user_name", "boolean", "FLOAT", "is not");
    
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_TRUE(strstr(msg, "user_name(boolean) is not user_name(FLOAT") != NULL);
    
    free(msg);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_format_type_error_message_basic);
    RUN_TEST(test_format_type_error_message_different_verb);
    RUN_TEST(test_format_type_error_message_special_chars);
    
    return UNITY_END();
}
