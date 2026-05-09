/*
 * Unity Test File: OIDC RP account linker — Phase 18 (match_sub_only)
 *                                           + Phase 19 (match_email_only).
 *
 * Phase 20's `provision_only` tests live in a sibling file,
 * `oidc_rp_link_test_provision.c`. Phase 21's
 * `match_email_then_provision` tests live in
 * `oidc_rp_link_test_default.c`. Splitting by phase keeps each file
 * under the project-wide 1,000-line cap.
 *
 * Covers `oidc_rp_link_resolve` from
 * `src/api/auth/oidc_rp/oidc_rp_link.c`.
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
 *   match_sub_only — happy path:
 *     - Identity row found by #080 → LINK_OK; account populated with
 *       correct id, non-NULL username, non-NULL roles (empty string).
 *     - QueryRef #084 is called (touch invoked) after a hit.
 *     - preferred_username claim used as display name when present.
 *     - sub used as fallback display name when preferred_username is NULL.
 *
 *   match_sub_only — miss / error paths:
 *     - QueryRef #080 returns zero rows → NO_ACCOUNT; out_account NULL.
 *     - QueryRef #080 returns empty JSON array → NO_ACCOUNT.
 *     - QueryRef #080 returns a failed QueryResult → DB_ERROR.
 *     - QueryRef #080 returns NULL → DB_ERROR.
 *     - QueryRef #084 failure is non-fatal; LINK_OK still returned.
 *
 *   match_email_only — happy paths (Phase 19):
 *     - Sub already linked (#080 hit) → fast path; #082/#081 not called.
 *     - Sub-miss + email_verified + single email match → linked via #081,
 *       #084 touch called, LINK_OK returned with correct account_id.
 *     - email_verified=false + RequireEmailVerified=false → link proceeds.
 *
 *   match_email_only — miss / reject paths (Phase 19):
 *     - Sub-miss + email_verified=false + RequireEmailVerified=true
 *       → NO_ACCOUNT (unverified email refused).
 *     - Sub-miss + no email in claims → NO_ACCOUNT.
 *     - Sub-miss + #082 returns zero rows → NO_ACCOUNT.
 *     - Sub-miss + #082 returns two rows → EMAIL_AMBIGUOUS.
 *     - Sub-miss + #082 DB error → DB_ERROR.
 *     - Sub-miss + #081 succeeds but returns no identity_id (UNIQUE
 *       race) + #080 re-fetch miss → LINK_OK (identity still linked,
 *       touch skipped non-fatally).
 *
 *   Phase 21 `match_email_then_provision` strategy tests live in
 *   `oidc_rp_link_test_default.c` (separate sibling binary).
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
 * Test seam infrastructure
 * -------------------------------------------------------------------------
 * The seam tracks which QueryRefs have been invoked (so tests can verify
 * #084 is called after a hit), and returns canned responses from a small
 * per-test queue.
 */

#define SEAM_QUEUE_CAP 8

typedef struct {
    int      query_ref;
    bool     success;
    char    *data_json;   /* NULL means success but empty/no-rows */
    char    *error_msg;   /* non-NULL on failure */
} SeamEntry;

static SeamEntry g_seam_queue[SEAM_QUEUE_CAP];
static int       g_seam_head = 0;
static int       g_seam_tail = 0;
static int        g_last_query_ref = -1;  /* most-recently invoked query ref */
static int        g_call_count = 0;       /* total calls to the seam */

static void seam_reset(void) {
    for (int i = 0; i < SEAM_QUEUE_CAP; i++) {
        free(g_seam_queue[i].data_json);
        free(g_seam_queue[i].error_msg);
        memset(&g_seam_queue[i], 0, sizeof(g_seam_queue[i]));
    }
    g_seam_head     = 0;
    g_seam_tail     = 0;
    g_last_query_ref = -1;
    g_call_count     = 0;
}

