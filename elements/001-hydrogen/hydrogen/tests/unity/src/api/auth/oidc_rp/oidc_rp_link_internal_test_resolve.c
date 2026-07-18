/*
 * Unity Test File: OIDC RP account linker — shared internal helpers.
 *
 * Maps to `src/api/auth/oidc_rp/oidc_rp_link_internal.c`. Covers the
 * shared infrastructure used by every per-strategy linker module:
 *   - oidc_rp_link_run_query          (seam routing)
 *   - oidc_rp_link_safe_iss / _safe_sub (log-safe truncation)
 *   - oidc_rp_link_build_account_info (account_info_t builder)
 *   - oidc_rp_link_query_080_lookup   (identity lookup)
 *   - oidc_rp_link_query_081_link_identity (identity link)
 *   - oidc_rp_link_query_082_lookup_by_email (account-by-email)
 *   - oidc_rp_link_query_083_provision (provision accounts row)
 *   - oidc_rp_link_query_084_touch     (best-effort touch)
 *
 * The real `execute_auth_query` requires a live database; we avoid
 * that by installing the `oidc_rp_link_test_set_query_fn` test seam in
 * `setUp` and removing it in `tearDown`. The seam returns canned JSON
 * rows that simulate QueryRef #080–#084 responses.
 *
 * Total: 27 tests.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/auth/oidc_rp/oidc_rp_link.h>
#include <src/api/auth/oidc_rp/oidc_rp_link_internal.h>
#include <src/api/auth/oidc_rp/oidc_rp_idtoken.h>
#include <src/api/auth/auth_service.h>
#include <src/config/config_oidc_rp.h>

#include <jansson.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Test seam infrastructure
 * -------------------------------------------------------------------------
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
static int       g_last_query_ref = -1;  /* most-recently invoked query ref */
static int       g_call_count = 0;       /* total calls to the seam */

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
    r->data_json     = entry.data_json;    /* transfer ownership */
    r->error_message = entry.error_msg;   /* transfer ownership */
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
/* run_query */
void test_run_query_routes_through_seam(void);
void test_run_query_null_seam_returns_null(void);
/* safe_iss */
void test_safe_iss_null_writes_null_marker(void);
void test_safe_iss_truncates_long_value(void);
void test_safe_iss_copies_short_value(void);
/* safe_sub */
void test_safe_sub_null_writes_null_marker(void);
void test_safe_sub_truncates_long_value(void);
void test_safe_sub_copies_short_value(void);
/* build_account_info */
void test_build_account_info_prefers_preferred_username(void);
void test_build_account_info_falls_back_to_sub(void);
void test_build_account_info_copies_email_when_present(void);
void test_build_account_info_no_email_yields_null_email(void);
void test_build_account_info_roles_empty_and_enabled(void);
void test_build_account_info_negative_id_accepted(void);
/* query_080 */
void test_query_080_hit_populates_ids(void);
void test_query_080_miss_leaves_ids_negative(void);
void test_query_080_null_result_sets_db_error(void);
void test_query_080_failed_result_sets_db_error(void);
/* query_081 */
void test_query_081_success_returns_identity_id(void);
void test_query_081_null_result_returns_false(void);
void test_query_081_failed_result_returns_true_retry(void);
/* query_082 */
void test_query_082_single_returns_one_and_account_id(void);
void test_query_082_zero_returns_zero(void);
void test_query_082_ambiguous_returns_two(void);
void test_query_082_db_error_returns_neg_one(void);
/* query_083 */
void test_query_083_success_returns_account_id(void);
void test_query_083_null_result_returns_neg_one(void);
void test_query_083_failed_result_returns_neg_one(void);
/* query_084 */
void test_query_084_success_is_non_fatal_ok(void);
void test_query_084_null_result_is_non_fatal(void);

/* -------------------------------------------------------------------------
 * oidc_rp_link_run_query
 * -------------------------------------------------------------------------
 */

void test_run_query_routes_through_seam(void) {
    seam_push(80, true, make_080_hit(1, 2), NULL);

    json_t *params = json_object();
    QueryResult *r = oidc_rp_link_run_query(80, "Lithium", params);
    json_decref(params);

    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_TRUE(r->success);
    TEST_ASSERT_EQUAL_INT(80, g_last_query_ref);
    free_query_result(r);
}

void test_run_query_null_seam_returns_null(void) {
    oidc_rp_link_test_clear_query_fn();

    json_t *params = json_object();
    QueryResult *r = oidc_rp_link_run_query(80, "Lithium", params);
    json_decref(params);

    /* No seam installed and execute_auth_query has no live DB → NULL. */
    TEST_ASSERT_NULL(r);

    /* Re-install the seam for the rest of the suite. */
    oidc_rp_link_test_set_query_fn(seam_fn);
}

