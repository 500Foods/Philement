/*
 * Unity Test File: mailrelay_api_auth coverage
 * Tests mailrelay_api_role_lookup_callback(), mailrelay_api_has_role_id()
 * and mailrelay_api_has_role() from src/api/mailrelay/mailrelay_api_auth.c
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <jansson.h>
#include <stdlib.h>
#include <string.h>

#include <src/api/auth/auth_service.h>
#include <src/api/mailrelay/mailrelay_api_auth.h>
#include <src/mailrelay/mailrelay_repository.h>

/* Mirror of the file-local TestRoleLookupContext in mailrelay_api_auth.c.
 * Layout MUST match: { bool found; int role_id; }. Passed as void*. */
typedef struct {
    bool found;
    int  role_id;
} TestRoleLookupContext;

/* Forward declarations */
void mailrelay_api_role_lookup_callback(MailRelayRepoResult* result, void* user_data);
bool mailrelay_api_has_role_id(const char* roles, const char* role_id_str);
bool mailrelay_api_has_role(const jwt_claims_t* claims, const char* role_name);

void test_role_lookup_callback_null_result_and_ctx(void);
void test_role_lookup_callback_bad_status(void);
void test_role_lookup_callback_null_data(void);
void test_role_lookup_callback_data_not_array(void);
void test_role_lookup_callback_empty_array(void);
void test_role_lookup_callback_row_missing_role(void);
void test_role_lookup_callback_role_found_lowercase(void);
void test_role_lookup_callback_role_found_uppercase(void);
void test_has_role_id_null_and_empty(void);
void test_has_role_id_match_at_start(void);
void test_has_role_id_match_in_middle(void);
void test_has_role_id_match_at_end(void);
void test_has_role_id_match_with_trailing_spaces(void);
void test_has_role_id_match_with_leading_tab(void);
void test_has_role_id_tab_is_not_delimiter(void);
void test_has_role_id_no_match(void);
void test_has_role_id_substring_not_match(void);
void test_has_role_null_claims(void);
void test_has_role_null_role_name(void);
void test_has_role_empty_role_name(void);
void test_has_role_repo_failure(void);
void test_has_role_repo_returns_not_found(void);
void test_has_role_found_and_matches(void);
void test_has_role_found_but_not_in_claims(void);

/* ---------------------------------------------------------------------------
 * mailrelay_api_has_role depends on mailrelay_repo_role_get_by_name which
 * ultimately runs through repo_execute_json. We inject a seam executor that
 * invokes the captured callback synchronously with a synthetic result so we can
 * drive every branch of mailrelay_api_has_role and the lookup callback.
 * ------------------------------------------------------------------------- */

static bool                       g_executor_return = true;
static MailRelayRepoResult        g_injected_result = { 0 };
static bool                       g_inject_result = false;

/* Owning pointer for synthetic JSON result data, freed between tests. */
static json_t*                   g_injected_data = NULL;

static bool mock_executor(int query_ref,
                          const char* params_json,
                          mailrelay_repo_callback_fn callback,
                          void* user_data) {
    (void)query_ref;
    (void)params_json;
    if (g_inject_result) {
        callback(&g_injected_result, user_data);
    }
    return g_executor_return;
}

static void install_seam(void) {
    g_executor_return = true;
    g_inject_result = false;
    memset(&g_injected_result, 0, sizeof(g_injected_result));
    g_injected_result.status = MAILRELAY_REPO_OK;
    if (g_injected_data) {
        json_decref(g_injected_data);
        g_injected_data = NULL;
    }
    mailrelay_repo_set_executor(mock_executor);
}

void setUp(void) {
    install_seam();
}

void tearDown(void) {
    if (g_injected_data) {
        json_decref(g_injected_data);
        g_injected_data = NULL;
    }
    mailrelay_repo_set_executor(NULL);
}

/* ===========================================================================
 * mailrelay_api_role_lookup_callback
 * =========================================================================== */

void test_role_lookup_callback_null_result_and_ctx(void) {
    TestRoleLookupContext ctx = { .found = true, .role_id = 7 };

    mailrelay_api_role_lookup_callback(NULL, &ctx);
    TEST_ASSERT_TRUE(ctx.found);
    TEST_ASSERT_EQUAL_INT(7, ctx.role_id);

    mailrelay_api_role_lookup_callback((MailRelayRepoResult*)0x1, NULL);
}

void test_role_lookup_callback_bad_status(void) {
    TestRoleLookupContext ctx = { .found = true, .role_id = 7 };
    MailRelayRepoResult result = { .status = MAILRELAY_REPO_QUERY_ERROR, .data = NULL };
    mailrelay_api_role_lookup_callback(&result, &ctx);
    TEST_ASSERT_FALSE(ctx.found);
}

