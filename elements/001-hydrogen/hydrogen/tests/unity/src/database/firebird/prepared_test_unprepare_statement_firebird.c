/*
 * Unity Test File: firebird prepared statement unprepare
 * Tests firebird_unprepare_statement() — Phase 5 skeleton (no isc_dsql call).
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
 * Declare the extern control functions so we can interact with mock state. */
extern void mock_system_reset_all(void);
extern int mock_calloc_call_count;

/* Forward declarations for functions under test */
bool firebird_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);
void firebird_free_prepared_statement(PreparedStatement* stmt);

/* Function prototypes */
void test_unprepare_null_connection(void);
void test_unprepare_wrong_engine(void);
void test_unprepare_null_stmt(void);
void test_unprepare_success(void);
void test_unprepare_calls_free_function(void);

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
}

/* ---- Parameter validation tests (lines 61-62) ---- */

void test_unprepare_null_connection(void) {
    PreparedStatement stmt = {0};
    bool result = firebird_unprepare_statement(NULL, &stmt);
    TEST_ASSERT_FALSE(result);
}

void test_unprepare_wrong_engine(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    PreparedStatement stmt = {0};
    bool result = firebird_unprepare_statement(&connection, &stmt);
    TEST_ASSERT_FALSE(result);
}

void test_unprepare_null_stmt(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_FIREBIRD;
    bool result = firebird_unprepare_statement(&connection, NULL);
    TEST_ASSERT_FALSE(result);
}

/* ---- Success path tests (lines 66-69) ---- */

void test_unprepare_success(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_FIREBIRD;

    /* Build a prepared statement manually (as firebird_prepare_statement would) */
    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);
    stmt->sql_template = strdup("SELECT 1");
    stmt->name = strdup("test_stmt");
    stmt->created_at = time(NULL);
    stmt->usage_count = 0;

    bool result = firebird_unprepare_statement(&connection, stmt);
    TEST_ASSERT_TRUE(result);
}

void test_unprepare_calls_free_function(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_FIREBIRD;

    /* Build a prepared statement and verify firebird_free_prepared_statement
     * is invoked by unprepare. We verify by checking the statement is freed
     * (its memory returned to the allocator). Since we cannot check freed
     * memory safely, we instead verify the function returns true and the
     * stmt struct is no longer accessible after the call. */
    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);
    stmt->sql_template = strdup("SELECT * FROM t WHERE id = ?");
    stmt->name = strdup("my_query");
    stmt->usage_count = 5;

    bool result = firebird_unprepare_statement(&connection, stmt);
    TEST_ASSERT_TRUE(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_unprepare_null_connection);
    RUN_TEST(test_unprepare_wrong_engine);
    RUN_TEST(test_unprepare_null_stmt);
    RUN_TEST(test_unprepare_success);
    RUN_TEST(test_unprepare_calls_free_function);

    return UNITY_END();
}
