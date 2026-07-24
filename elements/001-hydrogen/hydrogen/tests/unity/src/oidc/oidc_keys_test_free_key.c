/*
 * Unity Test File: oidc_free_key
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_keys.h>

void test_free_key_null(void);
void test_free_key_with_data(void);
void test_free_key_without_data(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_free_key_null(void) {
    oidc_free_key(NULL);
}

void test_free_key_with_data(void) {
    OIDCKeyContext *ctx = init_oidc_key_management(NULL, false, 90);
    TEST_ASSERT_NOT_NULL(ctx);
    OIDCKey *active = oidc_get_active_signing_key(ctx);
    TEST_ASSERT_NOT_NULL(active);

    /* Detach key so cleanup does not double-free */
    ctx->keys[0] = NULL;
    ctx->key_count = 0;
    oidc_free_key(active);
    cleanup_oidc_key_management(ctx);
}

void test_free_key_without_data(void) {
    OIDCKey *key = (OIDCKey*)calloc(1, sizeof(OIDCKey));
    TEST_ASSERT_NOT_NULL(key);
    oidc_free_key(key);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_free_key_null);
    RUN_TEST(test_free_key_with_data);
    RUN_TEST(test_free_key_without_data);
    return UNITY_END();
}
