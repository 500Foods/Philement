/*
 * Unity Test File: Reporting base64_utils
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/reporting/helpers/base64_utils.h>
#include <src/utils/utils_crypto.h>

void test_parse_data_uri_plain(void);
void test_parse_data_uri_with_prefix(void);
void test_parse_data_uri_null(void);
void test_reporting_base64_roundtrip(void);
void test_reporting_base64_data_uri(void);
void test_reporting_base64_invalid(void);
void test_reporting_base64_empty(void);
void test_utils_base64_decode_whitespace(void);

void setUp(void) {}
void tearDown(void) {}

void test_parse_data_uri_plain(void) {
    const char *start = NULL;
    TEST_ASSERT_TRUE(parse_data_uri("abc123==", &start));
    TEST_ASSERT_EQUAL_STRING("abc123==", start);
}

void test_parse_data_uri_with_prefix(void) {
    const char *start = NULL;
    const char *uri = "data:image/png;base64,iVBORw0KGgo=";
    TEST_ASSERT_TRUE(parse_data_uri(uri, &start));
    TEST_ASSERT_EQUAL_STRING("iVBORw0KGgo=", start);
}

void test_parse_data_uri_null(void) {
    const char *start = NULL;
    TEST_ASSERT_FALSE(parse_data_uri(NULL, &start));
    TEST_ASSERT_FALSE(parse_data_uri("data:text/plain,hello", NULL));
}

void test_reporting_base64_roundtrip(void) {
    const unsigned char data[] = {0x00, 0x01, 0x02, 0xFE, 0xFF, 'A', 'B'};
    char *encoded = reporting_base64_encode(data, sizeof(data));
    TEST_ASSERT_NOT_NULL(encoded);

    size_t out_len = 0;
    unsigned char *decoded = reporting_base64_decode(encoded, &out_len);
    TEST_ASSERT_NOT_NULL(decoded);
    TEST_ASSERT_EQUAL_size_t(sizeof(data), out_len);
    TEST_ASSERT_EQUAL_MEMORY(data, decoded, sizeof(data));

    free(encoded);
    free(decoded);
}

void test_reporting_base64_data_uri(void) {
    // "Hi" -> SGk=
    const char *uri = "data:image/png;base64,SGk=";
    size_t out_len = 0;
    unsigned char *decoded = reporting_base64_decode(uri, &out_len);
    TEST_ASSERT_NOT_NULL(decoded);
    TEST_ASSERT_EQUAL_size_t(2, out_len);
    TEST_ASSERT_EQUAL_MEMORY("Hi", decoded, 2);
    free(decoded);
}

void test_reporting_base64_invalid(void) {
    size_t out_len = 0;
    unsigned char *decoded = reporting_base64_decode("!!!!", &out_len);
    TEST_ASSERT_NULL(decoded);
}

void test_reporting_base64_empty(void) {
    size_t out_len = 0;
    TEST_ASSERT_NULL(reporting_base64_decode("", &out_len));
    TEST_ASSERT_NULL(reporting_base64_encode(NULL, 0));
}

void test_utils_base64_decode_whitespace(void) {
    size_t out_len = 0;
    unsigned char *decoded = utils_base64_decode("SG\nk=", &out_len);
    TEST_ASSERT_NOT_NULL(decoded);
    TEST_ASSERT_EQUAL_size_t(2, out_len);
    TEST_ASSERT_EQUAL_MEMORY("Hi", decoded, 2);
    free(decoded);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_parse_data_uri_plain);
    RUN_TEST(test_parse_data_uri_with_prefix);
    RUN_TEST(test_parse_data_uri_null);
    RUN_TEST(test_reporting_base64_roundtrip);
    RUN_TEST(test_reporting_base64_data_uri);
    RUN_TEST(test_reporting_base64_invalid);
    RUN_TEST(test_reporting_base64_empty);
    RUN_TEST(test_utils_base64_decode_whitespace);
    return UNITY_END();
}
