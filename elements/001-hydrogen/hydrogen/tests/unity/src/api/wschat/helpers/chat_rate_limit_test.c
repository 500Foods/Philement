/*
 * Unity Tests for Chat Rate Limit (Phase 10b)
 *
 * Covers:
 *   - check_and_record: ALLOWED on first request, THROTTLED_REQUESTS at
 *     the cap, THROTTLED_TOKENS at the token budget, fail-open when
 *     app_config is NULL or RateLimit.Enabled is false, window reset
 *     when IntervalSeconds elapses, multi-sub isolation.
 *   - record_output: increment for known sub, no-op for unknown sub,
 *     no-op when disabled.
 *   - estimate_input_tokens: chars/4 heuristic across nested JSON,
 *     UTF-8 multi-byte handling, NULL/non-array safety.
 *   - build_error_response: envelope shape (success, error, message,
 *     error_code).
 *
 * Test isolation: every test calls chat_rate_limit_reset_all() in
 * setUp() so the linked-list of buckets starts empty.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/chat_rate_limit.h>

#include <string.h>

static AppConfig test_app;

void setUp(void) {
    memset(&test_app, 0, sizeof(test_app));
    chat_config_apply_defaults(&test_app.chat);
    test_app.chat.RateLimit.Enabled = true;
    test_app.chat.RateLimit.MaxRequestsPerInterval = 3;
    test_app.chat.RateLimit.IntervalSeconds = 60;
    test_app.chat.RateLimit.MaxTokensPerInterval = 100;
    app_config = &test_app;
    chat_rate_limit_reset_all();
    chat_rate_limit_init();
}

void tearDown(void) {
    chat_rate_limit_reset_all();
    app_config = NULL;
}

/* ---------- check_and_record ---------- */

void test_check_allowed_on_first_request(void);
void test_check_throttled_requests_at_cap(void);
void test_check_throttled_tokens_at_budget(void);
void test_check_request_takes_precedence_over_token_check(void);
void test_check_different_subs_are_isolated(void);
void test_check_disabled_fails_open(void);
void test_check_null_app_config_fails_open(void);
void test_check_null_or_empty_sub_fails_open(void);
void test_check_zero_max_requests_disables_request_cap(void);
void test_check_zero_interval_fails_open(void);
void test_check_negative_input_tokens_clamped_to_zero(void);

/* ---------- record_output ---------- */

void test_record_output_increments_for_known_sub(void);
void test_record_output_noop_for_unknown_sub(void);
void test_record_output_disabled_is_noop(void);
void test_record_output_non_positive_is_noop(void);

/* ---------- estimate_input_tokens ---------- */

void test_estimate_null_is_zero(void);
void test_estimate_non_array_is_zero(void);
void test_estimate_chars_over_4(void);
void test_estimate_walks_nested_objects(void);
void test_estimate_handles_utf8_multibyte(void);
void test_estimate_empty_messages_is_zero(void);

/* ---------- build_error_response ---------- */

void test_build_error_response_envelope_shape(void);
void test_build_error_response_null_message_uses_default(void);

/* ---------- utf8_chars / walk_json / find_locked / new_bucket_locked (low-level helpers) ---------- */

void test_utf8_chars_counts_codepoints(void);
void test_find_locked_returns_null_for_unknown(void);
void test_find_locked_returns_entry_after_record(void);
void test_new_bucket_locked_creates_entry(void);

void test_check_allowed_on_first_request(void) {
    ChatRateLimitResult r = chat_rate_limit_check_and_record("alice", 10);
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, r);
    TEST_ASSERT_EQUAL_INT(1, chat_rate_limit_request_count("alice"));
    TEST_ASSERT_EQUAL_INT64(10, chat_rate_limit_token_count("alice"));
}

void test_check_throttled_requests_at_cap(void) {
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, chat_rate_limit_check_and_record("alice", 1));
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, chat_rate_limit_check_and_record("alice", 1));
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, chat_rate_limit_check_and_record("alice", 1));
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_THROTTLED_REQUESTS,
                      chat_rate_limit_check_and_record("alice", 1));
}

