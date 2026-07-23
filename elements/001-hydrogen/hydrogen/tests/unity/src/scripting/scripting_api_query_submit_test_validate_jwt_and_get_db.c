/*
 * Unity Test File: scripting_api_query_submit_test_validate_jwt_and_get_db
 *
 * Tests uncovered lines in scripting_api_query_submit.c for validate_jwt_and_get_db.
 * Covers: empty token, null token.
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
#include <src/api/auth/auth_service.h>

void test_validate_jwt_and_get_db_empty_token_returns_error(void);
void test_validate_jwt_and_get_db_null_token_returns_error(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_validate_jwt_and_get_db_empty_token_returns_error(void) {
    char* err_out = NULL;
    char* result = validate_jwt_and_get_db("", &err_out);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(err_out);
    TEST_ASSERT_NOT_NULL(strstr(err_out, "empty token"));
    free(err_out);
}

void test_validate_jwt_and_get_db_null_token_returns_error(void) {
    char* err_out = NULL;
    char* result = validate_jwt_and_get_db(NULL, &err_out);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(err_out);
    free(err_out);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_validate_jwt_and_get_db_empty_token_returns_error);
    RUN_TEST(test_validate_jwt_and_get_db_null_token_returns_error);

    return UNITY_END();
}