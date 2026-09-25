/*
 * Unity Test File: firebird prepared statement creation
 * Tests firebird_prepare_statement() — Phase 5 skeleton (no isc_dsql call).
 *
 * Build version: 1.0.0
 * TEST_VERSION: 20260925
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebird/types.h>
#include <src/database/firebird/prepared.h>

/* USE_MOCK_SYSTEM is passed by CMake for database/firebird test sources.
 * Declare the extern control functions so we can inject calloc failures. */
extern void mock_system_reset_all(void);
extern void mock_system_set_calloc_failure(int should_fail);
extern int mock_calloc_should_fail;
extern int mock_calloc_call_count;

/* Forward declarations for functions under test */
bool firebird_prepare_statement(DatabaseHandle* connection,
                                const char* name,
                                const char* sql,
                                PreparedStatement** stmt,
                                bool add_to_cache);

/* Function prototypes */
void test_prepare_null_connection(void);
void test_prepare_wrong_engine(void);
void test_prepare_null_sql(void);
void test_prepare_null_stmt_output(void);
void test_prepare_calloc_failure(void);
void test_prepare_no_name_success(void);
void test_prepare_with_name_success(void);
void test_prepare_with_sql_template_copied(void);
void test_prepare_usage_count_initialized(void);
void test_prepare_created_at_set(void);
void test_prepare_engine_specific_handle_null(void);
void test_prepare_add_to_cache_ignored(void);

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
}

/* ---- Parameter validation tests (lines 30-31) ---- */

void test_prepare_null_connection(void) {
    PreparedStatement* stmt = NULL;
    bool result = firebird_prepare_statement(NULL, "stmt1", "SELECT 1", &stmt, true);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

void test_prepare_wrong_engine(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    PreparedStatement* stmt = NULL;
    bool result = firebird_prepare_statement(&connection, "stmt1", "SELECT 1", &stmt, true);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

void test_prepare_null_sql(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_FIREBIRD;
    PreparedStatement* stmt = NULL;
    bool result = firebird_prepare_statement(&connection, "stmt1", NULL, &stmt, true);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

void test_prepare_null_stmt_output(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_FIREBIRD;
    bool result = firebird_prepare_statement(&connection, "stmt1", "SELECT 1", NULL, true);
    TEST_ASSERT_FALSE(result);
}

/* ---- Allocation failure test (lines 36-38) ---- */

void test_prepare_calloc_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_FIREBIRD;
    PreparedStatement* stmt = NULL;

    mock_system_set_calloc_failure(1);
    bool result = firebird_prepare_statement(&connection, "stmt1", "SELECT 1", &stmt, true);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

/* ---- Success path tests (lines 41-51) ---- */

void test_prepare_no_name_success(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_FIREBIRD;
    PreparedStatement* stmt = NULL;

    bool result = firebird_prepare_statement(&connection, NULL, "SELECT 1", &stmt, true);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(stmt);
    TEST_ASSERT_NULL(stmt->name);
    TEST_ASSERT_NOT_NULL(stmt->sql_template);
    TEST_ASSERT_EQUAL_STRING("SELECT 1", stmt->sql_template);
    TEST_ASSERT_EQUAL(0, stmt->usage_count);
    TEST_ASSERT_NULL(stmt->engine_specific_handle);

    free((void*)stmt->sql_template);
    free(stmt);
}

void test_prepare_with_name_success(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_FIREBIRD;
    PreparedStatement* stmt = NULL;

    bool result = firebird_prepare_statement(&connection, "my_stmt", "SELECT * FROM t", &stmt, true);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(stmt);
    TEST_ASSERT_NOT_NULL(stmt->name);
    TEST_ASSERT_EQUAL_STRING("my_stmt", stmt->name);
    TEST_ASSERT_NOT_NULL(stmt->sql_template);
    TEST_ASSERT_EQUAL_STRING("SELECT * FROM t", stmt->sql_template);

    free((void*)stmt->name);
    free((void*)stmt->sql_template);
    free(stmt);
}

void test_prepare_with_sql_template_copied(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_FIREBIRD;
    PreparedStatement* stmt = NULL;

    const char* sql = "SELECT a, b, c FROM table WHERE id = ?";
    bool result = firebird_prepare_statement(&connection, NULL, sql, &stmt, true);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(stmt);
    TEST_ASSERT_NOT_NULL(stmt->sql_template);
    /* Verify it's a copy, not the same pointer */
    TEST_ASSERT_NOT_EQUAL((uintptr_t)sql, (uintptr_t)stmt->sql_template);
    TEST_ASSERT_EQUAL_STRING(sql, stmt->sql_template);

    free((void*)stmt->sql_template);
    free(stmt);
}

void test_prepare_usage_count_initialized(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_FIREBIRD;
    PreparedStatement* stmt = NULL;

    bool result = firebird_prepare_statement(&connection, "stmt", "SELECT 1", &stmt, true);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, stmt->usage_count);

    free((void*)stmt->name);
    free((void*)stmt->sql_template);
    free(stmt);
}

void test_prepare_created_at_set(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_FIREBIRD;
    PreparedStatement* stmt = NULL;

    bool result = firebird_prepare_statement(&connection, "stmt", "SELECT 1", &stmt, true);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_EQUAL(0, stmt->created_at);

    free((void*)stmt->name);
    free((void*)stmt->sql_template);
    free(stmt);
}

void test_prepare_engine_specific_handle_null(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_FIREBIRD;
    PreparedStatement* stmt = NULL;

    bool result = firebird_prepare_statement(&connection, "stmt", "SELECT 1", &stmt, true);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NULL(stmt->engine_specific_handle);

    free((void*)stmt->name);
    free((void*)stmt->sql_template);
    free(stmt);
}

void test_prepare_add_to_cache_ignored(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_FIREBIRD;

    /* add_to_cache=true: Phase 5 ignores this flag, still succeeds */
    PreparedStatement* stmt_true = NULL;
    bool result_true = firebird_prepare_statement(&connection, "stmt", "SELECT 1", &stmt_true, true);
    TEST_ASSERT_TRUE(result_true);
    TEST_ASSERT_NOT_NULL(stmt_true);
    free((void*)stmt_true->name);
    free((void*)stmt_true->sql_template);
    free(stmt_true);

    /* add_to_cache=false: also succeeds */
    PreparedStatement* stmt_false = NULL;
    bool result_false = firebird_prepare_statement(&connection, "stmt", "SELECT 1", &stmt_false, false);
    TEST_ASSERT_TRUE(result_false);
    TEST_ASSERT_NOT_NULL(stmt_false);
    free((void*)stmt_false->name);
    free((void*)stmt_false->sql_template);
    free(stmt_false);
}

int main(void) {
    UNITY_BEGIN();

    /* Parameter validation */
    RUN_TEST(test_prepare_null_connection);
    RUN_TEST(test_prepare_wrong_engine);
    RUN_TEST(test_prepare_null_sql);
    RUN_TEST(test_prepare_null_stmt_output);

    /* Allocation failure */
    RUN_TEST(test_prepare_calloc_failure);

    /* Success path */
    RUN_TEST(test_prepare_no_name_success);
    RUN_TEST(test_prepare_with_name_success);
    RUN_TEST(test_prepare_with_sql_template_copied);
    RUN_TEST(test_prepare_usage_count_initialized);
    RUN_TEST(test_prepare_created_at_set);
    RUN_TEST(test_prepare_engine_specific_handle_null);
    RUN_TEST(test_prepare_add_to_cache_ignored);

    return UNITY_END();
}
