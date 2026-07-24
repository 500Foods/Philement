/*
 * Unity Test File: oidc_generate_jwks
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_keys.h>
#include <jansson.h>
#include <string.h>

void test_generate_jwks_null_context(void);
void test_generate_jwks_empty_context(void);
void test_generate_jwks_skips_null_and_archived(void);
void test_generate_jwks_skips_missing_key_data(void);
void test_generate_jwks_with_active_key(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_generate_jwks_null_context(void) {
    TEST_ASSERT_NULL(oidc_generate_jwks(NULL));
}

void test_generate_jwks_empty_context(void) {
    OIDCKeyContext ctx = {0};
    char *jwks = oidc_generate_jwks(&ctx);
    TEST_ASSERT_NOT_NULL(jwks);
    json_error_t err;
    json_t *root = json_loads(jwks, 0, &err);
    TEST_ASSERT_NOT_NULL(root);
    json_t *keys = json_object_get(root, "keys");
    TEST_ASSERT_TRUE(json_is_array(keys));
    TEST_ASSERT_EQUAL_INT(0, (int)json_array_size(keys));
    json_decref(root);
    free(jwks);
}

void test_generate_jwks_skips_null_and_archived(void) {
    static int dummy_key_data;
    OIDCKey archived = {0};
    archived.status = KEY_STATUS_ARCHIVED;
    archived.usage = KEY_USAGE_SIGNING;
    archived.key_data = &dummy_key_data;

    OIDCKey *slot0 = NULL;
    OIDCKey *slot1 = &archived;
    OIDCKey *arr[2];
    arr[0] = slot0;
    arr[1] = slot1;

    OIDCKeyContext ctx = {0};
    ctx.keys = arr;
    ctx.key_count = 2;

    char *jwks = oidc_generate_jwks(&ctx);
    TEST_ASSERT_NOT_NULL(jwks);
    json_error_t err;
    json_t *root = json_loads(jwks, 0, &err);
    TEST_ASSERT_NOT_NULL(root);
    json_t *keys = json_object_get(root, "keys");
    TEST_ASSERT_EQUAL_INT(0, (int)json_array_size(keys));
    json_decref(root);
    free(jwks);
}

void test_generate_jwks_skips_missing_key_data(void) {
    OIDCKey key = {0};
    key.status = KEY_STATUS_ACTIVE;
    key.usage = KEY_USAGE_SIGNING;
    key.key_data = NULL;

    OIDCKey *arr[1];
    arr[0] = &key;

    OIDCKeyContext ctx = {0};
    ctx.keys = arr;
    ctx.key_count = 1;

    char *jwks = oidc_generate_jwks(&ctx);
    TEST_ASSERT_NOT_NULL(jwks);
    free(jwks);
}

void test_generate_jwks_with_active_key(void) {
    OIDCKeyContext *ctx = init_oidc_key_management(NULL, false, 90);
    TEST_ASSERT_NOT_NULL(ctx);

    char *jwks = oidc_generate_jwks(ctx);
    TEST_ASSERT_NOT_NULL(jwks);
    TEST_ASSERT_NOT_NULL(strstr(jwks, "\"keys\""));
    free(jwks);
    cleanup_oidc_key_management(ctx);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_generate_jwks_null_context);
    RUN_TEST(test_generate_jwks_empty_context);
    RUN_TEST(test_generate_jwks_skips_null_and_archived);
    RUN_TEST(test_generate_jwks_skips_missing_key_data);
    RUN_TEST(test_generate_jwks_with_active_key);
    return UNITY_END();
}
