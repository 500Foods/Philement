/*
 * Unity Test File: scripting_api_query_submit_test_H_lua_authquery
 *
 * Tests H_lua_authquery: missing args, JWT failures, params, timeout, success.
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

#include <src/scripting/scripting_api.h>
#include <src/scripting/scripting_api_internal.h>
#include <src/scripting/scripting_handle.h>
#include <src/scripting/lua_context.h>
#include <src/api/auth/auth_service.h>

extern DatabaseQueueManager* global_queue_manager;

void test_H_lua_authquery_missing_args_returns_error(void);
void test_H_lua_authquery_jwt_invalid(void);
void test_H_lua_authquery_with_params_and_timeout(void);
void test_H_lua_authquery_success(void);

static lua_State* L = NULL;
static DatabaseQueue test_db_queue;
static DatabaseQueueManager test_dqm;
static AppConfig test_config;

static void setup_ok(void) {
    memset(&test_config, 0, sizeof(test_config));
    test_config.scripting.DefaultQueryTimeout = 20;
    app_config = &test_config;
    memset(&test_dqm, 0, sizeof(test_dqm));
    global_queue_manager = &test_dqm;
    mock_dbqueue_set_get_database_result(&test_db_queue);
    mock_dbqueue_set_submit_query_result(true);
    mock_generate_query_id_set_result("auth-1");
}

static void set_jwt_db(const char* db) {
    jwt_claims_t claims = {0};
    claims.database = db ? (char*)db : NULL;
    jwt_validation_result_t result = {0};
    result.valid = true;
    result.claims = &claims;
    mock_auth_service_jwt_set_validation_result(result);
}

void setUp(void) {
    mock_dbqueue_reset_all();
    mock_generate_query_id_reset();
    mock_auth_service_jwt_reset_all();

    L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    H_Handle_install_metatable(L);

    memset(&test_db_queue, 0, sizeof(test_db_queue));
    memset(&test_config, 0, sizeof(test_config));
    app_config = NULL;
    global_queue_manager = NULL;
}

void tearDown(void) {
    if (L) {
        lua_close(L);
        L = NULL;
    }
    app_config = NULL;
    global_queue_manager = NULL;
    mock_dbqueue_reset_all();
    mock_generate_query_id_reset();
    mock_auth_service_jwt_reset_all();
}

void test_H_lua_authquery_missing_args_returns_error(void) {
    int n = H_lua_authquery(L);
    TEST_ASSERT_EQUAL_INT(1, n);
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_NOT_NULL(strstr(h->error, "missing token or sql"));
}

void test_H_lua_authquery_jwt_invalid(void) {
    jwt_validation_result_t result = {0};
    result.valid = false;
    result.error = 3;
    mock_auth_service_jwt_set_validation_result(result);

    lua_pushstring(L, "bad.token");
    lua_pushstring(L, "SELECT 1");
    int n = H_lua_authquery(L);
    TEST_ASSERT_EQUAL_INT(1, n);
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_NOT_NULL(strstr(h->error, "JWT validation failed"));
}

void test_H_lua_authquery_with_params_and_timeout(void) {
    setup_ok();
    set_jwt_db("jwtdb");

    lua_pushstring(L, "good.jwt.token");
    lua_pushstring(L, "SELECT $1");
    lua_newtable(L);
    lua_pushboolean(L, 1);
    lua_setfield(L, -2, "active");
    lua_newtable(L);
    lua_pushinteger(L, 9);
    lua_setfield(L, -2, "timeout");

    int n = H_lua_authquery(L);
    TEST_ASSERT_EQUAL_INT(1, n);
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_NULL(h->error);
    TEST_ASSERT_EQUAL_STRING("auth-1", h->query_id);
}

void test_H_lua_authquery_success(void) {
    setup_ok();
    set_jwt_db("jwtdb");

    lua_pushstring(L, "good.jwt.token");
    lua_pushstring(L, "SELECT 1");
    int n = H_lua_authquery(L);
    TEST_ASSERT_EQUAL_INT(1, n);
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_NULL(h->error);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_H_lua_authquery_missing_args_returns_error);
    RUN_TEST(test_H_lua_authquery_jwt_invalid);
    RUN_TEST(test_H_lua_authquery_with_params_and_timeout);
    RUN_TEST(test_H_lua_authquery_success);

    return UNITY_END();
}
