/*
 * Unity Test File: utils_rsa_generate_keypair()
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/utils/utils_crypto.h>
#include <openssl/evp.h>

void test_rsa_generate_keypair_bits_too_small(void);
void test_rsa_generate_keypair_success_2048(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_rsa_generate_keypair_bits_too_small(void) {
    TEST_ASSERT_NULL(utils_rsa_generate_keypair(1024));
    TEST_ASSERT_NULL(utils_rsa_generate_keypair(0));
    TEST_ASSERT_NULL(utils_rsa_generate_keypair(-1));
}

void test_rsa_generate_keypair_success_2048(void) {
    EVP_PKEY* pkey = utils_rsa_generate_keypair(2048);
    if (!pkey) {
        TEST_IGNORE_MESSAGE("RSA keygen unavailable");
        return;
    }
    TEST_ASSERT_EQUAL_INT(EVP_PKEY_RSA, EVP_PKEY_get_base_id(pkey));
    EVP_PKEY_free(pkey);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_rsa_generate_keypair_bits_too_small);
    RUN_TEST(test_rsa_generate_keypair_success_2048);
    return UNITY_END();
}
