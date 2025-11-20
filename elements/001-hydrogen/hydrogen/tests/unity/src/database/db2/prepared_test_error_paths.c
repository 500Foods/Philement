/*
 * Unity Test File: prepared_test_error_paths
 * Tests for DB2 prepared statement error paths and edge cases
 * 
 * This file focuses on covering error conditions and edge cases
 * that are not covered by other tests, using mock_libdb2.
 */

#include <src/hydrogen.h>
#include <unity.h>

// Mock libdb2
#include <unity/mocks/mock_libdb2.h>

// Include DB2 types and prepared statement functions
#include <src/database/db2/types.h>
#include <src/database/db2/prepared.h>
#include <src/database/database.h>

// Forward declarations
bool db2_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt);
bool db2_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);
void db2_update_prepared_lru_counter(DatabaseHandle* connection, const char* stmt_name);
bool db2_add_prepared_statement(PreparedStatementCache* cache, const char* name);
bool db2_remove_prepared_statement(PreparedStatementCache* cache, const char* name);
bool db2_initialize_prepared_statement_cache(DatabaseHandle* connection, size_t cache_size);
bool db2_evict_lru_prepared_statement(DatabaseHandle* connection, const DB2Connection* db2_conn, const char* new_stmt_name);
bool db2_add_prepared_statement_to_cache(DatabaseHandle* connection, PreparedStatement* stmt, size_t cache_size);

// External function pointers
extern SQLAllocHandle_t SQLAllocHandle_ptr;
extern SQLPrepare_t SQLPrepare_ptr;
extern SQLFreeHandle_t SQLFreeHandle_ptr;
extern SQLGetDiagRec_t SQLGetDiagRec_ptr;

// Test function prototypes
void test_evict_lru_no_free_handle_ptr(void);
void test_prepare_statement_evict_lru_failure(void);
void test_prepare_statement_sqlprepare_failure(void);
void test_prepare_statement_sqlprepare_failure_no_free_handle(void);
void test_prepare_statement_add_to_cache_failure(void);
void test_update_lru_counter_null_connection(void);
void test_update_lru_counter_null_stmt_name(void);
void test_update_lru_counter_updates_correctly(void);
void test_update_lru_counter_statement_not_found(void);
void test_add_prepared_statement_stub(void);
void test_remove_prepared_statement_stub(void);

void setUp(void) {
    // Reset all mocks before each test
    mock_libdb2_reset_all();
    
    // Set function pointers to mock implementations
    SQLAllocHandle_ptr = mock_SQLAllocHandle;
    SQLPrepare_ptr = mock_SQLPrepare;
    SQLFreeHandle_ptr = mock_SQLFreeHandle;
    SQLGetDiagRec_ptr = mock_SQLGetDiagRec;
}

void tearDown(void) {
    // Cleanup after each test
    mock_libdb2_reset_all();
}

// Test: Evict LRU - SQLFreeHandle_ptr not available (lines 83-84)
void test_evict_lru_no_free_handle_ptr(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    connection.prepared_statement_count = 1;
    
    // Allocate and setup prepared statement arrays
    connection.prepared_statements = calloc(1, sizeof(PreparedStatement*));
    connection.prepared_statement_lru_counter = calloc(1, sizeof(uint64_t));
    TEST_ASSERT_NOT_NULL(connection.prepared_statements);
    TEST_ASSERT_NOT_NULL(connection.prepared_statement_lru_counter);
    
    // Create a prepared statement
    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);
    stmt->name = strdup("test_stmt");
    stmt->sql_template = strdup("SELECT 1");
    stmt->engine_specific_handle = (void*)0x1234;
    
    connection.prepared_statements[0] = stmt;
    connection.prepared_statement_lru_counter[0] = 1;
    
    DB2Connection db2_conn = {0};
    db2_conn.connection = (void*)0x5678;
    
    // Clear SQLFreeHandle_ptr to trigger error path
    SQLFreeHandle_ptr = NULL;
    
    bool result = db2_evict_lru_prepared_statement(&connection, &db2_conn, "new_stmt");
    
    TEST_ASSERT_FALSE(result);
    
    // Cleanup
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
}

