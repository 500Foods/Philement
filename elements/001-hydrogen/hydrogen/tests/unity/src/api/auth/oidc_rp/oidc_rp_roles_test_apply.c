/*
 * Unity Test File: OIDC RP role mapping — Phase 22.
 *
 * Tests `oidc_rp_roles_apply` for all four RoleMapping.Source values:
 *   DATABASE, IDP_REALM_ROLES, IDP_CLIENT_ROLES, MERGE.
 *
 * The real `execute_auth_query` requires a live database; we avoid that by
 * installing the `oidc_rp_roles_test_set_query_fn` test seam in `setUp` and
 * removing it in `tearDown`. The seam is a simple FIFO of canned QueryResult
 * responses, matching the identical pattern used in oidc_rp_link_test_*.c.
 *
 * Coverage (22 tests):
 *
 *   NULL-safety:
 *     - NULL provider → no-op (no crash).
 *     - NULL account  → no-op (no crash).
 *
 *   DATABASE source:
 *     - Single role_id returned → account->roles = "1".
 *     - Multiple role_ids returned → comma-separated "1,3,7".
 *     - No rows returned → account->roles = "".
 *     - QueryRef #017 transport failure (NULL result) → roles unchanged.
 *     - QueryRef #017 DB error (success=false) → roles unchanged.
 *     - Correct query_ref (17) is called.
 *
 *   IDP_REALM_ROLES source:
 *     - Single IdP role, no prefix → account->roles = "reader".
 *     - Multiple IdP roles, no prefix → "reader,writer".
 *     - Single IdP role, with prefix "kc:" → "kc:reader".
 *     - No IdP roles in claims → account->roles = "".
 *     - No DB query is made (QueryRef #017 must NOT be called).
 *
 *   IDP_CLIENT_ROLES source:
 *     - Behaves identically to IDP_REALM_ROLES in Phase 22
 *       (documented fallback; no crash, correct output).
 *
 *   MERGE source:
 *     - DB roles + IdP roles, no overlap → union "1,3,kc:reader".
 *     - Duplicate role between DB and IdP (same string) → deduplicated.
 *     - DB empty + IdP non-empty → IdP roles only.
 *     - DB non-empty + IdP empty → DB roles only.
 *     - DB failure (NULL from seam) + IdP non-empty → IdP roles only
 *       (non-fatal partial fallback).
 *
 * Total: 22 tests.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/auth/oidc_rp/oidc_rp_roles.h>
#include <src/api/auth/oidc_rp/oidc_rp_idtoken.h>
#include <src/api/auth/auth_service.h>
#include <src/config/config_oidc_rp.h>

#include <jansson.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Test seam FIFO
 * -------------------------------------------------------------------------
 * Identical structure to oidc_rp_link_test_default.c. Each process owns
 * its own seam globals; no shared state with link test binaries.
 */

#define SEAM_QUEUE_CAP 8

typedef struct {
    int   query_ref;
    bool  success;
    char *data_json;
    char *error_msg;
} SeamEntry;

static SeamEntry g_seam_queue[SEAM_QUEUE_CAP];
static int       g_seam_head      = 0;
static int       g_seam_tail      = 0;
static int       g_last_query_ref = -1;
static int       g_call_count     = 0;

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
    if (next == g_seam_head) return; /* queue full — test bug */
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
        return NULL; /* empty → transport failure */
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
 * Fixture helpers
 * -------------------------------------------------------------------------
 */

/* Build #017 result JSON with the given role_ids. */
static char *make_017_roles(const int * const ids, size_t count) {
    if (count == 0) return strdup("[]");

    /* "[{\"role_id\":1},{\"role_id\":3}]" */
    size_t cap = 32 + count * 24;
    char *buf = malloc(cap);
    if (!buf) return NULL;

    size_t pos = 0;
    buf[pos++] = '[';
    for (size_t i = 0; i < count; i++) {
        if (i > 0) buf[pos++] = ',';
        pos += (size_t)snprintf(buf + pos, cap - pos,
                                "{\"role_id\":%d}", ids[i]);
    }
    buf[pos++] = ']';
    buf[pos]   = '\0';
    return buf;
}

