/*
 * Unity Test File: oidc_context_add_key
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_keys.h>
#include <unity/mocks/mock_system.h>

/* Matches OIDC_MAX_KEYS in oidc_keys.c */
#define TEST_OIDC_MAX_KEYS 16

void test_context_add_key_null_context(void);
void test_context_add_key_null_key(void);
void test_context_add_key_full_context(void);
void test_context_add_key_realloc_failure(void);
void test_context_add_key_success(void);

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
}

void test_context_add_key_null_context(void) {
    OIDCKey key = {0};
    TEST_ASSERT_FALSE(oidc_context_add_key(NULL, &key));
}

void test_context_add_key_null_key(void) {
    OIDCKeyContext ctx = {0};
    TEST_ASSERT_FALSE(oidc_context_add_key(&ctx, NULL));
}

void test_context_add_key_full_context(void) {
    OIDCKeyContext ctx = {0};
    ctx.keys = (OIDCKey**)calloc((size_t)TEST_OIDC_MAX_KEYS, sizeof(OIDCKey*));
    TEST_ASSERT_NOT_NULL(ctx.keys);
    ctx.key_count = (size_t)TEST_OIDC_MAX_KEYS;

    OIDCKey *key = (OIDCKey*)calloc(1, sizeof(OIDCKey));
    TEST_ASSERT_NOT_NULL(key);
    TEST_ASSERT_FALSE(oidc_context_add_key(&ctx, key));

    free(ctx.keys);
    free(key);
}

void test_context_add_key_realloc_failure(void) {
    OIDCKeyContext ctx = {0};
    ctx.keys = (OIDCKey**)calloc(1, sizeof(OIDCKey*));
    TEST_ASSERT_NOT_NULL(ctx.keys);
    ctx.key_count = 1;

    OIDCKey *key = (OIDCKey*)calloc(1, sizeof(OIDCKey));
    TEST_ASSERT_NOT_NULL(key);

    mock_system_set_realloc_failure(1);
    TEST_ASSERT_FALSE(oidc_context_add_key(&ctx, key));

    mock_system_reset_all();
    free(ctx.keys);
    free(key);
}

void test_context_add_key_success(void) {
    OIDCKeyContext ctx = {0};

    OIDCKey *key = (OIDCKey*)calloc(1, sizeof(OIDCKey));
    TEST_ASSERT_NOT_NULL(key);
    TEST_ASSERT_TRUE(oidc_context_add_key(&ctx, key));
    TEST_ASSERT_EQUAL_INT(1, (int)ctx.key_count);
    TEST_ASSERT_EQUAL_PTR(key, ctx.keys[0]);

    free(ctx.keys);
    free(key);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_context_add_key_null_context);
    RUN_TEST(test_context_add_key_null_key);
    RUN_TEST(test_context_add_key_full_context);
    RUN_TEST(test_context_add_key_realloc_failure);
    RUN_TEST(test_context_add_key_success);
    return UNITY_END();
}
