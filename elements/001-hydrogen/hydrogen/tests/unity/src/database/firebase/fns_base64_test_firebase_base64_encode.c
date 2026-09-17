/*
 * Unity tests for firebase_base64_encode().
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebase/fns_base64.h>

void test_firebase_base64_encode_null(void);
void test_firebase_base64_encode_empty(void);
void test_firebase_base64_encode_hello(void);
void test_firebase_base64_encode_padding(void);
void test_firebase_base64_encode_binary(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_base64_encode_null(void) {
    TEST_ASSERT_NULL(firebase_base64_encode(NULL, 0));
    TEST_ASSERT_NULL(firebase_base64_encode(NULL, 4));
}

void test_firebase_base64_encode_empty(void) {
    const unsigned char data[1] = {0};
    char* encoded = firebase_base64_encode(data, 0);
    TEST_ASSERT_NOT_NULL(encoded);
    TEST_ASSERT_EQUAL_STRING("", encoded);
    free(encoded);
}

void test_firebase_base64_encode_hello(void) {
    const unsigned char* hello = (const unsigned char*)"hello";
    char* encoded = firebase_base64_encode(hello, 5);
    TEST_ASSERT_NOT_NULL(encoded);
    TEST_ASSERT_EQUAL_STRING("aGVsbG8=", encoded);
    free(encoded);
}

void test_firebase_base64_encode_padding(void) {
    const unsigned char* m = (const unsigned char*)"M";
    char* encoded = firebase_base64_encode(m, 1);
    TEST_ASSERT_NOT_NULL(encoded);
    TEST_ASSERT_EQUAL_STRING("TQ==", encoded);
    free(encoded);
}

void test_firebase_base64_encode_binary(void) {
    const unsigned char data[] = {0x00, 0xff, 0x10};
    char* encoded = firebase_base64_encode(data, sizeof(data));
    TEST_ASSERT_NOT_NULL(encoded);
    TEST_ASSERT_EQUAL_STRING("AP8Q", encoded);
    free(encoded);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_base64_encode_null);
    RUN_TEST(test_firebase_base64_encode_empty);
    RUN_TEST(test_firebase_base64_encode_hello);
    RUN_TEST(test_firebase_base64_encode_padding);
    RUN_TEST(test_firebase_base64_encode_binary);
    return UNITY_END();
}
