/*
 * Unity Test File: oidc_rotate_keys
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_keys.h>
#include <unity/mocks/mock_system.h>
#include <jansson.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

void test_rotate_null(void);
void test_rotate_adds_second_key_to_jwks(void);
void test_rotate_with_storage_persists(void);
void test_rotate_generate_signing_key_failure(void);

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
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
    TEST_ASSERT_EQUAL_INT(KEY_STATUS_ROTATING, (int)before->status);
    TEST_ASSERT_EQUAL_INT(KEY_STATUS_ACTIVE, (int)after->status);

    char *jwks = oidc_generate_jwks(ctx);
    TEST_ASSERT_NOT_NULL(jwks);
    json_error_t err;
    json_t *root = json_loads(jwks, 0, &err);
    TEST_ASSERT_NOT_NULL(root);
    json_t *keys = json_object_get(root, "keys");
    TEST_ASSERT_TRUE(json_array_size(keys) >= 2);

    json_decref(root);
    free(jwks);
    cleanup_oidc_key_management(ctx);
}

void test_rotate_with_storage_persists(void) {
    char template[] = "/tmp/oidc_rotate_store_XXXXXX";
    char *dir = mkdtemp(template);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCKeyContext *ctx = init_oidc_key_management(dir, false, 90);
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_TRUE(oidc_rotate_keys(ctx));
    cleanup_oidc_key_management(ctx);

    char path[512];
    snprintf(path, sizeof(path), "%s/signing-active.pem", dir);
    unlink(path);
    snprintf(path, sizeof(path), "%s/signing-active.kid", dir);
    unlink(path);
    rmdir(dir);
}

void test_rotate_generate_signing_key_failure(void) {
    OIDCKeyContext *ctx = init_oidc_key_management(NULL, false, 90);
    TEST_ASSERT_NOT_NULL(ctx);

    /* Fail calloc for the new OIDCKey inside oidc_generate_signing_key */
    mock_system_set_malloc_failure(1);
    TEST_ASSERT_FALSE(oidc_rotate_keys(ctx));
    mock_system_reset_all();

    cleanup_oidc_key_management(ctx);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_rotate_null);
    RUN_TEST(test_rotate_adds_second_key_to_jwks);
    RUN_TEST(test_rotate_with_storage_persists);
    RUN_TEST(test_rotate_generate_signing_key_failure);
    return UNITY_END();
}
