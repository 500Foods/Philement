/*
 * Unity Test File: scripting_api_query_resolve_test_get_default_query_timeout
 *
 * Tests uncovered lines in scripting_api_query_resolve.c for get_default_query_timeout.
 * Covers: config value set, unset/zero value, null config.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <src/scripting/scripting_api.h>
#include <src/scripting/scripting_api_internal.h>
#include <src/scripting/scripting_handle.h>
#include <src/scripting/lua_context.h>

void test_get_default_query_timeout_returns_config_value(void);
void test_get_default_query_timeout_returns_30_when_unset(void);
void test_get_default_query_timeout_returns_30_when_null_config(void);
void test_get_default_query_timeout_returns_30_when_zero(void);

void setUp(void) {
    app_config = NULL;
}

void tearDown(void) {
    app_config = NULL;
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

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_get_default_query_timeout_returns_config_value);
    RUN_TEST(test_get_default_query_timeout_returns_30_when_unset);
    RUN_TEST(test_get_default_query_timeout_returns_30_when_null_config);
    RUN_TEST(test_get_default_query_timeout_returns_30_when_zero);

    return UNITY_END();
}