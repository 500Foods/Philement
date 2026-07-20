/*
 * Unity Test File: chat response parser
 * This file contains unit tests for src/api/wschat/helpers/resp_parser.c
 *
 * Coverage focus: content/token extraction across providers, the OpenAI /
 * Anthropic / Ollama parsers (success, error, and JSON-parse-failure paths),
 * the generic dispatcher, stream-chunk parsing (OpenAI/Anthropic/Ollama/DONE/
 * event/reasoning/extra-fields), and standalone error extraction.
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <stdlib.h>
#include <string.h>

#include <src/api/wschat/helpers/resp_parser.h>
#include <src/api/wschat/helpers/engine_cache.h>

// Forward declarations for tests
void test_create_and_destroy(void);
void test_extract_content_null(void);
void test_extract_content_openai_message(void);
void test_extract_content_openai_delta(void);
void test_extract_content_anthropic(void);
void test_extract_content_ollama_message(void);
void test_extract_content_ollama_direct_response(void);
void test_extract_tokens_null(void);
void test_extract_tokens_openai(void);
void test_extract_tokens_anthropic(void);
void test_parse_openai_success(void);
void test_parse_openai_error_with_message(void);
void test_parse_openai_error_without_message(void);
void test_parse_openai_bad_json(void);
void test_parse_openai_null(void);
void test_parse_anthropic_success(void);
void test_parse_anthropic_error(void);
void test_parse_anthropic_bad_json(void);
void test_parse_ollama_success(void);
void test_parse_ollama_error(void);
void test_parse_ollama_bad_json(void);
void test_parse_dispatch(void);
void test_stream_chunk_null_and_empty(void);
void test_stream_chunk_done(void);
void test_stream_chunk_event_line(void);
void test_stream_chunk_bad_json(void);
void test_stream_chunk_anthropic_delta(void);
void test_stream_chunk_anthropic_stop(void);
void test_stream_chunk_ollama(void);
void test_stream_chunk_openai_delta_reasoning(void);
void test_stream_chunk_openai_extra_fields(void);
void test_extract_error(void);

void setUp(void) {
}

void tearDown(void) {
}

/* ---- create/destroy ---- */
void test_create_and_destroy(void) {
    ChatParsedResponse* r = chat_parsed_response_create();
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_FALSE(r->success);
    TEST_ASSERT_NULL(r->content);
    chat_parsed_response_destroy(r);
    // destroy(NULL) is a safe no-op
    chat_parsed_response_destroy(NULL);
}

/* ---- content extraction ---- */
void test_extract_content_null(void) {
    TEST_ASSERT_NULL(chat_response_extract_content(NULL, CEC_PROVIDER_OPENAI));
}

void test_extract_content_openai_message(void) {
    json_t* root = json_loads(
        "{\"choices\":[{\"message\":{\"content\":\"hello\"}}]}", 0, NULL);
    char* c = chat_response_extract_content(root, CEC_PROVIDER_OPENAI);
    TEST_ASSERT_EQUAL_STRING("hello", c);
    free(c);
    json_decref(root);
}

void test_extract_content_openai_delta(void) {
    // No message.content, but delta.content is present (streaming)
    json_t* root = json_loads(
        "{\"choices\":[{\"delta\":{\"content\":\"chunk\"}}]}", 0, NULL);
    char* c = chat_response_extract_content(root, CEC_PROVIDER_OPENAI);
    TEST_ASSERT_EQUAL_STRING("chunk", c);
    free(c);
    json_decref(root);
}

void test_extract_content_anthropic(void) {
    json_t* root = json_loads(
        "{\"content\":[{\"text\":\"anthropic reply\"}]}", 0, NULL);
    char* c = chat_response_extract_content(root, CEC_PROVIDER_ANTHROPIC);
    TEST_ASSERT_EQUAL_STRING("anthropic reply", c);
    free(c);
    json_decref(root);
}

