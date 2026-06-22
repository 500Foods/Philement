/*
 * Unity Test File: utils_rs256_verify()
 * This file contains unit tests for RS256 signature verification
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/utils/utils_crypto.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

// Forward declarations for functions being tested
bool utils_rs256_verify(EVP_PKEY* pkey,
                         const unsigned char* input, size_t input_len,
                         const unsigned char* signature, size_t sig_len);
EVP_PKEY* utils_jwk_rsa_to_pkey(const char* jwk_json);

// Function prototypes for test functions
void test_rs256_verify_null_pkey(void);
void test_rs256_verify_null_input(void);
void test_rs256_verify_zero_input_len(void);
void test_rs256_verify_null_signature(void);
void test_rs256_verify_zero_sig_len(void);
void test_rs256_verify_all_null_params(void);
void test_rs256_verify_bad_signature(void);
void test_rs256_verify_valid_signature(void);
void test_rs256_verify_wrong_data(void);
void test_rs256_verify_truncated_signature(void);
void test_rs256_verify_garbage_signature(void);

// Module-level key pair for tests that need a real key
static EVP_PKEY* g_test_pkey = NULL;
static EVP_PKEY* g_test_private_key = NULL;

void setUp(void) {
    // No per-test setup needed
}

void tearDown(void) {
    // No per-test teardown needed
}

// Helper: generate a 2048-bit RSA key pair for signing/verifying tests
static void generate_test_keypair(void) {
    if (g_test_private_key != NULL) {
        return;
    }

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);
    if (!ctx) {
        return;
    }

    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return;
    }

    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return;
    }

    if (EVP_PKEY_keygen(ctx, &g_test_private_key) <= 0) {
        g_test_private_key = NULL;
    }

    EVP_PKEY_CTX_free(ctx);

    // Extract the public key
    if (g_test_private_key) {
        // Use the private key for verification as it also contains the public key
        g_test_pkey = g_test_private_key;
    }
}

// Helper: sign data with RSA-SHA256
static unsigned char* sign_data(const unsigned char* data, size_t data_len,
                                  size_t* sig_len) {
    if (!g_test_private_key || !data || data_len == 0 || !sig_len) {
        return NULL;
    }

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        return NULL;
    }

    if (EVP_DigestSignInit(mdctx, NULL, EVP_sha256(), NULL, g_test_private_key) != 1) {
        EVP_MD_CTX_free(mdctx);
        return NULL;
    }

    if (EVP_DigestSignUpdate(mdctx, data, data_len) != 1) {
        EVP_MD_CTX_free(mdctx);
        return NULL;
    }

    // Get signature length
    if (EVP_DigestSignFinal(mdctx, NULL, sig_len) != 1) {
        EVP_MD_CTX_free(mdctx);
        return NULL;
    }

    unsigned char* sig = malloc(*sig_len);
    if (!sig) {
        EVP_MD_CTX_free(mdctx);
        return NULL;
    }

    if (EVP_DigestSignFinal(mdctx, sig, sig_len) != 1) {
        free(sig);
        EVP_MD_CTX_free(mdctx);
        return NULL;
    }

    EVP_MD_CTX_free(mdctx);
    return sig;
}

// Test null pkey parameter
void test_rs256_verify_null_pkey(void) {
    const unsigned char input[] = "test data";
    unsigned char sig[256] = {0};

    bool result = utils_rs256_verify(NULL, input, sizeof(input) - 1, sig, sizeof(sig));
    TEST_ASSERT_FALSE(result);
}

// Test null input parameter
void test_rs256_verify_null_input(void) {
    generate_test_keypair();
    if (!g_test_pkey) {
        TEST_IGNORE_MESSAGE("Could not generate test key pair, skipping test");
        return;
    }

    unsigned char sig[256] = {0};
    bool result = utils_rs256_verify(g_test_pkey, NULL, 10, sig, sizeof(sig));
    TEST_ASSERT_FALSE(result);
}

// Test zero input length
void test_rs256_verify_zero_input_len(void) {
    generate_test_keypair();
    if (!g_test_pkey) {
        TEST_IGNORE_MESSAGE("Could not generate test key pair, skipping test");
        return;
    }

    const unsigned char input[] = "test data";
    unsigned char sig[256] = {0};

    bool result = utils_rs256_verify(g_test_pkey, input, 0, sig, sizeof(sig));
    TEST_ASSERT_FALSE(result);
}

// Test null signature parameter
void test_rs256_verify_null_signature(void) {
    generate_test_keypair();
    if (!g_test_pkey) {
        TEST_IGNORE_MESSAGE("Could not generate test key pair, skipping test");
        return;
    }

    const unsigned char input[] = "test data";
    bool result = utils_rs256_verify(g_test_pkey, input, sizeof(input) - 1, NULL, 256);
    TEST_ASSERT_FALSE(result);
}

// Test zero signature length
void test_rs256_verify_zero_sig_len(void) {
    generate_test_keypair();
    if (!g_test_pkey) {
        TEST_IGNORE_MESSAGE("Could not generate test key pair, skipping test");
        return;
    }

    const unsigned char input[] = "test data";
    const unsigned char sig[256] = {0};

    bool result = utils_rs256_verify(g_test_pkey, input, sizeof(input) - 1, sig, 0);
    TEST_ASSERT_FALSE(result);
}

// Test all null parameters
void test_rs256_verify_all_null_params(void) {
    bool result = utils_rs256_verify(NULL, NULL, 0, NULL, 0);
    TEST_ASSERT_FALSE(result);
}

// Test bad (garbage) signature returns false
void test_rs256_verify_bad_signature(void) {
    generate_test_keypair();
    if (!g_test_pkey) {
        TEST_IGNORE_MESSAGE("Could not generate test key pair, skipping test");
        return;
    }

    const unsigned char input[] = "test data for bad sig";
    // Use garbage bytes as signature - should fail verification
    unsigned char bad_sig[256];
    memset(bad_sig, 0x42, sizeof(bad_sig));

    bool result = utils_rs256_verify(g_test_pkey, input, sizeof(input) - 1,
                                     bad_sig, sizeof(bad_sig));
    TEST_ASSERT_FALSE(result);
}

// Test valid signature verification (success path)
void test_rs256_verify_valid_signature(void) {
    generate_test_keypair();
    if (!g_test_pkey) {
        TEST_IGNORE_MESSAGE("Could not generate test key pair, skipping test");
        return;
    }

    const unsigned char input[] = "test.payload";
    size_t sig_len = 0;
    unsigned char* sig = sign_data(input, sizeof(input) - 1, &sig_len);

    if (!sig) {
        TEST_IGNORE_MESSAGE("Could not generate test signature, skipping test");
        return;
    }

    bool result = utils_rs256_verify(g_test_pkey, input, sizeof(input) - 1,
                                     sig, sig_len);
    TEST_ASSERT_TRUE(result);

    free(sig);
}

// Test verification fails with different data (wrong signing input)
void test_rs256_verify_wrong_data(void) {
    generate_test_keypair();
    if (!g_test_pkey) {
        TEST_IGNORE_MESSAGE("Could not generate test key pair, skipping test");
        return;
    }

    const unsigned char original[] = "original data";
    const unsigned char different[] = "different data";
    size_t sig_len = 0;
    unsigned char* sig = sign_data(original, sizeof(original) - 1, &sig_len);

    if (!sig) {
        TEST_IGNORE_MESSAGE("Could not generate test signature, skipping test");
        return;
    }

    // Signature was for "original data", should fail for "different data"
    bool result = utils_rs256_verify(g_test_pkey, different, sizeof(different) - 1,
                                     sig, sig_len);
    TEST_ASSERT_FALSE(result);

    free(sig);
}

// Test verification fails with truncated signature
void test_rs256_verify_truncated_signature(void) {
    generate_test_keypair();
    if (!g_test_pkey) {
        TEST_IGNORE_MESSAGE("Could not generate test key pair, skipping test");
        return;
    }

    const unsigned char input[] = "test data";
    size_t sig_len = 0;
    unsigned char* sig = sign_data(input, sizeof(input) - 1, &sig_len);

    if (!sig || sig_len < 10) {
        free(sig);
        TEST_IGNORE_MESSAGE("Could not generate test signature, skipping test");
        return;
    }

    // Truncate signature to just 10 bytes - should fail
    bool result = utils_rs256_verify(g_test_pkey, input, sizeof(input) - 1, sig, 10);
    TEST_ASSERT_FALSE(result);

    free(sig);
}

// Test verification fails with all-zero signature bytes
void test_rs256_verify_garbage_signature(void) {
    generate_test_keypair();
    if (!g_test_pkey) {
        TEST_IGNORE_MESSAGE("Could not generate test key pair, skipping test");
        return;
    }

    const unsigned char input[] = "test.input.data";
    // All zeros is not a valid RSA signature
    unsigned char zero_sig[256];
    memset(zero_sig, 0x00, sizeof(zero_sig));

    bool result = utils_rs256_verify(g_test_pkey, input, sizeof(input) - 1,
                                     zero_sig, sizeof(zero_sig));
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests (null/zero inputs)
    RUN_TEST(test_rs256_verify_null_pkey);
    RUN_TEST(test_rs256_verify_null_input);
    RUN_TEST(test_rs256_verify_zero_input_len);
    RUN_TEST(test_rs256_verify_null_signature);
    RUN_TEST(test_rs256_verify_zero_sig_len);
    RUN_TEST(test_rs256_verify_all_null_params);

    // Invalid signature tests (bad signature path)
    RUN_TEST(test_rs256_verify_bad_signature);
    RUN_TEST(test_rs256_verify_garbage_signature);
    RUN_TEST(test_rs256_verify_truncated_signature);

    // Valid signature test (success path)
    RUN_TEST(test_rs256_verify_valid_signature);

    // Wrong data tests (valid sig but wrong input)
    RUN_TEST(test_rs256_verify_wrong_data);

    // Free shared key pair
    if (g_test_private_key) {
        EVP_PKEY_free(g_test_private_key);
        g_test_private_key = NULL;
        g_test_pkey = NULL;
    }

    return UNITY_END();
}
