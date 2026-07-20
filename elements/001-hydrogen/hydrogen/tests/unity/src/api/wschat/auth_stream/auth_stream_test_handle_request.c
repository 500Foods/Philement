/*
 * Unity Test File: handle_auth_chat_stream_request
 * This file contains unit tests for handle_auth_chat_stream_request() in
 * src/api/wschat/auth_stream/auth_stream.c
 *
 * Drives error/early-return paths and the stub streaming success path.
 *
 * CHANGELOG:
 * 2026-07-19: Initial implementation covering handler branches
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/wschat/auth_stream/auth_stream.h>

// Mock includes
#define USE_MOCK_API_UTILS
#define USE_MOCK_AUTH_SERVICE_JWT
#define USE_MOCK_DBQUEUE
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_api_utils.h>
#include <unity/mocks/mock_auth_service_jwt.h>
#include <unity/mocks/mock_dbqueue.h>

#include <src/api/wschat/helpers/engine_cache.h>

extern DatabaseQueueManager *global_queue_manager;

// Test function prototypes
void test_handle_auth_chat_stream_request_buffer_continue(void);
void test_handle_auth_chat_stream_request_buffer_error(void);
void test_handle_auth_chat_stream_request_invalid_method(void);
void test_handle_auth_chat_stream_request_no_auth(void);
void test_handle_auth_chat_stream_request_invalid_json(void);
void test_handle_auth_chat_stream_request_parse_error(void);
void test_handle_auth_chat_stream_request_database_not_found(void);
void test_handle_auth_chat_stream_request_cec_missing(void);
void test_handle_auth_chat_stream_request_engine_missing(void);
void test_handle_auth_chat_stream_request_no_default_engine(void);
void test_handle_auth_chat_stream_request_engine_unhealthy(void);
void test_handle_auth_chat_stream_request_stream_stub_success(void);

static DatabaseQueue *make_db_queue_with_engine(bool healthy, bool make_default) {
    DatabaseQueue *dq = calloc(1, sizeof(DatabaseQueue));
    if (!dq) return NULL;
    dq->database_name = strdup("test_db");

    ChatEngineCache *cec = chat_engine_cache_create("test_db");
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "gpt-4", CEC_PROVIDER_OPENAI, "gpt-4-turbo",
        "https://api.example.com", "key", 1000, 0.7, make_default,
        0, 4, 8, 8, MODALITY_TEXT, false);
    engine->is_healthy = healthy;
    chat_engine_cache_add_engine(cec, engine);
    dq->chat_engine_cache = cec;
    return dq;
}

static void free_db_queue_with_engine(DatabaseQueue *dq) {
    if (!dq) return;
    if (dq->chat_engine_cache) {
        chat_engine_cache_destroy(dq->chat_engine_cache);
    }
    free(dq->database_name);
    free(dq);
}

static jwt_validation_result_t make_valid_jwt_with_database(jwt_claims_t *claims_storage,
                                                            const char *database) {
    memset(claims_storage, 0, sizeof(jwt_claims_t));
    claims_storage->database = (char *)database;
    claims_storage->user_id = 42;
    jwt_validation_result_t result = {true, claims_storage, JWT_ERROR_NONE};
    return result;
}

void setUp(void) {
    mock_mhd_reset_all();
    mock_api_utils_reset_all();
    mock_auth_service_jwt_reset_all();
    mock_dbqueue_reset_all();

    mock_mhd_set_lookup_result(NULL);
    mock_auth_service_jwt_set_validation_result(
        (jwt_validation_result_t){false, NULL, JWT_ERROR_NONE});
    mock_dbqueue_set_get_database_result(NULL);
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);

    global_queue_manager = NULL;
}

void tearDown(void) {
    global_queue_manager = NULL;
}

void test_handle_auth_chat_stream_request_buffer_continue(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection *)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_api_utils_set_buffer_result(API_BUFFER_CONTINUE);

    enum MHD_Result result = handle_auth_chat_stream_request(
        mock_connection, "/api/conduit/auth_chat/stream",
        "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_auth_chat_stream_request_buffer_error(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection *)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_api_utils_set_buffer_result(API_BUFFER_ERROR);

    enum MHD_Result result = handle_auth_chat_stream_request(
        mock_connection, "/api/conduit/auth_chat/stream",
        "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_auth_chat_stream_request_invalid_method(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection *)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    enum MHD_Result result = handle_auth_chat_stream_request(
        mock_connection, "/api/conduit/auth_chat/stream",
        "GET", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_auth_chat_stream_request_no_auth(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection *)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    enum MHD_Result result = handle_auth_chat_stream_request(
        mock_connection, "/api/conduit/auth_chat/stream",
        "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_auth_chat_stream_request_invalid_json(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection *)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_mhd_set_lookup_result("Bearer testtoken");
    jwt_claims_t claims_storage;
    mock_auth_service_jwt_set_validation_result(
        make_valid_jwt_with_database(&claims_storage, "test_db"));
    mock_api_utils_set_buffer_data("this is not json");

    enum MHD_Result result = handle_auth_chat_stream_request(
        mock_connection, "/api/conduit/auth_chat/stream",
        "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_auth_chat_stream_request_parse_error(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection *)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_mhd_set_lookup_result("Bearer testtoken");
    jwt_claims_t claims_storage;
    mock_auth_service_jwt_set_validation_result(
        make_valid_jwt_with_database(&claims_storage, "test_db"));
    mock_api_utils_set_buffer_data("{\"engine\":\"gpt-4\"}");

    enum MHD_Result result = handle_auth_chat_stream_request(
        mock_connection, "/api/conduit/auth_chat/stream",
        "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_auth_chat_stream_request_database_not_found(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection *)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_mhd_set_lookup_result("Bearer testtoken");
    jwt_claims_t claims_storage;
    mock_auth_service_jwt_set_validation_result(
        make_valid_jwt_with_database(&claims_storage, "test_db"));
    mock_api_utils_set_buffer_data(
        "{\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}]}");
    mock_dbqueue_set_get_database_result(NULL);

    enum MHD_Result result = handle_auth_chat_stream_request(
        mock_connection, "/api/conduit/auth_chat/stream",
        "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_auth_chat_stream_request_cec_missing(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection *)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_mhd_set_lookup_result("Bearer testtoken");
    jwt_claims_t claims_storage;
    mock_auth_service_jwt_set_validation_result(
        make_valid_jwt_with_database(&claims_storage, "test_db"));
    mock_api_utils_set_buffer_data(
        "{\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}],\"engine\":\"gpt-4\"}");

    DatabaseQueue *dq = calloc(1, sizeof(DatabaseQueue));
    if (dq) {
        dq->database_name = strdup("test_db");
        dq->chat_engine_cache = NULL;
        mock_dbqueue_set_get_database_result(dq);
    }

    enum MHD_Result result = handle_auth_chat_stream_request(
        mock_connection, "/api/conduit/auth_chat/stream",
        "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    free_db_queue_with_engine(dq);
}

void test_handle_auth_chat_stream_request_engine_missing(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection *)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_mhd_set_lookup_result("Bearer testtoken");
    jwt_claims_t claims_storage;
    mock_auth_service_jwt_set_validation_result(
        make_valid_jwt_with_database(&claims_storage, "test_db"));
    mock_api_utils_set_buffer_data(
        "{\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}],\"engine\":\"does-not-exist\"}");

    DatabaseQueue *dq = make_db_queue_with_engine(true, true);
    mock_dbqueue_set_get_database_result(dq);

    enum MHD_Result result = handle_auth_chat_stream_request(
        mock_connection, "/api/conduit/auth_chat/stream",
        "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    free_db_queue_with_engine(dq);
}

void test_handle_auth_chat_stream_request_no_default_engine(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection *)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_mhd_set_lookup_result("Bearer testtoken");
    jwt_claims_t claims_storage;
    mock_auth_service_jwt_set_validation_result(
        make_valid_jwt_with_database(&claims_storage, "test_db"));
    mock_api_utils_set_buffer_data(
        "{\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}]}");

    DatabaseQueue *dq = make_db_queue_with_engine(true, false);
    mock_dbqueue_set_get_database_result(dq);

    enum MHD_Result result = handle_auth_chat_stream_request(
        mock_connection, "/api/conduit/auth_chat/stream",
        "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    free_db_queue_with_engine(dq);
}

void test_handle_auth_chat_stream_request_engine_unhealthy(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection *)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_mhd_set_lookup_result("Bearer testtoken");
    jwt_claims_t claims_storage;
    mock_auth_service_jwt_set_validation_result(
        make_valid_jwt_with_database(&claims_storage, "test_db"));
    mock_api_utils_set_buffer_data(
        "{\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}]}");

    DatabaseQueue *dq = make_db_queue_with_engine(false, true);
    mock_dbqueue_set_get_database_result(dq);

    enum MHD_Result result = handle_auth_chat_stream_request(
        mock_connection, "/api/conduit/auth_chat/stream",
        "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    free_db_queue_with_engine(dq);
}

void test_handle_auth_chat_stream_request_stream_stub_success(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection *)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_mhd_set_lookup_result("Bearer testtoken");
    jwt_claims_t claims_storage;
    mock_auth_service_jwt_set_validation_result(
        make_valid_jwt_with_database(&claims_storage, "test_db"));
    mock_api_utils_set_buffer_data(
        "{\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}]}");

    DatabaseQueue *dq = make_db_queue_with_engine(true, true);
    mock_dbqueue_set_get_database_result(dq);

    enum MHD_Result result = handle_auth_chat_stream_request(
        mock_connection, "/api/conduit/auth_chat/stream",
        "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    free_db_queue_with_engine(dq);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_auth_chat_stream_request_buffer_continue);
    RUN_TEST(test_handle_auth_chat_stream_request_buffer_error);
    RUN_TEST(test_handle_auth_chat_stream_request_invalid_method);
    RUN_TEST(test_handle_auth_chat_stream_request_no_auth);
    RUN_TEST(test_handle_auth_chat_stream_request_invalid_json);
    RUN_TEST(test_handle_auth_chat_stream_request_parse_error);
    RUN_TEST(test_handle_auth_chat_stream_request_database_not_found);
    RUN_TEST(test_handle_auth_chat_stream_request_cec_missing);
    RUN_TEST(test_handle_auth_chat_stream_request_engine_missing);
    RUN_TEST(test_handle_auth_chat_stream_request_no_default_engine);
    RUN_TEST(test_handle_auth_chat_stream_request_engine_unhealthy);
    RUN_TEST(test_handle_auth_chat_stream_request_stream_stub_success);

    return UNITY_END();
}