/* Build a minimal provider config for the given RoleMapping.Source. */
static OIDCRPProviderConfig make_provider(OIDCRPRoleSource source,
                                           const char *prefix) {
    OIDCRPProviderConfig p;
    memset(&p, 0, sizeof(p));
    p.name      = (char *)"test-provider";
    p.client_id = (char *)"lithium-web";
    p.role_mapping.source         = source;
    p.role_mapping.idp_role_prefix = (char *)(prefix ? prefix : "");
    return p;
}

/* Build OidcRpIdTokenClaims with up to 4 realm roles. */
static OidcRpIdTokenClaims make_claims_with_roles(const char *r0,
                                                    const char *r1,
                                                    const char *r2,
                                                    const char *r3) {
    OidcRpIdTokenClaims c;
    memset(&c, 0, sizeof(c));
    /* roles[] are pointers into static strings owned by the caller. */
    if (r0) c.roles[0] = (char *)r0;
    if (r1) c.roles[1] = (char *)r1;
    if (r2) c.roles[2] = (char *)r2;
    if (r3) c.roles[3] = (char *)r3;
    return c;
}

/* Build a minimal account_info_t with roles="" (linker default). */
static account_info_t *make_account(int id) {
    account_info_t *a = calloc(1, sizeof(account_info_t));
    if (!a) return NULL;
    a->id         = id;
    a->enabled    = true;
    a->authorized = true;
    a->username   = strdup("testuser");
    a->roles      = strdup("");
    return a;
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * -------------------------------------------------------------------------
 */

void setUp(void) {
    seam_reset();
    oidc_rp_roles_test_set_query_fn(seam_fn);
}

void tearDown(void) {
    oidc_rp_roles_test_clear_query_fn();
    seam_reset();
}

/* -------------------------------------------------------------------------
 * Forward declarations
 * -------------------------------------------------------------------------
 */
void test_roles_null_provider_is_noop(void);
void test_roles_null_account_is_noop(void);
void test_roles_database_single_role(void);
void test_roles_database_multiple_roles(void);
void test_roles_database_no_rows(void);
void test_roles_database_transport_failure_unchanged(void);
void test_roles_database_query_error_unchanged(void);
void test_roles_database_calls_query_ref_17(void);
void test_roles_idp_realm_single_no_prefix(void);
void test_roles_idp_realm_multiple_no_prefix(void);
void test_roles_idp_realm_single_with_prefix(void);
void test_roles_idp_realm_no_claims_empty(void);
void test_roles_idp_realm_no_db_query(void);
void test_roles_idp_client_behaves_like_realm(void);
void test_roles_merge_db_and_idp_no_overlap(void);
void test_roles_merge_deduplicates_exact_match(void);
void test_roles_merge_db_empty_idp_nonempty(void);
void test_roles_merge_db_nonempty_idp_empty(void);
void test_roles_merge_db_failure_uses_idp(void);
void test_roles_merge_no_prefix_idp(void);
void test_roles_database_unknown_source_defaults_to_database(void);
void test_roles_idp_realm_multiple_with_prefix(void);

/* -------------------------------------------------------------------------
 * Tests — NULL-safety
 * -------------------------------------------------------------------------
 */

void test_roles_null_provider_is_noop(void) {
    account_info_t *a = make_account(1);
    OidcRpIdTokenClaims claims;
    memset(&claims, 0, sizeof(claims));

    /* NULL provider → no-op; must not crash. */
    oidc_rp_roles_apply(NULL, &claims, "Lithium", a);

    TEST_ASSERT_EQUAL_STRING("", a->roles);
    TEST_ASSERT_EQUAL_INT(0, g_call_count); /* seam never called */
    free_account_info(a);
}

void test_roles_null_account_is_noop(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_ROLE_SRC_DATABASE, NULL);
    OidcRpIdTokenClaims claims;
    memset(&claims, 0, sizeof(claims));

    /* NULL account → no-op; must not crash. */
    oidc_rp_roles_apply(&p, &claims, "Lithium", NULL);

    TEST_ASSERT_EQUAL_INT(0, g_call_count);
}

