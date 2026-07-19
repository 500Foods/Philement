/*
 * Unity Test File: auth_chat_parse_request
 * This file contains unit tests for auth_chat_parse_request() in
 * src/api/wschat/auth_chat/auth_chat.c
 *
 * auth_chat_parse_request() is pure request-JSON parsing/validation logic:
 * it extracts messages (required), optional context_hashes, engine,
 * temperature, max_tokens and stream fields, setting sensible defaults and
 * emitting an error_message on any validation failure.
 *
 * CHANGELOG:
 * 2026-07-18: Initial implementation covering parse/validation branches
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/wschat/auth_chat/auth_chat.h>
#include <src/api/wschat/helpers/context_hashing.h>

// Test function prototypes
void test_auth_chat_parse_request_null_json(void);
void test_auth_chat_parse_request_missing_messages(void);
void test_auth_chat_parse_request_messages_not_array(void);
void test_auth_chat_parse_request_valid_minimal(void);
void test_auth_chat_parse_request_engine_extracted(void);
void test_auth_chat_parse_request_temperature_default_when_absent(void);
void test_auth_chat_parse_request_temperature_out_of_range(void);
void test_auth_chat_parse_request_max_tokens_default_when_absent(void);
void test_auth_chat_parse_request_max_tokens_out_of_range(void);
void test_auth_chat_parse_request_stream_true(void);
void test_auth_chat_parse_request_all_fields(void);
void test_auth_chat_parse_request_invalid_context_hashes_type(void);

void setUp(void) {
    // Nothing to reset; this function has no global dependencies.
}

void tearDown(void) {
    // Caller owns the parsed outputs and frees them in each test.
}

// Helper: free the parse outputs the function populates.
static void free_parse_outputs(char *engine, json_t *messages,
                               char **context_hashes, size_t context_hash_count,
                               char *error_message) {
    free(engine);
    if (messages) json_decref(messages);
    if (context_hashes) chat_context_free_hash_array(context_hashes, context_hash_count);
    free(error_message);
}

// NULL request_json -> error_message set, returns false.
void test_auth_chat_parse_request_null_json(void) {
    char *engine = NULL;
    json_t *messages = NULL;
    char **context_hashes = NULL;
    size_t context_hash_count = 0;
    double temperature = -999.0;
    int max_tokens = -999;
    bool stream = true;
    char *error_message = NULL;

    bool ok = auth_chat_parse_request(NULL, &engine, &messages, &context_hashes,
                                      &context_hash_count, &temperature, &max_tokens,
                                      &stream, &error_message);

    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(error_message);
    TEST_ASSERT_NULL(messages);
    free_parse_outputs(engine, messages, context_hashes, context_hash_count, error_message);
}

// Valid JSON but no "messages" key -> error_message set, returns false.
void test_auth_chat_parse_request_missing_messages(void) {
    char *engine = NULL;
    json_t *messages = NULL;
    char **context_hashes = NULL;
    size_t context_hash_count = 0;
    double temperature = -999.0;
    int max_tokens = -999;
    bool stream = true;
    char *error_message = NULL;

    json_t *req = json_object();
    json_object_set_new(req, "engine", json_string("gpt-4"));

    bool ok = auth_chat_parse_request(req, &engine, &messages, &context_hashes,
                                      &context_hash_count, &temperature, &max_tokens,
                                      &stream, &error_message);

    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(error_message);
    TEST_ASSERT_NULL(messages);
    json_decref(req);
    free_parse_outputs(engine, messages, context_hashes, context_hash_count, error_message);
}

// "messages" present but not an array -> error_message set, returns false.
void test_auth_chat_parse_request_messages_not_array(void) {
    char *engine = NULL;
    json_t *messages = NULL;
    char **context_hashes = NULL;
    size_t context_hash_count = 0;
    double temperature = -999.0;
    int max_tokens = -999;
    bool stream = true;
    char *error_message = NULL;

    json_t *req = json_object();
    json_object_set_new(req, "messages", json_string("not-an-array"));

    bool ok = auth_chat_parse_request(req, &engine, &messages, &context_hashes,
                                      &context_hash_count, &temperature, &max_tokens,
                                      &stream, &error_message);

    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(error_message);
    json_decref(req);
    free_parse_outputs(engine, messages, context_hashes, context_hash_count, error_message);
}

// Minimal valid: messages array only. Defaults applied, returns true.
void test_auth_chat_parse_request_valid_minimal(void) {
    char *engine = NULL;
    json_t *messages = NULL;
    char **context_hashes = NULL;
    size_t context_hash_count = 0;
    double temperature = -999.0;
    int max_tokens = -999;
    bool stream = true;
    char *error_message = NULL;

    json_t *req = json_object();
    json_t *msgs = json_array();
    json_array_append_new(msgs, json_string("hi"));
    json_object_set_new(req, "messages", msgs);

    bool ok = auth_chat_parse_request(req, &engine, &messages, &context_hashes,
                                      &context_hash_count, &temperature, &max_tokens,
                                      &stream, &error_message);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_NULL(error_message);
    TEST_ASSERT_NOT_NULL(messages);
    TEST_ASSERT_NULL(engine);
    TEST_ASSERT_EQUAL(-1.0, temperature);
    TEST_ASSERT_EQUAL(-1, max_tokens);
    TEST_ASSERT_FALSE(stream);
    TEST_ASSERT_EQUAL(0, context_hash_count);

    json_decref(req);
    free_parse_outputs(engine, messages, context_hashes, context_hash_count, error_message);
}

// engine string is extracted via strdup when present.
void test_auth_chat_parse_request_engine_extracted(void) {
    char *engine = NULL;
    json_t *messages = NULL;
    char **context_hashes = NULL;
    size_t context_hash_count = 0;
    double temperature = -999.0;
    int max_tokens = -999;
    bool stream = false;
    char *error_message = NULL;

    json_t *req = json_object();
    json_t *msgs = json_array();
    json_array_append_new(msgs, json_string("hi"));
    json_object_set_new(req, "messages", msgs);
    json_object_set_new(req, "engine", json_string("claude-3"));

    bool ok = auth_chat_parse_request(req, &engine, &messages, &context_hashes,
                                      &context_hash_count, &temperature, &max_tokens,
                                      &stream, &error_message);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_STRING("claude-3", engine);

    json_decref(req);
    free_parse_outputs(engine, messages, context_hashes, context_hash_count, error_message);
}

// temperature absent -> default -1.0 preserved.
void test_auth_chat_parse_request_temperature_default_when_absent(void) {
    char *engine = NULL;
    json_t *messages = NULL;
    char **context_hashes = NULL;
    size_t context_hash_count = 0;
    double temperature = -1.0;
    int max_tokens = -1;
    bool stream = false;
    char *error_message = NULL;

    json_t *req = json_object();
    json_t *msgs = json_array();
    json_array_append_new(msgs, json_string("hi"));
    json_object_set_new(req, "messages", msgs);

    bool ok = auth_chat_parse_request(req, &engine, &messages, &context_hashes,
                                      &context_hash_count, &temperature, &max_tokens,
                                      &stream, &error_message);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL(-1.0, temperature);

    json_decref(req);
    free_parse_outputs(engine, messages, context_hashes, context_hash_count, error_message);
}

// temperature out of range (> 2.0) -> error_message set, returns false.
void test_auth_chat_parse_request_temperature_out_of_range(void) {
    char *engine = NULL;
    json_t *messages = NULL;
    char **context_hashes = NULL;
    size_t context_hash_count = 0;
    double temperature = -1.0;
    int max_tokens = -1;
    bool stream = false;
    char *error_message = NULL;

    json_t *req = json_object();
    json_t *msgs = json_array();
    json_array_append_new(msgs, json_string("hi"));
    json_object_set_new(req, "messages", msgs);
    json_object_set_new(req, "temperature", json_real(2.5));

    bool ok = auth_chat_parse_request(req, &engine, &messages, &context_hashes,
                                      &context_hash_count, &temperature, &max_tokens,
                                      &stream, &error_message);

    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(error_message);

    json_decref(req);
    free_parse_outputs(engine, messages, context_hashes, context_hash_count, error_message);
}

// max_tokens absent -> default -1 preserved.
void test_auth_chat_parse_request_max_tokens_default_when_absent(void) {
    char *engine = NULL;
    json_t *messages = NULL;
    char **context_hashes = NULL;
    size_t context_hash_count = 0;
    double temperature = -1.0;
    int max_tokens = -1;
    bool stream = false;
    char *error_message = NULL;

    json_t *req = json_object();
    json_t *msgs = json_array();
    json_array_append_new(msgs, json_string("hi"));
    json_object_set_new(req, "messages", msgs);

    bool ok = auth_chat_parse_request(req, &engine, &messages, &context_hashes,
                                      &context_hash_count, &temperature, &max_tokens,
                                      &stream, &error_message);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL(-1, max_tokens);

    json_decref(req);
    free_parse_outputs(engine, messages, context_hashes, context_hash_count, error_message);
}

// max_tokens out of range (< 1) -> error_message set, returns false.
void test_auth_chat_parse_request_max_tokens_out_of_range(void) {
    char *engine = NULL;
    json_t *messages = NULL;
    char **context_hashes = NULL;
    size_t context_hash_count = 0;
    double temperature = -1.0;
    int max_tokens = -1;
    bool stream = false;
    char *error_message = NULL;

    json_t *req = json_object();
    json_t *msgs = json_array();
    json_array_append_new(msgs, json_string("hi"));
    json_object_set_new(req, "messages", msgs);
    json_object_set_new(req, "max_tokens", json_integer(0));

    bool ok = auth_chat_parse_request(req, &engine, &messages, &context_hashes,
                                      &context_hash_count, &temperature, &max_tokens,
                                      &stream, &error_message);

    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(error_message);

    json_decref(req);
    free_parse_outputs(engine, messages, context_hashes, context_hash_count, error_message);
}

// stream=true is extracted.
void test_auth_chat_parse_request_stream_true(void) {
    char *engine = NULL;
    json_t *messages = NULL;
    char **context_hashes = NULL;
    size_t context_hash_count = 0;
    double temperature = -1.0;
    int max_tokens = -1;
    bool stream = false;
    char *error_message = NULL;

    json_t *req = json_object();
    json_t *msgs = json_array();
    json_array_append_new(msgs, json_string("hi"));
    json_object_set_new(req, "messages", msgs);
    json_object_set_new(req, "stream", json_true());

    bool ok = auth_chat_parse_request(req, &engine, &messages, &context_hashes,
                                      &context_hash_count, &temperature, &max_tokens,
                                      &stream, &error_message);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(stream);

    json_decref(req);
    free_parse_outputs(engine, messages, context_hashes, context_hash_count, error_message);
}

// All fields supplied with valid values -> returns true, all populated.
void test_auth_chat_parse_request_all_fields(void) {
    char *engine = NULL;
    json_t *messages = NULL;
    char **context_hashes = NULL;
    size_t context_hash_count = 0;
    double temperature = -1.0;
    int max_tokens = -1;
    bool stream = false;
    char *error_message = NULL;

    json_t *req = json_object();
    json_t *msgs = json_array();
    json_array_append_new(msgs, json_string("hi"));
    json_object_set_new(req, "messages", msgs);
    json_object_set_new(req, "engine", json_string("gpt-4"));
    json_object_set_new(req, "temperature", json_real(0.7));
    json_object_set_new(req, "max_tokens", json_integer(1000));
    json_object_set_new(req, "stream", json_false());

    bool ok = auth_chat_parse_request(req, &engine, &messages, &context_hashes,
                                      &context_hash_count, &temperature, &max_tokens,
                                      &stream, &error_message);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_STRING("gpt-4", engine);
    TEST_ASSERT_EQUAL_DOUBLE(0.7, temperature);
    TEST_ASSERT_EQUAL(1000, max_tokens);
    TEST_ASSERT_FALSE(stream);

    json_decref(req);
    free_parse_outputs(engine, messages, context_hashes, context_hash_count, error_message);
}

// context_hashes present but not an array -> error_message set, returns false.
void test_auth_chat_parse_request_invalid_context_hashes_type(void) {
    char *engine = NULL;
    json_t *messages = NULL;
    char **context_hashes = NULL;
    size_t context_hash_count = 0;
    double temperature = -1.0;
    int max_tokens = -1;
    bool stream = false;
    char *error_message = NULL;

    json_t *req = json_object();
    json_t *msgs = json_array();
    json_array_append_new(msgs, json_string("hi"));
    json_object_set_new(req, "messages", msgs);
    json_object_set_new(req, "context_hashes", json_string("not-an-array"));

    bool ok = auth_chat_parse_request(req, &engine, &messages, &context_hashes,
                                      &context_hash_count, &temperature, &max_tokens,
                                      &stream, &error_message);

    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(error_message);

    json_decref(req);
    free_parse_outputs(engine, messages, context_hashes, context_hash_count, error_message);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_auth_chat_parse_request_null_json);
    RUN_TEST(test_auth_chat_parse_request_missing_messages);
    RUN_TEST(test_auth_chat_parse_request_messages_not_array);
    RUN_TEST(test_auth_chat_parse_request_valid_minimal);
    RUN_TEST(test_auth_chat_parse_request_engine_extracted);
    RUN_TEST(test_auth_chat_parse_request_temperature_default_when_absent);
    RUN_TEST(test_auth_chat_parse_request_temperature_out_of_range);
    RUN_TEST(test_auth_chat_parse_request_max_tokens_default_when_absent);
    RUN_TEST(test_auth_chat_parse_request_max_tokens_out_of_range);
    RUN_TEST(test_auth_chat_parse_request_stream_true);
    RUN_TEST(test_auth_chat_parse_request_all_fields);
    RUN_TEST(test_auth_chat_parse_request_invalid_context_hashes_type);

    return UNITY_END();
}