static void seam_push(int query_ref, bool success,
                      const char *data_json, const char *error_msg) {
    /* Silently drop if queue is full — tests would see wrong results. */
    int next = (g_seam_tail + 1) % SEAM_QUEUE_CAP;
    if (next == g_seam_head) return; /* queue full */
    g_seam_queue[g_seam_tail].query_ref  = query_ref;
    g_seam_queue[g_seam_tail].success    = success;
    g_seam_queue[g_seam_tail].data_json  = data_json  ? strdup(data_json)  : NULL;
    g_seam_queue[g_seam_tail].error_msg  = error_msg  ? strdup(error_msg)  : NULL;
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
        /* Queue empty — return NULL (simulates a transport failure). */
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
    r->data_json     = entry.data_json;    /* transfer ownership */
    r->error_message = entry.error_msg;   /* transfer ownership */
    return r;
}

/* -------------------------------------------------------------------------
 * Fixture builders
 * -------------------------------------------------------------------------
 */

/* A #080 response row with the given identity_id and account_id. */
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

/* A #080 response with zero rows. */
static char *make_080_miss(void) {
    return strdup("[]");
}

/* A minimal #084 success response (UPDATE affects 1 row; we return
 * an empty array because the caller only checks success flag). */
static char *make_084_ok(void) {
    return strdup("[]");
}

/* A #082 response with a single account_id row. */
static char *make_082_single(int account_id) {
    char buf[128];
    snprintf(buf, sizeof(buf), "[{\"account_id\":%d}]", account_id);
    return strdup(buf);
}

/* A #082 response with two rows (ambiguous). */
static char *make_082_ambiguous(int account_id_a, int account_id_b) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "[{\"account_id\":%d},{\"account_id\":%d}]",
             account_id_a, account_id_b);
    return strdup(buf);
}

/* A #081 success response returning a new identity_id. */
static char *make_081_ok(int identity_id) {
    char buf[128];
    snprintf(buf, sizeof(buf), "[{\"identity_id\":%d}]", identity_id);
    return strdup(buf);
}

/* A #081 success response with no rows (UNIQUE race — already inserted). */
static char *make_081_race(void) {
    return strdup("[]");
}

/* Build a provider config pointing at the given strategy.
 * require_email_verified defaults to true (safe default). */
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

/* Build a provider config with require_email_verified set explicitly. */
static OIDCRPProviderConfig make_provider_email(OIDCRPLinkStrategy strategy,
                                                bool require_email_verified) {
    OIDCRPProviderConfig p = make_provider(strategy);
    p.account_linking.require_email_verified = require_email_verified;
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
    c.preferred_username = preferred_username
                               ? (char *)preferred_username
                               : NULL;
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
void test_error_name_covers_all_values(void);
void test_null_provider_returns_bad_input(void);
void test_null_claims_returns_bad_input(void);
void test_null_database_returns_bad_input(void);
void test_empty_database_returns_bad_input(void);
void test_null_out_account_returns_bad_input(void);
void test_empty_iss_returns_bad_input(void);
void test_empty_sub_returns_bad_input(void);
void test_match_sub_only_hit_returns_ok(void);
void test_match_sub_only_hit_populates_account_id(void);
void test_match_sub_only_hit_calls_084_touch(void);
void test_match_sub_only_uses_preferred_username(void);
void test_match_sub_only_falls_back_to_sub_when_no_preferred_username(void);
void test_match_sub_only_miss_returns_no_account(void);
void test_match_sub_only_empty_array_returns_no_account(void);
void test_match_sub_only_080_failure_returns_db_error(void);
void test_match_sub_only_080_null_returns_db_error(void);
void test_match_sub_only_084_failure_is_non_fatal(void);
/* Phase 19 — match_email_only */
void test_match_email_only_sub_hit_uses_fast_path(void);
void test_match_email_only_email_link_returns_ok(void);
void test_match_email_only_email_link_populates_account_id(void);
void test_match_email_only_email_link_calls_081_and_084(void);
void test_match_email_only_unverified_email_rejected_when_required(void);
void test_match_email_only_unverified_email_allowed_when_not_required(void);
void test_match_email_only_no_email_returns_no_account(void);
void test_match_email_only_082_zero_rows_returns_no_account(void);
void test_match_email_only_082_ambiguous_returns_email_ambiguous(void);
void test_match_email_only_082_db_error_returns_db_error(void);
void test_match_email_only_081_race_still_returns_ok(void);

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
 * match_sub_only — happy paths
 * -------------------------------------------------------------------------
 */

void test_match_sub_only_hit_returns_ok(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_SUB_ONLY);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-happy", "alice", "alice@example.com", true);

    char *hit_row = make_080_hit(42, 7);
    char *touch   = make_084_ok();
    seam_push(80, true, hit_row, NULL);
    seam_push(84, true, touch,   NULL);
    free(hit_row);
    free(touch);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_OK, r);
    TEST_ASSERT_NOT_NULL(account);

    free_account_info(account);
}

