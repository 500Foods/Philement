/*
 * Unity Test File: auth_chat_stream_parse_request
 * This file contains unit tests for auth_chat_stream_parse_request() in
 * src/api/wschat/auth_stream/auth_stream.c
 *
 * Thin wrapper around auth_chat_parse_request — covers the wrapper entry
 * and a few representative success/failure branches.
 *
 * CHANGELOG:
 * 2026-07-19: Initial implementation
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/wschat/auth_stream/auth_stream.h>
#include <src/api/wschat/helpers/context_hashing.h>

// Test function prototypes
void test_auth_chat_stream_parse_request_null_json(void);
void test_auth_chat_stream_parse_request_missing_messages(void);
void test_auth_chat_stream_parse_request_valid_minimal(void);
void test_auth_chat_stream_parse_request_all_fields(void);

void setUp(void) {
}

void tearDown(void) {
}

static void free_parse_outputs(char *engine, json_t *messages,
                               char **context_hashes, size_t context_hash_count,
                               char *error_message) {
    free(engine);
    if (messages) json_decref(messages);
    if (context_hashes) chat_context_free_hash_array(context_hashes, context_hash_count);
    free(error_message);
}

void test_auth_chat_stream_parse_request_null_json(void) {
    char *engine = NULL;
    json_t *messages = NULL;
    char **context_hashes = NULL;
    size_t context_hash_count = 0;
    double temperature = -999.0;
    int max_tokens = -999;
    bool stream = true;
    char *error_message = NULL;

    bool ok = auth_chat_stream_parse_request(NULL, &engine, &messages, &context_hashes,
                                             &context_hash_count, &temperature, &max_tokens,
                                             &stream, &error_message);

    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(error_message);
    free_parse_outputs(engine, messages, context_hashes, context_hash_count, error_message);
}

void test_auth_chat_stream_parse_request_missing_messages(void) {
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

    bool ok = auth_chat_stream_parse_request(req, &engine, &messages, &context_hashes,
                                             &context_hash_count, &temperature, &max_tokens,
                                             &stream, &error_message);

    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(error_message);
    json_decref(req);
    free_parse_outputs(engine, messages, context_hashes, context_hash_count, error_message);
}

void test_auth_chat_stream_parse_request_valid_minimal(void) {
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

    bool ok = auth_chat_stream_parse_request(req, &engine, &messages, &context_hashes,
                                             &context_hash_count, &temperature, &max_tokens,
                                             &stream, &error_message);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_NULL(error_message);
    TEST_ASSERT_NOT_NULL(messages);
    TEST_ASSERT_FALSE(stream);

    json_decref(req);
    free_parse_outputs(engine, messages, context_hashes, context_hash_count, error_message);
}

void test_auth_chat_stream_parse_request_all_fields(void) {
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
    json_object_set_new(req, "stream", json_true());

    bool ok = auth_chat_stream_parse_request(req, &engine, &messages, &context_hashes,
                                             &context_hash_count, &temperature, &max_tokens,
                                             &stream, &error_message);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_STRING("gpt-4", engine);
    TEST_ASSERT_EQUAL_DOUBLE(0.7, temperature);
    TEST_ASSERT_EQUAL(1000, max_tokens);
    TEST_ASSERT_TRUE(stream);

    json_decref(req);
    free_parse_outputs(engine, messages, context_hashes, context_hash_count, error_message);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_auth_chat_stream_parse_request_null_json);
    RUN_TEST(test_auth_chat_stream_parse_request_missing_messages);
    RUN_TEST(test_auth_chat_stream_parse_request_valid_minimal);
    RUN_TEST(test_auth_chat_stream_parse_request_all_fields);

    return UNITY_END();
}