/* -------------------------------------------------------------------------
 * oidc_rp_link_safe_iss
 * -------------------------------------------------------------------------
 */

void test_safe_iss_null_writes_null_marker(void) {
    char out[OIDC_RP_LINK_ISS_LOG_LEN + 1];
    oidc_rp_link_safe_iss(NULL, out);
    TEST_ASSERT_EQUAL_STRING("(null)", out);
}

void test_safe_iss_truncates_long_value(void) {
    char out[OIDC_RP_LINK_ISS_LOG_LEN + 1];
    const char *long_iss =
        "https://a-very-long-issuer-hostname.example.com/path/that/keeps/going";
    oidc_rp_link_safe_iss(long_iss, out);
    TEST_ASSERT_EQUAL_INT(OIDC_RP_LINK_ISS_LOG_LEN, (int)strlen(out));
}

void test_safe_iss_copies_short_value(void) {
    char out[OIDC_RP_LINK_ISS_LOG_LEN + 1];
    oidc_rp_link_safe_iss("https://example.com", out);
    TEST_ASSERT_EQUAL_STRING("https://example.com", out);
}

/* -------------------------------------------------------------------------
 * oidc_rp_link_safe_sub
 * -------------------------------------------------------------------------
 */

void test_safe_sub_null_writes_null_marker(void) {
    char out[OIDC_RP_LINK_SUB_LOG_LEN + 1];
    oidc_rp_link_safe_sub(NULL, out);
    TEST_ASSERT_EQUAL_STRING("(null)", out);
}

void test_safe_sub_truncates_long_value(void) {
    char out[OIDC_RP_LINK_SUB_LOG_LEN + 1];
    char long_sub[64];
    memset(long_sub, 'x', sizeof(long_sub) - 1);
    long_sub[sizeof(long_sub) - 1] = '\0';
    oidc_rp_link_safe_sub(long_sub, out);
    TEST_ASSERT_EQUAL_INT(OIDC_RP_LINK_SUB_LOG_LEN, (int)strlen(out));
}

void test_safe_sub_copies_short_value(void) {
    char out[OIDC_RP_LINK_SUB_LOG_LEN + 1];
    oidc_rp_link_safe_sub("sub-abc", out);
    TEST_ASSERT_EQUAL_STRING("sub-abc", out);
}

/* -------------------------------------------------------------------------
 * oidc_rp_link_build_account_info
 * -------------------------------------------------------------------------
 */

void test_build_account_info_prefers_preferred_username(void) {
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "the-sub", "alice", "alice@example.com", true);
    account_info_t *account = oidc_rp_link_build_account_info(7, &claims);
    TEST_ASSERT_NOT_NULL(account);
    TEST_ASSERT_EQUAL_INT(7, account->id);
    TEST_ASSERT_EQUAL_STRING("alice", account->username);
    free_account_info(account);
}

void test_build_account_info_falls_back_to_sub(void) {
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "my-sub", NULL, "my-sub@example.com", true);
    account_info_t *account = oidc_rp_link_build_account_info(9, &claims);
    TEST_ASSERT_NOT_NULL(account);
    TEST_ASSERT_EQUAL_STRING("my-sub", account->username);
    free_account_info(account);
}

void test_build_account_info_copies_email_when_present(void) {
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub", "bob", "bob@example.com", true);
    account_info_t *account = oidc_rp_link_build_account_info(3, &claims);
    TEST_ASSERT_NOT_NULL(account);
    TEST_ASSERT_NOT_NULL(account->email);
    TEST_ASSERT_EQUAL_STRING("bob@example.com", account->email);
    free_account_info(account);
}

void test_build_account_info_no_email_yields_null_email(void) {
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub", "carol", NULL, true);
    account_info_t *account = oidc_rp_link_build_account_info(4, &claims);
    TEST_ASSERT_NOT_NULL(account);
    TEST_ASSERT_NULL(account->email);
    free_account_info(account);
}

void test_build_account_info_roles_empty_and_enabled(void) {
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub", "dan", "dan@example.com", true);
    account_info_t *account = oidc_rp_link_build_account_info(5, &claims);
    TEST_ASSERT_NOT_NULL(account);
    TEST_ASSERT_TRUE(account->enabled);
    TEST_ASSERT_TRUE(account->authorized);
    TEST_ASSERT_NOT_NULL(account->roles);
    TEST_ASSERT_EQUAL_STRING("", account->roles);
    free_account_info(account);
}

void test_build_account_info_negative_id_accepted(void) {
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub", "erin", NULL, true);
    account_info_t *account = oidc_rp_link_build_account_info(-1, &claims);
    TEST_ASSERT_NOT_NULL(account);
    TEST_ASSERT_EQUAL_INT(-1, account->id);
    free_account_info(account);
}