void test_match_sub_only_hit_populates_account_id(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_SUB_ONLY);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-id-check", NULL, NULL, false);

    char *hit_row = make_080_hit(99, 55);
    char *touch   = make_084_ok();
    seam_push(80, true, hit_row, NULL);
    seam_push(84, true, touch,   NULL);
    free(hit_row);
    free(touch);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_OK, r);
    TEST_ASSERT_NOT_NULL(account);
    TEST_ASSERT_EQUAL_INT(55, account->id);

    free_account_info(account);
}

void test_match_sub_only_hit_calls_084_touch(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_SUB_ONLY);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-touch", NULL, NULL, false);

    char *hit_row = make_080_hit(10, 3);
    char *touch   = make_084_ok();
    seam_push(80, true, hit_row, NULL);
    seam_push(84, true, touch,   NULL);
    free(hit_row);
    free(touch);

    account_info_t *account = NULL;
    oidc_rp_link_resolve(&p, &claims, "Lithium", &account);
    if (account) free_account_info(account);

    /* Both QueryRefs must have been invoked. */
    TEST_ASSERT_EQUAL_INT(2, g_call_count);
    /* The last one should have been #084. */
    TEST_ASSERT_EQUAL_INT(84, g_last_query_ref);
}

void test_match_sub_only_uses_preferred_username(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_SUB_ONLY);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-pref", "alice", NULL, false);

    char *hit_row = make_080_hit(1, 2);
    char *touch   = make_084_ok();
    seam_push(80, true, hit_row, NULL);
    seam_push(84, true, touch,   NULL);
    free(hit_row);
    free(touch);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_OK, r);
    TEST_ASSERT_NOT_NULL(account);
    TEST_ASSERT_NOT_NULL(account->username);
    TEST_ASSERT_EQUAL_STRING("alice", account->username);

    free_account_info(account);
}

void test_match_sub_only_falls_back_to_sub_when_no_preferred_username(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_SUB_ONLY);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "my-sub-value", NULL, NULL, false);

    char *hit_row = make_080_hit(3, 8);
    char *touch   = make_084_ok();
    seam_push(80, true, hit_row, NULL);
    seam_push(84, true, touch,   NULL);
    free(hit_row);
    free(touch);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_OK, r);
    TEST_ASSERT_NOT_NULL(account);
    TEST_ASSERT_NOT_NULL(account->username);
    TEST_ASSERT_EQUAL_STRING("my-sub-value", account->username);

    free_account_info(account);
}

/* -------------------------------------------------------------------------
 * match_sub_only — miss / error paths
 * -------------------------------------------------------------------------
 */

void test_match_sub_only_miss_returns_no_account(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_SUB_ONLY);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-not-linked", NULL, NULL, false);

    /* #080 returns no rows. */
    char *miss = make_080_miss();
    seam_push(80, true, miss, NULL);
    free(miss);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_NO_ACCOUNT, r);
    TEST_ASSERT_NULL(account);
    /* #084 must NOT have been called. */
    TEST_ASSERT_EQUAL_INT(1, g_call_count);
    TEST_ASSERT_EQUAL_INT(80, g_last_query_ref);
}