/* -------------------------------------------------------------------------
 * Tests — DATABASE source
 * -------------------------------------------------------------------------
 */

void test_roles_database_single_role(void) {
    OIDCRPProviderConfig p      = make_provider(OIDC_RP_ROLE_SRC_DATABASE, NULL);
    OidcRpIdTokenClaims  claims = make_claims_with_roles(NULL, NULL, NULL, NULL);
    account_info_t *a           = make_account(42);

    const int ids[] = {1};
    char *data = make_017_roles(ids, 1);
    seam_push(17, true, data, NULL);
    free(data);

    oidc_rp_roles_apply(&p, &claims, "Lithium", a);

    TEST_ASSERT_EQUAL_STRING("1", a->roles);
    free_account_info(a);
}

void test_roles_database_multiple_roles(void) {
    OIDCRPProviderConfig p      = make_provider(OIDC_RP_ROLE_SRC_DATABASE, NULL);
    OidcRpIdTokenClaims  claims = make_claims_with_roles(NULL, NULL, NULL, NULL);
    account_info_t *a           = make_account(5);

    const int ids[] = {1, 3, 7};
    char *data = make_017_roles(ids, 3);
    seam_push(17, true, data, NULL);
    free(data);

    oidc_rp_roles_apply(&p, &claims, "Lithium", a);

    TEST_ASSERT_EQUAL_STRING("1,3,7", a->roles);
    free_account_info(a);
}

void test_roles_database_no_rows(void) {
    OIDCRPProviderConfig p      = make_provider(OIDC_RP_ROLE_SRC_DATABASE, NULL);
    OidcRpIdTokenClaims  claims = make_claims_with_roles(NULL, NULL, NULL, NULL);
    account_info_t *a           = make_account(9);

    seam_push(17, true, "[]", NULL);

    oidc_rp_roles_apply(&p, &claims, "Lithium", a);

    TEST_ASSERT_EQUAL_STRING("", a->roles);
    free_account_info(a);
}

void test_roles_database_transport_failure_unchanged(void) {
    OIDCRPProviderConfig p      = make_provider(OIDC_RP_ROLE_SRC_DATABASE, NULL);
    OidcRpIdTokenClaims  claims = make_claims_with_roles(NULL, NULL, NULL, NULL);
    account_info_t *a           = make_account(3);

    /* Queue is empty → seam returns NULL → transport failure → roles unchanged. */
    oidc_rp_roles_apply(&p, &claims, "Lithium", a);

    TEST_ASSERT_EQUAL_STRING("", a->roles); /* unchanged from linker default */
    free_account_info(a);
}

void test_roles_database_query_error_unchanged(void) {
    OIDCRPProviderConfig p      = make_provider(OIDC_RP_ROLE_SRC_DATABASE, NULL);
    OidcRpIdTokenClaims  claims = make_claims_with_roles(NULL, NULL, NULL, NULL);
    account_info_t *a           = make_account(11);

    seam_push(17, false, NULL, "DB connection failed");

    oidc_rp_roles_apply(&p, &claims, "Lithium", a);

    TEST_ASSERT_EQUAL_STRING("", a->roles); /* unchanged */
    free_account_info(a);
}

void test_roles_database_calls_query_ref_17(void) {
    OIDCRPProviderConfig p      = make_provider(OIDC_RP_ROLE_SRC_DATABASE, NULL);
    OidcRpIdTokenClaims  claims = make_claims_with_roles(NULL, NULL, NULL, NULL);
    account_info_t *a           = make_account(7);

    seam_push(17, true, "[]", NULL);

    oidc_rp_roles_apply(&p, &claims, "Lithium", a);

    TEST_ASSERT_EQUAL_INT(17, g_last_query_ref);
    TEST_ASSERT_EQUAL_INT(1,  g_call_count);
    free_account_info(a);
}

