/*
 * Unity Test File: execute_single_migration error paths
 * This file tests error conditions in execute_single_migration function
 * to improve coverage beyond 50%
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/migration/migration.h>
#include <src/payload/payload.h>
#include <lua.h>

// Forward declarations for functions being tested
bool execute_single_migration(DatabaseHandle* connection, const char* migration_file,
                             const char* engine_name, const char* migration_name,
                             const char* schema_name, const char* dqm_label);

// Forward declarations for test functions
void test_execute_single_migration_lua_setup_failure(void);
void test_execute_single_migration_with_null_schema(void);
void test_execute_single_migration_with_empty_strings(void);
void test_execute_single_migration_db_engines(void);

// Test setup and teardown
void setUp(void) {
    // Reset any state
}

void tearDown(void) {
    // Clean up
}

// ===== ERROR PATH TESTS =====

/*
 * Test lua_setup failure path (line 35)
 * This tests the early failure when Lua initialization fails
 */
void test_execute_single_migration_lua_setup_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    bool result = execute_single_migration(
        &connection,
        "test.lua",
        "sqlite",
        "nonexistent_migration_that_will_fail",
        NULL,
        "test-label"
    );
    
    //Will fail when trying to get payload files (line 43-46)
    TEST_ASSERT_FALSE(result);
}

/*
 * Test with NULL schema_name to ensure proper handling
 */
void test_execute_single_migration_with_null_schema(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    
    bool result = execute_single_migration(
        &connection,
        "001_test.lua",
        "postgresql",
        "test_migration",
        NULL,  // NULL schema_name
        "test-label"
    );
    
    TEST_ASSERT_FALSE(result);
}

/*
 * Test with various empty string combinations
 */
void test_execute_single_migration_with_empty_strings(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL;
    
    // Test with empty migration_file
    bool result1 = execute_single_migration(
        &connection,
        "",  // empty
        "mysql",
        "test",
        NULL,
        "test-label"
    );
    TEST_ASSERT_FALSE(result1);
    
    // Test with empty engine_name
    bool result2 = execute_single_migration(
        &connection,
        "test.lua",
        "",  // empty
        "test",
        NULL,
        "test-label"
    );
    TEST_ASSERT_FALSE(result2);
    
    // Test with empty migration_name  
    bool result3 = execute_single_migration(
        &connection,
        "test.lua",
        "mysql",
        "",  // empty
        NULL,
        "test-label"
    );
    TEST_ASSERT_FALSE(result3);
}

/*
 * Test all database engines to ensure all paths are covered
 */
void test_execute_single_migration_db_engines(void) {
    DatabaseHandle conn_sqlite = {0};
    conn_sqlite.engine_type = DB_ENGINE_SQLITE;
    bool r1 = execute_single_migration(&conn_sqlite, "t.lua", "sqlite", "test", NULL, "label");
    TEST_ASSERT_FALSE(r1);
    
    DatabaseHandle conn_pg = {0};
    conn_pg.engine_type = DB_ENGINE_POSTGRESQL;
    bool r2 = execute_single_migration(&conn_pg, "t.lua", "postgresql", "test", "public", "label");
    TEST_ASSERT_FALSE(r2);
    
    DatabaseHandle conn_mysql = {0};
    conn_mysql.engine_type = DB_ENGINE_MYSQL;
    bool r3 = execute_single_migration(&conn_mysql, "t.lua", "mysql", "test", NULL, "label");
    TEST_ASSERT_FALSE(r3);
    
    DatabaseHandle conn_db2 = {0};
    conn_db2.engine_type = DB_ENGINE_DB2;
    bool r4 = execute_single_migration(&conn_db2, "t.lua", "db2", "test", NULL, "label");
    TEST_ASSERT_FALSE(r4);
}

int main(void) {
    UNITY_BEGIN();
    
    // Error path tests
    RUN_TEST(test_execute_single_migration_lua_setup_failure);
    RUN_TEST(test_execute_single_migration_with_null_schema);
    RUN_TEST(test_execute_single_migration_with_empty_strings);
    RUN_TEST(test_execute_single_migration_db_engines);
    
    return UNITY_END();
}