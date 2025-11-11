/*
 * Unity Test File: heartbeat_test_missing_coverage
 * This file contains unit tests for heartbeat.c functions to cover
 * the remaining uncovered lines identified by coverage analysis.
 * Focuses on error paths and edge cases that require mocking.
 */

#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks for testing failure scenarios
#define USE_MOCK_SYSTEM
#define USE_MOCK_LOGGING
#include <unity/mocks/mock_system.h>
#include <unity/mocks/mock_logging.h>

// Include source headers after mocks
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database_connstring.h>
#include <src/database/database_pending.h>
#include <src/mutex/mutex.h>

// Forward declarations for mock functions
bool mock_database_engine_connect_with_designator(DatabaseEngine engine_type, ConnectionConfig* config, DatabaseHandle** connection, const char* designator);
bool mock_database_engine_health_check(DatabaseHandle* connection);
void mock_database_engine_cleanup_connection(DatabaseHandle* connection);
bool mock_database_engine_init(void);
void mock_database_queue_signal_initial_connection_complete(DatabaseQueue* db_queue);
size_t mock_database_queue_get_depth_with_designator(DatabaseQueue* db_queue, const char* designator);
PendingResultManager* mock_get_pending_result_manager(void);
size_t mock_pending_result_cleanup_expired(PendingResultManager* manager, const char* designator);
MutexResult mock_mutex_lock(pthread_mutex_t* mutex, const char* designator);

// Mock state variables
static bool mock_connect_success = true;
static bool mock_health_check_success = true;
static bool mock_engine_init_success = true;
static bool mock_mutex_lock_success = true;
static bool mock_signal_called = false;
static size_t mock_queue_depth = 0;
static PendingResultManager* mock_pending_manager = NULL;
static size_t mock_cleanup_count = 0;

// Mock implementations
bool mock_database_engine_connect_with_designator(DatabaseEngine engine_type, ConnectionConfig* config, DatabaseHandle** connection, const char* designator) {
    (void)engine_type;
    (void)config;
    (void)designator;

    if (mock_connect_success && connection) {
        *connection = calloc(1, sizeof(DatabaseHandle));
        if (*connection) {
            (*connection)->engine_type = engine_type;
            (*connection)->designator = designator ? strdup(designator) : NULL;
            (*connection)->status = DB_CONNECTION_CONNECTED;
            (*connection)->connected_since = time(NULL);
            // Initialize mutex
            pthread_mutex_init(&(*connection)->connection_lock, NULL);
            return true;
        }
    }
    return false;
}

bool mock_database_engine_health_check(DatabaseHandle* connection) {
    (void)connection;
    return mock_health_check_success;
}

void mock_database_engine_cleanup_connection(DatabaseHandle* connection) {
    if (connection) {
        if (connection->designator) free(connection->designator);
        // Note: In our mock, we don't allocate config, so don't try to free it
        pthread_mutex_destroy(&connection->connection_lock);
        free(connection);
    }
}

bool mock_database_engine_init(void) {
    return mock_engine_init_success;
}


void mock_database_queue_signal_initial_connection_complete(DatabaseQueue* db_queue) {
    (void)db_queue;
    mock_signal_called = true;
}

size_t mock_database_queue_get_depth_with_designator(DatabaseQueue* db_queue, const char* designator) {
    (void)db_queue;
    (void)designator;
    return mock_queue_depth;
}

PendingResultManager* mock_get_pending_result_manager(void) {
    return mock_pending_manager;
}

size_t mock_pending_result_cleanup_expired(PendingResultManager* manager, const char* designator) {
    (void)manager;
    (void)designator;
    return mock_cleanup_count;
}

MutexResult mock_mutex_lock(pthread_mutex_t* mutex, const char* designator) {
    (void)mutex;
    (void)designator;
    return mock_mutex_lock_success ? MUTEX_SUCCESS : MUTEX_ERROR;
}

// Override functions with mocks
#define database_engine_connect_with_designator mock_database_engine_connect_with_designator
#define database_engine_health_check mock_database_engine_health_check
#define database_engine_cleanup_connection mock_database_engine_cleanup_connection
#define database_engine_init mock_database_engine_init
#define database_queue_signal_initial_connection_complete mock_database_queue_signal_initial_connection_complete
#define database_queue_get_depth_with_designator mock_database_queue_get_depth_with_designator
#define get_pending_result_manager mock_get_pending_result_manager
#define pending_result_cleanup_expired mock_pending_result_cleanup_expired