/* -------------------------------------------------------------------------
 * Tests — IDP_REALM_ROLES source
 * -------------------------------------------------------------------------
 */

void test_roles_idp_realm_single_no_prefix(void) {
    OIDCRPProviderConfig p      = make_provider(OIDC_RP_ROLE_SRC_IDP_REALM_ROLES, "");
    OidcRpIdTokenClaims  claims = make_claims_with_roles("reader", NULL, NULL, NULL);
    account_info_t *a           = make_account(1);

    oidc_rp_roles_apply(&p, &claims, "Lithium", a);

    TEST_ASSERT_EQUAL_STRING("reader", a->roles);
    free_account_info(a);
}

void test_roles_idp_realm_multiple_no_prefix(void) {
    OIDCRPProviderConfig p      = make_provider(OIDC_RP_ROLE_SRC_IDP_REALM_ROLES, NULL);
    OidcRpIdTokenClaims  claims = make_claims_with_roles("reader", "writer", NULL, NULL);
    account_info_t *a           = make_account(2);

    oidc_rp_roles_apply(&p, &claims, "Lithium", a);

    TEST_ASSERT_EQUAL_STRING("reader,writer", a->roles);
    free_account_info(a);
}

void test_roles_idp_realm_single_with_prefix(void) {
    OIDCRPProviderConfig p      = make_provider(OIDC_RP_ROLE_SRC_IDP_REALM_ROLES, "kc:");
    OidcRpIdTokenClaims  claims = make_claims_with_roles("reader", NULL, NULL, NULL);
    account_info_t *a           = make_account(3);

    oidc_rp_roles_apply(&p, &claims, "Lithium", a);

    TEST_ASSERT_EQUAL_STRING("kc:reader", a->roles);
    free_account_info(a);
}

void test_roles_idp_realm_multiple_with_prefix(void) {
    OIDCRPProviderConfig p      = make_provider(OIDC_RP_ROLE_SRC_IDP_REALM_ROLES, "kc:");
    OidcRpIdTokenClaims  claims = make_claims_with_roles("reader", "writer", NULL, NULL);
    account_info_t *a           = make_account(4);

    oidc_rp_roles_apply(&p, &claims, "Lithium", a);

    TEST_ASSERT_EQUAL_STRING("kc:reader,kc:writer", a->roles);
    free_account_info(a);
}

void test_roles_idp_realm_no_claims_empty(void) {
    OIDCRPProviderConfig p      = make_provider(OIDC_RP_ROLE_SRC_IDP_REALM_ROLES, "kc:");
    OidcRpIdTokenClaims  claims = make_claims_with_roles(NULL, NULL, NULL, NULL);
    account_info_t *a           = make_account(5);

    oidc_rp_roles_apply(&p, &claims, "Lithium", a);

    TEST_ASSERT_EQUAL_STRING("", a->roles);
    free_account_info(a);
}

void test_roles_idp_realm_no_db_query(void) {
    OIDCRPProviderConfig p      = make_provider(OIDC_RP_ROLE_SRC_IDP_REALM_ROLES, NULL);
    OidcRpIdTokenClaims  claims = make_claims_with_roles("editor", NULL, NULL, NULL);
    account_info_t *a           = make_account(6);

    /* No seam entries pushed; if the implementation calls the DB it returns
     * NULL (transport failure). Verify the DB was NOT called. */
    oidc_rp_roles_apply(&p, &claims, "Lithium", a);

    TEST_ASSERT_EQUAL_STRING("editor", a->roles);
    TEST_ASSERT_EQUAL_INT(0, g_call_count); /* no DB query */
    free_account_info(a);
}

