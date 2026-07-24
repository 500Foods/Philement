/*
 * Unity Test File: init/shutdown and oidc_service_release_context
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_service.h>
#include <src/config/config_oidc.h>
#include <unity/mocks/mock_system.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

void test_init_null_config(void);
void test_init_with_database_name(void);
void test_init_malloc_context_failure(void);
void test_init_auth_code_store_failure(void);
void test_init_refresh_store_failure(void);
void test_init_key_management_failure(void);
void test_init_seed_client_asprintf_failure(void);
void test_release_null(void);
void test_shutdown_when_not_init(void);

static void cleanup_key_dir(const char *dir) {
    char path[512];
    snprintf(path, sizeof(path), "%s/signing-active.pem", dir);
    unlink(path);
    snprintf(path, sizeof(path), "%s/signing-active.kid", dir);
    unlink(path);
    rmdir(dir);
}

static void fill_minimal_cfg(OIDCConfig *cfg, char *dir) {
    memset(cfg, 0, sizeof(*cfg));
    cfg->enabled = true;
    cfg->issuer = (char*)"http://localhost:5450";
    cfg->keys.storage_path = dir;
    cfg->keys.encryption_enabled = false;
    cfg->keys.rotation_interval_days = 90;
    cfg->tokens.access_token_lifetime = 3600;
    cfg->tokens.refresh_token_lifetime = 86400;
    cfg->tokens.id_token_lifetime = 3600;
}

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    shutdown_oidc_service();
    mock_system_reset_all();
}

void test_init_null_config(void) {
    TEST_ASSERT_FALSE(init_oidc_service(NULL));
    TEST_ASSERT_NULL(get_oidc_context());
}

void test_init_with_database_name(void) {
    char tmpl[] = "/tmp/oidc_life_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCConfig cfg;
    fill_minimal_cfg(&cfg, dir);
    cfg.database = (char*)"accounts_db";
    TEST_ASSERT_TRUE(init_oidc_service(&cfg));

    OIDCContext *ctx = get_oidc_context();
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_TRUE(ctx->initialized);
    TEST_ASSERT_EQUAL_STRING("accounts_db", ctx->database_name);

    shutdown_oidc_service();
    TEST_ASSERT_NULL(get_oidc_context());
    cleanup_key_dir(dir);
}

void test_init_malloc_context_failure(void) {
    OIDCConfig cfg;
    fill_minimal_cfg(&cfg, (char*)"/tmp");
    mock_system_set_malloc_failure(1);
    TEST_ASSERT_FALSE(init_oidc_service(&cfg));
    TEST_ASSERT_NULL(get_oidc_context());
}

void test_init_auth_code_store_failure(void) {
    char tmpl[] = "/tmp/oidc_life_acs_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCConfig cfg;
    fill_minimal_cfg(&cfg, dir);
    /* 1 = OIDCContext malloc, 2 = auth_code_store calloc */
    mock_system_set_malloc_failure(2);
    TEST_ASSERT_FALSE(init_oidc_service(&cfg));
    TEST_ASSERT_NULL(get_oidc_context());
    rmdir(dir);
}

void test_init_refresh_store_failure(void) {
    char tmpl[] = "/tmp/oidc_life_ref_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCConfig cfg;
    fill_minimal_cfg(&cfg, dir);
    /* 1 = context, 2 = auth_code_store, 3 = refresh_store */
    mock_system_set_malloc_failure(3);
    TEST_ASSERT_FALSE(init_oidc_service(&cfg));
    TEST_ASSERT_NULL(get_oidc_context());
    rmdir(dir);
}

void test_init_key_management_failure(void) {
    char template[] = "/tmp/oidc_life_key_XXXXXX";
    char *dir = mkdtemp(template);
    TEST_ASSERT_NOT_NULL(dir);

    char path[512];
    snprintf(path, sizeof(path), "%s/not_a_dir", dir);
    FILE *fp = fopen(path, "w");
    TEST_ASSERT_NOT_NULL(fp);
    fprintf(fp, "x");
    fclose(fp);

    OIDCConfig cfg;
    fill_minimal_cfg(&cfg, path);
    TEST_ASSERT_FALSE(init_oidc_service(&cfg));
    TEST_ASSERT_NULL(get_oidc_context());

    unlink(path);
    rmdir(dir);
}

void test_init_seed_client_asprintf_failure(void) {
    char tmpl[] = "/tmp/oidc_life_seed_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCConfig cfg;
    fill_minimal_cfg(&cfg, dir);
    cfg.client_id = (char*)"seed-client";
    cfg.redirect_uri = (char*)"https://app.example/cb";

    mock_system_set_asprintf_failure(1);
    TEST_ASSERT_FALSE(init_oidc_service(&cfg));
    TEST_ASSERT_NULL(get_oidc_context());
    rmdir(dir);
}

void test_release_null(void) {
    oidc_service_release_context(NULL);
}

void test_shutdown_when_not_init(void) {
    shutdown_oidc_service();
    shutdown_oidc_service();
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_null_config);
    RUN_TEST(test_init_with_database_name);
    RUN_TEST(test_init_malloc_context_failure);
    RUN_TEST(test_init_auth_code_store_failure);
    RUN_TEST(test_init_refresh_store_failure);
    RUN_TEST(test_init_key_management_failure);
    RUN_TEST(test_init_seed_client_asprintf_failure);
    RUN_TEST(test_release_null);
    RUN_TEST(test_shutdown_when_not_init);
    return UNITY_END();
}
