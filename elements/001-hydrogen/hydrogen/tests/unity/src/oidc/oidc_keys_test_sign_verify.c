/*
 * Unity Test File: oidc_sign_data / oidc_verify_signature
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_keys.h>

/* Non-null sentinel for key_data null-check branches (never dereferenced). */
static int dummy_key_data;

void test_sign_data_null_key(void);
void test_sign_data_null_key_data(void);
void test_sign_data_null_data(void);
void test_sign_data_zero_data_len(void);
void test_sign_data_null_signature(void);
void test_sign_data_null_signature_len(void);

void test_verify_signature_null_key(void);
void test_verify_signature_null_key_data(void);
void test_verify_signature_null_data(void);
void test_verify_signature_zero_data_len(void);
void test_verify_signature_null_signature(void);
void test_verify_signature_zero_signature_len(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_sign_data_null_key(void) {
    unsigned char *sig = NULL;
    size_t sig_len = 0;
    TEST_ASSERT_FALSE(oidc_sign_data(NULL, (const unsigned char*)"data", 4, &sig, &sig_len));
}

void test_sign_data_null_key_data(void) {
    OIDCKey key = {0};
    unsigned char *sig = NULL;
    size_t sig_len = 0;
    TEST_ASSERT_FALSE(oidc_sign_data(&key, (const unsigned char*)"data", 4, &sig, &sig_len));
}

void test_sign_data_null_data(void) {
    OIDCKey key = {0};
    key.key_data = &dummy_key_data;
    unsigned char *sig = NULL;
    size_t sig_len = 0;
    TEST_ASSERT_FALSE(oidc_sign_data(&key, NULL, 4, &sig, &sig_len));
}

void test_sign_data_zero_data_len(void) {
    OIDCKey key = {0};
    key.key_data = &dummy_key_data;
    unsigned char *sig = NULL;
    size_t sig_len = 0;
    TEST_ASSERT_FALSE(oidc_sign_data(&key, (const unsigned char*)"data", 0, &sig, &sig_len));
}

void test_sign_data_null_signature(void) {
    OIDCKey key = {0};
    key.key_data = &dummy_key_data;
    TEST_ASSERT_FALSE(oidc_sign_data(&key, (const unsigned char*)"data", 4, NULL, NULL));
}

void test_sign_data_null_signature_len(void) {
    OIDCKey key = {0};
    key.key_data = &dummy_key_data;
    unsigned char *sig = NULL;
    TEST_ASSERT_FALSE(oidc_sign_data(&key, (const unsigned char*)"data", 4, &sig, NULL));
}

void test_verify_signature_null_key(void) {
    unsigned char *sig = NULL;
    size_t sig_len = 0;
    TEST_ASSERT_FALSE(oidc_verify_signature(NULL, (const unsigned char*)"data", 4, sig, sig_len));
}

void test_verify_signature_null_key_data(void) {
    OIDCKey key = {0};
    unsigned char *sig = NULL;
    size_t sig_len = 0;
    TEST_ASSERT_FALSE(oidc_verify_signature(&key, (const unsigned char*)"data", 4, sig, sig_len));
}

void test_verify_signature_null_data(void) {
    OIDCKey key = {0};
    key.key_data = &dummy_key_data;
    unsigned char *sig = NULL;
    size_t sig_len = 0;
    TEST_ASSERT_FALSE(oidc_verify_signature(&key, NULL, 4, sig, sig_len));
}

void test_verify_signature_zero_data_len(void) {
    OIDCKey key = {0};
    key.key_data = &dummy_key_data;
    unsigned char *sig = NULL;
    size_t sig_len = 0;
    TEST_ASSERT_FALSE(oidc_verify_signature(&key, (const unsigned char*)"data", 0, sig, sig_len));
}

void test_verify_signature_null_signature(void) {
    OIDCKey key = {0};
    key.key_data = &dummy_key_data;
    TEST_ASSERT_FALSE(oidc_verify_signature(&key, (const unsigned char*)"data", 4, NULL, 0));
}

void test_verify_signature_zero_signature_len(void) {
    OIDCKey key = {0};
    key.key_data = &dummy_key_data;
    unsigned char *sig = NULL;
    TEST_ASSERT_FALSE(oidc_verify_signature(&key, (const unsigned char*)"data", 4, sig, 0));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_sign_data_null_key);
    RUN_TEST(test_sign_data_null_key_data);
    RUN_TEST(test_sign_data_null_data);
    RUN_TEST(test_sign_data_zero_data_len);
    RUN_TEST(test_sign_data_null_signature);
    RUN_TEST(test_sign_data_null_signature_len);
    RUN_TEST(test_verify_signature_null_key);
    RUN_TEST(test_verify_signature_null_key_data);
    RUN_TEST(test_verify_signature_null_data);
    RUN_TEST(test_verify_signature_zero_data_len);
    RUN_TEST(test_verify_signature_null_signature);
    RUN_TEST(test_verify_signature_zero_signature_len);
    return UNITY_END();
}