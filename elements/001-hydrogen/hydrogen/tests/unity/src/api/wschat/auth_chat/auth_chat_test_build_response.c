/*
 * Unity Test File: auth_chat_build_response
 * This file contains unit tests for auth_chat_build_response() in
 * src/api/wschat/auth_chat/auth_chat.c
 *
 * auth_chat_build_response() builds the success JSON envelope from the
 * engine/model/content/token/convos fields. These tests verify the structure
 * and field values, including NULL-safe handling of optional content/model.
 *
 * CHANGELOG:
 * 2026-07-18: Initial implementation covering response envelope fields
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/wschat/auth_chat/auth_chat.h>

// Test function prototypes
void test_auth_chat_build_response_basic_fields(void);
void test_auth_chat_build_response_handles_null_content(void);
void test_auth_chat_build_response_handles_null_model(void);
void test_auth_chat_build_response_zero_tokens(void);

void setUp(void) {
    // No global state.
}

void tearDown(void) {
    // Each test decrefs its own response.
}

// Standard success envelope contains all expected keys with correct values.
void test_auth_chat_build_response_basic_fields(void) {
    json_t *response = auth_chat_build_response(
        "mydb", "gpt-4", "gpt-4-turbo", "Hello there!",
        10, 20, 30, "stop", 850.5, 7, "chat.7.1234567890");

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    json_t *success = json_object_get(response, "success");
    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_TRUE(json_is_true(success));

    TEST_ASSERT_EQUAL_STRING("mydb", json_string_value(json_object_get(response, "database")));
    TEST_ASSERT_EQUAL_STRING("gpt-4", json_string_value(json_object_get(response, "engine")));
    TEST_ASSERT_EQUAL_STRING("gpt-4-turbo", json_string_value(json_object_get(response, "model")));
    TEST_ASSERT_EQUAL_STRING("Hello there!", json_string_value(json_object_get(response, "content")));
    TEST_ASSERT_EQUAL_STRING("stop", json_string_value(json_object_get(response, "finish_reason")));
    TEST_ASSERT_EQUAL_STRING("chat.7.1234567890", json_string_value(json_object_get(response, "convos_ref")));

    TEST_ASSERT_EQUAL(7, json_integer_value(json_object_get(response, "convos_id")));
    TEST_ASSERT_EQUAL_DOUBLE(850.5, json_real_value(json_object_get(response, "response_time_ms")));

    json_t *tokens = json_object_get(response, "tokens");
    TEST_ASSERT_NOT_NULL(tokens);
    TEST_ASSERT_TRUE(json_is_object(tokens));
    TEST_ASSERT_EQUAL(10, json_integer_value(json_object_get(tokens, "prompt")));
    TEST_ASSERT_EQUAL(20, json_integer_value(json_object_get(tokens, "completion")));
    TEST_ASSERT_EQUAL(30, json_integer_value(json_object_get(tokens, "total")));

    json_decref(response);
}

// NULL content -> json_string(NULL) yields no "content" value; call survives.
void test_auth_chat_build_response_handles_null_content(void) {
    json_t *response = auth_chat_build_response(
        "mydb", "gpt-4", "gpt-4-turbo", NULL,
        0, 0, 0, "stop", 0.0, 1, "chat.1.0");

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_NULL(json_object_get(response, "content"));

    json_decref(response);
}

// NULL model -> json_string(NULL) yields no "model" value; engine still set.
void test_auth_chat_build_response_handles_null_model(void) {
    json_t *response = auth_chat_build_response(
        "mydb", "gpt-4", NULL, "content",
        1, 2, 3, "stop", 1.0, 2, "chat.2.0");

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_EQUAL_STRING("gpt-4", json_string_value(json_object_get(response, "engine")));
    TEST_ASSERT_NULL(json_object_get(response, "model"));

    json_decref(response);
}

// Zero token counts are preserved in the envelope.
void test_auth_chat_build_response_zero_tokens(void) {
    json_t *response = auth_chat_build_response(
        "mydb", "gpt-4", "gpt-4-turbo", "content",
        0, 0, 0, "stop", 0.0, 3, "chat.3.0");

    TEST_ASSERT_NOT_NULL(response);
    json_t *tokens = json_object_get(response, "tokens");
    TEST_ASSERT_EQUAL(0, json_integer_value(json_object_get(tokens, "prompt")));
    TEST_ASSERT_EQUAL(0, json_integer_value(json_object_get(tokens, "completion")));
    TEST_ASSERT_EQUAL(0, json_integer_value(json_object_get(tokens, "total")));

    json_decref(response);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_auth_chat_build_response_basic_fields);
    RUN_TEST(test_auth_chat_build_response_handles_null_content);
    RUN_TEST(test_auth_chat_build_response_handles_null_model);
    RUN_TEST(test_auth_chat_build_response_zero_tokens);

    return UNITY_END();
}
