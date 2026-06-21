/*
 * Unity Test File: Authenticated Multi-Model Chat API endpoint
 * This file contains unit tests for handle_auth_chats_request() in
 * src/api/wschat/auth_chats/auth_chats.c
 *
 * These tests drive the error/early-return paths of the handler so that the
 * REAL api_send_json_response() runs (it is intentionally not mocked). That
 * function takes ownership of its json_obj argument and decrefs it internally.
 * The handler must therefore NOT decref the same object again. Running these
 * tests under the AddressSanitizer (debug) build verifies there is no
 * heap-use-after-free / double-free on any of these response paths.
 *
 * CHANGELOG:
 * 2026-06-20: Initial implementation covering response ownership / decref paths
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/wschat/auth_chats/auth_chats.h>

// Mock includes
#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_API_UTILS
#define USE_MOCK_AUTH_SERVICE_JWT
#define USE_MOCK_DBQUEUE
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_api_utils.h>
#include <unity/mocks/mock_auth_service_jwt.h>
#include <unity/mocks/mock_dbqueue.h>

// External global variable (defined in dbqueue.h)
extern DatabaseQueueManager* global_queue_manager;

// Test function prototypes
void test_handle_auth_chats_request_invalid_method(void);
void test_handle_auth_chats_request_no_auth(void);
void test_handle_auth_chats_request_invalid_json(void);
void test_handle_auth_chats_request_missing_engines(void);
void test_handle_auth_chats_request_database_not_found(void);

void setUp(void) {
    // Reset all mocks before each test
    mock_mhd_reset_all();
    mock_api_utils_reset_all();
    mock_auth_service_jwt_reset_all();
    mock_dbqueue_reset_all();

    // Default behaviors: no Authorization header, JWT invalid, no database
    mock_mhd_set_lookup_result(NULL);
    mock_auth_service_jwt_set_validation_result((jwt_validation_result_t){false, NULL, JWT_ERROR_NONE});
    mock_dbqueue_set_get_database_result(NULL);

    // POST buffering completes immediately by default
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);

    global_queue_manager = NULL;
}

void tearDown(void) {
    global_queue_manager = NULL;
}

// Helper to build a valid JWT validation result whose claims carry a database
static jwt_validation_result_t make_valid_jwt_with_database(jwt_claims_t* claims_storage,
                                                            const char* database) {
    memset(claims_storage, 0, sizeof(jwt_claims_t));
    claims_storage->database = (char*)database;  // mock_set copies via strdup
    claims_storage->user_id = 42;
    jwt_validation_result_t result = {true, claims_storage, JWT_ERROR_NONE};
    return result;
}

// GET instead of POST -> "Method not allowed" response path.
void test_handle_auth_chats_request_invalid_method(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    enum MHD_Result result = handle_auth_chats_request(mock_connection,
                                                       "/api/conduit/auth_chats",
                                                       "GET", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// POST with no/invalid auth -> "Authentication required" response path.
void test_handle_auth_chats_request_no_auth(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    enum MHD_Result result = handle_auth_chats_request(mock_connection,
                                                       "/api/conduit/auth_chats",
                                                       "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// POST with valid JWT but non-JSON body -> "Invalid JSON" response path.
void test_handle_auth_chats_request_invalid_json(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_mhd_set_lookup_result("Bearer testtoken");
    jwt_claims_t claims_storage;
    mock_auth_service_jwt_set_validation_result(
        make_valid_jwt_with_database(&claims_storage, "test_db"));

    mock_api_utils_set_buffer_data("not valid json");

    enum MHD_Result result = handle_auth_chats_request(mock_connection,
                                                       "/api/conduit/auth_chats",
                                                       "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// POST with valid JWT and valid JSON but missing 'engines' array ->
// "Missing or invalid 'engines' array" response path.
void test_handle_auth_chats_request_missing_engines(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_mhd_set_lookup_result("Bearer testtoken");
    jwt_claims_t claims_storage;
    mock_auth_service_jwt_set_validation_result(
        make_valid_jwt_with_database(&claims_storage, "test_db"));

    // Valid JSON object but no "engines" array
    mock_api_utils_set_buffer_data("{\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}]}");

    enum MHD_Result result = handle_auth_chats_request(mock_connection,
                                                       "/api/conduit/auth_chats",
                                                       "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// POST with valid JWT, valid engines + messages, but no database queue ->
// "Database not found" response path.
void test_handle_auth_chats_request_database_not_found(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_mhd_set_lookup_result("Bearer testtoken");
    jwt_claims_t claims_storage;
    mock_auth_service_jwt_set_validation_result(
        make_valid_jwt_with_database(&claims_storage, "test_db"));

    mock_api_utils_set_buffer_data(
        "{\"engines\":[\"gpt-4\"],\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}]}");

    mock_dbqueue_set_get_database_result(NULL);

    enum MHD_Result result = handle_auth_chats_request(mock_connection,
                                                       "/api/conduit/auth_chats",
                                                       "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_auth_chats_request_invalid_method);
    RUN_TEST(test_handle_auth_chats_request_no_auth);
    RUN_TEST(test_handle_auth_chats_request_invalid_json);
    RUN_TEST(test_handle_auth_chats_request_missing_engines);
    RUN_TEST(test_handle_auth_chats_request_database_not_found);

    return UNITY_END();
}