/* -------------------------------------------------------------------------
 * oidc_rp_link_query_080_lookup
 * -------------------------------------------------------------------------
 */

void test_query_080_hit_populates_ids(void) {
    char *hit = make_080_hit(11, 22);
    seam_push(80, true, hit, NULL);
    free(hit);

    int identity_id = -1, account_id = -1;
    bool db_error = false;
    oidc_rp_link_query_080_lookup("https://example.com", "sub-x",
                                  "Lithium", &identity_id, &account_id, &db_error);

    TEST_ASSERT_FALSE(db_error);
    TEST_ASSERT_EQUAL_INT(11, identity_id);
    TEST_ASSERT_EQUAL_INT(22, account_id);
}

void test_query_080_miss_leaves_ids_negative(void) {
    char *miss = make_080_miss();
    seam_push(80, true, miss, NULL);
    free(miss);

    int identity_id = -1, account_id = -1;
    bool db_error = true;
    oidc_rp_link_query_080_lookup("https://example.com", "sub-x",
                                  "Lithium", &identity_id, &account_id, &db_error);

    TEST_ASSERT_FALSE(db_error);
    TEST_ASSERT_EQUAL_INT(-1, identity_id);
    TEST_ASSERT_EQUAL_INT(-1, account_id);
}

void test_query_080_null_result_sets_db_error(void) {
    /* Empty seam queue → NULL result. */
    int identity_id = -1, account_id = -1;
    bool db_error = false;
    oidc_rp_link_query_080_lookup("https://example.com", "sub-x",
                                  "Lithium", &identity_id, &account_id, &db_error);

    TEST_ASSERT_TRUE(db_error);
}

void test_query_080_failed_result_sets_db_error(void) {
    seam_push(80, false, NULL, "boom");
    int identity_id = -1, account_id = -1;
    bool db_error = false;
    oidc_rp_link_query_080_lookup("https://example.com", "sub-x",
                                  "Lithium", &identity_id, &account_id, &db_error);

    TEST_ASSERT_TRUE(db_error);
}

/* -------------------------------------------------------------------------
 * oidc_rp_link_query_081_link_identity
 * -------------------------------------------------------------------------
 */

void test_query_081_success_returns_identity_id(void) {
    char *ok = make_081_ok(55);
    seam_push(81, true, ok, NULL);
    free(ok);

    int identity_id = -1;
    bool ok_link = oidc_rp_link_query_081_link_identity(
        7, "https://example.com", "sub-x", "x@example.com", true, "Lithium",
        &identity_id);

    TEST_ASSERT_TRUE(ok_link);
    TEST_ASSERT_EQUAL_INT(55, identity_id);
}

void test_query_081_null_result_returns_false(void) {
    /* Empty seam queue → NULL result. */
    int identity_id = -1;
    bool ok_link = oidc_rp_link_query_081_link_identity(
        7, "https://example.com", "sub-x", NULL, true, "Lithium", &identity_id);

    TEST_ASSERT_FALSE(ok_link);
}

void test_query_081_failed_result_returns_true_retry(void) {
    /* A DB-layer failure (incl. UNIQUE race) is reported as success so the
     * caller can retry with #080. */
    seam_push(81, false, NULL, "UNIQUE violation");
    int identity_id = -1;
    bool ok_link = oidc_rp_link_query_081_link_identity(
        7, "https://example.com", "sub-x", NULL, true, "Lithium", &identity_id);

    TEST_ASSERT_TRUE(ok_link);
    TEST_ASSERT_EQUAL_INT(-1, identity_id);
}

/* -------------------------------------------------------------------------
 * oidc_rp_link_query_082_lookup_by_email
 * -------------------------------------------------------------------------
 */

void test_query_082_single_returns_one_and_account_id(void) {
    char *single = make_082_single(42);
    seam_push(82, true, single, NULL);
    free(single);

    int account_id = -1;
    int rows = oidc_rp_link_query_082_lookup_by_email("a@example.com", "Lithium",
                                                      &account_id);
    TEST_ASSERT_EQUAL_INT(1, rows);
    TEST_ASSERT_EQUAL_INT(42, account_id);
}

void test_query_082_zero_returns_zero(void) {
    char *miss = make_080_miss();
    seam_push(82, true, miss, NULL);
    free(miss);

    int account_id = -1;
    int rows = oidc_rp_link_query_082_lookup_by_email("nobody@example.com",
                                                      "Lithium", &account_id);
    TEST_ASSERT_EQUAL_INT(0, rows);
    TEST_ASSERT_EQUAL_INT(-1, account_id);
}

