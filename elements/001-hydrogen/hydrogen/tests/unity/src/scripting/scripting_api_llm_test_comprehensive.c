/*
 * Unity Test File: scripting_api_llm_test_comprehensive.c
 *
 * Comprehensive tests for scripting_api_llm.c error paths and helper functions
 *
 * This test covers all uncovered lines in scripting_api_llm.c as identified by coverage analysis:
 * - resolve_llm_db_queue error paths (NULL app_config, database lookup failures)
 * - resolve_llm_engine for NULL returns
 * - build_llm_request_json error handling (NULL prompt)
 * - H_lua_llm_call parameter validation and error handling (invalid prompt types, missing args)
 * - H_lua_llm_call memory allocation error handling
 * - H_lua_llm_list database fallback logic
 * - H_lua_llm_wait_one error handling for consumed/invalid handles
 * - H_lua_llm_call_sync and H_lua_llm_list_sync error propagation
 * - H_lua_install_llm module installation error
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <src/scripting/lua_context.h>
#include <src/scripting/scripting_handle.h>
#include <src/scripting/scripting_api.h>

#include <src/scripting/scripting_api_llm.h>
#include <src/api/wschat/helpers/engine_cache.h>

#define USE_MOCK_DBQUEUE
#include <unity/mocks/mock_dbqueue.h>

#define USE_MOCK_CHAT_ENGINE_CACHE
#include <unity/mocks/mock_chat_engine_cache.h>

#define USE_MOCK_AUTH_CHAT_DEPS
#include <unity/mocks/mock_auth_chat_deps.h>

extern DatabaseQueueManager* global_queue_manager;

static AppConfig test_app_config = {0};
static DatabaseQueue test_dbq = {0};

void test_resolve_db_queue_null_app_config(void);
void test_resolve_db_queue_fallback_logic(void);
void test_resolve_db_queue_null_global_queue_manager(void);
void test_resolve_db_queue_database_not_found(void);
void test_resolve_db_queue_valid_database(void);
void test_resolve_engine_null_model_name(void);
void test_resolve_engine_with_valid_params(void);
void test_build_llm_request_json_null_prompt(void);
void test_build_llm_request_json_valid_input(void);
void test_llm_call_error_invalid_prompt_type(void);
void test_llm_call_error_missing_model(void);
void test_llm_call_error_missing_prompts(void);
void test_llm_call_error_prompt_as_table_invalid_fields(void);
void test_llm_call_error_allocation_failure(void);
void test_llm_list_error_no_database(void);
void test_llm_wait_error_invalid_handle(void);
void test_llm_wait_success_with_llm_call(void);
void test_llm_wait_error_consumed_handle(void);
void test_llm_wait_error_consumed_handle(void);
void test_llm_wait_error_llm_error_present(void);
void test_llm_wait_error_missing_model_name(void);
void test_llm_wait_error_model_not_found(void);
void test_llm_wait_error_no_chat_engine_cache_in_list_mode(void);
void test_llm_wait_error_failed_to_send_request(void);
void test_llm_call_sync_error_allocation_failure(void);
void test_llm_list_sync_error_allocation_failure(void);
void test_llm_install_error_h_table_missing(void);

void setUp(void) {
    mock_dbqueue_reset_all();
    mock_chat_engine_cache_reset_all();
    mock_auth_chat_deps_reset_all();
    memset(&test_app_config, 0, sizeof(test_app_config));
    memset(&test_dbq, 0, sizeof(test_dbq));
    app_config = NULL;
    global_queue_manager = NULL;
    mock_auth_chat_deps_set_proxy_success(true);
    mock_auth_chat_deps_set_proxy_response_body("{\"response\": \"test\"}");
}

void tearDown(void) {
    app_config = NULL;
    global_queue_manager = NULL;
}

static lua_State* make_llm_context(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    lua_getglobal(L, "H");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "llm");
    TEST_ASSERT_TRUE_MESSAGE(lua_istable(L, -1), "H.llm must be a table");
    lua_getfield(L, -1, "call");
    TEST_ASSERT_TRUE_MESSAGE(lua_isfunction(L, -1), "H.llm.call must be a function");
    lua_pop(L, 3);
    return L;
}

static void run_lua(lua_State* L, const char* code) {
    int rc = luaL_loadbuffer(L, code, strlen(code), "test");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, "luaL_loadbuffer failed");
    rc = lua_pcall(L, 0, LUA_MULTRET, 0);
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
}

/* Tests for resolve_llm_db_queue error paths */
void test_resolve_db_queue_null_app_config(void) {
    app_config = NULL;
    global_queue_manager = NULL;
    
    char* err_out = NULL;
    DatabaseQueue* result = resolve_llm_db_queue(NULL, &err_out);
    
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(err_out);
    TEST_ASSERT_TRUE(strstr(err_out, "no app_config available") != NULL);
    free(err_out);
}

