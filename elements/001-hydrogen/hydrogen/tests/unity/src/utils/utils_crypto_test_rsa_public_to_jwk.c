/*
 * Unity Test File: utils_rsa_public_to_jwk() and PEM helpers
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/utils/utils_crypto.h>
#include <string.h>
#include <openssl/evp.h>
#include <jansson.h>

void test_rsa_jwk_null_pkey(void);
void test_rsa_jwk_round_trip_parse(void);
void test_rsa_pem_round_trip(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_rsa_jwk_null_pkey(void) {
    TEST_ASSERT_NULL(utils_rsa_public_to_jwk(NULL, "kid1"));
    TEST_ASSERT_NULL(utils_rsa_private_to_pem(NULL));
    TEST_ASSERT_NULL(utils_rsa_public_to_pem(NULL));
    TEST_ASSERT_NULL(utils_rsa_private_from_pem(NULL));
    TEST_ASSERT_NULL(utils_rsa_private_from_pem(""));
}

void test_rsa_jwk_round_trip_parse(void) {
    EVP_PKEY* pkey = utils_rsa_generate_keypair(2048);
    if (!pkey) {
        TEST_IGNORE_MESSAGE("RSA keygen unavailable");
        return;
    }

    char* jwk = utils_rsa_public_to_jwk(pkey, "test-kid-1");
    TEST_ASSERT_NOT_NULL(jwk);

    json_error_t err;
    json_t* root = json_loads(jwk, 0, &err);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(json_is_object(root));
    TEST_ASSERT_EQUAL_STRING("RSA", json_string_value(json_object_get(root, "kty")));
    TEST_ASSERT_EQUAL_STRING("RS256", json_string_value(json_object_get(root, "alg")));
    TEST_ASSERT_EQUAL_STRING("sig", json_string_value(json_object_get(root, "use")));
    TEST_ASSERT_EQUAL_STRING("test-kid-1", json_string_value(json_object_get(root, "kid")));
    TEST_ASSERT_TRUE(json_is_string(json_object_get(root, "n")));
    TEST_ASSERT_TRUE(json_is_string(json_object_get(root, "e")));
    const char* n_val = json_string_value(json_object_get(root, "n"));
    TEST_ASSERT_TRUE(n_val != NULL && strlen(n_val) > 20U);
    json_decref(root);

    EVP_PKEY* pub = utils_jwk_rsa_to_pkey(jwk);
    TEST_ASSERT_NOT_NULL(pub);

    const unsigned char msg[] = "jwk-round-trip";
    unsigned char* sig = NULL;
    size_t sig_len = 0;
    TEST_ASSERT_TRUE(utils_rs256_sign(pkey, msg, sizeof(msg) - 1U, &sig, &sig_len));
    TEST_ASSERT_TRUE(utils_rs256_verify(pub, msg, sizeof(msg) - 1U, sig, sig_len));

    free(sig);
    free(jwk);
    EVP_PKEY_free(pub);
    EVP_PKEY_free(pkey);
}

void test_rsa_pem_round_trip(void) {
    EVP_PKEY* pkey = utils_rsa_generate_keypair(2048);
    if (!pkey) {
        TEST_IGNORE_MESSAGE("RSA keygen unavailable");
        return;
    }

    char* priv_pem = utils_rsa_private_to_pem(pkey);
    char* pub_pem = utils_rsa_public_to_pem(pkey);
    TEST_ASSERT_NOT_NULL(priv_pem);
    TEST_ASSERT_NOT_NULL(pub_pem);
    TEST_ASSERT_NOT_NULL(strstr(priv_pem, "BEGIN"));
    TEST_ASSERT_NOT_NULL(strstr(pub_pem, "BEGIN"));

    EVP_PKEY* loaded = utils_rsa_private_from_pem(priv_pem);
    TEST_ASSERT_NOT_NULL(loaded);

    const unsigned char msg[] = "pem-round-trip";
    unsigned char* sig = NULL;
    size_t sig_len = 0;
    TEST_ASSERT_TRUE(utils_rs256_sign(loaded, msg, sizeof(msg) - 1U, &sig, &sig_len));
    TEST_ASSERT_TRUE(utils_rs256_verify(pkey, msg, sizeof(msg) - 1U, sig, sig_len));

    free(sig);
    free(priv_pem);
    free(pub_pem);
    EVP_PKEY_free(loaded);
    EVP_PKEY_free(pkey);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_rsa_jwk_null_pkey);
    RUN_TEST(test_rsa_jwk_round_trip_parse);
    RUN_TEST(test_rsa_pem_round_trip);
    return UNITY_END();
}
