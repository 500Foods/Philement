/*
 * Unity Test File: oidc_authenticate_resource_owner + oidc_get_accounts_database
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_service.h>
#include <src/config/config.h>

#include <string.h>

extern AppConfig *app_config;

void test_auth_ro_nulls(void);
void test_get_accounts_database_fallback_oidc(void);
void test_get_accounts_database_fallback_connection_name(void);
void test_get_accounts_database_fallback_connection_connection_name(void);
void test_get_accounts_database_default(void);
void test_process_authorization_legacy(void);

void setUp(void) {
}

void tearDown(void) {
    if (app_config) {
        free(app_config);
        app_config = NULL;
    }
}

void test_auth_ro_nulls(void) {
    int id = 99;
    TEST_ASSERT_FALSE(oidc_authenticate_resource_owner(NULL, "p", &id));
    TEST_ASSERT_FALSE(oidc_authenticate_resource_owner("u", NULL, &id));
    TEST_ASSERT_FALSE(oidc_authenticate_resource_owner("u", "p", NULL));
    /* No account backend in unit env → not found */
    TEST_ASSERT_FALSE(oidc_authenticate_resource_owner("nobody", "pass", &id));
    TEST_ASSERT_EQUAL_INT(0, id);
}

void test_get_accounts_database_default(void) {
    TEST_ASSERT_EQUAL_STRING("demo", oidc_get_accounts_database());
}

void test_get_accounts_database_fallback_oidc(void) {
    app_config = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(app_config);
    app_config->oidc.database = (char*)"from_oidc_cfg";
    TEST_ASSERT_EQUAL_STRING("from_oidc_cfg", oidc_get_accounts_database());
}

void test_get_accounts_database_fallback_connection_name(void) {
    app_config = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(app_config);
    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = (char*)"conn_name_db";
    TEST_ASSERT_EQUAL_STRING("conn_name_db", oidc_get_accounts_database());
}

void test_get_accounts_database_fallback_connection_connection_name(void) {
    app_config = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(app_config);
    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = (char*)"";
    app_config->databases.connections[0].connection_name = (char*)"legacy_conn";
    TEST_ASSERT_EQUAL_STRING("legacy_conn", oidc_get_accounts_database());
}

void test_process_authorization_legacy(void) {
    TEST_ASSERT_NULL(oidc_process_authorization_request(
        "c", "https://x", "token", "openid", "s", "n", "ch", "S256"));
    TEST_ASSERT_NULL(oidc_process_authorization_request(
        "c", "https://x", "code", "openid", "s", "n", "ch", "S256"));
    TEST_ASSERT_NULL(oidc_process_authorization_request(
        "c", "https://x", NULL, "openid", "s", "n", "ch", "S256"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_auth_ro_nulls);
    RUN_TEST(test_get_accounts_database_default);
    RUN_TEST(test_get_accounts_database_fallback_oidc);
    RUN_TEST(test_get_accounts_database_fallback_connection_name);
    RUN_TEST(test_get_accounts_database_fallback_connection_connection_name);
    RUN_TEST(test_process_authorization_legacy);
    return UNITY_END();
}
