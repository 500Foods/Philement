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
void test_rsa_jwk_without_kid(void);
void test_rsa_jwk_rejects_non_rsa_key(void);
void test_rsa_private_pem_rejects_public_only(void);
void test_rsa_private_pem_rejects_hmac_key(void);
void test_rsa_public_pem_rejects_hmac_key(void);
void test_rsa_private_from_pem_invalid(void);

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

static EVP_PKEY* make_hmac_key(void) {
    unsigned char raw[32];
    memset(raw, 0x5A, sizeof(raw));
    return EVP_PKEY_new_raw_private_key(EVP_PKEY_HMAC, NULL, raw, sizeof(raw));
}

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

void test_rsa_jwk_without_kid(void) {
    EVP_PKEY* pkey = utils_rsa_generate_keypair(2048);
    if (!pkey) {
        TEST_IGNORE_MESSAGE("RSA keygen unavailable");
        return;
    }
    char* jwk = utils_rsa_public_to_jwk(pkey, NULL);
    TEST_ASSERT_NOT_NULL(jwk);
    TEST_ASSERT_NULL(strstr(jwk, "\"kid\""));
    char* jwk_empty = utils_rsa_public_to_jwk(pkey, "");
    TEST_ASSERT_NOT_NULL(jwk_empty);
    TEST_ASSERT_NULL(strstr(jwk_empty, "\"kid\""));
    free(jwk);
    free(jwk_empty);
    EVP_PKEY_free(pkey);
}

void test_rsa_jwk_rejects_non_rsa_key(void) {
    EVP_PKEY* x = make_x25519_key();
    if (!x) {
        TEST_IGNORE_MESSAGE("X25519 keygen unavailable");
        return;
    }
    TEST_ASSERT_NULL(utils_rsa_public_to_jwk(x, "k1"));
    EVP_PKEY_free(x);
}

void test_rsa_private_pem_rejects_public_only(void) {
    EVP_PKEY* priv = utils_rsa_generate_keypair(2048);
    if (!priv) {
        TEST_IGNORE_MESSAGE("RSA keygen unavailable");
        return;
    }
    char* jwk = utils_rsa_public_to_jwk(priv, "x");
    TEST_ASSERT_NOT_NULL(jwk);
    EVP_PKEY* pub = utils_jwk_rsa_to_pkey(jwk);
    TEST_ASSERT_NOT_NULL(pub);

    TEST_ASSERT_NULL(utils_rsa_private_to_pem(pub));

    char* pub_pem = utils_rsa_public_to_pem(pub);
    TEST_ASSERT_NOT_NULL(pub_pem);

    free(pub_pem);
    free(jwk);
    EVP_PKEY_free(pub);
    EVP_PKEY_free(priv);
}

void test_rsa_private_pem_rejects_hmac_key(void) {
    EVP_PKEY* hmac = make_hmac_key();
    if (!hmac) {
        TEST_IGNORE_MESSAGE("HMAC raw key unavailable");
        return;
    }
    TEST_ASSERT_NULL(utils_rsa_private_to_pem(hmac));
    EVP_PKEY_free(hmac);
}

void test_rsa_public_pem_rejects_hmac_key(void) {
    EVP_PKEY* hmac = make_hmac_key();
    if (!hmac) {
        TEST_IGNORE_MESSAGE("HMAC raw key unavailable");
        return;
    }
    TEST_ASSERT_NULL(utils_rsa_public_to_pem(hmac));
    EVP_PKEY_free(hmac);
}

void test_rsa_private_from_pem_invalid(void) {
    TEST_ASSERT_NULL(utils_rsa_private_from_pem("not-a-pem"));
    TEST_ASSERT_NULL(utils_rsa_private_from_pem(
        "-----BEGIN PRIVATE KEY-----\nAAAA\n-----END PRIVATE KEY-----\n"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_rsa_jwk_null_pkey);
    RUN_TEST(test_rsa_jwk_round_trip_parse);
    RUN_TEST(test_rsa_pem_round_trip);
    RUN_TEST(test_rsa_jwk_without_kid);
    RUN_TEST(test_rsa_jwk_rejects_non_rsa_key);
    RUN_TEST(test_rsa_private_pem_rejects_public_only);
    RUN_TEST(test_rsa_private_pem_rejects_hmac_key);
    RUN_TEST(test_rsa_public_pem_rejects_hmac_key);
    RUN_TEST(test_rsa_private_from_pem_invalid);
    return UNITY_END();
}
