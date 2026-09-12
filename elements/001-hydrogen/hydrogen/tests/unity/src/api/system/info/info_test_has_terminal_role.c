/*
 * Unity Test File: system_info_has_terminal_role coverage
 *
 * Tests the role-checking logic in info.c that gates whether the
 * "terminal" object is included in the /api/system/info JSON response.
 *
 * The function system_info_has_terminal_role() delegates to
 * mailrelay_api_has_role_id() with the terminal role token "32".
 * These tests verify the boundary behavior: NULL claims, empty roles,
 * correct role match, non-terminal roles, and substring safety.
 */

#define USE_MOCK_INFO
#define UNITY_TEST_MODE

#include <src/hydrogen.h>
#include <unity.h>

#include <jansson.h>

#include <src/api/auth/auth_service.h>
#include <src/api/system/info/info.h>

void test_has_terminal_role_null_claims(void);
void test_has_terminal_role_null_roles_claim(void);
void test_has_terminal_role_empty_roles(void);
void test_has_terminal_role_only_terminal(void);
void test_has_terminal_role_terminal_in_comma_list(void);
void test_has_terminal_role_non_terminal_role(void);
void test_has_terminal_role_admin_is_not_terminal(void);
void test_has_terminal_role_substring_not_match(void);
void test_has_terminal_role_whitespace_padding(void);

void setUp(void) {
    /* No global state to initialize for this pure function test */
}

void tearDown(void) {
    /* No global state to clean up */
}

void test_has_terminal_role_null_claims(void) {
    bool result = system_info_has_terminal_role(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_has_terminal_role_null_roles_claim(void) {
    jwt_claims_t claims = {0};
    /* roles is NULL (default for zeroed struct) */
    TEST_ASSERT_NULL(claims.roles);
    bool result = system_info_has_terminal_role(&claims);
    TEST_ASSERT_FALSE(result);
}

void test_has_terminal_role_empty_roles(void) {
    jwt_claims_t claims = {0};
    char roles[] = "";
    claims.roles = roles;
    bool result = system_info_has_terminal_role(&claims);
    TEST_ASSERT_FALSE(result);
}

void test_has_terminal_role_only_terminal(void) {
    jwt_claims_t claims = {0};
    char roles[] = "32";
    claims.roles = roles;
    bool result = system_info_has_terminal_role(&claims);
    TEST_ASSERT_TRUE(result);
}

void test_has_terminal_role_terminal_in_comma_list(void) {
    jwt_claims_t claims = {0};
    char roles[] = "1,32,5";
    claims.roles = roles;
    bool result = system_info_has_terminal_role(&claims);
    TEST_ASSERT_TRUE(result);
}

void test_has_terminal_role_non_terminal_role(void) {
    jwt_claims_t claims = {0};
    char roles[] = "1,5,10";
    claims.roles = roles;
    bool result = system_info_has_terminal_role(&claims);
    TEST_ASSERT_FALSE(result);
}

void test_has_terminal_role_admin_is_not_terminal(void) {
    jwt_claims_t claims = {0};
    char roles[] = "1";  /* Admin role is role_id 1, not 32 */
    claims.roles = roles;
    bool result = system_info_has_terminal_role(&claims);
    TEST_ASSERT_FALSE(result);
}

void test_has_terminal_role_substring_not_match(void) {
    jwt_claims_t claims = {0};
    char roles[] = "132";  /* Contains "32" as substring but should NOT match */
    claims.roles = roles;
    bool result = system_info_has_terminal_role(&claims);
    TEST_ASSERT_FALSE(result);
}

void test_has_terminal_role_whitespace_padding(void) {
    jwt_claims_t claims = {0};
    char roles[] = " 32 ";  /* Spaces around the token should still match */
    claims.roles = roles;
    bool result = system_info_has_terminal_role(&claims);
    TEST_ASSERT_TRUE(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_has_terminal_role_null_claims);
    RUN_TEST(test_has_terminal_role_null_roles_claim);
    RUN_TEST(test_has_terminal_role_empty_roles);
    RUN_TEST(test_has_terminal_role_only_terminal);
    RUN_TEST(test_has_terminal_role_terminal_in_comma_list);
    RUN_TEST(test_has_terminal_role_non_terminal_role);
    RUN_TEST(test_has_terminal_role_admin_is_not_terminal);
    RUN_TEST(test_has_terminal_role_substring_not_match);
    RUN_TEST(test_has_terminal_role_whitespace_padding);

    return UNITY_END();
}
