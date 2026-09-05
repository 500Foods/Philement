#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/resp_parser.h>
#include <string.h>

void test_response_create_destroy(void);
void test_response_create_null(void);
void test_response_destroy_null(void);
void test_response_parse_openai_null(void);
void test_response_parse_openai_basic(void);
void test_response_parse_openai_with_error(void);
void test_response_parse_openai_with_tokens(void);
void test_response_parse_openai_no_content(void);
void test_response_parse_openai_invalid_json(void);
void test_response_parse_anthropic_basic(void);
void test_response_parse_anthropic_with_error(void);
void test_response_parse_ollama_basic(void);
void test_response_parse_ollama_with_error(void);
void test_response_parse_generic_openai(void);
void test_response_parse_generic_anthropic(void);
void test_response_parse_generic_ollama(void);
void test_response_parse_generic_unknown(void);
void test_extract_tokens_null(void);
void test_extract_tokens_openai(void);
void test_extract_tokens_anthropic(void);
void test_extract_content_null(void);
void test_extract_content_openai(void);
void test_extract_content_anthropic(void);
void test_extract_content_ollama(void);
void test_stream_chunk_parse_null(void);
void test_stream_chunk_parse_done(void);
void test_stream_chunk_parse_event_prefix(void);
void test_stream_chunk_parse_openai_delta(void);
void test_stream_chunk_parse_ollama(void);
void test_stream_chunk_parse_data_prefix(void);
void test_stream_chunk_parse_empty(void);
void test_stream_chunk_parse_responses_api(void);
void test_stream_chunk_destroy_null(void);

void setUp(void) {}
void tearDown(void) {}

void test_response_create_destroy(void) {
    ChatParsedResponse *r = chat_parsed_response_create();
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_FALSE(r->success);
    TEST_ASSERT_NULL(r->content);
    TEST_ASSERT_NULL(r->model);
    TEST_ASSERT_EQUAL_INT(0, r->prompt_tokens);
    chat_parsed_response_destroy(r);
}

void test_response_create_null(void) {
    ChatParsedResponse *r = chat_parsed_response_create();
    TEST_ASSERT_NOT_NULL(r);
    chat_parsed_response_destroy(r);
}

void test_response_destroy_null(void) {
    chat_parsed_response_destroy(NULL);
}

void test_response_parse_openai_null(void) {
    TEST_ASSERT_NULL(chat_response_parse_openai(NULL));
}

void test_response_parse_openai_basic(void) {
    const char *json = "{\"model\":\"gpt-4\",\"choices\":[{\"message\":{\"content\":\"Hello!\"},\"finish_reason\":\"stop\"}]}";
    ChatParsedResponse *r = chat_response_parse_openai(json);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_TRUE(r->success);
    TEST_ASSERT_EQUAL_STRING("gpt-4", r->model);
    TEST_ASSERT_EQUAL_STRING("Hello!", r->content);
    TEST_ASSERT_EQUAL_STRING("stop", r->finish_reason);
    chat_parsed_response_destroy(r);
}

void test_response_parse_openai_with_error(void) {
    const char *json = "{\"error\":{\"message\":\"Rate limit exceeded\"}}";
    ChatParsedResponse *r = chat_response_parse_openai(json);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_FALSE(r->success);
    TEST_ASSERT_EQUAL_STRING("Rate limit exceeded", r->error_message);
    chat_parsed_response_destroy(r);
}

void test_response_parse_openai_with_tokens(void) {
    const char *json = "{\"model\":\"gpt-4\",\"choices\":[{\"message\":{\"content\":\"Hi\"}}],\"usage\":{\"prompt_tokens\":10,\"completion_tokens\":5,\"total_tokens\":15}}";
    ChatParsedResponse *r = chat_response_parse_openai(json);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_INT(10, r->prompt_tokens);
    TEST_ASSERT_EQUAL_INT(5, r->completion_tokens);
    TEST_ASSERT_EQUAL_INT(15, r->total_tokens);
    chat_parsed_response_destroy(r);
}

