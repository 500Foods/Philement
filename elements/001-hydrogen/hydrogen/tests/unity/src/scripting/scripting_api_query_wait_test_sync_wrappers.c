/*
 * Unity Test File: scripting_api_query_wait_test_sync_wrappers
 *
 * Tests sync wrappers and H_lua_finish_sync_wait helper paths.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define USE_MOCK_DBQUEUE
#include <unity/mocks/mock_dbqueue.h>
#define USE_MOCK_GENERATE_QUERY_ID
#include <unity/mocks/mock_generate_query_id.h>
#define USE_MOCK_AUTH_SERVICE_JWT
#include <unity/mocks/mock_auth_service_jwt.h>
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#include <src/scripting/scripting_api.h>
#include <src/scripting/scripting_api_internal.h>
#include <src/scripting/scripting_handle.h>
#include <src/scripting/lua_context.h>
#include <src/api/auth/auth_service.h>

extern DatabaseQueueManager* global_queue_manager;

void test_H_lua_query_sync_propagates_error(void);
void test_H_lua_altquery_sync_propagates_error(void);
void test_H_lua_authquery_sync_propagates_error(void);
void test_H_lua_finish_sync_wait_alloc_failed(void);
void test_H_lua_finish_sync_wait_create_failed(void);
void test_H_lua_finish_sync_wait_success_path(void);
void test_H_lua_query_sync_after_submit_wait(void);
void test_H_lua_altquery_sync_after_submit_wait(void);
void test_H_lua_authquery_sync_after_submit_wait(void);

static lua_State* L = NULL;
static DatabaseQueue test_db_queue;
static DatabaseQueueManager test_dqm;
static AppConfig test_config;

static void setup_submit_ok(void) {
    memset(&test_config, 0, sizeof(test_config));
    test_config.scripting.DefaultDatabase = strdup("testdb");
    test_config.scripting.DefaultQueryTimeout = 2;
    app_config = &test_config;
    global_queue_manager = &test_dqm;
    mock_dbqueue_set_get_database_result(&test_db_queue);
    mock_dbqueue_set_submit_query_result(true);
    mock_generate_query_id_set_result("qid-sync-1");
}

void setUp(void) {
    mock_dbqueue_reset_all();
    mock_generate_query_id_reset();
    mock_auth_service_jwt_reset_all();
    mock_system_reset_all();

    L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    H_Handle_install_metatable(L);

    lua_newtable(L);
    lua_setglobal(L, "H");

    memset(&test_db_queue, 0, sizeof(test_db_queue));
    memset(&test_dqm, 0, sizeof(test_dqm));
    app_config = NULL;
    global_queue_manager = NULL;
}

void tearDown(void) {
    if (L) {
        lua_close(L);
        L = NULL;
    }
    free(test_config.scripting.DefaultDatabase);
    test_config.scripting.DefaultDatabase = NULL;
    app_config = NULL;
    global_queue_manager = NULL;
    mock_dbqueue_reset_all();
    mock_generate_query_id_reset();
    mock_auth_service_jwt_reset_all();
    mock_system_reset_all();
}

void test_H_lua_query_sync_propagates_error(void) {
    int n = H_lua_query_sync(L);
    TEST_ASSERT_EQUAL_INT(2, n);

    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    lua_pop(L, 2);
}

void test_H_lua_altquery_sync_propagates_error(void) {
    lua_pushstring(L, "testdb");
    lua_pushstring(L, "SELECT 1");
    int n = H_lua_altquery_sync(L);
    TEST_ASSERT_EQUAL_INT(2, n);

    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    lua_pop(L, 2);
}

void test_H_lua_authquery_sync_propagates_error(void) {
    jwt_claims_t claims = {0};
    claims.database = strdup("testdb");

    jwt_validation_result_t result = {0};
    result.valid = true;
    result.claims = &claims;

    mock_auth_service_jwt_set_validation_result(result);

    lua_pushstring(L, "valid.jwt.token");
    lua_pushstring(L, "SELECT 1");
    int n = H_lua_authquery_sync(L);
    TEST_ASSERT_EQUAL_INT(2, n);

    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    lua_pop(L, 2);

    free(claims.database);
}

void test_H_lua_finish_sync_wait_alloc_failed(void) {
    int n = H_lua_finish_sync_wait(L, 0,
                                   "H.query_sync: handle allocation failed",
                                   "H.query_sync: handle creation failed");
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    TEST_ASSERT_NOT_NULL(strstr(lua_tostring(L, -1), "handle allocation failed"));
    lua_pop(L, 2);
}

void test_H_lua_finish_sync_wait_create_failed(void) {
    lua_pushstring(L, "not-a-handle");
    int n = H_lua_finish_sync_wait(L, 1,
                                   "H.altquery_sync: handle allocation failed",
                                   "H.altquery_sync: handle creation failed");
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    TEST_ASSERT_NOT_NULL(strstr(lua_tostring(L, -1), "handle creation failed"));
    lua_pop(L, 2);
}

void test_H_lua_finish_sync_wait_success_path(void) {
    AppConfig config = {0};
    config.scripting.DefaultQueryTimeout = 2;
    app_config = &config;

    DatabaseQuery q = {0};
    q.query_id = (char*)"sync-ok";
    q.query_template = (char*)"[]";
    q.affected_rows = 3;
    mock_dbqueue_set_await_result(&q);

    H_Handle* h = H_Handle_new(L, H_HK_QUERY);
    TEST_ASSERT_NOT_NULL(h);
    h->query_id = strdup("sync-ok");
    h->db_queue = &test_db_queue;

    int n = H_lua_finish_sync_wait(L, 1,
                                   "alloc",
                                   "create");
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL(LUA_TTABLE, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -1));
    TEST_ASSERT_TRUE(h->consumed);
    lua_pop(L, 2);
}

void test_H_lua_query_sync_after_submit_wait(void) {
    setup_submit_ok();

    DatabaseQuery q = {0};
    q.query_id = (char*)"qid-sync-1";
    q.query_template = (char*)"[{\"a\":1}]";
    q.affected_rows = 1;
    mock_dbqueue_set_await_result(&q);

    lua_pushstring(L, "SELECT 1");
    int n = H_lua_query_sync(L);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL(LUA_TTABLE, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -1));
    lua_pop(L, 2);
}

void test_H_lua_altquery_sync_after_submit_wait(void) {
    setup_submit_ok();

    DatabaseQuery q = {0};
    q.query_id = (char*)"qid-sync-1";
    q.query_template = (char*)"[]";
    mock_dbqueue_set_await_result(&q);

    lua_pushstring(L, "testdb");
    lua_pushstring(L, "SELECT 1");
    int n = H_lua_altquery_sync(L);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL(LUA_TTABLE, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -1));
    lua_pop(L, 2);
}

void test_H_lua_authquery_sync_after_submit_wait(void) {
    setup_submit_ok();

    jwt_claims_t claims = {0};
    claims.database = strdup("testdb");
    jwt_validation_result_t result = {0};
    result.valid = true;
    result.claims = &claims;
    mock_auth_service_jwt_set_validation_result(result);

    DatabaseQuery q = {0};
    q.query_id = (char*)"qid-sync-1";
    q.query_template = (char*)"[]";
    mock_dbqueue_set_await_result(&q);

    lua_pushstring(L, "valid.jwt.token");
    lua_pushstring(L, "SELECT 1");
    int n = H_lua_authquery_sync(L);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL(LUA_TTABLE, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -1));
    lua_pop(L, 2);

    free(claims.database);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_H_lua_query_sync_propagates_error);
    RUN_TEST(test_H_lua_altquery_sync_propagates_error);
    RUN_TEST(test_H_lua_authquery_sync_propagates_error);
    RUN_TEST(test_H_lua_finish_sync_wait_alloc_failed);
    RUN_TEST(test_H_lua_finish_sync_wait_create_failed);
    RUN_TEST(test_H_lua_finish_sync_wait_success_path);
    RUN_TEST(test_H_lua_query_sync_after_submit_wait);
    RUN_TEST(test_H_lua_altquery_sync_after_submit_wait);
    RUN_TEST(test_H_lua_authquery_sync_after_submit_wait);

    return UNITY_END();
}