void test_extract_content_ollama_message(void) {
    json_t* root = json_loads(
        "{\"message\":{\"content\":\"ollama msg\"}}", 0, NULL);
    char* c = chat_response_extract_content(root, CEC_PROVIDER_OLLAMA);
    TEST_ASSERT_EQUAL_STRING("ollama msg", c);
    free(c);
    json_decref(root);
}

void test_extract_content_ollama_direct_response(void) {
    // No message.content, falls back to top-level "response"
    json_t* root = json_loads("{\"response\":\"direct\"}", 0, NULL);
    char* c = chat_response_extract_content(root, CEC_PROVIDER_OLLAMA);
    TEST_ASSERT_EQUAL_STRING("direct", c);
    free(c);
    json_decref(root);
}

/* ---- token extraction ---- */
void test_extract_tokens_null(void) {
    int p = 0, c = 0, t = 0;
    TEST_ASSERT_FALSE(chat_response_extract_tokens(NULL, &p, &c, &t));
}

void test_extract_tokens_openai(void) {
    json_t* root = json_loads(
        "{\"usage\":{\"prompt_tokens\":10,\"completion_tokens\":20,\"total_tokens\":30}}",
        0, NULL);
    int p = 0, c = 0, t = 0;
    TEST_ASSERT_TRUE(chat_response_extract_tokens(root, &p, &c, &t));
    TEST_ASSERT_EQUAL_INT(10, p);
    TEST_ASSERT_EQUAL_INT(20, c);
    TEST_ASSERT_EQUAL_INT(30, t);
    json_decref(root);
}

void test_extract_tokens_anthropic(void) {
    json_t* root = json_loads(
        "{\"usage\":{\"input_tokens\":5,\"output_tokens\":7}}", 0, NULL);
    int p = 0, c = 0, t = 0;
    TEST_ASSERT_TRUE(chat_response_extract_tokens(root, &p, &c, &t));
    TEST_ASSERT_EQUAL_INT(5, p);
    TEST_ASSERT_EQUAL_INT(7, c);
    json_decref(root);
}

/* ---- OpenAI parser ---- */
void test_parse_openai_success(void) {
    const char* json =
        "{\"model\":\"gpt-4\",\"choices\":[{\"message\":{\"content\":\"hi\"},"
        "\"finish_reason\":\"stop\"}],"
        "\"usage\":{\"prompt_tokens\":1,\"completion_tokens\":2,\"total_tokens\":3}}";
    ChatParsedResponse* r = chat_response_parse_openai(json);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_TRUE(r->success);
    TEST_ASSERT_EQUAL_STRING("hi", r->content);
    TEST_ASSERT_EQUAL_STRING("gpt-4", r->model);
    TEST_ASSERT_EQUAL_STRING("stop", r->finish_reason);
    TEST_ASSERT_EQUAL_INT(3, r->total_tokens);
    chat_parsed_response_destroy(r);
}

void test_parse_openai_error_with_message(void) {
    const char* json = "{\"error\":{\"message\":\"rate limited\"}}";
    ChatParsedResponse* r = chat_response_parse_openai(json);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_FALSE(r->success);
    TEST_ASSERT_EQUAL_STRING("rate limited", r->error_message);
    chat_parsed_response_destroy(r);
}

void test_parse_openai_error_without_message(void) {
    // error object present but no message field -> generic error text
    const char* json = "{\"error\":{\"code\":500}}";
    ChatParsedResponse* r = chat_response_parse_openai(json);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_FALSE(r->success);
    TEST_ASSERT_EQUAL_STRING("API returned error", r->error_message);
    chat_parsed_response_destroy(r);
}

void test_parse_openai_bad_json(void) {
    ChatParsedResponse* r = chat_response_parse_openai("not json {");
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_FALSE(r->success);
    TEST_ASSERT_EQUAL_STRING("Failed to parse JSON response", r->error_message);
    chat_parsed_response_destroy(r);
}

