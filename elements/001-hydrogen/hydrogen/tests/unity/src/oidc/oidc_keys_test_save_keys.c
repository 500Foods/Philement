/*
 * Unity Test File: oidc_save_keys_to_storage
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_keys.h>
#include <unity/mocks/mock_system.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void test_save_keys_null_context(void);
void test_save_keys_null_storage_path(void);
void test_save_keys_no_active_key(void);
void test_save_keys_success(void);
void test_save_keys_build_path_failure(void);

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
}

void test_save_keys_null_context(void) {
    TEST_ASSERT_FALSE(oidc_save_keys_to_storage(NULL));
}

void test_save_keys_null_storage_path(void) {
    OIDCKeyContext ctx = {0};
    TEST_ASSERT_FALSE(oidc_save_keys_to_storage(&ctx));
}

void test_save_keys_no_active_key(void) {
    char template[] = "/tmp/oidc_save_empty_XXXXXX";
    char *dir = mkdtemp(template);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCKeyContext ctx = {0};
    ctx.storage_path = dir;
    TEST_ASSERT_FALSE(oidc_save_keys_to_storage(&ctx));
    rmdir(dir);
}

void test_save_keys_success(void) {
    char template[] = "/tmp/oidc_save_ok_XXXXXX";
    char *dir = mkdtemp(template);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCKeyContext *ctx = init_oidc_key_management(NULL, false, 90);
    TEST_ASSERT_NOT_NULL(ctx);
    ctx->storage_path = strdup(dir);
    TEST_ASSERT_NOT_NULL(ctx->storage_path);
    TEST_ASSERT_TRUE(oidc_save_keys_to_storage(ctx));

    char path[512];
    snprintf(path, sizeof(path), "%s/signing-active.pem", dir);
    FILE *fp = fopen(path, "r");
    TEST_ASSERT_NOT_NULL(fp);
    fclose(fp);

    cleanup_oidc_key_management(ctx);
    unlink(path);
    snprintf(path, sizeof(path), "%s/signing-active.kid", dir);
    unlink(path);
    rmdir(dir);
}

void test_save_keys_build_path_failure(void) {
    char template[] = "/tmp/oidc_save_path_XXXXXX";
    char *dir = mkdtemp(template);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCKeyContext *ctx = init_oidc_key_management(NULL, false, 90);
    TEST_ASSERT_NOT_NULL(ctx);
    ctx->storage_path = strdup(dir);
    TEST_ASSERT_NOT_NULL(ctx->storage_path);

    /* Fail first malloc inside oidc_build_storage_path (pem_path) */
    mock_system_set_malloc_failure(1);
    TEST_ASSERT_FALSE(oidc_save_keys_to_storage(ctx));
    mock_system_reset_all();

    cleanup_oidc_key_management(ctx);
    rmdir(dir);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_save_keys_null_context);
    RUN_TEST(test_save_keys_null_storage_path);
    RUN_TEST(test_save_keys_no_active_key);
    RUN_TEST(test_save_keys_success);
    RUN_TEST(test_save_keys_build_path_failure);
    return UNITY_END();
}
