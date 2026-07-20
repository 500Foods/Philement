/*
 * Unity Test File: http_client_test_try_injection.c
 *
 * Tests try_test_injection in src/scripting/http_client.c. This is the
 * worker behind the scripting HTTP test seam: it searches the FIFO of
 * canned fixtures for one whose substring matches the URL and returns a
 * heap-allocated canned OidcRpHttpResponse. It never touches the network.
 *
 * Focus: the parameter-validation and matching branches that the
 * higher-level get/post tests do not isolate well (NULL url, NULL
 * substring fixture, NULL body fixture, no-match, consumed-skip).
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <stdlib.h>

#include <src/scripting/http_client.h>
#include <src/api/auth/oidc_rp/oidc_rp_http.h>

// Forward declarations
void test_try_injection_null_url_returns_null(void);
void test_try_injection_null_substring_fixture_skipped(void);
void test_try_injection_null_body_fixture_returns_null_body(void);
void test_try_injection_no_match_returns_null(void);
void test_try_injection_consumed_fixture_skipped(void);
void test_try_injection_matches_oldest_unconsumed(void);

void setUp(void) {
    scripting_http_test_clear_responses();
    app_config = NULL;
}

void tearDown(void) {
    scripting_http_test_clear_responses();
    app_config = NULL;
}

// A NULL url is rejected immediately (line 101).
void test_try_injection_null_url_returns_null(void) {
    scripting_http_test_set_response("example.com", 200, "OK");
    TEST_ASSERT_NULL(try_test_injection(NULL));
}

// A fixture with a NULL/empty substring can never match (line 107).
void test_try_injection_null_substring_fixture_skipped(void) {
    scripting_http_test_set_response(NULL, 200, "OK");
    TEST_ASSERT_NULL(try_test_injection("http://example.com/"));
    // The unusable fixture must not be counted as consumed.
    TEST_ASSERT_EQUAL(0, scripting_http_test_get_consumed_count());
}

// A fixture with a NULL body yields a response whose body is NULL
// (line 112 branch that skips strdup).
void test_try_injection_null_body_fixture_returns_null_body(void) {
    scripting_http_test_set_response("example.com", 204, NULL);

    struct OidcRpHttpResponse* resp = try_test_injection("http://example.com/");
    TEST_ASSERT_NOT_NULL(resp);
    TEST_ASSERT_EQUAL(204, resp->http_status);
    TEST_ASSERT_NULL(resp->body);
    TEST_ASSERT_EQUAL(1, scripting_http_test_get_consumed_count());

    oidc_rp_http_response_free(resp);
}

// A URL that matches no queued substring returns NULL.
void test_try_injection_no_match_returns_null(void) {
    scripting_http_test_set_response("example.com", 200, "OK");
    TEST_ASSERT_NULL(try_test_injection("http://other.net/"));
}

// A fixture already consumed is skipped in favour of the next match.
void test_try_injection_consumed_fixture_skipped(void) {
    scripting_http_test_set_response("shared", 200, "first");
    scripting_http_test_set_response("shared", 200, "second");

    struct OidcRpHttpResponse* r1 = try_test_injection("http://shared/");
    TEST_ASSERT_NOT_NULL(r1);
    TEST_ASSERT_EQUAL_STRING("first", r1->body);
    oidc_rp_http_response_free(r1);

    // Second call must consume the second fixture, not the first again.
    struct OidcRpHttpResponse* r2 = try_test_injection("http://shared/");
    TEST_ASSERT_NOT_NULL(r2);
    TEST_ASSERT_EQUAL_STRING("second", r2->body);
    oidc_rp_http_response_free(r2);

    TEST_ASSERT_EQUAL(2, scripting_http_test_get_consumed_count());
}

// When multiple queued fixtures exist, the oldest unconsumed one whose
// substring appears in the URL wins (FIFO ordering).
void test_try_injection_matches_oldest_unconsumed(void) {
    scripting_http_test_set_response("old.com", 200, "old");
    scripting_http_test_set_response("new.com", 200, "new");

    struct OidcRpHttpResponse* resp = try_test_injection("http://old.com/path");
    TEST_ASSERT_NOT_NULL(resp);
    TEST_ASSERT_EQUAL_STRING("old", resp->body);
    oidc_rp_http_response_free(resp);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_try_injection_null_url_returns_null);
    RUN_TEST(test_try_injection_null_substring_fixture_skipped);
    RUN_TEST(test_try_injection_null_body_fixture_returns_null_body);
    RUN_TEST(test_try_injection_no_match_returns_null);
    RUN_TEST(test_try_injection_consumed_fixture_skipped);
    RUN_TEST(test_try_injection_matches_oldest_unconsumed);

    return UNITY_END();
}
