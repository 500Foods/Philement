/*
 * Unity Test File: auth_chats handler success path
 * Exercises fan-out orchestration with deterministic mocked provider results.
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source headers
#include <src/api/wschat/auth_chats/auth_chats.h>

// Mock includes; USE_MOCK_* definitions are supplied by CMake.
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_api_utils.h>
#include <unity/mocks/mock_auth_service_jwt.h>
#include <unity/mocks/mock_dbqueue.h>
#include <unity/mocks/mock_auth_chat_deps.h>

extern DatabaseQueueManager *global_queue_manager;

void test_handle_auth_chats_request_success(void);
void test_handle_auth_chats_request_partial_failure(void);
void test_handle_auth_chats_request_no_healthy_engines(void);
void test_handle_auth_chats_request_chat_disabled(void);
void test_handle_auth_chats_request_buffer_paths(void);

static const char *OPENAI_RESPONSE =
    "{\"choices\":[{\"message\":{\"content\":\"hello\"},\"finish_reason\":\"stop\"}],"
    "\"model\":\"test-model\",\"usage\":{\"prompt_tokens\":2,"
    "\"completion_tokens\":3,\"total_tokens\":5}}";

void setUp(void) {
    mock_mhd_reset_all();
    mock_api_utils_reset_all();
    mock_auth_service_jwt_reset_all();
    mock_dbqueue_reset_all();
    mock_auth_chat_deps_reset_all();
    mock_mhd_set_lookup_result("Bearer token");
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    global_queue_manager = NULL;
}

void tearDown(void) {
    global_queue_manager = NULL;
}

static DatabaseQueue *make_queue(bool with_cache, bool healthy) {
    DatabaseQueue *queue = calloc(1, sizeof(DatabaseQueue));
    if (!with_cache) return queue;
    queue->chat_engine_cache = chat_engine_cache_create("test_db");
    ChatEngineConfig *engine = chat_engine_config_create(
        1, "engine", CEC_PROVIDER_OPENAI, "test-model", "https://example.test", "key",
        1000, 0.7, false, 0, 4, 8, 4, MODALITY_TEXT, false);
    engine->is_healthy = healthy;
    chat_engine_cache_add_engine(queue->chat_engine_cache, engine);
    return queue;
}

static void free_queue(DatabaseQueue *queue) {
    if (!queue) return;
    chat_engine_cache_destroy(queue->chat_engine_cache);
    free(queue);
}

static void set_valid_request(void) {
    jwt_claims_t claims = {0};
    claims.database = (char *)"test_db";
    claims.user_id = 7;
    mock_auth_service_jwt_set_validation_result(
        (jwt_validation_result_t){true, &claims, JWT_ERROR_NONE});
    mock_api_utils_set_buffer_data(
        "{\"engines\":[\"engine\"],\"messages\":[{\"role\":\"user\","
        "\"content\":\"hi\"}],\"temperature\":0.4,\"max_tokens\":64}");
}

static enum MHD_Result run_handler(void) {
    void *con_cls = NULL;
    size_t upload_size = 0;
    return handle_auth_chats_request((struct MHD_Connection *)0x123,
                                     "/api/conduit/auth_chats", "POST", NULL,
                                     &upload_size, &con_cls);
}

void test_handle_auth_chats_request_success(void) {
    set_valid_request();
    mock_auth_chat_deps_set_proxy_response_body(OPENAI_RESPONSE);
    DatabaseQueue *queue = make_queue(true, true);
    TEST_ASSERT_NOT_NULL(chat_engine_cache_lookup_by_name(queue->chat_engine_cache, "engine"));
    mock_dbqueue_set_get_database_result(queue);

    TEST_ASSERT_EQUAL(MHD_YES, run_handler());
    TEST_ASSERT_EQUAL_INT(1, mock_auth_chat_deps_proxy_call_count());
    free_queue(queue);
}

void test_handle_auth_chats_request_partial_failure(void) {
    set_valid_request();
    mock_auth_chat_deps_set_proxy_response_body(OPENAI_RESPONSE);
    mock_auth_chat_deps_set_multi_failure_index(0);
    DatabaseQueue *queue = make_queue(true, true);
    TEST_ASSERT_NOT_NULL(chat_engine_cache_lookup_by_name(queue->chat_engine_cache, "engine"));
    mock_dbqueue_set_get_database_result(queue);

    TEST_ASSERT_EQUAL(MHD_YES, run_handler());
    TEST_ASSERT_EQUAL_INT(1, mock_auth_chat_deps_proxy_call_count());
    free_queue(queue);
}

void test_handle_auth_chats_request_no_healthy_engines(void) {
    set_valid_request();
    DatabaseQueue *queue = make_queue(true, false);
    mock_dbqueue_set_get_database_result(queue);

    TEST_ASSERT_EQUAL(MHD_YES, run_handler());
    TEST_ASSERT_EQUAL_INT(0, mock_auth_chat_deps_proxy_call_count());
    free_queue(queue);
}

void test_handle_auth_chats_request_chat_disabled(void) {
    set_valid_request();
    DatabaseQueue *queue = make_queue(false, false);
    mock_dbqueue_set_get_database_result(queue);

    TEST_ASSERT_EQUAL(MHD_YES, run_handler());
    free_queue(queue);
}

void test_handle_auth_chats_request_buffer_paths(void) {
    mock_api_utils_set_buffer_result(API_BUFFER_CONTINUE);
    TEST_ASSERT_EQUAL(MHD_YES, run_handler());
    mock_api_utils_set_buffer_result(API_BUFFER_ERROR);
    TEST_ASSERT_EQUAL(MHD_YES, run_handler());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_handle_auth_chats_request_success);
    RUN_TEST(test_handle_auth_chats_request_partial_failure);
    RUN_TEST(test_handle_auth_chats_request_no_healthy_engines);
    RUN_TEST(test_handle_auth_chats_request_chat_disabled);
    RUN_TEST(test_handle_auth_chats_request_buffer_paths);
    return UNITY_END();
}
