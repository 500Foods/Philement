/*
 * Unity Test File: OIDC RP account linker — Phase 20 (provision_only).
 *
 * Maps to `src/api/auth/oidc_rp/oidc_rp_link_provision.c`. Covers the
 * `provision_only` strategy via `oidc_rp_link_resolve` with
 * `Strategy = "provision_only"`. The Phase 18 (match_sub_only) and
 * Phase 19 (match_email_only) tests live in sibling files
 * (`oidc_rp_link_sub_test_resolve.c` and `oidc_rp_link_email_test_resolve.c`);
 * Phase 21's `match_email_then_provision` tests live in
 * `oidc_rp_link_default_test_resolve.c`. Splitting by strategy keeps
 * each Unity source file under the project-wide 1,000-line cap.
 *
 * The real `execute_auth_query` requires a live database; we avoid
 * that by installing the `oidc_rp_link_test_set_query_fn` test seam in
 * `setUp` and removing it in `tearDown`. The seam returns canned JSON
 * rows that simulate QueryRef #080, #081, #083, and #084 responses.
 *
 * Coverage:
 *   provision_only — happy paths:
 *     - Sub already provisioned (#080 hit) → fast path; #083 not called.
 *     - Sub-miss + Enabled=true + email_verified + domain on allow-list
 *       → #083 provisions, #081 links, #084 touches, LINK_OK with new
 *       account_id populated.
 *     - Sub-miss + Enabled=true + empty AllowedEmailDomains list
 *       → any domain accepted (no allow-list = no restriction).
 *     - Domain match is case-insensitive (`Philement.COM` matches
 *       `philement.com`).
 *
 *   provision_only — reject / failure paths:
 *     - Sub-miss + Enabled=false → NO_ACCOUNT (no provisioning attempted).
 *     - Sub-miss + no email in claims → NO_ACCOUNT.
 *     - Sub-miss + email_verified=false + RequireEmailVerified=true
 *       → NO_ACCOUNT.
 *     - Sub-miss + email_verified=false + RequireEmailVerified=false
 *       → provisioning proceeds.
 *     - Sub-miss + email domain NOT on allow-list
 *       → PROVISION_DISALLOWED_EMAIL; #083 must NOT be called.
 *     - Subdomain does NOT match parent domain (no wildcard support).
 *     - #083 returns -1 → DB_ERROR.
 *     - #083 succeeds but #081 fails → DB_ERROR (orphan account is
 *       logged but not rolled back; future sign-in retries the link).
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
 * Test seam infrastructure (FIFO of canned QueryResult responses)
 * -------------------------------------------------------------------------
 */

#define SEAM_QUEUE_CAP 8

typedef struct {
    int      query_ref;
    bool     success;
    char    *data_json;
    char    *error_msg;
} SeamEntry;

static SeamEntry g_seam_queue[SEAM_QUEUE_CAP];
static int       g_seam_head = 0;
static int       g_seam_tail = 0;
static int       g_last_query_ref = -1;
static int       g_call_count = 0;

static void seam_reset(void) {
    for (int i = 0; i < SEAM_QUEUE_CAP; i++) {
        free(g_seam_queue[i].data_json);
        free(g_seam_queue[i].error_msg);
        memset(&g_seam_queue[i], 0, sizeof(g_seam_queue[i]));
    }
    g_seam_head      = 0;
    g_seam_tail      = 0;
    g_last_query_ref = -1;
    g_call_count     = 0;
}

static void seam_push(int query_ref, bool success,
                      const char *data_json, const char *error_msg) {
    int next = (g_seam_tail + 1) % SEAM_QUEUE_CAP;
    if (next == g_seam_head) return;
    g_seam_queue[g_seam_tail].query_ref = query_ref;
    g_seam_queue[g_seam_tail].success   = success;
    g_seam_queue[g_seam_tail].data_json = data_json ? strdup(data_json) : NULL;
    g_seam_queue[g_seam_tail].error_msg = error_msg ? strdup(error_msg) : NULL;
    g_seam_tail = next;
}

static QueryResult *seam_fn(int query_ref,
                             const char *database,
                             json_t *params) {
    (void)database;
    (void)params;

    g_last_query_ref = query_ref;
    g_call_count++;

    if (g_seam_head == g_seam_tail) {
        return NULL;
    }

    SeamEntry entry = g_seam_queue[g_seam_head];
    g_seam_queue[g_seam_head].data_json = NULL;
    g_seam_queue[g_seam_head].error_msg = NULL;
    g_seam_head = (g_seam_head + 1) % SEAM_QUEUE_CAP;

    QueryResult *r = calloc(1, sizeof(QueryResult));
    if (!r) {
        free(entry.data_json);
        free(entry.error_msg);
        return NULL;
    }
    r->success       = entry.success;
    r->data_json     = entry.data_json;
    r->error_message = entry.error_msg;
    return r;
}