/* -------------------------------------------------------------------------
 * Tests — IDP_CLIENT_ROLES source (Phase 22 fallback = realm roles)
 * -------------------------------------------------------------------------
 */

void test_roles_idp_client_behaves_like_realm(void) {
    OIDCRPProviderConfig p      = make_provider(OIDC_RP_ROLE_SRC_IDP_CLIENT_ROLES, "cl:");
    OidcRpIdTokenClaims  claims = make_claims_with_roles("admin", NULL, NULL, NULL);
    account_info_t *a           = make_account(7);

    oidc_rp_roles_apply(&p, &claims, "Lithium", a);

    /* Phase 22 fallback: IDP_CLIENT_ROLES mirrors IDP_REALM_ROLES. */
    TEST_ASSERT_EQUAL_STRING("cl:admin", a->roles);
    TEST_ASSERT_EQUAL_INT(0, g_call_count); /* no DB query */
    free_account_info(a);
}

/* -------------------------------------------------------------------------
 * Tests — MERGE source
 * -------------------------------------------------------------------------
 */

void test_roles_merge_db_and_idp_no_overlap(void) {
    OIDCRPProviderConfig p      = make_provider(OIDC_RP_ROLE_SRC_MERGE, "kc:");
    OidcRpIdTokenClaims  claims = make_claims_with_roles("reader", NULL, NULL, NULL);
    account_info_t *a           = make_account(8);

    /* DB: role_id 1 and 3 */
    const int ids[] = {1, 3};
    char *data = make_017_roles(ids, 2);
    seam_push(17, true, data, NULL);
    free(data);

    oidc_rp_roles_apply(&p, &claims, "Lithium", a);

    /* DB roles first ("1,3"), then IdP role with prefix ("kc:reader"). */
    TEST_ASSERT_EQUAL_STRING("1,3,kc:reader", a->roles);
    free_account_info(a);
}

void test_roles_merge_deduplicates_exact_match(void) {
    /* IdP role (without prefix) matches a DB role string exactly.
     * Use empty prefix so the IdP role "1" is the same as DB role_id "1". */
    OIDCRPProviderConfig p      = make_provider(OIDC_RP_ROLE_SRC_MERGE, "");
    OidcRpIdTokenClaims  claims = make_claims_with_roles("1", "editor", NULL, NULL);
    account_info_t *a           = make_account(9);

    const int ids[] = {1, 3};
    char *data = make_017_roles(ids, 2);
    seam_push(17, true, data, NULL);
    free(data);

    oidc_rp_roles_apply(&p, &claims, "Lithium", a);

    /* "1" appears in both DB and IdP; should appear only once. */
    TEST_ASSERT_EQUAL_STRING("1,3,editor", a->roles);
    free_account_info(a);
}

void test_roles_merge_db_empty_idp_nonempty(void) {
    OIDCRPProviderConfig p      = make_provider(OIDC_RP_ROLE_SRC_MERGE, "kc:");
    OidcRpIdTokenClaims  claims = make_claims_with_roles("writer", NULL, NULL, NULL);
    account_info_t *a           = make_account(10);

    seam_push(17, true, "[]", NULL); /* DB returns no roles */

    oidc_rp_roles_apply(&p, &claims, "Lithium", a);

    TEST_ASSERT_EQUAL_STRING("kc:writer", a->roles);
    free_account_info(a);
}

void test_roles_merge_db_nonempty_idp_empty(void) {
    OIDCRPProviderConfig p      = make_provider(OIDC_RP_ROLE_SRC_MERGE, "kc:");
    OidcRpIdTokenClaims  claims = make_claims_with_roles(NULL, NULL, NULL, NULL);
    account_info_t *a           = make_account(11);

    const int ids[] = {5};
    char *data = make_017_roles(ids, 1);
    seam_push(17, true, data, NULL);
    free(data);

    oidc_rp_roles_apply(&p, &claims, "Lithium", a);

    TEST_ASSERT_EQUAL_STRING("5", a->roles);
    free_account_info(a);
}

