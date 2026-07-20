/*
 * Unity Test File: http_client_test_post.c
 *
 * Tests scripting_http_post in src/scripting/http_client.c:
 *   - NULL url (with and without headers) returns an error response and
 *     frees any caller-supplied headers (lines 188-193)
 *   - a URL matching a queued test fixture returns the canned response
 *     (no network) and frees the headers when present (lines 199-200)
 *
 * As with the get tests, the real network path is avoided everywhere by
 * passing a NULL url or by registering a matching fixture.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <stdlib.h>

#include <curl/curl.h>

#include <src/scripting/http_client.h>
#include <src/api/auth/oidc_rp/oidc_rp_http.h>

// Forward declarations
void test_http_post_null_url_no_headers_returns_error(void);
void test_http_post_null_url_with_headers_frees_them(void);
void test_http_post_fixture_match_returns_canned(void);
void test_http_post_fixture_match_with_headers_frees_them(void);

void setUp(void) {
    scripting_http_test_clear_responses();
    app_config = NULL;
}

void tearDown(void) {
    scripting_http_test_clear_responses();
    app_config = NULL;
}

// NULL url with no headers: builds an error response, no network.
void test_http_post_null_url_no_headers_returns_error(void) {
    struct OidcRpHttpResponse* resp =
        scripting_http_post(NULL, "body", "application/json", NULL, 10, true);
    TEST_ASSERT_NOT_NULL(resp);
    TEST_ASSERT_EQUAL(0, resp->http_status);
    TEST_ASSERT_NOT_NULL(resp->error_message);
    TEST_ASSERT_EQUAL_STRING("URL is NULL or empty", resp->error_message);
    oidc_rp_http_response_free(resp);
}

// NULL url with a real headers slist: the slist is freed and an error
// response returned (exercises line 188, the headers guard).
void test_http_post_null_url_with_headers_frees_them(void) {
    struct curl_slist* headers = curl_slist_append(NULL, "X-Test: 1");
    TEST_ASSERT_NOT_NULL(headers);

    struct OidcRpHttpResponse* resp =
        scripting_http_post(NULL, "body", "application/json", headers, 10, true);
    TEST_ASSERT_NOT_NULL(resp);
    TEST_ASSERT_NOT_NULL(resp->error_message);
    TEST_ASSERT_EQUAL_STRING("URL is NULL or empty", resp->error_message);
    oidc_rp_http_response_free(resp);
}

// A URL that matches a queued fixture returns the canned response without
// a network call.
void test_http_post_fixture_match_returns_canned(void) {
    scripting_http_test_set_response("api.example.com", 201, "created");

    struct OidcRpHttpResponse* resp = scripting_http_post(
        "http://api.example.com/items", "{\"name\":\"x\"}",
        "application/json", NULL, 5, false);
    TEST_ASSERT_NOT_NULL(resp);
    TEST_ASSERT_EQUAL(201, resp->http_status);
    TEST_ASSERT_NOT_NULL(resp->body);
    TEST_ASSERT_EQUAL_STRING("created", resp->body);
    TEST_ASSERT_EQUAL(1, scripting_http_test_get_consumed_count());
    oidc_rp_http_response_free(resp);
}

// A URL matching a fixture while headers are supplied: the fixture wins
// and the headers are freed (lines 199-200, headers guard true branch).
void test_http_post_fixture_match_with_headers_frees_them(void) {
    scripting_http_test_set_response("api.example.com", 202, "accepted");

    struct curl_slist* headers = curl_slist_append(NULL, "Authorization: Bearer x");
    TEST_ASSERT_NOT_NULL(headers);

    struct OidcRpHttpResponse* resp = scripting_http_post(
        "http://api.example.com/items", "{\"name\":\"x\"}",
        "application/json", headers, 5, false);
    TEST_ASSERT_NOT_NULL(resp);
    TEST_ASSERT_EQUAL(202, resp->http_status);
    TEST_ASSERT_EQUAL_STRING("accepted", resp->body);
    TEST_ASSERT_EQUAL(1, scripting_http_test_get_consumed_count());
    oidc_rp_http_response_free(resp);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_http_post_null_url_no_headers_returns_error);
    RUN_TEST(test_http_post_null_url_with_headers_frees_them);
    RUN_TEST(test_http_post_fixture_match_returns_canned);
    RUN_TEST(test_http_post_fixture_match_with_headers_frees_them);

    return UNITY_END();
}
