/*
 * Unity Test File: OIDC RP HTTP — coverage gap closure
 *
 * Targets the lines in src/api/auth/oidc_rp/oidc_rp_http.c that the
 * existing oidc_rp_http_test_get.c + blackbox suite leave uncovered:
 *
 *   - header_list_append(): capacity growth (realloc) + malloc failure
 *     paths for the name/value copies.
 *   - write_callback(): body-cap-exceeded (returns 0), buffer growth
 *     (realloc), and realloc-failure (returns 0).
 *   - response_set_error(): freeing a previously-set error_message.
 *   - header_callback(): status line (no ':'), empty name/value, and
 *     the trailing-WS / CRLF trimming branches.
 *   - url_matches(): NULL/empty substring, NULL url, match, no-match.
 *   - preflight_request(): fixture-claimed fast path + scheme rejection.
 *   - header_list_append through the public *_with_headers_slist
 *     variants (Content-Type already in slist vs appended).
 *
 * The test seam (oidc_rp_http_test_set_response) keeps every test off
 * the real network. mock_system (malloc/realloc failure) is globally
 * active for the allocation-failure paths.
 *
 * NOTE: HeaderList / ResponseBuffer are private to oidc_rp_http.c.
 * Their layouts are simple and stable; we mirror them locally and cast
 * so we can drive the internal helpers directly.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/auth/oidc_rp/oidc_rp_http.h>

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#include <stdlib.h>
#include <string.h>

// Local mirrors of the module-private accumulator structs. Layout must
// match oidc_rp_http.c exactly (char*/size_t/size_t[/size_t]).
typedef struct LocalHeaderList {
    OidcRpHttpHeader *items;
    size_t count;
    size_t capacity;
} LocalHeaderList;

typedef struct LocalResponseBuffer {
    char *data;
    size_t size;
    size_t capacity;
    size_t max_body;
} LocalResponseBuffer;

void test_response_set_error_frees_prior_message(void);
void test_response_set_error_null_resp(void);
void test_url_matches_wildcard(void);
void test_url_matches_null_url(void);
void test_url_matches_substring(void);
void test_url_matches_no_match(void);
void test_header_list_append_basic(void);
void test_header_list_append_grows_capacity(void);
void test_header_list_append_alloc_failure(void);
void test_write_callback_appends(void);
void test_write_callback_grows_buffer(void);
void test_write_callback_cap_exceeded(void);
void test_write_callback_realloc_failure(void);
void test_header_callback_status_line_no_colon(void);
void test_header_callback_normal_header_trims_ws(void);
void test_header_callback_empty_value(void);
void test_header_callback_empty_name(void);
void test_preflight_claims_fixture(void);
void test_preflight_rejects_scheme(void);
void test_get_with_cap_and_timeout_uses_seam(void);
void test_get_with_headers_slist_null_headers(void);
void test_get_with_headers_slist_with_headers(void);
void test_post_with_headers_slist_content_type_in_slist(void);
void test_post_with_headers_slist_content_type_appended(void);
void test_post_with_headers_slist_null_body(void);
void test_test_seam_drops_oldest_when_queue_full(void);
void test_response_alloc_calloc_failure_get(void);
void test_response_alloc_calloc_failure_post(void);
void test_response_alloc_calloc_failure_get_slist(void);
void test_response_alloc_calloc_failure_post_slist(void);
void test_get_transport_error_connection_refused(void);
void test_post_transport_error_connection_refused(void);
void test_get_cap_transport_error_connection_refused(void);
void test_post_slist_transport_error_connection_refused(void);
void test_get_body_malloc_failure(void);
void test_post_body_malloc_failure(void);
void test_get_slist_body_malloc_failure(void);
void test_post_slist_body_malloc_failure(void);
void test_get_slist_calloc_failure_frees_headers(void);
void test_post_slist_calloc_failure_frees_headers(void);

void setUp(void) {
    oidc_rp_http_test_clear_responses();
    mock_system_reset_all();
}

void tearDown(void) {
    oidc_rp_http_test_clear_responses();
    mock_system_reset_all();
}

// ---------------------------------------------------------------------------
// response_set_error
// ---------------------------------------------------------------------------

void test_response_set_error_null_resp(void) {
    response_set_error(NULL, "ignored");  // must not crash
    TEST_PASS();
}

