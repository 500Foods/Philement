/*
 * Unity Test File: OIDC RP account linker — Phase 21
 *                  (`match_email_then_provision` — the default strategy).
 *
 * Maps to `src/api/auth/oidc_rp/oidc_rp_link_default.c`. Covers
 * `oidc_rp_link_resolve` for
 * `Strategy = "match_email_then_provision"`. The Phase 18 (match_sub_only),
 * Phase 19 (match_email_only), and Phase 20 (provision_only) tests live in
 * sibling files (`oidc_rp_link_sub_test_resolve.c`,
 * `oidc_rp_link_email_test_resolve.c`, and `oidc_rp_link_provision_test_resolve.c`).
 * Phase 21 was split into its own binary to keep each Unity source file
 * under the project-wide 1,000-line cap.
 *
 * The real `execute_auth_query` requires a live database; we avoid that by
 * installing the `oidc_rp_link_test_set_query_fn` test seam in `setUp` and
 * removing it in `tearDown`.
 *
 * Coverage — all sub-paths of `match_email_then_provision`:
 *
 *   Sub fast-path (#080 hit):
 *     - Already-linked user → #080 hit → #084 touch → LINK_OK; neither
 *       #082 nor #083 is called.
 *
 *   Email-link path (#080 miss, #082 hit):
 *     - Sub-miss + verified email + single email match → #080 miss → #082
 *       single hit → #081 link → #084 touch → LINK_OK.
 *     - Sub-miss + unverified email + RequireEmailVerified=false → email-link
 *       path proceeds (verified flag not required).
 *     - Sub-miss + verified email + two email matches → EMAIL_AMBIGUOUS;
 *       provisioning must NOT be attempted even if Enabled=true.
 *     - Sub-miss + verified email + zero email matches + Enabled=false
 *       → NO_ACCOUNT (provisioning disabled; nothing left to try).
 *     - Sub-miss + unverified email + RequireEmailVerified=true
 *       → email-link skipped, provisioning skipped → NO_ACCOUNT.
 *     - Sub-miss + no email in claims → NO_ACCOUNT.
 *     - #082 DB error → DB_ERROR (does not fall through to provisioning).
 *
 *   Provision path (#080 miss, #082 miss or skipped):
 *     - Sub-miss + email-miss + Enabled=true + domain allowed → provision
 *       (#083 → #081 → #084) → LINK_OK with new account_id.
 *     - Sub-miss + email-miss + Enabled=true + domain NOT on allow-list
 *       → PROVISION_DISALLOWED_EMAIL.
 *     - Sub-miss + email-miss + Enabled=true + empty allow-list → any
 *       domain accepted.
 *     - Sub-miss + no email + Enabled=true → NO_ACCOUNT (cannot provision
 *       without email).
 *     - Sub-miss + email-miss + #083 fails → DB_ERROR.
 *     - Sub-miss + email-miss + #083 succeeds but #081 fails → DB_ERROR
 *       (orphan account path).
 *
 * Total: 14 tests.
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
 * Mirrors the identical infrastructure in the Phase 18/19/20 test files.
 * Each Unity binary runs as an independent process so the seam globals
 * do not conflict.
 */

#define SEAM_QUEUE_CAP 8

typedef struct {
    int      query_ref;
    bool     success;
    char    *data_json;
    char    *error_msg;
} SeamEntry;

static SeamEntry g_seam_queue[SEAM_QUEUE_CAP];
static int       g_seam_head     = 0;
static int       g_seam_tail     = 0;
static int       g_last_query_ref = -1;
static int       g_call_count    = 0;

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
    if (next == g_seam_head) return; /* queue full */
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
        return NULL; /* empty → simulates transport failure */
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
static char *make_084_ok(void)   { return strdup("[]"); }

static char *make_082_single(int account_id) {
    char buf[128];
    snprintf(buf, sizeof(buf), "[{\"account_id\":%d}]", account_id);
    return strdup(buf);
}

static char *make_082_ambiguous(int a, int b) {
    char buf[256];
    snprintf(buf, sizeof(buf), "[{\"account_id\":%d},{\"account_id\":%d}]", a, b);
    return strdup(buf);
}

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

/* Build a provider config for match_email_then_provision.
 * domains: NULL-terminated array of allowed domain strings, or NULL for
 * no allow-list. provision_enabled controls ProvisionDefaults.Enabled. */