void test_response_parse_openai_no_content(void) {
    const char *json = "{\"model\":\"gpt-4\",\"choices\":[{\"message\":{}}]}";
    ChatParsedResponse *r = chat_response_parse_openai(json);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_FALSE(r->success);
    TEST_ASSERT_NULL(r->content);
    chat_parsed_response_destroy(r);
}

void test_response_parse_openai_invalid_json(void) {
    const char *json = "not json";
    ChatParsedResponse *r = chat_response_parse_openai(json);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_FALSE(r->success);
    TEST_ASSERT_NOT_NULL(r->error_message);
    chat_parsed_response_destroy(r);
}

void test_response_parse_anthropic_basic(void) {
    const char *json = "{\"model\":\"claude-3\",\"content\":[{\"text\":\"Hello from Anthropic\"}],\"stop_reason\":\"end_turn\",\"usage\":{\"input_tokens\":10,\"output_tokens\":5}}";
    ChatParsedResponse *r = chat_response_parse_anthropic(json);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_TRUE(r->success);
    TEST_ASSERT_EQUAL_STRING("claude-3", r->model);
    TEST_ASSERT_EQUAL_STRING("Hello from Anthropic", r->content);
    TEST_ASSERT_EQUAL_STRING("end_turn", r->finish_reason);
    TEST_ASSERT_EQUAL_INT(10, r->prompt_tokens);
    TEST_ASSERT_EQUAL_INT(5, r->completion_tokens);
    chat_parsed_response_destroy(r);
}

void test_response_parse_anthropic_with_error(void) {
    const char *json = "{\"error\":{\"message\":\"Anthropic error\"}}";
    ChatParsedResponse *r = chat_response_parse_anthropic(json);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_FALSE(r->success);
    TEST_ASSERT_EQUAL_STRING("Anthropic error", r->error_message);
    chat_parsed_response_destroy(r);
}

void test_response_parse_ollama_basic(void) {
    const char *json = "{\"model\":\"llama2\",\"message\":{\"content\":\"Ollama response\"}}";
    ChatParsedResponse *r = chat_response_parse_ollama(json);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_TRUE(r->success);
    TEST_ASSERT_EQUAL_STRING("llama2", r->model);
    TEST_ASSERT_EQUAL_STRING("Ollama response", r->content);
    TEST_ASSERT_EQUAL_STRING("stop", r->finish_reason);
    chat_parsed_response_destroy(r);
}

void test_response_parse_ollama_with_error(void) {
    const char *json = "{\"error\":\"ollama error\"}";
    ChatParsedResponse *r = chat_response_parse_ollama(json);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_FALSE(r->success);
    TEST_ASSERT_EQUAL_STRING("ollama error", r->error_message);
    chat_parsed_response_destroy(r);
}

void test_response_parse_generic_openai(void) {
    const char *json = "{\"model\":\"gpt-4\",\"choices\":[{\"message\":{\"content\":\"Hi\"}}]}";
    ChatParsedResponse *r = chat_response_parse(json, CEC_PROVIDER_OPENAI);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_TRUE(r->success);
    chat_parsed_response_destroy(r);
}

void test_response_parse_generic_anthropic(void) {
    const char *json = "{\"content\":[{\"text\":\"Hi\"}],\"stop_reason\":\"end_turn\"}";
    ChatParsedResponse *r = chat_response_parse(json, CEC_PROVIDER_ANTHROPIC);
    TEST_ASSERT_NOT_NULL(r);
    chat_parsed_response_destroy(r);
}

void test_response_parse_generic_ollama(void) {
    const char *json = "{\"message\":{\"content\":\"Hi\"}}";
    ChatParsedResponse *r = chat_response_parse(json, CEC_PROVIDER_OLLAMA);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_TRUE(r->success);
    chat_parsed_response_destroy(r);
}

void test_response_parse_generic_unknown(void) {
    const char *json = "{\"choices\":[{\"message\":{\"content\":\"Hi\"}}]}";
    ChatParsedResponse *r = chat_response_parse(json, CEC_PROVIDER_UNKNOWN);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_TRUE(r->success);
    chat_parsed_response_destroy(r);
}

