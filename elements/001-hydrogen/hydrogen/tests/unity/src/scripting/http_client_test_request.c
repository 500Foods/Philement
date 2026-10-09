/*
 * Unity Test File: http_client_test_request.c
 *
 * Tests scripting_http_request and scripting_http_method_allowed:
 *   - an allowlisted CalDAV verb (PROPFIND) returns a canned body
 *   - CONNECT and TRACE are rejected and do not consume a fixture
 *   - a canned 207 and a canned 412 leave error_message unset
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <stdlib.h>

#include <curl/curl.h>

#include <src/scripting/http_client.h>
#include <src/api/auth/oidc_rp/oidc_rp_http.h>

void test_http_method_allowlist(void);
void test_http_request_propfind_returns_canned(void);
void test_http_request_connect_rejected_skips_fixture(void);
void test_http_request_trace_rejected_frees_headers(void);
void test_http_request_207_is_data(void);
void test_http_request_412_is_data(void);
void test_http_request_propfind_bad_scheme_reaches_helper(void);

void setUp(void) {
    scripting_http_test_clear_responses();
    app_config = NULL;
}

void tearDown(void) {
    scripting_http_test_clear_responses();
    app_config = NULL;
}

void test_http_method_allowlist(void) {
    TEST_ASSERT_TRUE(scripting_http_method_allowed("GET"));
    TEST_ASSERT_TRUE(scripting_http_method_allowed("POST"));
    TEST_ASSERT_TRUE(scripting_http_method_allowed("PUT"));
    TEST_ASSERT_TRUE(scripting_http_method_allowed("DELETE"));
    TEST_ASSERT_TRUE(scripting_http_method_allowed("PROPFIND"));
    TEST_ASSERT_TRUE(scripting_http_method_allowed("REPORT"));
    TEST_ASSERT_TRUE(scripting_http_method_allowed("MKCALENDAR"));
    TEST_ASSERT_TRUE(scripting_http_method_allowed("PROPPATCH"));
    TEST_ASSERT_FALSE(scripting_http_method_allowed("CONNECT"));
    TEST_ASSERT_FALSE(scripting_http_method_allowed("TRACE"));
    TEST_ASSERT_FALSE(scripting_http_method_allowed("get"));
    TEST_ASSERT_FALSE(scripting_http_method_allowed("PATCH"));
    TEST_ASSERT_FALSE(scripting_http_method_allowed(NULL));
    TEST_ASSERT_FALSE(scripting_http_method_allowed(""));
}

void test_http_request_propfind_returns_canned(void) {
    scripting_http_test_set_response("cal.example", 207, "multi-status");

    struct OidcRpHttpResponse* resp = scripting_http_request(
        "PROPFIND", "http://cal.example/dav", "<propfind/>",
        "application/xml", NULL, 5, true);
    TEST_ASSERT_NOT_NULL(resp);
    TEST_ASSERT_EQUAL(207, resp->http_status);
    TEST_ASSERT_NOT_NULL(resp->body);
    TEST_ASSERT_EQUAL_STRING("multi-status", resp->body);
    TEST_ASSERT_NULL(resp->error_message);
    TEST_ASSERT_EQUAL(1, scripting_http_test_get_consumed_count());
    oidc_rp_http_response_free(resp);
}

void test_http_request_connect_rejected_skips_fixture(void) {
    scripting_http_test_set_response("cal.example", 200, "should-not-run");

    struct OidcRpHttpResponse* resp = scripting_http_request(
        "CONNECT", "http://cal.example/dav", NULL, NULL, NULL, 5, true);
    TEST_ASSERT_NOT_NULL(resp);
    TEST_ASSERT_NOT_NULL(resp->error_message);
    TEST_ASSERT_EQUAL_STRING("HTTP method is not allowed", resp->error_message);
    TEST_ASSERT_EQUAL(0, scripting_http_test_get_consumed_count());
    oidc_rp_http_response_free(resp);
}

void test_http_request_trace_rejected_frees_headers(void) {
    struct curl_slist* headers = curl_slist_append(NULL, "X-Test: 1");
    TEST_ASSERT_NOT_NULL(headers);

    struct OidcRpHttpResponse* resp = scripting_http_request(
        "TRACE", "http://cal.example/dav", NULL, NULL, headers, 5, true);
    TEST_ASSERT_NOT_NULL(resp);
    TEST_ASSERT_EQUAL_STRING("HTTP method is not allowed", resp->error_message);
    oidc_rp_http_response_free(resp);
}

void test_http_request_207_is_data(void) {
    scripting_http_test_set_response("cal.example", 207, "<multistatus/>");

    struct OidcRpHttpResponse* resp = scripting_http_request(
        "REPORT", "http://cal.example/dav", "<calendar-query/>",
        "application/xml", NULL, 5, true);
    TEST_ASSERT_NOT_NULL(resp);
    TEST_ASSERT_EQUAL(207, resp->http_status);
    TEST_ASSERT_EQUAL_STRING("<multistatus/>", resp->body);
    TEST_ASSERT_NULL(resp->error_message);
    oidc_rp_http_response_free(resp);
}

void test_http_request_412_is_data(void) {
    scripting_http_test_set_response("cal.example", 412, "precondition");

    struct OidcRpHttpResponse* resp = scripting_http_request(
        "PUT", "http://cal.example/dav/event.ics", "BEGIN:VCALENDAR",
        "text/calendar", NULL, 5, true);
    TEST_ASSERT_NOT_NULL(resp);
    TEST_ASSERT_EQUAL(412, resp->http_status);
    TEST_ASSERT_EQUAL_STRING("precondition", resp->body);
    TEST_ASSERT_NULL(resp->error_message);
    oidc_rp_http_response_free(resp);
}

void test_http_request_propfind_bad_scheme_reaches_helper(void) {
    struct OidcRpHttpResponse* resp = scripting_http_request(
        "PROPFIND", "not-a-url", NULL, NULL, NULL, 5, true);
    TEST_ASSERT_NOT_NULL(resp);
    TEST_ASSERT_NOT_NULL(resp->error_message);
    TEST_ASSERT_EQUAL_STRING("URL scheme must be http or https", resp->error_message);
    TEST_ASSERT_EQUAL(0, scripting_http_test_get_consumed_count());
    oidc_rp_http_response_free(resp);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_http_method_allowlist);
    RUN_TEST(test_http_request_propfind_returns_canned);
    RUN_TEST(test_http_request_connect_rejected_skips_fixture);
    RUN_TEST(test_http_request_trace_rejected_frees_headers);
    RUN_TEST(test_http_request_207_is_data);
    RUN_TEST(test_http_request_412_is_data);
    RUN_TEST(test_http_request_propfind_bad_scheme_reaches_helper);

    return UNITY_END();
}
