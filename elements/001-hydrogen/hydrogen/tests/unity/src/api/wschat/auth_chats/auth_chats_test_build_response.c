/*
 * Unity Test File: auth_chats_build_response
 * Tests success, provider failure and malformed provider response envelopes.
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/wschat/auth_chats/auth_chats.h>

void test_auth_chats_build_error_response_handles_message_and_null(void);
void test_auth_chats_build_response_handles_null_result(void);
void test_auth_chats_build_response_serializes_mixed_results(void);
void test_auth_chats_build_response_handles_parse_failure_and_unknown_engine(void);

void setUp(void) {
}

void tearDown(void) {
}

static ChatProxyResult *make_result(ChatProxyResultCode code, const char *body,
                                    const char *error, double elapsed) {
    ChatProxyResult *result = calloc(1, sizeof(ChatProxyResult));
    TEST_ASSERT_NOT_NULL(result);
    result->code = code;
    result->http_status = code == CHAT_PROXY_OK ? 200 : 502;
    result->response_body = body ? strdup(body) : NULL;
    result->error_message = error ? strdup(error) : NULL;
    result->total_time_ms = elapsed;
    return result;
}

void test_auth_chats_build_error_response_handles_message_and_null(void) {
    json_t *response = auth_chats_build_error_response("bad request");
    TEST_ASSERT_TRUE(json_is_false(json_object_get(response, "success")));
    TEST_ASSERT_EQUAL_STRING("bad request", json_string_value(json_object_get(response, "error")));
    json_decref(response);

    response = auth_chats_build_error_response(NULL);
    TEST_ASSERT_EQUAL_STRING("Unknown error", json_string_value(json_object_get(response, "error")));
    json_decref(response);
}

void test_auth_chats_build_response_handles_null_result(void) {
    json_t *response = auth_chats_build_response("db", "id", 2, NULL, NULL);
    TEST_ASSERT_EQUAL_INT(0, json_integer_value(json_object_get(response, "engines_succeeded")));
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(json_object_get(response, "results")));
    json_t *timing = json_object_get(response, "timing");
    TEST_ASSERT_EQUAL_DOUBLE(0.0, json_number_value(json_object_get(timing, "total_time_ms")));
    json_decref(response);
}

void test_auth_chats_build_response_serializes_mixed_results(void) {
    const char *body = "{\"choices\":[{\"message\":{\"content\":\"hello\"},"
        "\"finish_reason\":\"stop\"}],\"model\":\"model-a\","
        "\"usage\":{\"prompt_tokens\":2,\"completion_tokens\":3,\"total_tokens\":5}}";
    ChatMultiResult *multi = chat_multi_result_create(2);
    multi->results[0] = make_result(CHAT_PROXY_OK, body, NULL, 4.5);
    multi->results[1] = make_result(CHAT_PROXY_ERROR_UNKNOWN, NULL, "offline", 7.5);
    multi->success_count = 1;
    multi->failure_count = 1;
    multi->total_time_ms = 8.0;
    char *names[] = {(char *)"one", (char *)"two"};

    json_t *response = auth_chats_build_response("db", "id", 2, names, multi);
    json_t *results = json_object_get(response, "results");
    json_t *success = json_array_get(results, 0);
    json_t *failure = json_array_get(results, 1);
    TEST_ASSERT_EQUAL_STRING("hello", json_string_value(json_object_get(success, "content")));
    TEST_ASSERT_EQUAL_INT(5, json_integer_value(json_object_get(json_object_get(success, "tokens"), "total")));
    TEST_ASSERT_EQUAL_STRING("offline", json_string_value(json_object_get(failure, "error")));
    TEST_ASSERT_TRUE(json_is_false(json_object_get(failure, "success")));

    json_decref(response);
    chat_multi_result_destroy(multi);
}

void test_auth_chats_build_response_handles_parse_failure_and_unknown_engine(void) {
    ChatMultiResult *multi = chat_multi_result_create(2);
    multi->results[0] = make_result(CHAT_PROXY_OK, "not-json", NULL, 1.0);
    multi->results[1] = NULL;
    multi->success_count = 1;
    multi->failure_count = 1;

    json_t *response = auth_chats_build_response("db", "id", 2, NULL, multi);
    json_t *results = json_object_get(response, "results");
    TEST_ASSERT_EQUAL_STRING("Failed to parse response",
                             json_string_value(json_object_get(json_array_get(results, 0), "error")));
    TEST_ASSERT_EQUAL_STRING("unknown",
                             json_string_value(json_object_get(json_array_get(results, 1), "engine")));
    TEST_ASSERT_EQUAL_STRING("Request failed",
                             json_string_value(json_object_get(json_array_get(results, 1), "error")));

    json_decref(response);
    chat_multi_result_destroy(multi);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_auth_chats_build_error_response_handles_message_and_null);
    RUN_TEST(test_auth_chats_build_response_handles_null_result);
    RUN_TEST(test_auth_chats_build_response_serializes_mixed_results);
    RUN_TEST(test_auth_chats_build_response_handles_parse_failure_and_unknown_engine);
    return UNITY_END();
}
