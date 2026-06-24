/*
 * Cap Verify Helper Test
 *
 * Tests the cap_verify_token helper using a fake POST seam.
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <src/api/conduit/helpers/cap_verify.h>
#include <src/config/config_webserver.h>
#include <src/config/config.h>

// Forward declarations for functions being tested
cap_verify_result_t cap_verify_token(const char* token,
                                     const char* site_id,
                                     char* error_out,
                                     size_t error_sz);
void cap_verify_test_set_post_fn(
    char* (*fn)(const char*, const char*, long*, char*, size_t));
void cap_verify_test_clear_post_fn(void);

// Forward declarations for test functions
void test_cap_verify_null_token(void);
void test_cap_verify_empty_token(void);
void test_cap_verify_missing_app_config(void);
void test_cap_verify_missing_server(void);
void test_cap_verify_missing_secret(void);
void test_cap_verify_invalid_server_url(void);
void test_cap_verify_transport_failure(void);
void test_cap_verify_non_200_response(void);
void test_cap_verify_invalid_json(void);
void test_cap_verify_rejected_token(void);
void test_cap_verify_success(void);
void test_cap_verify_url_and_body(void);
void test_cap_verify_http_5xx_fallback(void);

// Fake POST state
static char fake_last_url[1024];
static char fake_last_body[2048];
static char* fake_response_body = NULL;
static long fake_response_status = 0;
static bool fake_transport_fail = false;

static char* fake_cap_verify_post(const char* url,
                                  const char* body,
                                  long* out_status,
                                  char* error_out,
                                  size_t error_sz) {
    strncpy(fake_last_url, url ? url : "", sizeof(fake_last_url) - 1);
    fake_last_url[sizeof(fake_last_url) - 1] = '\0';
    strncpy(fake_last_body, body ? body : "", sizeof(fake_last_body) - 1);
    fake_last_body[sizeof(fake_last_body) - 1] = '\0';

    if (fake_transport_fail) {
        snprintf(error_out, error_sz, "CAP_VERIFY_FAILED: transport error");
        return NULL;
    }

    *out_status = fake_response_status;
    if (!fake_response_body) {
        snprintf(error_out, error_sz, "CAP_VERIFY_FAILED: no response");
        return NULL;
    }
    return strdup(fake_response_body);
}

static void reset_fake_state(void) {
    fake_last_url[0] = '\0';
    fake_last_body[0] = '\0';
    free(fake_response_body);
    fake_response_body = NULL;
    fake_response_status = 0;
    fake_transport_fail = false;
}

// Test configuration and global pointer
static AppConfig test_config;

void setUp(void) {
    reset_fake_state();
    memset(&test_config, 0, sizeof(test_config));
    app_config = &test_config;
    cap_verify_test_set_post_fn(fake_cap_verify_post);
}

void tearDown(void) {
    cap_verify_test_clear_post_fn();
    app_config = NULL;
    cleanup_webserver_config(&test_config.webserver);
    memset(&test_config, 0, sizeof(test_config));
    reset_fake_state();
}

static void setup_valid_config(void) {
    test_config.webserver.chacha_server = strdup("https://chacha.example.com");
    test_config.webserver.chacha_secret = strdup("test-secret");
}

void test_cap_verify_null_token(void) {
    setup_valid_config();
    char error[256];
    cap_verify_result_t result = cap_verify_token(NULL, "site-1", error, sizeof(error));
    TEST_ASSERT_EQUAL(CAP_VERIFY_HARD_FAIL, result);
    TEST_ASSERT_EQUAL_STRING("CAP_TOKEN_MISSING", error);
}

void test_cap_verify_empty_token(void) {
    setup_valid_config();
    char error[256];
    cap_verify_result_t result = cap_verify_token("", "site-1", error, sizeof(error));
    TEST_ASSERT_EQUAL(CAP_VERIFY_HARD_FAIL, result);
    TEST_ASSERT_EQUAL_STRING("CAP_TOKEN_MISSING", error);
}

void test_cap_verify_missing_app_config(void) {
    app_config = NULL;
    char error[256];
    cap_verify_result_t result = cap_verify_token("token", "site-1", error, sizeof(error));
    TEST_ASSERT_EQUAL(CAP_VERIFY_HARD_FAIL, result);
    TEST_ASSERT_EQUAL_STRING("CAP_VERIFY_FAILED: configuration unavailable", error);
}

void test_cap_verify_missing_server(void) {
    test_config.webserver.chacha_secret = strdup("test-secret");
    char error[256];
    cap_verify_result_t result = cap_verify_token("token", "site-1", error, sizeof(error));
    TEST_ASSERT_EQUAL(CAP_VERIFY_HARD_FAIL, result);
    TEST_ASSERT_EQUAL_STRING("CAP_VERIFY_FAILED: ChaCha server not configured", error);
}

void test_cap_verify_missing_secret(void) {
    test_config.webserver.chacha_server = strdup("https://chacha.example.com");
    char error[256];
    cap_verify_result_t result = cap_verify_token("token", "site-1", error, sizeof(error));
    TEST_ASSERT_EQUAL(CAP_VERIFY_HARD_FAIL, result);
    TEST_ASSERT_EQUAL_STRING("CAP_VERIFY_FAILED: ChaCha secret not configured", error);
}

void test_cap_verify_invalid_server_url(void) {
    test_config.webserver.chacha_server = strdup("ftp://chacha.example.com");
    test_config.webserver.chacha_secret = strdup("test-secret");
    char error[256];
    cap_verify_result_t result = cap_verify_token("token", "site-1", error, sizeof(error));
    TEST_ASSERT_EQUAL(CAP_VERIFY_HARD_FAIL, result);
    TEST_ASSERT_EQUAL_STRING("CAP_VERIFY_FAILED: invalid ChaCha server URL", error);
}

void test_cap_verify_transport_failure(void) {
    setup_valid_config();
    fake_transport_fail = true;
    char error[256];
    cap_verify_result_t result = cap_verify_token("token", "site-1", error, sizeof(error));
    TEST_ASSERT_EQUAL(CAP_VERIFY_FALLBACK, result);
    TEST_ASSERT_EQUAL_STRING("CAP_VERIFY_FAILED: transport error", error);
}

void test_cap_verify_non_200_response(void) {
    setup_valid_config();
    fake_response_status = 400;
    fake_response_body = strdup("{\"success\": false}");
    char error[256];
    cap_verify_result_t result = cap_verify_token("token", "site-1", error, sizeof(error));
    TEST_ASSERT_EQUAL(CAP_VERIFY_HARD_FAIL, result);
    TEST_ASSERT_EQUAL_STRING("CAP_VERIFY_FAILED: HTTP 400", error);
}

void test_cap_verify_invalid_json(void) {
    setup_valid_config();
    fake_response_status = 200;
    fake_response_body = strdup("not json");
    char error[256];
    cap_verify_result_t result = cap_verify_token("token", "site-1", error, sizeof(error));
    TEST_ASSERT_EQUAL(CAP_VERIFY_HARD_FAIL, result);
    TEST_ASSERT_EQUAL_STRING("CAP_VERIFY_FAILED: invalid siteverify response", error);
}

void test_cap_verify_rejected_token(void) {
    setup_valid_config();
    fake_response_status = 200;
    fake_response_body = strdup("{\"success\": false, \"error-codes\": [\"timeout-or-duplicate\"]}");
    char error[256];
    cap_verify_result_t result = cap_verify_token("token", "site-1", error, sizeof(error));
    TEST_ASSERT_EQUAL(CAP_VERIFY_HARD_FAIL, result);
    TEST_ASSERT_EQUAL_STRING("CAP_VERIFY_FAILED: token rejected", error);
}

void test_cap_verify_success(void) {
    setup_valid_config();
    fake_response_status = 200;
    fake_response_body = strdup("{\"success\": true}");
    char error[256] = {0};
    cap_verify_result_t result = cap_verify_token("token", "site-1", error, sizeof(error));
    TEST_ASSERT_EQUAL(CAP_VERIFY_OK, result);
    TEST_ASSERT_EQUAL_STRING("", error);
}

void test_cap_verify_url_and_body(void) {
    setup_valid_config();
    fake_response_status = 200;
    fake_response_body = strdup("{\"success\": true}");
    char error[256];
    cap_verify_result_t result = cap_verify_token("my-token", "site-abc", error, sizeof(error));
    TEST_ASSERT_EQUAL(CAP_VERIFY_OK, result);
    TEST_ASSERT_EQUAL_STRING("https://chacha.example.com/site-abc/siteverify", fake_last_url);
    TEST_ASSERT_NOT_NULL(strstr(fake_last_body, "\"secret\":\"test-secret\""));
    TEST_ASSERT_NOT_NULL(strstr(fake_last_body, "\"response\":\"my-token\""));
}

void test_cap_verify_http_5xx_fallback(void) {
    setup_valid_config();
    fake_response_status = 502;
    fake_response_body = strdup("Bad Gateway");
    char error[256];
    cap_verify_result_t result = cap_verify_token("token", "site-1", error, sizeof(error));
    TEST_ASSERT_EQUAL(CAP_VERIFY_FALLBACK, result);
    TEST_ASSERT_EQUAL_STRING("CAP_VERIFY_FAILED: HTTP 502", error);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_cap_verify_null_token);
    RUN_TEST(test_cap_verify_empty_token);
    RUN_TEST(test_cap_verify_missing_app_config);
    RUN_TEST(test_cap_verify_missing_server);
    RUN_TEST(test_cap_verify_missing_secret);
    RUN_TEST(test_cap_verify_invalid_server_url);
    RUN_TEST(test_cap_verify_transport_failure);
    RUN_TEST(test_cap_verify_non_200_response);
    RUN_TEST(test_cap_verify_invalid_json);
    RUN_TEST(test_cap_verify_rejected_token);
    RUN_TEST(test_cap_verify_success);
    RUN_TEST(test_cap_verify_url_and_body);
    RUN_TEST(test_cap_verify_http_5xx_fallback);
    return UNITY_END();
}