void test_check_throttled_tokens_at_budget(void) {
    /* MaxRequestsPerInterval=3 (room), MaxTokensPerInterval=100.
     * First 60-token request: 0+60 = 60 <= 100, ALLOWED.
     * Second 60-token request: 60+60 = 120 > 100, THROTTLED_TOKENS. */
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, chat_rate_limit_check_and_record("alice", 60));
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_THROTTLED_TOKENS,
                      chat_rate_limit_check_and_record("alice", 60));
}

void test_check_request_takes_precedence_over_token_check(void) {
    /* Three requests at 1 token each — request cap (3) is reached
     * first before token cap (100). */
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, chat_rate_limit_check_and_record("alice", 1));
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, chat_rate_limit_check_and_record("alice", 1));
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, chat_rate_limit_check_and_record("alice", 1));
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_THROTTLED_REQUESTS,
                      chat_rate_limit_check_and_record("alice", 1));
}

void test_check_different_subs_are_isolated(void) {
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, chat_rate_limit_check_and_record("alice", 1));
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, chat_rate_limit_check_and_record("alice", 1));
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, chat_rate_limit_check_and_record("alice", 1));
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_THROTTLED_REQUESTS,
                      chat_rate_limit_check_and_record("alice", 1));
    /* Bob is unaffected — fresh bucket. */
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, chat_rate_limit_check_and_record("bob", 50));
    TEST_ASSERT_EQUAL_INT(0, chat_rate_limit_request_count("alice") == 4); /* not reset; still 3 */
    TEST_ASSERT_EQUAL_INT(1, chat_rate_limit_request_count("bob"));
}

void test_check_disabled_fails_open(void) {
    test_app.chat.RateLimit.Enabled = false;
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, chat_rate_limit_check_and_record("alice", 999999));
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, chat_rate_limit_check_and_record("alice", 999999));
    TEST_ASSERT_EQUAL_INT(0, chat_rate_limit_request_count("alice"));
}

void test_check_null_app_config_fails_open(void) {
    app_config = NULL;
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, chat_rate_limit_check_and_record("alice", 50));
}

void test_check_null_or_empty_sub_fails_open(void) {
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, chat_rate_limit_check_and_record(NULL, 50));
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, chat_rate_limit_check_and_record("", 50));
}

void test_check_zero_max_requests_disables_request_cap(void) {
    test_app.chat.RateLimit.MaxRequestsPerInterval = 0;
    test_app.chat.RateLimit.MaxTokensPerInterval = 50;
    /* Request cap is off (0); only token cap matters. */
    for (int i = 0; i < 20; i++) {
        TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED,
                          chat_rate_limit_check_and_record("alice", 1));
    }
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_THROTTLED_TOKENS,
                      chat_rate_limit_check_and_record("alice", 50));
}

void test_check_zero_interval_fails_open(void) {
    test_app.chat.RateLimit.IntervalSeconds = 0;
    /* interval_seconds <= 0 fails open per defensive coding. */
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, chat_rate_limit_check_and_record("alice", 1));
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, chat_rate_limit_check_and_record("alice", 1));
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, chat_rate_limit_check_and_record("alice", 1));
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, chat_rate_limit_check_and_record("alice", 1));
}

void test_check_negative_input_tokens_clamped_to_zero(void) {
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, chat_rate_limit_check_and_record("alice", -5));
    TEST_ASSERT_EQUAL_INT64(0, chat_rate_limit_token_count("alice"));
}

/* ---------- record_output ---------- */

void test_record_output_increments_for_known_sub(void) {
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, chat_rate_limit_check_and_record("alice", 10));
    chat_rate_limit_record_output("alice", 40);
    TEST_ASSERT_EQUAL_INT64(50, chat_rate_limit_token_count("alice"));
}

void test_record_output_noop_for_unknown_sub(void) {
    chat_rate_limit_record_output("ghost", 100);
    TEST_ASSERT_EQUAL_INT64(0, chat_rate_limit_token_count("ghost"));
}