void test_role_lookup_callback_null_data(void) {
    TestRoleLookupContext ctx = { .found = true, .role_id = 7 };
    MailRelayRepoResult result = { .status = MAILRELAY_REPO_OK, .data = NULL };
    mailrelay_api_role_lookup_callback(&result, &ctx);
    TEST_ASSERT_FALSE(ctx.found);
}

void test_role_lookup_callback_data_not_array(void) {
    TestRoleLookupContext ctx = { .found = true, .role_id = 7 };
    json_t* obj = json_object();
    MailRelayRepoResult result = { .status = MAILRELAY_REPO_OK, .data = obj };
    mailrelay_api_role_lookup_callback(&result, &ctx);
    TEST_ASSERT_FALSE(ctx.found);
    json_decref(obj);
}

void test_role_lookup_callback_empty_array(void) {
    TestRoleLookupContext ctx = { .found = true, .role_id = 7 };
    json_t* arr = json_array();
    MailRelayRepoResult result = { .status = MAILRELAY_REPO_OK, .data = arr };
    mailrelay_api_role_lookup_callback(&result, &ctx);
    TEST_ASSERT_FALSE(ctx.found);
    json_decref(arr);
}

void test_role_lookup_callback_row_missing_role(void) {
    /* When the row has no role_id the callback does not set found; it relies
     * on the caller zero-initializing the context. Verify it leaves the
     * initial value untouched. */
    TestRoleLookupContext ctx = { .found = false, .role_id = 0 };
    json_t* row = json_object();
    json_t* arr = json_array();
    json_array_append_new(arr, row);
    MailRelayRepoResult result = { .status = MAILRELAY_REPO_OK, .data = arr };
    mailrelay_api_role_lookup_callback(&result, &ctx);
    TEST_ASSERT_FALSE(ctx.found);
    TEST_ASSERT_EQUAL_INT(0, ctx.role_id);
    json_decref(arr);
}

void test_role_lookup_callback_role_found_lowercase(void) {
    TestRoleLookupContext ctx = { .found = false, .role_id = 0 };
    json_t* row = json_object();
    json_object_set_new(row, "role_id", json_integer(42));
    json_t* arr = json_array();
    json_array_append_new(arr, row);
    MailRelayRepoResult result = { .status = MAILRELAY_REPO_OK, .data = arr };
    mailrelay_api_role_lookup_callback(&result, &ctx);
    TEST_ASSERT_TRUE(ctx.found);
    TEST_ASSERT_EQUAL_INT(42, ctx.role_id);
    json_decref(arr);
}

void test_role_lookup_callback_role_found_uppercase(void) {
    TestRoleLookupContext ctx = { .found = false, .role_id = 0 };
    json_t* row = json_object();
    json_object_set_new(row, "ROLE_ID", json_integer(13));
    json_t* arr = json_array();
    json_array_append_new(arr, row);
    MailRelayRepoResult result = { .status = MAILRELAY_REPO_OK, .data = arr };
    mailrelay_api_role_lookup_callback(&result, &ctx);
    TEST_ASSERT_TRUE(ctx.found);
    TEST_ASSERT_EQUAL_INT(13, ctx.role_id);
    json_decref(arr);
}

/* ===========================================================================
 * mailrelay_api_has_role_id
 * =========================================================================== */

void test_has_role_id_null_and_empty(void) {
    TEST_ASSERT_FALSE(mailrelay_api_has_role_id(NULL, "1"));
    TEST_ASSERT_FALSE(mailrelay_api_has_role_id("1,2,3", NULL));
    TEST_ASSERT_FALSE(mailrelay_api_has_role_id("1,2,3", ""));
}

void test_has_role_id_match_at_start(void) {
    TEST_ASSERT_TRUE(mailrelay_api_has_role_id("5,6,7", "5"));
}

void test_has_role_id_match_in_middle(void) {
    TEST_ASSERT_TRUE(mailrelay_api_has_role_id("5,6,7", "6"));
}

void test_has_role_id_match_at_end(void) {
    TEST_ASSERT_TRUE(mailrelay_api_has_role_id("5,6,7", "7"));
}

void test_has_role_id_match_with_trailing_spaces(void) {
    TEST_ASSERT_TRUE(mailrelay_api_has_role_id(" 5 , 6 , 7 ", "6"));
}

void test_has_role_id_match_with_leading_tab(void) {
    /* A leading tab is skipped like a space; ids are still comma-separated. */
    TEST_ASSERT_TRUE(mailrelay_api_has_role_id("\t6,7,8", "6"));
}

