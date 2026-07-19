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
 * 2026-07-18: Added coverage for buffer-error, parse error, stream-not-
 *             implemented, missing CEC, missing/named engine, unhealthy engine
 *             and no-default-engine early-return branches
 *
 * TEST_VERSION: 1.1.0
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

// Real engine cache code is linked so we can build a populated DatabaseQueue
// to exercise the engine/cec handling branches.
#include <src/api/wschat/helpers/engine_cache.h>

// External global variable (defined in dbqueue.h)
extern DatabaseQueueManager* global_queue_manager;

// Test function prototypes
void test_handle_auth_chat_request_invalid_method(void);
void test_handle_auth_chat_request_no_auth(void);
void test_handle_auth_chat_request_buffer_error(void);
void test_handle_auth_chat_request_invalid_json(void);
void test_handle_auth_chat_request_parse_error(void);
void test_handle_auth_chat_request_database_not_found(void);
void test_handle_auth_chat_request_stream_not_implemented(void);
void test_handle_auth_chat_request_cec_missing(void);
void test_handle_auth_chat_request_engine_missing(void);
void test_handle_auth_chat_request_engine_unhealthy(void);
void test_handle_auth_chat_request_no_default_engine(void);

// Build a DatabaseQueue carrying a freshly created engine cache with a single
// healthy engine. The caller owns the returned DatabaseQueue and the cache.
static DatabaseQueue* make_db_queue_with_engine(bool healthy, bool make_default) {
    DatabaseQueue* dq = calloc(1, sizeof(DatabaseQueue));
    if (!dq) return NULL;
    dq->database_name = strdup("test_db");

    ChatEngineCache* cec = chat_engine_cache_create("test_db");
    ChatEngineConfig* engine = chat_engine_config_create(
        1, "gpt-4", CEC_PROVIDER_OPENAI, "gpt-4-turbo",
        "https://api.example.com", "key", 1000, 0.7, make_default,
        0, 4, 8, 8, MODALITY_TEXT, false);
    engine->is_healthy = healthy;
    chat_engine_cache_add_engine(cec, engine);
    dq->chat_engine_cache = cec;
    return dq;
}