/* -------------------------------------------------------------------------
 * Fixture builders
 * -------------------------------------------------------------------------
 */

static char *make_080_hit(int identity_id, int account_id) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "[{\"identity_id\":%d,\"account_id\":%d,"
             "\"issuer\":\"https://example.com\","
             "\"subject\":\"sub-abc\",\"email\":null,"
             "\"email_verified\":0,\"last_seen_at\":\"2026-01-01T00:00:00Z\"}]",
             identity_id, account_id);
    return strdup(buf);
}

static char *make_080_miss(void) { return strdup("[]"); }
static char *make_084_ok(void)  { return strdup("[]"); }

static char *make_081_ok(int identity_id) {
    char buf[128];
    snprintf(buf, sizeof(buf), "[{\"identity_id\":%d}]", identity_id);
    return strdup(buf);
}

static char *make_083_ok(int account_id) {
    char buf[128];
    snprintf(buf, sizeof(buf), "[{\"account_id\":%d}]", account_id);
    return strdup(buf);
}

/* Build a provider config for the provision_only strategy. See
 * docs/H/plans/OIDC-PLAN.md Phase 20 for the field semantics. */
static OIDCRPProviderConfig make_provider_provision(bool provision_enabled,
                                                     bool require_email_verified,
                                                     const char * const *domains) {
    OIDCRPProviderConfig p;
    memset(&p, 0, sizeof(p));
    p.name          = (char *)"test-provider";
    p.issuer        = (char *)"https://example.com";
    p.client_id     = (char *)"lithium-web";
    p.client_secret = (char *)"secret";
    p.redirect_uri  = (char *)"http://localhost/callback";
    p.verify_ssl    = false;
    p.account_linking.strategy                              = OIDC_RP_LINK_PROVISION_ONLY;
    p.account_linking.require_email_verified                = require_email_verified;
    p.account_linking.provision_defaults.enabled            = provision_enabled;
    p.account_linking.provision_defaults.authorized         = true;
    p.account_linking.provision_defaults.default_role_count = 0;

    p.account_linking.allowed_email_domain_count = 0;
    if (domains) {
        for (size_t i = 0;
             i < OIDC_RP_MAX_EMAIL_DOMAINS && domains[i];
             i++) {
            p.account_linking.allowed_email_domains[i] = (char *)domains[i];
            p.account_linking.allowed_email_domain_count++;
        }
    }
    return p;
}

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
    seam_reset();
    oidc_rp_link_test_set_query_fn(seam_fn);
}

void tearDown(void) {
    oidc_rp_link_test_clear_query_fn();
    seam_reset();
}

/* -------------------------------------------------------------------------
 * Forward declarations
 * -------------------------------------------------------------------------
 */
void test_provision_only_sub_hit_uses_fast_path(void);
void test_provision_only_happy_path_provisions_account(void);
void test_provision_only_no_allowlist_accepts_any_domain(void);
void test_provision_only_domain_match_is_case_insensitive(void);
void test_provision_only_disabled_returns_no_account(void);
void test_provision_only_no_email_returns_no_account(void);
void test_provision_only_unverified_email_rejected_when_required(void);
void test_provision_only_unverified_email_allowed_when_not_required(void);
void test_provision_only_disallowed_domain_returns_provision_disallowed_email(void);
void test_provision_only_subdomain_does_not_match_parent(void);
void test_provision_only_083_failure_returns_db_error(void);
void test_provision_only_081_failure_after_083_returns_db_error(void);

/* -------------------------------------------------------------------------
 * Tests
 * -------------------------------------------------------------------------
 */

/* Fast path: already-provisioned identity (#080 hits) — #083 must NOT
 * be called, even though provisioning is enabled. */
void test_provision_only_sub_hit_uses_fast_path(void) {
    static const char *const allow[] = {"example.com", NULL};
    OIDCRPProviderConfig p = make_provider_provision(true, true, allow);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-already", "alice", "alice@example.com", true);

    char *hit_row = make_080_hit(20, 7);
    char *touch   = make_084_ok();
    seam_push(80, true, hit_row, NULL);
    seam_push(84, true, touch,   NULL);
    free(hit_row);
    free(touch);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_OK, r);
    TEST_ASSERT_NOT_NULL(account);
    TEST_ASSERT_EQUAL_INT(7, account->id);
    TEST_ASSERT_EQUAL_INT(2, g_call_count);
    TEST_ASSERT_EQUAL_INT(84, g_last_query_ref);

    free_account_info(account);
}