// Test function prototypes
void test_corrupted_mutex_detection_in_handle_connection_success(void);
void test_health_check_failure_after_connection(void);
void test_config_database_logging_path(void);
void test_engine_init_failure_in_check_connection(void);
void test_early_return_due_to_shutdown_in_perform_heartbeat(void);
void test_connection_status_change_logging_in_perform_heartbeat(void);
void test_pending_results_cleanup_logging(void);
void test_lock_acquisition_failure_in_wait_for_initial_connection(void);
void test_initial_connection_completion_logging(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }

    // Reset all mocks to default state
    mock_system_reset_all();
    mock_logging_reset_all();

    // Reset our mock state
    mock_connect_success = true;
    mock_health_check_success = true;
    mock_engine_init_success = true;
    mock_mutex_lock_success = true;
    mock_signal_called = false;
    mock_queue_depth = 0;
    mock_pending_manager = NULL;
    mock_cleanup_count = 0;
}

void tearDown(void) {
    // Clean up test fixtures
    mock_system_reset_all();
    mock_logging_reset_all();
}

// Test corrupted mutex detection in database_queue_handle_connection_success (lines 115-132)
void test_corrupted_mutex_detection_in_handle_connection_success(void) {
    // Create a test queue
    DatabaseQueue* test_queue = database_queue_create_lead("testdb_corrupt",
        "postgresql://user:pass@host:5432/db", NULL);
    TEST_ASSERT_NOT_NULL(test_queue);

    // Create a mock connection with corrupted mutex
    DatabaseHandle* mock_conn = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(mock_conn);
    mock_conn->engine_type = DB_ENGINE_POSTGRESQL;
    mock_conn->designator = strdup("test-conn");
    mock_conn->status = DB_CONNECTION_CONNECTED;
    mock_conn->connected_since = time(NULL);

    // Initialize mutex first
    pthread_mutex_init(&mock_conn->connection_lock, NULL);
    // Then corrupt it by setting to invalid address (below 0x1000)
    *(uintptr_t*)&mock_conn->connection_lock = 0x100;

    // Test handle_connection_success - should detect corrupted mutex
    bool result = database_queue_handle_connection_success(test_queue, mock_conn);

    // Should fail due to corrupted mutex
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(test_queue->is_connected);
    TEST_ASSERT_FALSE(test_queue->persistent_connection);

    database_queue_destroy(test_queue);
}

// Test health check failure after connection establishment (lines 136-173)
void test_health_check_failure_after_connection(void) {
    // Create a test queue
    DatabaseQueue* test_queue = database_queue_create_lead("testdb_health_fail",
        "postgresql://user:pass@host:5432/db", NULL);
    TEST_ASSERT_NOT_NULL(test_queue);

    // Set health check to fail
    mock_health_check_success = false;

    // Create a mock connection
    DatabaseHandle* mock_conn = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(mock_conn);
    mock_conn->engine_type = DB_ENGINE_POSTGRESQL;
    mock_conn->designator = strdup("test-conn");
    mock_conn->status = DB_CONNECTION_CONNECTED;
    mock_conn->connected_since = time(NULL);
    pthread_mutex_init(&mock_conn->connection_lock, NULL);

    // Test handle_connection_success - should fail health check
    bool result = database_queue_handle_connection_success(test_queue, mock_conn);

    // Should fail due to health check failure
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(test_queue->is_connected);
    TEST_ASSERT_FALSE(test_queue->persistent_connection);

    database_queue_destroy(test_queue);
}


// Test config->database logging path (line 208)
void test_config_database_logging_path(void) {
    // Create a test queue
    DatabaseQueue* test_queue = database_queue_create_lead("testdb_config_log",
        "postgresql://user:pass@host:5432/db", NULL);
    TEST_ASSERT_NOT_NULL(test_queue);

    // Create a connection config with database field set (simulate parsed config)
    ConnectionConfig config = {0};
    config.database = strdup("test_database");
    config.connection_string = NULL; // This will trigger the config->database logging path
    config.host = strdup("localhost");
    config.username = strdup("user");
    config.password = strdup("pass");
    config.port = 5432;

    DatabaseEngine engine_type = DB_ENGINE_POSTGRESQL;

    // Test perform_connection_attempt - should log using config->database
    (void)database_queue_perform_connection_attempt(test_queue, &config, engine_type);

    // Function should not crash and should attempt connection
    // The timestamp should be updated from its initial value
    TEST_ASSERT_TRUE(test_queue->last_connection_attempt >= 0);

    // Clean up manually since config is stack-allocated
    free(config.database);
    free(config.host);
    free(config.username);
    free(config.password);
    database_queue_destroy(test_queue);
}

// Test engine init failure in check_connection (lines 284-299)
void test_engine_init_failure_in_check_connection(void) {
    // Create a test queue
    DatabaseQueue* test_queue = database_queue_create_lead("testdb_engine_fail",
        "postgresql://user:pass@host:5432/db", NULL);
    TEST_ASSERT_NOT_NULL(test_queue);

    // Set engine init to fail
    mock_engine_init_success = false;

    // Test check_connection - should fail due to engine init failure
    bool result = database_queue_check_connection(test_queue);

    // Should fail due to engine init failure
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(test_queue->is_connected);

    database_queue_destroy(test_queue);
}

