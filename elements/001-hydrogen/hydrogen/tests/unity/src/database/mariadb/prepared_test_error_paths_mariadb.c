// This is a new file, so I'll create the entire content:

/*
 * Unity Test File: prepared_test_error_paths_mysql
 * Tests for MySQL prepared statement error paths and edge cases
 * 
 * This file focuses on covering error conditions and edge cases
 * that are not covered by other tests, using mock_libmariadb.
 */

#include <src/hydrogen.h>
#include <unity.h>

// Mock libmariadb
#include <unity/mocks/mock_libmariadb.h>

// Include MySQL types and prepared statement functions
#include <src/database/mariadb/types.h>
#include <src/database/mariadb/prepared.h>
#include <src/database/database.h>

// Forward declarations
bool mariadb_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt, bool add_to_cache);
bool mariadb_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);
void mariadb_update_prepared_lru_counter(DatabaseHandle* connection, const char* stmt_name);
bool mariadb_add_prepared_statement(PreparedStatementCache* cache, const char* name);
bool mariadb_remove_prepared_statement(PreparedStatementCache* cache, const char* name);
bool mariadb_initialize_prepared_statement_cache(DatabaseHandle* connection, size_t cache_size);
bool mariadb_evict_lru_prepared_statement(DatabaseHandle* connection, const MariadbConnection* mariadb_conn, const char* new_stmt_name);
bool mariadb_add_prepared_statement_to_cache(DatabaseHandle* connection, PreparedStatement* stmt, size_t cache_size);
bool mariadb_validate_prepared_statement_functions(void);
void* mariadb_create_statement_handle(void* mariadb_connection);
bool mariadb_prepare_statement_handle(void* stmt_handle, const char* sql);
size_t mariadb_find_lru_statement_index(DatabaseHandle* connection);
void mariadb_evict_lru_statement(DatabaseHandle* connection, size_t lru_index);
bool mariadb_add_statement_to_cache(DatabaseHandle* connection, PreparedStatement* stmt, size_t cache_size);
bool mariadb_remove_statement_from_cache(DatabaseHandle* connection, const PreparedStatement* stmt);
void mariadb_cleanup_prepared_statement(PreparedStatement* stmt);
bool load_libmariadb_functions(const char* designator);

// External function pointers
extern mariadb_stmt_init_t mariadb_stmt_init_ptr;
extern mariadb_stmt_prepare_t mariadb_stmt_prepare_ptr;
extern mariadb_stmt_close_t mariadb_stmt_close_ptr;
extern mariadb_error_t mariadb_error_ptr;

// Test function prototypes
void test_add_to_cache_eviction_failure(void);
void test_prepare_statement_null_parameters(void);
void test_prepare_statement_add_to_cache_failure(void);
void test_update_lru_counter_null_parameters(void);
void test_update_lru_counter_updates_correctly(void);
void test_validate_prepared_statement_functions(void);
void test_create_statement_handle(void);
void test_prepare_statement_handle(void);
void test_find_lru_statement_index(void);
void test_evict_lru_statement(void);
void test_add_statement_to_cache(void);
void test_remove_statement_from_cache(void);
void test_cleanup_prepared_statement(void);
void test_add_prepared_statement_stub(void);
void test_remove_prepared_statement_stub(void);

void setUp(void) {
    // Reset all mocks before each test
    mock_libmariadb_reset_all();
    
    // Load MySQL function pointers (with mocks, this just ensures they're initialized)
    load_libmariadb_functions(NULL);
}

void tearDown(void) {
    // Cleanup after each test
    mock_libmariadb_reset_all();
}

