/*
 * Unity tests for firebase_brotli_decompress().
 * lua-brotli quality 11 fixtures plus extras UDF short-string fixture.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebase/fns_base64.h>
#include <src/database/firebase/fns_brotli.h>

void test_firebase_brotli_decompress_null(void);
void test_firebase_brotli_decompress_empty(void);
void test_firebase_brotli_decompress_lua_quality_11_hello(void);
void test_firebase_brotli_decompress_extras_udf_hello(void);
void test_firebase_brotli_decompress_lua_quality_11_payload(void);
void test_firebase_brotli_decompress_invalid(void);
void test_firebase_brotli_wrapped_expression(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_brotli_decompress_null(void) {
    TEST_ASSERT_NULL(firebase_brotli_decompress(NULL, 4, NULL));
}

void test_firebase_brotli_decompress_empty(void) {
    const unsigned char data[1] = {0};
    size_t out_len = 99;
    char* text = firebase_brotli_decompress(data, 0, &out_len);
    TEST_ASSERT_NOT_NULL(text);
    TEST_ASSERT_EQUAL_STRING("", text);
    TEST_ASSERT_EQUAL_size_t(0, out_len);
    free(text);
}

void test_firebase_brotli_decompress_lua_quality_11_hello(void) {
    size_t decoded_len = 0;
    unsigned char* decoded = firebase_base64_decode("iwWASGVsbG8gV29ybGQhAw==", &decoded_len);
    TEST_ASSERT_NOT_NULL(decoded);
    size_t out_len = 0;
    char* text = firebase_brotli_decompress(decoded, decoded_len, &out_len);
    free(decoded);
    TEST_ASSERT_NOT_NULL(text);
    TEST_ASSERT_EQUAL_STRING("Hello World!", text);
    TEST_ASSERT_EQUAL_size_t(12, out_len);
    free(text);
}

void test_firebase_brotli_decompress_extras_udf_hello(void) {
    size_t decoded_len = 0;
    unsigned char* decoded = firebase_base64_decode("jwWASGVsbG8gV29ybGQhAw==", &decoded_len);
    TEST_ASSERT_NOT_NULL(decoded);
    char* text = firebase_brotli_decompress(decoded, decoded_len, NULL);
    free(decoded);
    TEST_ASSERT_NOT_NULL(text);
    TEST_ASSERT_EQUAL_STRING("Hello World!", text);
    free(text);
}

void test_firebase_brotli_decompress_lua_quality_11_payload(void) {
    const char* expected = "Phase4 firebase brotli quality 11 fixture";
    size_t decoded_len = 0;
    unsigned char* decoded = firebase_base64_decode(
        "CxSAUGhhc2U0IGZpcmViYXNlIGJyb3RsaSBxdWFsaXR5IDExIGZpeHR1cmUD", &decoded_len);
    TEST_ASSERT_NOT_NULL(decoded);
    size_t out_len = 0;
    char* text = firebase_brotli_decompress(decoded, decoded_len, &out_len);
    free(decoded);
    TEST_ASSERT_NOT_NULL(text);
    TEST_ASSERT_EQUAL_STRING(expected, text);
    TEST_ASSERT_EQUAL_size_t(strlen(expected), out_len);
    free(text);
}

void test_firebase_brotli_decompress_invalid(void) {
    const unsigned char junk[] = {0x00, 0x01, 0x02, 0x03, 0xff};
    TEST_ASSERT_NULL(firebase_brotli_decompress(junk, sizeof(junk), NULL));
}

void test_firebase_brotli_wrapped_expression(void) {
    const char* expr =
        "FB_BROTLI_DECOMPRESS(FB_BASE64_DECODE("
        "'CxSAUGhhc2U0IGZpcmViYXNlIGJyb3RsaSBxdWFsaXR5IDExIGZpeHR1cmUD'))";
    const char* start = strchr(expr, '\'');
    TEST_ASSERT_NOT_NULL(start);
    start++;
    const char* end = strchr(start, '\'');
    TEST_ASSERT_NOT_NULL(end);
    size_t n = (size_t)(end - start);
    char* b64 = malloc(n + 1);
    TEST_ASSERT_NOT_NULL(b64);
    memcpy(b64, start, n);
    b64[n] = '\0';

    size_t decoded_len = 0;
    unsigned char* decoded = firebase_base64_decode(b64, &decoded_len);
    free(b64);
    TEST_ASSERT_NOT_NULL(decoded);
    char* text = firebase_brotli_decompress(decoded, decoded_len, NULL);
    free(decoded);
    TEST_ASSERT_NOT_NULL(text);
    TEST_ASSERT_EQUAL_STRING("Phase4 firebase brotli quality 11 fixture", text);
    free(text);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_brotli_decompress_null);
    RUN_TEST(test_firebase_brotli_decompress_empty);
    RUN_TEST(test_firebase_brotli_decompress_lua_quality_11_hello);
    RUN_TEST(test_firebase_brotli_decompress_extras_udf_hello);
    RUN_TEST(test_firebase_brotli_decompress_lua_quality_11_payload);
    RUN_TEST(test_firebase_brotli_decompress_invalid);
    RUN_TEST(test_firebase_brotli_wrapped_expression);
    return UNITY_END();
}