void test_parse_openai_null(void) {
    TEST_ASSERT_NULL(chat_response_parse_openai(NULL));
}

/* ---- Anthropic parser ---- */
void test_parse_anthropic_success(void) {
    const char* json =
        "{\"model\":\"claude\",\"content\":[{\"text\":\"yo\"}],"
        "\"stop_reason\":\"end_turn\","
        "\"usage\":{\"input_tokens\":4,\"output_tokens\":6}}";
    ChatParsedResponse* r = chat_response_parse_anthropic(json);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_TRUE(r->success);
    TEST_ASSERT_EQUAL_STRING("yo", r->content);
    TEST_ASSERT_EQUAL_STRING("claude", r->model);
    TEST_ASSERT_EQUAL_STRING("end_turn", r->finish_reason);
    TEST_ASSERT_EQUAL_INT(4, r->prompt_tokens);
    chat_parsed_response_destroy(r);
}

void test_parse_anthropic_error(void) {
    const char* json = "{\"error\":{\"message\":\"overloaded\"}}";
    ChatParsedResponse* r = chat_response_parse_anthropic(json);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_FALSE(r->success);
    TEST_ASSERT_EQUAL_STRING("overloaded", r->error_message);
    chat_parsed_response_destroy(r);

    // error present but no message -> generic
    ChatParsedResponse* r2 = chat_response_parse_anthropic("{\"error\":{}}");
    TEST_ASSERT_EQUAL_STRING("API returned error", r2->error_message);
    chat_parsed_response_destroy(r2);
}

void test_parse_anthropic_bad_json(void) {
    ChatParsedResponse* r = chat_response_parse_anthropic("}{");
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_STRING("Failed to parse JSON response", r->error_message);
    chat_parsed_response_destroy(r);
    TEST_ASSERT_NULL(chat_response_parse_anthropic(NULL));
}

/* ---- Ollama parser ---- */
void test_parse_ollama_success(void) {
    const char* json =
        "{\"model\":\"llama3\",\"message\":{\"content\":\"ollama out\"}}";
    ChatParsedResponse* r = chat_response_parse_ollama(json);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_TRUE(r->success);
    TEST_ASSERT_EQUAL_STRING("ollama out", r->content);
    TEST_ASSERT_EQUAL_STRING("llama3", r->model);
    TEST_ASSERT_EQUAL_STRING("stop", r->finish_reason);
    chat_parsed_response_destroy(r);
}

void test_parse_ollama_error(void) {
    // Ollama error is a top-level string
    const char* json = "{\"error\":\"model not found\"}";
    ChatParsedResponse* r = chat_response_parse_ollama(json);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_FALSE(r->success);
    TEST_ASSERT_EQUAL_STRING("model not found", r->error_message);
    chat_parsed_response_destroy(r);
}

void test_parse_ollama_bad_json(void) {
    ChatParsedResponse* r = chat_response_parse_ollama("nope");
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_STRING("Failed to parse JSON response", r->error_message);
    chat_parsed_response_destroy(r);
    TEST_ASSERT_NULL(chat_response_parse_ollama(NULL));
}

/* ---- generic dispatch ---- */
void test_parse_dispatch(void) {
    ChatParsedResponse* a = chat_response_parse(
        "{\"content\":[{\"text\":\"x\"}]}", CEC_PROVIDER_ANTHROPIC);
    TEST_ASSERT_TRUE(a->success);
    chat_parsed_response_destroy(a);

    ChatParsedResponse* o = chat_response_parse(
        "{\"message\":{\"content\":\"y\"}}", CEC_PROVIDER_OLLAMA);
    TEST_ASSERT_TRUE(o->success);
    chat_parsed_response_destroy(o);

    ChatParsedResponse* g = chat_response_parse(
        "{\"choices\":[{\"message\":{\"content\":\"z\"}}]}", CEC_PROVIDER_OPENAI);
    TEST_ASSERT_TRUE(g->success);
    chat_parsed_response_destroy(g);

    // Unknown provider falls through to OpenAI parser
    ChatParsedResponse* u = chat_response_parse(
        "{\"choices\":[{\"message\":{\"content\":\"w\"}}]}", CEC_PROVIDER_UNKNOWN);
    TEST_ASSERT_TRUE(u->success);
    chat_parsed_response_destroy(u);
}