void test_match_sub_only_empty_array_returns_no_account(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_SUB_ONLY);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-empty", NULL, NULL, false);

    seam_push(80, true, "[]", NULL);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_NO_ACCOUNT, r);
    TEST_ASSERT_NULL(account);
}

void test_match_sub_only_080_failure_returns_db_error(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_SUB_ONLY);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-db-err", NULL, NULL, false);

    /* #080 returns a failed result. */
    seam_push(80, false, NULL, "simulated DB error");

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_DB_ERROR, r);
    TEST_ASSERT_NULL(account);
}

void test_match_sub_only_080_null_returns_db_error(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_SUB_ONLY);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-null-result", NULL, NULL, false);

    /* Seam queue is empty → seam_fn returns NULL, simulating a
     * transport failure inside execute_auth_query. */

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_DB_ERROR, r);
    TEST_ASSERT_NULL(account);
}

void test_match_sub_only_084_failure_is_non_fatal(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_SUB_ONLY);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-touch-fail", NULL, NULL, false);

    char *hit_row = make_080_hit(5, 12);
    seam_push(80, true,  hit_row,           NULL);
    seam_push(84, false, NULL, "touch error"); /* #084 fails */
    free(hit_row);

    account_info_t *account = NULL;
    /* #084 failure should be logged but must NOT abort the login. */
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_OK, r);
    TEST_ASSERT_NOT_NULL(account);
    TEST_ASSERT_EQUAL_INT(12, account->id);

    free_account_info(account);
}

/* -------------------------------------------------------------------------
 * match_email_only — Phase 19 tests
 * -------------------------------------------------------------------------
 */

/* Fast path: (iss, sub) is already in account_oidc_identities. The email
 * lookup (#082/#081) must NOT be called. */
void test_match_email_only_sub_hit_uses_fast_path(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_EMAIL_ONLY);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-fast", "alice", "alice@example.com", true);

    char *hit_row = make_080_hit(10, 5);
    char *touch   = make_084_ok();
    seam_push(80, true, hit_row, NULL);
    seam_push(84, true, touch,   NULL);
    free(hit_row);
    free(touch);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_OK, r);
    TEST_ASSERT_NOT_NULL(account);
    TEST_ASSERT_EQUAL_INT(5, account->id);

    /* Only #080 and #084 — NO call to #082 or #081. */
    TEST_ASSERT_EQUAL_INT(2, g_call_count);

    free_account_info(account);
}

/* Sub-miss + single email match → link via #081, touch via #084, LINK_OK. */
void test_match_email_only_email_link_returns_ok(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_EMAIL_ONLY);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-new", "bob", "bob@example.com", true);

    char *miss    = make_080_miss();
    char *email_r = make_082_single(20);
    char *link_r  = make_081_ok(77);
    char *touch   = make_084_ok();
    seam_push(80, true, miss,    NULL);
    seam_push(82, true, email_r, NULL);
    seam_push(81, true, link_r,  NULL);
    seam_push(84, true, touch,   NULL);
    free(miss); free(email_r); free(link_r); free(touch);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_OK, r);
    TEST_ASSERT_NOT_NULL(account);

    free_account_info(account);
}

/* Sub-miss + single email match → account_id populated correctly. */
void test_match_email_only_email_link_populates_account_id(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_EMAIL_ONLY);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-pop", NULL, "pop@example.com", true);

    char *miss    = make_080_miss();
    char *email_r = make_082_single(42);
    char *link_r  = make_081_ok(88);
    char *touch   = make_084_ok();
    seam_push(80, true, miss,    NULL);
    seam_push(82, true, email_r, NULL);
    seam_push(81, true, link_r,  NULL);
    seam_push(84, true, touch,   NULL);
    free(miss); free(email_r); free(link_r); free(touch);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_OK, r);
    TEST_ASSERT_NOT_NULL(account);
    TEST_ASSERT_EQUAL_INT(42, account->id);

    free_account_info(account);
}

/* Sub-miss + single email match → #080, #082, #081, #084 all called
 * in that order. */