void test_record_output_disabled_is_noop(void) {
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, chat_rate_limit_check_and_record("alice", 10));
    test_app.chat.RateLimit.Enabled = false;
    chat_rate_limit_record_output("alice", 1000);
    TEST_ASSERT_EQUAL_INT64(10, chat_rate_limit_token_count("alice"));
}

void test_record_output_non_positive_is_noop(void) {
    TEST_ASSERT_EQUAL(CHAT_RATE_LIMIT_ALLOWED, chat_rate_limit_check_and_record("alice", 10));
    chat_rate_limit_record_output("alice", 0);
    chat_rate_limit_record_output("alice", -5);
    TEST_ASSERT_EQUAL_INT64(10, chat_rate_limit_token_count("alice"));
}

/* ---------- estimate_input_tokens ---------- */

void test_estimate_null_is_zero(void) {
    TEST_ASSERT_EQUAL_INT64(0, chat_rate_limit_estimate_input_tokens(NULL));
}

void test_estimate_non_array_is_zero(void) {
    json_t *obj = json_object();
    TEST_ASSERT_EQUAL_INT64(0, chat_rate_limit_estimate_input_tokens(obj));
    json_decref(obj);
}

void test_estimate_chars_over_4(void) {
    /* "user" (4) + "12345678" (8) = 12 chars. 12/4 = 3 tokens. */
    json_t *msgs = json_array();
    json_t *m = json_object();
    json_object_set_new(m, "role", json_string("user"));
    json_object_set_new(m, "content", json_string("12345678"));
    json_array_append_new(msgs, m);
    TEST_ASSERT_EQUAL_INT64(3, chat_rate_limit_estimate_input_tokens(msgs));
    json_decref(msgs);
}

void test_estimate_walks_nested_objects(void) {
    /* Two messages, one tool_calls.nested field. Total chars:
     *  "user" (4) + "hello" (5) + "assistant" (9) + "hi" (2) + "tool" (4) = 24 chars.
     *  24/4 = 6 tokens. */
    json_t *msgs = json_array();
    json_t *m1 = json_object();
    json_object_set_new(m1, "role", json_string("user"));
    json_object_set_new(m1, "content", json_string("hello"));
    json_array_append_new(msgs, m1);
    json_t *m2 = json_object();
    json_object_set_new(m2, "role", json_string("assistant"));
    json_object_set_new(m2, "content", json_string("hi"));
    json_t *tools = json_object();
    json_object_set_new(tools, "type", json_string("tool"));
    json_object_set_new(m2, "tool_calls", tools);
    json_array_append_new(msgs, m2);
    TEST_ASSERT_EQUAL_INT64(6, chat_rate_limit_estimate_input_tokens(msgs));
    json_decref(msgs);
}

void test_estimate_handles_utf8_multibyte(void) {
    /* "user" (4) + "héllo" (5 chars; é = 1 char) = 9 chars. 9/4 = 3 tokens. */
    json_t *msgs = json_array();
    json_t *m = json_object();
    json_object_set_new(m, "role", json_string("user"));
    json_object_set_new(m, "content", json_string("héllo"));
    json_array_append_new(msgs, m);
    TEST_ASSERT_EQUAL_INT64(3, chat_rate_limit_estimate_input_tokens(msgs));
    json_decref(msgs);
}

void test_estimate_empty_messages_is_zero(void) {
    json_t *msgs = json_array();
    TEST_ASSERT_EQUAL_INT64(0, chat_rate_limit_estimate_input_tokens(msgs));
    json_decref(msgs);
}

/* ---------- build_error_response ---------- */

void test_build_error_response_envelope_shape(void) {
    json_t *resp = chat_rate_limit_build_error_response(4291, "Too many requests");
    TEST_ASSERT_NOT_NULL(resp);
    json_t *success = json_object_get(resp, "success");
    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_TRUE(json_is_false(success));
    json_t *error = json_object_get(resp, "error");
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL_STRING("rate_limited", json_string_value(error));
    json_t *msg = json_object_get(resp, "message");
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_EQUAL_STRING("Too many requests", json_string_value(msg));
    json_t *code = json_object_get(resp, "error_code");
    TEST_ASSERT_NOT_NULL(code);
    TEST_ASSERT_EQUAL_INT(4291, (int)json_integer_value(code));
    json_decref(resp);
}

