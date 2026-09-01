/*
 * Unity Tests for Chat Response Parser — Phase 2: Responses API Reasoning
 *
 * Tests that Responses API SSE events are parsed correctly:
 * - response.output_text.delta → content
 * - response.reasoning_summary_text.delta → reasoning_content
 * - response.completed → is_done with response object
 * - response.output_item.added with type: "reasoning" → extra_fields
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/resp_parser.h>

void setUp(void) { }
void tearDown(void) { }

void test_responses_output_text_delta_extracts_content(void);
void test_responses_reasoning_delta_extracts_reasoning_content(void);
void test_responses_completed_sets_done(void);
void test_responses_completed_includes_response_object(void);
void test_responses_output_item_added_reasoning_sets_flag(void);
void test_responses_output_item_added_message_no_flag(void);
void test_responses_other_event_produces_empty_chunk(void);
void test_responses_created_event_produces_empty_chunk(void);

void test_responses_output_text_delta_extracts_content(void) {
    const char* json = "{\"type\":\"response.output_text.delta\",\"delta\":\"Hello world\"}";
    ChatStreamChunk* chunk = chat_stream_chunk_parse(json);
    TEST_ASSERT_NOT_NULL(chunk);
    TEST_ASSERT_EQUAL_STRING("Hello world", chunk->content);
    TEST_ASSERT_FALSE(chunk->is_done);
    TEST_ASSERT_NULL(chunk->reasoning_content);
    chat_stream_chunk_destroy(chunk);
}

void test_responses_reasoning_delta_extracts_reasoning_content(void) {
    const char* json = "{\"type\":\"response.reasoning_summary_text.delta\",\"delta\":\"Let me think\"}";
    ChatStreamChunk* chunk = chat_stream_chunk_parse(json);
    TEST_ASSERT_NOT_NULL(chunk);
    TEST_ASSERT_NULL(chunk->content);
    TEST_ASSERT_EQUAL_STRING("Let me think", chunk->reasoning_content);
    TEST_ASSERT_FALSE(chunk->is_done);
    chat_stream_chunk_destroy(chunk);
}

void test_responses_completed_sets_done(void) {
    const char* json = "{\"type\":\"response.completed\",\"response\":{\"id\":\"resp_123\"}}";
    ChatStreamChunk* chunk = chat_stream_chunk_parse(json);
    TEST_ASSERT_NOT_NULL(chunk);
    TEST_ASSERT_TRUE(chunk->is_done);
    chat_stream_chunk_destroy(chunk);
}

void test_responses_completed_includes_response_object(void) {
    const char* json = "{\"type\":\"response.completed\",\"response\":{\"id\":\"resp_456\",\"usage\":{\"input_tokens\":10,\"output_tokens\":20}}}";
    ChatStreamChunk* chunk = chat_stream_chunk_parse(json);
    TEST_ASSERT_NOT_NULL(chunk);
    TEST_ASSERT_TRUE(chunk->is_done);
    TEST_ASSERT_NOT_NULL(chunk->extra_fields);
    json_t* resp = json_object_get(chunk->extra_fields, "response");
    TEST_ASSERT_NOT_NULL(resp);
    json_t* id = json_object_get(resp, "id");
    TEST_ASSERT_NOT_NULL(id);
    TEST_ASSERT_EQUAL_STRING("resp_456", json_string_value(id));
    chat_stream_chunk_destroy(chunk);
}

void test_responses_output_item_added_reasoning_sets_flag(void) {
    const char* json = "{\"type\":\"response.output_item.added\",\"item\":{\"type\":\"reasoning\",\"id\":\"rs_1\"}}";
    ChatStreamChunk* chunk = chat_stream_chunk_parse(json);
    TEST_ASSERT_NOT_NULL(chunk);
    TEST_ASSERT_NOT_NULL(chunk->extra_fields);
    json_t* flag = json_object_get(chunk->extra_fields, "reasoning_item_added");
    TEST_ASSERT_NOT_NULL(flag);
    TEST_ASSERT_TRUE(json_boolean_value(flag));
    chat_stream_chunk_destroy(chunk);
}

void test_responses_output_item_added_message_no_flag(void) {
    const char* json = "{\"type\":\"response.output_item.added\",\"item\":{\"type\":\"message\",\"id\":\"msg_1\"}}";
    ChatStreamChunk* chunk = chat_stream_chunk_parse(json);
    TEST_ASSERT_NOT_NULL(chunk);
    TEST_ASSERT_NULL(chunk->extra_fields);
    chat_stream_chunk_destroy(chunk);
}

void test_responses_other_event_produces_empty_chunk(void) {
    const char* json = "{\"type\":\"response.in_progress\"}";
    ChatStreamChunk* chunk = chat_stream_chunk_parse(json);
    TEST_ASSERT_NOT_NULL(chunk);
    TEST_ASSERT_NULL(chunk->content);
    TEST_ASSERT_NULL(chunk->reasoning_content);
    TEST_ASSERT_FALSE(chunk->is_done);
    chat_stream_chunk_destroy(chunk);
}

void test_responses_created_event_produces_empty_chunk(void) {
    const char* json = "{\"type\":\"response.created\",\"response\":{\"id\":\"resp_new\"}}";
    ChatStreamChunk* chunk = chat_stream_chunk_parse(json);
    TEST_ASSERT_NOT_NULL(chunk);
    TEST_ASSERT_NULL(chunk->content);
    TEST_ASSERT_FALSE(chunk->is_done);
    chat_stream_chunk_destroy(chunk);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_responses_output_text_delta_extracts_content);
    RUN_TEST(test_responses_reasoning_delta_extracts_reasoning_content);
    RUN_TEST(test_responses_completed_sets_done);
    RUN_TEST(test_responses_completed_includes_response_object);
    RUN_TEST(test_responses_output_item_added_reasoning_sets_flag);
    RUN_TEST(test_responses_output_item_added_message_no_flag);
    RUN_TEST(test_responses_other_event_produces_empty_chunk);
    RUN_TEST(test_responses_created_event_produces_empty_chunk);
    return UNITY_END();
}