static OIDCRPProviderConfig make_provider(bool provision_enabled,
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
    p.account_linking.strategy               = OIDC_RP_LINK_MATCH_EMAIL_THEN_PROVISION;
    p.account_linking.require_email_verified = require_email_verified;
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
void test_default_sub_hit_uses_fast_path(void);
void test_default_sub_hit_does_not_call_082_or_083(void);
void test_default_email_link_returns_ok(void);
void test_default_email_link_unverified_allowed_when_not_required(void);
void test_default_email_ambiguous_does_not_provision(void);
void test_default_email_miss_provision_disabled_returns_no_account(void);
void test_default_unverified_email_required_returns_no_account(void);
void test_default_no_email_returns_no_account(void);
void test_default_082_db_error_returns_db_error(void);
void test_default_provision_happy_path(void);
void test_default_provision_domain_not_allowed(void);
void test_default_provision_empty_allowlist_accepts_any(void);
void test_default_provision_083_failure_returns_db_error(void);
void test_default_provision_081_failure_after_083_returns_db_error(void);

/* -------------------------------------------------------------------------
 * Tests
 * -------------------------------------------------------------------------
 */

/* Sub fast-path: #080 hits → LINK_OK, #082 and #083 never called. */
void test_default_sub_hit_uses_fast_path(void) {
    static const char * const domains[] = {"example.com", NULL};
    OIDCRPProviderConfig p = make_provider(true, true, domains);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-linked", "alice", "alice@example.com", true);

    char *hit = make_080_hit(5, 42);
    char *touch = make_084_ok();
    seam_push(80, true, hit,   NULL);
    seam_push(84, true, touch, NULL);
    free(hit);
    free(touch);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_OK, r);
    TEST_ASSERT_NOT_NULL(account);
    TEST_ASSERT_EQUAL_INT(42, account->id);
    free_account_info(account);
}

/* Sub fast-path: verify #082 and #083 are NOT invoked (only #080 + #084). */
void test_default_sub_hit_does_not_call_082_or_083(void) {
    static const char * const domains[] = {"example.com", NULL};
    OIDCRPProviderConfig p = make_provider(true, true, domains);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-linked", NULL, "x@example.com", true);

    char *hit   = make_080_hit(3, 10);
    char *touch = make_084_ok();
    seam_push(80, true, hit,   NULL);
    seam_push(84, true, touch, NULL);
    free(hit);
    free(touch);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_OK, r);
    /* Seam was called exactly twice: #080 + #084. */
    TEST_ASSERT_EQUAL_INT(2, g_call_count);
    /* Last call was #084 (touch), NOT #082 or #083. */
    TEST_ASSERT_EQUAL_INT(84, g_last_query_ref);
    free_account_info(account);
}

/* Email-link: sub-miss + verified email + single match → LINK_OK. */
void test_default_email_link_returns_ok(void) {
    static const char * const domains[] = {"example.com", NULL};
    OIDCRPProviderConfig p = make_provider(false, true, domains);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-new", "alice", "alice@example.com", true);

    char *miss      = make_080_miss();
    char *single    = make_082_single(7);
    char *link      = make_081_ok(20);
    char *touch     = make_084_ok();
    seam_push(80, true, miss,   NULL);
    seam_push(82, true, single, NULL);
    seam_push(81, true, link,   NULL);
    seam_push(84, true, touch,  NULL);
    free(miss);
    free(single);
    free(link);
    free(touch);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_OK, r);
    TEST_ASSERT_NOT_NULL(account);
    TEST_ASSERT_EQUAL_INT(7, account->id);
    free_account_info(account);
}

/* Email-link: unverified email proceeds when RequireEmailVerified=false. */
void test_default_email_link_unverified_allowed_when_not_required(void) {
    static const char * const domains[] = {"example.com", NULL};
    OIDCRPProviderConfig p = make_provider(false, false, domains);
    /* email_verified = false, RequireEmailVerified = false → should link. */
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-new2", "bob", "bob@example.com", false);

    char *miss   = make_080_miss();
    char *single = make_082_single(9);
    char *link   = make_081_ok(21);
    char *touch  = make_084_ok();
    seam_push(80, true, miss,   NULL);
    seam_push(82, true, single, NULL);
    seam_push(81, true, link,   NULL);
    seam_push(84, true, touch,  NULL);
    free(miss);
    free(single);
    free(link);
    free(touch);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_OK, r);
    TEST_ASSERT_NOT_NULL(account);
    free_account_info(account);
}