void test_extract_tokens_null(void) {
    int pt = -1, ct = -1, tt = -1;
    TEST_ASSERT_FALSE(chat_response_extract_tokens(NULL, &pt, &ct, &tt));
}

void test_extract_tokens_openai(void) {
    json_t *root = json_object();
    json_t *usage = json_object();
    json_object_set_new(usage, "prompt_tokens", json_integer(100));
    json_object_set_new(usage, "completion_tokens", json_integer(50));
    json_object_set_new(usage, "total_tokens", json_integer(150));
    json_object_set(root, "usage", usage);

    int pt = 0, ct = 0, tt = 0;
    TEST_ASSERT_TRUE(chat_response_extract_tokens(root, &pt, &ct, &tt));
    TEST_ASSERT_EQUAL_INT(100, pt);
    TEST_ASSERT_EQUAL_INT(50, ct);
    TEST_ASSERT_EQUAL_INT(150, tt);

    json_decref(root);
}

void test_extract_tokens_anthropic(void) {
    json_t *root = json_object();
    json_t *usage = json_object();
    json_object_set_new(usage, "input_tokens", json_integer(80));
    json_object_set_new(usage, "output_tokens", json_integer(20));
    json_object_set(root, "usage", usage);

    int pt = 0, ct = 0, tt = 0;
    TEST_ASSERT_TRUE(chat_response_extract_tokens(root, &pt, &ct, &tt));
    TEST_ASSERT_EQUAL_INT(80, pt);
    TEST_ASSERT_EQUAL_INT(20, ct);

    json_decref(root);
}

void test_extract_content_null(void) {
    TEST_ASSERT_NULL(chat_response_extract_content(NULL, CEC_PROVIDER_OPENAI));
}

void test_extract_content_openai(void) {
    json_t *root = json_object();
    json_t *choices = json_array();
    json_t *choice = json_object();
    json_t *message = json_object();
    json_object_set_new(message, "content", json_string("Hello"));
    json_object_set(choice, "message", message);
    json_array_append_new(choices, choice);
    json_object_set(root, "choices", choices);

    char *content = chat_response_extract_content(root, CEC_PROVIDER_OPENAI);
    TEST_ASSERT_EQUAL_STRING("Hello", content);
    free(content);
    json_decref(root);
}

void test_extract_content_anthropic(void) {
    json_t *root = json_object();
    json_t *content = json_array();
    json_t *item = json_object();
    json_object_set_new(item, "type", json_string("text"));
    json_object_set_new(item, "text", json_string("Hi from Claude"));
    json_array_append_new(content, item);
    json_object_set(root, "content", content);

    char *result = chat_response_extract_content(root, CEC_PROVIDER_ANTHROPIC);
    TEST_ASSERT_EQUAL_STRING("Hi from Claude", result);
    free(result);
    json_decref(root);
}

void test_extract_content_ollama(void) {
    json_t *root = json_object();
    json_t *msg = json_object();
    json_object_set_new(msg, "content", json_string("Ollama reply"));
    json_object_set(root, "message", msg);

    char *result = chat_response_extract_content(root, CEC_PROVIDER_OLLAMA);
    TEST_ASSERT_EQUAL_STRING("Ollama reply", result);
    free(result);
    json_decref(root);
}

void test_stream_chunk_parse_null(void) {
    TEST_ASSERT_NULL(chat_stream_chunk_parse(NULL));
}

void test_stream_chunk_parse_empty(void) {
    TEST_ASSERT_NULL(chat_stream_chunk_parse(""));
}

void test_stream_chunk_parse_done(void) {
    ChatStreamChunk *chunk = chat_stream_chunk_parse("[DONE]");
    TEST_ASSERT_NOT_NULL(chunk);
    TEST_ASSERT_TRUE(chunk->is_done);
    chat_stream_chunk_destroy(chunk);
}

void test_stream_chunk_parse_event_prefix(void) {
    ChatStreamChunk *chunk = chat_stream_chunk_parse("event: message\n");
    TEST_ASSERT_NULL(chunk);
}

