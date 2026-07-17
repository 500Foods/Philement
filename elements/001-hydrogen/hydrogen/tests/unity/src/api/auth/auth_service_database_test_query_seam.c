/*
 * Unity Test File: query execution success/error paths via test seam
 * This file uses the auth_service_database query seam to inject canned
 * QueryResult responses, exercising the parsing and error branches of the
 * database wrapper functions without requiring a live database.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_database.h>
#include <jansson.h>
#include <string.h>
#include <time.h>

// Forward declarations for the seam install/clear helpers
void auth_service_database_test_set_query_fn(AuthServiceDatabaseQueryFn fn);
void auth_service_database_test_clear_query_fn(void);

// Forward declarations for functions being tested
account_info_t* lookup_account(const char* login_id, const char* database);
bool verify_password_and_status(const char* password, int account_id, const char* database, account_info_t* account);
bool verify_api_key(const char* api_key, const char* database, system_info_t* sys_info);
bool check_username_availability(const char* username, const char* database);
int create_account_record(const char* username, const char* email,
                          const char* hashed_password, const char* full_name, const char* database);
bool is_token_revoked(const char* token_hash, const char* ip_address, const char* database);
int check_failed_attempts(const char* login_id, const char* client_ip,
                          time_t window_start, const char* database);
void block_ip_address(const char* client_ip, int duration_minutes, const char* database);
void free_query_result(QueryResult* result);

// Forward declarations for test functions
void test_free_query_result_with_columns(void);
void test_execute_auth_query_seam_success_path(void);
void test_lookup_account_success(void);
void test_lookup_account_uppercase_fallback(void);
void test_lookup_account_parse_error(void);
void test_lookup_account_query_failure(void);
void test_verify_password_and_status_success(void);
void test_verify_password_and_status_query_failure(void);
void test_verify_password_and_status_parse_error(void);
void test_verify_password_and_status_username_populated(void);
void test_verify_api_key_success_full(void);
void test_verify_api_key_missing_fields(void);
void test_verify_api_key_integer_valid_until(void);
void test_verify_api_key_invalid_timestamp(void);
void test_verify_api_key_null_data_json(void);
void test_verify_api_key_no_row(void);
void test_verify_api_key_query_failure(void);
void test_check_username_availability_success(void);
void test_check_username_availability_uppercase_fallback(void);
void test_check_username_availability_query_failure(void);
void test_create_account_record_success(void);
void test_create_account_record_query_failure(void);
void test_create_account_record_no_space_full_name(void);
void test_create_account_record_null_full_name(void);
void test_is_token_revoked_revoked(void);
void test_is_token_revoked_active(void);
void test_is_token_revoked_json_array(void);
void test_is_token_revoked_query_null(void);
void test_is_token_revoked_query_failure(void);
void test_check_failed_attempts_success(void);
void test_check_failed_attempts_query_failure(void);
void test_block_ip_address_success(void);
void test_block_ip_address_failure(void);

/* -------------------------------------------------------------------------
 * Seam backend: a configurable canned response template.
 *
 * seam_query_fn() returns a FRESH QueryResult copy on every call so the
 * caller (the wrapper under test) owns the result exclusively and may
 * free it without affecting the template. This avoids double-free bugs
 * that would occur if the same pointer were handed out and freed twice.
 * -------------------------------------------------------------------------
 */
static bool g_template_success = false;
static char* g_template_data = NULL;     /* owned data_json string (may be NULL) */
static char* g_template_error = NULL;    /* owned error_message string (may be NULL) */
static size_t g_template_row_count = 0;

static QueryResult* seam_query_fn(int query_ref, const char* database, json_t* params) {
    (void)query_ref;
    (void)database;
    (void)params;
    QueryResult* r = calloc(1, sizeof(QueryResult));
    if (!r) return NULL;
    r->success = g_template_success;
    r->data_json = g_template_data ? strdup(g_template_data) : NULL;
    r->error_message = g_template_error ? strdup(g_template_error) : NULL;
    r->row_count = g_template_row_count;
    return r;
}

/* Set a success template carrying data_json (NULL => empty success). */
static void set_success(const char* data_json) {
    g_template_success = true;
    free(g_template_data);
    g_template_data = data_json ? strdup(data_json) : NULL;
    free(g_template_error);
    g_template_error = NULL;
    g_template_row_count = 0;
}