/* ---- stream chunk ---- */
void test_stream_chunk_null_and_empty(void) {
    TEST_ASSERT_NULL(chat_stream_chunk_parse(NULL));
    TEST_ASSERT_NULL(chat_stream_chunk_parse(""));
    // destroy(NULL) safe
    chat_stream_chunk_destroy(NULL);
}

void test_stream_chunk_done(void) {
    // "[DONE]" possibly prefixed with "data: "
    ChatStreamChunk* c = chat_stream_chunk_parse("data: [DONE]");
    TEST_ASSERT_NOT_NULL(c);
    TEST_ASSERT_TRUE(c->is_done);
    chat_stream_chunk_destroy(c);

    ChatStreamChunk* c2 = chat_stream_chunk_parse("[DONE]");
    TEST_ASSERT_NOT_NULL(c2);
    TEST_ASSERT_TRUE(c2->is_done);
    chat_stream_chunk_destroy(c2);
}

void test_stream_chunk_event_line(void) {
    // "event: " lines are skipped (Anthropic SSE)
    TEST_ASSERT_NULL(chat_stream_chunk_parse("event: message_start"));
}

void test_stream_chunk_bad_json(void) {
    TEST_ASSERT_NULL(chat_stream_chunk_parse("data: not-json"));
}

void test_stream_chunk_anthropic_delta(void) {
    const char* line =
        "data: {\"type\":\"content_block_delta\",\"delta\":{\"text\":\"hi\"}}";
    ChatStreamChunk* c = chat_stream_chunk_parse(line);
    TEST_ASSERT_NOT_NULL(c);
    TEST_ASSERT_EQUAL_STRING("hi", c->content);
    TEST_ASSERT_FALSE(c->is_done);
    chat_stream_chunk_destroy(c);
}

void test_stream_chunk_anthropic_stop(void) {
    ChatStreamChunk* c = chat_stream_chunk_parse(
        "{\"type\":\"message_stop\"}");
    TEST_ASSERT_NOT_NULL(c);
    TEST_ASSERT_TRUE(c->is_done);
    chat_stream_chunk_destroy(c);
}

void test_stream_chunk_ollama(void) {
    const char* line =
        "{\"response\":\"tok\",\"done\":true,\"done_reason\":\"stop\"}";
    ChatStreamChunk* c = chat_stream_chunk_parse(line);
    TEST_ASSERT_NOT_NULL(c);
    TEST_ASSERT_EQUAL_STRING("tok", c->content);
    TEST_ASSERT_TRUE(c->is_done);
    TEST_ASSERT_EQUAL_STRING("stop", c->finish_reason);
    chat_stream_chunk_destroy(c);

    // done=false path
    ChatStreamChunk* c2 = chat_stream_chunk_parse(
        "{\"response\":\"tok2\",\"done\":false}");
    TEST_ASSERT_NOT_NULL(c2);
    TEST_ASSERT_EQUAL_STRING("tok2", c2->content);
    TEST_ASSERT_FALSE(c2->is_done);
    chat_stream_chunk_destroy(c2);
}