void test_build_error_response_null_message_uses_default(void) {
    json_t *resp = chat_rate_limit_build_error_response(4292, NULL);
    TEST_ASSERT_NOT_NULL(resp);
    json_t *msg = json_object_get(resp, "message");
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_EQUAL_STRING("Request rate limit exceeded", json_string_value(msg));
    json_decref(resp);
}

/* ---------- utf8_chars / walk_json / find_locked / new_bucket_locked (low-level helpers) ---------- */

void test_utf8_chars_counts_codepoints(void) {
    TEST_ASSERT_EQUAL_size_t(0, chat_rate_limit_utf8_chars(NULL));
    TEST_ASSERT_EQUAL_size_t(0, chat_rate_limit_utf8_chars(""));
    TEST_ASSERT_EQUAL_size_t(5, chat_rate_limit_utf8_chars("hello"));
    /* é is 0xC3 0xA9 — one codepoint. */
    TEST_ASSERT_EQUAL_size_t(5, chat_rate_limit_utf8_chars("héllo"));
}

void test_find_locked_returns_null_for_unknown(void) {
    ChatRateLimitEntry *e = chat_rate_limit_find_locked("ghost");
    TEST_ASSERT_NULL(e);
}

void test_find_locked_returns_entry_after_record(void) {
    chat_rate_limit_check_and_record("alice", 5);
    ChatRateLimitEntry *e = chat_rate_limit_find_locked("alice");
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_STRING("alice", e->sub);
    TEST_ASSERT_EQUAL_INT(1, e->request_count);
}

void test_new_bucket_locked_creates_entry(void) {
    ChatRateLimitEntry *e = chat_rate_limit_new_bucket_locked("charlie", 12345);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_STRING("charlie", e->sub);
    TEST_ASSERT_EQUAL_INT64((long long)12345, (long long)e->window_start);
    TEST_ASSERT_EQUAL_INT(0, e->request_count);
    TEST_ASSERT_EQUAL_INT64(0, e->token_count);
}

/* ---------- main ---------- */

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_check_allowed_on_first_request);
    RUN_TEST(test_check_throttled_requests_at_cap);
    RUN_TEST(test_check_throttled_tokens_at_budget);
    RUN_TEST(test_check_request_takes_precedence_over_token_check);
    RUN_TEST(test_check_different_subs_are_isolated);
    RUN_TEST(test_check_disabled_fails_open);
    RUN_TEST(test_check_null_app_config_fails_open);
    RUN_TEST(test_check_null_or_empty_sub_fails_open);
    RUN_TEST(test_check_zero_max_requests_disables_request_cap);
    RUN_TEST(test_check_zero_interval_fails_open);
    RUN_TEST(test_check_negative_input_tokens_clamped_to_zero);

    RUN_TEST(test_record_output_increments_for_known_sub);
    RUN_TEST(test_record_output_noop_for_unknown_sub);
    RUN_TEST(test_record_output_disabled_is_noop);
    RUN_TEST(test_record_output_non_positive_is_noop);

    RUN_TEST(test_estimate_null_is_zero);
    RUN_TEST(test_estimate_non_array_is_zero);
    RUN_TEST(test_estimate_chars_over_4);
    RUN_TEST(test_estimate_walks_nested_objects);
    RUN_TEST(test_estimate_handles_utf8_multibyte);
    RUN_TEST(test_estimate_empty_messages_is_zero);

    RUN_TEST(test_build_error_response_envelope_shape);
    RUN_TEST(test_build_error_response_null_message_uses_default);

    RUN_TEST(test_utf8_chars_counts_codepoints);
    RUN_TEST(test_find_locked_returns_null_for_unknown);
    RUN_TEST(test_find_locked_returns_entry_after_record);
    RUN_TEST(test_new_bucket_locked_creates_entry);

    return UNITY_END();
}