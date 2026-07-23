/*
 * Unity Test File: init_oidc_key_management / JWKS / persistence
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_keys.h>
#include <src/utils/utils_crypto.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <jansson.h>

void test_oidc_keys_init_null_storage_generates_key(void);
void test_oidc_keys_persist_and_reload_same_kid(void);
void test_oidc_keys_jwks_has_real_modulus(void);
void test_oidc_keys_sign_verify_and_find(void);
void test_oidc_keys_unknown_kid(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_oidc_keys_init_null_storage_generates_key(void) {
    OIDCKeyContext *ctx = init_oidc_key_management(NULL, false, 90);
    TEST_ASSERT_NOT_NULL(ctx);
    OIDCKey *active = oidc_get_active_signing_key(ctx);
    TEST_ASSERT_NOT_NULL(active);
    TEST_ASSERT_TRUE(strlen(active->kid) > 0U);
    cleanup_oidc_key_management(ctx);
}

void test_oidc_keys_persist_and_reload_same_kid(void) {
    char template[] = "/tmp/oidc_keys_test_XXXXXX";
    char *dir = mkdtemp(template);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCKeyContext *ctx1 = init_oidc_key_management(dir, false, 90);
    TEST_ASSERT_NOT_NULL(ctx1);
    OIDCKey *a1 = oidc_get_active_signing_key(ctx1);
    TEST_ASSERT_NOT_NULL(a1);
    char kid1[OIDC_KEY_ID_LENGTH + 1];
    snprintf(kid1, sizeof(kid1), "%s", a1->kid);
    cleanup_oidc_key_management(ctx1);

    OIDCKeyContext *ctx2 = init_oidc_key_management(dir, false, 90);
    TEST_ASSERT_NOT_NULL(ctx2);
    OIDCKey *a2 = oidc_get_active_signing_key(ctx2);
    TEST_ASSERT_NOT_NULL(a2);
    TEST_ASSERT_EQUAL_STRING(kid1, a2->kid);
    cleanup_oidc_key_management(ctx2);

    char path[512];
    snprintf(path, sizeof(path), "%s/signing-active.pem", dir);
    unlink(path);
    snprintf(path, sizeof(path), "%s/signing-active.kid", dir);
    unlink(path);
    rmdir(dir);
}

void test_oidc_keys_jwks_has_real_modulus(void) {
    OIDCKeyContext *ctx = init_oidc_key_management(NULL, false, 90);
    TEST_ASSERT_NOT_NULL(ctx);

    char *jwks = oidc_generate_jwks(ctx);
    TEST_ASSERT_NOT_NULL(jwks);
    TEST_ASSERT_NULL(strstr(jwks, "example-modulus"));

    json_error_t err;
    json_t *root = json_loads(jwks, 0, &err);
    TEST_ASSERT_NOT_NULL(root);
    json_t *keys = json_object_get(root, "keys");
    TEST_ASSERT_TRUE(json_is_array(keys));
    TEST_ASSERT_TRUE(json_array_size(keys) >= 1);
    json_t *k0 = json_array_get(keys, 0);
    const char *n = json_string_value(json_object_get(k0, "n"));
    TEST_ASSERT_NOT_NULL(n);
    TEST_ASSERT_TRUE(strlen(n) > 20U);
    json_decref(root);
    free(jwks);
    cleanup_oidc_key_management(ctx);
}

void test_oidc_keys_sign_verify_and_find(void) {
    OIDCKeyContext *ctx = init_oidc_key_management(NULL, false, 90);
    TEST_ASSERT_NOT_NULL(ctx);
    OIDCKey *active = oidc_get_active_signing_key(ctx);
    TEST_ASSERT_NOT_NULL(active);

    const unsigned char msg[] = "oidc-sign-test";
    unsigned char *sig = NULL;
    size_t sig_len = 0;
    TEST_ASSERT_TRUE(oidc_sign_data(active, msg, sizeof(msg) - 1U, &sig, &sig_len));
    TEST_ASSERT_TRUE(oidc_verify_signature(active, msg, sizeof(msg) - 1U, sig, sig_len));

    OIDCKey *found = oidc_find_key_by_id(ctx, active->kid);
    TEST_ASSERT_EQUAL_PTR(active, found);

    free(sig);
    cleanup_oidc_key_management(ctx);
}

void test_oidc_keys_unknown_kid(void) {
    OIDCKeyContext *ctx = init_oidc_key_management(NULL, false, 90);
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_NULL(oidc_find_key_by_id(ctx, "no-such-kid"));
    TEST_ASSERT_NULL(oidc_find_key_by_id(ctx, NULL));
    TEST_ASSERT_NULL(oidc_get_active_signing_key(NULL));
    cleanup_oidc_key_management(ctx);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_oidc_keys_init_null_storage_generates_key);
    RUN_TEST(test_oidc_keys_persist_and_reload_same_kid);
    RUN_TEST(test_oidc_keys_jwks_has_real_modulus);
    RUN_TEST(test_oidc_keys_sign_verify_and_find);
    RUN_TEST(test_oidc_keys_unknown_kid);
    return UNITY_END();
}