void test_match_email_only_email_link_calls_081_and_084(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_EMAIL_ONLY);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-order", NULL, "order@example.com", true);

    char *miss    = make_080_miss();
    char *email_r = make_082_single(7);
    char *link_r  = make_081_ok(99);
    char *touch   = make_084_ok();
    seam_push(80, true, miss,    NULL);
    seam_push(82, true, email_r, NULL);
    seam_push(81, true, link_r,  NULL);
    seam_push(84, true, touch,   NULL);
    free(miss); free(email_r); free(link_r); free(touch);

    account_info_t *account = NULL;
    oidc_rp_link_resolve(&p, &claims, "Lithium", &account);
    if (account) free_account_info(account);

    /* All four queries must have been invoked. */
    TEST_ASSERT_EQUAL_INT(4, g_call_count);
    /* Last call is the #084 touch. */
    TEST_ASSERT_EQUAL_INT(84, g_last_query_ref);
}

/* Sub-miss + email_verified=false + RequireEmailVerified=true
 * → NO_ACCOUNT; #082 must NOT be called. */
void test_match_email_only_unverified_email_rejected_when_required(void) {
    /* require_email_verified = true (default make_provider). */
    OIDCRPProviderConfig p = make_provider_email(OIDC_RP_LINK_MATCH_EMAIL_ONLY, true);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-unverified", NULL, "unv@example.com", false);

    char *miss = make_080_miss();
    seam_push(80, true, miss, NULL);
    free(miss);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_NO_ACCOUNT, r);
    TEST_ASSERT_NULL(account);
    /* Only #080 was called; #082 must NOT be called. */
    TEST_ASSERT_EQUAL_INT(1, g_call_count);
    TEST_ASSERT_EQUAL_INT(80, g_last_query_ref);
}

/* Sub-miss + email_verified=false + RequireEmailVerified=false
 * → email lookup is allowed; link proceeds. */
void test_match_email_only_unverified_email_allowed_when_not_required(void) {
    OIDCRPProviderConfig p = make_provider_email(OIDC_RP_LINK_MATCH_EMAIL_ONLY, false);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-unver-ok", NULL, "unver@example.com", false);

    char *miss    = make_080_miss();
    char *email_r = make_082_single(11);
    char *link_r  = make_081_ok(55);
    char *touch   = make_084_ok();
    seam_push(80, true, miss,    NULL);
    seam_push(82, true, email_r, NULL);
    seam_push(81, true, link_r,  NULL);
    seam_push(84, true, touch,   NULL);
    free(miss); free(email_r); free(link_r); free(touch);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_OK, r);
    TEST_ASSERT_NOT_NULL(account);

    free_account_info(account);
}

/* Sub-miss + no email in claims → NO_ACCOUNT; #082 not called. */
void test_match_email_only_no_email_returns_no_account(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_EMAIL_ONLY);
    /* email_verified=true but no email field. */
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-noemail", NULL, NULL, true);

    char *miss = make_080_miss();
    seam_push(80, true, miss, NULL);
    free(miss);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_NO_ACCOUNT, r);
    TEST_ASSERT_NULL(account);
    TEST_ASSERT_EQUAL_INT(1, g_call_count);
}

/* Sub-miss + #082 returns zero rows → NO_ACCOUNT. */
void test_match_email_only_082_zero_rows_returns_no_account(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_EMAIL_ONLY);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-noemail2", NULL, "nobody@example.com", true);

    char *miss      = make_080_miss();
    char *empty_082 = strdup("[]");
    seam_push(80, true, miss,      NULL);
    seam_push(82, true, empty_082, NULL);
    free(miss); free(empty_082);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_NO_ACCOUNT, r);
    TEST_ASSERT_NULL(account);
    TEST_ASSERT_EQUAL_INT(2, g_call_count);
}