void test_stream_chunk_openai_delta_reasoning(void) {
    const char* line =
        "data: {\"id\":\"abc\",\"model\":\"gpt-4\",\"choices\":[{"
        "\"delta\":{\"content\":\"part\",\"reasoning_content\":\"think\"},"
        "\"finish_reason\":\"stop\"}]}";
    ChatStreamChunk* c = chat_stream_chunk_parse(line);
    TEST_ASSERT_NOT_NULL(c);
    TEST_ASSERT_EQUAL_STRING("abc", c->id);
    TEST_ASSERT_EQUAL_STRING("gpt-4", c->model);
    TEST_ASSERT_EQUAL_STRING("part", c->content);
    TEST_ASSERT_EQUAL_STRING("think", c->reasoning_content);
    TEST_ASSERT_EQUAL_STRING("stop", c->finish_reason);
    chat_stream_chunk_destroy(c);
}

void test_stream_chunk_openai_extra_fields(void) {
    const char* line =
        "{\"choices\":[{\"delta\":{\"content\":\"c\"}}],"
        "\"retrieval\":{\"docs\":1},\"citations\":[\"src\"]}";
    ChatStreamChunk* c = chat_stream_chunk_parse(line);
    TEST_ASSERT_NOT_NULL(c);
    TEST_ASSERT_NOT_NULL(c->extra_fields);
    TEST_ASSERT_NOT_NULL(json_object_get(c->extra_fields, "retrieval"));
    TEST_ASSERT_NOT_NULL(json_object_get(c->extra_fields, "citations"));
    chat_stream_chunk_destroy(c);
}

/* ---- error extraction ---- */
void test_extract_error(void) {
    TEST_ASSERT_NULL(chat_response_extract_error(NULL));
    TEST_ASSERT_NULL(chat_response_extract_error("bad json"));
    // no error field -> NULL
    TEST_ASSERT_NULL(chat_response_extract_error("{\"ok\":true}"));

    char* m = chat_response_extract_error("{\"error\":{\"message\":\"boom\"}}");
    TEST_ASSERT_EQUAL_STRING("boom", m);
    free(m);

    // error is a plain string
    char* s = chat_response_extract_error("{\"error\":\"stringy\"}");
    TEST_ASSERT_EQUAL_STRING("stringy", s);
    free(s);

    // error object without message and not a string -> NULL result
    TEST_ASSERT_NULL(chat_response_extract_error("{\"error\":{\"code\":1}}"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_create_and_destroy);
    RUN_TEST(test_extract_content_null);
    RUN_TEST(test_extract_content_openai_message);
    RUN_TEST(test_extract_content_openai_delta);
    RUN_TEST(test_extract_content_anthropic);
    RUN_TEST(test_extract_content_ollama_message);
    RUN_TEST(test_extract_content_ollama_direct_response);
    RUN_TEST(test_extract_tokens_null);
    RUN_TEST(test_extract_tokens_openai);
    RUN_TEST(test_extract_tokens_anthropic);
    RUN_TEST(test_parse_openai_success);
    RUN_TEST(test_parse_openai_error_with_message);
    RUN_TEST(test_parse_openai_error_without_message);
    RUN_TEST(test_parse_openai_bad_json);
    RUN_TEST(test_parse_openai_null);
    RUN_TEST(test_parse_anthropic_success);
    RUN_TEST(test_parse_anthropic_error);
    RUN_TEST(test_parse_anthropic_bad_json);
    RUN_TEST(test_parse_ollama_success);
    RUN_TEST(test_parse_ollama_error);
    RUN_TEST(test_parse_ollama_bad_json);
    RUN_TEST(test_parse_dispatch);
    RUN_TEST(test_stream_chunk_null_and_empty);
    RUN_TEST(test_stream_chunk_done);
    RUN_TEST(test_stream_chunk_event_line);
    RUN_TEST(test_stream_chunk_bad_json);
    RUN_TEST(test_stream_chunk_anthropic_delta);
    RUN_TEST(test_stream_chunk_anthropic_stop);
    RUN_TEST(test_stream_chunk_ollama);
    RUN_TEST(test_stream_chunk_openai_delta_reasoning);
    RUN_TEST(test_stream_chunk_openai_extra_fields);
    RUN_TEST(test_extract_error);
    return UNITY_END();
}