void test_stream_chunk_parse_openai_delta(void) {
    const char *line = "data: {\"choices\":[{\"delta\":{\"content\":\"Hello\"},\"finish_reason\":null}]}";
    ChatStreamChunk *chunk = chat_stream_chunk_parse(line);
    TEST_ASSERT_NOT_NULL(chunk);
    TEST_ASSERT_EQUAL_STRING("Hello", chunk->content);
    TEST_ASSERT_NULL(chunk->finish_reason);
    chat_stream_chunk_destroy(chunk);
}

void test_stream_chunk_parse_ollama(void) {
    const char *line = "{\"model\":\"llama2\",\"response\":\"chunk1\",\"done\":false}";
    ChatStreamChunk *chunk = chat_stream_chunk_parse(line);
    TEST_ASSERT_NOT_NULL(chunk);
    TEST_ASSERT_EQUAL_STRING("chunk1", chunk->content);
    TEST_ASSERT_FALSE(chunk->is_done);
    chat_stream_chunk_destroy(chunk);
}

void test_stream_chunk_parse_data_prefix(void) {
    const char *line = "data: {\"model\":\"gpt-4\",\"choices\":[{\"delta\":{\"content\":\"Hi\"}}]}";
    ChatStreamChunk *chunk = chat_stream_chunk_parse(line);
    TEST_ASSERT_NOT_NULL(chunk);
    TEST_ASSERT_EQUAL_STRING("Hi", chunk->content);
    chat_stream_chunk_destroy(chunk);
}

void test_stream_chunk_parse_responses_api(void) {
    const char *line = "{\"type\":\"response.output_text.delta\",\"delta\":\"Hello\"}";
    ChatStreamChunk *chunk = chat_stream_chunk_parse(line);
    TEST_ASSERT_NOT_NULL(chunk);
    TEST_ASSERT_EQUAL_STRING("Hello", chunk->content);
    chat_stream_chunk_destroy(chunk);
}

void test_stream_chunk_destroy_null(void) {
    chat_stream_chunk_destroy(NULL);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_response_create_destroy);
    RUN_TEST(test_response_create_null);
    RUN_TEST(test_response_destroy_null);
    RUN_TEST(test_response_parse_openai_null);
    RUN_TEST(test_response_parse_openai_basic);
    RUN_TEST(test_response_parse_openai_with_error);
    RUN_TEST(test_response_parse_openai_with_tokens);
    RUN_TEST(test_response_parse_openai_no_content);
    RUN_TEST(test_response_parse_openai_invalid_json);
    RUN_TEST(test_response_parse_anthropic_basic);
    RUN_TEST(test_response_parse_anthropic_with_error);
    RUN_TEST(test_response_parse_ollama_basic);
    RUN_TEST(test_response_parse_ollama_with_error);
    RUN_TEST(test_response_parse_generic_openai);
    RUN_TEST(test_response_parse_generic_anthropic);
    RUN_TEST(test_response_parse_generic_ollama);
    RUN_TEST(test_response_parse_generic_unknown);
    RUN_TEST(test_extract_tokens_null);
    RUN_TEST(test_extract_tokens_openai);
    RUN_TEST(test_extract_tokens_anthropic);
    RUN_TEST(test_extract_content_null);
    RUN_TEST(test_extract_content_openai);
    RUN_TEST(test_extract_content_anthropic);
    RUN_TEST(test_extract_content_ollama);
    RUN_TEST(test_stream_chunk_parse_null);
    RUN_TEST(test_stream_chunk_parse_empty);
    RUN_TEST(test_stream_chunk_parse_done);
    RUN_TEST(test_stream_chunk_parse_event_prefix);
    RUN_TEST(test_stream_chunk_parse_openai_delta);
    RUN_TEST(test_stream_chunk_parse_ollama);
    RUN_TEST(test_stream_chunk_parse_data_prefix);
    RUN_TEST(test_stream_chunk_parse_responses_api);
    RUN_TEST(test_stream_chunk_destroy_null);
    return UNITY_END();
}
