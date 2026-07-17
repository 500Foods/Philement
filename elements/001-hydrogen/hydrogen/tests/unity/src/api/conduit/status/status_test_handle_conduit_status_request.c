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
#include <src/api/wschat/helpers/engine_cache.h>

// Mock includes
#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_API_UTILS
#define USE_MOCK_DBQUEUE
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_api_utils.h>
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
void test_handle_conduit_status_request_with_chat_models(void);
void test_handle_conduit_status_request_jwt_denied_not_bearer(void);
void test_handle_conduit_status_request_jwt_empty_token(void);

void setUp(void) {
    // Reset all mocks before each test
    mock_mhd_reset_all();
    mock_api_utils_reset_all();
    mock_dbqueue_reset_all();

    // Set default mock behaviors
    mock_mhd_set_lookup_result(NULL);  // No Authorization header by default
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

// Test database with chat enabled - models array should be present when authenticated
void test_handle_conduit_status_request_with_chat_models(void) {
    // Setup
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
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
                db_queue->database_name = (char*)"test_db";
                db_queue->bootstrap_completed = true;

                QueryTableCache* cache = calloc(1, sizeof(QueryTableCache));
                TEST_ASSERT_NOT_NULL(cache);
                if (cache) {
                    cache->entry_count = 5;
                    db_queue->query_cache = cache;

                     // Create CEC with one engine (models block is covered by
                     // status_test_build_models; here we just ensure the
                     // non-authenticated path with a CEC present succeeds).
                     ChatEngineCache* cec = chat_engine_cache_create("test_db");
                     TEST_ASSERT_NOT_NULL(cec);
                     if (cec) {
                         ChatEngineConfig* engine = chat_engine_config_create(
                             1, "gpt4", CEC_PROVIDER_OPENAI, "gpt-4-turbo",
                             "https://api.openai.com/v1/chat/completions", "sk-test",
                             4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);
                         TEST_ASSERT_NOT_NULL(engine);
                         if (engine) {
                             chat_engine_cache_add_engine(cec, engine);
                             db_queue->chat_engine_cache = cec;

                             global_queue_manager->databases[0] = db_queue;

                             // Test
                             enum MHD_Result result = handle_conduit_status_request(mock_connection, "/api/conduit/status", "GET", NULL, NULL, NULL);

                             // Assert
                             TEST_ASSERT_EQUAL(MHD_YES, result);

                             chat_engine_cache_destroy(cec);
                         }
                     }
                    free(cache);
                }
                free(db_queue);
            }
            free(global_queue_manager->databases);
        }
        free(global_queue_manager);
    }
}

// Test authenticated request where Authorization header is present but not Bearer
// format -> JWT validation skipped, models block not exercised, request still succeeds.
void test_handle_conduit_status_request_jwt_denied_not_bearer(void) {
    // Setup
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    // Header present but lacking "Bearer " prefix
    mock_mhd_set_lookup_result("Basic dXNlcjpwYXNz");

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
                db_queue->database_name = (char*)"test_db";
                db_queue->bootstrap_completed = true;

                QueryTableCache* cache = calloc(1, sizeof(QueryTableCache));
                TEST_ASSERT_NOT_NULL(cache);
                if (cache) {
                    cache->entry_count = 5;
                    db_queue->query_cache = cache;
                    global_queue_manager->databases[0] = db_queue;

                    // Test - has_jwt should be false (no valid Bearer token)
                    enum MHD_Result result = handle_conduit_status_request(mock_connection, "/api/conduit/status", "GET", NULL, NULL, NULL);

                    // Assert
                    TEST_ASSERT_EQUAL(MHD_YES, result);

                    free(cache);
                }
                free(db_queue);
            }
            free(global_queue_manager->databases);
        }
        free(global_queue_manager);
    }
}

// Test authenticated request where Authorization header is "Bearer " with an
// empty token -> JWT validation skipped, request still succeeds.
void test_handle_conduit_status_request_jwt_empty_token(void) {
    // Setup
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    // "Bearer " prefix with no token following
    mock_mhd_set_lookup_result("Bearer ");

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
                db_queue->database_name = (char*)"test_db";
                db_queue->bootstrap_completed = true;

                QueryTableCache* cache = calloc(1, sizeof(QueryTableCache));
                TEST_ASSERT_NOT_NULL(cache);
                if (cache) {
                    cache->entry_count = 5;
                    db_queue->query_cache = cache;
                    global_queue_manager->databases[0] = db_queue;

                    // Test - has_jwt should be false (empty token)
                    enum MHD_Result result = handle_conduit_status_request(mock_connection, "/api/conduit/status", "GET", NULL, NULL, NULL);

                    // Assert
                    TEST_ASSERT_EQUAL(MHD_YES, result);

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
    RUN_TEST(test_handle_conduit_status_request_jwt_denied_not_bearer);
    RUN_TEST(test_handle_conduit_status_request_jwt_empty_token);
    RUN_TEST(test_handle_conduit_status_request_with_chat_models);

    return UNITY_END();
}