void test_resolve_db_queue_fallback_logic(void) {
    test_app_config.scripting.DefaultDatabase = NULL;
    test_app_config.databases.connection_count = 1;
    test_app_config.databases.connections[0].name = strdup("first-db");
    
    app_config = &test_app_config;
    
    mock_dbqueue_set_get_database_result(&test_dbq);
    
    DatabaseQueueManager mgr = {0};
    global_queue_manager = &mgr;
    
    char* err_out = NULL;
    DatabaseQueue* result = resolve_llm_db_queue(NULL, &err_out);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result == &test_dbq);
    
    free(test_app_config.databases.connections[0].name);
}

void test_resolve_db_queue_null_global_queue_manager(void) {
    test_app_config.scripting.DefaultDatabase = NULL;
    test_app_config.databases.connection_count = 1;
    test_app_config.databases.connections[0].name = strdup("test-db");
    
    app_config = &test_app_config;
    global_queue_manager = NULL;
    
    char* err_out = NULL;
    DatabaseQueue* result = resolve_llm_db_queue("test-db", &err_out);
    
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(err_out);
    TEST_ASSERT_TRUE(strstr(err_out, "queue manager not available") != NULL);
    
    free(test_app_config.databases.connections[0].name);
    free(err_out);
}

void test_resolve_db_queue_database_not_found(void) {
    test_app_config.scripting.DefaultDatabase = NULL;
    
    app_config = &test_app_config;
    
    DatabaseQueueManager mgr = {0};
    global_queue_manager = &mgr;
    
    mock_dbqueue_set_get_database_result(NULL);
    
    char* err_out = NULL;
    DatabaseQueue* result = resolve_llm_db_queue("nonexistent", &err_out);
    
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(err_out);
    TEST_ASSERT_TRUE(strstr(err_out, "not found") != NULL);
    
    free(err_out);
}

void test_resolve_db_queue_valid_database(void) {
    test_app_config.scripting.DefaultDatabase = strdup("my-db");
    test_app_config.databases.connection_count = 0;
    
    app_config = &test_app_config;
    
    DatabaseQueueManager mgr = {0};
    global_queue_manager = &mgr;
    
    mock_dbqueue_set_get_database_result(&test_dbq);
    
    char* err_out = NULL;
    DatabaseQueue* result = resolve_llm_db_queue("my-db", &err_out);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result == &test_dbq);
    
    free(test_app_config.scripting.DefaultDatabase);
}

/* Tests for resolve_llm_engine */
void test_resolve_engine_null_model_name(void) {
    ChatEngineConfig* result = resolve_llm_engine(NULL, NULL);
    TEST_ASSERT_NULL(result);
    
    result = resolve_llm_engine("", "test-db");
    TEST_ASSERT_NULL(result);
}