/* Email-ambiguous: two accounts share the email → EMAIL_AMBIGUOUS.
 * Even though provisioning is Enabled=true, we must NOT fall through. */
void test_default_email_ambiguous_does_not_provision(void) {
    static const char * const domains[] = {"example.com", NULL};
    OIDCRPProviderConfig p = make_provider(true, true, domains);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-ambig", NULL, "both@example.com", true);

    char *miss    = make_080_miss();
    char *ambig   = make_082_ambiguous(1, 2);
    seam_push(80, true, miss,  NULL);
    seam_push(82, true, ambig, NULL);
    free(miss);
    free(ambig);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_EMAIL_AMBIGUOUS, r);
    TEST_ASSERT_NULL(account);
    /* #083 must NOT have been called. */
    TEST_ASSERT_NOT_EQUAL(83, g_last_query_ref);
}

/* Email-miss + Enabled=false → NO_ACCOUNT (no provisioning attempted). */
void test_default_email_miss_provision_disabled_returns_no_account(void) {
    static const char * const domains[] = {"example.com", NULL};
    OIDCRPProviderConfig p = make_provider(false, true, domains);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-nomatch", NULL, "z@example.com", true);

    char *miss      = make_080_miss();
    char *zero_rows = strdup("[]");
    seam_push(80, true, miss,      NULL);
    seam_push(82, true, zero_rows, NULL);
    free(miss);
    free(zero_rows);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_NO_ACCOUNT, r);
    TEST_ASSERT_NULL(account);
}

/* Unverified email + RequireEmailVerified=true → email-link and provision
 * both skipped → NO_ACCOUNT. */
void test_default_unverified_email_required_returns_no_account(void) {
    static const char * const domains[] = {"example.com", NULL};
    OIDCRPProviderConfig p = make_provider(true, true, domains);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-unv", NULL, "unv@example.com", false);

    char *miss = make_080_miss();
    seam_push(80, true, miss, NULL);
    free(miss);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_NO_ACCOUNT, r);
    TEST_ASSERT_NULL(account);
    /* Neither #082 nor #083 should have been called. */
    TEST_ASSERT_NOT_EQUAL(82, g_last_query_ref);
    TEST_ASSERT_NOT_EQUAL(83, g_last_query_ref);
}

/* No email in claims → NO_ACCOUNT (both email-link and provision need it). */
void test_default_no_email_returns_no_account(void) {
    static const char * const domains[] = {"example.com", NULL};
    OIDCRPProviderConfig p = make_provider(true, false, domains);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-noemail", NULL, NULL, false);

    char *miss = make_080_miss();
    seam_push(80, true, miss, NULL);
    free(miss);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_NO_ACCOUNT, r);
    TEST_ASSERT_NULL(account);
}

/* #082 DB error → DB_ERROR; does not fall through to provisioning. */
void test_default_082_db_error_returns_db_error(void) {
    static const char * const domains[] = {"example.com", NULL};
    OIDCRPProviderConfig p = make_provider(true, true, domains);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-dberr", NULL, "err@example.com", true);

    char *miss = make_080_miss();
    seam_push(80, true,  miss,  NULL);
    seam_push(82, false, NULL,  "database connection failed");
    free(miss);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_DB_ERROR, r);
    TEST_ASSERT_NULL(account);
    /* #083 was NOT called. */
    TEST_ASSERT_NOT_EQUAL(83, g_last_query_ref);
}

/* Provision happy path: sub-miss + email-miss + domain allowed → new account. */
void test_default_provision_happy_path(void) {
    static const char * const domains[] = {"example.com", NULL};
    OIDCRPProviderConfig p = make_provider(true, false, domains);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-brand-new", "charlie",
        "charlie@example.com", true);

    char *miss080   = make_080_miss();
    char *miss082   = strdup("[]");
    char *prov083   = make_083_ok(99);
    char *link081   = make_081_ok(55);
    char *touch084  = make_084_ok();
    seam_push(80, true, miss080,  NULL);
    seam_push(82, true, miss082,  NULL);
    seam_push(83, true, prov083,  NULL);
    seam_push(81, true, link081,  NULL);
    seam_push(84, true, touch084, NULL);
    free(miss080);
    free(miss082);
    free(prov083);
    free(link081);
    free(touch084);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_OK, r);
    TEST_ASSERT_NOT_NULL(account);
    TEST_ASSERT_EQUAL_INT(99, account->id);
    free_account_info(account);
}