// Test: Add to cache - eviction failure (line 93)
void test_add_to_cache_eviction_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MARIADB;
    connection.prepared_statement_count = 1;
    
    // Set up cache arrays
    connection.prepared_statements = calloc(1, sizeof(PreparedStatement*));
    connection.prepared_statement_lru_counter = calloc(1, sizeof(uint64_t));
    TEST_ASSERT_NOT_NULL(connection.prepared_statements);
    TEST_ASSERT_NOT_NULL(connection.prepared_statement_lru_counter);
    
    // Create existing statement
    PreparedStatement* existing_stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(existing_stmt);
    existing_stmt->name = strdup("existing");
    existing_stmt->sql_template = strdup("SELECT 1");
    existing_stmt->engine_specific_handle = (void*)0x1234;
    connection.prepared_statements[0] = existing_stmt;
    connection.prepared_statement_lru_counter[0] = 1;
    
    MariadbConnection mariadb_conn = {0};
    mariadb_conn.connection = (void*)0x5678;
    connection.connection_handle = &mariadb_conn;
    
    // Create new statement to add
    PreparedStatement* new_stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(new_stmt);
    new_stmt->name = strdup("new_stmt");
    new_stmt->sql_template = strdup("SELECT 2");
    
    // Clear mariadb_stmt_close_ptr to make eviction fail
    mariadb_stmt_close_t saved_close_ptr = mariadb_stmt_close_ptr;
    mariadb_stmt_close_ptr = NULL;
    
    bool result = mariadb_add_prepared_statement_to_cache(&connection, new_stmt, 1);
    
    TEST_ASSERT_FALSE(result);
    
    // Cleanup
    mariadb_stmt_close_ptr = saved_close_ptr;
    free(new_stmt->name);
    free(new_stmt->sql_template);
    free(new_stmt);
    free(existing_stmt->name);
    free(existing_stmt->sql_template);
    free(existing_stmt);
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
}

// Test: Prepare statement - NULL parameters (line 112)
void test_prepare_statement_null_parameters(void) {
    PreparedStatement* stmt = NULL;
    
    // Test NULL connection
    bool result = mariadb_prepare_statement(NULL, "test", "SELECT 1", &stmt, true);
    TEST_ASSERT_FALSE(result);
    
    // Test NULL name
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MARIADB;
    result = mariadb_prepare_statement(&connection, NULL, "SELECT 1", &stmt, true);
    TEST_ASSERT_FALSE(result);
    
    // Test NULL sql
    result = mariadb_prepare_statement(&connection, "test", NULL, &stmt, true);
    TEST_ASSERT_FALSE(result);
    
    // Test NULL stmt pointer
    result = mariadb_prepare_statement(&connection, "test", "SELECT 1", NULL, true);
    TEST_ASSERT_FALSE(result);
    
    // Test wrong engine type
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    result = mariadb_prepare_statement(&connection, "test", "SELECT 1", &stmt, true);
    TEST_ASSERT_FALSE(result);
}

// Test: Prepare statement - add to cache failure (lines 172-176)
void test_prepare_statement_add_to_cache_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MARIADB;
    connection.config = calloc(1, sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(connection.config);
    connection.config->prepared_statement_cache_size = 1;
    
    MariadbConnection mariadb_conn = {0};
    mariadb_conn.connection = (void*)0x1234;
    connection.connection_handle = &mariadb_conn;
    
    // Create first statement to fill cache
    mock_libmariadb_set_mysql_stmt_init_result((void*)0x1111);
    mock_libmariadb_set_mysql_stmt_prepare_result(0);
    
    PreparedStatement* stmt1 = NULL;
    TEST_ASSERT_TRUE(mariadb_prepare_statement(&connection, "stmt_1", "SELECT 1", &stmt1, true));
    TEST_ASSERT_NOT_NULL(stmt1);
    TEST_ASSERT_EQUAL(1, connection.prepared_statement_count);
    
    // Try to add another - will need to evict, but make close unavailable
    mariadb_stmt_close_t saved_close_ptr = mariadb_stmt_close_ptr;
    mariadb_stmt_close_ptr = NULL;
    
    mock_libmariadb_set_mysql_stmt_init_result((void*)0x2222);
    PreparedStatement* stmt2 = NULL;
    bool result = mariadb_prepare_statement(&connection, "stmt_2", "SELECT 2", &stmt2, true);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt2);
    
    // Cleanup
    mariadb_stmt_close_ptr = saved_close_ptr;
    free(stmt1->name);
    free(stmt1->sql_template);
    free(stmt1);
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
    free(connection.config);
}

