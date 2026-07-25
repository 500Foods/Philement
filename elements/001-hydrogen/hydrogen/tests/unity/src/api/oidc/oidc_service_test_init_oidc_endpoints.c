/*
 * Unity Test File: init_oidc_endpoints
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/oidc/oidc_service.h>

void test_init_null_context(void);
void test_init_valid_context(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_init_null_context(void) {
    TEST_ASSERT_FALSE(init_oidc_endpoints(NULL));
}

void test_init_valid_context(void) {
    OIDCContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    TEST_ASSERT_TRUE(init_oidc_endpoints(&ctx));
    cleanup_oidc_endpoints();
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_null_context);
    RUN_TEST(test_init_valid_context);
    return UNITY_END();
}
