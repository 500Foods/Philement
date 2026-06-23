/*
 * Unity Test File: WebSocket Chat Message Handler Path Tests
 * Tests handle_chat_message paths that require database/engine setup.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the websocket chat module
#include <src/database/dbqueue/dbqueue.h>
#include <src/api/wschat/helpers/engine_cache.h>
#include <src/api/wschat/helpers/proxy.h>
#include <src/api/wschat/helpers/proxy_multi.h>
#include <src/websocket/websocket_server_chat.h>
#include <src/websocket/websocket_server_internal.h>
#include <unity/mocks/mock_libwebsockets.h>

// Test function prototypes
void test_handle_chat_message_chat_not_enabled(void);
void test_handle_chat_message_engine_not_found(void);
void test_handle_chat_message_engine_unhealthy(void);
void test_handle_chat_message_build_request_fails(void);
void test_handle_chat_message_streaming_success(void);
void test_handle_chat_message_streaming_start_fails(void);

// Test fixtures
static WebSocketSessionData test_session;
static DatabaseQueueManager *test_manager = NULL;
static DatabaseQueue *test_queue = NULL;
static ChatEngineCache *test_cache = NULL;

extern DatabaseQueueManager* global_queue_manager;

static json_t* create_valid_request(bool stream) {
    json_t *request = json_object();
    json_t *payload = json_object();
    json_t *messages = json_array();
    json_t *msg = json_object();

    json_object_set_new(msg, "role", json_string("user"));
    json_object_set_new(msg, "content", json_string("hello"));
    json_array_append_new(messages, msg);

    json_object_set_new(payload, "messages", messages);
    if (stream) {
        json_object_set_new(payload, "stream", json_true());
    }
    json_object_set_new(request, "payload", payload);

    return request;
}

static void add_engine_to_cache(const char *name, bool is_default, bool is_healthy) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, name, CEC_PROVIDER_OPENAI, "gpt-4",
        "http://localhost:9999/v1/chat/completions",
        "fake-api-key",
        4096, 0.7, is_default,
        300, 10, 10, 100,
        MODALITY_TEXT, false
    );
    engine->is_healthy = is_healthy;
    chat_engine_cache_add_engine(test_cache, engine);
}

void setUp(void) {
    memset(&test_session, 0, sizeof(test_session));
    test_session.connection_valid = true;
    test_session.chat_database = strdup("test_db");

    test_cache = chat_engine_cache_create("test_db");
    test_queue = (DatabaseQueue*)calloc(1, sizeof(DatabaseQueue));
    test_queue->database_name = strdup("test_db");
    test_queue->chat_engine_cache = test_cache;

    test_manager = database_queue_manager_create(10);
    database_queue_manager_add_database(test_manager, test_queue);
    global_queue_manager = test_manager;

    mock_lws_reset_all();
}

void tearDown(void) {
    if (test_session.chat_database) {
        free(test_session.chat_database);
        test_session.chat_database = NULL;
    }
    if (test_session.multi_stream_ctx) {
        MultiStreamManager *manager = chat_proxy_get_multi_manager();
        if (manager) {
            chat_proxy_multi_stream_stop(manager, test_session.multi_stream_ctx);
        }
        test_session.multi_stream_ctx = NULL;
    }

    // Remove test_queue from manager before destroying manager so it doesn't
    // try to free our minimally-initialized fake queue.
    if (test_manager && test_queue) {
        for (size_t i = 0; i < test_manager->max_databases; i++) {
            if (test_manager->databases[i] == test_queue) {
                test_manager->databases[i] = NULL;
                break;
            }
        }
        test_manager->database_count = 0;
        database_queue_manager_destroy(test_manager);
        test_manager = NULL;
    }

    if (test_queue) {
        free(test_queue->database_name);
        free(test_queue);
        test_queue = NULL;
    }

    if (test_cache) {
        chat_engine_cache_destroy(test_cache);
        test_cache = NULL;
    }

    global_queue_manager = NULL;
    mock_lws_reset_all();
}

void test_handle_chat_message_chat_not_enabled(void) {
    // Queue exists but has no chat engine cache
    test_queue->chat_engine_cache = NULL;

    json_t *request = create_valid_request(false);
    int result = handle_chat_message((struct lws *)0x12345678, &test_session, request);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_handle_chat_message_engine_not_found(void) {
    // Add only a default engine, then request a non-existent named engine
    add_engine_to_cache("default-engine", true, true);

    json_t *request = json_object();
    json_t *payload = json_object();
    json_t *messages = json_array();
    json_t *msg = json_object();
    json_object_set_new(msg, "role", json_string("user"));
    json_object_set_new(msg, "content", json_string("hello"));
    json_array_append_new(messages, msg);
    json_object_set_new(payload, "messages", messages);
    json_object_set_new(payload, "engine", json_string("missing-engine"));
    json_object_set_new(request, "payload", payload);

    int result = handle_chat_message((struct lws *)0x12345678, &test_session, request);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_handle_chat_message_engine_unhealthy(void) {
    add_engine_to_cache("default-engine", true, false);

    json_t *request = create_valid_request(false);
    int result = handle_chat_message((struct lws *)0x12345678, &test_session, request);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_handle_chat_message_build_request_fails(void) {
    add_engine_to_cache("default-engine", true, true);

    // Empty messages array causes chat_messages to be NULL and chat_request_build to fail
    json_t *request = json_object();
    json_t *payload = json_object();
    json_t *messages = json_array();
    json_object_set_new(payload, "messages", messages);
    json_object_set_new(request, "payload", payload);

    int result = handle_chat_message((struct lws *)0x12345678, &test_session, request);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_handle_chat_message_streaming_success(void) {
    add_engine_to_cache("default-engine", true, true);

    // Ensure multi-stream manager is initialized
    chat_subsystem_cleanup();
    int init_result = chat_subsystem_init();
    TEST_ASSERT_EQUAL_INT(0, init_result);

    json_t *request = create_valid_request(true);
    int result = handle_chat_message((struct lws *)0x12345678, &test_session, request);

    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_TRUE(test_session.chat_stream_active);
    TEST_ASSERT_NOT_NULL(test_session.multi_stream_ctx);

    chat_session_cleanup(&test_session, NULL);
    chat_subsystem_cleanup();
}

void test_handle_chat_message_streaming_start_fails(void) {
    add_engine_to_cache("default-engine", true, true);

    // Ensure multi-stream manager is NOT initialized
    chat_subsystem_cleanup();

    json_t *request = create_valid_request(true);
    int result = handle_chat_message((struct lws *)0x12345678, &test_session, request);

    TEST_ASSERT_EQUAL_INT(-1, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_chat_message_chat_not_enabled);
    RUN_TEST(test_handle_chat_message_engine_not_found);
    RUN_TEST(test_handle_chat_message_engine_unhealthy);
    RUN_TEST(test_handle_chat_message_build_request_fails);
    RUN_TEST(test_handle_chat_message_streaming_success);
    RUN_TEST(test_handle_chat_message_streaming_start_fails);

    return UNITY_END();
}
