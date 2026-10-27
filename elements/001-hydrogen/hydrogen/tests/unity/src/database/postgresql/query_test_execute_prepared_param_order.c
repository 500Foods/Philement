/*
 * Unity Test File: postgresql_execute_prepared bind order
 *
 * Tests postgresql_execute_prepared() in src/database/postgresql/query.c.
 *
 * Transaction blocks force prepared statements. PREPARE uses the $N SQL
 * built by convert_named_to_positional (SQL appearance order of :NAME).
 * The execute must bind values in that same order, not in the JSON type
 * group order of parse_typed_parameters (INTEGER, STRING, BOOLEAN, ...).
 *
 * Transaction contract under test: one parameter JSON is shared by every
 * statement in the block. A name has one value for the whole block; each
 * statement may use none, some, or all of the names, in any order.
 *
 * Regression for query 4514 (Working Title): :WORKING_TITLE appeared before
 * :USERID / :SUGGESTION_ID, so the title was bound into an integer cast.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/postgresql/connection.h>
#include <src/database/postgresql/query.h>

// Library pointers from src/database/postgresql/connection.c
typedef void* (*TestPQexec_t)(void* conn, const char* query);
typedef void* (*TestPQexecPrepared_t)(void* conn, const char* stmtName, int nParams,
                                      const char* const* paramValues, const int* paramLengths,
                                      const int* paramFormats, int resultFormat);
extern TestPQexec_t PQexec_ptr;
extern TestPQexecPrepared_t PQexecPrepared_ptr;

// Capture helpers
void* capture_PQexec(void* conn, const char* query);
void* capture_PQexecPrepared(void* conn, const char* stmtName, int nParams,
                             const char* const* paramValues, const int* paramLengths,
                             const int* paramFormats, int resultFormat);
void run_prepared(const char* sql, const char* json);

// Test prototypes
void test_execute_prepared_string_before_integers_4514(void);
void test_execute_prepared_ledger_insert_integers_first(void);
void test_execute_prepared_integers_reverse_of_json_order(void);
void test_execute_prepared_subset_of_shared_params(void);
void test_execute_prepared_no_params_used(void);
void test_execute_prepared_repeated_name_binds_once(void);
void test_execute_prepared_all_types_interleaved(void);
void test_execute_prepared_transaction_block_shared_json(void);

#define CAP_MAX 16
#define CAP_LEN 128
static char cap_values[CAP_MAX][CAP_LEN];
static int cap_count = -1;
static TestPQexec_t saved_exec = NULL;
static TestPQexecPrepared_t saved_exec_prepared = NULL;

// Lithium-style buckets: INTEGER group, then STRING group
static const char* LITHIUM_JSON =
    "{\"INTEGER\":{\"SUGGESTION_ID\":42,\"USERID\":7},"
    "\"STRING\":{\"WORKING_TITLE\":\"All about mDNS\"}}";

static const char* ALL_TYPES_JSON =
    "{\"INTEGER\":{\"A_INT\":1,\"B_INT\":2},"
    "\"STRING\":{\"C_STR\":\"three\"},"
    "\"BOOLEAN\":{\"D_BOOL\":true},"
    "\"FLOAT\":{\"E_FLT\":5.5}}";

void* capture_PQexec(void* conn, const char* query) {
    (void)conn;
    (void)query;
    return NULL;  // statement_timeout set "fails" quietly; not under test
}

void* capture_PQexecPrepared(void* conn, const char* stmtName, int nParams,
                             const char* const* paramValues, const int* paramLengths,
                             const int* paramFormats, int resultFormat) {
    (void)conn; (void)stmtName; (void)paramLengths; (void)paramFormats; (void)resultFormat;
    cap_count = nParams;
    for (int i = 0; i < nParams && i < CAP_MAX; i++) {
        snprintf(cap_values[i], CAP_LEN, "%s", (paramValues && paramValues[i]) ? paramValues[i] : "(null)");
    }
    return NULL;  // execute_prepared then reports failure; we only check binds
}

void run_prepared(const char* sql, const char* json) {
    PostgresConnection pg = {0};
    pg.connection = (void*)0x1;
    DatabaseHandle conn = {0};
    conn.engine_type = DB_ENGINE_POSTGRESQL;
    conn.connection_handle = &pg;
    PreparedStatement stmt = {0};
    stmt.name = (char*)"test_stmt";
    QueryRequest req = {0};
    req.sql_template = (char*)sql;
    req.parameters_json = (char*)json;
    req.timeout_seconds = 30;
    QueryResult* result = NULL;

    cap_count = -1;
    memset(cap_values, 0, sizeof(cap_values));
    (void)postgresql_execute_prepared(&conn, &stmt, &req, &result);
    if (result) {
        free(result->error_message);
        free(result->data_json);
        free(result);
    }
}

void setUp(void) {
    saved_exec = PQexec_ptr;
    saved_exec_prepared = PQexecPrepared_ptr;
    PQexec_ptr = capture_PQexec;
    PQexecPrepared_ptr = capture_PQexecPrepared;
}

void tearDown(void) {
    PQexec_ptr = saved_exec;
    PQexecPrepared_ptr = saved_exec_prepared;
}

void test_execute_prepared_string_before_integers_4514(void) {
    run_prepared("UPDATE s SET working_title = :WORKING_TITLE, updated_id = CAST(:USERID AS integer) "
                 "WHERE suggestion_id = CAST(:SUGGESTION_ID AS integer)", LITHIUM_JSON);
    TEST_ASSERT_EQUAL_INT(3, cap_count);
    TEST_ASSERT_EQUAL_STRING("All about mDNS", cap_values[0]);
    TEST_ASSERT_EQUAL_STRING("7", cap_values[1]);
    TEST_ASSERT_EQUAL_STRING("42", cap_values[2]);
}

void test_execute_prepared_ledger_insert_integers_first(void) {
    run_prepared("INSERT INTO l (suggestion_id, created_id, note) VALUES (:SUGGESTION_ID, :USERID, :WORKING_TITLE)",
                 LITHIUM_JSON);
    TEST_ASSERT_EQUAL_INT(3, cap_count);
    TEST_ASSERT_EQUAL_STRING("42", cap_values[0]);
    TEST_ASSERT_EQUAL_STRING("7", cap_values[1]);
    TEST_ASSERT_EQUAL_STRING("All about mDNS", cap_values[2]);
}

void test_execute_prepared_integers_reverse_of_json_order(void) {
    // Same type, SQL order differs from JSON order: silent swap before the fix
    run_prepared("UPDATE s SET updated_id = :USERID WHERE suggestion_id = :SUGGESTION_ID", LITHIUM_JSON);
    TEST_ASSERT_EQUAL_INT(2, cap_count);
    TEST_ASSERT_EQUAL_STRING("7", cap_values[0]);
    TEST_ASSERT_EQUAL_STRING("42", cap_values[1]);
}

void test_execute_prepared_subset_of_shared_params(void) {
    run_prepared("SELECT 1 FROM s WHERE title = :WORKING_TITLE", LITHIUM_JSON);
    TEST_ASSERT_EQUAL_INT(1, cap_count);
    TEST_ASSERT_EQUAL_STRING("All about mDNS", cap_values[0]);
}

void test_execute_prepared_no_params_used(void) {
    run_prepared("SELECT 1", LITHIUM_JSON);
    TEST_ASSERT_EQUAL_INT(0, cap_count);
}

void test_execute_prepared_repeated_name_binds_once(void) {
    // PostgreSQL reuses $N for a repeated name, so it binds once
    run_prepared("UPDATE s SET a = :WORKING_TITLE, b = :USERID WHERE c = :WORKING_TITLE AND d = :USERID",
                 LITHIUM_JSON);
    TEST_ASSERT_EQUAL_INT(2, cap_count);
    TEST_ASSERT_EQUAL_STRING("All about mDNS", cap_values[0]);
    TEST_ASSERT_EQUAL_STRING("7", cap_values[1]);
}

void test_execute_prepared_all_types_interleaved(void) {
    run_prepared("SELECT :E_FLT, :C_STR, :B_INT, :D_BOOL, :A_INT", ALL_TYPES_JSON);
    TEST_ASSERT_EQUAL_INT(5, cap_count);
    TEST_ASSERT_EQUAL_FLOAT(5.5f, (float)atof(cap_values[0]));
    TEST_ASSERT_EQUAL_STRING("three", cap_values[1]);
    TEST_ASSERT_EQUAL_STRING("2", cap_values[2]);
    TEST_ASSERT_EQUAL_STRING("true", cap_values[3]);
    TEST_ASSERT_EQUAL_STRING("1", cap_values[4]);
}

void test_execute_prepared_transaction_block_shared_json(void) {
    // One block, one JSON, statements using all / some / none / reordered names
    const char* stmts[] = {
        "UPDATE s SET t = :WORKING_TITLE, u = :USERID WHERE id = :SUGGESTION_ID",
        "INSERT INTO l VALUES (:SUGGESTION_ID, :USERID, 'working_title')",
        "SELECT now()",
        "SELECT :WORKING_TITLE",
        "DELETE FROM x WHERE u = :USERID"
    };
    const int counts[] = {3, 2, 0, 1, 1};
    const char* firsts[] = {"All about mDNS", "42", NULL, "All about mDNS", "7"};
    for (size_t i = 0; i < sizeof(stmts) / sizeof(stmts[0]); i++) {
        run_prepared(stmts[i], LITHIUM_JSON);
        TEST_ASSERT_EQUAL_INT_MESSAGE(counts[i], cap_count, stmts[i]);
        if (firsts[i]) {
            TEST_ASSERT_EQUAL_STRING_MESSAGE(firsts[i], cap_values[0], stmts[i]);
        }
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_execute_prepared_string_before_integers_4514);
    RUN_TEST(test_execute_prepared_ledger_insert_integers_first);
    RUN_TEST(test_execute_prepared_integers_reverse_of_json_order);
    RUN_TEST(test_execute_prepared_subset_of_shared_params);
    RUN_TEST(test_execute_prepared_no_params_used);
    RUN_TEST(test_execute_prepared_repeated_name_binds_once);
    RUN_TEST(test_execute_prepared_all_types_interleaved);
    RUN_TEST(test_execute_prepared_transaction_block_shared_json);

    return UNITY_END();
}
