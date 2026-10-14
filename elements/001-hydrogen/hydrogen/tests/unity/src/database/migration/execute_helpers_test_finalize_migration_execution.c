/*
 * Unity Test File: execute_helpers_test_finalize_migration_execution
 * Unit tests for finalize_migration_execution() and execute_copied_sql_and_cleanup()
 * in execute_helpers.c
 */

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#include <lua.h>
#include <lauxlib.h>

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/migration/migration.h>
#include <src/payload/payload.h>

// Forward declarations for functions being tested
bool execute_copied_sql_and_cleanup(DatabaseHandle* connection, const char* migration_file,
                                    char* sql_copy, size_t sql_length,
                                    const char* dqm_label);
bool finalize_migration_execution(DatabaseHandle* connection, const char* migration_file,
                                  const char* sql_result, size_t sql_length, int query_count,
                                  lua_State* L, PayloadFile* payload_files, size_t payload_count,
                                  const char* dqm_label);

// Forward declarations for test functions
void test_execute_copied_sql_and_cleanup_valid_sql(void);
void test_execute_copied_sql_and_cleanup_null_sql(void);
void test_execute_copied_sql_and_cleanup_zero_length(void);
void test_finalize_migration_execution_success(void);
void test_finalize_migration_execution_null_sql_result(void);
void test_finalize_migration_execution_zero_length(void);
void test_finalize_migration_execution_copy_failure(void);

/* A zeroed SQLite handle is enough: the transaction path is mocked. */
static DatabaseHandle make_connection(void) {
    DatabaseHandle connection;
    memset(&connection, 0, sizeof(connection));
    connection.engine_type = DB_ENGINE_SQLITE;
    return connection;
}

/* A one entry payload file list so free_payload_files has real work to do. */
static PayloadFile* make_payload_files(size_t* count_out) {
    PayloadFile* files = calloc(1, sizeof(PayloadFile));
    TEST_ASSERT_NOT_NULL(files);
    files[0].name = strdup("acuranzo/acuranzo_1001.lua");
    files[0].data = malloc(16);
    TEST_ASSERT_NOT_NULL(files[0].data);
    memcpy(files[0].data, "CREATE TABLE t;", 16);
    files[0].size = 16;
    files[0].is_compressed = false;
    *count_out = 1;
    return files;
}

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
}

// ===== execute_copied_sql_and_cleanup TESTS =====

void test_execute_copied_sql_and_cleanup_valid_sql(void) {
    DatabaseHandle connection = make_connection();
    const char* sql = "CREATE TABLE test (id INTEGER);";
    char* sql_copy = strdup(sql);
    TEST_ASSERT_NOT_NULL(sql_copy);

    bool result = execute_copied_sql_and_cleanup(&connection, "test.lua", sql_copy,
                                                 strlen(sql), "test-label");

    TEST_ASSERT_TRUE(result);
}

void test_execute_copied_sql_and_cleanup_null_sql(void) {
    DatabaseHandle connection = make_connection();

    /* free(NULL) is a no-op, so this only exercises the guard. */
    bool result = execute_copied_sql_and_cleanup(&connection, "test.lua", NULL, 16, "test-label");

    TEST_ASSERT_FALSE(result);
}

void test_execute_copied_sql_and_cleanup_zero_length(void) {
    DatabaseHandle connection = make_connection();
    char* sql_copy = strdup("CREATE TABLE test (id INTEGER);");
    TEST_ASSERT_NOT_NULL(sql_copy);

    bool result = execute_copied_sql_and_cleanup(&connection, "test.lua", sql_copy, 0, "test-label");

    TEST_ASSERT_FALSE(result);
}

// ===== finalize_migration_execution TESTS =====

void test_finalize_migration_execution_success(void) {
    DatabaseHandle connection = make_connection();
    size_t payload_count = 0;
    PayloadFile* payload_files = make_payload_files(&payload_count);
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    const char* sql = "CREATE TABLE test (id INTEGER);";

    bool result = finalize_migration_execution(&connection, "acuranzo/acuranzo_1001.lua", sql,
                                               strlen(sql), 3, L, payload_files, payload_count,
                                               "test-label");

    TEST_ASSERT_TRUE(result);
}

void test_finalize_migration_execution_null_sql_result(void) {
    DatabaseHandle connection = make_connection();
    size_t payload_count = 0;
    PayloadFile* payload_files = make_payload_files(&payload_count);
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);

    /* A NULL SQL result skips the copy-failure branch and fails at execution. */
    bool result = finalize_migration_execution(&connection, "acuranzo/acuranzo_1001.lua", NULL,
                                               16, 0, L, payload_files, payload_count,
                                               "test-label");

    TEST_ASSERT_FALSE(result);
}

void test_finalize_migration_execution_zero_length(void) {
    DatabaseHandle connection = make_connection();
    size_t payload_count = 0;
    PayloadFile* payload_files = make_payload_files(&payload_count);
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    const char* sql = "CREATE TABLE test (id INTEGER);";

    /* A zero length also skips the copy-failure branch. */
    bool result = finalize_migration_execution(&connection, "acuranzo/acuranzo_1001.lua", sql,
                                               0, 0, L, payload_files, payload_count,
                                               "test-label");

    TEST_ASSERT_FALSE(result);
}

void test_finalize_migration_execution_copy_failure(void) {
    DatabaseHandle connection = make_connection();
    size_t payload_count = 0;
    PayloadFile* payload_files = make_payload_files(&payload_count);
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    const char* sql = "CREATE TABLE test (id INTEGER);";

    /* The SQL copy is the first allocation the function makes. */
    mock_system_set_malloc_failure(1);

    bool result = finalize_migration_execution(&connection, "acuranzo/acuranzo_1001.lua", sql,
                                               strlen(sql), 3, L, payload_files, payload_count,
                                               "test-label");

    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_execute_copied_sql_and_cleanup_valid_sql);
    RUN_TEST(test_execute_copied_sql_and_cleanup_null_sql);
    RUN_TEST(test_execute_copied_sql_and_cleanup_zero_length);

    RUN_TEST(test_finalize_migration_execution_success);
    RUN_TEST(test_finalize_migration_execution_null_sql_result);
    RUN_TEST(test_finalize_migration_execution_zero_length);
    RUN_TEST(test_finalize_migration_execution_copy_failure);

    return UNITY_END();
}