void test_query_082_ambiguous_returns_two(void) {
    char *ambig = make_082_ambiguous(3, 7);
    seam_push(82, true, ambig, NULL);
    free(ambig);

    int account_id = -1;
    int rows = oidc_rp_link_query_082_lookup_by_email("shared@example.com",
                                                      "Lithium", &account_id);
    TEST_ASSERT_EQUAL_INT(2, rows);
}

void test_query_082_db_error_returns_neg_one(void) {
    seam_push(82, false, NULL, "db down");
    int account_id = -1;
    int rows = oidc_rp_link_query_082_lookup_by_email("x@example.com",
                                                      "Lithium", &account_id);
    TEST_ASSERT_EQUAL_INT(-1, rows);
}

/* -------------------------------------------------------------------------
 * oidc_rp_link_query_083_provision
 * -------------------------------------------------------------------------
 */

void test_query_083_success_returns_account_id(void) {
    char *ok = make_083_ok(99);
    seam_push(83, true, ok, NULL);
    free(ok);

    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-new", "frank", "frank@example.com", true);
    int account_id = oidc_rp_link_query_083_provision(&claims, "Lithium");

    TEST_ASSERT_EQUAL_INT(99, account_id);
}

void test_query_083_null_result_returns_neg_one(void) {
    /* Empty seam queue → NULL result. */
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-new", "grace", "grace@example.com", true);
    int account_id = oidc_rp_link_query_083_provision(&claims, "Lithium");

    TEST_ASSERT_EQUAL_INT(-1, account_id);
}

void test_query_083_failed_result_returns_neg_one(void) {
    seam_push(83, false, NULL, "insert failed");
    OidcRpIdTokenClaims claims = make_claims(
        "https://example.com", "sub-new", "heidi", "heidi@example.com", true);
    int account_id = oidc_rp_link_query_083_provision(&claims, "Lithium");

    TEST_ASSERT_EQUAL_INT(-1, account_id);
}

/* -------------------------------------------------------------------------
 * oidc_rp_link_query_084_touch
 * -------------------------------------------------------------------------
 */

void test_query_084_success_is_non_fatal_ok(void) {
    seam_push(84, true, "[]", NULL);
    /* A successful touch must not raise (we only assert it returns). */
    oidc_rp_link_query_084_touch(12, "x@example.com", true, "Lithium");
    TEST_ASSERT_EQUAL_INT(84, g_last_query_ref);
}

void test_query_084_null_result_is_non_fatal(void) {
    /* Empty seam queue → NULL result; touch is best-effort and must not
     * crash or abort. */
    oidc_rp_link_query_084_touch(12, NULL, false, "Lithium");
    TEST_ASSERT_EQUAL_INT(1, g_call_count);
}

/* -------------------------------------------------------------------------
 * main
 * -------------------------------------------------------------------------
 */

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_run_query_routes_through_seam);
    RUN_TEST(test_run_query_null_seam_returns_null);

    RUN_TEST(test_safe_iss_null_writes_null_marker);
    RUN_TEST(test_safe_iss_truncates_long_value);
    RUN_TEST(test_safe_iss_copies_short_value);

    RUN_TEST(test_safe_sub_null_writes_null_marker);
    RUN_TEST(test_safe_sub_truncates_long_value);
    RUN_TEST(test_safe_sub_copies_short_value);

    RUN_TEST(test_build_account_info_prefers_preferred_username);
    RUN_TEST(test_build_account_info_falls_back_to_sub);
    RUN_TEST(test_build_account_info_copies_email_when_present);
    RUN_TEST(test_build_account_info_no_email_yields_null_email);
    RUN_TEST(test_build_account_info_roles_empty_and_enabled);
    RUN_TEST(test_build_account_info_negative_id_accepted);

    RUN_TEST(test_query_080_hit_populates_ids);
    RUN_TEST(test_query_080_miss_leaves_ids_negative);
    RUN_TEST(test_query_080_null_result_sets_db_error);
    RUN_TEST(test_query_080_failed_result_sets_db_error);

    RUN_TEST(test_query_081_success_returns_identity_id);
    RUN_TEST(test_query_081_null_result_returns_false);
    RUN_TEST(test_query_081_failed_result_returns_true_retry);

    RUN_TEST(test_query_082_single_returns_one_and_account_id);
    RUN_TEST(test_query_082_zero_returns_zero);
    RUN_TEST(test_query_082_ambiguous_returns_two);
    RUN_TEST(test_query_082_db_error_returns_neg_one);

    RUN_TEST(test_query_083_success_returns_account_id);
    RUN_TEST(test_query_083_null_result_returns_neg_one);
    RUN_TEST(test_query_083_failed_result_returns_neg_one);

    RUN_TEST(test_query_084_success_is_non_fatal_ok);
    RUN_TEST(test_query_084_null_result_is_non_fatal);

    return UNITY_END();
}