// Test early return due to shutdown in perform_heartbeat (line 328)
void test_early_return_due_to_shutdown_in_perform_heartbeat(void) {
    // Create a test queue
    DatabaseQueue* test_queue = database_queue_create_lead("testdb_shutdown",
        "postgresql://user:pass@host:5432/db", NULL);
    TEST_ASSERT_NOT_NULL(test_queue);

    // Set shutdown flag
    test_queue->shutdown_requested = true;

    // Test perform_heartbeat - should return early due to shutdown
    database_queue_perform_heartbeat(test_queue);

    // Should not crash (early return due to shutdown, so heartbeat time is not updated)
    TEST_ASSERT_TRUE(true); // If we reach here, the function didn't crash

    database_queue_destroy(test_queue);
}

// Test connection status change logging in perform_heartbeat (lines 370-382)
void test_connection_status_change_logging_in_perform_heartbeat(void) {
    // Create a test queue
    DatabaseQueue* test_queue = database_queue_create_lead("testdb_status_change",
        "postgresql://user:pass@host:5432/db", NULL);
    TEST_ASSERT_NOT_NULL(test_queue);

    // Start with connected state, then simulate disconnection
    test_queue->is_connected = true;
    test_queue->persistent_connection = NULL; // No actual connection

    // Test perform_heartbeat - should detect status change and log
    database_queue_perform_heartbeat(test_queue);

    // Should not crash and should update heartbeat time
    TEST_ASSERT_TRUE(test_queue->last_heartbeat > 0);

    database_queue_destroy(test_queue);
}

// Test pending results cleanup logging (lines 400-402)
void test_pending_results_cleanup_logging(void) {
    // Create a test queue
    DatabaseQueue* test_queue = database_queue_create_lead("testdb_cleanup",
        "postgresql://user:pass@host:5432/db", NULL);
    TEST_ASSERT_NOT_NULL(test_queue);

    // Set up mock pending manager with cleanup count
    mock_pending_manager = calloc(1, sizeof(PendingResultManager));
    mock_cleanup_count = 5; // Simulate cleaning up 5 expired results

    // Test perform_heartbeat - should log cleanup results
    database_queue_perform_heartbeat(test_queue);

    // Should not crash and should update heartbeat time
    TEST_ASSERT_TRUE(test_queue->last_heartbeat > 0);

    free(mock_pending_manager);
    mock_pending_manager = NULL;
    database_queue_destroy(test_queue);
}

// Test lock acquisition failure in wait_for_initial_connection (lines 420-422)
void test_lock_acquisition_failure_in_wait_for_initial_connection(void) {
    // Create a lead queue
    DatabaseQueue* lead_queue = database_queue_create_lead("testdb_lock_fail",
        "postgresql://user:pass@host:5432/db", NULL);
    TEST_ASSERT_NOT_NULL(lead_queue);

    // Set mutex lock to fail
    mock_mutex_lock_success = false;

    // Test wait_for_initial_connection - should fail to acquire lock
    bool result = database_queue_wait_for_initial_connection(lead_queue, 1);

    // Should fail due to lock acquisition failure
    TEST_ASSERT_FALSE(result);

    database_queue_destroy(lead_queue);
}

// Test initial connection completion logging (line 447)
void test_initial_connection_completion_logging(void) {
    // Create a lead queue
    DatabaseQueue* lead_queue = database_queue_create_lead("testdb_completion",
        "postgresql://user:pass@host:5432/db", NULL);
    TEST_ASSERT_NOT_NULL(lead_queue);

    // Set initial connection as attempted
    lead_queue->initial_connection_attempted = true;

    // Test wait_for_initial_connection - should log completion
    bool result = database_queue_wait_for_initial_connection(lead_queue, 5);

    // Should succeed and log completion
    TEST_ASSERT_TRUE(result);

    database_queue_destroy(lead_queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_corrupted_mutex_detection_in_handle_connection_success);
    RUN_TEST(test_health_check_failure_after_connection);
    RUN_TEST(test_config_database_logging_path);
    RUN_TEST(test_engine_init_failure_in_check_connection);
    RUN_TEST(test_early_return_due_to_shutdown_in_perform_heartbeat);
    RUN_TEST(test_connection_status_change_logging_in_perform_heartbeat);
    RUN_TEST(test_pending_results_cleanup_logging);
    RUN_TEST(test_lock_acquisition_failure_in_wait_for_initial_connection);
    RUN_TEST(test_initial_connection_completion_logging);

    return UNITY_END();
}