void test_response_set_error_frees_prior_message(void) {
    OidcRpHttpResponse *r = response_alloc();
    TEST_ASSERT_NOT_NULL(r);
    response_set_error(r, "first message");
    TEST_ASSERT_NOT_NULL(r->error_message);
    TEST_ASSERT_EQUAL_STRING("first message", r->error_message);
    // Second call must free the first allocation before strdup'ing again.
    response_set_error(r, "second message");
    TEST_ASSERT_EQUAL_STRING("second message", r->error_message);
    response_set_error(r, NULL);  // resets to NULL
    TEST_ASSERT_NULL(r->error_message);
    oidc_rp_http_response_free(r);
}

// ---------------------------------------------------------------------------
// url_matches
// ---------------------------------------------------------------------------

void test_url_matches_wildcard(void) {
    TEST_ASSERT_TRUE(url_matches("https://example.com/x", NULL));
    TEST_ASSERT_TRUE(url_matches("https://example.com/x", ""));
}

void test_url_matches_null_url(void) {
    TEST_ASSERT_FALSE(url_matches(NULL, "sub"));
}

void test_url_matches_substring(void) {
    TEST_ASSERT_TRUE(url_matches("https://example.com/jwks", "/jwks"));
}

void test_url_matches_no_match(void) {
    TEST_ASSERT_FALSE(url_matches("https://example.com/keys", "/jwks"));
}

// ---------------------------------------------------------------------------
// header_list_append
// ---------------------------------------------------------------------------

void test_header_list_append_basic(void) {
    LocalHeaderList list = {0};
    bool ok = header_list_append((HeaderList *)&list, "Content-Type", 12,
                                 "application/json", 16);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_size_t(1, list.count);
    TEST_ASSERT_EQUAL_STRING("Content-Type", list.items[0].name);
    TEST_ASSERT_EQUAL_STRING("application/json", list.items[0].value);
    for (size_t i = 0; i < list.count; i++) {
        free(list.items[i].name);
        free(list.items[i].value);
    }
    free(list.items);
}

void test_header_list_append_grows_capacity(void) {
    LocalHeaderList list = {0};
    char name[16];
    char val[16];
    for (int i = 0; i < 40; i++) {
        snprintf(name, sizeof(name), "H%d", i);
        snprintf(val, sizeof(val), "V%d", i);
        bool ok = header_list_append((HeaderList *)&list, name,
                                     strlen(name), val, strlen(val));
        TEST_ASSERT_TRUE(ok);
    }
    TEST_ASSERT_EQUAL_size_t(40, list.count);
    TEST_ASSERT_TRUE(list.capacity >= 40);
    for (size_t i = 0; i < list.count; i++) {
        free(list.items[i].name);
        free(list.items[i].value);
    }
    free(list.items);
}

void test_header_list_append_alloc_failure(void) {
    LocalHeaderList list = {0};
    // Force the name/value copy mallocs to fail.
    mock_system_set_malloc_failure(1);
    bool ok = header_list_append((HeaderList *)&list, "Name", 4, "Val", 3);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_EQUAL_size_t(0, list.count);
    for (size_t i = 0; i < list.count; i++) {
        free(list.items[i].name);
        free(list.items[i].value);
    }
    free(list.items);
}

// ---------------------------------------------------------------------------
// write_callback
// ---------------------------------------------------------------------------

void test_write_callback_appends(void) {
    LocalResponseBuffer buf = {0};
    buf.data = malloc(64);
    buf.capacity = 64;
    buf.max_body = 1024;
    const char *chunk = "hello";
    size_t got = write_callback(chunk, 1, 5, &buf);
    TEST_ASSERT_EQUAL_size_t(5, got);
    TEST_ASSERT_EQUAL_size_t(5, buf.size);
    TEST_ASSERT_EQUAL_STRING("hello", buf.data);
    free(buf.data);
}

void test_write_callback_grows_buffer(void) {
    LocalResponseBuffer buf = {0};
    buf.data = malloc(4);
    buf.capacity = 4;
    buf.max_body = 1024;
    // 4 bytes is too small for "hello" (5) + NUL; triggers realloc growth.
    const char *chunk = "hello world";
    size_t got = write_callback(chunk, 1, 11, &buf);
    TEST_ASSERT_EQUAL_size_t(11, got);
    TEST_ASSERT_EQUAL_size_t(11, buf.size);
    TEST_ASSERT_EQUAL_STRING("hello world", buf.data);
    TEST_ASSERT_TRUE(buf.capacity >= 12);
    free(buf.data);
}

