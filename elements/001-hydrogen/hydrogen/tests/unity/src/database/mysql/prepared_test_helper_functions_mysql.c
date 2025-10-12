/*
 * Unity Test File: prepared_test_helper_functions_mysql
 * Tests for MySQL prepared statement helper functions
 */

#include <src/hydrogen.h>
#include <unity.h>

// MySQL mocks are enabled via CMake
#include <unity/mocks/mock_libmysqlclient.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/mysql/types.h>
#include <src/database/mysql/connection.h>
#include <src/database/mysql/prepared.h>

// External declarations for MySQL function pointers
extern mysql_stmt_init_t mysql_stmt_init_ptr;
extern mysql_stmt_prepare_t mysql_stmt_prepare_ptr;
extern mysql_stmt_execute_t mysql_stmt_execute_ptr;
extern mysql_stmt_close_t mysql_stmt_close_ptr;
extern mysql_error_t mysql_error_ptr;

// Forward declarations for helper functions being tested
bool mysql_validate_prepared_statement_functions(void);
void* mysql_create_statement_handle(void* mysql_connection);
bool mysql_prepare_statement_handle(void* stmt_handle, const char* sql);
bool mysql_initialize_prepared_statement_cache(DatabaseHandle* connection, size_t cache_size);
size_t mysql_find_lru_statement_index(DatabaseHandle* connection);
void mysql_evict_lru_statement(DatabaseHandle* connection, size_t lru_index);
bool mysql_add_statement_to_cache(DatabaseHandle* connection, PreparedStatement* stmt, size_t cache_size);
bool mysql_remove_statement_from_cache(DatabaseHandle* connection, const PreparedStatement* stmt);
void mysql_cleanup_prepared_statement(PreparedStatement* stmt);

// Test function prototypes
void test_mysql_validate_prepared_statement_functions_available(void);
void test_mysql_validate_prepared_statement_functions_unavailable(void);
void test_mysql_create_statement_handle_success(void);
void test_mysql_create_statement_handle_failure(void);
void test_mysql_prepare_statement_handle_success(void);
void test_mysql_prepare_statement_handle_failure(void);
void test_mysql_initialize_prepared_statement_cache_first_time(void);
void test_mysql_initialize_prepared_statement_cache_already_initialized(void);
void test_mysql_initialize_prepared_statement_cache_allocation_failure(void);
void test_mysql_find_lru_statement_index_basic(void);
void test_mysql_find_lru_statement_index_empty_cache(void);
void test_mysql_evict_lru_statement_basic(void);
void test_mysql_evict_lru_statement_null_handle(void);
void test_mysql_add_statement_to_cache_basic(void);
void test_mysql_add_statement_to_cache_with_eviction(void);
void test_mysql_remove_statement_from_cache_basic(void);
void test_mysql_remove_statement_from_cache_not_found(void);
void test_mysql_cleanup_prepared_statement_basic(void);
void test_mysql_cleanup_prepared_statement_null(void);

void setUp(void) {
    // Reset mocks before each test
    mock_libmysqlclient_reset_all();

    // Initialize MySQL function pointers to use mocks
    load_libmysql_functions(NULL);

    // Set prepared statement function pointers to mock implementations
    mysql_stmt_init_ptr = mock_mysql_stmt_init;
    mysql_stmt_prepare_ptr = mock_mysql_stmt_prepare;
    mysql_stmt_execute_ptr = mock_mysql_stmt_execute;
    mysql_stmt_close_ptr = mock_mysql_stmt_close;
    mysql_error_ptr = mock_mysql_error;
}

void tearDown(void) {
    // Cleanup after each test
    mock_libmysqlclient_reset_all();
}

// Test: Validate prepared statement functions when available
void test_mysql_validate_prepared_statement_functions_available(void) {
    // Function pointers are already set in setUp
    bool result = mysql_validate_prepared_statement_functions();
    TEST_ASSERT_TRUE(result);
}

// Test: Validate prepared statement functions when unavailable
void test_mysql_validate_prepared_statement_functions_unavailable(void) {
    // Clear function pointers
    mysql_stmt_init_ptr = NULL;
    mysql_stmt_prepare_ptr = NULL;
    mysql_stmt_execute_ptr = NULL;
    mysql_stmt_close_ptr = NULL;

    bool result = mysql_validate_prepared_statement_functions();
    TEST_ASSERT_FALSE(result);
}

// Test: Create statement handle successfully
void test_mysql_create_statement_handle_success(void) {
    void* mysql_conn = (void*)0x1234;

    // Mock mysql_stmt_init to succeed
    mock_libmysqlclient_set_mysql_stmt_init_result((void*)0x5678);

    void* stmt_handle = mysql_create_statement_handle(mysql_conn);
    TEST_ASSERT_NOT_NULL(stmt_handle);
    TEST_ASSERT_EQUAL((void*)0x5678, stmt_handle);
}

