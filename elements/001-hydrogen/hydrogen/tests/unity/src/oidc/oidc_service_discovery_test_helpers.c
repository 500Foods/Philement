/*
 * Unity Test File: oidc_discovery_join_url / endpoint_or_default
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_service.h>
#include <unity/mocks/mock_system.h>

#include <string.h>

void test_join_url(void);
void test_join_url_asprintf_failure(void);
void test_endpoint_or_default(void);

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
}

void test_join_url(void) {
    TEST_ASSERT_NULL(oidc_discovery_join_url("http://x", NULL));

    char *u = oidc_discovery_join_url("http://issuer", "/oauth/token");
    TEST_ASSERT_NOT_NULL(u);
    TEST_ASSERT_EQUAL_STRING("http://issuer/oauth/token", u);
    free(u);

    u = oidc_discovery_join_url(NULL, "/p");
    TEST_ASSERT_NOT_NULL(u);
    TEST_ASSERT_EQUAL_STRING("/p", u);
    free(u);

    u = oidc_discovery_join_url("", "/p");
    TEST_ASSERT_NOT_NULL(u);
    TEST_ASSERT_EQUAL_STRING("/p", u);
    free(u);
}

void test_join_url_asprintf_failure(void) {
    mock_system_set_asprintf_failure(1);
    TEST_ASSERT_NULL(oidc_discovery_join_url("http://x", "/p"));
}

void test_endpoint_or_default(void) {
    TEST_ASSERT_EQUAL_STRING("/oauth/token",
        oidc_discovery_endpoint_or_default(NULL, "/oauth/token"));
    TEST_ASSERT_EQUAL_STRING("/oauth/token",
        oidc_discovery_endpoint_or_default("", "/oauth/token"));
    TEST_ASSERT_EQUAL_STRING("/custom",
        oidc_discovery_endpoint_or_default("/custom", "/oauth/token"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_join_url);
    RUN_TEST(test_join_url_asprintf_failure);
    RUN_TEST(test_endpoint_or_default);
    return UNITY_END();
}
