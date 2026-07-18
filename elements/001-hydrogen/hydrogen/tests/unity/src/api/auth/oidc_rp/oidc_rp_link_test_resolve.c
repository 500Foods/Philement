/*
 * Unity Test File: OIDC RP account linker — coordinator dispatch
 *                  (`oidc_rp_link.c`).
 *
 * Maps to `src/api/auth/oidc_rp/oidc_rp_link.c`. Covers the public
 * entry point `oidc_rp_link_resolve` (argument validation + strategy
 * dispatch) and the error-name table `oidc_rp_link_result_name`.
 *
 * The per-strategy implementations are covered by sibling files:
 *   - oidc_rp_link_sub_test_resolve.c      (match_sub_only)
 *   - oidc_rp_link_email_test_resolve.c    (match_email_only)
 *   - oidc_rp_link_provision_test_resolve.c(provision_only)
 *   - oidc_rp_link_default_test_resolve.c  (match_email_then_provision)
 *
 * The real `execute_auth_query` requires a live database; we avoid
 * that by installing the `oidc_rp_link_test_set_query_fn` test seam in
 * `setUp` and removing it in `tearDown`. The seam returns canned JSON
 * rows that simulate QueryRef #080, #081, #082, and #084 responses.
 *
 * Coverage:
 *   Lifecycle / argument validation:
 *     - error_name returns stable strings for all enum values.
 *     - NULL provider → BAD_INPUT.
 *     - NULL claims → BAD_INPUT.
 *     - NULL database → BAD_INPUT.
 *     - Empty database string → BAD_INPUT.
 *     - NULL out_account → BAD_INPUT.
 *     - Claims with empty iss → BAD_INPUT.
 *     - Claims with empty sub → BAD_INPUT.
 *
 * Total: 8 tests.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/auth/oidc_rp/oidc_rp_link.h>
#include <src/api/auth/oidc_rp/oidc_rp_idtoken.h>
#include <src/api/auth/auth_service.h>
#include <src/config/config_oidc_rp.h>

#include <jansson.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * No seam infrastructure here.
 * -------------------------------------------------------------------------
 * These tests only exercise `oidc_rp_link_resolve`'s argument-validation
 * guard (which returns before any QueryRef is issued) and the error-name
 * table, so no canned-query seam is required.
 */

/* Build a minimal valid provider config (any strategy will do; these
 * tests fail before dispatch reaches a strategy). */
static OIDCRPProviderConfig make_provider(OIDCRPLinkStrategy strategy) {
    OIDCRPProviderConfig p;
    memset(&p, 0, sizeof(p));
    p.name          = (char *)"test-provider";
    p.issuer        = (char *)"https://example.com";
    p.client_id     = (char *)"lithium-web";
    p.client_secret = (char *)"secret";
    p.redirect_uri  = (char *)"http://localhost/callback";
    p.verify_ssl    = false;
    p.account_linking.strategy              = strategy;
    p.account_linking.require_email_verified = true;
    return p;
}

