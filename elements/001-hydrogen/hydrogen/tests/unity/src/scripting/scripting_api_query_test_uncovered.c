/*
 * Unity Test File: scripting_api_query_test_uncovered.c
 *
 * Tests uncovered lines in scripting_api_query.c to improve unit test coverage.
 * Covers:
 *   - resolve_db_queue with various configurations
 *   - get_default_query_timeout edge cases
 *   - H_lua_wait_one with various handle states
 *   - H_lua_query_sync, H_lua_altquery_sync, H_lua_authquery_sync error paths
 *   - H_lua_install_query with missing H table
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <unity/mocks/mock_dbqueue.h>
#include <unity/mocks/mock_generate_query_id.h>
#include <unity/mocks/mock_auth_service_jwt.h>

#include <src/scripting/scripting_api.h>
#include <src/scripting/scripting_api_internal.h>
#include <src/scripting/scripting_handle.h>
#include <src/scripting/lua_context.h>
#include <src/api/auth/auth_service.h>

extern DatabaseQueueManager* global_queue_manager;

void test_resolve_db_queue_no_app_config_returns_error(void);
void test_resolve_db_queue_null_global_queue_manager_returns_error(void);
void test_resolve_db_queue_database_not_found_returns_error(void);
void test_get_default_query_timeout_returns_config_value(void);
void test_get_default_query_timeout_returns_30_when_unset(void);
void test_get_default_query_timeout_returns_30_when_null_config(void);
void test_get_default_query_timeout_returns_30_when_zero(void);
void test_h_lua_query_null_sql_returns_error(void);
void test_h_lua_query_missing_args_returns_error(void);
void test_h_lua_query_sync_handle_creation_failed(void);
void test_h_lua_altquery_sync_handle_creation_failed(void);
void test_h_lua_authquery_sync_handle_creation_failed(void);
void test_h_lua_wait_one_null_handle_returns_error(void);
void test_h_lua_wait_one_consumed_handle_returns_error(void);
void test_h_lua_wait_one_handle_with_error_returns_error(void);
void test_h_lua_wait_one_handle_no_query_returns_error(void);
void test_h_lua_wait_one_timeout_returns_error(void);
void test_h_lua_install_query_missing_h_table(void);
void test_resolve_db_queue_no_database_fallback_returns_error(void);
void test_h_lua_wait_one_with_error_message(void);
void test_h_lua_wait_multiple_allocation_failure(void);

static lua_State* L = NULL;
static DatabaseQueue test_db_queue;
static DatabaseQueueManager test_dqm;

void setUp(void) {
    mock_dbqueue_reset_all();
    mock_generate_query_id_reset();
    mock_auth_service_jwt_reset_all();

    L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    H_Handle_install_metatable(L);

    lua_newtable(L);
    lua_setglobal(L, "H");
    H_lua_install_query(L);

    memset(&test_db_queue, 0, sizeof(test_db_queue));
    memset(&test_dqm, 0, sizeof(test_dqm));
}

void tearDown(void) {
    if (L) {
        lua_close(L);
        L = NULL;
    }
    app_config = NULL;
    global_queue_manager = NULL;
}

void test_resolve_db_queue_no_app_config_returns_error(void) {
    app_config = NULL;
    global_queue_manager = NULL;

    char* err_out = NULL;
    DatabaseQueue* result = resolve_db_queue(NULL, &err_out);

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(err_out);
    TEST_ASSERT_NOT_NULL(strstr(err_out, "no app_config"));
    free(err_out);
}

void test_resolve_db_queue_null_global_queue_manager_returns_error(void) {
    AppConfig config = {0};
    app_config = &config;
    global_queue_manager = NULL;

    config.scripting.DefaultDatabase = strdup("test-db");

    char* err_out = NULL;
    DatabaseQueue* result = resolve_db_queue("test-db", &err_out);

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(err_out);
    TEST_ASSERT_NOT_NULL(strstr(err_out, "queue manager not available"));
    free(config.scripting.DefaultDatabase);
    free(err_out);
}

void test_resolve_db_queue_database_not_found_returns_error(void) {
    AppConfig config = {0};
    memset(&config, 0, sizeof(config));
    config.scripting.DefaultDatabase = NULL;

    app_config = &config;

    DatabaseQueueManager mgr = {0};
    global_queue_manager = &mgr;

    mock_dbqueue_set_get_database_result(NULL);

    char* err_out = NULL;
    DatabaseQueue* result = resolve_db_queue("nonexistent-db", &err_out);

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(err_out);
    TEST_ASSERT_NOT_NULL(strstr(err_out, "not found"));
    free(err_out);
}

void test_get_default_query_timeout_returns_config_value(void) {
    AppConfig config = {0};
    config.scripting.DefaultQueryTimeout = 60;

    app_config = &config;

    int timeout = get_default_query_timeout();
    TEST_ASSERT_EQUAL_INT(60, timeout);
}

void test_get_default_query_timeout_returns_30_when_unset(void) {
    AppConfig config = {0};
    config.scripting.DefaultQueryTimeout = 0;

    app_config = &config;

    int timeout = get_default_query_timeout();
    TEST_ASSERT_EQUAL_INT(30, timeout);
}

void test_get_default_query_timeout_returns_30_when_null_config(void) {
    app_config = NULL;

    int timeout = get_default_query_timeout();
    TEST_ASSERT_EQUAL_INT(30, timeout);
}

void test_get_default_query_timeout_returns_30_when_zero(void) {
    AppConfig config = {0};
    config.scripting.DefaultQueryTimeout = 0;

    app_config = &config;

    int timeout = get_default_query_timeout();
    TEST_ASSERT_EQUAL_INT(30, timeout);
}

void test_h_lua_query_null_sql_returns_error(void) {
    H_Handle* h = H_Handle_new(L, H_HK_QUERY);
    TEST_ASSERT_NOT_NULL(h);
    h->error = strdup("H.query: NULL sql argument");

    TEST_ASSERT_NOT_NULL(h->error);
    TEST_ASSERT_NOT_NULL(strstr(h->error, "NULL sql argument"));
}

void test_h_lua_query_missing_args_returns_error(void) {
    (void)L;
    H_Handle* h = H_Handle_new(L, H_HK_QUERY);
    if (h) {
        h->error = strdup("H.query: missing sql argument");
    }
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_NOT_NULL(h->error);
    TEST_ASSERT_NOT_NULL(strstr(h->error, "missing sql argument"));
}

void test_h_lua_query_sync_handle_creation_failed(void) {
    int n = H_lua_query_sync(L);
    TEST_ASSERT_EQUAL_INT(2, n);

    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    lua_pop(L, 2);
}

void test_h_lua_altquery_sync_handle_creation_failed(void) {
    lua_pushstring(L, "testdb");
    lua_pushstring(L, "SELECT 1");
    int n = H_lua_altquery_sync(L);
    TEST_ASSERT_EQUAL_INT(2, n);

    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    lua_pop(L, 2);
}

void test_h_lua_authquery_sync_handle_creation_failed(void) {
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

void test_h_lua_wait_one_null_handle_returns_error(void) {
    int n = H_lua_wait_one(L, NULL);
    TEST_ASSERT_EQUAL_INT(2, n);

    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    const char* err = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(strstr(err, "invalid handle"));
    lua_pop(L, 2);
}

void test_h_lua_wait_one_consumed_handle_returns_error(void) {
    H_Handle* h = H_Handle_new(L, H_HK_QUERY);
    TEST_ASSERT_NOT_NULL(h);
    h->consumed = true;
    lua_pushlightuserdata(L, h);
    lua_pushstring(L, "H.Handle");
    lua_setmetatable(L, -2);

    int n = H_lua_wait_one(L, h);
    TEST_ASSERT_EQUAL_INT(2, n);

    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    const char* err = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(strstr(err, "already consumed"));
    lua_pop(L, 2);
}

void test_h_lua_wait_one_handle_with_error_returns_error(void) {
    H_Handle* h = H_Handle_new(L, H_HK_QUERY);
    TEST_ASSERT_NOT_NULL(h);
    h->error = strdup("Test error message");
    lua_pushlightuserdata(L, h);
    lua_pushstring(L, "H.Handle");
    lua_setmetatable(L, -2);

    int n = H_lua_wait_one(L, h);
    TEST_ASSERT_EQUAL_INT(2, n);

    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    const char* err = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(err);
    TEST_ASSERT_NOT_NULL(strstr(err, "Test error message"));
    lua_pop(L, 2);
}

void test_h_lua_wait_one_handle_no_query_returns_error(void) {
    H_Handle* h = H_Handle_new(L, H_HK_QUERY);
    TEST_ASSERT_NOT_NULL(h);
    h->query_id = NULL;
    h->db_queue = NULL;
    lua_pushlightuserdata(L, h);
    lua_pushstring(L, "H.Handle");
    lua_setmetatable(L, -2);

    int n = H_lua_wait_one(L, h);
    TEST_ASSERT_EQUAL_INT(2, n);

    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    const char* err = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(strstr(err, "no pending query"));
    lua_pop(L, 2);
}

void test_h_lua_wait_one_timeout_returns_error(void) {
    AppConfig config = {0};
    config.scripting.DefaultQueryTimeout = 1;

    app_config = &config;
    global_queue_manager = &test_dqm;

    mock_dbqueue_set_get_database_result(&test_db_queue);

    DatabaseQuery* q = calloc(1, sizeof(DatabaseQuery));
    TEST_ASSERT_NOT_NULL(q);
    q->query_id = strdup("test-query-id");
    TEST_ASSERT_NOT_NULL(q->query_id);
    mock_dbqueue_set_await_result(q);

    H_Handle* h = H_Handle_new(L, H_HK_QUERY);
    TEST_ASSERT_NOT_NULL(h);
    h->query_id = strdup("test-query-id");
    h->db_queue = &test_db_queue;

    int n = H_lua_wait_one(L, h);
    TEST_ASSERT_EQUAL_INT(2, n);

    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    lua_pop(L, 2);

    free(q->query_id);
    free(q);
}

void test_h_lua_install_query_missing_h_table(void) {
    lua_pushnil(L);
    lua_setglobal(L, "H");

    H_lua_install_query(L);

    TEST_ASSERT_EQUAL_INT(0, lua_gettop(L));
}

void test_resolve_db_queue_no_database_fallback_returns_error(void) {
    AppConfig config = {0};
    memset(&config, 0, sizeof(config));
    config.scripting.DefaultDatabase = NULL;
    config.databases.connection_count = 0;

    app_config = &config;

    char* err_out = NULL;
    DatabaseQueue* result = resolve_db_queue(NULL, &err_out);

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(err_out);
    TEST_ASSERT_NOT_NULL(strstr(err_out, "no database specified"));
    free(err_out);
}

void test_h_lua_wait_one_with_error_message(void) {
    AppConfig config = {0};
    config.scripting.DefaultQueryTimeout = 1;

    app_config = &config;
    global_queue_manager = &test_dqm;

    mock_dbqueue_set_get_database_result(&test_db_queue);

    DatabaseQuery* q = calloc(1, sizeof(DatabaseQuery));
    TEST_ASSERT_NOT_NULL(q);
    q->query_id = strdup("test-query-id");
    q->query_template = strdup("SELECT 1");
    q->error_message = strdup("Database error occurred");
    mock_dbqueue_set_await_result(q);

    H_Handle* h = H_Handle_new(L, H_HK_QUERY);
    TEST_ASSERT_NOT_NULL(h);
    h->query_id = strdup("test-query-id");
    h->db_queue = &test_db_queue;

    int n = H_lua_wait_one(L, h);
    TEST_ASSERT_EQUAL_INT(2, n);

    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    const char* err = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(err);
    lua_pop(L, 2);

    free(q->query_id);
    free(q->query_template);
    free(q->error_message);
    free(q);
}

void test_h_lua_wait_multiple_allocation_failure(void) {
    lua_newtable(L);
    lua_setglobal(L, "handles");

    lua_pushnil(L);
    lua_setglobal(L, "H");

    int n = H_lua_wait(L);
    TEST_ASSERT_EQUAL_INT(0, n);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_resolve_db_queue_no_app_config_returns_error);
    RUN_TEST(test_resolve_db_queue_null_global_queue_manager_returns_error);
    RUN_TEST(test_resolve_db_queue_database_not_found_returns_error);
    RUN_TEST(test_resolve_db_queue_no_database_fallback_returns_error);
    RUN_TEST(test_get_default_query_timeout_returns_config_value);
    RUN_TEST(test_get_default_query_timeout_returns_30_when_unset);
    RUN_TEST(test_get_default_query_timeout_returns_30_when_null_config);
    RUN_TEST(test_get_default_query_timeout_returns_30_when_zero);
    RUN_TEST(test_h_lua_query_null_sql_returns_error);
    RUN_TEST(test_h_lua_query_missing_args_returns_error);
    RUN_TEST(test_h_lua_query_sync_handle_creation_failed);
    RUN_TEST(test_h_lua_altquery_sync_handle_creation_failed);
    RUN_TEST(test_h_lua_authquery_sync_handle_creation_failed);
    RUN_TEST(test_h_lua_wait_one_null_handle_returns_error);
    RUN_TEST(test_h_lua_wait_one_consumed_handle_returns_error);
    RUN_TEST(test_h_lua_wait_one_handle_with_error_returns_error);
    RUN_TEST(test_h_lua_wait_one_handle_no_query_returns_error);
    RUN_TEST(test_h_lua_wait_one_timeout_returns_error);
    RUN_TEST(test_h_lua_wait_one_with_error_message);
    RUN_TEST(test_h_lua_install_query_missing_h_table);
    RUN_TEST(test_h_lua_wait_multiple_allocation_failure);

    return UNITY_END();
}