// Test: Update LRU counter - NULL parameters (line 227)
void test_update_lru_counter_null_parameters(void) {
    // NULL connection
    mariadb_update_prepared_lru_counter(NULL, "test_stmt");
    TEST_ASSERT_TRUE(true); // Should not crash
    
    // NULL stmt_name
    DatabaseHandle connection = {0};
    mariadb_update_prepared_lru_counter(&connection, NULL);
    TEST_ASSERT_TRUE(true); // Should not crash
}

// Test: Update LRU counter - updates correctly (lines 231-236)
void test_update_lru_counter_updates_correctly(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MARIADB;
    connection.prepared_statement_count = 2;
    
    connection.prepared_statements = calloc(2, sizeof(PreparedStatement*));
    connection.prepared_statement_lru_counter = calloc(2, sizeof(uint64_t));
    TEST_ASSERT_NOT_NULL(connection.prepared_statements);
    TEST_ASSERT_NOT_NULL(connection.prepared_statement_lru_counter);
    
    PreparedStatement* stmt1 = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt1);
    stmt1->name = strdup("stmt_1");
    stmt1->usage_count = 0;
    connection.prepared_statements[0] = stmt1;
    connection.prepared_statement_lru_counter[0] = 100;
    
    PreparedStatement* stmt2 = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt2);
    stmt2->name = strdup("stmt_2");
    stmt2->usage_count = 0;
    connection.prepared_statements[1] = stmt2;
    connection.prepared_statement_lru_counter[1] = 200;
    
    uint64_t initial_counter = connection.prepared_statement_lru_counter[0];
    
    mariadb_update_prepared_lru_counter(&connection, "stmt_1");
    
    TEST_ASSERT_NOT_EQUAL(initial_counter, connection.prepared_statement_lru_counter[0]);
    TEST_ASSERT_EQUAL(1, stmt1->usage_count);
    TEST_ASSERT_EQUAL(0, stmt2->usage_count);
    
    free(stmt1->name);
    free(stmt1);
    free(stmt2->name);
    free(stmt2);
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
}

// Test: Validate prepared statement functions (lines 243-244)
void test_validate_prepared_statement_functions(void) {
    // With all pointers set
    bool result = mariadb_validate_prepared_statement_functions();
    TEST_ASSERT_TRUE(result);
    
    // With one pointer NULL
    mariadb_stmt_init_t saved_ptr = mariadb_stmt_init_ptr;
    mariadb_stmt_init_ptr = NULL;
    result = mariadb_validate_prepared_statement_functions();
    TEST_ASSERT_FALSE(result);
    
    // Reset for other tests
    mariadb_stmt_init_ptr = saved_ptr;
}

// Test: Create statement handle (lines 247-249)
void test_create_statement_handle(void) {
    void* mariadb_conn = (void*)0x1234;
    
    // With valid pointer
    mock_libmariadb_set_mysql_stmt_init_result((void*)0x5678);
    void* stmt = mariadb_create_statement_handle(mariadb_conn);
    TEST_ASSERT_NOT_NULL(stmt);
    
    // With NULL connection
    stmt = mariadb_create_statement_handle(NULL);
    TEST_ASSERT_NULL(stmt);
    
    // With NULL function pointer
    mariadb_stmt_init_t saved_ptr = mariadb_stmt_init_ptr;
    mariadb_stmt_init_ptr = NULL;
    stmt = mariadb_create_statement_handle(mariadb_conn);
    TEST_ASSERT_NULL(stmt);
    
    // Reset
    mariadb_stmt_init_ptr = saved_ptr;
}

// Test: Prepare statement handle (lines 252-254)
void test_prepare_statement_handle(void) {
    void* stmt_handle = (void*)0x1234;
    const char* sql = "SELECT 1";
    
    // With valid parameters - success
    mock_libmariadb_set_mysql_stmt_prepare_result(0);
    bool result = mariadb_prepare_statement_handle(stmt_handle, sql);
    TEST_ASSERT_TRUE(result);
    
    // With valid parameters - failure
    mock_libmariadb_set_mysql_stmt_prepare_result(1);
    result = mariadb_prepare_statement_handle(stmt_handle, sql);
    TEST_ASSERT_FALSE(result);
    
    // With NULL stmt_handle
    result = mariadb_prepare_statement_handle(NULL, sql);
    TEST_ASSERT_FALSE(result);
    
    // With NULL sql
    result = mariadb_prepare_statement_handle(stmt_handle, NULL);
    TEST_ASSERT_FALSE(result);
    
    // With NULL function pointer
    mariadb_stmt_prepare_t saved_ptr = mariadb_stmt_prepare_ptr;
    mariadb_stmt_prepare_ptr = NULL;
    result = mariadb_prepare_statement_handle(stmt_handle, sql);
    TEST_ASSERT_FALSE(result);
    
    // Reset
    mariadb_stmt_prepare_ptr = saved_ptr;
}