// Test: Prepare statement - evict_lru failure (line 109)
void test_prepare_statement_evict_lru_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    connection.config = calloc(1, sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(connection.config);
    connection.config->prepared_statement_cache_size = 1;
    
    DB2Connection db2_conn = {0};
    db2_conn.connection = (void*)0x1234;
    connection.connection_handle = &db2_conn;
    
    // Setup mocks for success
    mock_libdb2_set_SQLAllocHandle_result(SQL_SUCCESS);
    mock_libdb2_set_SQLAllocHandle_output_handle((void*)0x1111);
    
    // Create first statement to fill cache
    PreparedStatement* stmt1 = NULL;
    TEST_ASSERT_TRUE(db2_prepare_statement(&connection, "stmt_1", "SELECT 1", &stmt1));
    TEST_ASSERT_NOT_NULL(stmt1);
    TEST_ASSERT_EQUAL(1, connection.prepared_statement_count);
    
    // Now try to add another - will need to evict, but make SQLFreeHandle_ptr unavailable
    SQLFreeHandle_ptr = NULL;
    
    mock_libdb2_set_SQLAllocHandle_output_handle((void*)0x2222);
    PreparedStatement* stmt2 = NULL;
    bool result = db2_prepare_statement(&connection, "stmt_2", "SELECT 2", &stmt2);
    
    // Should fail because eviction fails
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt2);
    
    // Cleanup
    free(stmt1->name);
    free(stmt1->sql_template);
    free(stmt1);
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
    free(connection.config);
}

// Test: Prepare statement - SQLPrepare failure with diagnostic (lines 158-167)
void test_prepare_statement_sqlprepare_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    
    DB2Connection db2_conn = {0};
    db2_conn.connection = (void*)0x1234;
    connection.connection_handle = &db2_conn;
    
    // Setup mocks
    mock_libdb2_set_SQLAllocHandle_result(SQL_SUCCESS);
    mock_libdb2_set_SQLAllocHandle_output_handle((void*)0x5678);
    
    // Make SQLPrepare fail
    mock_libdb2_set_SQLPrepare_result(-1); // SQL_ERROR
    mock_libdb2_set_SQLGetDiagRec_error("42000", 12345, "Syntax error in SQL statement");
    
    PreparedStatement* stmt = NULL;
    bool result = db2_prepare_statement(&connection, "test_stmt", "INVALID SQL", &stmt);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

// Test: Prepare statement - SQLPrepare failure with no SQLFreeHandle (lines 164-165)
void test_prepare_statement_sqlprepare_failure_no_free_handle(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    
    DB2Connection db2_conn = {0};
    db2_conn.connection = (void*)0x1234;
    connection.connection_handle = &db2_conn;
    
    // Setup mocks
    mock_libdb2_set_SQLAllocHandle_result(SQL_SUCCESS);
    mock_libdb2_set_SQLAllocHandle_output_handle((void*)0x5678);
    
    // Make SQLPrepare fail AND clear SQLFreeHandle_ptr
    mock_libdb2_set_SQLPrepare_result(-1); // SQL_ERROR
    SQLFreeHandle_ptr = NULL;
    
    PreparedStatement* stmt = NULL;
    bool result = db2_prepare_statement(&connection, "test_stmt", "INVALID SQL", &stmt);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

// Test: Prepare statement - add to cache failure (lines 212-221)
void test_prepare_statement_add_to_cache_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    connection.config = calloc(1, sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(connection.config);
    connection.config->prepared_statement_cache_size = 1;
    
    DB2Connection db2_conn = {0};
    db2_conn.connection = (void*)0x1234;
    connection.connection_handle = &db2_conn;
    
    // Setup mocks
    mock_libdb2_set_SQLAllocHandle_result(SQL_SUCCESS);
    
    // Create first statement normally
    mock_libdb2_set_SQLAllocHandle_output_handle((void*)0x1111);
    PreparedStatement* stmt1 = NULL;
    TEST_ASSERT_TRUE(db2_prepare_statement(&connection, "stmt_1", "SELECT 1", &stmt1));
    
    // Now try to add another with cache full and eviction fails
    SQLFreeHandle_ptr = NULL;
    mock_libdb2_set_SQLAllocHandle_output_handle((void*)0x2222);
    
    PreparedStatement* stmt2 = NULL;
    bool result = db2_prepare_statement(&connection, "stmt_2", "SELECT 2", &stmt2);
    
    // Should fail because eviction fails
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt2);
    
    // Cleanup
    free(stmt1->name);
    free(stmt1->sql_template);
    free(stmt1);
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
    free(connection.config);
}

// Test: db2_update_prepared_lru_counter - NULL connection (line 272)
void test_update_lru_counter_null_connection(void) {
    db2_update_prepared_lru_counter(NULL, "test_stmt");
    // Should not crash - just return
    TEST_ASSERT_TRUE(true);
}

