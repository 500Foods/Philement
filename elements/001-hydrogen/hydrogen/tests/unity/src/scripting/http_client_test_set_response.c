/*
 * Unity Test File: http_client_test_set_response.c
 *
 * Tests the Phase 17 test-injection-seam management functions in
 * src/scripting/http_client.c:
 *   - scripting_http_test_set_response (incl. the queue-full drop path)
 *   - scripting_http_test_clear_responses
 *   - scripting_http_test_get_consumed_count
 *
 * These functions never touch the network, so they are safe to exercise
 * directly. They manage a module-global FIFO of canned responses that is
 * consumed by try_test_injection / scripting_http_get/_post.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <stdlib.h>

#include <src/scripting/http_client.h>
#include <src/api/auth/oidc_rp/oidc_rp_http.h>

// Forward declarations
void test_set_response_stores_fixture_and_is_consumable(void);
void test_set_response_with_empty_substring_is_never_matched(void);
void test_clear_responses_empties_queue_and_resets_counter(void);
void test_consumed_count_tracks_consuptions(void);
void test_set_response_queue_full_drops_oldest(void);

void setUp(void) {
    scripting_http_test_clear_responses();
    app_config = NULL;
}

void tearDown(void) {
    scripting_http_test_clear_responses();
    app_config = NULL;
}

// A fixture set with a substring can be consumed via try_test_injection.
void test_set_response_stores_fixture_and_is_consumable(void) {
    scripting_http_test_set_response("example.com", 200, "OK");

    struct OidcRpHttpResponse* resp = try_test_injection("http://example.com/x");
    TEST_ASSERT_NOT_NULL(resp);
    TEST_ASSERT_EQUAL(200, resp->http_status);
    TEST_ASSERT_NOT_NULL(resp->body);
    TEST_ASSERT_EQUAL_STRING("OK", resp->body);

    oidc_rp_http_response_free(resp);
}

// set_response with an empty/NULL substring stores a fixture whose
// url_substring is NULL; try_test_injection must skip such fixtures and
// never match (covers the NULL-substring branch in try_test_injection).
void test_set_response_with_empty_substring_is_never_matched(void) {
    scripting_http_test_set_response("", 200, "OK");
    scripting_http_test_set_response(NULL, 500, "ignored");

    // Any URL should be rejected because no usable substring matches.
    struct OidcRpHttpResponse* resp = try_test_injection("http://anything/");
    TEST_ASSERT_NULL(resp);

    // No fixture was consumed.
    TEST_ASSERT_EQUAL(0, scripting_http_test_get_consumed_count());
}

// clear_responses resets the queue and the consumed counter.
void test_clear_responses_empties_queue_and_resets_counter(void) {
    scripting_http_test_set_response("a", 200, "1");
    scripting_http_test_set_response("b", 200, "2");

    // Consume one so the counter is non-zero before clearing.
    struct OidcRpHttpResponse* resp = try_test_injection("http://a/");
    TEST_ASSERT_NOT_NULL(resp);
    oidc_rp_http_response_free(resp);
    TEST_ASSERT_EQUAL(1, scripting_http_test_get_consumed_count());

    scripting_http_test_clear_responses();

    TEST_ASSERT_EQUAL(0, scripting_http_test_get_consumed_count());
    // After clearing, even a previously-set substring no longer matches.
    TEST_ASSERT_NULL(try_test_injection("http://a/"));
    TEST_ASSERT_NULL(try_test_injection("http://b/"));
}

// The consumed counter increments once per fixture consumed.
void test_consumed_count_tracks_consuptions(void) {
    scripting_http_test_set_response("a", 200, "1");
    scripting_http_test_set_response("b", 200, "2");

    struct OidcRpHttpResponse* r1 = try_test_injection("http://a/");
    struct OidcRpHttpResponse* r2 = try_test_injection("http://b/");

    TEST_ASSERT_NOT_NULL(r1);
    TEST_ASSERT_NOT_NULL(r2);
    TEST_ASSERT_EQUAL(2, scripting_http_test_get_consumed_count());

    oidc_rp_http_response_free(r1);
    oidc_rp_http_response_free(r2);
}

// When more than SCRIPTING_HTTP_TEST_QUEUE_CAP (16) fixtures are queued,
// the oldest entry is dropped (lines 56-61). Setting 17 fixtures should
// drop the first and keep the 17th.
void test_set_response_queue_full_drops_oldest(void) {
    for (int i = 0; i < 17; i++) {
        char sub[32];
        (void)snprintf(sub, sizeof(sub), "url%d", i);
        scripting_http_test_set_response(sub, 200, "x");
    }

    // The very first fixture ("url0") should have been dropped.
    TEST_ASSERT_NULL(try_test_injection("http://url0/"));

    // The most recently added fixture ("url16") must still be present.
    struct OidcRpHttpResponse* resp = try_test_injection("http://url16/");
    TEST_ASSERT_NOT_NULL(resp);
    oidc_rp_http_response_free(resp);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_set_response_stores_fixture_and_is_consumable);
    RUN_TEST(test_set_response_with_empty_substring_is_never_matched);
    RUN_TEST(test_clear_responses_empties_queue_and_resets_counter);
    RUN_TEST(test_consumed_count_tracks_consuptions);
    RUN_TEST(test_set_response_queue_full_drops_oldest);

    return UNITY_END();
}