// Test: Find LRU statement index (lines 257-270)
void test_find_lru_statement_index(void) {
    // NULL connection
    size_t index = mariadb_find_lru_statement_index(NULL);
    TEST_ASSERT_EQUAL(0, index);
    
    // With statements
    DatabaseHandle connection = {0};
    connection.prepared_statement_count = 3;
    connection.prepared_statement_lru_counter = calloc(3, sizeof(uint64_t));
    TEST_ASSERT_NOT_NULL(connection.prepared_statement_lru_counter);
    
    connection.prepared_statement_lru_counter[0] = 100;
    connection.prepared_statement_lru_counter[1] = 50;  // Lowest
    connection.prepared_statement_lru_counter[2] = 200;
    
    index = mariadb_find_lru_statement_index(&connection);
    TEST_ASSERT_EQUAL(1, index);
    
    free(connection.prepared_statement_lru_counter);
}

// Test: Evict LRU statement (lines 273-289)
void test_evict_lru_statement(void) {
    DatabaseHandle connection = {0};
    connection.prepared_statement_count = 2;
    
    connection.prepared_statements = calloc(2, sizeof(PreparedStatement*));
    connection.prepared_statement_lru_counter = calloc(2, sizeof(uint64_t));
    TEST_ASSERT_NOT_NULL(connection.prepared_statements);
    TEST_ASSERT_NOT_NULL(connection.prepared_statement_lru_counter);
    
    PreparedStatement* stmt1 = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt1);
    stmt1->name = strdup("stmt_1");
    stmt1->sql_template = strdup("SELECT 1");
    stmt1->engine_specific_handle = (void*)0x1111;
    connection.prepared_statements[0] = stmt1;
    connection.prepared_statement_lru_counter[0] = 100;
    
    PreparedStatement* stmt2 = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt2);
    stmt2->name = strdup("stmt_2");
    stmt2->sql_template = strdup("SELECT 2");
    stmt2->engine_specific_handle = (void*)0x2222;
    connection.prepared_statements[1] = stmt2;
    connection.prepared_statement_lru_counter[1] = 200;
    
    // Evict first statement (index 0)
    mariadb_evict_lru_statement(&connection, 0);
    
    TEST_ASSERT_EQUAL(1, connection.prepared_statement_count);
    TEST_ASSERT_EQUAL(stmt2, connection.prepared_statements[0]);
    TEST_ASSERT_EQUAL(200, connection.prepared_statement_lru_counter[0]);
    
    // Cleanup
    free(stmt2->name);
    free(stmt2->sql_template);
    free(stmt2);
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
}

// Test: Add statement to cache wrapper (lines 292-293)
void test_add_statement_to_cache(void) {
    DatabaseHandle connection = {0};
    connection.prepared_statement_count = 0;
    connection.prepared_statements = calloc(10, sizeof(PreparedStatement*));
    connection.prepared_statement_lru_counter = calloc(10, sizeof(uint64_t));
    TEST_ASSERT_NOT_NULL(connection.prepared_statements);
    TEST_ASSERT_NOT_NULL(connection.prepared_statement_lru_counter);
    
    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);
    stmt->name = strdup("test_stmt");
    stmt->sql_template = strdup("SELECT 1");
    
    bool result = mariadb_add_statement_to_cache(&connection, stmt, 10);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, connection.prepared_statement_count);
    
    // Cleanup
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
}

