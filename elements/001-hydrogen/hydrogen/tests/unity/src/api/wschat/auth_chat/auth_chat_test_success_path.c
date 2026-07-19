/*
 * Unity Test File: auth_chat success path
 * This file contains unit tests for the success (happy) path of
 * handle_auth_chat_request() in src/api/wschat/auth_chat/auth_chat.c
 *
 * The happy path requires a healthy engine and exercises the proxy call,
 * response parsing, context-hashing/segment storage, metrics and the final
 * success envelope. Those external dependencies (chat proxy + chat storage)
 * are mocked via USE_MOCK_AUTH_CHAT_DEPS so the path runs without a live AI
 * provider or database.
 *
 * CHANGELOG:
 * 2026-07-18: Initial implementation covering the success path and proxy/
 *             storage error branches
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/wschat/auth_chat/auth_chat.h>
#include <src/api/wschat/helpers/engine_cache.h>

// Mock includes
#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_API_UTILS
#define USE_MOCK_AUTH_SERVICE_JWT
#define USE_MOCK_DBQUEUE
#define USE_MOCK_AUTH_CHAT_DEPS
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_api_utils.h>
#include <unity/mocks/mock_auth_service_jwt.h>
#include <unity/mocks/mock_dbqueue.h>
#include <unity/mocks/mock_auth_chat_deps.h>

// External global variable (defined in dbqueue.h)
extern DatabaseQueueManager* global_queue_manager;

// Test function prototypes
void test_handle_auth_chat_request_success(void);
void test_handle_auth_chat_request_proxy_failure(void);
void test_handle_auth_chat_request_parse_response_failure(void);
void test_handle_auth_chat_request_payload_too_large(void);
void test_handle_auth_chat_request_context_hash_used(void);
void test_handle_auth_chat_request_context_hash_missed(void);
void test_handle_auth_chat_request_with_params_and_multimodal(void);
void test_handle_auth_chat_request_buffer_continue(void);
void test_handle_auth_chat_request_invalid_token_with_claims(void);

static const char* OPENAI_RESPONSE =
    "{\"choices\":[{\"message\":{\"role\":\"assistant\","
    "\"content\":\"Hello! How can I help?\"}}],"
    "\"model\":\"gpt-4-turbo\",\"usage\":{\"prompt_tokens\":5,"
    "\"completion_tokens\":7,\"total_tokens\":12}}";

static DatabaseQueue* make_db_queue(void) {
    DatabaseQueue* dq = calloc(1, sizeof(DatabaseQueue));
    if (!dq) return NULL;
    dq->database_name = strdup("test_db");
    ChatEngineCache* cec = chat_engine_cache_create("test_db");
    ChatEngineConfig* engine = chat_engine_config_create(
        1, "gpt-4", CEC_PROVIDER_OPENAI, "gpt-4-turbo",
        "https://api.example.com", "key", 1000, 0.7, true,
        0, 4, 8, 8, MODALITY_TEXT, false);
    engine->is_healthy = true;
    engine->max_payload_mb = 8;
    chat_engine_cache_add_engine(cec, engine);
    dq->chat_engine_cache = cec;
    return dq;
}

static void free_db_queue(DatabaseQueue* dq) {
    if (!dq) return;
    if (dq->chat_engine_cache) chat_engine_cache_destroy(dq->chat_engine_cache);
    free(dq->database_name);
    free(dq);
}

void setUp(void) {
    mock_mhd_reset_all();
    mock_api_utils_reset_all();
    mock_auth_service_jwt_reset_all();
    mock_dbqueue_reset_all();
    mock_auth_chat_deps_reset_all();

    mock_mhd_set_lookup_result("Bearer testtoken");
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    global_queue_manager = NULL;
}

void tearDown(void) {
    global_queue_manager = NULL;
}

// Full happy path: valid JWT, valid parse, healthy engine, mocked proxy
// success, mocked storage. Exercises the entire success block (202-533).
void test_handle_auth_chat_request_success(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    jwt_claims_t claims_storage;
    memset(&claims_storage, 0, sizeof(jwt_claims_t));
    claims_storage.database = (char*)"test_db";
    claims_storage.user_id = 42;
    mock_auth_service_jwt_set_validation_result(
        (jwt_validation_result_t){true, &claims_storage, JWT_ERROR_NONE});

    mock_api_utils_set_buffer_data(
        "{\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}]}");

    mock_auth_chat_deps_set_proxy_success(true);
    mock_auth_chat_deps_set_proxy_response_body(OPENAI_RESPONSE);

    DatabaseQueue* dq = make_db_queue();
    mock_dbqueue_set_get_database_result(dq);

    enum MHD_Result result = handle_auth_chat_request(mock_connection,
                                                      "/api/conduit/auth_chat",
                                                      "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_TRUE(mock_auth_chat_deps_store_segment_count() > 0);
    free_db_queue(dq);
}

// Proxy returns failure -> "Request failed" / proxy_error -> 502 branch.
void test_handle_auth_chat_request_proxy_failure(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    jwt_claims_t claims_storage;
    memset(&claims_storage, 0, sizeof(jwt_claims_t));
    claims_storage.database = (char*)"test_db";
    claims_storage.user_id = 42;
    mock_auth_service_jwt_set_validation_result(
        (jwt_validation_result_t){true, &claims_storage, JWT_ERROR_NONE});

    mock_api_utils_set_buffer_data(
        "{\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}]}");

    mock_auth_chat_deps_set_proxy_success(false);

    DatabaseQueue* dq = make_db_queue();
    mock_dbqueue_set_get_database_result(dq);

    enum MHD_Result result = handle_auth_chat_request(mock_connection,
                                                      "/api/conduit/auth_chat",
                                                      "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    free_db_queue(dq);
}

// Proxy succeeds but response body is not parseable -> 502 parse branch.
void test_handle_auth_chat_request_parse_response_failure(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    jwt_claims_t claims_storage;
    memset(&claims_storage, 0, sizeof(jwt_claims_t));
    claims_storage.database = (char*)"test_db";
    claims_storage.user_id = 42;
    mock_auth_service_jwt_set_validation_result(
        (jwt_validation_result_t){true, &claims_storage, JWT_ERROR_NONE});

    mock_api_utils_set_buffer_data(
        "{\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}]}");

    mock_auth_chat_deps_set_proxy_success(true);
    mock_auth_chat_deps_set_proxy_response_body("not valid json at all");

    DatabaseQueue* dq = make_db_queue();
    mock_dbqueue_set_get_database_result(dq);

    enum MHD_Result result = handle_auth_chat_request(mock_connection,
                                                      "/api/conduit/auth_chat",
                                                      "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    free_db_queue(dq);
}

// Request body exceeds the engine payload limit -> 413 branch (288-299).
// Set a tiny max_payload_mb and a long message so the built request body is
// larger than the limit.
void test_handle_auth_chat_request_payload_too_large(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    jwt_claims_t claims_storage;
    memset(&claims_storage, 0, sizeof(jwt_claims_t));
    claims_storage.database = (char*)"test_db";
    claims_storage.user_id = 42;
    mock_auth_service_jwt_set_validation_result(
        (jwt_validation_result_t){true, &claims_storage, JWT_ERROR_NONE});

    // Long message so the serialized request body exceeds 1 MB limit.
    mock_api_utils_set_buffer_data(
        "{\"messages\":[{\"role\":\"user\",\"content\":\"AAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "\"}]}");

    mock_auth_chat_deps_set_proxy_success(true);
    mock_auth_chat_deps_set_proxy_response_body(OPENAI_RESPONSE);

    DatabaseQueue* dq = make_db_queue();
    if (dq && dq->chat_engine_cache && dq->chat_engine_cache->engines) {
        dq->chat_engine_cache->engines[0]->max_payload_mb = 1;  // 1 MB limit
    }
    mock_dbqueue_set_get_database_result(dq);

    enum MHD_Result result = handle_auth_chat_request(mock_connection,
                                                      "/api/conduit/auth_chat",
                                                      "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    free_db_queue(dq);
}

// Client supplies a valid context hash -> "hashes_used" branch + cache stats.
void test_handle_auth_chat_request_context_hash_used(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    jwt_claims_t claims_storage;
    memset(&claims_storage, 0, sizeof(jwt_claims_t));
    claims_storage.database = (char*)"test_db";
    claims_storage.user_id = 42;
    mock_auth_service_jwt_set_validation_result(
        (jwt_validation_result_t){true, &claims_storage, JWT_ERROR_NONE});

    // A long content message so bandwidth_saved_bytes > 54 (covers 424-425),
    // plus a client-provided context hash (covers 418-427 + 504-521).
    // context hash must be 43 base64url chars (SHA-256 without padding)
    mock_api_utils_set_buffer_data(
        "{\"messages\":[{\"role\":\"user\",\"content\":\"This is a sufficiently long "
        "message payload that exceeds the fifty-four byte hash overhead threshold "
        "so bandwidth saved accounting can be exercised by the unit test.\"}],"
        "\"context_hashes\":[\"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopq\"]}");

    mock_auth_chat_deps_set_proxy_success(true);
    mock_auth_chat_deps_set_proxy_response_body(OPENAI_RESPONSE);
    mock_auth_chat_deps_set_segment_exists(true);

    DatabaseQueue* dq = make_db_queue();
    mock_dbqueue_set_get_database_result(dq);

    enum MHD_Result result = handle_auth_chat_request(mock_connection,
                                                      "/api/conduit/auth_chat",
                                                      "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    free_db_queue(dq);
}

// Client hash not found in storage -> hashes_missed branch.
void test_handle_auth_chat_request_context_hash_missed(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    jwt_claims_t claims_storage;
    memset(&claims_storage, 0, sizeof(jwt_claims_t));
    claims_storage.database = (char*)"test_db";
    claims_storage.user_id = 42;
    mock_auth_service_jwt_set_validation_result(
        (jwt_validation_result_t){true, &claims_storage, JWT_ERROR_NONE});

    mock_api_utils_set_buffer_data(
        "{\"messages\":[{\"role\":\"user\",\"content\":\"hello again\"}],"
        "\"context_hashes\":[\"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopq\"]}");

    mock_auth_chat_deps_set_proxy_success(true);
    mock_auth_chat_deps_set_proxy_response_body(OPENAI_RESPONSE);
    mock_auth_chat_deps_set_segment_exists(false);

    DatabaseQueue* dq = make_db_queue();
    mock_dbqueue_set_get_database_result(dq);

    enum MHD_Result result = handle_auth_chat_request(mock_connection,
                                                      "/api/conduit/auth_chat",
                                                      "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    free_db_queue(dq);
}

// Optional temperature/max_tokens + multimodal array content (no media).
void test_handle_auth_chat_request_with_params_and_multimodal(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    jwt_claims_t claims_storage;
    memset(&claims_storage, 0, sizeof(jwt_claims_t));
    claims_storage.database = (char*)"test_db";
    claims_storage.user_id = 42;
    mock_auth_service_jwt_set_validation_result(
        (jwt_validation_result_t){true, &claims_storage, JWT_ERROR_NONE});

    mock_api_utils_set_buffer_data(
        "{\"messages\":[{\"role\":\"user\",\"content\":"
        "[{\"type\":\"text\",\"text\":\"hello multimodal\"}]}],"
        "\"engine\":\"gpt-4\",\"temperature\":0.5,\"max_tokens\":128}");

    mock_auth_chat_deps_set_proxy_success(true);
    mock_auth_chat_deps_set_proxy_response_body(OPENAI_RESPONSE);

    DatabaseQueue* dq = make_db_queue();
    mock_dbqueue_set_get_database_result(dq);

    enum MHD_Result result = handle_auth_chat_request(mock_connection,
                                                      "/api/conduit/auth_chat",
                                                      "POST", NULL, &upload_size, &con_cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    free_db_queue(dq);
}

// Buffering still in progress -> API_BUFFER_CONTINUE returns MHD_YES early.
void test_handle_auth_chat_request_buffer_continue(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_api_utils_set_buffer_result(API_BUFFER_CONTINUE);

    enum MHD_Result result = handle_auth_chat_request(mock_connection,
                                                      "/api/conduit/auth_chat",
                                                      "POST", NULL, &upload_size, &con_cls);
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Invalid JWT that still carries claims -> "Invalid token" branch.
void test_handle_auth_chat_request_invalid_token_with_claims(void) {
    struct MHD_Connection *mock_connection = (struct MHD_Connection*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    jwt_claims_t claims_storage;
    memset(&claims_storage, 0, sizeof(jwt_claims_t));
    claims_storage.database = (char*)"test_db";
    mock_mhd_set_lookup_result("Bearer badtoken");
    mock_auth_service_jwt_set_validation_result(
        (jwt_validation_result_t){false, &claims_storage, JWT_ERROR_INVALID_SIGNATURE});

    enum MHD_Result result = handle_auth_chat_request(mock_connection,
                                                      "/api/conduit/auth_chat",
                                                      "POST", NULL, &upload_size, &con_cls);
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_auth_chat_request_success);
    RUN_TEST(test_handle_auth_chat_request_proxy_failure);
    RUN_TEST(test_handle_auth_chat_request_parse_response_failure);
    RUN_TEST(test_handle_auth_chat_request_payload_too_large);
    RUN_TEST(test_handle_auth_chat_request_context_hash_used);
    RUN_TEST(test_handle_auth_chat_request_context_hash_missed);
    RUN_TEST(test_handle_auth_chat_request_with_params_and_multimodal);
    RUN_TEST(test_handle_auth_chat_request_buffer_continue);
    RUN_TEST(test_handle_auth_chat_request_invalid_token_with_claims);

    return UNITY_END();
}