static void free_db_queue_with_engine(DatabaseQueue* dq) {
    if (!dq) return;
    if (dq->chat_engine_cache) {
        chat_engine_cache_destroy(dq->chat_engine_cache);
    }
    free(dq->database_name);
    free(dq);
}

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
// Exercises api_send_json_response(error) on the JWT-failure branch
// (jwt_result.claims is NULL -> "Authentication required").
void test_handle_auth_chat_request_no_auth(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    enum MHD_Result result = handle_auth_chat_request(mock_connection,
                                                      "/api/conduit/auth_chat",
                                                      "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// POST body buffering reports an error -> API_BUFFER_ERROR branch
// (api_send_error_and_cleanup). Exercises the very first early-return.
void test_handle_auth_chat_request_buffer_error(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_api_utils_set_buffer_result(API_BUFFER_ERROR);

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

// Valid JWT + valid parse, no engine requested, and cache has no default
// engine -> "No default engine configured" response path.
void test_handle_auth_chat_request_no_default_engine(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_mhd_set_lookup_result("Bearer testtoken");
    jwt_claims_t claims_storage;
    mock_auth_service_jwt_set_validation_result(
        make_valid_jwt_with_database(&claims_storage, "test_db"));

    mock_api_utils_set_buffer_data(
        "{\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}]}");

    // Engine present but NOT marked default; no engine name in request ->
    // chat_engine_cache_get_default() returns NULL.
    DatabaseQueue* dq = make_db_queue_with_engine(true, false);
    mock_dbqueue_set_get_database_result(dq);

    enum MHD_Result result = handle_auth_chat_request(mock_connection,
                                                      "/api/conduit/auth_chat",
                                                      "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    free_db_queue_with_engine(dq);
}

// Valid JWT + valid JSON, but request parsing reports an error (e.g. missing
// messages) -> "Invalid request" response path.
void test_handle_auth_chat_request_parse_error(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_mhd_set_lookup_result("Bearer testtoken");
    jwt_claims_t claims_storage;
    mock_auth_service_jwt_set_validation_result(
        make_valid_jwt_with_database(&claims_storage, "test_db"));

    // Valid JSON object but no "messages" array -> parse fails.
    mock_api_utils_set_buffer_data("{\"engine\":\"gpt-4\"}");

    enum MHD_Result result = handle_auth_chat_request(mock_connection,
                                                      "/api/conduit/auth_chat",
                                                      "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Valid JWT + valid parse, but stream=true -> "Streaming not yet implemented".
void test_handle_auth_chat_request_stream_not_implemented(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_mhd_set_lookup_result("Bearer testtoken");
    jwt_claims_t claims_storage;
    mock_auth_service_jwt_set_validation_result(
        make_valid_jwt_with_database(&claims_storage, "test_db"));

    mock_api_utils_set_buffer_data(
        "{\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}],\"stream\":true}");

    DatabaseQueue* dq = make_db_queue_with_engine(true, true);
    mock_dbqueue_set_get_database_result(dq);

    enum MHD_Result result = handle_auth_chat_request(mock_connection,
                                                      "/api/conduit/auth_chat",
                                                      "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    free_db_queue_with_engine(dq);
}

// Database queue exists but has no ChatEngineCache -> "Chat not enabled".
void test_handle_auth_chat_request_cec_missing(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_mhd_set_lookup_result("Bearer testtoken");
    jwt_claims_t claims_storage;
    mock_auth_service_jwt_set_validation_result(
        make_valid_jwt_with_database(&claims_storage, "test_db"));

    mock_api_utils_set_buffer_data(
        "{\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}],\"engine\":\"gpt-4\"}");

    DatabaseQueue* dq = calloc(1, sizeof(DatabaseQueue));
    if (dq) {
        dq->database_name = strdup("test_db");
        dq->chat_engine_cache = NULL;
        mock_dbqueue_set_get_database_result(dq);
    }

    enum MHD_Result result = handle_auth_chat_request(mock_connection,
                                                      "/api/conduit/auth_chat",
                                                      "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    free_db_queue_with_engine(dq);
}

// Cache present but requested engine name not found -> "Engine not found".
void test_handle_auth_chat_request_engine_missing(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_mhd_set_lookup_result("Bearer testtoken");
    jwt_claims_t claims_storage;
    mock_auth_service_jwt_set_validation_result(
        make_valid_jwt_with_database(&claims_storage, "test_db"));

    mock_api_utils_set_buffer_data(
        "{\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}],\"engine\":\"does-not-exist\"}");

    DatabaseQueue* dq = make_db_queue_with_engine(true, true);
    mock_dbqueue_set_get_database_result(dq);

    enum MHD_Result result = handle_auth_chat_request(mock_connection,
                                                      "/api/conduit/auth_chat",
                                                      "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    free_db_queue_with_engine(dq);
}

// Cache present, default engine exists, but it is unhealthy ->
// "Engine is currently unavailable".
void test_handle_auth_chat_request_engine_unhealthy(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_mhd_set_lookup_result("Bearer testtoken");
    jwt_claims_t claims_storage;
    mock_auth_service_jwt_set_validation_result(
        make_valid_jwt_with_database(&claims_storage, "test_db"));

    mock_api_utils_set_buffer_data(
        "{\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}]}");

    DatabaseQueue* dq = make_db_queue_with_engine(false, true);
    mock_dbqueue_set_get_database_result(dq);

    enum MHD_Result result = handle_auth_chat_request(mock_connection,
                                                      "/api/conduit/auth_chat",
                                                      "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    free_db_queue_with_engine(dq);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_auth_chat_request_invalid_method);
    RUN_TEST(test_handle_auth_chat_request_no_auth);
    RUN_TEST(test_handle_auth_chat_request_buffer_error);
    RUN_TEST(test_handle_auth_chat_request_invalid_json);
    RUN_TEST(test_handle_auth_chat_request_parse_error);
    RUN_TEST(test_handle_auth_chat_request_database_not_found);
    RUN_TEST(test_handle_auth_chat_request_stream_not_implemented);
    RUN_TEST(test_handle_auth_chat_request_cec_missing);
    RUN_TEST(test_handle_auth_chat_request_engine_missing);
    RUN_TEST(test_handle_auth_chat_request_engine_unhealthy);
    RUN_TEST(test_handle_auth_chat_request_no_default_engine);

    return UNITY_END();
}
