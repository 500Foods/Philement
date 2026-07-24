/*
 * Unity Test File: oidc_rotate_keys
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_keys.h>

#include <jansson.h>
#include <string.h>

void test_rotate_null(void);
void test_rotate_adds_second_key_to_jwks(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_rotate_null(void) {
    TEST_ASSERT_FALSE(oidc_rotate_keys(NULL));
}

void test_rotate_adds_second_key_to_jwks(void) {
    OIDCKeyContext *ctx = init_oidc_key_management(NULL, false, 90);
    TEST_ASSERT_NOT_NULL(ctx);
    OIDCKey *before = oidc_get_active_signing_key(ctx);
    TEST_ASSERT_NOT_NULL(before);
    char kid_before[128];
    snprintf(kid_before, sizeof(kid_before), "%s", before->kid);

    TEST_ASSERT_TRUE(oidc_rotate_keys(ctx));
    OIDCKey *after = oidc_get_active_signing_key(ctx);
    TEST_ASSERT_NOT_NULL(after);
    TEST_ASSERT_TRUE(strcmp(kid_before, after->kid) != 0);

    char *jwks = oidc_generate_jwks(ctx);
    TEST_ASSERT_NOT_NULL(jwks);
    json_error_t err;
    json_t *root = json_loads(jwks, 0, &err);
    json_t *keys = json_object_get(root, "keys");
    TEST_ASSERT_TRUE(json_array_size(keys) >= 2);

    json_decref(root);
    free(jwks);
    cleanup_oidc_key_management(ctx);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_rotate_null);
    RUN_TEST(test_rotate_adds_second_key_to_jwks);
    return UNITY_END();
}