/* Sub-miss + Enabled=true + email_verified + domain on allow-list:
 * #083 provisions, #081 links, #084 touches. */
void test_provision_only_happy_path_provisions_account(void) {
    static const char *const allow[] = {"example.com", NULL};
    OIDCRPProviderConfig p = make_provider_provision(true, true, allow);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-new", "newbie", "newbie@example.com", true);

    char *miss   = make_080_miss();
    char *prov   = make_083_ok(101);
    char *link_r = make_081_ok(55);
    char *touch  = make_084_ok();
    seam_push(80, true, miss,   NULL);
    seam_push(83, true, prov,   NULL);
    seam_push(81, true, link_r, NULL);
    seam_push(84, true, touch,  NULL);
    free(miss); free(prov); free(link_r); free(touch);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_OK, r);
    TEST_ASSERT_NOT_NULL(account);
    TEST_ASSERT_EQUAL_INT(101, account->id);
    TEST_ASSERT_EQUAL_INT(4, g_call_count);
    TEST_ASSERT_EQUAL_INT(84, g_last_query_ref);

    free_account_info(account);
}

/* No allow-list entries → any domain accepted. */
void test_provision_only_no_allowlist_accepts_any_domain(void) {
    OIDCRPProviderConfig p = make_provider_provision(true, true, NULL);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-anywhere", NULL, "alice@anywhere.test", true);

    char *miss   = make_080_miss();
    char *prov   = make_083_ok(202);
    char *link_r = make_081_ok(99);
    char *touch  = make_084_ok();
    seam_push(80, true, miss,   NULL);
    seam_push(83, true, prov,   NULL);
    seam_push(81, true, link_r, NULL);
    seam_push(84, true, touch,  NULL);
    free(miss); free(prov); free(link_r); free(touch);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_OK, r);
    TEST_ASSERT_NOT_NULL(account);
    TEST_ASSERT_EQUAL_INT(202, account->id);

    free_account_info(account);
}

/* Domain match is case-insensitive. */
void test_provision_only_domain_match_is_case_insensitive(void) {
    static const char *const allow[] = {"philement.com", NULL};
    OIDCRPProviderConfig p = make_provider_provision(true, true, allow);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-mixed", NULL, "alice@PhIlEmEnT.CoM", true);

    char *miss   = make_080_miss();
    char *prov   = make_083_ok(303);
    char *link_r = make_081_ok(45);
    char *touch  = make_084_ok();
    seam_push(80, true, miss,   NULL);
    seam_push(83, true, prov,   NULL);
    seam_push(81, true, link_r, NULL);
    seam_push(84, true, touch,  NULL);
    free(miss); free(prov); free(link_r); free(touch);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_OK, r);
    TEST_ASSERT_NOT_NULL(account);
    TEST_ASSERT_EQUAL_INT(303, account->id);

    free_account_info(account);
}

/* Sub-miss + Enabled=false → NO_ACCOUNT. #083 must NOT be called. */
void test_provision_only_disabled_returns_no_account(void) {
    static const char *const allow[] = {"example.com", NULL};
    OIDCRPProviderConfig p = make_provider_provision(false, true, allow);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-noprov", NULL, "x@example.com", true);

    char *miss = make_080_miss();
    seam_push(80, true, miss, NULL);
    free(miss);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_NO_ACCOUNT, r);
    TEST_ASSERT_NULL(account);
    TEST_ASSERT_EQUAL_INT(1, g_call_count);
    TEST_ASSERT_EQUAL_INT(80, g_last_query_ref);
}

/* Sub-miss + no email → NO_ACCOUNT. */
void test_provision_only_no_email_returns_no_account(void) {
    OIDCRPProviderConfig p = make_provider_provision(true, true, NULL);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-noemail", NULL, NULL, false);

    char *miss = make_080_miss();
    seam_push(80, true, miss, NULL);
    free(miss);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_NO_ACCOUNT, r);
    TEST_ASSERT_NULL(account);
    TEST_ASSERT_EQUAL_INT(1, g_call_count);
}

/* Sub-miss + email_verified=false + RequireEmailVerified=true → NO_ACCOUNT. */
void test_provision_only_unverified_email_rejected_when_required(void) {
    OIDCRPProviderConfig p = make_provider_provision(true, true, NULL);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-unv", NULL, "unv@example.com", false);

    char *miss = make_080_miss();
    seam_push(80, true, miss, NULL);
    free(miss);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_NO_ACCOUNT, r);
    TEST_ASSERT_NULL(account);
    TEST_ASSERT_EQUAL_INT(1, g_call_count);
    TEST_ASSERT_EQUAL_INT(80, g_last_query_ref);
}

