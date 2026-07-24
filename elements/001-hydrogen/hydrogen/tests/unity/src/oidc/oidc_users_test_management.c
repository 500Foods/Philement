/*
 * Unity Test File: oidc_users management stubs
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_users.h>

#include <string.h>

void test_init_and_cleanup(void);
void test_cleanup_null_and_storage(void);
void test_authenticate_paths(void);
void test_create_user_paths(void);
void test_get_user_info_paths(void);
void test_update_user_paths(void);

void setUp(void) {
}

void tearDown(void) {
}

static void free_auth_result(OIDCAuthResult *r) {
    free(r->user_id);
    free(r->error);
    r->user_id = NULL;
    r->error = NULL;
}

void test_init_and_cleanup(void) {
    OIDCUserContext *ctx = init_oidc_user_management(5, true, 8);
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_NULL(ctx->user_storage);
    TEST_ASSERT_EQUAL_size_t(0U, ctx->user_count);
    TEST_ASSERT_EQUAL_INT(5, ctx->max_failed_attempts);
    TEST_ASSERT_TRUE(ctx->require_email_verification);
    TEST_ASSERT_EQUAL_INT(8, ctx->password_min_length);
    cleanup_oidc_user_management(ctx);
}

void test_cleanup_null_and_storage(void) {
    cleanup_oidc_user_management(NULL);

    OIDCUserContext *ctx = init_oidc_user_management(3, false, 4);
    TEST_ASSERT_NOT_NULL(ctx);
    /* Exercise non-NULL user_storage branch (stub free path) */
    ctx->user_storage = (void*)0x1;
    cleanup_oidc_user_management(ctx);
}

void test_authenticate_paths(void) {
    OIDCAuthResult r;

    r = oidc_authenticate_user(NULL, "u", "p");
    TEST_ASSERT_FALSE(r.success);
    TEST_ASSERT_EQUAL_INT(AUTH_LEVEL_NONE, r.level);
    TEST_ASSERT_NOT_NULL(r.error);
    free_auth_result(&r);

    OIDCUserContext *ctx = init_oidc_user_management(5, false, 6);
    TEST_ASSERT_NOT_NULL(ctx);

    r = oidc_authenticate_user(ctx, NULL, "p");
    TEST_ASSERT_FALSE(r.success);
    TEST_ASSERT_NOT_NULL(r.error);
    free_auth_result(&r);

    r = oidc_authenticate_user(ctx, "u", NULL);
    TEST_ASSERT_FALSE(r.success);
    TEST_ASSERT_NOT_NULL(r.error);
    free_auth_result(&r);

    r = oidc_authenticate_user(ctx, "test_user", "any");
    TEST_ASSERT_TRUE(r.success);
    TEST_ASSERT_EQUAL_INT(AUTH_LEVEL_SINGLE_FACTOR, r.level);
    TEST_ASSERT_EQUAL_STRING("user_12345", r.user_id);
    TEST_ASSERT_NULL(r.error);
    free_auth_result(&r);

    r = oidc_authenticate_user(ctx, "other", "pass");
    TEST_ASSERT_FALSE(r.success);
    TEST_ASSERT_EQUAL_INT(AUTH_LEVEL_NONE, r.level);
    TEST_ASSERT_EQUAL_STRING("Invalid credentials", r.error);
    TEST_ASSERT_NULL(r.user_id);
    free_auth_result(&r);

    cleanup_oidc_user_management(ctx);
}

void test_create_user_paths(void) {
    TEST_ASSERT_NULL(oidc_create_user(NULL, "u", "e@x", "password1", "G", "F"));

    OIDCUserContext *ctx = init_oidc_user_management(5, false, 8);
    TEST_ASSERT_NOT_NULL(ctx);

    TEST_ASSERT_NULL(oidc_create_user(ctx, NULL, "e@x", "password1", "G", "F"));
    TEST_ASSERT_NULL(oidc_create_user(ctx, "u", NULL, "password1", "G", "F"));
    TEST_ASSERT_NULL(oidc_create_user(ctx, "u", "e@x", NULL, "G", "F"));

    /* password shorter than min length */
    TEST_ASSERT_NULL(oidc_create_user(ctx, "u", "e@x", "short", "G", "F"));

    char *uid = oidc_create_user(ctx, "u", "e@x", "password1", "Given", "Family");
    TEST_ASSERT_NOT_NULL(uid);
    TEST_ASSERT_EQUAL_STRING("user_12345", uid);
    free(uid);

    cleanup_oidc_user_management(ctx);
}

void test_get_user_info_paths(void) {
    TEST_ASSERT_NULL(oidc_get_user_info(NULL, "id"));
    OIDCUserContext *ctx = init_oidc_user_management(5, false, 8);
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_NULL(oidc_get_user_info(ctx, NULL));

    char *info = oidc_get_user_info(ctx, "user_12345");
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_TRUE(strstr(info, "\"sub\"") != NULL);
    TEST_ASSERT_TRUE(strstr(info, "user_12345") != NULL);
    TEST_ASSERT_TRUE(strstr(info, "test@example.com") != NULL);
    free(info);

    cleanup_oidc_user_management(ctx);
}

void test_update_user_paths(void) {
    TEST_ASSERT_EQUAL_INT(-1, oidc_update_user(NULL, "id", "f", "v"));
    OIDCUserContext *ctx = init_oidc_user_management(5, false, 8);
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_EQUAL_INT(-1, oidc_update_user(ctx, NULL, "f", "v"));
    TEST_ASSERT_EQUAL_INT(-1, oidc_update_user(ctx, "id", NULL, "v"));
    TEST_ASSERT_EQUAL_INT(-1, oidc_update_user(ctx, "id", "f", NULL));
    TEST_ASSERT_EQUAL_INT(0, oidc_update_user(ctx, "user_12345", "email", "n@e.com"));
    cleanup_oidc_user_management(ctx);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_and_cleanup);
    RUN_TEST(test_cleanup_null_and_storage);
    RUN_TEST(test_authenticate_paths);
    RUN_TEST(test_create_user_paths);
    RUN_TEST(test_get_user_info_paths);
    RUN_TEST(test_update_user_paths);
    return UNITY_END();
}