// Test: db2_update_prepared_lru_counter - NULL stmt_name (line 272)
void test_update_lru_counter_null_stmt_name(void) {
    DatabaseHandle connection = {0};
    db2_update_prepared_lru_counter(&connection, NULL);
    // Should not crash - just return
    TEST_ASSERT_TRUE(true);
}

// Test: db2_update_prepared_lru_counter - updates counter and usage (lines 276-283)
void test_update_lru_counter_updates_correctly(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    connection.prepared_statement_count = 2;
    
    // Allocate arrays
    connection.prepared_statements = calloc(2, sizeof(PreparedStatement*));
    connection.prepared_statement_lru_counter = calloc(2, sizeof(uint64_t));
    TEST_ASSERT_NOT_NULL(connection.prepared_statements);
    TEST_ASSERT_NOT_NULL(connection.prepared_statement_lru_counter);
    
    // Create test statements
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
    
    // Update LRU counter for stmt_1
    db2_update_prepared_lru_counter(&connection, "stmt_1");
    
    // Verify counter was updated (changed from initial_counter) and usage incremented
    // Note: The global_lru_counter is a static variable starting at 0, so it will be a low number
    TEST_ASSERT_NOT_EQUAL(initial_counter, connection.prepared_statement_lru_counter[0]);
    TEST_ASSERT_EQUAL(1, stmt1->usage_count);
    TEST_ASSERT_EQUAL(0, stmt2->usage_count); // stmt2 unchanged
    
    // Cleanup
    free(stmt1->name);
    free(stmt1);
    free(stmt2->name);
    free(stmt2);
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
}

// Test: db2_update_prepared_lru_counter - statement not found
void test_update_lru_counter_statement_not_found(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    connection.prepared_statement_count = 1;
    
    // Allocate arrays
    connection.prepared_statements = calloc(1, sizeof(PreparedStatement*));
    connection.prepared_statement_lru_counter = calloc(1, sizeof(uint64_t));
    TEST_ASSERT_NOT_NULL(connection.prepared_statements);
    TEST_ASSERT_NOT_NULL(connection.prepared_statement_lru_counter);
    
    // Create test statement
    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);
    stmt->name = strdup("stmt_1");
    stmt->usage_count = 0;
    connection.prepared_statements[0] = stmt;
    connection.prepared_statement_lru_counter[0] = 100;
    
    // Try to update counter for non-existent statement
    db2_update_prepared_lru_counter(&connection, "nonexistent_stmt");
    
    // Original statement should be unchanged
    TEST_ASSERT_EQUAL(100, connection.prepared_statement_lru_counter[0]);
    TEST_ASSERT_EQUAL(0, stmt->usage_count);
    
    // Cleanup
    free(stmt->name);
    free(stmt);
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
}

// Test: db2_add_prepared_statement - stub function (lines 288-292)
void test_add_prepared_statement_stub(void) {
    PreparedStatementCache cache = {0};
    
    bool result = db2_add_prepared_statement(&cache, "test_stmt");
    
    // Stub always returns true
    TEST_ASSERT_TRUE(result);
}

// Test: db2_remove_prepared_statement - stub function (lines 295-299)
void test_remove_prepared_statement_stub(void) {
    PreparedStatementCache cache = {0};
    
    bool result = db2_remove_prepared_statement(&cache, "test_stmt");
    
    // Stub always returns true
    TEST_ASSERT_TRUE(result);
}

int main(void) {
    UNITY_BEGIN();
    
    // Eviction error path tests
    RUN_TEST(test_evict_lru_no_free_handle_ptr);
    RUN_TEST(test_prepare_statement_evict_lru_failure);
    
    // SQLPrepare failure tests
    RUN_TEST(test_prepare_statement_sqlprepare_failure);
    RUN_TEST(test_prepare_statement_sqlprepare_failure_no_free_handle);
    
    // Cache management failure tests
    RUN_TEST(test_prepare_statement_add_to_cache_failure);
    
    // LRU counter update tests
    RUN_TEST(test_update_lru_counter_null_connection);
    RUN_TEST(test_update_lru_counter_null_stmt_name);
    RUN_TEST(test_update_lru_counter_updates_correctly);
    RUN_TEST(test_update_lru_counter_statement_not_found);
    
    // Legacy stub function tests
    RUN_TEST(test_add_prepared_statement_stub);
    RUN_TEST(test_remove_prepared_statement_stub);
    
    return UNITY_END();
}