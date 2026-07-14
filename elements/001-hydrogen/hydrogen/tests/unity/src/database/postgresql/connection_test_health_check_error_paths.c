/*
 * Unity Test File: PostgreSQL Connection Health Check Error Paths
 *
 * This file targets error and branch paths in src/database/postgresql/connection.c
 * that are uncovered by existing unit and blackbox tests (per extras/add_coverage.sh):
 *   - postgresql_connect building conninfo from individual config fields
 *   - postgresql_health_check: bad connection status, in-transaction logging,
 *     PQping failure fallback, query timeout, and all non-OK result status branches
 *   - postgresql_cancel_inflight early-return guards
 */

#include <src/hydrogen.h>
#include <unity.h>

// Enable libpq mock (globally defined for postgresql unity tests, declared here too)
#include <unity/mocks/mock_libpq.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/postgresql/connection.h>

// Forward declarations for functions being tested
bool postgresql_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator);
bool postgresql_health_check(DatabaseHandle* connection);
void postgresql_cancel_inflight(DatabaseHandle* connection);

// Test function prototypes
void test_postgresql_connect_builds_conninfo_from_fields(void);
void test_health_check_connection_status_not_ok(void);
void test_health_check_in_transaction_state(void);
void test_health_check_ping_failed_falls_back_to_query(void);
void test_health_check_query_timeout_exceeded(void);
void test_health_check_fatal_error_status_with_message(void);
void test_cancel_inflight_null_connection(void);
void test_cancel_inflight_cancel_pointers_unavailable(void);

// Helper function prototypes
DatabaseHandle* create_test_database_handle(void);
void destroy_test_database_handle(DatabaseHandle* handle);

void setUp(void) {
    mock_libpq_initialize();
    mock_libpq_reset_all();
    load_libpq_functions(NULL);
}

void tearDown(void) {
    // Nothing to clean up between tests
}

// Helper function to create a test database handle
DatabaseHandle* create_test_database_handle(void) {
    DatabaseHandle* handle = calloc(1, sizeof(DatabaseHandle));
    if (!handle) return NULL;

    PostgresConnection* pg_conn = calloc(1, sizeof(PostgresConnection));
    if (!pg_conn) {
        free(handle);
        return NULL;
    }

    handle->engine_type = DB_ENGINE_POSTGRESQL;
    handle->connection_handle = pg_conn;
    pg_conn->connection = (void*)0x12345678; // Mock connection pointer

    return handle;
}

// Helper function to destroy test database handle
void destroy_test_database_handle(DatabaseHandle* handle) {
    if (handle) {
        if (handle->connection_handle) {
            PostgresConnection* pg_conn = (PostgresConnection*)handle->connection_handle;
            if (pg_conn->prepared_statements) {
                destroy_prepared_statement_cache(pg_conn->prepared_statements);
            }
            free(pg_conn);
        }
        free(handle->designator);
        free(handle);
    }
}

// Covers src/database/postgresql/connection.c:335-342 - building conninfo from
// individual config fields when no explicit connection_string is supplied.
void test_postgresql_connect_builds_conninfo_from_fields(void) {
    ConnectionConfig config = {
        .host = (char*)"db-host",
        .port = 6543,
        .database = (char*)"appdb",
        .username = (char*)"appuser",
        .password = (char*)"apppass",
        .timeout_seconds = 12,
        .connection_string = NULL
    };
    DatabaseHandle* connection = NULL;

    mock_libpq_set_PQconnectdb_result((void*)0x12345678);
    mock_libpq_set_PQstatus_result(CONNECTION_OK);

    bool result = postgresql_connect(&config, &connection, "test");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(connection);

    if (connection) {
        bool disconnect_result = postgresql_disconnect(connection);
        TEST_ASSERT_TRUE(disconnect_result);
    }
}