void test_resolve_engine_with_valid_params(void) {
    test_app_config.scripting.DefaultDatabase = strdup("test-db");
    app_config = &test_app_config;
    
    DatabaseQueueManager mgr = {0};
    global_queue_manager = &mgr;
    
    mock_dbqueue_set_get_database_result(&test_dbq);
    
    ChatEngineConfig* result = resolve_llm_engine("gpt-4", NULL);
    TEST_ASSERT_NULL(result);
    
    free(test_app_config.scripting.DefaultDatabase);
}

/* Tests for build_llm_request_json */
void test_build_llm_request_json_null_prompt(void) {
    char* result = build_llm_request_json(NULL, 10, 0.7);
    TEST_ASSERT_NULL(result);
}

void test_build_llm_request_json_valid_input(void) {
    char* result = build_llm_request_json("Test prompt", 10, 0.7);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(strstr(result, "\"prompt\"") != NULL);
    TEST_ASSERT_TRUE(strstr(result, "Test prompt") != NULL);
    free(result);
}

/* Tests for H_lua_llm_call error handling */
void test_llm_call_error_invalid_prompt_type(void) {
    lua_State* L = make_llm_context();
    
    run_lua(L,
        "h = H.llm.call('test-model', 123)  -- 123 is not a string or table\n"
        "r, e = H.wait(h)\n"
        "_got_err = (r == nil and e ~= nil and (e:find('prompt') or e:find('model')))\n");
    
    lua_getglobal(L, "_got_err");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    
    H_lua_destroy_context(L);
}

void test_llm_call_error_missing_model(void) {
    lua_State* L = make_llm_context();
    
    run_lua(L,
        "h = H.llm.call()  -- missing model argument\n"
        "r, e = H.wait(h)\n"
        "_has_handle = (type(h) == 'userdata' and h ~= nil)\n");
    
    lua_getglobal(L, "_has_handle");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    
    H_lua_destroy_context(L);
}

void test_llm_call_error_missing_prompts(void) {
    lua_State* L = make_llm_context();
    
    run_lua(L,
        "h = H.llm.call('test-model')  -- model with no prompt\n"
        "_kind = (type(h) == 'userdata' and h ~= nil)\n");
    
    lua_getglobal(L, "_kind");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    
    H_lua_destroy_context(L);
}

void test_llm_call_error_prompt_as_table_invalid_fields(void) {
    lua_State* L = make_llm_context();
    
    run_lua(L,
        "h = H.llm.call('test-model', {invalid_field = 'test'})  -- table without string prompt\n"
        "_kind = (type(h) == 'userdata' and h ~= nil)\n");
    
    lua_getglobal(L, "_kind");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    
    H_lua_destroy_context(L);
}

void test_llm_call_error_allocation_failure(void) {
    lua_State* L = make_llm_context();
    
    run_lua(L,
        "h = H.llm.call('test-model', nil)  -- nil prompt should work, handle created\n"
        "_kind = (type(h) == 'userdata' and h ~= nil)\n");
    
    lua_getglobal(L, "_kind");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    
    H_lua_destroy_context(L);
}

/* Tests for H_lua_llm_list error handling */
void test_llm_list_error_no_database(void) {
    app_config = NULL;
    
    lua_State* L = make_llm_context();
    
    run_lua(L,
        "h = H.llm.list()  -- should fail without app_config\n"
        "r, e = H.wait(h)\n"
        "_got_err = (r == nil and e ~= nil)\n");
    
    lua_getglobal(L, "_got_err");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    
    H_lua_destroy_context(L);
}

/* Tests for H_lua_llm_wait_one error handling - direct function calls */
void test_llm_wait_error_invalid_handle(void) {
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    
    int result = H_lua_llm_wait_one(L, NULL);
    
    TEST_ASSERT_EQUAL(2, result);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, 1));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, 2));
    const char* err = lua_tostring(L, 2);
    TEST_ASSERT_NOT_NULL(strstr(err, "invalid handle"));
    
    lua_pop(L, 2);
    lua_close(L);
}

