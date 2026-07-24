/*
 * Unity Test File: oidc_token_error_json, build/mint helpers, refresh flags
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_service.h>
#include <src/oidc/oidc_clients.h>

#include <string.h>

void test_token_error_json_null_error(void);
void test_token_error_json_basic(void);
void test_token_error_json_null_description(void);
void test_client_allows_refresh(void);
void test_should_issue_refresh(void);
void test_build_token_response_with_refresh(void);
void test_build_token_response_without_refresh(void);
void test_build_token_response_null_tokens(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_token_error_json_null_error(void) {
    TEST_ASSERT_NULL(oidc_token_error_json(NULL, "x"));
}

void test_token_error_json_basic(void) {
    char *j = oidc_token_error_json("invalid_grant", "bad code");
    TEST_ASSERT_NOT_NULL(j);
    TEST_ASSERT_NOT_NULL(strstr(j, "invalid_grant"));
    TEST_ASSERT_NOT_NULL(strstr(j, "bad code"));
    free(j);
}

void test_token_error_json_null_description(void) {
    char *j = oidc_token_error_json("server_error", NULL);
    TEST_ASSERT_NOT_NULL(j);
    TEST_ASSERT_NOT_NULL(strstr(j, "server_error"));
    free(j);
}

void test_client_allows_refresh(void) {
    TEST_ASSERT_FALSE(oidc_client_allows_refresh(NULL));

    OIDCClient *c = oidc_client_create(
        "h1", NULL, "T", false, true,
        "[\"https://x/cb\"]",
        "authorization_code", "code");
    TEST_ASSERT_NOT_NULL(c);
    TEST_ASSERT_FALSE(oidc_client_allows_refresh(c));
    oidc_client_free(c);

    c = oidc_client_create(
        "h2", NULL, "T", false, true,
        "[\"https://x/cb\"]",
        "authorization_code refresh_token", "code");
    TEST_ASSERT_NOT_NULL(c);
    TEST_ASSERT_TRUE(oidc_client_allows_refresh(c));
    oidc_client_free(c);
}

void test_should_issue_refresh(void) {
    TEST_ASSERT_FALSE(oidc_should_issue_refresh(NULL, "openid"));

    OIDCClient *c = oidc_client_create(
        "h3", NULL, "T", false, true,
        "[\"https://x/cb\"]",
        "authorization_code", "code");
    TEST_ASSERT_FALSE(oidc_should_issue_refresh(c, "openid offline_access"));
    oidc_client_free(c);

    c = oidc_client_create(
        "h4", NULL, "T", false, true,
        "[\"https://x/cb\"]",
        "authorization_code refresh_token", "code");
    TEST_ASSERT_TRUE(oidc_should_issue_refresh(c, "openid"));
    TEST_ASSERT_TRUE(oidc_should_issue_refresh(c, "openid offline_access"));
    oidc_client_free(c);
}

void test_build_token_response_with_refresh(void) {
    char *j = oidc_build_token_response_json("at", "it", 3600, "openid", "rt");
    TEST_ASSERT_NOT_NULL(j);
    TEST_ASSERT_NOT_NULL(strstr(j, "\"access_token\":\"at\""));
    TEST_ASSERT_NOT_NULL(strstr(j, "\"refresh_token\":\"rt\""));
    TEST_ASSERT_NOT_NULL(strstr(j, "\"expires_in\":3600"));
    free(j);
}

void test_build_token_response_without_refresh(void) {
    char *j = oidc_build_token_response_json("at", "it", 60, NULL, NULL);
    TEST_ASSERT_NOT_NULL(j);
    TEST_ASSERT_NOT_NULL(strstr(j, "\"scope\":\"openid\""));
    TEST_ASSERT_NULL(strstr(j, "refresh_token"));
    free(j);

    j = oidc_build_token_response_json("at", "it", 60, "openid profile", "");
    TEST_ASSERT_NOT_NULL(j);
    TEST_ASSERT_NULL(strstr(j, "refresh_token"));
    free(j);
}

void test_build_token_response_null_tokens(void) {
    TEST_ASSERT_NULL(oidc_build_token_response_json(NULL, "it", 1, "openid", NULL));
    TEST_ASSERT_NULL(oidc_build_token_response_json("at", NULL, 1, "openid", NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_token_error_json_null_error);
    RUN_TEST(test_token_error_json_basic);
    RUN_TEST(test_token_error_json_null_description);
    RUN_TEST(test_client_allows_refresh);
    RUN_TEST(test_should_issue_refresh);
    RUN_TEST(test_build_token_response_with_refresh);
    RUN_TEST(test_build_token_response_without_refresh);
    RUN_TEST(test_build_token_response_null_tokens);
    return UNITY_END();
}