/* Sub-miss + email_verified=false + RequireEmailVerified=false → provisioning proceeds. */
void test_provision_only_unverified_email_allowed_when_not_required(void) {
    OIDCRPProviderConfig p = make_provider_provision(true, false, NULL);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-unv-ok", NULL, "ok@example.com", false);

    char *miss   = make_080_miss();
    char *prov   = make_083_ok(404);
    char *link_r = make_081_ok(60);
    char *touch  = make_084_ok();
    seam_push(80, true, miss,   NULL);
    seam_push(83, true, prov,   NULL);
    seam_push(81, true, link_r, NULL);
    seam_push(84, true, touch,  NULL);
    free(miss); free(prov); free(link_r); free(touch);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_OK, r);
    TEST_ASSERT_NOT_NULL(account);
    TEST_ASSERT_EQUAL_INT(404, account->id);

    free_account_info(account);
}

/* Sub-miss + domain not on allow-list → PROVISION_DISALLOWED_EMAIL.
 * #083 must NOT be called. */
void test_provision_only_disallowed_domain_returns_provision_disallowed_email(void) {
    static const char *const allow[] = {"philement.com", "500passwords.com", NULL};
    OIDCRPProviderConfig p = make_provider_provision(true, true, allow);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-evil", NULL, "bob@evil.com", true);

    char *miss = make_080_miss();
    seam_push(80, true, miss, NULL);
    free(miss);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_PROVISION_DISALLOWED_EMAIL, r);
    TEST_ASSERT_NULL(account);
    TEST_ASSERT_EQUAL_INT(1, g_call_count);
    TEST_ASSERT_EQUAL_INT(80, g_last_query_ref);
}

/* Subdomain does NOT match parent (no wildcard support in Phase 20). */
void test_provision_only_subdomain_does_not_match_parent(void) {
    static const char *const allow[] = {"philement.com", NULL};
    OIDCRPProviderConfig p = make_provider_provision(true, true, allow);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-sub", NULL, "alice@dev.philement.com", true);

    char *miss = make_080_miss();
    seam_push(80, true, miss, NULL);
    free(miss);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_PROVISION_DISALLOWED_EMAIL, r);
    TEST_ASSERT_NULL(account);
}

/* #083 returns transport failure → DB_ERROR. */
void test_provision_only_083_failure_returns_db_error(void) {
    OIDCRPProviderConfig p = make_provider_provision(true, true, NULL);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-fail", NULL, "fail@example.com", true);

    char *miss = make_080_miss();
    seam_push(80, true,  miss, NULL);
    seam_push(83, false, NULL, "simulated #083 failure");
    free(miss);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_DB_ERROR, r);
    TEST_ASSERT_NULL(account);
    TEST_ASSERT_EQUAL_INT(2, g_call_count);
    TEST_ASSERT_EQUAL_INT(83, g_last_query_ref);
}

/* #083 succeeds but #081 fails — DB_ERROR (orphan account is logged but
 * not rolled back; future sign-in retries the link). */
void test_provision_only_081_failure_after_083_returns_db_error(void) {
    OIDCRPProviderConfig p = make_provider_provision(true, true, NULL);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-orphan", NULL, "orphan@example.com", true);

    char *miss = make_080_miss();
    char *prov = make_083_ok(505);
    /* Seam queue empty after these two pushes; #081 returns NULL. */
    seam_push(80, true, miss, NULL);
    seam_push(83, true, prov, NULL);
    free(miss); free(prov);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_DB_ERROR, r);
    TEST_ASSERT_NULL(account);
    TEST_ASSERT_EQUAL_INT(81, g_last_query_ref);
}

/* -------------------------------------------------------------------------
 * main
 * -------------------------------------------------------------------------
 */

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_provision_only_sub_hit_uses_fast_path);
    RUN_TEST(test_provision_only_happy_path_provisions_account);
    RUN_TEST(test_provision_only_no_allowlist_accepts_any_domain);
    RUN_TEST(test_provision_only_domain_match_is_case_insensitive);
    RUN_TEST(test_provision_only_disabled_returns_no_account);
    RUN_TEST(test_provision_only_no_email_returns_no_account);
    RUN_TEST(test_provision_only_unverified_email_rejected_when_required);
    RUN_TEST(test_provision_only_unverified_email_allowed_when_not_required);
    RUN_TEST(test_provision_only_disallowed_domain_returns_provision_disallowed_email);
    RUN_TEST(test_provision_only_subdomain_does_not_match_parent);
    RUN_TEST(test_provision_only_083_failure_returns_db_error);
    RUN_TEST(test_provision_only_081_failure_after_083_returns_db_error);

    return UNITY_END();
}