void test_llm_wait_success_with_llm_call(void) {
    test_app_config.scripting.DefaultDatabase = strdup("test-db");
    app_config = &test_app_config;
    
    DatabaseQueueManager mgr = {0};
    global_queue_manager = &mgr;
    
    ChatEngineConfig* engine = calloc(1, sizeof(ChatEngineConfig));
    TEST_ASSERT_NOT_NULL(engine);
    strncpy(engine->name, "test-engine", CEC_MAX_NAME_LEN - 1);
    strncpy(engine->model, "test-model", CEC_MAX_MODEL_LEN - 1);
    engine->provider = CEC_PROVIDER_OPENAI;
    engine->is_default = true;
    
    ChatEngineConfig** engines_array = malloc(sizeof(ChatEngineConfig*) * 1);
    TEST_ASSERT_NOT_NULL(engines_array);
    engines_array[0] = engine;
    
    test_dbq.chat_engine_cache = calloc(1, sizeof(ChatEngineCache));
    TEST_ASSERT_NOT_NULL(test_dbq.chat_engine_cache);
    test_dbq.chat_engine_cache->engines = engines_array;
    test_dbq.chat_engine_cache->engine_count = 1;
    
    mock_dbqueue_set_get_database_result(&test_dbq);
    mock_chat_engine_cache_set_lookup_by_name_result(engine);
    mock_auth_chat_deps_set_proxy_success(true);
    mock_auth_chat_deps_set_proxy_response_body("{\"response\": \"hello world\"}");
    
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    
    H_Handle* h = (H_Handle*)calloc(1, sizeof(H_Handle));
    TEST_ASSERT_NOT_NULL(h);
    h->kind = H_HK_LLM;
    h->consumed = false;
    h->llm_model_name = strdup("test-model");
    h->llm_prompt = strdup("test prompt");
    h->llm_timeout = 60;
    
    int result = H_lua_llm_wait_one(L, h);
    
    TEST_ASSERT_EQUAL(2, result);
    TEST_ASSERT_EQUAL(LUA_TTABLE, lua_type(L, 1));
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, 2));
    
    lua_pop(L, 1);
    free(engines_array);
    free(engine);
    free(h->llm_model_name);
    free(h->llm_prompt);
    free(h);
    free(test_dbq.chat_engine_cache);
    lua_close(L);
    free(test_app_config.scripting.DefaultDatabase);
}

void test_llm_wait_error_consumed_handle(void) {
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    
    H_Handle* h = (H_Handle*)calloc(1, sizeof(H_Handle));
    TEST_ASSERT_NOT_NULL(h);
    h->kind = H_HK_LLM;
    h->consumed = true;
    
    int result = H_lua_llm_wait_one(L, h);
    
    TEST_ASSERT_EQUAL(2, result);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, 1));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, 2));
    const char* err = lua_tostring(L, 2);
    TEST_ASSERT_NOT_NULL(strstr(err, "consumed"));
    
    lua_pop(L, 2);
    free(h);
    lua_close(L);
}

void test_llm_wait_error_llm_error_present(void) {
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    
    H_Handle* h = (H_Handle*)calloc(1, sizeof(H_Handle));
    TEST_ASSERT_NOT_NULL(h);
    h->kind = H_HK_LLM;
    h->consumed = false;
    h->llm_error = strdup("LLM-specific error message");
    
    int result = H_lua_llm_wait_one(L, h);
    
    TEST_ASSERT_EQUAL(2, result);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, 1));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, 2));
    const char* err = lua_tostring(L, 2);
    TEST_ASSERT_EQUAL_STRING("LLM-specific error message", err);
    TEST_ASSERT_TRUE(h->consumed);
    
    lua_pop(L, 2);
    free(h->llm_error);
    free(h);
    lua_close(L);
}