// Test: Create statement handle failure
void test_mysql_create_statement_handle_failure(void) {
    void* mysql_conn = (void*)0x1234;

    // Mock mysql_stmt_init to fail
    mock_libmysqlclient_set_mysql_stmt_init_result(NULL);

    void* stmt_handle = mysql_create_statement_handle(mysql_conn);
    TEST_ASSERT_NULL(stmt_handle);
}

// Test: Prepare statement handle successfully
void test_mysql_prepare_statement_handle_success(void) {
    void* stmt_handle = (void*)0x5678;
    const char* sql = "SELECT * FROM users WHERE id = ?";

    // Mock mysql_stmt_prepare to succeed
    mock_libmysqlclient_set_mysql_stmt_prepare_result(0);

    bool result = mysql_prepare_statement_handle(stmt_handle, sql);
    TEST_ASSERT_TRUE(result);
}

// Test: Prepare statement handle failure
void test_mysql_prepare_statement_handle_failure(void) {
    void* stmt_handle = (void*)0x5678;
    const char* sql = "INVALID SQL";

    // Mock mysql_stmt_prepare to fail
    mock_libmysqlclient_set_mysql_stmt_prepare_result(1);

    bool result = mysql_prepare_statement_handle(stmt_handle, sql);
    TEST_ASSERT_FALSE(result);
}

// Test: Initialize prepared statement cache first time
void test_mysql_initialize_prepared_statement_cache_first_time(void) {
    DatabaseHandle connection = {0};
    size_t cache_size = 100;

    bool result = mysql_initialize_prepared_statement_cache(&connection, cache_size);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(connection.prepared_statements);
    TEST_ASSERT_NOT_NULL(connection.prepared_statement_lru_counter);
    TEST_ASSERT_EQUAL(0, connection.prepared_statement_count);

    // Cleanup
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
}

// Test: Initialize prepared statement cache already initialized
void test_mysql_initialize_prepared_statement_cache_already_initialized(void) {
    DatabaseHandle connection = {0};
    size_t cache_size = 100;

    // Initialize once
    connection.prepared_statements = calloc(cache_size, sizeof(PreparedStatement*));
    connection.prepared_statement_lru_counter = calloc(cache_size, sizeof(uint64_t));
    connection.prepared_statement_count = 0;

    // Try to initialize again
    bool result = mysql_initialize_prepared_statement_cache(&connection, cache_size);
    TEST_ASSERT_TRUE(result); // Should succeed since already initialized

    // Cleanup
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
}

// Test: Find LRU statement index in basic case
void test_mysql_find_lru_statement_index_basic(void) {
    DatabaseHandle connection = {0};
    connection.prepared_statement_count = 3;

    // Set up LRU counters with known values
    connection.prepared_statement_lru_counter = calloc(3, sizeof(uint64_t));
    connection.prepared_statement_lru_counter[0] = 100; // Least recently used
    connection.prepared_statement_lru_counter[1] = 300;
    connection.prepared_statement_lru_counter[2] = 200;

    size_t lru_index = mysql_find_lru_statement_index(&connection);
    TEST_ASSERT_EQUAL(0, lru_index); // Index 0 has the lowest LRU value

    // Cleanup
    free(connection.prepared_statement_lru_counter);
}

// Test: Find LRU statement index with empty cache
void test_mysql_find_lru_statement_index_empty_cache(void) {
    DatabaseHandle connection = {0};
    connection.prepared_statement_count = 0;

    size_t lru_index = mysql_find_lru_statement_index(&connection);
    TEST_ASSERT_EQUAL(0, lru_index); // Should return 0 for empty cache
}

// Test: Evict LRU statement basic case
void test_mysql_evict_lru_statement_basic(void) {
    DatabaseHandle connection = {0};
    connection.prepared_statement_count = 2;

    // Set up prepared statements
    connection.prepared_statements = calloc(2, sizeof(PreparedStatement*));
    connection.prepared_statement_lru_counter = calloc(2, sizeof(uint64_t));

    PreparedStatement* stmt1 = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt1);
    if (stmt1) {
        stmt1->name = strdup("stmt1");
        stmt1->sql_template = strdup("SELECT 1");
        stmt1->engine_specific_handle = (void*)0x1111;
    }

    PreparedStatement* stmt2 = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt2);
    if (stmt2) {
        stmt2->name = strdup("stmt2");
        stmt2->sql_template = strdup("SELECT 2");
        stmt2->engine_specific_handle = (void*)0x2222;
    }

    connection.prepared_statements[0] = stmt1;
    connection.prepared_statements[1] = stmt2;
    connection.prepared_statement_lru_counter[0] = 100; // LRU
    connection.prepared_statement_lru_counter[1] = 200;

    // Evict LRU statement (index 0)
    mysql_evict_lru_statement(&connection, 0);

    TEST_ASSERT_EQUAL(1, connection.prepared_statement_count);
    TEST_ASSERT_EQUAL(stmt2, connection.prepared_statements[0]);

    // Cleanup
    free(stmt2->name);
    free(stmt2->sql_template);
    free(stmt2);
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
}

