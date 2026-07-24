/*
 * Unity Test File: oidc_load_keys_from_storage
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_keys.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void test_load_keys_null_context(void);
void test_load_keys_null_storage_path(void);
void test_load_keys_no_pem_file(void);
void test_load_keys_invalid_pem(void);
void test_load_keys_kid_newline_trim(void);
void test_load_keys_empty_kid_generates(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_load_keys_null_context(void) {
    TEST_ASSERT_FALSE(oidc_load_keys_from_storage(NULL));
}

void test_load_keys_null_storage_path(void) {
    OIDCKeyContext ctx = {0};
    TEST_ASSERT_FALSE(oidc_load_keys_from_storage(&ctx));
}

void test_load_keys_no_pem_file(void) {
    char template[] = "/tmp/oidc_load_test_XXXXXX";
    char *dir = mkdtemp(template);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCKeyContext ctx = {0};
    ctx.storage_path = dir;
    TEST_ASSERT_FALSE(oidc_load_keys_from_storage(&ctx));

    rmdir(dir);
}

void test_load_keys_invalid_pem(void) {
    char template[] = "/tmp/oidc_load_bad_XXXXXX";
    char *dir = mkdtemp(template);
    TEST_ASSERT_NOT_NULL(dir);

    char path[512];
    snprintf(path, sizeof(path), "%s/signing-active.pem", dir);
    TEST_ASSERT_TRUE(oidc_write_text_file(path, "not-a-pem"));
    snprintf(path, sizeof(path), "%s/signing-active.kid", dir);
    TEST_ASSERT_TRUE(oidc_write_text_file(path, "some-kid\n"));

    OIDCKeyContext ctx = {0};
    ctx.storage_path = dir;
    TEST_ASSERT_FALSE(oidc_load_keys_from_storage(&ctx));

    snprintf(path, sizeof(path), "%s/signing-active.pem", dir);
    unlink(path);
    snprintf(path, sizeof(path), "%s/signing-active.kid", dir);
    unlink(path);
    rmdir(dir);
}

void test_load_keys_kid_newline_trim(void) {
    char template[] = "/tmp/oidc_load_trim_XXXXXX";
    char *dir = mkdtemp(template);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCKeyContext *ctx1 = init_oidc_key_management(dir, false, 90);
    TEST_ASSERT_NOT_NULL(ctx1);
    OIDCKey *a1 = oidc_get_active_signing_key(ctx1);
    TEST_ASSERT_NOT_NULL(a1);
    char kid_expected[OIDC_KEY_ID_LENGTH + 1];
    snprintf(kid_expected, sizeof(kid_expected), "%s", a1->kid);
    cleanup_oidc_key_management(ctx1);

    /* Rewrite kid with trailing CR/LF */
    char path[512];
    snprintf(path, sizeof(path), "%s/signing-active.kid", dir);
    char kid_line[128];
    snprintf(kid_line, sizeof(kid_line), "%s\r\n", kid_expected);
    TEST_ASSERT_TRUE(oidc_write_text_file(path, kid_line));

    OIDCKeyContext ctx2 = {0};
    ctx2.storage_path = dir;
    TEST_ASSERT_TRUE(oidc_load_keys_from_storage(&ctx2));
    TEST_ASSERT_EQUAL_INT(1, (int)ctx2.key_count);
    TEST_ASSERT_EQUAL_STRING(kid_expected, ctx2.keys[0]->kid);

    oidc_free_key(ctx2.keys[0]);
    free(ctx2.keys);

    snprintf(path, sizeof(path), "%s/signing-active.pem", dir);
    unlink(path);
    snprintf(path, sizeof(path), "%s/signing-active.kid", dir);
    unlink(path);
    rmdir(dir);
}

void test_load_keys_empty_kid_generates(void) {
    char template[] = "/tmp/oidc_load_nokid_XXXXXX";
    char *dir = mkdtemp(template);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCKeyContext *ctx1 = init_oidc_key_management(dir, false, 90);
    TEST_ASSERT_NOT_NULL(ctx1);
    cleanup_oidc_key_management(ctx1);

    char path[512];
    snprintf(path, sizeof(path), "%s/signing-active.kid", dir);
    TEST_ASSERT_TRUE(oidc_write_text_file(path, ""));

    OIDCKeyContext ctx2 = {0};
    ctx2.storage_path = dir;
    TEST_ASSERT_TRUE(oidc_load_keys_from_storage(&ctx2));
    TEST_ASSERT_TRUE(strlen(ctx2.keys[0]->kid) > 0U);

    oidc_free_key(ctx2.keys[0]);
    free(ctx2.keys);

    snprintf(path, sizeof(path), "%s/signing-active.pem", dir);
    unlink(path);
    snprintf(path, sizeof(path), "%s/signing-active.kid", dir);
    unlink(path);
    rmdir(dir);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_load_keys_null_context);
    RUN_TEST(test_load_keys_null_storage_path);
    RUN_TEST(test_load_keys_no_pem_file);
    RUN_TEST(test_load_keys_invalid_pem);
    RUN_TEST(test_load_keys_kid_newline_trim);
    RUN_TEST(test_load_keys_empty_kid_generates);
    return UNITY_END();
}
