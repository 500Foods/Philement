/*
 * Unity Test File: oidc_refresh_issue / rotate / reuse family burn
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_refresh_tokens.h>
#include <string.h>

void test_refresh_issue_and_rotate(void);
void test_refresh_reuse_burns_family(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_refresh_issue_and_rotate(void) {
    OIDCRefreshStore *store = oidc_refresh_store_create(3600);
    TEST_ASSERT_NOT_NULL(store);

    char *t1 = oidc_refresh_issue(store, "cli", 10, "openid offline_access", NULL);
    TEST_ASSERT_NOT_NULL(t1);

    OIDCRefreshRecord rec;
    memset(&rec, 0, sizeof(rec));
    char *t2 = NULL;
    TEST_ASSERT_TRUE(oidc_refresh_rotate(store, t1, "cli", &rec, &t2));
    TEST_ASSERT_NOT_NULL(t2);
    TEST_ASSERT_EQUAL_INT(10, rec.account_id);
    TEST_ASSERT_TRUE(strcmp(t1, t2) != 0);

    /* Old token must not rotate again successfully without burning (reuse). */
    char *t3 = NULL;
    TEST_ASSERT_FALSE(oidc_refresh_rotate(store, t1, "cli", NULL, &t3));
    TEST_ASSERT_NULL(t3);

    /* New token still worked only if family not burned — reuse burns family. */
    char *t4 = NULL;
    TEST_ASSERT_FALSE(oidc_refresh_rotate(store, t2, "cli", NULL, &t4));
    TEST_ASSERT_NULL(t4);

    free(t1);
    free(t2);
    oidc_refresh_store_destroy(store);
}

void test_refresh_reuse_burns_family(void) {
    OIDCRefreshStore *store = oidc_refresh_store_create(3600);
    char *t1 = oidc_refresh_issue(store, "c", 1, "openid", NULL);
    OIDCRefreshRecord rec;
    char *t2 = NULL;
    TEST_ASSERT_TRUE(oidc_refresh_rotate(store, t1, "c", &rec, &t2));

    /* Second use of t1 burns family including t2 */
    TEST_ASSERT_FALSE(oidc_refresh_rotate(store, t1, "c", NULL, NULL));

    free(t1);
    free(t2);
    oidc_refresh_store_destroy(store);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_refresh_issue_and_rotate);
    RUN_TEST(test_refresh_reuse_burns_family);
    return UNITY_END();
}