/* Set a success template with an explicit row_count but no data_json. */
static void set_success_rows(size_t row_count) {
    g_template_success = true;
    free(g_template_data);
    g_template_data = NULL;
    free(g_template_error);
    g_template_error = NULL;
    g_template_row_count = row_count;
}

/* Set a failure template carrying an error message (NULL => empty failure). */
static void set_failure(const char* error_message) {
    g_template_success = false;
    free(g_template_data);
    g_template_data = NULL;
    free(g_template_error);
    g_template_error = error_message ? strdup(error_message) : NULL;
    g_template_row_count = 0;
}

/* When true, seam_query_fn returns NULL instead of a result. */
static bool g_return_null = false;

static QueryResult* seam_query_fn_nullable(int query_ref, const char* database, json_t* params) {
    if (g_return_null) return NULL;
    return seam_query_fn(query_ref, database, params);
}

void setUp(void) {
    g_return_null = false;
    g_template_success = false;
    g_template_data = NULL;
    g_template_error = NULL;
    g_template_row_count = 0;
    auth_service_database_test_set_query_fn(seam_query_fn_nullable);
}

void tearDown(void) {
    auth_service_database_test_clear_query_fn();
    g_return_null = false;
    free(g_template_data);
    g_template_data = NULL;
    free(g_template_error);
    g_template_error = NULL;
    g_template_row_count = 0;
}

/**
 * Test: free_query_result_with_columns
 * Purpose: Cover the column_names free loop (lines 38-43)
 */
void test_free_query_result_with_columns(void) {
    QueryResult* result = calloc(1, sizeof(QueryResult));
    TEST_ASSERT_NOT_NULL(result);

    result->success = true;
    result->column_count = 2;
    result->column_names = calloc(2, sizeof(char*));
    result->column_names[0] = strdup("col_a");
    result->column_names[1] = NULL; // last column name intentionally absent

    free_query_result(result);
    TEST_ASSERT_TRUE(true);
}

/**
 * Test: execute_auth_query_seam_success_path
 * Purpose: Cover the success-path assembly in execute_auth_query (lines 195-232)
 */
void test_execute_auth_query_seam_success_path(void) {
    set_success("[{\"account_id\": 7}]");

    json_t* params = json_object();
    QueryResult* result = execute_auth_query(8, "test_db", params);
    json_decref(params);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    free_query_result(result);
}

/**
 * Test: lookup_account_success
 * Purpose: Cover the success parsing branch (lines 281-312)
 */
void test_lookup_account_success(void) {
    set_success("[{\"account_id\": 42}]");

    account_info_t* account = lookup_account("alice", "test_db");
    TEST_ASSERT_NOT_NULL(account);
    TEST_ASSERT_EQUAL_INT(42, account->id);
    TEST_ASSERT_TRUE(account->enabled);
    TEST_ASSERT_TRUE(account->authorized);
    free_account_info(account);
}

/**
 * Test: lookup_account_uppercase_fallback
 * Purpose: Cover the DB2 uppercase ACCOUNT_ID fallback (lines 291-293)
 */
void test_lookup_account_uppercase_fallback(void) {
    set_success("[{\"ACCOUNT_ID\": 99}]");

    account_info_t* account = lookup_account("bob", "test_db");
    TEST_ASSERT_NOT_NULL(account);
    TEST_ASSERT_EQUAL_INT(99, account->id);
    free_account_info(account);
}

/**
 * Test: lookup_account_parse_error
 * Purpose: Cover JSON parse failure branch (lines 265-268)
 */
void test_lookup_account_parse_error(void) {
    set_success("not valid json");

    account_info_t* account = lookup_account("carol", "test_db");
    TEST_ASSERT_NULL(account);
}

/**
 * Test: lookup_account_query_failure
 * Purpose: Cover the !success / NULL result branch (lines 256-259)
 */
void test_lookup_account_query_failure(void) {
    set_failure("db down");

    account_info_t* account = lookup_account("erin", "test_db");
    TEST_ASSERT_NULL(account);
}

/**
 * Test: verify_password_and_status_success
 * Purpose: Cover the success path (lines 350-384) without a row
 */
