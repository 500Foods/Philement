/*
 * Unity Test File: Firebird Connection Management
 * Tests firebird_connect, firebird_disconnect, firebird_health_check,
 * firebird_reset_connection, and the watchdog cancel hooks.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebird/types.h>
#include <src/database/firebird/connection.h>
#include <src/database/firebird/interface.h>
#include <unity/mocks/mock_libfbclient.h>

extern bool firebird_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator);
extern bool firebird_disconnect(DatabaseHandle* connection);
extern bool firebird_health_check(DatabaseHandle* connection);
extern bool firebird_reset_connection(DatabaseHandle* connection);
extern void firebird_cancel_inflight(DatabaseHandle* connection);
extern void firebird_active_stmt_set(DatabaseHandle* connection, void* stmt_handle);
extern void firebird_active_stmt_clear(DatabaseHandle* connection, const void* stmt_handle);
extern DatabaseEngineInterface* firebird_get_interface(void);

// Test function prototypes
void test_firebird_connect_not_null(void);
void test_firebird_connect_invalid_params(void);
void test_firebird_connect_success(void);
void test_firebird_disconnect_null(void);
void test_firebird_disconnect_invalid_engine(void);
void test_firebird_disconnect_success(void);
void test_firebird_health_check_null(void);
void test_firebird_health_check_invalid_engine(void);
void test_firebird_reset_connection_null(void);
void test_firebird_reset_connection_invalid_engine(void);
void test_firebird_cancel_inflight_null(void);
void test_firebird_cancel_inflight_no_stmt(void);
void test_firebird_active_stmt_set_clear(void);

void setUp(void) {
    mock_libfbc_reset_all();
}

void tearDown(void) {
    mock_libfbc_reset_all();
}

void test_firebird_connect_not_null(void) {
    DatabaseEngineInterface* iface = firebird_get_interface();
    TEST_ASSERT_NOT_NULL(iface);
    TEST_ASSERT_NOT_NULL(iface->connect);
}

void test_firebird_connect_invalid_params(void) {
    DatabaseHandle* conn = NULL;

    // NULL config
    bool result = firebird_connect(NULL, &conn, SR_DATABASE);
    TEST_ASSERT_FALSE(result);

    // NULL output pointer
    ConnectionConfig config = {0};
    result = firebird_connect(&config, NULL, SR_DATABASE);
    TEST_ASSERT_FALSE(result);
}

void test_firebird_connect_success(void) {
    ConnectionConfig config = {0};
    config.database = strdup("/tmp/test.fdb");
    config.username = strdup("SYSDBA");
    config.password = strdup("secret");
    config.port = 3050;

    DatabaseHandle* conn = NULL;
    bool result = firebird_connect(&config, &conn, SR_DATABASE);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(conn);
    TEST_ASSERT_EQUAL(DB_ENGINE_FIREBIRD, conn->engine_type);
    TEST_ASSERT_EQUAL(DB_CONNECTION_CONNECTED, conn->status);
    TEST_ASSERT_EQUAL_INT(1, mock_libfbc_get_isc_attach_database_call_count());

    // Clean up
    firebird_disconnect(conn);
}

void test_firebird_disconnect_null(void) {
    bool result = firebird_disconnect(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_firebird_disconnect_invalid_engine(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_POSTGRESQL;
    bool result = firebird_disconnect(&handle);
    TEST_ASSERT_FALSE(result);
}

void test_firebird_disconnect_success(void) {
    ConnectionConfig config = {0};
    config.database = strdup("/tmp/test.fdb");
    config.username = strdup("SYSDBA");
    config.password = strdup("secret");

    DatabaseHandle* conn = NULL;
    bool connected = firebird_connect(&config, &conn, SR_DATABASE);
    TEST_ASSERT_TRUE(connected);
    TEST_ASSERT_NOT_NULL(conn);

    bool result = firebird_disconnect(conn);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(DB_CONNECTION_DISCONNECTED, conn->status);

    // Clean up the handle
    pthread_mutex_destroy(&conn->connection_lock);
    if (conn->designator) free((void*)conn->designator);
    free(conn);
}

void test_firebird_health_check_null(void) {
    bool result = firebird_health_check(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_firebird_health_check_invalid_engine(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_POSTGRESQL;
    bool result = firebird_health_check(&handle);
    TEST_ASSERT_FALSE(result);
}

void test_firebird_reset_connection_null(void) {
    bool result = firebird_reset_connection(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_firebird_reset_connection_invalid_engine(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_POSTGRESQL;
    bool result = firebird_reset_connection(&handle);
    TEST_ASSERT_FALSE(result);
}

void test_firebird_cancel_inflight_null(void) {
    // Should not crash or assert
    firebird_cancel_inflight(NULL);
    // If we get here, the test passes
}

void test_firebird_cancel_inflight_no_stmt(void) {
    ConnectionConfig config = {0};
    config.database = strdup("/tmp/test.fdb");
    config.username = strdup("SYSDBA");
    config.password = strdup("secret");

    DatabaseHandle* conn = NULL;
    firebird_connect(&config, &conn, SR_DATABASE);
    TEST_ASSERT_NOT_NULL(conn);

    // No active statement — cancel should be a no-op
    firebird_cancel_inflight(conn);

    firebird_disconnect(conn);
    pthread_mutex_destroy(&conn->connection_lock);
    if (conn->designator) free((void*)conn->designator);
    free(conn);
}

void test_firebird_active_stmt_set_clear(void) {
    ConnectionConfig config = {0};
    config.database = strdup("/tmp/test.fdb");
    config.username = strdup("SYSDBA");
    config.password = strdup("secret");

    DatabaseHandle* conn = NULL;
    firebird_connect(&config, &conn, SR_DATABASE);
    TEST_ASSERT_NOT_NULL(conn);

    // Set a fake stmt handle — should not crash
    void* fake_stmt = (void*)0x1234;
    firebird_active_stmt_set(NULL, fake_stmt);   // null-safe

    // Set on the valid connection
    firebird_active_stmt_set(conn, fake_stmt);

    // Clear it
    firebird_active_stmt_clear(conn, fake_stmt);

    // Clear with NULL stmt on null connection
    firebird_active_stmt_clear(NULL, NULL);

    firebird_disconnect(conn);
    pthread_mutex_destroy(&conn->connection_lock);
    if (conn->designator) free((void*)conn->designator);
    free(conn);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_firebird_connect_not_null);
    RUN_TEST(test_firebird_connect_invalid_params);
    RUN_TEST(test_firebird_connect_success);
    RUN_TEST(test_firebird_disconnect_null);
    RUN_TEST(test_firebird_disconnect_invalid_engine);
    RUN_TEST(test_firebird_disconnect_success);
    RUN_TEST(test_firebird_health_check_null);
    RUN_TEST(test_firebird_health_check_invalid_engine);
    RUN_TEST(test_firebird_reset_connection_null);
    RUN_TEST(test_firebird_reset_connection_invalid_engine);
    RUN_TEST(test_firebird_cancel_inflight_null);
    RUN_TEST(test_firebird_cancel_inflight_no_stmt);
    RUN_TEST(test_firebird_active_stmt_set_clear);

    return UNITY_END();
}