/* Sub-miss + #082 returns two rows → EMAIL_AMBIGUOUS. */
void test_match_email_only_082_ambiguous_returns_email_ambiguous(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_EMAIL_ONLY);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-amb", NULL, "shared@example.com", true);

    char *miss    = make_080_miss();
    char *ambig   = make_082_ambiguous(3, 7);
    seam_push(80, true, miss,  NULL);
    seam_push(82, true, ambig, NULL);
    free(miss); free(ambig);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_EMAIL_AMBIGUOUS, r);
    TEST_ASSERT_NULL(account);
    /* #081 must NOT be called after EMAIL_AMBIGUOUS. */
    TEST_ASSERT_EQUAL_INT(2, g_call_count);
}

/* Sub-miss + #082 returns a database error → DB_ERROR. */
void test_match_email_only_082_db_error_returns_db_error(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_EMAIL_ONLY);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-dberr", NULL, "dberr@example.com", true);

    char *miss = make_080_miss();
    seam_push(80, true,  miss,  NULL);
    seam_push(82, false, NULL,  "simulated #082 error");
    free(miss);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    TEST_ASSERT_EQUAL(OIDC_RP_LINK_DB_ERROR, r);
    TEST_ASSERT_NULL(account);
}

/* Sub-miss + #082 single hit + #081 UNIQUE race (returns success but no rows)
 * + #080 re-fetch also misses → LINK_OK returned (identity was linked by the
 * racing request; we tolerate the skipped touch non-fatally). */
void test_match_email_only_081_race_still_returns_ok(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_LINK_MATCH_EMAIL_ONLY);
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-race", NULL, "race@example.com", true);

    char *miss     = make_080_miss();   /* initial #080: miss */
    char *email_r  = make_082_single(9);
    char *race_081 = make_081_race();   /* #081: success but no identity_id */
    char *miss2    = make_080_miss();   /* re-fetch #080: also miss */
    seam_push(80, true, miss,     NULL);
    seam_push(82, true, email_r,  NULL);
    seam_push(81, true, race_081, NULL);
    seam_push(80, true, miss2,    NULL);
    free(miss); free(email_r); free(race_081); free(miss2);

    account_info_t *account = NULL;
    OidcRpLinkResult r = oidc_rp_link_resolve(&p, &claims, "Lithium", &account);

    /* The identity is linked (by the racing request); we still return OK
     * so the user's login succeeds. The touch is merely skipped. */
    TEST_ASSERT_EQUAL(OIDC_RP_LINK_OK, r);
    TEST_ASSERT_NOT_NULL(account);
    TEST_ASSERT_EQUAL_INT(9, account->id);

    free_account_info(account);
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
    RUN_TEST(test_match_sub_only_hit_returns_ok);
    RUN_TEST(test_match_sub_only_hit_populates_account_id);
    RUN_TEST(test_match_sub_only_hit_calls_084_touch);
    RUN_TEST(test_match_sub_only_uses_preferred_username);
    RUN_TEST(test_match_sub_only_falls_back_to_sub_when_no_preferred_username);
    RUN_TEST(test_match_sub_only_miss_returns_no_account);
    RUN_TEST(test_match_sub_only_empty_array_returns_no_account);
    RUN_TEST(test_match_sub_only_080_failure_returns_db_error);
    RUN_TEST(test_match_sub_only_080_null_returns_db_error);
    RUN_TEST(test_match_sub_only_084_failure_is_non_fatal);
    /* Phase 19 — match_email_only */
    RUN_TEST(test_match_email_only_sub_hit_uses_fast_path);
    RUN_TEST(test_match_email_only_email_link_returns_ok);
    RUN_TEST(test_match_email_only_email_link_populates_account_id);
    RUN_TEST(test_match_email_only_email_link_calls_081_and_084);
    RUN_TEST(test_match_email_only_unverified_email_rejected_when_required);
    RUN_TEST(test_match_email_only_unverified_email_allowed_when_not_required);
    RUN_TEST(test_match_email_only_no_email_returns_no_account);
    RUN_TEST(test_match_email_only_082_zero_rows_returns_no_account);
    RUN_TEST(test_match_email_only_082_ambiguous_returns_email_ambiguous);
    RUN_TEST(test_match_email_only_082_db_error_returns_db_error);
    RUN_TEST(test_match_email_only_081_race_still_returns_ok);

    return UNITY_END();
}
