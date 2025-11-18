/*
 * Unity Test File: prepared_test_postgresql_timeout_scenarios
 * Tests for PostgreSQL prepared statement timeout handling
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/database/postgresql/prepared.h>
#include <src/database/postgresql/types.h>
#include <src/database/postgresql/connection.h>
#include <src/database/database.h>

// Include mocks
#include <tests/unity/mocks/mock_libpq.h>

// Forward declarations
bool postgresql_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt);
bool postgresql_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);

// Mock function pointers
extern PQprepare_t PQprepare_ptr;
extern PQexec_t PQexec_ptr;
extern PQresultStatus_t PQresultStatus_ptr;
extern PQclear_t PQclear_ptr;
extern PQerrorMessage_t PQerrorMessage_ptr;

// Helper function prototypes
DatabaseHandle* create_test_connection(void);
void cleanup_test_connection(DatabaseHandle* conn);

// Test function prototypes
void test_prepare_statement_basic_functionality(void);
void test_prepare_statement_null_pg_connection(void);
void test_unprepare_statement_basic_functionality(void);

// Test fixtures
void setUp(void) {
    mock_libpq_reset_all();
    
    PQprepare_ptr = mock_PQprepare;
    PQexec_ptr = mock_PQexec;
    PQresultStatus_ptr = mock_PQresultStatus;
    PQclear_ptr = mock_PQclear;
    PQerrorMessage_ptr = mock_PQerrorMessage;
    
    mock_libpq_set_PQexec_result((void*)0x87654321);
    mock_libpq_set_PQresultStatus_result(PGRES_COMMAND_OK);
    mock_libpq_set_check_timeout_expired_result(false);
}

void tearDown(void) {
    mock_libpq_reset_all();
}

// Helper to create test connection
DatabaseHandle* create_test_connection(void) {
    DatabaseHandle* conn = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(conn);
    
    conn->engine_type = DB_ENGINE_POSTGRESQL;
    conn->prepared_statement_count = 0;
    conn->prepared_statements = NULL;
    conn->prepared_statement_lru_counter = NULL;
    
    PostgresConnection* pg_conn = calloc(1, sizeof(PostgresConnection));
    TEST_ASSERT_NOT_NULL(pg_conn);
    pg_conn->connection = (void*)0x12345678;
    pg_conn->in_transaction = false;
    
    conn->connection_handle = pg_conn;
    return conn;
}

// Helper to cleanup connection
void cleanup_test_connection(DatabaseHandle* conn) {
    if (!conn) return;
    
    if (conn->prepared_statements) {
        for (size_t i = 0; i < conn->prepared_statement_count; i++) {
            if (conn->prepared_statements[i]) {
                free(conn->prepared_statements[i]->name);
                free(conn->prepared_statements[i]->sql_template);
                free(conn->prepared_statements[i]);
            }
        }
        free(conn->prepared_statements);
    }
    
    if (conn->prepared_statement_lru_counter) {
        free(conn->prepared_statement_lru_counter);
    }
    if (conn->connection_handle) {
        free(conn->connection_handle);
    }
    free(conn);
}

// Test: PREPARE operation (no longer has individual timeout checks - timeouts set at connection level)
void test_prepare_statement_basic_functionality(void) {
    DatabaseHandle* conn = create_test_connection();

    PreparedStatement* stmt = NULL;
    bool result = postgresql_prepare_statement(conn, "test_stmt", "SELECT 1", &stmt);

    // Should succeed (timeout handling moved to connection level)
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(stmt);

    cleanup_test_connection(conn);
}

// Test: NULL pg_connection->connection
void test_prepare_statement_null_pg_connection(void) {
    DatabaseHandle* conn = create_test_connection();
    
    // Set pg_conn->connection to NULL
    PostgresConnection* pg_conn = (PostgresConnection*)conn->connection_handle;
    pg_conn->connection = NULL;
    
    PreparedStatement* stmt = NULL;
    bool result = postgresql_prepare_statement(conn, "test_stmt", "SELECT 1", &stmt);
    
    // Should fail due to NULL connection
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
    
    cleanup_test_connection(conn);
}

// Test: DEALLOCATE operation (no longer has individual timeout checks - timeouts set at connection level)
void test_unprepare_statement_basic_functionality(void) {
    DatabaseHandle* conn = create_test_connection();

    // First create a statement successfully
    PreparedStatement* stmt = NULL;
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "test_stmt", "SELECT 1", &stmt));
    TEST_ASSERT_NOT_NULL(stmt);

    // Now unprepare the statement
    bool result = postgresql_unprepare_statement(conn, stmt);

    // Should succeed (timeout handling moved to connection level)
    TEST_ASSERT_TRUE(result);

    cleanup_test_connection(conn);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_prepare_statement_basic_functionality);
    RUN_TEST(test_prepare_statement_null_pg_connection);
    RUN_TEST(test_unprepare_statement_basic_functionality);

    return UNITY_END();
}