// Test: Remove statement from cache (lines 296-310)
void test_remove_statement_from_cache(void) {
    // NULL parameters
    bool result = mariadb_remove_statement_from_cache(NULL, NULL);
    TEST_ASSERT_FALSE(result);
    
    DatabaseHandle connection = {0};
    result = mariadb_remove_statement_from_cache(&connection, NULL);
    TEST_ASSERT_FALSE(result);
    
    // With statements
    connection.prepared_statement_count = 2;
    connection.prepared_statements = calloc(2, sizeof(PreparedStatement*));
    connection.prepared_statement_lru_counter = calloc(2, sizeof(uint64_t));
    TEST_ASSERT_NOT_NULL(connection.prepared_statements);
    TEST_ASSERT_NOT_NULL(connection.prepared_statement_lru_counter);
    
    PreparedStatement* stmt1 = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt1);
    connection.prepared_statements[0] = stmt1;
    connection.prepared_statement_lru_counter[0] = 100;
    
    PreparedStatement* stmt2 = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt2);
    connection.prepared_statements[1] = stmt2;
    connection.prepared_statement_lru_counter[1] = 200;
    
    // Remove first statement
    result = mariadb_remove_statement_from_cache(&connection, stmt1);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, connection.prepared_statement_count);
    TEST_ASSERT_EQUAL(stmt2, connection.prepared_statements[0]);
    
    // Try to remove non-existent statement
    result = mariadb_remove_statement_from_cache(&connection, stmt1);
    TEST_ASSERT_FALSE(result);
    
    // Cleanup
    free(stmt1);
    free(stmt2);
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
}

// Test: Cleanup prepared statement (lines 313-322)
void test_cleanup_prepared_statement(void) {
    // NULL statement
    mariadb_cleanup_prepared_statement(NULL);
    TEST_ASSERT_TRUE(true); // Should not crash
    
    // With valid statement
    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);
    stmt->name = strdup("test_stmt");
    stmt->sql_template = strdup("SELECT 1");
    stmt->engine_specific_handle = (void*)0x1234;
    
    mariadb_cleanup_prepared_statement(stmt);
    // Statement is freed, no assertions needed
    
    // With NULL handle pointer
    stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);
    stmt->name = strdup("test_stmt");
    stmt->sql_template = strdup("SELECT 1");
    stmt->engine_specific_handle = (void*)0x1234;
    
    mariadb_stmt_close_t saved_ptr = mariadb_stmt_close_ptr;
    mariadb_stmt_close_ptr = NULL;
    mariadb_cleanup_prepared_statement(stmt);
    
    // Reset
    mariadb_stmt_close_ptr = saved_ptr;
}

// Test: Add prepared statement stub (lines 325-328)
void test_add_prepared_statement_stub(void) {
    PreparedStatementCache cache = {0};
    bool result = mariadb_add_prepared_statement(&cache, "test_stmt");
    TEST_ASSERT_TRUE(result);
}

// Test: Remove prepared statement stub (lines 331-334)
void test_remove_prepared_statement_stub(void) {
    PreparedStatementCache cache = {0};
    bool result = mariadb_remove_prepared_statement(&cache, "test_stmt");
    TEST_ASSERT_TRUE(result);
}

int main(void) {
    UNITY_BEGIN();
    
    // Cache management tests
    RUN_TEST(test_add_to_cache_eviction_failure);
    
    // Prepare statement error path tests
    RUN_TEST(test_prepare_statement_null_parameters);
    RUN_TEST(test_prepare_statement_add_to_cache_failure);
    
    // LRU counter tests
    RUN_TEST(test_update_lru_counter_null_parameters);
    RUN_TEST(test_update_lru_counter_updates_correctly);
    
    // Utility function tests
    RUN_TEST(test_validate_prepared_statement_functions);
    RUN_TEST(test_create_statement_handle);
    RUN_TEST(test_prepare_statement_handle);
    
    // LRU management tests
    RUN_TEST(test_find_lru_statement_index);
    RUN_TEST(test_evict_lru_statement);
    
    // Cache operation wrapper tests
    RUN_TEST(test_add_statement_to_cache);
    RUN_TEST(test_remove_statement_from_cache);
    
    // Cleanup tests
    RUN_TEST(test_cleanup_prepared_statement);
    
    // Stub function tests
    RUN_TEST(test_add_prepared_statement_stub);
    RUN_TEST(test_remove_prepared_statement_stub);
    
    return UNITY_END();
}