void test_verify_password_and_status_success(void) {
    set_success("[]");

    account_info_t account = {0};
    bool verified = verify_password_and_status("secret", 1, "test_db", &account);
    TEST_ASSERT_FALSE(verified); // empty array => no row => not verified
}

/**
 * Test: verify_password_and_status_query_failure
 * Purpose: Cover the query failure branch (lines 351-354)
 */
void test_verify_password_and_status_query_failure(void) {
    set_failure("query failed");

    account_info_t account = {0};
    bool verified = verify_password_and_status("secret", 1, "test_db", &account);
    TEST_ASSERT_FALSE(verified);
}

/**
 * Test: verify_password_and_status_parse_error
 * Purpose: Cover the parse error branch (lines 359-363)
 */
void test_verify_password_and_status_parse_error(void) {
    set_success("garbage");

    account_info_t account = {0};
    bool verified = verify_password_and_status("secret", 1, "test_db", &account);
    TEST_ASSERT_FALSE(verified);
}

/**
 * Test: verify_password_and_status_username_populated
 * Purpose: Cover the row-present username population branch (lines 370-378)
 */
void test_verify_password_and_status_username_populated(void) {
    set_success("[{\"name\": \"frank\"}]");

    account_info_t account = {0};
    account.username = strdup("stale");
    bool verified = verify_password_and_status("secret", 1, "test_db", &account);
    TEST_ASSERT_TRUE(verified);
    TEST_ASSERT_EQUAL_STRING("frank", account.username);
    free(account.username);
}

/**
 * Test: verify_api_key_success_full
 * Purpose: Cover the success parsing of all fields (lines 867-912)
 */
void test_verify_api_key_success_full(void) {
    set_success("[{\"system_id\": 3, \"license_id\": 5, \"valid_until\": \"2035-01-01-00.00.00.000000\"}]");

    system_info_t sys_info = {0};
    bool ok = verify_api_key("key", "test_db", &sys_info);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_INT(3, sys_info.system_id);
    TEST_ASSERT_EQUAL_INT(5, sys_info.app_id);
    TEST_ASSERT_TRUE(sys_info.license_expiry > 0);
}

/**
 * Test: verify_api_key_missing_fields
 * Purpose: Cover the missing-field fallback branches (lines 873-874, 880-881, 901-902)
 */
void test_verify_api_key_missing_fields(void) {
    set_success("[{\"name\": \"x\"}]");

    system_info_t sys_info = {0};
    bool ok = verify_api_key("key", "test_db", &sys_info);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_INT(0, sys_info.system_id);
    TEST_ASSERT_EQUAL_INT(0, sys_info.app_id);
    TEST_ASSERT_EQUAL_INT(0, (int)sys_info.license_expiry);
}

/**
 * Test: verify_api_key_integer_valid_until
 * Purpose: Cover valid_until as integer branch (lines 899-900)
 */
void test_verify_api_key_integer_valid_until(void) {
    set_success("[{\"system_id\": 1, \"license_id\": 2, \"valid_until\": 2000000000}]");

    system_info_t sys_info = {0};
    bool ok = verify_api_key("key", "test_db", &sys_info);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_INT(2000000000, (int)sys_info.license_expiry);
}

/**
 * Test: verify_api_key_invalid_timestamp
 * Purpose: Cover malformed timestamp fallback branch (lines 896-897)
 */
void test_verify_api_key_invalid_timestamp(void) {
    set_success("[{\"system_id\": 1, \"license_id\": 2, \"valid_until\": \"not-a-ts\"}]");

    system_info_t sys_info = {0};
    bool ok = verify_api_key("key", "test_db", &sys_info);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_INT(0, (int)sys_info.license_expiry);
}

/**
 * Test: verify_api_key_null_data_json
 * Purpose: Cover the NULL data_json branch (lines 841-845)
 */
void test_verify_api_key_null_data_json(void) {
    set_success(NULL);

    system_info_t sys_info = {0};
    bool ok = verify_api_key("key", "test_db", &sys_info);
    TEST_ASSERT_FALSE(ok);
}

/**
 * Test: verify_api_key_no_row
 * Purpose: Cover the missing-row branch (lines 857-861)
 */
