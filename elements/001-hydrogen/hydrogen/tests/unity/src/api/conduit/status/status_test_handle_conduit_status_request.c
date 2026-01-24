/*
 * Unity Test File: Conduit Status API endpoint
 * This file contains unit tests for the status endpoint in
 * src/api/conduit/status/status.c
 *
 * CHANGELOG:
 * 2026-01-24: Initial implementation with basic test coverage
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/conduit/status/status.h>

// Mock includes
#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_API_UTILS
#define USE_MOCK_AUTH_SERVICE_JWT
#define USE_MOCK_DBQUEUE
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_api_utils.h>
#include <unity/mocks/mock_auth_service_jwt.h>
#include <unity/mocks/mock_dbqueue.h>

// Local mock for api_send_json_response
enum MHD_Result mock_api_send_json_response(struct MHD_Connection *connection, json_t *json_obj, unsigned int status_code);

// Mock implementation
enum MHD_Result mock_api_send_json_response(struct MHD_Connection *connection, json_t *json_obj, unsigned int status_code) {
    (void)connection;
    (void)json_obj;
    (void)status_code;
    return MHD_YES; // Always succeed for tests
}

// External global variable (defined in dbqueue.h)
extern DatabaseQueueManager* global_queue_manager;

// Test function prototypes
void test_handle_conduit_status_request_invalid_method(void);
void test_handle_conduit_status_request_no_queue_manager(void);
void test_handle_conduit_status_request_empty_databases(void);
void test_handle_conduit_status_request_database_ready_no_jwt(void);

void setUp(void) {
    // Reset all mocks before each test
    mock_mhd_reset_all();
    mock_api_utils_reset_all();
    mock_auth_service_jwt_reset_all();
    mock_dbqueue_reset_all();

    // Set default mock behaviors
    mock_mhd_set_lookup_result(NULL);  // No Authorization header by default
    mock_auth_service_jwt_set_validation_result((jwt_validation_result_t){false, NULL, JWT_ERROR_NONE});
    mock_dbqueue_set_get_database_result(NULL);

    // Clear global queue manager
    global_queue_manager = NULL;
}

void tearDown(void) {
    // Clean up after each test
    global_queue_manager = NULL;
}

// Test invalid HTTP method (POST instead of GET)
void test_handle_conduit_status_request_invalid_method(void) {
    // Setup
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;  // Mock pointer
    global_queue_manager = calloc(1, sizeof(DatabaseQueueManager));
    TEST_ASSERT_NOT_NULL(global_queue_manager);
    if (global_queue_manager) {
        global_queue_manager->max_databases = 0;

        // Test
        enum MHD_Result result = handle_conduit_status_request(mock_connection, "/api/conduit/status", "POST", NULL, NULL, NULL);

        // Assert
        TEST_ASSERT_EQUAL(MHD_NO, result);

        // Cleanup
        free(global_queue_manager);
    }
}

// Test missing queue manager
void test_handle_conduit_status_request_no_queue_manager(void) {
    // Setup
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;  // Mock pointer
    global_queue_manager = NULL;  // Already NULL from setUp

    // Test
    enum MHD_Result result = handle_conduit_status_request(mock_connection, "/api/conduit/status", "GET", NULL, NULL, NULL);

    // Assert
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test empty database manager (no databases)
void test_handle_conduit_status_request_empty_databases(void) {
    // Setup
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;  // Mock pointer
    global_queue_manager = calloc(1, sizeof(DatabaseQueueManager));
    TEST_ASSERT_NOT_NULL(global_queue_manager);
    if (global_queue_manager) {
        global_queue_manager->max_databases = 0;

        // Test
        enum MHD_Result result = handle_conduit_status_request(mock_connection, "/api/conduit/status", "GET", NULL, NULL, NULL);

        // Assert
        TEST_ASSERT_EQUAL(MHD_YES, result);

        // Cleanup
        free(global_queue_manager);
    }
}

// Test database ready - no JWT authentication
void test_handle_conduit_status_request_database_ready_no_jwt(void) {
    // Setup
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;  // Mock pointer
    global_queue_manager = calloc(1, sizeof(DatabaseQueueManager));
    TEST_ASSERT_NOT_NULL(global_queue_manager);
    if (global_queue_manager) {
        global_queue_manager->max_databases = 1;
        global_queue_manager->databases = calloc(1, sizeof(DatabaseQueue*));
        TEST_ASSERT_NOT_NULL(global_queue_manager->databases);
        if (global_queue_manager->databases) {
            DatabaseQueue* db_queue = calloc(1, sizeof(DatabaseQueue));
            TEST_ASSERT_NOT_NULL(db_queue);
            if (db_queue) {
                db_queue->database_name = (char*)"test_db";  // Cast to avoid const qualifier warning
                db_queue->bootstrap_completed = true;

                QueryTableCache* cache = calloc(1, sizeof(QueryTableCache));
                TEST_ASSERT_NOT_NULL(cache);
                if (cache) {
                    cache->entry_count = 5;  // Has entries
                    db_queue->query_cache = cache;
                    global_queue_manager->databases[0] = db_queue;

                    // No JWT header (default from setUp)

                    // Test
                    enum MHD_Result result = handle_conduit_status_request(mock_connection, "/api/conduit/status", "GET", NULL, NULL, NULL);

                    // Assert
                    TEST_ASSERT_EQUAL(MHD_YES, result);

                    // Cleanup
                    free(cache);
                }
                free(db_queue);
            }
            free(global_queue_manager->databases);
        }
        free(global_queue_manager);
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_conduit_status_request_invalid_method);
    RUN_TEST(test_handle_conduit_status_request_no_queue_manager);
    RUN_TEST(test_handle_conduit_status_request_empty_databases);
    RUN_TEST(test_handle_conduit_status_request_database_ready_no_jwt);

    return UNITY_END();
}