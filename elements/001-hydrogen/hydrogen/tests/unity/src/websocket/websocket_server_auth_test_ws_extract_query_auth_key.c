/*
 * Unity Test File: ws_extract_query_auth_key
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/websocket/websocket_server_internal.h>

bool ws_extract_query_auth_key(struct lws *wsi, char *out, size_t out_len);

void test_extract_null_wsi(void);
void test_extract_null_out(void);
void test_extract_short_out_len(void);
void test_extract_uri_args_only(void);
void test_extract_get_uri_with_query(void);
void test_extract_uri_args_preferred_over_get_uri(void);
void test_extract_missing_key(void);
void test_extract_empty_key(void);
void test_extract_empty_key_with_extra_params(void);
void test_extract_extra_query_params(void);
void test_extract_key_as_second_param(void);
void test_extract_named_param_ignores_notkey(void);
void test_extract_percent_encoding(void);
void test_extract_percent_encoding_uppercase(void);
void test_extract_invalid_percent_keeps_literal(void);
void test_extract_embedded_null_percent_fails(void);
void test_extract_truncated_out_fails(void);
void test_extract_get_uri_path_only(void);

void setUp(void) {
    mock_lws_reset_all();
}

void tearDown(void) {
    mock_lws_reset_all();
}

void test_extract_null_wsi(void) {
    char out[64];
    memset(out, 'X', sizeof(out));
    TEST_ASSERT_FALSE(ws_extract_query_auth_key(NULL, out, sizeof(out)));
    TEST_ASSERT_EQUAL_CHAR('\0', out[0]);
}

void test_extract_null_out(void) {
    struct lws *wsi = (struct lws *)0x1;
    TEST_ASSERT_FALSE(ws_extract_query_auth_key(wsi, NULL, 64));
}

void test_extract_short_out_len(void) {
    char out[1] = {'X'};
    struct lws *wsi = (struct lws *)0x1;
    TEST_ASSERT_FALSE(ws_extract_query_auth_key(wsi, out, 1));
}

void test_extract_uri_args_only(void) {
    char out[64];
    struct lws *wsi = (struct lws *)0x1;

    mock_lws_set_uri_data("/wss");
    mock_lws_set_uri_args("key=test_key_123");

    TEST_ASSERT_TRUE(ws_extract_query_auth_key(wsi, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("test_key_123", out);
}

void test_extract_get_uri_with_query(void) {
    char out[64];
    struct lws *wsi = (struct lws *)0x1;

    mock_lws_set_uri_data("/wss?key=from_get_uri");

    TEST_ASSERT_TRUE(ws_extract_query_auth_key(wsi, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("from_get_uri", out);
}

void test_extract_uri_args_preferred_over_get_uri(void) {
    char out[64];
    struct lws *wsi = (struct lws *)0x1;

    mock_lws_set_uri_data("/wss?key=from_get_uri");
    mock_lws_set_uri_args("key=from_uri_args");

    TEST_ASSERT_TRUE(ws_extract_query_auth_key(wsi, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("from_uri_args", out);
}

void test_extract_missing_key(void) {
    char out[64] = {'X', '\0'};
    struct lws *wsi = (struct lws *)0x1;

    mock_lws_set_uri_data("/wss");
    mock_lws_set_uri_args("other=value");

    TEST_ASSERT_FALSE(ws_extract_query_auth_key(wsi, out, sizeof(out)));
    TEST_ASSERT_EQUAL_CHAR('\0', out[0]);
}

void test_extract_empty_key(void) {
    char out[64] = {'X', '\0'};
    struct lws *wsi = (struct lws *)0x1;

    mock_lws_set_uri_args("key=");

    TEST_ASSERT_FALSE(ws_extract_query_auth_key(wsi, out, sizeof(out)));
    TEST_ASSERT_EQUAL_CHAR('\0', out[0]);
}

void test_extract_empty_key_with_extra_params(void) {
    char out[64] = {'X', '\0'};
    struct lws *wsi = (struct lws *)0x1;

    mock_lws_set_uri_args("key=&other=value");
    mock_lws_set_uri_data("/wss?key=fallback");

    TEST_ASSERT_FALSE(ws_extract_query_auth_key(wsi, out, sizeof(out)));
    TEST_ASSERT_EQUAL_CHAR('\0', out[0]);
}

void test_extract_extra_query_params(void) {
    char out[64];
    struct lws *wsi = (struct lws *)0x1;

    mock_lws_set_uri_args("foo=bar&key=test_key_123&baz=1");

    TEST_ASSERT_TRUE(ws_extract_query_auth_key(wsi, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("test_key_123", out);
}

void test_extract_key_as_second_param(void) {
    char out[64];
    struct lws *wsi = (struct lws *)0x1;

    mock_lws_set_uri_data("/terminal/ws?other=value&key=second_param");

    TEST_ASSERT_TRUE(ws_extract_query_auth_key(wsi, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("second_param", out);
}

void test_extract_named_param_ignores_notkey(void) {
    char out[64] = {'X', '\0'};
    struct lws *wsi = (struct lws *)0x1;

    mock_lws_set_uri_args("notkey=test_key_123&foo_key=test_key_123");

    TEST_ASSERT_FALSE(ws_extract_query_auth_key(wsi, out, sizeof(out)));
    TEST_ASSERT_EQUAL_CHAR('\0', out[0]);
}

void test_extract_percent_encoding(void) {
    char out[64];
    struct lws *wsi = (struct lws *)0x1;

    mock_lws_set_uri_args("key=test%20key%20123");

    TEST_ASSERT_TRUE(ws_extract_query_auth_key(wsi, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("test key 123", out);
}

void test_extract_percent_encoding_uppercase(void) {
    char out[64];
    struct lws *wsi = (struct lws *)0x1;

    mock_lws_set_uri_args("key=ab%2Fcd");

    TEST_ASSERT_TRUE(ws_extract_query_auth_key(wsi, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("ab/cd", out);
}

void test_extract_invalid_percent_keeps_literal(void) {
    char out[64];
    struct lws *wsi = (struct lws *)0x1;

    mock_lws_set_uri_args("key=test%XXz");

    TEST_ASSERT_TRUE(ws_extract_query_auth_key(wsi, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("test%XXz", out);
}

void test_extract_embedded_null_percent_fails(void) {
    char out[64] = {'X', '\0'};
    struct lws *wsi = (struct lws *)0x1;

    mock_lws_set_uri_args("key=ab%00cd");

    TEST_ASSERT_FALSE(ws_extract_query_auth_key(wsi, out, sizeof(out)));
    TEST_ASSERT_EQUAL_CHAR('\0', out[0]);
}

void test_extract_truncated_out_fails(void) {
    char out[8];
    struct lws *wsi = (struct lws *)0x1;

    mock_lws_set_uri_args("key=abcdefghijk");

    TEST_ASSERT_FALSE(ws_extract_query_auth_key(wsi, out, sizeof(out)));
    TEST_ASSERT_EQUAL_CHAR('\0', out[0]);
}

void test_extract_get_uri_path_only(void) {
    char out[64] = {'X', '\0'};
    struct lws *wsi = (struct lws *)0x1;

    mock_lws_set_uri_data("/wss");

    TEST_ASSERT_FALSE(ws_extract_query_auth_key(wsi, out, sizeof(out)));
    TEST_ASSERT_EQUAL_CHAR('\0', out[0]);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_extract_null_wsi);
    RUN_TEST(test_extract_null_out);
    RUN_TEST(test_extract_short_out_len);
    RUN_TEST(test_extract_uri_args_only);
    RUN_TEST(test_extract_get_uri_with_query);
    RUN_TEST(test_extract_uri_args_preferred_over_get_uri);
    RUN_TEST(test_extract_missing_key);
    RUN_TEST(test_extract_empty_key);
    RUN_TEST(test_extract_empty_key_with_extra_params);
    RUN_TEST(test_extract_extra_query_params);
    RUN_TEST(test_extract_key_as_second_param);
    RUN_TEST(test_extract_named_param_ignores_notkey);
    RUN_TEST(test_extract_percent_encoding);
    RUN_TEST(test_extract_percent_encoding_uppercase);
    RUN_TEST(test_extract_invalid_percent_keeps_literal);
    RUN_TEST(test_extract_embedded_null_percent_fails);
    RUN_TEST(test_extract_truncated_out_fails);
    RUN_TEST(test_extract_get_uri_path_only);

    return UNITY_END();
}
