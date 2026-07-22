/*
 * Unity Test File: scripting_api_llm_test_resolve_engine.c
 *
 * Tests resolve_llm_engine() helper function for error and success paths:
 *   - NULL model_name returns NULL
 *   - Empty model_name returns NULL
 *   - NULL chat_engine_cache returns NULL
 *   - Model not found returns NULL
 *   - Valid model returns engine config
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <stdlib.h>

#define USE_MOCK_DBQUEUE
#include <unity/mocks/mock_dbqueue.h>

#define USE_MOCK_CHAT_ENGINE_CACHE
#include <unity/mocks/mock_chat_engine_cache.h>

#include <src/scripting/scripting_api_llm.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/api/wschat/helpers/engine_cache.h>

/* Declare global_queue_manager as extern - it's defined in the source */
extern DatabaseQueueManager* global_queue_manager;

/* Forward declarations */
void test_resolve_llm_engine_null_model_name(void);
void test_resolve_llm_engine_empty_model_name(void);
void test_resolve_llm_engine_null_cache_returns_null(void);
void test_resolve_llm_engine_model_not_found(void);
void test_resolve_llm_engine_valid_model(void);

void setUp(void) {
    mock_dbqueue_reset_all();
    mock_chat_engine_cache_reset_all();
}

void tearDown(void) {
}

/* resolve_llm_engine with NULL model_name returns NULL */
void test_resolve_llm_engine_null_model_name(void) {
    AppConfig config = {0};
    config.scripting.DefaultDatabase = strdup("test-db");
    app_config = &config;

    DatabaseQueue dbq = {0};
    dbq.chat_engine_cache = NULL;

    DatabaseQueueManager mgr = {0};
    global_queue_manager = &mgr;

    mock_dbqueue_set_get_database_result(&dbq);
    mock_chat_engine_cache_set_lookup_by_name_result(NULL);

    ChatEngineConfig* result = resolve_llm_engine(NULL, "test-db");

    TEST_ASSERT_NULL(result);

    free(config.scripting.DefaultDatabase);
}

/* resolve_llm_engine with empty model_name returns NULL */
void test_resolve_llm_engine_empty_model_name(void) {
    AppConfig config = {0};
    config.scripting.DefaultDatabase = strdup("test-db");
    app_config = &config;

    DatabaseQueue dbq = {0};
    dbq.chat_engine_cache = NULL;

    DatabaseQueueManager mgr = {0};
    global_queue_manager = &mgr;

    mock_dbqueue_set_get_database_result(&dbq);

    ChatEngineConfig* result = resolve_llm_engine("", "test-db");

    TEST_ASSERT_NULL(result);

    free(config.scripting.DefaultDatabase);
}

/* resolve_llm_engine with NULL chat_engine_cache returns NULL */
void test_resolve_llm_engine_null_cache_returns_null(void) {
    AppConfig config = {0};
    config.scripting.DefaultDatabase = strdup("test-db");
    app_config = &config;

    DatabaseQueue dbq = {0};
    dbq.chat_engine_cache = NULL;

    DatabaseQueueManager mgr = {0};
    global_queue_manager = &mgr;

    mock_dbqueue_set_get_database_result(&dbq);
    mock_chat_engine_cache_set_lookup_by_name_result(NULL);

    ChatEngineConfig* result = resolve_llm_engine("any-model", "test-db");

    TEST_ASSERT_NULL(result);

    free(config.scripting.DefaultDatabase);
}

/* resolve_llm_engine with model not found returns NULL */
void test_resolve_llm_engine_model_not_found(void) {
    AppConfig config = {0};
    config.scripting.DefaultDatabase = strdup("test-db");
    app_config = &config;

    DatabaseQueue dbq = {0};
    dbq.chat_engine_cache = (ChatEngineCache*)0x1;

    DatabaseQueueManager mgr = {0};
    global_queue_manager = &mgr;

    mock_dbqueue_set_get_database_result(&dbq);
    mock_chat_engine_cache_set_lookup_by_name_result(NULL);

    ChatEngineConfig* result = resolve_llm_engine("nonexistent-model", "test-db");

    TEST_ASSERT_NULL(result);

    free(config.scripting.DefaultDatabase);
}

/* resolve_llm_engine with valid model returns engine config */
void test_resolve_llm_engine_valid_model(void) {
    AppConfig config = {0};
    config.scripting.DefaultDatabase = strdup("test-db");
    app_config = &config;

    DatabaseQueue dbq = {0};
    dbq.chat_engine_cache = (ChatEngineCache*)0x1;

    ChatEngineConfig engine = {0};
    snprintf(engine.name, sizeof(engine.name), "test-model");
    snprintf(engine.model, sizeof(engine.model), "gpt-4");

    DatabaseQueueManager mgr = {0};
    global_queue_manager = &mgr;

    mock_dbqueue_set_get_database_result(&dbq);
    mock_chat_engine_cache_set_lookup_by_name_result(&engine);

    ChatEngineConfig* result = resolve_llm_engine("test-model", "test-db");

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test-model", result->name);

    free(config.scripting.DefaultDatabase);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_resolve_llm_engine_null_model_name);
    RUN_TEST(test_resolve_llm_engine_empty_model_name);
    RUN_TEST(test_resolve_llm_engine_null_cache_returns_null);
    RUN_TEST(test_resolve_llm_engine_model_not_found);
    RUN_TEST(test_resolve_llm_engine_valid_model);

    return UNITY_END();
}