/* Build minimal valid ID-token claims. */
static OidcRpIdTokenClaims make_claims(const char *iss,
                                       const char *sub,
                                       const char *preferred_username,
                                       const char *email,
                                       bool email_verified) {
    OidcRpIdTokenClaims c;
    memset(&c, 0, sizeof(c));
    c.iss                = iss  ? (char *)iss  : NULL;
    c.sub                = sub  ? (char *)sub  : NULL;
    c.preferred_username = preferred_username ? (char *)preferred_username : NULL;
    c.email              = email ? (char *)email : NULL;
    c.email_verified     = email_verified;
    return c;
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * -------------------------------------------------------------------------
 */

void setUp(void) {
    /* No seam required for these argument-validation/error-name tests. */
}

void tearDown(void) {
}

/* -------------------------------------------------------------------------
 * Forward declarations
 * -------------------------------------------------------------------------
 */
void test_error_name_covers_all_values(void);
void test_null_provider_returns_bad_input(void);
void test_null_claims_returns_bad_input(void);
void test_null_database_returns_bad_input(void);
void test_empty_database_returns_bad_input(void);
void test_null_out_account_returns_bad_input(void);
void test_empty_iss_returns_bad_input(void);
void test_empty_sub_returns_bad_input(void);

/* -------------------------------------------------------------------------
 * error_name
 * -------------------------------------------------------------------------
 */

void test_error_name_covers_all_values(void) {
    TEST_ASSERT_EQUAL_STRING("ok",               oidc_rp_link_result_name(OIDC_RP_LINK_OK));
    TEST_ASSERT_EQUAL_STRING("no_account",       oidc_rp_link_result_name(OIDC_RP_LINK_NO_ACCOUNT));
    TEST_ASSERT_EQUAL_STRING("bad_input",        oidc_rp_link_result_name(OIDC_RP_LINK_BAD_INPUT));
    TEST_ASSERT_EQUAL_STRING("db_error",         oidc_rp_link_result_name(OIDC_RP_LINK_DB_ERROR));
    TEST_ASSERT_EQUAL_STRING("disabled",         oidc_rp_link_result_name(OIDC_RP_LINK_DISABLED));
    TEST_ASSERT_EQUAL_STRING("account_disabled", oidc_rp_link_result_name(OIDC_RP_LINK_ACCOUNT_DISABLED));
    TEST_ASSERT_EQUAL_STRING("email_ambiguous",  oidc_rp_link_result_name(OIDC_RP_LINK_EMAIL_AMBIGUOUS));
    TEST_ASSERT_EQUAL_STRING("provision_disallowed_email",
                             oidc_rp_link_result_name(OIDC_RP_LINK_PROVISION_DISALLOWED_EMAIL));
    /* Unknown value. */
    TEST_ASSERT_EQUAL_STRING("unknown",          oidc_rp_link_result_name((OidcRpLinkResult)99));
}

/* -------------------------------------------------------------------------
 * Input validation
 * -------------------------------------------------------------------------
 */

void test_null_provider_returns_bad_input(void) {
    OidcRpIdTokenClaims claims = make_claims("https://example.com", "sub-x", NULL, NULL, false);
    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(NULL, &claims, "Lithium", &account);
    TEST_ASSERT_EQUAL(OIDC_RP_LINK_BAD_INPUT, r);
    TEST_ASSERT_NULL(account);
}

void test_null_claims_returns_bad_input(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_SUB_ONLY);
    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, NULL, "Lithium", &account);
    TEST_ASSERT_EQUAL(OIDC_RP_LINK_BAD_INPUT, r);
    TEST_ASSERT_NULL(account);
}

void test_null_database_returns_bad_input(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_SUB_ONLY);
    OidcRpIdTokenClaims claims = make_claims("https://example.com", "sub-x", NULL, NULL, false);
    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, NULL, &account);
    TEST_ASSERT_EQUAL(OIDC_RP_LINK_BAD_INPUT, r);
    TEST_ASSERT_NULL(account);
}

void test_empty_database_returns_bad_input(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_SUB_ONLY);
    OidcRpIdTokenClaims claims = make_claims("https://example.com", "sub-x", NULL, NULL, false);
    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "", &account);
    TEST_ASSERT_EQUAL(OIDC_RP_LINK_BAD_INPUT, r);
    TEST_ASSERT_NULL(account);
}

void test_null_out_account_returns_bad_input(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_SUB_ONLY);
    OidcRpIdTokenClaims claims = make_claims("https://example.com", "sub-x", NULL, NULL, false);
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", NULL);
    TEST_ASSERT_EQUAL(OIDC_RP_LINK_BAD_INPUT, r);
}

void test_empty_iss_returns_bad_input(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_SUB_ONLY);
    OidcRpIdTokenClaims claims = make_claims("", "sub-x", NULL, NULL, false);
    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);
    TEST_ASSERT_EQUAL(OIDC_RP_LINK_BAD_INPUT, r);
    TEST_ASSERT_NULL(account);
}

void test_empty_sub_returns_bad_input(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_SUB_ONLY);
    OidcRpIdTokenClaims claims = make_claims("https://example.com", "", NULL, NULL, false);
    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);
    TEST_ASSERT_EQUAL(OIDC_RP_LINK_BAD_INPUT, r);
    TEST_ASSERT_NULL(account);
}

/* -------------------------------------------------------------------------
 * main
 * -------------------------------------------------------------------------
 */

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_error_name_covers_all_values);
    RUN_TEST(test_null_provider_returns_bad_input);
    RUN_TEST(test_null_claims_returns_bad_input);
    RUN_TEST(test_null_database_returns_bad_input);
    RUN_TEST(test_empty_database_returns_bad_input);
    RUN_TEST(test_null_out_account_returns_bad_input);
    RUN_TEST(test_empty_iss_returns_bad_input);
    RUN_TEST(test_empty_sub_returns_bad_input);

    return UNITY_END();
}
