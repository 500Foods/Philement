/*
 * Unity Test File: auth_chat helper functions
 * Covers extractable helpers in src/api/wschat/auth_chat/auth_chat.c that are
 * easier to unit-test than the full MHD handler path.
 *
 * CHANGELOG:
 * 2026-07-19: Initial helper coverage for JWT, params, content, payload,
 *             segment stats, and context hashing attachment
 *
 * TEST_VERSION: 1.0.0
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/auth_chat/auth_chat.h>
#include <src/api/wschat/helpers/engine_cache.h>
#include <src/api/wschat/helpers/req_builder.h>

#define USE_MOCK_AUTH_SERVICE_JWT
#define USE_MOCK_AUTH_CHAT_DEPS
#include <unity/mocks/mock_auth_service_jwt.h>
#include <unity/mocks/mock_auth_chat_deps.h>

void test_auth_chat_validate_jwt_from_header_null(void);
void test_auth_chat_validate_jwt_from_header_bad_prefix(void);
void test_auth_chat_validate_jwt_from_header_valid(void);
void test_auth_chat_resolve_request_params_overrides(void);
void test_auth_chat_resolve_request_params_defaults(void);
void test_auth_chat_resolve_request_params_null_engine(void);
void test_auth_chat_content_to_string_string_and_array(void);
void test_auth_chat_content_to_string_unsupported(void);
void test_auth_chat_resolve_content_string_media_success(void);
void test_auth_chat_resolve_content_string_media_failure(void);
void test_auth_chat_resolve_content_string_array_no_media(void);
void test_auth_chat_messages_json_to_list_basic(void);
void test_auth_chat_messages_json_to_list_skips_invalid(void);
void test_auth_chat_payload_exceeds_limit_branches(void);
void test_auth_chat_collect_segment_stats_hash_hit_and_miss(void);
void test_auth_chat_collect_segment_stats_no_messages(void);
void test_auth_chat_attach_context_hashing_stats(void);
void test_auth_chat_free_segment_stats_null_safe(void);

void setUp(void) {
    mock_auth_service_jwt_reset_all();
    mock_auth_chat_deps_reset_all();
}

void tearDown(void) {
}

void test_auth_chat_validate_jwt_from_header_null(void) {
    jwt_validation_result_t result = auth_chat_validate_jwt_from_header(NULL);
    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_NULL(result.claims);
}

void test_auth_chat_validate_jwt_from_header_bad_prefix(void) {
    jwt_validation_result_t result = auth_chat_validate_jwt_from_header("Basic abc");
    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_NULL(result.claims);
}

void test_auth_chat_validate_jwt_from_header_valid(void) {
    jwt_claims_t claims_storage;
    memset(&claims_storage, 0, sizeof(claims_storage));
    claims_storage.database = (char*)"testdb";
    claims_storage.user_id = 9;
    mock_auth_service_jwt_set_validation_result(
        (jwt_validation_result_t){true, &claims_storage, JWT_ERROR_NONE});

    jwt_validation_result_t result = auth_chat_validate_jwt_from_header("Bearer good.token");
    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_NOT_NULL(result.claims);
    TEST_ASSERT_EQUAL_STRING("testdb", result.claims->database);
    free_jwt_validation_result(&result);
}

void test_auth_chat_resolve_request_params_overrides(void) {
    ChatEngineConfig engine = {0};
    engine.temperature_default = 0.2;
    engine.max_tokens = 111;

    ChatRequestParams params = auth_chat_resolve_request_params(&engine, 0.9, 500, true);
    TEST_ASSERT_EQUAL_DOUBLE(0.9, params.temperature);
    TEST_ASSERT_EQUAL(500, params.max_tokens);
    TEST_ASSERT_TRUE(params.stream);
}

void test_auth_chat_resolve_request_params_defaults(void) {
    ChatEngineConfig engine = {0};
    engine.temperature_default = 0.3;
    engine.max_tokens = 222;

    ChatRequestParams params = auth_chat_resolve_request_params(&engine, -1.0, -1, false);
    TEST_ASSERT_EQUAL_DOUBLE(0.3, params.temperature);
    TEST_ASSERT_EQUAL(222, params.max_tokens);
    TEST_ASSERT_FALSE(params.stream);
}

void test_auth_chat_resolve_request_params_null_engine(void) {
    ChatRequestParams params = auth_chat_resolve_request_params(NULL, 1.0, 10, true);
    TEST_ASSERT_TRUE(params.stream);
}

void test_auth_chat_content_to_string_string_and_array(void) {
    json_t *str = json_string("hello");
    char *out = auth_chat_content_to_string(str);
    TEST_ASSERT_EQUAL_STRING("hello", out);
    free(out);
    json_decref(str);

    json_t *arr = json_array();
    json_array_append_new(arr, json_string("part"));
    out = auth_chat_content_to_string(arr);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_NOT_NULL(strstr(out, "part"));
    free(out);
    json_decref(arr);
}

void test_auth_chat_content_to_string_unsupported(void) {
    TEST_ASSERT_NULL(auth_chat_content_to_string(NULL));
    json_t *num = json_integer(7);
    TEST_ASSERT_NULL(auth_chat_content_to_string(num));
    json_decref(num);
}

void test_auth_chat_resolve_content_string_media_success(void) {
    mock_auth_chat_deps_set_media_resolve(true, "[{\"type\":\"text\",\"text\":\"ok\"}]", NULL);

    json_t *arr = json_array();
    json_t *item = json_object();
    json_object_set_new(item, "type", json_string("image_url"));
    json_object_set_new(item, "url", json_string("media:abc123"));
    json_array_append_new(arr, item);

    char *out = auth_chat_resolve_content_string("db", arr);
    TEST_ASSERT_EQUAL_STRING("[{\"type\":\"text\",\"text\":\"ok\"}]", out);
    free(out);
    json_decref(arr);
}

void test_auth_chat_resolve_content_string_media_failure(void) {
    mock_auth_chat_deps_set_media_resolve(false, NULL, "not found");

    json_t *arr = json_array();
    json_t *item = json_object();
    json_object_set_new(item, "type", json_string("image_url"));
    json_object_set_new(item, "url", json_string("media:missing"));
    json_array_append_new(arr, item);

    char *out = auth_chat_resolve_content_string("db", arr);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_NOT_NULL(strstr(out, "media:missing"));
    free(out);
    json_decref(arr);
}

void test_auth_chat_resolve_content_string_array_no_media(void) {
    json_t *arr = json_array();
    json_t *item = json_object();
    json_object_set_new(item, "type", json_string("text"));
    json_object_set_new(item, "text", json_string("plain"));
    json_array_append_new(arr, item);

    char *out = auth_chat_resolve_content_string("db", arr);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_NOT_NULL(strstr(out, "plain"));
    free(out);
    json_decref(arr);
}

void test_auth_chat_messages_json_to_list_basic(void) {
    json_t *messages = json_array();
    json_t *msg = json_object();
    json_object_set_new(msg, "role", json_string("user"));
    json_object_set_new(msg, "content", json_string("hi"));
    json_array_append_new(messages, msg);

    ChatMessage *list = auth_chat_messages_json_to_list("db", messages);
    TEST_ASSERT_NOT_NULL(list);
    TEST_ASSERT_EQUAL_STRING("hi", list->content);
    chat_message_list_destroy(list);
    json_decref(messages);
}

void test_auth_chat_messages_json_to_list_skips_invalid(void) {
    json_t *messages = json_array();
    json_array_append_new(messages, json_string("not-object"));
    json_t *bad = json_object();
    json_object_set_new(bad, "role", json_string("user"));
    json_array_append_new(messages, bad);

    ChatMessage *list = auth_chat_messages_json_to_list("db", messages);
    TEST_ASSERT_NULL(list);
    json_decref(messages);

    TEST_ASSERT_NULL(auth_chat_messages_json_to_list("db", NULL));
}

void test_auth_chat_payload_exceeds_limit_branches(void) {
    char buf[128];
    TEST_ASSERT_FALSE(auth_chat_payload_exceeds_limit(100, 0, buf, sizeof(buf)));
    TEST_ASSERT_FALSE(auth_chat_payload_exceeds_limit(100, 1, buf, sizeof(buf)));
    TEST_ASSERT_TRUE(auth_chat_payload_exceeds_limit((size_t)2 * 1024 * 1024, 1, buf, sizeof(buf)));
    TEST_ASSERT_NOT_NULL(strstr(buf, "exceeds engine limit"));
}

void test_auth_chat_collect_segment_stats_hash_hit_and_miss(void) {
    json_t *messages = json_array();

    json_t *m1 = json_object();
    json_object_set_new(m1, "role", json_string("user"));
    json_object_set_new(m1, "content",
        json_string("This message is intentionally long enough to exceed the fifty-four "
                    "byte hash overhead used when computing bandwidth savings."));
    json_array_append_new(messages, m1);

    json_t *m2 = json_object();
    json_object_set_new(m2, "role", json_string("assistant"));
    json_object_set_new(m2, "content", json_string("short"));
    json_array_append_new(messages, m2);

    // 43-char base64url hashes (format required by chat_context_validate_hash)
    char *h1 = strdup("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopq");
    char *h2 = strdup("zyxwvutsrqponmlkjihgfedcbaZYXWVUTSRQPONMLKJ");
    char *hashes[] = {h1, h2};

    mock_auth_chat_deps_set_segment_exists(true);
    AuthChatSegmentStats stats = {0};
    bool ok = auth_chat_collect_segment_stats("db", messages, hashes, 2, "assistant reply", &stats);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(stats.segment_hash_count >= 2);
    TEST_ASSERT_TRUE(stats.hashes_used >= 1);
    TEST_ASSERT_TRUE(stats.bandwidth_saved_bytes > 0);
    TEST_ASSERT_TRUE(stats.bandwidth_saved_percent > 0.0);

    auth_chat_free_segment_stats(&stats);

    mock_auth_chat_deps_set_segment_exists(false);
    ok = auth_chat_collect_segment_stats("db", messages, hashes, 2, "assistant reply", &stats);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(stats.hashes_missed >= 1);
    auth_chat_free_segment_stats(&stats);

    free(h1);
    free(h2);
    json_decref(messages);
}

void test_auth_chat_collect_segment_stats_no_messages(void) {
    AuthChatSegmentStats stats = {0};
    bool ok = auth_chat_collect_segment_stats("db", NULL, NULL, 0, "only-response", &stats);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL(1, stats.segment_hash_count);
    auth_chat_free_segment_stats(&stats);

    TEST_ASSERT_FALSE(auth_chat_collect_segment_stats("db", NULL, NULL, 0, "x", NULL));
}

void test_auth_chat_attach_context_hashing_stats(void) {
    json_t *response = json_object();
    auth_chat_attach_context_hashing_stats(response, "db", 2, 1, 100, 12.5);

    json_t *ctx = json_object_get(response, "context_hashing");
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_EQUAL(2, json_integer_value(json_object_get(ctx, "hashes_used")));
    TEST_ASSERT_EQUAL(1, json_integer_value(json_object_get(ctx, "hashes_missed")));
    TEST_ASSERT_EQUAL(100, json_integer_value(json_object_get(ctx, "bandwidth_saved_bytes")));
    TEST_ASSERT_NOT_NULL(json_object_get(ctx, "cache_hits"));

    json_decref(response);
    auth_chat_attach_context_hashing_stats(NULL, "db", 0, 0, 0, 0.0);
}

void test_auth_chat_free_segment_stats_null_safe(void) {
    auth_chat_free_segment_stats(NULL);
    AuthChatSegmentStats empty = {0};
    auth_chat_free_segment_stats(&empty);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_auth_chat_validate_jwt_from_header_null);
    RUN_TEST(test_auth_chat_validate_jwt_from_header_bad_prefix);
    RUN_TEST(test_auth_chat_validate_jwt_from_header_valid);
    RUN_TEST(test_auth_chat_resolve_request_params_overrides);
    RUN_TEST(test_auth_chat_resolve_request_params_defaults);
    RUN_TEST(test_auth_chat_resolve_request_params_null_engine);
    RUN_TEST(test_auth_chat_content_to_string_string_and_array);
    RUN_TEST(test_auth_chat_content_to_string_unsupported);
    RUN_TEST(test_auth_chat_resolve_content_string_media_success);
    RUN_TEST(test_auth_chat_resolve_content_string_media_failure);
    RUN_TEST(test_auth_chat_resolve_content_string_array_no_media);
    RUN_TEST(test_auth_chat_messages_json_to_list_basic);
    RUN_TEST(test_auth_chat_messages_json_to_list_skips_invalid);
    RUN_TEST(test_auth_chat_payload_exceeds_limit_branches);
    RUN_TEST(test_auth_chat_collect_segment_stats_hash_hit_and_miss);
    RUN_TEST(test_auth_chat_collect_segment_stats_no_messages);
    RUN_TEST(test_auth_chat_attach_context_hashing_stats);
    RUN_TEST(test_auth_chat_free_segment_stats_null_safe);

    return UNITY_END();
}
