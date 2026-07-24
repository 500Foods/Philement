/*
 * Unity Test File: oidc_inactive_json / introspect_*_json helpers
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_service.h>
#include <src/oidc/oidc_tokens.h>
#include <src/oidc/oidc_refresh_tokens.h>

#include <string.h>
#include <time.h>

void test_inactive_json(void);
void test_introspect_access_inactive_paths(void);
void test_introspect_access_active(void);
void test_introspect_refresh_paths(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_inactive_json(void) {
    char *j = oidc_inactive_json();
    TEST_ASSERT_NOT_NULL(j);
    TEST_ASSERT_EQUAL_STRING("{\"active\":false}", j);
    free(j);
}

void test_introspect_access_inactive_paths(void) {
    char *j = oidc_introspect_access_json(NULL, "cli");
    TEST_ASSERT_NOT_NULL(j);
    TEST_ASSERT_NOT_NULL(strstr(j, "false"));
    free(j);

    OIDCTokenClaims claims;
    memset(&claims, 0, sizeof(claims));
    j = oidc_introspect_access_json(&claims, "cli");
    TEST_ASSERT_NOT_NULL(j);
    TEST_ASSERT_NOT_NULL(strstr(j, "false"));
    free(j);

    claims.sub = (char*)"42";
    claims.client_id = (char*)"other";
    j = oidc_introspect_access_json(&claims, "cli");
    TEST_ASSERT_NOT_NULL(j);
    TEST_ASSERT_NOT_NULL(strstr(j, "false"));
    free(j);

    claims.client_id = NULL;
    char *aud0 = (char*)"other-aud";
    claims.aud = &aud0;
    claims.aud_count = 1;
    j = oidc_introspect_access_json(&claims, "cli");
    TEST_ASSERT_NOT_NULL(j);
    TEST_ASSERT_NOT_NULL(strstr(j, "false"));
    free(j);
}

void test_introspect_access_active(void) {
    OIDCTokenClaims claims;
    memset(&claims, 0, sizeof(claims));
    claims.sub = (char*)"99";
    claims.client_id = (char*)"cli";
    claims.scope = (char*)"openid email";
    claims.exp = 9999999999L;
    claims.iat = 1000;
    claims.iss = (char*)"http://issuer";

    char *j = oidc_introspect_access_json(&claims, "cli");
    TEST_ASSERT_NOT_NULL(j);
    TEST_ASSERT_NOT_NULL(strstr(j, "\"active\":true"));
    TEST_ASSERT_NOT_NULL(strstr(j, "access_token"));
    TEST_ASSERT_NOT_NULL(strstr(j, "openid email"));
    TEST_ASSERT_NOT_NULL(strstr(j, "http://issuer"));
    free(j);

    /* No client_id on claims → use request client_id */
    claims.client_id = NULL;
    j = oidc_introspect_access_json(&claims, "from-req");
    TEST_ASSERT_NOT_NULL(j);
    TEST_ASSERT_NOT_NULL(strstr(j, "from-req"));
    free(j);
}

void test_introspect_refresh_paths(void) {
    char *j = oidc_introspect_refresh_json(NULL, "cli");
    TEST_ASSERT_NOT_NULL(j);
    free(j);

    OIDCRefreshRecord rec;
    memset(&rec, 0, sizeof(rec));
    strncpy(rec.client_id, "cli", sizeof(rec.client_id) - 1);
    rec.account_id = 7;
    strncpy(rec.scope, "openid", sizeof(rec.scope) - 1);
    rec.expires_at = time(NULL) + 3600;
    rec.revoked_at = 0;

    j = oidc_introspect_refresh_json(&rec, NULL);
    TEST_ASSERT_NOT_NULL(j);
    TEST_ASSERT_NOT_NULL(strstr(j, "false"));
    free(j);

    j = oidc_introspect_refresh_json(&rec, "other");
    TEST_ASSERT_NOT_NULL(j);
    TEST_ASSERT_NOT_NULL(strstr(j, "false"));
    free(j);

    rec.revoked_at = time(NULL);
    j = oidc_introspect_refresh_json(&rec, "cli");
    TEST_ASSERT_NOT_NULL(j);
    TEST_ASSERT_NOT_NULL(strstr(j, "false"));
    free(j);

    rec.revoked_at = 0;
    rec.expires_at = time(NULL) - 10;
    j = oidc_introspect_refresh_json(&rec, "cli");
    TEST_ASSERT_NOT_NULL(j);
    TEST_ASSERT_NOT_NULL(strstr(j, "false"));
    free(j);

    rec.expires_at = time(NULL) + 7200;
    j = oidc_introspect_refresh_json(&rec, "cli");
    TEST_ASSERT_NOT_NULL(j);
    TEST_ASSERT_NOT_NULL(strstr(j, "\"active\":true"));
    TEST_ASSERT_NOT_NULL(strstr(j, "refresh_token"));
    TEST_ASSERT_NOT_NULL(strstr(j, "\"sub\":\"7\""));
    free(j);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_inactive_json);
    RUN_TEST(test_introspect_access_inactive_paths);
    RUN_TEST(test_introspect_access_active);
    RUN_TEST(test_introspect_refresh_paths);
    return UNITY_END();
}
