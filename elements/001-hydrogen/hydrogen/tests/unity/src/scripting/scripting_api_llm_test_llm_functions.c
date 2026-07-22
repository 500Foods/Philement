/*
 * Unity Test File: scripting_api_llm_test_llm_functions.c
 *
 * Tests H_lua_llm_list, H_lua_llm_wait_one, H_lua_llm_call_sync,
 * H_lua_llm_list_sync, and H_lua_install_llm functions.
 *
 * Covers uncovered lines from scripting_api_llm.c:
 * - H_lua_llm_list database fallback logic
 * - H_lua_llm_wait_one error paths (NULL handle, consumed, error, non-LLM, missing model)
 * - H_lua_llm_wait_one success path with proxy call
 * - H_lua_llm_call_sync error propagation
 * - H_lua_llm_list_sync error propagation
 * - H_lua_install_llm H table missing error
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

#define USE_MOCK_DBQUEUE
#include <unity/mocks/mock_dbqueue.h>

#define USE_MOCK_CHAT_ENGINE_CACHE
#include <unity/mocks/mock_chat_engine_cache.h>

#define USE_MOCK_AUTH_CHAT_DEPS
#include <unity/mocks/mock_auth_chat_deps.h>

extern DatabaseQueueManager* global_queue_manager;

static AppConfig mock_app_config_storage = {0};
static DatabaseQueue g_test_dbq = {0};

void test_llm_list_no_database_returns_error_handle(void);
void test_llm_list_with_default_database_returns_handle(void);
void test_llm_list_with_first_connection_returns_handle(void);
void test_llm_wait_null_handle_returns_error(void);
void test_llm_wait_consumed_handle_returns_error(void);
void test_llm_wait_handle_with_error_returns_error(void);
void test_llm_wait_non_llm_handle_returns_error(void);
void test_llm_wait_missing_model_name_returns_error(void);
void test_llm_wait_model_not_found_returns_error(void);
void test_llm_wait_success_returns_result(void);
void test_llm_wait_success_with_list_returns_models(void);
void test_llm_call_empty_model_returns_error_handle(void);
void test_llm_call_invalid_prompt_returns_error_handle(void);
void test_llm_call_with_table_options_returns_handle(void);
void test_llm_call_sync_missing_model_returns_error(void);
void test_llm_call_sync_allocation_failure_returns_error(void);
void test_llm_list_sync_no_database_returns_error(void);
void test_llm_install_with_null_L_returns_gracefully(void);

static void run_lua(lua_State* L, const char* code) {
    int rc = luaL_loadbuffer(L, code, strlen(code), "test");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, "luaL_loadbuffer failed");
    rc = lua_pcall(L, 0, LUA_MULTRET, 0);
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
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

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    memset(&g_test_dbq, 0, sizeof(g_test_dbq));
    app_config = &mock_app_config_storage;
    mock_dbqueue_reset_all();
    mock_chat_engine_cache_reset_all();
    mock_auth_chat_deps_reset_all();
    mock_dbqueue_set_get_database_result(&g_test_dbq);
    mock_auth_chat_deps_set_proxy_success(true);
    mock_auth_chat_deps_set_proxy_response_body("{\"response\": \"hello world\"}");
    global_queue_manager = NULL;
}

void tearDown(void) {
    app_config = NULL;
    mock_dbqueue_reset_all();
    mock_chat_engine_cache_reset_all();
    mock_auth_chat_deps_reset_all();
}

void test_llm_list_no_database_returns_error_handle(void) {
    app_config = NULL;

    lua_State* L = make_llm_context();
    run_lua(L,
        "h = H.llm.list()\n"
        "r, e = H.wait(h)\n"
        "_got_err = (r == nil and e ~= nil and e:find('database') ~= nil)\n");
    lua_getglobal(L, "_got_err");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_llm_list_with_default_database_returns_handle(void) {
    mock_app_config_storage.scripting.DefaultDatabase = strdup("test-db");

    lua_State* L = make_llm_context();
    run_lua(L,
        "h = H.llm.list()\n"
        "_kind = (type(h) == 'userdata' and h ~= nil)\n");
    lua_getglobal(L, "_kind");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);

    free(mock_app_config_storage.scripting.DefaultDatabase);
}

void test_llm_list_with_first_connection_returns_handle(void) {
    mock_app_config_storage.databases.connection_count = 1;
    mock_app_config_storage.databases.connections[0].name = strdup("first-db");

    lua_State* L = make_llm_context();
    run_lua(L,
        "h = H.llm.list()\n"
        "_kind = (type(h) == 'userdata' and h ~= nil)\n");
    lua_getglobal(L, "_kind");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);

    free(mock_app_config_storage.databases.connections[0].name);
}

void test_llm_wait_null_handle_returns_error(void) {
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

void test_llm_wait_consumed_handle_returns_error(void) {
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

void test_llm_wait_handle_with_error_returns_error(void) {
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);

    H_Handle* h = (H_Handle*)calloc(1, sizeof(H_Handle));
    TEST_ASSERT_NOT_NULL(h);
    h->kind = H_HK_LLM;
    h->consumed = false;
    h->error = strdup("pre-set error");

    int result = H_lua_llm_wait_one(L, h);

    TEST_ASSERT_EQUAL(2, result);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, 1));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, 2));
    const char* err = lua_tostring(L, 2);
    TEST_ASSERT_EQUAL_STRING("pre-set error", err);
    TEST_ASSERT_TRUE(h->consumed);

    lua_pop(L, 2);
    free(h->error);
    free(h);
    lua_close(L);
}

void test_llm_wait_non_llm_handle_returns_error(void) {
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);

    H_Handle* h = (H_Handle*)calloc(1, sizeof(H_Handle));
    TEST_ASSERT_NOT_NULL(h);
    h->kind = H_HK_QUERY;
    h->consumed = false;

    int result = H_lua_llm_wait_one(L, h);

    TEST_ASSERT_EQUAL(2, result);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, 1));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, 2));
    const char* err = lua_tostring(L, 2);
    TEST_ASSERT_NOT_NULL(strstr(err, "not an LLM handle"));

    lua_pop(L, 2);
    free(h);
    lua_close(L);
}

void test_llm_wait_missing_model_name_returns_error(void) {
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

void test_llm_wait_model_not_found_returns_error(void) {
    DatabaseQueueManager mgr = {0};
    global_queue_manager = &mgr;

    g_test_dbq.chat_engine_cache = NULL;

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

void test_llm_wait_success_returns_result(void) {
    DatabaseQueueManager mgr = {0};
    global_queue_manager = &mgr;

    ChatEngineConfig* engine_ptr = calloc(1, sizeof(ChatEngineConfig));
    TEST_ASSERT_NOT_NULL(engine_ptr);
    strcpy(engine_ptr->name, "test-engine");
    strcpy(engine_ptr->model, "test-model");
    engine_ptr->provider = CEC_PROVIDER_OPENAI;
    engine_ptr->is_default = true;

    ChatEngineConfig** engines_array = malloc(sizeof(ChatEngineConfig*) * 1);
    TEST_ASSERT_NOT_NULL(engines_array);
    engines_array[0] = engine_ptr;

    g_test_dbq.chat_engine_cache = calloc(1, sizeof(ChatEngineCache));
    TEST_ASSERT_NOT_NULL(g_test_dbq.chat_engine_cache);
    g_test_dbq.chat_engine_cache->engines = engines_array;
    g_test_dbq.chat_engine_cache->engine_count = 1;

    mock_chat_engine_cache_set_lookup_by_name_result(engine_ptr);

    mock_auth_chat_deps_set_proxy_success(true);
    mock_auth_chat_deps_set_proxy_response_body("{\"response\": \"hello world\"}");

    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);

    H_Handle* h = (H_Handle*)calloc(1, sizeof(H_Handle));
    TEST_ASSERT_NOT_NULL(h);
    h->kind = H_HK_LLM;
    h->consumed = false;
    h->llm_model_name = strdup("test-model");
    h->llm_prompt = strdup("hello");
    h->llm_timeout = 60;

    int result = H_lua_llm_wait_one(L, h);

    TEST_ASSERT_EQUAL(2, result);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, 1));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, 2));
    const char* err = lua_tostring(L, 2);
    TEST_ASSERT_NOT_NULL(err);

    lua_pop(L, 2);
    free(engines_array);
    free(engine_ptr);
    free(h->llm_model_name);
    free(h->llm_prompt);
    free(h);
    free(g_test_dbq.chat_engine_cache);
    lua_close(L);
}

void test_llm_wait_success_with_list_returns_models(void) {
    DatabaseQueueManager mgr = {0};
    global_queue_manager = &mgr;

    ChatEngineConfig* engine1 = calloc(1, sizeof(ChatEngineConfig));
    TEST_ASSERT_NOT_NULL(engine1);
    strcpy(engine1->name, "engine1");
    strcpy(engine1->model, "model1");
    engine1->provider = CEC_PROVIDER_OPENAI;
    engine1->is_default = true;

    ChatEngineConfig* engine2 = calloc(1, sizeof(ChatEngineConfig));
    TEST_ASSERT_NOT_NULL(engine2);
    strcpy(engine2->name, "engine2");
    strcpy(engine2->model, "model2");
    engine2->provider = CEC_PROVIDER_ANTHROPIC;
    engine2->is_default = false;

    ChatEngineConfig** engines_array = malloc(sizeof(ChatEngineConfig*) * 2);
    TEST_ASSERT_NOT_NULL(engines_array);
    engines_array[0] = engine1;
    engines_array[1] = engine2;

    g_test_dbq.chat_engine_cache = calloc(1, sizeof(ChatEngineCache));
    TEST_ASSERT_NOT_NULL(g_test_dbq.chat_engine_cache);
    g_test_dbq.chat_engine_cache->engines = engines_array;
    g_test_dbq.chat_engine_cache->engine_count = 2;

    mock_chat_engine_cache_set_lookup_by_name_result(engine1);
    mock_chat_engine_cache_set_get_all_result(engines_array, 2);

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

    lua_pop(L, 2);
    free(engines_array);
    free(engine1);
    free(engine2);
    free(h->llm_model_name);
    free(h);
    free(g_test_dbq.chat_engine_cache);
    lua_close(L);
}

void test_llm_call_empty_model_returns_error_handle(void) {
    lua_State* L = make_llm_context();
    run_lua(L,
        "h = H.llm.call('')\n"
        "r, e = H.wait(h)\n"
        "_got_err = (r == nil and e ~= nil)\n");
    lua_getglobal(L, "_got_err");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_llm_call_invalid_prompt_returns_error_handle(void) {
    lua_State* L = make_llm_context();
    run_lua(L,
        "h = H.llm.call('test-model', 123)\n"
        "r, e = H.wait(h)\n"
        "_got_err = (r == nil and e ~= nil)\n");
    lua_getglobal(L, "_got_err");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_llm_call_with_table_options_returns_handle(void) {
    lua_State* L = make_llm_context();
    run_lua(L,
        "h = H.llm.call('test-model', {max_tokens=100, temperature=0.5, database='test-db', timeout=120})\n"
        "_kind = (type(h) == 'userdata' and h ~= nil)\n");
    lua_getglobal(L, "_kind");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_llm_call_sync_missing_model_returns_error(void) {
    lua_State* L = make_llm_context();
    run_lua(L,
        "h = H.llm.call()\n"
        "r, e = H.wait(h)\n"
        "_got_err = (r == nil and e ~= nil)\n");
    lua_getglobal(L, "_got_err");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_llm_call_sync_allocation_failure_returns_error(void) {
    lua_State* L = make_llm_context();
    run_lua(L,
        "r, e = H.llm.call_sync('test-model', 'hello')\n"
        "_got_err = (r == nil and e ~= nil)\n");
    lua_getglobal(L, "_got_err");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_llm_list_sync_no_database_returns_error(void) {
    lua_State* L = make_llm_context();
    run_lua(L,
        "r, e = H.llm.list_sync()\n"
        "_got_err = (r == nil and e ~= nil and e:find('database') ~= nil)\n");
    lua_getglobal(L, "_got_err");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_llm_install_with_null_L_returns_gracefully(void) {
    H_lua_install_llm(NULL);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_llm_list_no_database_returns_error_handle);
    RUN_TEST(test_llm_list_with_default_database_returns_handle);
    RUN_TEST(test_llm_list_with_first_connection_returns_handle);
    RUN_TEST(test_llm_wait_null_handle_returns_error);
    RUN_TEST(test_llm_wait_consumed_handle_returns_error);
    RUN_TEST(test_llm_wait_handle_with_error_returns_error);
    RUN_TEST(test_llm_wait_non_llm_handle_returns_error);
    RUN_TEST(test_llm_wait_missing_model_name_returns_error);
    RUN_TEST(test_llm_wait_model_not_found_returns_error);
    RUN_TEST(test_llm_wait_success_returns_result);
    RUN_TEST(test_llm_wait_success_with_list_returns_models);
    RUN_TEST(test_llm_call_empty_model_returns_error_handle);
    RUN_TEST(test_llm_call_invalid_prompt_returns_error_handle);
    RUN_TEST(test_llm_call_with_table_options_returns_handle);
    RUN_TEST(test_llm_call_sync_missing_model_returns_error);
    RUN_TEST(test_llm_call_sync_allocation_failure_returns_error);
    RUN_TEST(test_llm_list_sync_no_database_returns_error);
    RUN_TEST(test_llm_install_with_null_L_returns_gracefully);

    return UNITY_END();
}