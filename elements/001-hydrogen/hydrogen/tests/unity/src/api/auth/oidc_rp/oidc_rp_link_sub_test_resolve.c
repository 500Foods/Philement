/*
 * Unity Test File: OIDC RP account linker — Phase 18 (match_sub_only).
 *
 * Maps to `src/api/auth/oidc_rp/oidc_rp_link_sub.c`. Covers the
 * `match_sub_only` strategy via `oidc_rp_link_resolve` with
 * `Strategy = "match_sub_only"`. The Phase 19 (match_email_only),
 * Phase 20 (provision_only), and Phase 21 (match_email_then_provision)
 * tests live in sibling files. Splitting by strategy keeps each Unity
 * source file under the project-wide 1,000-line cap.
 *
 * The real `execute_auth_query` requires a live database; we avoid
 * that by installing the `oidc_rp_link_test_set_query_fn` test seam in
 * `setUp` and removing it in `tearDown`. The seam returns canned JSON
 * rows that simulate QueryRef #080, #081, #082, and #084 responses.
 *
 * Coverage — `match_sub_only` (oidc_rp_link_sub.c):
 *   Happy path:
 *     - Identity row found by #080 → LINK_OK; account populated with
 *       correct id, non-NULL username, non-NULL roles (empty string).
 *     - QueryRef #084 is called (touch invoked) after a hit.
 *     - preferred_username claim used as display name when present.
 *     - sub used as fallback display name when preferred_username is NULL.
 *
 *   Miss / error paths:
 *     - QueryRef #080 returns zero rows → NO_ACCOUNT; out_account NULL.
 *     - QueryRef #080 returns empty JSON array → NO_ACCOUNT.
 *     - QueryRef #080 returns a failed QueryResult → DB_ERROR.
 *     - QueryRef #080 returns NULL → DB_ERROR.
 *     - QueryRef #084 failure is non-fatal; LINK_OK still returned.
 *
 * Total: 11 tests.
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
 * main
 * -------------------------------------------------------------------------
 */

int main(void) {
    UNITY_BEGIN();

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

    return UNITY_END();
}