void test_has_role_id_tab_is_not_delimiter(void) {
    /* Tabs between ids are not treated as separators; "5\t6\t7" has no
     * standalone "6" token, so it must not match. */
    TEST_ASSERT_FALSE(mailrelay_api_has_role_id("5\t6\t7", "6"));
}

void test_has_role_id_no_match(void) {
    TEST_ASSERT_FALSE(mailrelay_api_has_role_id("1,2,3", "4"));
}

void test_has_role_id_substring_not_match(void) {
    /* "12" is present as a standalone id here */
    TEST_ASSERT_TRUE(mailrelay_api_has_role_id("12", "12"));
    /* but not as a standalone id inside "1,2,3" */
    TEST_ASSERT_FALSE(mailrelay_api_has_role_id("1,2,3", "12"));
    /* and not a prefix of "121" */
    TEST_ASSERT_FALSE(mailrelay_api_has_role_id("121,2,3", "12"));
}

/* ===========================================================================
 * mailrelay_api_has_role
 * =========================================================================== */

void test_has_role_null_claims(void) {
    TEST_ASSERT_FALSE(mailrelay_api_has_role(NULL, "mail_send"));
}

void test_has_role_null_role_name(void) {
    jwt_claims_t claims = { .roles = (char*)"1,2,3" };
    TEST_ASSERT_FALSE(mailrelay_api_has_role(&claims, NULL));
}

void test_has_role_empty_role_name(void) {
    jwt_claims_t claims = { .roles = (char*)"1,2,3" };
    TEST_ASSERT_FALSE(mailrelay_api_has_role(&claims, ""));
}

void test_has_role_repo_failure(void) {
    g_executor_return = false;
    jwt_claims_t claims = { .roles = (char*)"1,2,3" };
    TEST_ASSERT_FALSE(mailrelay_api_has_role(&claims, "mail_send"));
}

void test_has_role_repo_returns_not_found(void) {
    jwt_claims_t claims = { .roles = (char*)"1,2,3" };

    g_injected_result.status = MAILRELAY_REPO_QUERY_ERROR;
    g_injected_result.data = NULL;
    g_inject_result = true;
    TEST_ASSERT_FALSE(mailrelay_api_has_role(&claims, "mail_send"));
}

static void set_injected_role_array(int role_id) {
    json_t* row = json_object();
    json_object_set_new(row, "role_id", json_integer(role_id));
    g_injected_data = json_array();
    json_array_append_new(g_injected_data, row);
    g_injected_result.status = MAILRELAY_REPO_OK;
    g_injected_result.data = g_injected_data;
    g_inject_result = true;
}

void test_has_role_found_and_matches(void) {
    jwt_claims_t claims = { .roles = (char*)"10,11,12" };
    set_injected_role_array(11);
    TEST_ASSERT_TRUE(mailrelay_api_has_role(&claims, "mail_send"));
}

void test_has_role_found_but_not_in_claims(void) {
    jwt_claims_t claims = { .roles = (char*)"10,20,30" };
    set_injected_role_array(11);
    TEST_ASSERT_FALSE(mailrelay_api_has_role(&claims, "mail_send"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_role_lookup_callback_null_result_and_ctx);
    RUN_TEST(test_role_lookup_callback_bad_status);
    RUN_TEST(test_role_lookup_callback_null_data);
    RUN_TEST(test_role_lookup_callback_data_not_array);
    RUN_TEST(test_role_lookup_callback_empty_array);
    RUN_TEST(test_role_lookup_callback_row_missing_role);
    RUN_TEST(test_role_lookup_callback_role_found_lowercase);
    RUN_TEST(test_role_lookup_callback_role_found_uppercase);

    RUN_TEST(test_has_role_id_null_and_empty);
    RUN_TEST(test_has_role_id_match_at_start);
    RUN_TEST(test_has_role_id_match_in_middle);
    RUN_TEST(test_has_role_id_match_at_end);
    RUN_TEST(test_has_role_id_match_with_trailing_spaces);
    RUN_TEST(test_has_role_id_match_with_leading_tab);
    RUN_TEST(test_has_role_id_tab_is_not_delimiter);
    RUN_TEST(test_has_role_id_no_match);
    RUN_TEST(test_has_role_id_substring_not_match);

    RUN_TEST(test_has_role_null_claims);
    RUN_TEST(test_has_role_null_role_name);
    RUN_TEST(test_has_role_empty_role_name);
    RUN_TEST(test_has_role_repo_failure);
    RUN_TEST(test_has_role_repo_returns_not_found);
    RUN_TEST(test_has_role_found_and_matches);
    RUN_TEST(test_has_role_found_but_not_in_claims);

    return UNITY_END();
}