// Test: Add statement to cache basic case
void test_mysql_add_statement_to_cache_basic(void) {
    DatabaseHandle connection = {0};
    size_t cache_size = 2;

    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);
    if (stmt) {
        stmt->name = strdup("test_stmt");
        stmt->sql_template = strdup("SELECT 1");
        stmt->engine_specific_handle = (void*)0x5678;
    }

    bool result = mysql_add_statement_to_cache(&connection, stmt, cache_size);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, connection.prepared_statement_count);
    TEST_ASSERT_EQUAL(stmt, connection.prepared_statements[0]);

    // Cleanup
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
}

// Test: Remove statement from cache basic case
void test_mysql_remove_statement_from_cache_basic(void) {
    DatabaseHandle connection = {0};
    connection.prepared_statement_count = 1;

    connection.prepared_statements = calloc(1, sizeof(PreparedStatement*));
    connection.prepared_statement_lru_counter = calloc(1, sizeof(uint64_t));

    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);
    if (stmt) {
        stmt->name = strdup("test_stmt");
        stmt->sql_template = strdup("SELECT 1");
    }

    connection.prepared_statements[0] = stmt;

    bool result = mysql_remove_statement_from_cache(&connection, stmt);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, connection.prepared_statement_count);

    // Cleanup
    if (stmt) {
        free(stmt->name);
        free(stmt->sql_template);
        free(stmt);
    }
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
}

// Test: Remove statement from cache not found
void test_mysql_remove_statement_from_cache_not_found(void) {
    DatabaseHandle connection = {0};
    connection.prepared_statement_count = 1;

    connection.prepared_statements = calloc(1, sizeof(PreparedStatement*));

    PreparedStatement* stmt1 = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt1);
    if (stmt1) {
        stmt1->name = strdup("stmt1");
    }
    connection.prepared_statements[0] = stmt1;

    PreparedStatement* stmt2 = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt2);
    if (stmt2) {
        stmt2->name = strdup("stmt2");
    }

    bool result = mysql_remove_statement_from_cache(&connection, stmt2);
    TEST_ASSERT_FALSE(result); // Should not find stmt2
    TEST_ASSERT_EQUAL(1, connection.prepared_statement_count); // Count should remain the same

    // Cleanup
    if (stmt1) {
        free(stmt1->name);
        free(stmt1);
    }
    if (stmt2) {
        free(stmt2->name);
        free(stmt2);
    }
    free(connection.prepared_statements);
}

// Test: Cleanup prepared statement basic case
void test_mysql_cleanup_prepared_statement_basic(void) {
    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);
    if (stmt) {
        stmt->name = strdup("test_stmt");
        stmt->sql_template = strdup("SELECT 1");
    }

    // Should not crash
    mysql_cleanup_prepared_statement(stmt);

    // Statement should be freed, so we don't need to cleanup here
}

// Test: Cleanup prepared statement null
void test_mysql_cleanup_prepared_statement_null(void) {
    // Should not crash with NULL input
    mysql_cleanup_prepared_statement(NULL);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mysql_validate_prepared_statement_functions_available);
    RUN_TEST(test_mysql_validate_prepared_statement_functions_unavailable);
    RUN_TEST(test_mysql_create_statement_handle_success);
    RUN_TEST(test_mysql_create_statement_handle_failure);
    RUN_TEST(test_mysql_prepare_statement_handle_success);
    RUN_TEST(test_mysql_prepare_statement_handle_failure);
    RUN_TEST(test_mysql_initialize_prepared_statement_cache_first_time);
    RUN_TEST(test_mysql_initialize_prepared_statement_cache_already_initialized);
    RUN_TEST(test_mysql_find_lru_statement_index_basic);
    RUN_TEST(test_mysql_find_lru_statement_index_empty_cache);
    RUN_TEST(test_mysql_evict_lru_statement_basic);
    RUN_TEST(test_mysql_add_statement_to_cache_basic);
    RUN_TEST(test_mysql_remove_statement_from_cache_basic);
    RUN_TEST(test_mysql_remove_statement_from_cache_not_found);
    RUN_TEST(test_mysql_cleanup_prepared_statement_basic);
    RUN_TEST(test_mysql_cleanup_prepared_statement_null);

    return UNITY_END();
}