void test_verify_api_key_no_row(void) {
    set_success("[]");

    system_info_t sys_info = {0};
    bool ok = verify_api_key("key", "test_db", &sys_info);
    TEST_ASSERT_FALSE(ok);
}

/**
 * Test: verify_api_key_query_failure
 * Purpose: Cover the query failure branch (lines 833-837)
 */
void test_verify_api_key_query_failure(void) {
    set_failure("boom");

    system_info_t sys_info = {0};
    bool ok = verify_api_key("key", "test_db", &sys_info);
    TEST_ASSERT_FALSE(ok);
}

/**
 * Test: check_username_availability_success
 * Purpose: Cover the available branch (lines 439-458)
 */
void test_check_username_availability_success(void) {
    set_success("[{\"username_count\": 0}]");

    bool available = check_username_availability("newuser", "test_db");
    TEST_ASSERT_TRUE(available);
}

/**
 * Test: check_username_availability_uppercase_fallback
 * Purpose: Cover the DB2 uppercase USERNAME_COUNT fallback (lines 446-449)
 */
void test_check_username_availability_uppercase_fallback(void) {
    set_success("[{\"USERNAME_COUNT\": 0}]");

    bool available = check_username_availability("newuser", "test_db");
    TEST_ASSERT_TRUE(available);
}

/**
 * Test: check_username_availability_query_failure
 * Purpose: Cover the NULL result branch (lines 432-435)
 */
void test_check_username_availability_query_failure(void) {
    set_failure("boom");

    bool available = check_username_availability("newuser", "test_db");
    TEST_ASSERT_FALSE(available);
}

/**
 * Test: create_account_record_success
 * Purpose: Cover the success parsing branch (lines 511-546)
 */
void test_create_account_record_success(void) {
    set_success("[{\"account_id\": 123}]");

    int id = create_account_record("user", "user@example.com", "hash", "First Last", "test_db");
    TEST_ASSERT_EQUAL_INT(123, id);
}

/**
 * Test: create_account_record_query_failure
 * Purpose: Cover the query failure branch (lines 511-516)
 */
void test_create_account_record_query_failure(void) {
    set_failure("boom");

    int id = create_account_record("user", "user@example.com", "hash", "First Last", "test_db");
    TEST_ASSERT_EQUAL_INT(-1, id);
}

/**
 * Test: create_account_record_null_full_name
 * Purpose: Cover the empty full_name branch (lines 489-493) and 486-487
 */
void test_create_account_record_null_full_name(void) {
    set_success("[{\"account_id\": 7}]");

    int id = create_account_record("user", "user@example.com", "hash", NULL, "test_db");
    TEST_ASSERT_EQUAL_INT(7, id);
}

/**
 * Test: create_account_record_no_space_full_name
 * Purpose: Cover the no-space full_name branch (lines 486-487)
 */
void test_create_account_record_no_space_full_name(void) {
    set_success("[{\"account_id\": 8}]");

    int id = create_account_record("user", "user@example.com", "hash", "SingleName", "test_db");
    TEST_ASSERT_EQUAL_INT(8, id);
}

/**
 * Test: is_token_revoked_revoked
 * Purpose: Cover the revoked branch (no valid row, lines 672-685)
 */
void test_is_token_revoked_revoked(void) {
    set_success("[]");

    bool revoked = is_token_revoked("hash", "1.2.3.4", "test_db");
    TEST_ASSERT_TRUE(revoked);
}

/**
 * Test: is_token_revoked_active
 * Purpose: Cover the active branch via row_count > 0 (lines 672, 685)
 */
void test_is_token_revoked_active(void) {
    set_success_rows(1);

    bool revoked = is_token_revoked("hash", "1.2.3.4", "test_db");
    TEST_ASSERT_FALSE(revoked);
}

/**
 * Test: is_token_revoked_json_array
 * Purpose: Cover the data_json array fallback branch (lines 673-679)
 */
void test_is_token_revoked_json_array(void) {
    set_success("[{\"x\": 1}]");

    bool revoked = is_token_revoked("hash", "1.2.3.4", "test_db");
    TEST_ASSERT_FALSE(revoked);
}

/**
 * Test: is_token_revoked_query_null
 * Purpose: Cover the NULL result branch (lines 658-661)
 */
