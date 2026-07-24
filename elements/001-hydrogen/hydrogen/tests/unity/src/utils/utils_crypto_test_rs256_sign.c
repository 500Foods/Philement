/*
 * Unity Test File: utils_rs256_sign()
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/utils/utils_crypto.h>
#include <openssl/evp.h>

void test_rs256_sign_null_params(void);
void test_rs256_sign_round_trip_verify(void);
void test_rs256_sign_wrong_data_fails_verify(void);
void test_rs256_sign_public_only_key_fails(void);
void test_rs256_sign_x25519_key_fails_init(void);

static EVP_PKEY* make_x25519_key(void) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_from_name(NULL, "X25519", NULL);
    if (!ctx) {
        return NULL;
    }
    EVP_PKEY* pkey = NULL;
    if (EVP_PKEY_keygen_init(ctx) <= 0 || EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        if (pkey) {
            EVP_PKEY_free(pkey);
            pkey = NULL;
        }
    }
    EVP_PKEY_CTX_free(ctx);
    return pkey;
}

void setUp(void) {
}

void tearDown(void) {
}

void test_rs256_sign_null_params(void) {
    unsigned char* sig = NULL;
    size_t sig_len = 0;
    const unsigned char data[] = "x";
    EVP_PKEY* pkey = utils_rsa_generate_keypair(2048);

    TEST_ASSERT_FALSE(utils_rs256_sign(NULL, data, 1, &sig, &sig_len));
    TEST_ASSERT_FALSE(utils_rs256_sign(pkey, NULL, 1, &sig, &sig_len));
    TEST_ASSERT_FALSE(utils_rs256_sign(pkey, data, 0, &sig, &sig_len));
    TEST_ASSERT_FALSE(utils_rs256_sign(pkey, data, 1, NULL, &sig_len));
    TEST_ASSERT_FALSE(utils_rs256_sign(pkey, data, 1, &sig, NULL));

    if (pkey) {
        EVP_PKEY_free(pkey);
    }
}

void test_rs256_sign_round_trip_verify(void) {
    EVP_PKEY* pkey = utils_rsa_generate_keypair(2048);
    if (!pkey) {
        TEST_IGNORE_MESSAGE("RSA keygen unavailable");
        return;
    }

    const unsigned char input[] = "header.payload";
    unsigned char* sig = NULL;
    size_t sig_len = 0;

    TEST_ASSERT_TRUE(utils_rs256_sign(pkey, input, sizeof(input) - 1U, &sig, &sig_len));
    TEST_ASSERT_NOT_NULL(sig);
    TEST_ASSERT_TRUE(sig_len > 0U);
    TEST_ASSERT_TRUE(utils_rs256_verify(pkey, input, sizeof(input) - 1U, sig, sig_len));

    free(sig);
    EVP_PKEY_free(pkey);
}

void test_rs256_sign_wrong_data_fails_verify(void) {
    EVP_PKEY* pkey = utils_rsa_generate_keypair(2048);
    if (!pkey) {
        TEST_IGNORE_MESSAGE("RSA keygen unavailable");
        return;
    }

    const unsigned char input[] = "original";
    const unsigned char other[] = "tampered";
    unsigned char* sig = NULL;
    size_t sig_len = 0;

    TEST_ASSERT_TRUE(utils_rs256_sign(pkey, input, sizeof(input) - 1U, &sig, &sig_len));
    TEST_ASSERT_FALSE(utils_rs256_verify(pkey, other, sizeof(other) - 1U, sig, sig_len));

    free(sig);
    EVP_PKEY_free(pkey);
}

void test_rs256_sign_public_only_key_fails(void) {
    EVP_PKEY* priv = utils_rsa_generate_keypair(2048);
    if (!priv) {
        TEST_IGNORE_MESSAGE("RSA keygen unavailable");
        return;
    }
    char* jwk = utils_rsa_public_to_jwk(priv, NULL);
    TEST_ASSERT_NOT_NULL(jwk);
    EVP_PKEY* pub = utils_jwk_rsa_to_pkey(jwk);
    TEST_ASSERT_NOT_NULL(pub);

    const unsigned char input[] = "cannot-sign-with-public";
    unsigned char* sig = NULL;
    size_t sig_len = 0;
    TEST_ASSERT_FALSE(utils_rs256_sign(pub, input, sizeof(input) - 1U, &sig, &sig_len));
    TEST_ASSERT_NULL(sig);

    free(jwk);
    EVP_PKEY_free(pub);
    EVP_PKEY_free(priv);
}

void test_rs256_sign_x25519_key_fails_init(void) {
    EVP_PKEY* x = make_x25519_key();
    if (!x) {
        TEST_IGNORE_MESSAGE("X25519 keygen unavailable");
        return;
    }
    const unsigned char input[] = "rs256-not-x25519";
    unsigned char* sig = NULL;
    size_t sig_len = 0;
    TEST_ASSERT_FALSE(utils_rs256_sign(x, input, sizeof(input) - 1U, &sig, &sig_len));
    TEST_ASSERT_NULL(sig);
    EVP_PKEY_free(x);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_rs256_sign_null_params);
    RUN_TEST(test_rs256_sign_round_trip_verify);
    RUN_TEST(test_rs256_sign_wrong_data_fails_verify);
    RUN_TEST(test_rs256_sign_public_only_key_fails);
    RUN_TEST(test_rs256_sign_x25519_key_fails_init);
    return UNITY_END();
}