void test_write_callback_cap_exceeded(void) {
    LocalResponseBuffer buf = {0};
    buf.data = malloc(16);
    buf.capacity = 16;
    buf.size = 0;
    buf.max_body = 4;  // cap smaller than the incoming chunk
    const char *chunk = "toobig";
    size_t got = write_callback(chunk, 1, 6, &buf);
    TEST_ASSERT_EQUAL_size_t(0, got);  // aborted
    free(buf.data);
}

void test_write_callback_realloc_failure(void) {
    LocalResponseBuffer buf = {0};
    buf.data = malloc(4);
    buf.capacity = 4;
    buf.max_body = 1024;
    mock_system_set_realloc_failure(1);
    const char *chunk = "hello world";
    size_t got = write_callback(chunk, 1, 11, &buf);
    TEST_ASSERT_EQUAL_size_t(0, got);  // realloc failure aborts
    free(buf.data);
}

// ---------------------------------------------------------------------------
// header_callback
// ---------------------------------------------------------------------------

void test_header_callback_status_line_no_colon(void) {
    LocalHeaderList list = {0};
    const char *line = "HTTP/1.1 200 OK\r\n";
    size_t got = header_callback(line, 1, strlen(line), &list);
    TEST_ASSERT_EQUAL_size_t(strlen(line), got);
    TEST_ASSERT_EQUAL_size_t(0, list.count);  // status line skipped
}

void test_header_callback_normal_header_trims_ws(void) {
    LocalHeaderList list = {0};
    const char *line = "Content-Type:   application/json  \r\n";
    size_t got = header_callback(line, 1, strlen(line), &list);
    TEST_ASSERT_EQUAL_size_t(strlen(line), got);
    TEST_ASSERT_EQUAL_size_t(1, list.count);
    TEST_ASSERT_EQUAL_STRING("Content-Type", list.items[0].name);
    TEST_ASSERT_EQUAL_STRING("application/json", list.items[0].value);
    for (size_t i = 0; i < list.count; i++) {
        free(list.items[i].name);
        free(list.items[i].value);
    }
    free(list.items);
}

void test_header_callback_empty_value(void) {
    LocalHeaderList list = {0};
    const char *line = "X-Empty:\r\n";
    size_t got = header_callback(line, 1, strlen(line), &list);
    TEST_ASSERT_EQUAL_size_t(strlen(line), got);
    TEST_ASSERT_EQUAL_size_t(0, list.count);  // empty value dropped
}

void test_header_callback_empty_name(void) {
    LocalHeaderList list = {0};
    // Colon immediately — zero-length name -> dropped.
    const char *line = ": value\r\n";
    size_t got = header_callback(line, 1, strlen(line), &list);
    TEST_ASSERT_EQUAL_size_t(strlen(line), got);
    TEST_ASSERT_EQUAL_size_t(0, list.count);
}

// ---------------------------------------------------------------------------
// preflight_request
// ---------------------------------------------------------------------------

void test_preflight_claims_fixture(void) {
    oidc_rp_http_test_set_response("/jwks", 200, "BODY");
    OidcRpHttpResponse *r = response_alloc();
    bool proceed = true;
    bool claimed = preflight_request(r, "https://idp/jwks", &proceed);
    TEST_ASSERT_TRUE(claimed);
    TEST_ASSERT_FALSE(proceed);
    TEST_ASSERT_EQUAL_INT(200, r->http_status);
    TEST_ASSERT_EQUAL_STRING("BODY", r->body);
    oidc_rp_http_response_free(r);
}

void test_preflight_rejects_scheme(void) {
    OidcRpHttpResponse *r = response_alloc();
    bool proceed = true;
    bool claimed = preflight_request(r, "ftp://nope", &proceed);
    TEST_ASSERT_FALSE(claimed);
    TEST_ASSERT_FALSE(proceed);
    TEST_ASSERT_NOT_NULL(r->error_message);
    oidc_rp_http_response_free(r);
}

// ---------------------------------------------------------------------------
// Public *_with_cap_and_timeout / *_with_headers_slist via the seam
// ---------------------------------------------------------------------------

void test_get_with_cap_and_timeout_uses_seam(void) {
    oidc_rp_http_test_set_response(NULL, 200, "cap-body");
    OidcRpHttpResponse *r =
        oidc_rp_http_get_with_cap_and_timeout("https://idp/x", true, NULL,
                                              2048, 5);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_INT(200, r->http_status);
    TEST_ASSERT_EQUAL_STRING("cap-body", r->body);
    oidc_rp_http_response_free(r);
}