void test_roles_merge_db_failure_uses_idp(void) {
    /* DB seam queue empty → transport failure → fallback to IdP roles. */
    OIDCRPProviderConfig p      = make_provider(OIDC_RP_ROLE_SRC_MERGE, "kc:");
    OidcRpIdTokenClaims  claims = make_claims_with_roles("editor", NULL, NULL, NULL);
    account_info_t *a           = make_account(12);

    /* No seam entries for #017 → NULL returned → DB half fails. */
    oidc_rp_roles_apply(&p, &claims, "Lithium", a);

    TEST_ASSERT_EQUAL_STRING("kc:editor", a->roles);
    free_account_info(a);
}

void test_roles_merge_no_prefix_idp(void) {
    OIDCRPProviderConfig p      = make_provider(OIDC_RP_ROLE_SRC_MERGE, NULL);
    OidcRpIdTokenClaims  claims = make_claims_with_roles("admin", NULL, NULL, NULL);
    account_info_t *a           = make_account(13);

    const int ids[] = {2};
    char *data = make_017_roles(ids, 1);
    seam_push(17, true, data, NULL);
    free(data);

    oidc_rp_roles_apply(&p, &claims, "Lithium", a);

    TEST_ASSERT_EQUAL_STRING("2,admin", a->roles);
    free_account_info(a);
}

/* -------------------------------------------------------------------------
 * Tests — edge cases
 * -------------------------------------------------------------------------
 */

void test_roles_database_unknown_source_defaults_to_database(void) {
    OIDCRPProviderConfig p      = make_provider((OIDCRPRoleSource)99, NULL);
    OidcRpIdTokenClaims  claims = make_claims_with_roles(NULL, NULL, NULL, NULL);
    account_info_t *a           = make_account(14);

    const int ids[] = {4};
    char *data = make_017_roles(ids, 1);
    seam_push(17, true, data, NULL);
    free(data);

    oidc_rp_roles_apply(&p, &claims, "Lithium", a);

    /* Unknown source → DATABASE fallback → role_id "4". */
    TEST_ASSERT_EQUAL_STRING("4", a->roles);
    TEST_ASSERT_EQUAL_INT(17, g_last_query_ref);
    free_account_info(a);
}

/* -------------------------------------------------------------------------
 * main
 * -------------------------------------------------------------------------
 */

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_roles_null_provider_is_noop);
    RUN_TEST(test_roles_null_account_is_noop);

    RUN_TEST(test_roles_database_single_role);
    RUN_TEST(test_roles_database_multiple_roles);
    RUN_TEST(test_roles_database_no_rows);
    RUN_TEST(test_roles_database_transport_failure_unchanged);
    RUN_TEST(test_roles_database_query_error_unchanged);
    RUN_TEST(test_roles_database_calls_query_ref_17);

    RUN_TEST(test_roles_idp_realm_single_no_prefix);
    RUN_TEST(test_roles_idp_realm_multiple_no_prefix);
    RUN_TEST(test_roles_idp_realm_single_with_prefix);
    RUN_TEST(test_roles_idp_realm_multiple_with_prefix);
    RUN_TEST(test_roles_idp_realm_no_claims_empty);
    RUN_TEST(test_roles_idp_realm_no_db_query);

    RUN_TEST(test_roles_idp_client_behaves_like_realm);

    RUN_TEST(test_roles_merge_db_and_idp_no_overlap);
    RUN_TEST(test_roles_merge_deduplicates_exact_match);
    RUN_TEST(test_roles_merge_db_empty_idp_nonempty);
    RUN_TEST(test_roles_merge_db_nonempty_idp_empty);
    RUN_TEST(test_roles_merge_db_failure_uses_idp);
    RUN_TEST(test_roles_merge_no_prefix_idp);

    RUN_TEST(test_roles_database_unknown_source_defaults_to_database);

    return UNITY_END();
}
