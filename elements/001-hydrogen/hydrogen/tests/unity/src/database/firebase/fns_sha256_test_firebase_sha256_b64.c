/*
 * Unity tests for firebase_sha256_b64().
 * Login compatibility gate: recorded SQLite crypto_sha256 + crypto_encode.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebase/fns_sha256.h>

void test_firebase_sha256_b64_null(void);
void test_firebase_sha256_b64_sqlite_fixture(void);
void test_firebase_sha256_b64_deterministic(void);
void test_firebase_sha256_b64_different_ids(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_sha256_b64_null(void) {
    TEST_ASSERT_NULL(firebase_sha256_b64(NULL, "x"));
    TEST_ASSERT_NULL(firebase_sha256_b64("0", NULL));
    TEST_ASSERT_NULL(firebase_sha256_b64(NULL, NULL));
}

void test_firebase_sha256_b64_sqlite_fixture(void) {
    /* Independently verified 2026-09-16:
     *   sqlite3: crypto_encode(crypto_sha256('0' || 'testpass'), 'base64')
     *   openssl: printf '0testpass' | openssl dgst -sha256 -binary | openssl base64 -A
     */
    char* hash = firebase_sha256_b64("0", "testpass");
    TEST_ASSERT_NOT_NULL(hash);
    TEST_ASSERT_EQUAL_STRING("CUQEdl7cgIo2iGBfQmsuosLbdT9uLVpbm/rRJGQlbw0=", hash);
    free(hash);
}

void test_firebase_sha256_b64_deterministic(void) {
    char* a = firebase_sha256_b64("1", "secret");
    char* b = firebase_sha256_b64("1", "secret");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_EQUAL_STRING(a, b);
    free(a);
    free(b);
}

void test_firebase_sha256_b64_different_ids(void) {
    char* a = firebase_sha256_b64("1", "secret");
    char* b = firebase_sha256_b64("2", "secret");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_TRUE(strcmp(a, b) != 0);
    free(a);
    free(b);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_sha256_b64_null);
    RUN_TEST(test_firebase_sha256_b64_sqlite_fixture);
    RUN_TEST(test_firebase_sha256_b64_deterministic);
    RUN_TEST(test_firebase_sha256_b64_different_ids);
    return UNITY_END();
}
