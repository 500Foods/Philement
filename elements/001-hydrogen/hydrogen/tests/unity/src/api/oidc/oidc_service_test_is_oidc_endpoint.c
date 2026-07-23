/*
 * Unity Test File: is_oidc_endpoint / well-known / oauth validators
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/oidc/oidc_service.h>

void test_is_oidc_endpoint_null(void);
void test_is_oidc_endpoint_known_paths(void);
void test_is_oidc_endpoint_unknown(void);
void test_is_oidc_well_known(void);
void test_is_oidc_oauth_request(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_is_oidc_endpoint_null(void) {
    TEST_ASSERT_FALSE(is_oidc_endpoint(NULL));
    TEST_ASSERT_FALSE(is_oidc_well_known_request(NULL));
    TEST_ASSERT_FALSE(is_oidc_oauth_request(NULL));
}

void test_is_oidc_endpoint_known_paths(void) {
    TEST_ASSERT_TRUE(is_oidc_endpoint("/.well-known/openid-configuration"));
    TEST_ASSERT_TRUE(is_oidc_endpoint("/oauth/jwks"));
    TEST_ASSERT_TRUE(is_oidc_endpoint("/oauth/authorize"));
    TEST_ASSERT_TRUE(is_oidc_endpoint("/oauth/token"));
    TEST_ASSERT_TRUE(is_oidc_endpoint("/oauth/userinfo"));
}

void test_is_oidc_endpoint_unknown(void) {
    TEST_ASSERT_FALSE(is_oidc_endpoint("/api/auth/login"));
    TEST_ASSERT_FALSE(is_oidc_endpoint("/api/auth/oidc/start"));
    TEST_ASSERT_FALSE(is_oidc_endpoint("/health"));
}

void test_is_oidc_well_known(void) {
    TEST_ASSERT_TRUE(is_oidc_well_known_request("/.well-known/openid-configuration"));
    TEST_ASSERT_FALSE(is_oidc_well_known_request("/oauth/jwks"));
}

void test_is_oidc_oauth_request(void) {
    TEST_ASSERT_TRUE(is_oidc_oauth_request("/oauth/jwks"));
    TEST_ASSERT_TRUE(is_oidc_oauth_request("/oauth/token"));
    TEST_ASSERT_FALSE(is_oidc_oauth_request("/.well-known/openid-configuration"));
    TEST_ASSERT_FALSE(is_oidc_oauth_request("/api/oauth/token"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_is_oidc_endpoint_null);
    RUN_TEST(test_is_oidc_endpoint_known_paths);
    RUN_TEST(test_is_oidc_endpoint_unknown);
    RUN_TEST(test_is_oidc_well_known);
    RUN_TEST(test_is_oidc_oauth_request);
    return UNITY_END();
}
