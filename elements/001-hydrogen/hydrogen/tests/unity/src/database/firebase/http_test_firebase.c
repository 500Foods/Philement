/*
 * Unity tests for the Firebase HTTP seam.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebase/http.h>
#include <src/database/firebase/types.h>

void test_firebase_http_response_alloc_free(void);
void test_firebase_http_status_is_healthy(void);
void test_firebase_http_build_documents_url_emulator(void);
void test_firebase_http_build_documents_url_nulls(void);
void test_firebase_http_get_null_url(void);
void test_firebase_http_get_fixture_200(void);
void test_firebase_http_get_fixture_500(void);
void test_firebase_http_get_invalid_scheme(void);
void test_firebase_http_url_matches(void);
void test_firebase_http_write_callback_grows(void);
void test_firebase_http_xferinfo_abort(void);
void test_firebase_http_apply_curl_opts_null(void);
void test_firebase_http_get_transport_failure(void);
void test_firebase_http_test_queue_overflow_and_clear(void);
void test_firebase_http_build_documents_url_defaults(void);

void setUp(void) {
    firebase_http_test_clear_responses();
}

void tearDown(void) {
    firebase_http_test_clear_responses();
}

void test_firebase_http_response_alloc_free(void) {
    FirebaseHttpResponse* resp = firebase_http_response_alloc();
    TEST_ASSERT_NOT_NULL(resp);
    firebase_http_response_set_error(resp, "boom");
    TEST_ASSERT_EQUAL_STRING("boom", resp->error_message);
    firebase_http_response_free(resp);
    firebase_http_response_free(NULL);
}

void test_firebase_http_status_is_healthy(void) {
    TEST_ASSERT_TRUE(firebase_http_status_is_healthy(200));
    TEST_ASSERT_TRUE(firebase_http_status_is_healthy(204));
    TEST_ASSERT_FALSE(firebase_http_status_is_healthy(0));
    TEST_ASSERT_FALSE(firebase_http_status_is_healthy(404));
    TEST_ASSERT_FALSE(firebase_http_status_is_healthy(500));
}

void test_firebase_http_build_documents_url_emulator(void) {
    char* url = firebase_http_build_documents_url("127.0.0.1", 8080, "hydrodemo", "(default)", true);
    TEST_ASSERT_NOT_NULL(url);
    TEST_ASSERT_EQUAL_STRING(
        "http://127.0.0.1:8080/v1/projects/hydrodemo/databases/(default)/documents/", url);
    free(url);
}

void test_firebase_http_build_documents_url_nulls(void) {
    TEST_ASSERT_NULL(firebase_http_build_documents_url(NULL, 8080, "hydrodemo", "(default)", true));
    TEST_ASSERT_NULL(firebase_http_build_documents_url("127.0.0.1", 8080, NULL, "(default)", true));
}

void test_firebase_http_get_null_url(void) {
    FirebaseHttpResponse* resp = firebase_http_get(NULL, NULL, false, NULL, NULL);
    TEST_ASSERT_NOT_NULL(resp);
    TEST_ASSERT_EQUAL(0, resp->http_status);
    TEST_ASSERT_NOT_NULL(resp->error_message);
    firebase_http_response_free(resp);
}

void test_firebase_http_get_fixture_200(void) {
    firebase_http_test_set_response("documents/", 200, "{}");
    FirebaseHttpResponse* resp = firebase_http_get(
        "http://127.0.0.1:8080/v1/projects/hydrodemo/databases/(default)/documents/",
        NULL, false, NULL, NULL);
    TEST_ASSERT_NOT_NULL(resp);
    TEST_ASSERT_EQUAL(200, resp->http_status);
    TEST_ASSERT_EQUAL_STRING("{}", resp->body);
    firebase_http_response_free(resp);
}

void test_firebase_http_get_fixture_500(void) {
    firebase_http_test_set_response("hydrodemo", 500, "nope");
    FirebaseHttpResponse* resp = firebase_http_get(
        "http://127.0.0.1:8080/v1/projects/hydrodemo/databases/(default)/documents/",
        NULL, false, NULL, NULL);
    TEST_ASSERT_NOT_NULL(resp);
    TEST_ASSERT_EQUAL(500, resp->http_status);
    TEST_ASSERT_FALSE(firebase_http_status_is_healthy(resp->http_status));
    firebase_http_response_free(resp);
}

void test_firebase_http_get_invalid_scheme(void) {
    FirebaseHttpResponse* resp = firebase_http_get("ftp://example", NULL, false, NULL, NULL);
    TEST_ASSERT_NOT_NULL(resp);
    TEST_ASSERT_NOT_NULL(resp->error_message);
    firebase_http_response_free(resp);
}

void test_firebase_http_url_matches(void) {
    TEST_ASSERT_TRUE(firebase_http_url_matches("http://x/documents/", NULL));
    TEST_ASSERT_TRUE(firebase_http_url_matches("http://x/documents/", "documents/"));
    TEST_ASSERT_FALSE(firebase_http_url_matches("http://x", "documents/"));
    TEST_ASSERT_FALSE(firebase_http_url_matches(NULL, "x"));
}

void test_firebase_http_write_callback_grows(void) {
    FirebaseHttpBuffer buf = {0};
    buf.data = calloc(1, 8);
    buf.capacity = 8;
    TEST_ASSERT_NOT_NULL(buf.data);
    const char* chunk = "abcdefghijklmnop";
    size_t n = firebase_http_write_callback((char*)chunk, 1, strlen(chunk), &buf);
    TEST_ASSERT_EQUAL(strlen(chunk), n);
    TEST_ASSERT_EQUAL_STRING(chunk, buf.data);
    TEST_ASSERT_EQUAL(0, firebase_http_write_callback((char*)chunk, 1, 4, NULL));
    free(buf.data);
}

void test_firebase_http_xferinfo_abort(void) {
    volatile bool abort_flag = false;
    TEST_ASSERT_EQUAL(0, firebase_http_xferinfo_callback((void*)&abort_flag, 0, 0, 0, 0));
    abort_flag = true;
    TEST_ASSERT_EQUAL(1, firebase_http_xferinfo_callback((void*)&abort_flag, 0, 0, 0, 0));
    TEST_ASSERT_EQUAL(0, firebase_http_xferinfo_callback(NULL, 0, 0, 0, 0));
}

void test_firebase_http_apply_curl_opts_null(void) {
    firebase_http_apply_curl_opts(NULL, false, NULL);
}

void test_firebase_http_get_transport_failure(void) {
    CURL* slot = NULL;
    volatile bool abort_flag = false;
    FirebaseHttpResponse* resp = firebase_http_get(
        "http://127.0.0.1:1/v1/projects/hydrodemo/databases/(default)/documents/",
        "Authorization: Bearer test", false, &slot, &abort_flag);
    TEST_ASSERT_NOT_NULL(resp);
    TEST_ASSERT_EQUAL(0, resp->http_status);
    TEST_ASSERT_NOT_NULL(resp->error_message);
    TEST_ASSERT_NULL(slot);
    firebase_http_response_free(resp);
}

void test_firebase_http_test_queue_overflow_and_clear(void) {
    for (int i = 0; i < 10; i++) {
        firebase_http_test_set_response("overflow", 200, "{}");
    }
    firebase_http_test_clear_responses();
    TEST_ASSERT_FALSE(firebase_http_take_fixture("overflow", NULL, NULL));
}

void test_firebase_http_build_documents_url_defaults(void) {
    char* url = firebase_http_build_documents_url("127.0.0.1", 0, "hydrodemo", NULL, true);
    TEST_ASSERT_NOT_NULL(url);
    TEST_ASSERT_NOT_NULL(strstr(url, ":8080/"));
    TEST_ASSERT_NOT_NULL(strstr(url, "(default)"));
    free(url);
    url = firebase_http_build_documents_url("firestore.googleapis.com", 0, "hydrodemo", "(default)", false);
    TEST_ASSERT_NOT_NULL(url);
    TEST_ASSERT_NOT_NULL(strstr(url, "https://"));
    TEST_ASSERT_NOT_NULL(strstr(url, ":443/"));
    free(url);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_http_response_alloc_free);
    RUN_TEST(test_firebase_http_status_is_healthy);
    RUN_TEST(test_firebase_http_build_documents_url_emulator);
    RUN_TEST(test_firebase_http_build_documents_url_nulls);
    RUN_TEST(test_firebase_http_get_null_url);
    RUN_TEST(test_firebase_http_get_fixture_200);
    RUN_TEST(test_firebase_http_get_fixture_500);
    RUN_TEST(test_firebase_http_get_invalid_scheme);
    RUN_TEST(test_firebase_http_url_matches);
    RUN_TEST(test_firebase_http_write_callback_grows);
    RUN_TEST(test_firebase_http_xferinfo_abort);
    RUN_TEST(test_firebase_http_apply_curl_opts_null);
    RUN_TEST(test_firebase_http_get_transport_failure);
    RUN_TEST(test_firebase_http_test_queue_overflow_and_clear);
    RUN_TEST(test_firebase_http_build_documents_url_defaults);
    return UNITY_END();
}
