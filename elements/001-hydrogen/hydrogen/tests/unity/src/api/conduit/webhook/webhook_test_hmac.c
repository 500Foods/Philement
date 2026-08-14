/*
 * Unity Test File: conduit webhook HMAC helpers
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/conduit/webhook/webhook.h>
#include <src/utils/utils_crypto.h>

void test_extract_hook_conduit_path(void);
void test_extract_hook_alias_path(void);
void test_extract_hook_rejects_extra_segment(void);
void test_extract_signature_token_v1(void);
void test_extract_signature_token_sha256_prefix(void);
void test_extract_timestamp(void);
void test_hex_equal_case_insensitive(void);
void test_verify_hmac_sha256_good(void);
void test_verify_hmac_sha256_bad(void);
void test_verify_hmac_timestamp_mode(void);
void test_build_params_omits_hydrogen(void);

void setUp(void) {}
void tearDown(void) {}

void test_extract_hook_conduit_path(void) {
    char *h = conduit_webhook_extract_hook("conduit/webhook/stripe");
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_EQUAL_STRING("stripe", h);
    free(h);
}

void test_extract_hook_alias_path(void) {
    char *h = conduit_webhook_extract_hook("webhook/stripe");
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_EQUAL_STRING("stripe", h);
    free(h);
    h = conduit_webhook_extract_hook_from_url("/webhook/stripe");
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_EQUAL_STRING("stripe", h);
    free(h);
}

void test_extract_hook_rejects_extra_segment(void) {
    TEST_ASSERT_NULL(conduit_webhook_extract_hook("conduit/webhook/stripe/extra"));
    TEST_ASSERT_NULL(conduit_webhook_extract_hook("conduit/webhook/"));
    TEST_ASSERT_NULL(conduit_webhook_extract_hook("conduit/script"));
}

void test_extract_signature_token_v1(void) {
    char *t = conduit_webhook_extract_signature_token(
        "t=1492774577,v1=abc123,v0=zzz");
    TEST_ASSERT_NOT_NULL(t);
    TEST_ASSERT_EQUAL_STRING("abc123", t);
    free(t);
}

void test_extract_signature_token_sha256_prefix(void) {
    char *t = conduit_webhook_extract_signature_token("sha256=deadbeef");
    TEST_ASSERT_NOT_NULL(t);
    TEST_ASSERT_EQUAL_STRING("deadbeef", t);
    free(t);
}

void test_extract_timestamp(void) {
    char *t = conduit_webhook_extract_timestamp("t=1492774577,v1=abc");
    TEST_ASSERT_NOT_NULL(t);
    TEST_ASSERT_EQUAL_STRING("1492774577", t);
    free(t);
}

void test_hex_equal_case_insensitive(void) {
    TEST_ASSERT_TRUE(conduit_webhook_hex_equal("aBCd", "abcd"));
    TEST_ASSERT_FALSE(conduit_webhook_hex_equal("ab", "abc"));
    TEST_ASSERT_FALSE(conduit_webhook_hex_equal(NULL, "ab"));
}

void test_verify_hmac_sha256_good(void) {
    const char *body = "{\"ok\":true}";
    const char *secret = "s3cret";
    unsigned int mac_len = 0;
    unsigned char *mac = utils_hmac_sha256((const unsigned char *)body,
                                           strlen(body), secret, strlen(secret),
                                           &mac_len);
    TEST_ASSERT_NOT_NULL(mac);
    char *hex = conduit_webhook_bytes_to_hex(mac, mac_len);
    TEST_ASSERT_NOT_NULL(hex);
    TEST_ASSERT_TRUE(conduit_webhook_verify_hmac(
        (const unsigned char *)body, strlen(body), secret, hex, "sha256"));
    char prefixed[128];
    snprintf(prefixed, sizeof(prefixed), "sha256=%s", hex);
    TEST_ASSERT_TRUE(conduit_webhook_verify_hmac(
        (const unsigned char *)body, strlen(body), secret, prefixed, "sha256"));
    free(hex);
    free(mac);
}

void test_verify_hmac_sha256_bad(void) {
    const char *body = "{\"ok\":true}";
    TEST_ASSERT_FALSE(conduit_webhook_verify_hmac(
        (const unsigned char *)body, strlen(body), "s3cret", "00", "sha256"));
}

void test_verify_hmac_timestamp_mode(void) {
    const char *body = "{\"ok\":true}";
    const char *secret = "s3cret";
    const char *ts = "1492774577";
    char signed_buf[64];
    size_t n = (size_t)snprintf(signed_buf, sizeof(signed_buf), "%s.%s", ts, body);
    unsigned int mac_len = 0;
    unsigned char *mac = utils_hmac_sha256((const unsigned char *)signed_buf, n,
                                           secret, strlen(secret), &mac_len);
    TEST_ASSERT_NOT_NULL(mac);
    char *hex = conduit_webhook_bytes_to_hex(mac, mac_len);
    char header[160];
    snprintf(header, sizeof(header), "t=%s,v1=%s", ts, hex);
    TEST_ASSERT_TRUE(conduit_webhook_verify_hmac(
        (const unsigned char *)body, strlen(body), secret, header,
        "sha256-timestamp"));
    TEST_ASSERT_FALSE(conduit_webhook_verify_hmac(
        (const unsigned char *)body, strlen(body), secret, header, "sha256"));
    free(hex);
    free(mac);
}

void test_build_params_omits_hydrogen(void) {
    json_t *hdrs = json_object();
    json_object_set_new(hdrs, "X-Test", json_string("1"));
    char *js = conduit_webhook_build_params_json(
        "stripe", (const unsigned char *)"{\"a\":1}", 7, "application/json",
        hdrs);
    json_decref(hdrs);
    TEST_ASSERT_NOT_NULL(js);
    TEST_ASSERT_NOT_NULL(strstr(js, "\"hook\":\"stripe\""));
    TEST_ASSERT_NOT_NULL(strstr(js, "\"body\":\"{\\\"a\\\":1}\""));
    TEST_ASSERT_NULL(strstr(js, "_hydrogen"));
    free(js);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_extract_hook_conduit_path);
    RUN_TEST(test_extract_hook_alias_path);
    RUN_TEST(test_extract_hook_rejects_extra_segment);
    RUN_TEST(test_extract_signature_token_v1);
    RUN_TEST(test_extract_signature_token_sha256_prefix);
    RUN_TEST(test_extract_timestamp);
    RUN_TEST(test_hex_equal_case_insensitive);
    RUN_TEST(test_verify_hmac_sha256_good);
    RUN_TEST(test_verify_hmac_sha256_bad);
    RUN_TEST(test_verify_hmac_timestamp_mode);
    RUN_TEST(test_build_params_omits_hydrogen);
    return UNITY_END();
}