/* Domain not on allow-list → PROVISION_DISALLOWED_EMAIL. */
void test_default_provision_domain_not_allowed(void) {
    static const char * const domains[] = {"philement.com", NULL};
    OIDCRPProviderConfig p = make_provider(true, false, domains);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-blocked", NULL, "user@example.com", true);

    char *miss080 = make_080_miss();
    char *miss082 = strdup("[]");
    seam_push(80, true, miss080, NULL);
    seam_push(82, true, miss082, NULL);
    free(miss080);
    free(miss082);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_PROVISION_DISALLOWED_EMAIL, r);
    TEST_ASSERT_NULL(account);
}

/* Empty allow-list = no restriction; any domain proceeds to provision. */
void test_default_provision_empty_allowlist_accepts_any(void) {
    OIDCRPProviderConfig p = make_provider(true, false, NULL);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-any", "dave", "dave@random.org", true);

    char *miss080  = make_080_miss();
    char *miss082  = strdup("[]");
    char *prov083  = make_083_ok(77);
    char *link081  = make_081_ok(33);
    char *touch084 = make_084_ok();
    seam_push(80, true, miss080,  NULL);
    seam_push(82, true, miss082,  NULL);
    seam_push(83, true, prov083,  NULL);
    seam_push(81, true, link081,  NULL);
    seam_push(84, true, touch084, NULL);
    free(miss080);
    free(miss082);
    free(prov083);
    free(link081);
    free(touch084);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_OK, r);
    TEST_ASSERT_NOT_NULL(account);
    TEST_ASSERT_EQUAL_INT(77, account->id);
    free_account_info(account);
}

/* #083 fails → DB_ERROR. */
void test_default_provision_083_failure_returns_db_error(void) {
    OIDCRPProviderConfig p = make_provider(true, false, NULL);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-083fail", NULL, "x@any.io", true);

    char *miss080 = make_080_miss();
    char *miss082 = strdup("[]");
    seam_push(80, true,  miss080, NULL);
    seam_push(82, true,  miss082, NULL);
    seam_push(83, false, NULL,    "INSERT failed");
    free(miss080);
    free(miss082);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_DB_ERROR, r);
    TEST_ASSERT_NULL(account);
}

/* #083 succeeds but #081 fails → DB_ERROR (orphan account path).
 * This tests the "half-written rows" warning documented in Phase 20 DoD. */
void test_default_provision_081_failure_after_083_returns_db_error(void) {
    OIDCRPProviderConfig p = make_provider(true, false, NULL);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-orphan", NULL, "orphan@any.io", true);

    char *miss080 = make_080_miss();
    char *miss082 = strdup("[]");
    char *prov083 = make_083_ok(88);
    seam_push(80, true,  miss080, NULL);
    seam_push(82, true,  miss082, NULL);
    seam_push(83, true,  prov083, NULL);
    /* #081: return false (genuine failure, not a UNIQUE race). */
    /* Returning NULL from the seam (empty queue after #083) causes
     * run_query to return NULL, which query_081_link_identity maps to
     * false. So we simply leave the queue empty here. */
    free(miss080);
    free(miss082);
    free(prov083);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_DB_ERROR, r);
    TEST_ASSERT_NULL(account);
}

/* -------------------------------------------------------------------------
 * main
 * -------------------------------------------------------------------------
 */

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_default_sub_hit_uses_fast_path);
    RUN_TEST(test_default_sub_hit_does_not_call_082_or_083);
    RUN_TEST(test_default_email_link_returns_ok);
    RUN_TEST(test_default_email_link_unverified_allowed_when_not_required);
    RUN_TEST(test_default_email_ambiguous_does_not_provision);
    RUN_TEST(test_default_email_miss_provision_disabled_returns_no_account);
    RUN_TEST(test_default_unverified_email_required_returns_no_account);
    RUN_TEST(test_default_no_email_returns_no_account);
    RUN_TEST(test_default_082_db_error_returns_db_error);
    RUN_TEST(test_default_provision_happy_path);
    RUN_TEST(test_default_provision_domain_not_allowed);
    RUN_TEST(test_default_provision_empty_allowlist_accepts_any);
    RUN_TEST(test_default_provision_083_failure_returns_db_error);
    RUN_TEST(test_default_provision_081_failure_after_083_returns_db_error);

    return UNITY_END();
}
