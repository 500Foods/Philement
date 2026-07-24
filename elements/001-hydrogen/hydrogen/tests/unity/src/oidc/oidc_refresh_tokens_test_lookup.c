/*
 * Unity Test File: oidc_refresh_lookup / revoke
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_refresh_tokens.h>

#include <string.h>

void test_lookup_nulls(void);
void test_lookup_and_revoke(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_lookup_nulls(void) {
    OIDCRefreshStore *store = oidc_refresh_store_create(60);
    OIDCRefreshRecord rec;
    TEST_ASSERT_FALSE(oidc_refresh_lookup(NULL, "t", &rec));
    TEST_ASSERT_FALSE(oidc_refresh_lookup(store, NULL, &rec));
    TEST_ASSERT_FALSE(oidc_refresh_lookup(store, "t", NULL));
    oidc_refresh_store_destroy(store);
}

void test_lookup_and_revoke(void) {
    OIDCRefreshStore *store = oidc_refresh_store_create(3600);
    TEST_ASSERT_NOT_NULL(store);

    char *plain = oidc_refresh_issue(store, "cli", 7, "openid", NULL);
    TEST_ASSERT_NOT_NULL(plain);

    OIDCRefreshRecord rec;
    memset(&rec, 0, sizeof(rec));
    TEST_ASSERT_TRUE(oidc_refresh_lookup(store, plain, &rec));
    TEST_ASSERT_EQUAL_INT(7, rec.account_id);
    TEST_ASSERT_EQUAL_STRING("cli", rec.client_id);
    TEST_ASSERT_EQUAL_INT(0, (int)rec.revoked_at);

    TEST_ASSERT_TRUE(oidc_refresh_revoke(store, plain));
    TEST_ASSERT_TRUE(oidc_refresh_lookup(store, plain, &rec));
    TEST_ASSERT_TRUE(rec.revoked_at != 0);

    TEST_ASSERT_FALSE(oidc_refresh_lookup(store, "nope", &rec));

    free(plain);
    oidc_refresh_store_destroy(store);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_lookup_nulls);
    RUN_TEST(test_lookup_and_revoke);
    return UNITY_END();
}
