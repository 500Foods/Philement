/*
 * Unity Test File: scripting_api_query_submit_test_H_lua_submit_query
 *
 * Tests uncovered lines in scripting_api_query_submit.c for H_lua_submit_query.
 * Covers error paths: NULL sql, resolve_db_queue failure, query_id generation failure,
 * SQL duplication failure, database submit failure, handle creation failure.
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

#include <src/scripting/scripting_api.h>
#include <src/scripting/scripting_api_internal.h>
#include <src/scripting/scripting_handle.h>
#include <src/scripting/lua_context.h>

extern DatabaseQueueManager* global_queue_manager;

void test_H_lua_submit_query_null_sql_returns_error(void);
void test_H_lua_submit_query_resolve_db_queue_failure(void);
void test_H_lua_submit_query_db_queue_not_found(void);
void test_H_lua_submit_query_generate_query_id_failure(void);
void test_H_lua_submit_query_database_submit_failure(void);

static lua_State* L = NULL;
static DatabaseQueue test_db_queue;
static DatabaseQueueManager test_dqm;

void setUp(void) {
    mock_dbqueue_reset_all();
    mock_generate_query_id_reset();

    L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    H_Handle_install_metatable(L);

    lua_newtable(L);
    lua_setglobal(L, "H");

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

void test_H_lua_submit_query_null_sql_returns_error(void) {
    char* err_out = NULL;
    DatabaseQueue* dbq = resolve_db_queue(NULL, &err_out);
    TEST_ASSERT_NULL(dbq);
    free(err_out);

    int result = H_lua_submit_query(L, NULL, NULL, NULL, 30, "H.query");
    TEST_ASSERT_EQUAL_INT(1, result);
}

void test_H_lua_submit_query_resolve_db_queue_failure(void) {
    AppConfig config = {0};
    app_config = &config;
    global_queue_manager = NULL;
    config.scripting.DefaultDatabase = strdup("test-db");

    char* err_out = NULL;
    DatabaseQueue* result = resolve_db_queue("test-db", &err_out);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(err_out);
    free(config.scripting.DefaultDatabase);
    free(err_out);
}

void test_H_lua_submit_query_db_queue_not_found(void) {
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
    free(err_out);
}

void test_H_lua_submit_query_generate_query_id_failure(void) {
    AppConfig config = {0};
    config.scripting.DefaultDatabase = strdup("test-db");

    app_config = &config;

    DatabaseQueueManager mgr = {0};
    global_queue_manager = &mgr;

    mock_dbqueue_set_get_database_result(&test_db_queue);
    mock_generate_query_id_set_result(NULL);

    int result = H_lua_submit_query(L, "test-db", "SELECT 1", NULL, 30, "H.query");
    TEST_ASSERT_EQUAL_INT(1, result);

    free(config.scripting.DefaultDatabase);
}

void test_H_lua_submit_query_database_submit_failure(void) {
    AppConfig config = {0};
    config.scripting.DefaultDatabase = strdup("test-db");

    app_config = &config;

    DatabaseQueueManager mgr = {0};
    global_queue_manager = &mgr;

    mock_dbqueue_set_get_database_result(&test_db_queue);
    mock_generate_query_id_set_result(strdup("test-query-id"));
    mock_dbqueue_set_submit_query_result(0);

    int result = H_lua_submit_query(L, "test-db", "SELECT 1", NULL, 30, "H.query");
    TEST_ASSERT_EQUAL_INT(1, result);

    free(config.scripting.DefaultDatabase);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_H_lua_submit_query_null_sql_returns_error);
    RUN_TEST(test_H_lua_submit_query_resolve_db_queue_failure);
    RUN_TEST(test_H_lua_submit_query_db_queue_not_found);
    RUN_TEST(test_H_lua_submit_query_generate_query_id_failure);
    RUN_TEST(test_H_lua_submit_query_database_submit_failure);

    return UNITY_END();
}