/*
 * Unity Test File: auth_chat_build_error_response
 * This file contains unit tests for auth_chat_build_error_response() in
 * src/api/wschat/auth_chat/auth_chat.c
 *
 * auth_chat_build_error_response() builds the error JSON envelope used by all
 * of the handler's early-return paths.
 *
 * CHANGELOG:
 * 2026-07-18: Initial implementation covering error envelope fields
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/wschat/auth_chat/auth_chat.h>

// Test function prototypes
void test_auth_chat_build_error_response_basic(void);
void test_auth_chat_build_error_response_empty_message(void);

void setUp(void) {
    // No global state.
}

void tearDown(void) {
    // Each test decrefs its own response.
}

// Error envelope has success=false and the provided error string.
void test_auth_chat_build_error_response_basic(void) {
    json_t *response = auth_chat_build_error_response("Engine not found");

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    json_t *success = json_object_get(response, "success");
    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_FALSE(json_is_true(success));

    TEST_ASSERT_EQUAL_STRING("Engine not found",
                             json_string_value(json_object_get(response, "error")));

    json_decref(response);
}

// An empty error message is still carried through.
void test_auth_chat_build_error_response_empty_message(void) {
    json_t *response = auth_chat_build_error_response("");

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(response, "success")));
    TEST_ASSERT_EQUAL_STRING("", json_string_value(json_object_get(response, "error")));

    json_decref(response);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_auth_chat_build_error_response_basic);
    RUN_TEST(test_auth_chat_build_error_response_empty_message);

    return UNITY_END();
}