void test_get_with_headers_slist_null_headers(void) {
    oidc_rp_http_test_set_response(NULL, 200, "slist-null");
    OidcRpHttpResponse *r =
        oidc_rp_http_get_with_headers_slist("https://idp/x", true, NULL,
                                            2048, 5);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_INT(200, r->http_status);
    TEST_ASSERT_EQUAL_STRING("slist-null", r->body);
    oidc_rp_http_response_free(r);
}

void test_get_with_headers_slist_with_headers(void) {
    oidc_rp_http_test_set_response(NULL, 200, "slist-ok");
    struct curl_slist *headers = curl_slist_append(NULL, "X-Custom: 1");
    OidcRpHttpResponse *r =
        oidc_rp_http_get_with_headers_slist("https://idp/x", true, headers,
                                            2048, 5);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_INT(200, r->http_status);
    TEST_ASSERT_EQUAL_STRING("slist-ok", r->body);
    // headers ownership transferred to + freed inside the callee; do NOT
    // free again.
    oidc_rp_http_response_free(r);
}

void test_post_with_headers_slist_content_type_in_slist(void) {
    oidc_rp_http_test_set_response(NULL, 200, "slist-ct");
    // Content-Type already present in the slist: must NOT be appended twice
    // (the helper only appends when absent).
    struct curl_slist *headers = curl_slist_append(NULL,
        "Content-Type: application/json");
    OidcRpHttpResponse *r =
        oidc_rp_http_post_with_headers_slist("https://idp/x", true,
            "grant=1", "application/json", headers, 2048, 5);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_INT(200, r->http_status);
    TEST_ASSERT_EQUAL_STRING("slist-ct", r->body);
    oidc_rp_http_response_free(r);
}

void test_post_with_headers_slist_content_type_appended(void) {
    oidc_rp_http_test_set_response(NULL, 201, "slist-append-ct");
    // Content-Type NOT in slist: helper should append it.
    struct curl_slist *headers = curl_slist_append(NULL, "X-Custom: z");
    OidcRpHttpResponse *r =
        oidc_rp_http_post_with_headers_slist("https://idp/x", true,
            "grant=1", "application/x-www-form-urlencoded", headers, 2048, 5);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_INT(201, r->http_status);
    TEST_ASSERT_EQUAL_STRING("slist-append-ct", r->body);
    oidc_rp_http_response_free(r);
}

void test_post_with_headers_slist_null_body(void) {
    oidc_rp_http_test_set_response(NULL, 204, "");
    struct curl_slist *headers = NULL;
    OidcRpHttpResponse *r =
        oidc_rp_http_post_with_headers_slist("https://idp/revoke", true,
            NULL, NULL, headers, 2048, 5);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_INT(204, r->http_status);
    TEST_ASSERT_EQUAL_STRING("", r->body);
    oidc_rp_http_response_free(r);
}

void test_test_seam_drops_oldest_when_queue_full(void) {
    // The fixture queue holds OIDC_RP_HTTP_TEST_QUEUE_CAP (8) entries.
    // Registering one more must evict the oldest (freeing its
    // url_substring + body) and still accept the new entry. We then
    // consume it via the seam.
    for (int i = 0; i < 9; i++) {
        char body[16];
        snprintf(body, sizeof(body), "b%d", i);
        oidc_rp_http_test_set_response(NULL, 200, body);
    }
    OidcRpHttpResponse *r =
        oidc_rp_http_get("https://idp/x", true, NULL);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_INT(200, r->http_status);
    // The first registered (b0) was evicted; the surviving entries are
    // b1..b8, consumed FIFO, so we get b1 here.
    TEST_ASSERT_EQUAL_STRING("b1", r->body);
    oidc_rp_http_response_free(r);
}

// ---------------------------------------------------------------------------
// Allocation-failure paths (response_alloc / calloc)
//
// Force calloc to fail on its first (and only) call so response_alloc()
// returns NULL. Each public entry point must then bail out with NULL
// instead of touching curl. These cover the `if (!resp) return NULL;`
// guards at the top of every wrapper.
// ---------------------------------------------------------------------------

void test_response_alloc_calloc_failure_get(void) {
    mock_system_set_calloc_failure(1);
    OidcRpHttpResponse *r = oidc_rp_http_get("https://idp/x", true, NULL);
    TEST_ASSERT_NULL(r);
}

