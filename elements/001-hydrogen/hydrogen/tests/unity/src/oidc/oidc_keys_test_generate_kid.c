/*
 * Unity Test File: oidc_generate_kid
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_keys.h>
#include <unity/mocks/mock_crypto.h>
#include <string.h>

void test_generate_kid_null_output(void);
void test_generate_kid_short_buffer(void);
void test_generate_kid_random_bytes_failure(void);
void test_generate_kid_base64url_encode_failure(void);
void test_generate_kid_success(void);

void setUp(void) {
    mock_crypto_reset_all();
}

void tearDown(void) {
    mock_crypto_reset_all();
}

void test_generate_kid_null_output(void) {
    oidc_generate_kid(NULL, 10);
}

void test_generate_kid_short_buffer(void) {
    char kid[8];
    memset(kid, 'X', sizeof(kid));
    kid[7] = '\0';
    oidc_generate_kid(kid, sizeof(kid));
    TEST_ASSERT_EQUAL_STRING("XXXXXXX", kid);
}

void test_generate_kid_random_bytes_failure(void) {
    mock_crypto_set_random_bytes_failure(1);
    char kid[OIDC_KEY_ID_LENGTH + 1];
    memset(kid, 0, sizeof(kid));
    oidc_generate_kid(kid, sizeof(kid));
    TEST_ASSERT_EQUAL_STRING("oidc-default", kid);
}

void test_generate_kid_base64url_encode_failure(void) {
    mock_crypto_set_random_bytes_failure(0);
    mock_crypto_set_base64url_encode_failure(1);
    char kid[OIDC_KEY_ID_LENGTH + 1];
    memset(kid, 0, sizeof(kid));
    oidc_generate_kid(kid, sizeof(kid));
    TEST_ASSERT_EQUAL_STRING("oidc-default", kid);
}

void test_generate_kid_success(void) {
    char kid[OIDC_KEY_ID_LENGTH + 1];
    memset(kid, 0, sizeof(kid));
    oidc_generate_kid(kid, sizeof(kid));
    TEST_ASSERT_TRUE(strlen(kid) > 0U);
    TEST_ASSERT_TRUE(strcmp(kid, "oidc-default") != 0);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_generate_kid_null_output);
    RUN_TEST(test_generate_kid_short_buffer);
    RUN_TEST(test_generate_kid_random_bytes_failure);
    RUN_TEST(test_generate_kid_base64url_encode_failure);
    RUN_TEST(test_generate_kid_success);
    return UNITY_END();
}
