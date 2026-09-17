/*
 * Unity tests for firebase_base64_decode().
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebase/fns_base64.h>

void test_firebase_base64_decode_null(void);
void test_firebase_base64_decode_hello(void);
void test_firebase_base64_decode_padding(void);
void test_firebase_base64_decode_invalid(void);
void test_firebase_base64_decode_round_trip(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_base64_decode_null(void) {
    size_t out_len = 99;
    TEST_ASSERT_NULL(firebase_base64_decode(NULL, &out_len));
    TEST_ASSERT_NULL(firebase_base64_decode("aGVsbG8=", NULL));
}

void test_firebase_base64_decode_hello(void) {
    size_t out_len = 0;
    unsigned char* decoded = firebase_base64_decode("aGVsbG8=", &out_len);
    TEST_ASSERT_NOT_NULL(decoded);
    TEST_ASSERT_EQUAL_size_t(5, out_len);
    TEST_ASSERT_EQUAL_STRING("hello", (char*)decoded);
    free(decoded);
}

void test_firebase_base64_decode_padding(void) {
    size_t out_len = 0;
    unsigned char* decoded = firebase_base64_decode("TQ==", &out_len);
    TEST_ASSERT_NOT_NULL(decoded);
    TEST_ASSERT_EQUAL_size_t(1, out_len);
    TEST_ASSERT_EQUAL('M', decoded[0]);
    free(decoded);
}

void test_firebase_base64_decode_invalid(void) {
    size_t out_len = 0;
    TEST_ASSERT_NULL(firebase_base64_decode("!!!!", &out_len));
    TEST_ASSERT_NULL(firebase_base64_decode("", &out_len));
}

void test_firebase_base64_decode_round_trip(void) {
    const unsigned char src[] = {0xfb, 0xff, 0x00};
    char* encoded = firebase_base64_encode(src, sizeof(src));
    TEST_ASSERT_NOT_NULL(encoded);
    TEST_ASSERT_EQUAL_STRING("+/8A", encoded);
    size_t out_len = 0;
    unsigned char* decoded = firebase_base64_decode(encoded, &out_len);
    TEST_ASSERT_NOT_NULL(decoded);
    TEST_ASSERT_EQUAL_size_t(sizeof(src), out_len);
    TEST_ASSERT_EQUAL_MEMORY(src, decoded, sizeof(src));
    free(encoded);
    free(decoded);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_base64_decode_null);
    RUN_TEST(test_firebase_base64_decode_hello);
    RUN_TEST(test_firebase_base64_decode_padding);
    RUN_TEST(test_firebase_base64_decode_invalid);
    RUN_TEST(test_firebase_base64_decode_round_trip);
    return UNITY_END();
}