void test_response_alloc_calloc_failure_post(void) {
    mock_system_set_calloc_failure(1);
    OidcRpHttpResponse *r = oidc_rp_http_post("https://idp/x", true,
        NULL, NULL, NULL, NULL);
    TEST_ASSERT_NULL(r);
}

void test_response_alloc_calloc_failure_get_slist(void) {
    mock_system_set_calloc_failure(1);
    OidcRpHttpResponse *r =
        oidc_rp_http_get_with_headers_slist("https://idp/x", true, NULL,
                                            0, 0);
    TEST_ASSERT_NULL(r);
}

void test_response_alloc_calloc_failure_post_slist(void) {
    mock_system_set_calloc_failure(1);
    OidcRpHttpResponse *r =
        oidc_rp_http_post_with_headers_slist("https://idp/x", true,
            NULL, NULL, NULL, 0, 0);
    TEST_ASSERT_NULL(r);
}

// ---------------------------------------------------------------------------
// Transport-error paths (real curl, refused connection)
//
// With NO fixture registered the request proceeds to libcurl. Pointing
// at a closed local port makes curl_easy_perform fail fast with a
// connection-refused error, exercising perform_and_finalize()'s error
// branch (status forced to 0, error_message set) and the per-wrapper
// cleanup (curl_slist_free_all + curl_easy_cleanup). A 1-second timeout
// keeps the attempt short and deterministic.
// ---------------------------------------------------------------------------

static void assert_transport_failure(OidcRpHttpResponse *r) {
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_INT(0, r->http_status);
    TEST_ASSERT_NULL(r->body);
    TEST_ASSERT_NOT_NULL(r->error_message);
    oidc_rp_http_response_free(r);
}

void test_get_transport_error_connection_refused(void) {
    OidcRpHttpResponse *r =
        oidc_rp_http_get("http://127.0.0.1:1/", true, NULL);
    assert_transport_failure(r);
}

void test_post_transport_error_connection_refused(void) {
    OidcRpHttpResponse *r = oidc_rp_http_post("http://127.0.0.1:1/", true,
        "grant=1", NULL, NULL, NULL);
    assert_transport_failure(r);
}

void test_get_cap_transport_error_connection_refused(void) {
    OidcRpHttpResponse *r =
        oidc_rp_http_get_with_cap_and_timeout("http://127.0.0.1:1/", true,
                                              NULL, 0, 1);
    assert_transport_failure(r);
}

void test_post_slist_transport_error_connection_refused(void) {
    OidcRpHttpResponse *r =
        oidc_rp_http_post_with_headers_slist("http://127.0.0.1:1/", true,
            "grant=1", NULL, NULL, 0, 1);
    assert_transport_failure(r);
}

// ---------------------------------------------------------------------------
// Body-buffer allocation failure
//
// With no fixture and a valid URL, the wrapper reaches
// `body.data = malloc(...)`. Forcing the 2nd malloc (response_alloc's
// calloc is #1) to fail makes that allocation return NULL, exercising
// the curl_easy_cleanup + "malloc failed" error path in each wrapper.
// ---------------------------------------------------------------------------

void test_get_body_malloc_failure(void) {
    mock_system_set_malloc_failure(2);
    OidcRpHttpResponse *r =
        oidc_rp_http_get("http://127.0.0.1:1/", true, NULL);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_NULL(r->body);
    TEST_ASSERT_NOT_NULL(r->error_message);
    TEST_ASSERT_TRUE(strstr(r->error_message, "malloc failed") != NULL);
    oidc_rp_http_response_free(r);
}

void test_post_body_malloc_failure(void) {
    mock_system_set_malloc_failure(2);
    OidcRpHttpResponse *r = oidc_rp_http_post("http://127.0.0.1:1/", true,
        NULL, NULL, NULL, NULL);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_NULL(r->body);
    TEST_ASSERT_NOT_NULL(r->error_message);
    TEST_ASSERT_TRUE(strstr(r->error_message, "malloc failed") != NULL);
    oidc_rp_http_response_free(r);
}

void test_get_slist_body_malloc_failure(void) {
    mock_system_set_malloc_failure(2);
    OidcRpHttpResponse *r =
        oidc_rp_http_get_with_headers_slist("http://127.0.0.1:1/", true,
                                            NULL, 0, 0);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_NULL(r->body);
    TEST_ASSERT_NOT_NULL(r->error_message);
    TEST_ASSERT_TRUE(strstr(r->error_message, "malloc failed") != NULL);
    oidc_rp_http_response_free(r);
}