// Covers connection.c:495-496 - connection status reported as not CONNECTION_OK.
void test_health_check_connection_status_not_ok(void) {
    DatabaseHandle* connection = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(connection);

    mock_libpq_set_PQstatus_result(CONNECTION_BAD);

    bool result = postgresql_health_check(connection);
    TEST_ASSERT_FALSE(result);

    destroy_test_database_handle(connection);
}

// Covers connection.c:502 - logging when connection is in a transaction state.
void test_health_check_in_transaction_state(void) {
    DatabaseHandle* connection = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(connection);

    PostgresConnection* pg_conn = (PostgresConnection*)connection->connection_handle;
    pg_conn->in_transaction = true;

    mock_libpq_set_PQexec_result((void*)0x87654321);
    mock_libpq_set_PQresultStatus_result(PGRES_TUPLES_OK);

    bool result = postgresql_health_check(connection);
    TEST_ASSERT_TRUE(result);

    destroy_test_database_handle(connection);
}

// Covers connection.c:521 - PQping fails and health check falls back to query method.
void test_health_check_ping_failed_falls_back_to_query(void) {
    DatabaseHandle* connection = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(connection);

    ConnectionConfig config = {
        .connection_string = (char*)"host=localhost port=5432 dbname=test"
    };
    connection->config = &config;

    // PING_NO_RESPONSE == 2 -> not PING_OK, triggers fallback log
    mock_libpq_set_PQping_result(2);
    mock_libpq_set_PQexec_result((void*)0x87654321);
    mock_libpq_set_PQresultStatus_result(PGRES_TUPLES_OK);

    bool result = postgresql_health_check(connection);
    TEST_ASSERT_TRUE(result);

    destroy_test_database_handle(connection);
}

// Covers connection.c:539-541 - query execution time exceeded the 5 second budget.
void test_health_check_query_timeout_exceeded(void) {
    DatabaseHandle* connection = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(connection);

    mock_libpq_set_PQexec_result((void*)0x87654321);
    mock_libpq_set_check_timeout_expired_use_mock(true);
    mock_libpq_set_check_timeout_expired_result(true);

    bool result = postgresql_health_check(connection);
    TEST_ASSERT_FALSE(result);

    destroy_test_database_handle(connection);
}

// Covers connection.c:565-588 - non-OK result status switch, error message logging,
// and consecutive_failures increment.
void test_health_check_fatal_error_status_with_message(void) {
    DatabaseHandle* connection = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(connection);

    mock_libpq_set_PQexec_result((void*)0x87654321);
    mock_libpq_set_PQresultStatus_result(PGRES_FATAL_ERROR);
    mock_libpq_set_PQerrorMessage_result("relation \"t\" does not exist");
    mock_libpq_set_PQntuples_result(0);
    mock_libpq_set_PQnfields_result(0);

    bool result = postgresql_health_check(connection);
    TEST_ASSERT_FALSE(result);

    destroy_test_database_handle(connection);
}

// Covers connection.c:649-650 - NULL connection guard.
void test_cancel_inflight_null_connection(void) {
    postgresql_cancel_inflight(NULL);
    TEST_ASSERT_TRUE(true); // Should return silently without crashing
}

// Covers connection.c:652-653 - cancel function pointers unavailable (always NULL
// under the mock build), so the function returns early.
void test_cancel_inflight_cancel_pointers_unavailable(void) {
    DatabaseHandle* connection = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(connection);

    postgresql_cancel_inflight(connection);

    destroy_test_database_handle(connection);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_postgresql_connect_builds_conninfo_from_fields);
    RUN_TEST(test_health_check_connection_status_not_ok);
    RUN_TEST(test_health_check_in_transaction_state);
    RUN_TEST(test_health_check_ping_failed_falls_back_to_query);
    RUN_TEST(test_health_check_query_timeout_exceeded);
    RUN_TEST(test_health_check_fatal_error_status_with_message);
    RUN_TEST(test_cancel_inflight_null_connection);
    RUN_TEST(test_cancel_inflight_cancel_pointers_unavailable);

    return UNITY_END();
}
