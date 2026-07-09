/*
 * Unity Test File: auth_roles_from_database
 *
 * Tests the shared role loader promoted from oidc_rp_roles.c. It is now used
 * by both OIDC and password login paths to build the canonical JWT `roles`
 * claim (a comma-separated list of role_id integers).
 *
 * The real `execute_auth_query` requires a live database; we avoid that by
 * installing the `oidc_rp_roles_test_set_query_fn` test seam.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/auth/auth_service.h>
#include <src/api/auth/oidc_rp/oidc_rp_roles.h>

#include <jansson.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations for test functions
void test_auth_roles_null_database(void);
void test_auth_roles_no_rows(void);
void test_auth_roles_single_role(void);
void test_auth_roles_multiple_roles(void);
void test_auth_roles_query_failure(void);
void test_auth_roles_malformed_json(void);
void test_auth_roles_uppercase_column(void);

// Mock query state
static int         g_last_query_ref = -1;
static bool        g_return_null    = false;
static bool        g_return_error   = false;
static const char* g_data_json      = NULL;

static void mock_reset(void) {
    g_last_query_ref = -1;
    g_return_null    = false;
    g_return_error   = false;
    g_data_json      = NULL;
}

static QueryResult *mock_query_fn(int query_ref,
                                   const char *database,
                                   json_t *params) {
    (void)database;
    (void)params;

    g_last_query_ref = query_ref;

    if (g_return_null) {
        return NULL;
    }

    QueryResult *result = calloc(1, sizeof(QueryResult));
    TEST_ASSERT_NOT_NULL(result);

    if (g_return_error) {
        result->success       = false;
        result->error_message = strdup("mock DB error");
        return result;
    }

    result->success   = true;
    result->data_json = g_data_json ? strdup(g_data_json) : NULL;
    return result;
}

void setUp(void) {
    mock_reset();
    oidc_rp_roles_test_set_query_fn(mock_query_fn);
}

void tearDown(void) {
    oidc_rp_roles_test_clear_query_fn();
    mock_reset();
}

void test_auth_roles_null_database(void) {
    g_return_null = true;

    char *roles = auth_roles_from_database(5, NULL);

    TEST_ASSERT_NULL(roles);
    TEST_ASSERT_EQUAL(17, g_last_query_ref);
}

void test_auth_roles_no_rows(void) {
    g_data_json = "[]";

    char *roles = auth_roles_from_database(5, "Acuranzo");

    TEST_ASSERT_NOT_NULL(roles);
    TEST_ASSERT_EQUAL_STRING("", roles);
    TEST_ASSERT_EQUAL(17, g_last_query_ref);
    free(roles);
}

void test_auth_roles_single_role(void) {
    g_data_json = "[{\"role_id\": 1}]";

    char *roles = auth_roles_from_database(5, "Acuranzo");

    TEST_ASSERT_NOT_NULL(roles);
    TEST_ASSERT_EQUAL_STRING("1", roles);
    TEST_ASSERT_EQUAL(17, g_last_query_ref);
    free(roles);
}

void test_auth_roles_multiple_roles(void) {
    g_data_json = "[{\"role_id\": 1}, {\"role_id\": 3}, {\"role_id\": 7}]";

    char *roles = auth_roles_from_database(5, "Acuranzo");

    TEST_ASSERT_NOT_NULL(roles);
    TEST_ASSERT_EQUAL_STRING("1,3,7", roles);
    TEST_ASSERT_EQUAL(17, g_last_query_ref);
    free(roles);
}

void test_auth_roles_query_failure(void) {
    g_return_error = true;

    char *roles = auth_roles_from_database(5, "Acuranzo");

    TEST_ASSERT_NULL(roles);
    TEST_ASSERT_EQUAL(17, g_last_query_ref);
}

void test_auth_roles_malformed_json(void) {
    g_data_json = "not-valid-json";

    char *roles = auth_roles_from_database(5, "Acuranzo");

    TEST_ASSERT_NOT_NULL(roles);
    TEST_ASSERT_EQUAL_STRING("", roles);
    free(roles);
}

void test_auth_roles_uppercase_column(void) {
    g_data_json = "[{\"ROLE_ID\": 42}]";

    char *roles = auth_roles_from_database(5, "Acuranzo");

    TEST_ASSERT_NOT_NULL(roles);
    TEST_ASSERT_EQUAL_STRING("42", roles);
    TEST_ASSERT_EQUAL(17, g_last_query_ref);
    free(roles);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_auth_roles_null_database);
    RUN_TEST(test_auth_roles_no_rows);
    RUN_TEST(test_auth_roles_single_role);
    RUN_TEST(test_auth_roles_multiple_roles);
    RUN_TEST(test_auth_roles_query_failure);
    RUN_TEST(test_auth_roles_malformed_json);
    RUN_TEST(test_auth_roles_uppercase_column);

    return UNITY_END();
}