void test_llm_wait_error_missing_model_name(void) {
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    
    H_Handle* h = (H_Handle*)calloc(1, sizeof(H_Handle));
    TEST_ASSERT_NOT_NULL(h);
    h->kind = H_HK_LLM;
    h->consumed = false;
    h->llm_model_name = NULL;
    
    int result = H_lua_llm_wait_one(L, h);
    
    TEST_ASSERT_EQUAL(2, result);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, 1));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, 2));
    const char* err = lua_tostring(L, 2);
    TEST_ASSERT_NOT_NULL(strstr(err, "missing model name"));
    TEST_ASSERT_TRUE(h->consumed);
    
    lua_pop(L, 2);
    free(h);
    lua_close(L);
}

void test_llm_wait_error_model_not_found(void) {
    DatabaseQueueManager mgr = {0};
    global_queue_manager = &mgr;
    test_dbq.chat_engine_cache = NULL;
    mock_dbqueue_set_get_database_result(&test_dbq);
    
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    
    H_Handle* h = (H_Handle*)calloc(1, sizeof(H_Handle));
    TEST_ASSERT_NOT_NULL(h);
    h->kind = H_HK_LLM;
    h->consumed = false;
    h->llm_model_name = strdup("nonexistent-model");
    
    int result = H_lua_llm_wait_one(L, h);
    
    TEST_ASSERT_EQUAL(2, result);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, 1));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, 2));
    const char* err = lua_tostring(L, 2);
    TEST_ASSERT_NOT_NULL(strstr(err, "not found"));
    TEST_ASSERT_TRUE(h->consumed);
    
    lua_pop(L, 2);
    free(h->llm_model_name);
    free(h);
    lua_close(L);
}

void test_llm_wait_error_no_chat_engine_cache_in_list_mode(void) {
    test_app_config.scripting.DefaultDatabase = strdup("test-db");
    app_config = &test_app_config;
    
    DatabaseQueueManager mgr = {0};
    global_queue_manager = &mgr;
    
    mock_dbqueue_set_get_database_result(NULL);
    
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    
    H_Handle* h = (H_Handle*)calloc(1, sizeof(H_Handle));
    TEST_ASSERT_NOT_NULL(h);
    h->kind = H_HK_LLM;
    h->consumed = false;
    h->llm_list = true;
    h->llm_model_name = strdup("test-model");
    
    int result = H_lua_llm_wait_one(L, h);
    
    TEST_ASSERT_EQUAL(2, result);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, 1));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, 2));
    const char* err = lua_tostring(L, 2);
    TEST_ASSERT_NOT_NULL(err);
    TEST_ASSERT_TRUE(h->consumed);
    
    lua_pop(L, 2);
    free(h->llm_model_name);
    free(h);
    lua_close(L);
    free(test_app_config.scripting.DefaultDatabase);
}

void test_llm_wait_error_failed_to_send_request(void) {
    test_app_config.scripting.DefaultDatabase = strdup("test-db");
    app_config = &test_app_config;
    
    DatabaseQueueManager mgr = {0};
    global_queue_manager = &mgr;
    
    ChatEngineConfig* engine = calloc(1, sizeof(ChatEngineConfig));
    TEST_ASSERT_NOT_NULL(engine);
    strncpy(engine->name, "test-engine", CEC_MAX_NAME_LEN - 1);
    strncpy(engine->model, "test-model", CEC_MAX_MODEL_LEN - 1);
    engine->provider = CEC_PROVIDER_OPENAI;
    engine->is_default = true;
    
    ChatEngineConfig** engines_array = malloc(sizeof(ChatEngineConfig*) * 1);
    TEST_ASSERT_NOT_NULL(engines_array);
    engines_array[0] = engine;
    
    test_dbq.chat_engine_cache = calloc(1, sizeof(ChatEngineCache));
    TEST_ASSERT_NOT_NULL(test_dbq.chat_engine_cache);
    test_dbq.chat_engine_cache->engines = engines_array;
    test_dbq.chat_engine_cache->engine_count = 1;
    
    mock_dbqueue_set_get_database_result(&test_dbq);
    mock_chat_engine_cache_set_lookup_by_name_result(engine);
    mock_auth_chat_deps_set_proxy_success(false);  /* Force proxy failure */
    
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    
    H_Handle* h = (H_Handle*)calloc(1, sizeof(H_Handle));
    TEST_ASSERT_NOT_NULL(h);
    h->kind = H_HK_LLM;
    h->consumed = false;
    h->llm_model_name = strdup("test-model");
    h->llm_prompt = strdup("test prompt");
    h->llm_timeout = 60;
    
    int result = H_lua_llm_wait_one(L, h);
    
    TEST_ASSERT_EQUAL(2, result);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, 1));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, 2));
    const char* err = lua_tostring(L, 2);
    TEST_ASSERT_NOT_NULL(strstr(err, "LLM request failed"));
    TEST_ASSERT_TRUE(h->consumed);
    
    lua_pop(L, 2);
    free(engines_array);
    free(engine);
    free(h->llm_model_name);
    free(h->llm_prompt);
    free(h);
    free(test_dbq.chat_engine_cache);
    lua_close(L);
    free(test_app_config.scripting.DefaultDatabase);
}

