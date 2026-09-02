/*
 * Unity Test File: WebSocket Chat Message Handler Path Tests
 * Tests handle_chat_message paths that require database/engine setup.
 *
 * Streaming tests initialize the global MultiStreamManager without spawning
 * the curl worker. chat_subsystem_init() starts that worker and then
 * chat_session_cleanup() calls curl_multi_remove_handle() concurrently —
 * that hang is why Test 10's 30s timeout killed this binary in the suite.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

#include <curl/curl.h>
#include <pthread.h>

// Include necessary headers for the websocket chat module
#include <src/database/dbqueue/dbqueue.h>
#include <src/api/wschat/helpers/engine_cache.h>
#include <src/api/wschat/helpers/proxy.h>
#include <src/api/wschat/helpers/proxy_multi.h>
#include <src/websocket/websocket_server_chat.h>
#include <src/websocket/websocket_server_internal.h>
#include <unity/mocks/mock_libwebsockets.h>

void test_handle_chat_message_chat_not_enabled(void);
void test_handle_chat_message_no_default_engine(void);
void test_handle_chat_message_engine_not_found(void);
void test_handle_chat_message_engine_unhealthy(void);
void test_handle_chat_message_invalid_temperature(void);
void test_handle_chat_message_invalid_max_tokens(void);
void test_handle_chat_message_build_request_fails(void);
void test_handle_chat_message_streaming_success(void);
void test_handle_chat_message_streaming_named_engine(void);
void test_handle_chat_message_streaming_with_request_id(void);
void test_handle_chat_message_streaming_replaces_active_stream(void);
void test_handle_chat_message_streaming_array_content(void);
void test_handle_chat_message_streaming_start_fails(void);

static WebSocketSessionData test_session;
static DatabaseQueueManager *test_manager = NULL;
static DatabaseQueue *test_queue = NULL;
static ChatEngineCache *test_cache = NULL;

extern DatabaseQueueManager* global_queue_manager;

static json_t* create_request(const char *engine, bool stream, const char *request_id,
                              const char *content, bool array_content, bool empty_messages,
                              json_t *temperature, json_t *max_tokens) {
    json_t *request = json_object();
    json_t *payload = json_object();
    json_t *messages = json_array();

    if (!empty_messages) {
        json_t *msg = json_object();
        json_object_set_new(msg, "role", json_string("user"));
        if (array_content) {
            json_t *parts = json_array();
            json_t *part = json_object();
            json_object_set_new(part, "type", json_string("text"));
            json_object_set_new(part, "text", json_string(content ? content : "hello"));
            json_array_append_new(parts, part);
            json_object_set_new(msg, "content", parts);
        } else {
            json_object_set_new(msg, "content", json_string(content ? content : "hello"));
        }
        json_array_append_new(messages, msg);
    }

    json_object_set_new(payload, "messages", messages);
    if (stream) {
        json_object_set_new(payload, "stream", json_true());
    }
    if (engine) {
        json_object_set_new(payload, "engine", json_string(engine));
    }
    if (temperature) {
        json_object_set_new(payload, "temperature", temperature);
    }
    if (max_tokens) {
        json_object_set_new(payload, "max_tokens", max_tokens);
    }
    json_object_set_new(request, "payload", payload);
    if (request_id) {
        json_object_set_new(request, "id", json_string(request_id));
    }

    return request;
}

static void add_engine_to_cache(const char *name, bool is_default, bool is_healthy) {
    ChatEngineConfig *engine = chat_engine_config_create(
        1, name, CEC_PROVIDER_OPENAI, "gpt-4",
        "http://127.0.0.1:1/v1/chat/completions",
        "fake-api-key",
        4096, 0.7, is_default,
        300, 10, 10, 100,
        MODALITY_TEXT, false
    );
    engine->is_healthy = is_healthy;
    chat_engine_cache_add_engine(test_cache, engine);
}

static void init_manager_no_worker(void) {
    MultiStreamManager *manager = chat_proxy_get_multi_manager();

    if (manager->initialized) {
        chat_proxy_multi_cleanup(manager);
    }

    memset(manager, 0, sizeof(*manager));
    manager->multi_handle = curl_multi_init();
    TEST_ASSERT_NOT_NULL(manager->multi_handle);
    pthread_mutex_init(&manager->streams_mutex, NULL);
    manager->initialized = true;
    manager->worker_thread_started = false;
    manager->shutdown_requested = false;
}

static int call_handle(json_t *request) {
    return handle_chat_message((struct lws *)0x12345678, &test_session, request);
}

void setUp(void) {
    memset(&test_session, 0, sizeof(test_session));
    test_session.connection_valid = true;
    test_session.chat_database = strdup("test_db");

    test_cache = chat_engine_cache_create("test_db");
    test_queue = (DatabaseQueue*)calloc(1, sizeof(DatabaseQueue));
    TEST_ASSERT_NOT_NULL(test_queue);
    test_queue->database_name = strdup("test_db");
    test_queue->chat_engine_cache = test_cache;

    test_manager = database_queue_manager_create(10);
    database_queue_manager_add_database(test_manager, test_queue);
    global_queue_manager = test_manager;

    mock_lws_reset_all();
}

void tearDown(void) {
    chat_proxy_multi_cleanup(chat_proxy_get_multi_manager());
    test_session.multi_stream_ctx = NULL;

    if (test_session.chat_database) {
        free(test_session.chat_database);
        test_session.chat_database = NULL;
    }

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
        chat_engine_cache_clear(test_cache);
        test_cache = NULL;
    }

    global_queue_manager = NULL;
    mock_lws_reset_all();
}

void test_handle_chat_message_chat_not_enabled(void) {
    test_queue->chat_engine_cache = NULL;

    TEST_ASSERT_EQUAL_INT(-1, call_handle(create_request(NULL, false, NULL, NULL, false, false, NULL, NULL)));
}

void test_handle_chat_message_no_default_engine(void) {
    TEST_ASSERT_EQUAL_INT(-1, call_handle(create_request(NULL, false, NULL, NULL, false, false, NULL, NULL)));
}

void test_handle_chat_message_engine_not_found(void) {
    add_engine_to_cache("default-engine", true, true);

    TEST_ASSERT_EQUAL_INT(-1, call_handle(create_request("missing-engine", false, NULL, NULL, false, false, NULL, NULL)));
}

void test_handle_chat_message_engine_unhealthy(void) {
    add_engine_to_cache("default-engine", true, false);

    TEST_ASSERT_EQUAL_INT(-1, call_handle(create_request(NULL, false, NULL, NULL, false, false, NULL, NULL)));
}

void test_handle_chat_message_invalid_temperature(void) {
    add_engine_to_cache("default-engine", true, true);

    TEST_ASSERT_EQUAL_INT(-1, call_handle(create_request(NULL, false, NULL, NULL, false, false,
                                                         json_real(3.5), NULL)));
}

void test_handle_chat_message_invalid_max_tokens(void) {
    add_engine_to_cache("default-engine", true, true);

    TEST_ASSERT_EQUAL_INT(-1, call_handle(create_request(NULL, false, NULL, NULL, false, false,
                                                         NULL, json_integer(0))));
}

void test_handle_chat_message_build_request_fails(void) {
    add_engine_to_cache("default-engine", true, true);

    TEST_ASSERT_EQUAL_INT(-1, call_handle(create_request(NULL, false, NULL, NULL, false, true, NULL, NULL)));
}

void test_handle_chat_message_streaming_success(void) {
    add_engine_to_cache("default-engine", true, true);
    init_manager_no_worker();

    TEST_ASSERT_EQUAL_INT(0, call_handle(create_request(NULL, true, NULL, NULL, false, false, NULL, NULL)));
    TEST_ASSERT_TRUE(test_session.chat_stream_active);
    TEST_ASSERT_NOT_NULL(test_session.multi_stream_ctx);
}

void test_handle_chat_message_streaming_named_engine(void) {
    add_engine_to_cache("named-engine", false, true);
    init_manager_no_worker();

    TEST_ASSERT_EQUAL_INT(0, call_handle(create_request("named-engine", true, NULL, NULL, false, false, NULL, NULL)));
    TEST_ASSERT_TRUE(test_session.chat_stream_active);
    TEST_ASSERT_NOT_NULL(test_session.multi_stream_ctx);
}

void test_handle_chat_message_streaming_with_request_id(void) {
    add_engine_to_cache("default-engine", true, true);
    init_manager_no_worker();

    TEST_ASSERT_EQUAL_INT(0, call_handle(create_request(NULL, true, "req-42", NULL, false, false, NULL, NULL)));
    TEST_ASSERT_TRUE(test_session.chat_stream_active);
    TEST_ASSERT_NOT_NULL(test_session.multi_stream_ctx);
}

void test_handle_chat_message_streaming_replaces_active_stream(void) {
    add_engine_to_cache("default-engine", true, true);
    init_manager_no_worker();
    test_session.chat_stream_active = true;

    TEST_ASSERT_EQUAL_INT(0, call_handle(create_request(NULL, true, NULL, NULL, false, false, NULL, NULL)));
    TEST_ASSERT_TRUE(test_session.chat_stream_active);
    TEST_ASSERT_NOT_NULL(test_session.multi_stream_ctx);
}

void test_handle_chat_message_streaming_array_content(void) {
    add_engine_to_cache("default-engine", true, true);
    init_manager_no_worker();

    TEST_ASSERT_EQUAL_INT(0, call_handle(create_request(NULL, true, NULL, "array-hello", true, false, NULL, NULL)));
    TEST_ASSERT_TRUE(test_session.chat_stream_active);
    TEST_ASSERT_NOT_NULL(test_session.multi_stream_ctx);
}

void test_handle_chat_message_streaming_start_fails(void) {
    add_engine_to_cache("default-engine", true, true);

    TEST_ASSERT_EQUAL_INT(-1, call_handle(create_request(NULL, true, NULL, NULL, false, false, NULL, NULL)));
    TEST_ASSERT_FALSE(test_session.chat_stream_active);
    TEST_ASSERT_NULL(test_session.multi_stream_ctx);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_chat_message_chat_not_enabled);
    RUN_TEST(test_handle_chat_message_no_default_engine);
    RUN_TEST(test_handle_chat_message_engine_not_found);
    RUN_TEST(test_handle_chat_message_engine_unhealthy);
    RUN_TEST(test_handle_chat_message_invalid_temperature);
    RUN_TEST(test_handle_chat_message_invalid_max_tokens);
    RUN_TEST(test_handle_chat_message_build_request_fails);
    RUN_TEST(test_handle_chat_message_streaming_success);
    RUN_TEST(test_handle_chat_message_streaming_named_engine);
    RUN_TEST(test_handle_chat_message_streaming_with_request_id);
    RUN_TEST(test_handle_chat_message_streaming_replaces_active_stream);
    RUN_TEST(test_handle_chat_message_streaming_array_content);
    RUN_TEST(test_handle_chat_message_streaming_start_fails);

    return UNITY_END();
}
