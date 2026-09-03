#include <src/hydrogen.h>
#include <unity.h>
#include <src/api/wschat/helpers/engine_cache.h>
#include <src/api/wschat/helpers/local_mcp.h>

void test_extract_openai(void);
void test_extract_anthropic(void);
void test_extract_responses(void);
void test_extract_none(void);
void test_append_openai_results(void);
void test_accumulate_stream_tool_calls(void);
void test_tool_result_text(void);

void setUp(void) {}
void tearDown(void) {}

void test_extract_openai(void) {
    const char *body =
        "{\"choices\":[{\"message\":{\"tool_calls\":[{"
        "\"id\":\"call_1\",\"type\":\"function\","
        "\"function\":{\"name\":\"System.Info\",\"arguments\":\"{}\"}}]}}]}";
    json_t *calls = chat_local_mcp_extract_tool_calls(body, CEC_PROVIDER_OPENAI);
    TEST_ASSERT_NOT_NULL(calls);
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(calls));
    TEST_ASSERT_EQUAL_STRING("System.Info",
        json_string_value(json_object_get(json_array_get(calls, 0), "name")));
    json_decref(calls);
}

void test_extract_anthropic(void) {
    const char *body =
        "{\"content\":[{\"type\":\"tool_use\",\"id\":\"tu_1\",\"name\":\"System.Info\",\"input\":{}}]}";
    json_t *calls = chat_local_mcp_extract_tool_calls(body, CEC_PROVIDER_ANTHROPIC);
    TEST_ASSERT_NOT_NULL(calls);
    TEST_ASSERT_EQUAL_STRING("tu_1",
        json_string_value(json_object_get(json_array_get(calls, 0), "id")));
    json_decref(calls);
}

void test_extract_responses(void) {
    const char *body =
        "{\"output\":[{\"type\":\"function_call\",\"call_id\":\"fc_1\",\"name\":\"System.Info\",\"arguments\":\"{}\"}]}";
    json_t *calls = chat_local_mcp_extract_tool_calls(body, CEC_PROVIDER_OPENAI);
    TEST_ASSERT_NOT_NULL(calls);
    TEST_ASSERT_EQUAL_STRING("fc_1",
        json_string_value(json_object_get(json_array_get(calls, 0), "id")));
    json_decref(calls);
}

void test_extract_none(void) {
    TEST_ASSERT_NULL(chat_local_mcp_extract_tool_calls(
        "{\"choices\":[{\"message\":{\"content\":\"hi\"}}]}", CEC_PROVIDER_OPENAI));
}

void test_append_openai_results(void) {
    json_t *calls = chat_local_mcp_extract_tool_calls(
        "{\"choices\":[{\"message\":{\"tool_calls\":[{"
        "\"id\":\"call_1\",\"function\":{\"name\":\"System.Info\",\"arguments\":\"{}\"}}]}}]}",
        CEC_PROVIDER_OPENAI);
    json_t *results = json_array();
    json_t *row = json_object();
    json_object_set_new(row, "id", json_string("call_1"));
    json_object_set_new(row, "name", json_string("System.Info"));
    json_object_set_new(row, "content", json_string("{\"ok\":true}"));
    json_array_append_new(results, row);
    char *next = chat_local_mcp_append_tool_results(
        "{\"model\":\"x\",\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}]}",
        calls, results, CEC_PROVIDER_OPENAI, false);
    TEST_ASSERT_NOT_NULL(next);
    json_t *root = json_loads(next, 0, NULL);
    TEST_ASSERT_EQUAL_UINT(3, json_array_size(json_object_get(root, "messages")));
    json_decref(root);
    json_decref(calls);
    json_decref(results);
    free(next);
}

void test_accumulate_stream_tool_calls(void) {
    json_t *acc = NULL;
    json_t *delta1 = json_loads(
        "[{\"index\":0,\"id\":\"call_1\",\"function\":{\"name\":\"System.Info\",\"arguments\":\"\"}}]", 0, NULL);
    json_t *delta2 = json_loads(
        "[{\"index\":0,\"function\":{\"arguments\":\"{\\\"a\\\":1}\"}}]", 0, NULL);
    chat_local_mcp_accumulate_stream_tool_calls(&acc, delta1);
    chat_local_mcp_accumulate_stream_tool_calls(&acc, delta2);
    json_t *final = chat_local_mcp_finalize_accumulated(acc);
    TEST_ASSERT_NOT_NULL(final);
    TEST_ASSERT_EQUAL_STRING("System.Info",
        json_string_value(json_object_get(json_array_get(final, 0), "name")));
    json_decref(delta1);
    json_decref(delta2);
    json_decref(acc);
    json_decref(final);
}

void test_tool_result_text(void) {
    json_t *result = json_loads(
        "{\"content\":[{\"type\":\"text\",\"text\":\"hello\"}],\"isError\":false}", 0, NULL);
    char *text = chat_local_mcp_tool_result_text(result);
    TEST_ASSERT_EQUAL_STRING("hello", text);
    free(text);
    json_decref(result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_extract_openai);
    RUN_TEST(test_extract_anthropic);
    RUN_TEST(test_extract_responses);
    RUN_TEST(test_extract_none);
    RUN_TEST(test_append_openai_results);
    RUN_TEST(test_accumulate_stream_tool_calls);
    RUN_TEST(test_tool_result_text);
    return UNITY_END();
}
