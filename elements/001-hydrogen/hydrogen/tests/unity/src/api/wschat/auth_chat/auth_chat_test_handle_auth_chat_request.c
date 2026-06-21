/*
 * Unity Test File: Authenticated Chat API endpoint
 * This file contains unit tests for handle_auth_chat_request() in
 * src/api/wschat/auth_chat/auth_chat.c
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
#include <src/api/wschat/auth_chat/auth_chat.h>

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
void test_handle_auth_chat_request_invalid_method(void);
void test_handle_auth_chat_request_no_auth(void);
void test_handle_auth_chat_request_invalid_json(void);
void test_handle_auth_chat_request_database_not_found(void);

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
// Exercises api_send_json_response(error) with no JWT involved.
void test_handle_auth_chat_request_invalid_method(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    enum MHD_Result result = handle_auth_chat_request(mock_connection,
                                                      "/api/conduit/auth_chat",
                                                      "GET", NULL, &upload_size, &con_cls);

    // The mocked api_send_json_response path is not used; the real one returns
    // MHD_YES via the mocked MHD_queue_response. No double-free should occur.
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// POST with no/invalid auth -> "Authentication required" response path.
// Exercises api_send_json_response(error) on the JWT-failure branch.
void test_handle_auth_chat_request_no_auth(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    enum MHD_Result result = handle_auth_chat_request(mock_connection,
                                                      "/api/conduit/auth_chat",
                                                      "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// POST with valid JWT (database claim present) but a non-JSON body ->
// "Invalid JSON in request body" response path.
// Exercises api_send_json_response(error_response).
void test_handle_auth_chat_request_invalid_json(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Provide a valid JWT carrying a database claim
    mock_mhd_set_lookup_result("Bearer testtoken");
    jwt_claims_t claims_storage;
    mock_auth_service_jwt_set_validation_result(
        make_valid_jwt_with_database(&claims_storage, "test_db"));

    // Provide a body that is not valid JSON
    mock_api_utils_set_buffer_data("this is not json");

    enum MHD_Result result = handle_auth_chat_request(mock_connection,
                                                      "/api/conduit/auth_chat",
                                                      "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// POST with valid JWT and valid JSON but no database queue ->
// "Database not found" response path.
// Exercises api_send_json_response(error_response) after request parsing.
void test_handle_auth_chat_request_database_not_found(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_mhd_set_lookup_result("Bearer testtoken");
    jwt_claims_t claims_storage;
    mock_auth_service_jwt_set_validation_result(
        make_valid_jwt_with_database(&claims_storage, "test_db"));

    // Valid JSON with a minimal messages array so parsing succeeds
    mock_api_utils_set_buffer_data("{\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}]}");

    // No database queue available
    mock_dbqueue_set_get_database_result(NULL);

    enum MHD_Result result = handle_auth_chat_request(mock_connection,
                                                      "/api/conduit/auth_chat",
                                                      "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_auth_chat_request_invalid_method);
    RUN_TEST(test_handle_auth_chat_request_no_auth);
    RUN_TEST(test_handle_auth_chat_request_invalid_json);
    RUN_TEST(test_handle_auth_chat_request_database_not_found);

    return UNITY_END();
}