void test_is_token_revoked_query_null(void) {
    g_return_null = true;

    bool revoked = is_token_revoked("hash", "1.2.3.4", "test_db");
    TEST_ASSERT_TRUE(revoked);
}

/**
 * Test: is_token_revoked_query_failure
 * Purpose: Cover the !success branch (lines 663-668)
 */
void test_is_token_revoked_query_failure(void) {
    set_failure("boom");

    bool revoked = is_token_revoked("hash", "1.2.3.4", "test_db");
    TEST_ASSERT_TRUE(revoked);
}

/**
 * Test: check_failed_attempts_success
 * Purpose: Cover the success parsing branch (lines 721-736)
 */
void test_check_failed_attempts_success(void) {
    set_success("[{\"count\": 3}]");

    int count = check_failed_attempts("login", "1.2.3.4", 0, "test_db");
    TEST_ASSERT_EQUAL_INT(3, count);
}

/**
 * Test: check_failed_attempts_query_failure
 * Purpose: Cover the query failure branch (lines 714-718)
 */
void test_check_failed_attempts_query_failure(void) {
    set_failure("boom");

    int count = check_failed_attempts("login", "1.2.3.4", 0, "test_db");
    TEST_ASSERT_EQUAL_INT(0, count);
}

/**
 * Test: block_ip_address_success
 * Purpose: Cover the success branch (lines 750-771)
 */
void test_block_ip_address_success(void) {
    set_success("[{\"ok\": 1}]");

    block_ip_address("1.2.3.4", 30, "test_db");
    TEST_ASSERT_TRUE(true);
}

/**
 * Test: block_ip_address_failure
 * Purpose: Cover the failure branch (lines 765-768)
 */
void test_block_ip_address_failure(void) {
    set_failure("boom");

    block_ip_address("1.2.3.4", 30, "test_db");
    TEST_ASSERT_TRUE(true);
}

int main(void) {
    UNITY_BEGIN();

    // free_query_result column loop
    RUN_TEST(test_free_query_result_with_columns);

    // execute_auth_query success path
    RUN_TEST(test_execute_auth_query_seam_success_path);

    // lookup_account branches
    RUN_TEST(test_lookup_account_success);
    RUN_TEST(test_lookup_account_uppercase_fallback);
    RUN_TEST(test_lookup_account_parse_error);
    RUN_TEST(test_lookup_account_query_failure);

    // verify_password_and_status branches
    RUN_TEST(test_verify_password_and_status_success);
    RUN_TEST(test_verify_password_and_status_query_failure);
    RUN_TEST(test_verify_password_and_status_parse_error);
    RUN_TEST(test_verify_password_and_status_username_populated);

    // verify_api_key branches
    RUN_TEST(test_verify_api_key_success_full);
    RUN_TEST(test_verify_api_key_missing_fields);
    RUN_TEST(test_verify_api_key_integer_valid_until);
    RUN_TEST(test_verify_api_key_invalid_timestamp);
    RUN_TEST(test_verify_api_key_null_data_json);
    RUN_TEST(test_verify_api_key_no_row);
    RUN_TEST(test_verify_api_key_query_failure);

    // check_username_availability branches
    RUN_TEST(test_check_username_availability_success);
    RUN_TEST(test_check_username_availability_uppercase_fallback);
    RUN_TEST(test_check_username_availability_query_failure);

    // create_account_record branches
    RUN_TEST(test_create_account_record_success);
    RUN_TEST(test_create_account_record_query_failure);
    RUN_TEST(test_create_account_record_no_space_full_name);
    RUN_TEST(test_create_account_record_null_full_name);

    // is_token_revoked branches
    RUN_TEST(test_is_token_revoked_revoked);
    RUN_TEST(test_is_token_revoked_active);
    RUN_TEST(test_is_token_revoked_json_array);
    RUN_TEST(test_is_token_revoked_query_null);
    RUN_TEST(test_is_token_revoked_query_failure);

    // check_failed_attempts branches
    RUN_TEST(test_check_failed_attempts_success);
    RUN_TEST(test_check_failed_attempts_query_failure);

    // block_ip_address branches
    RUN_TEST(test_block_ip_address_success);
    RUN_TEST(test_block_ip_address_failure);

    return UNITY_END();
}
