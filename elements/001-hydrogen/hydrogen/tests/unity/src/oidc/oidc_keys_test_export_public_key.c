/*
 * Unity Test File: oidc_export_public_key_pem
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_keys.h>
#include <string.h>

void test_export_public_key_pem_null_key(void);
void test_export_public_key_pem_null_key_data(void);
void test_export_public_key_pem_success(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_export_public_key_pem_null_key(void) {
    TEST_ASSERT_NULL(oidc_export_public_key_pem(NULL));
}

void test_export_public_key_pem_null_key_data(void) {
    OIDCKey key = {0};
    TEST_ASSERT_NULL(oidc_export_public_key_pem(&key));
}

void test_export_public_key_pem_success(void) {
    OIDCKeyContext *ctx = init_oidc_key_management(NULL, false, 90);
    TEST_ASSERT_NOT_NULL(ctx);
    OIDCKey *active = oidc_get_active_signing_key(ctx);
    TEST_ASSERT_NOT_NULL(active);

    char *pem = oidc_export_public_key_pem(active);
    TEST_ASSERT_NOT_NULL(pem);
    TEST_ASSERT_NOT_NULL(strstr(pem, "BEGIN PUBLIC KEY"));
    free(pem);
    cleanup_oidc_key_management(ctx);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_export_public_key_pem_null_key);
    RUN_TEST(test_export_public_key_pem_null_key_data);
    RUN_TEST(test_export_public_key_pem_success);
    return UNITY_END();
}