void test_post_slist_body_malloc_failure(void) {
    mock_system_set_malloc_failure(2);
    OidcRpHttpResponse *r =
        oidc_rp_http_post_with_headers_slist("http://127.0.0.1:1/", true,
            NULL, NULL, NULL, 0, 0);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_NULL(r->body);
    TEST_ASSERT_NOT_NULL(r->error_message);
    TEST_ASSERT_TRUE(strstr(r->error_message, "malloc failed") != NULL);
    oidc_rp_http_response_free(r);
}

// When response_alloc() fails (calloc mock) while a caller-supplied
// headers slist was passed, the wrapper must free that slist before
// returning NULL (it took ownership). These cover the
// `if (headers) curl_slist_free_all(headers)` guards at the top of the
// slist variants.
void test_get_slist_calloc_failure_frees_headers(void) {
    mock_system_set_calloc_failure(1);
    struct curl_slist *headers = curl_slist_append(NULL, "X-Custom: 1");
    OidcRpHttpResponse *r =
        oidc_rp_http_get_with_headers_slist("https://idp/x", true,
                                            headers, 0, 0);
    TEST_ASSERT_NULL(r);
}

void test_post_slist_calloc_failure_frees_headers(void) {
    mock_system_set_calloc_failure(1);
    struct curl_slist *headers = curl_slist_append(NULL, "X-Custom: 1");
    OidcRpHttpResponse *r =
        oidc_rp_http_post_with_headers_slist("https://idp/x", true,
            NULL, NULL, headers, 0, 0);
    TEST_ASSERT_NULL(r);
}

// ---------------------------------------------------------------------------
// Test runner
// ---------------------------------------------------------------------------

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_response_set_error_null_resp);
    RUN_TEST(test_response_set_error_frees_prior_message);
    RUN_TEST(test_url_matches_wildcard);
    RUN_TEST(test_url_matches_null_url);
    RUN_TEST(test_url_matches_substring);
    RUN_TEST(test_url_matches_no_match);
    RUN_TEST(test_header_list_append_basic);
    RUN_TEST(test_header_list_append_grows_capacity);
    RUN_TEST(test_header_list_append_alloc_failure);
    RUN_TEST(test_write_callback_appends);
    RUN_TEST(test_write_callback_grows_buffer);
    RUN_TEST(test_write_callback_cap_exceeded);
    RUN_TEST(test_write_callback_realloc_failure);
    RUN_TEST(test_header_callback_status_line_no_colon);
    RUN_TEST(test_header_callback_normal_header_trims_ws);
    RUN_TEST(test_header_callback_empty_value);
    RUN_TEST(test_header_callback_empty_name);
    RUN_TEST(test_preflight_claims_fixture);
    RUN_TEST(test_preflight_rejects_scheme);
    RUN_TEST(test_get_with_cap_and_timeout_uses_seam);
    RUN_TEST(test_get_with_headers_slist_null_headers);
    RUN_TEST(test_get_with_headers_slist_with_headers);
    RUN_TEST(test_post_with_headers_slist_content_type_in_slist);
    RUN_TEST(test_post_with_headers_slist_content_type_appended);
    RUN_TEST(test_post_with_headers_slist_null_body);
    RUN_TEST(test_test_seam_drops_oldest_when_queue_full);
    RUN_TEST(test_response_alloc_calloc_failure_get);
    RUN_TEST(test_response_alloc_calloc_failure_post);
    RUN_TEST(test_response_alloc_calloc_failure_get_slist);
    RUN_TEST(test_response_alloc_calloc_failure_post_slist);
    RUN_TEST(test_get_transport_error_connection_refused);
    RUN_TEST(test_post_transport_error_connection_refused);
    RUN_TEST(test_get_cap_transport_error_connection_refused);
    RUN_TEST(test_post_slist_transport_error_connection_refused);
    RUN_TEST(test_get_body_malloc_failure);
    RUN_TEST(test_post_body_malloc_failure);
    RUN_TEST(test_get_slist_body_malloc_failure);
    RUN_TEST(test_post_slist_body_malloc_failure);
    RUN_TEST(test_get_slist_calloc_failure_frees_headers);
    RUN_TEST(test_post_slist_calloc_failure_frees_headers);
    return UNITY_END();
}