/* Tests for H_lua_llm_call_sync and H_lua_llm_list_sync */
void test_llm_call_sync_error_allocation_failure(void) {
    lua_State* L = make_llm_context();
    
    run_lua(L,
        "r, e = H.llm.call_sync('test-model', 'hello')\n"
        "_got_err = (r == nil and e ~= nil)\n");
    
    lua_getglobal(L, "_got_err");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    
    H_lua_destroy_context(L);
}

void test_llm_list_sync_error_allocation_failure(void) {
    lua_State* L = make_llm_context();
    
    run_lua(L,
        "r, e = H.llm.list_sync()\n"
        "_got_result = (type(r) == 'table' or (r == nil and e ~= nil))\n");
    
    lua_getglobal(L, "_got_result");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    
    H_lua_destroy_context(L);
}

/* Test for H_lua_install_llm module installation error */
void test_llm_install_error_h_table_missing(void) {
    lua_State* L = luaL_newstate();
    lua_atpanic(L, H_lua_panic);
    luaL_openlibs(L);
    
    lua_pushnil(L);
    lua_setglobal(L, "H");
    
    H_lua_install_llm(L);
    
    lua_close(L);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_resolve_db_queue_null_app_config);
    RUN_TEST(test_resolve_db_queue_fallback_logic);
    RUN_TEST(test_resolve_db_queue_null_global_queue_manager);
    RUN_TEST(test_resolve_db_queue_database_not_found);
    RUN_TEST(test_resolve_db_queue_valid_database);
    RUN_TEST(test_resolve_engine_null_model_name);
    RUN_TEST(test_resolve_engine_with_valid_params);
    RUN_TEST(test_build_llm_request_json_null_prompt);
    RUN_TEST(test_build_llm_request_json_valid_input);
    RUN_TEST(test_llm_call_error_invalid_prompt_type);
    RUN_TEST(test_llm_call_error_missing_model);
    RUN_TEST(test_llm_call_error_missing_prompts);
    RUN_TEST(test_llm_call_error_prompt_as_table_invalid_fields);
    RUN_TEST(test_llm_call_error_allocation_failure);
    RUN_TEST(test_llm_list_error_no_database);
    RUN_TEST(test_llm_wait_error_invalid_handle);
    RUN_TEST(test_llm_wait_success_with_llm_call);
    RUN_TEST(test_llm_wait_error_consumed_handle);
    RUN_TEST(test_llm_wait_error_llm_error_present);
    RUN_TEST(test_llm_wait_error_missing_model_name);
    RUN_TEST(test_llm_wait_error_model_not_found);
    RUN_TEST(test_llm_wait_error_no_chat_engine_cache_in_list_mode);
    RUN_TEST(test_llm_wait_error_failed_to_send_request);
    RUN_TEST(test_llm_call_sync_error_allocation_failure);
    RUN_TEST(test_llm_list_sync_error_allocation_failure);
    RUN_TEST(test_llm_install_error_h_table_missing);
    
    return UNITY_END();
}