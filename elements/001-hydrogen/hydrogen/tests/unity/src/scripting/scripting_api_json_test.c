/*
 * Unity Test File: scripting_api_json_test.c
 *
 * Tests the JSON/Lua conversion helpers in scripting_api_json.c:
 *   - push_json_object_as_table with valid and invalid inputs
 *   - push_json_array_as_table array conversion
 *   - push_json_value_as_lua with all JSON types including nested structures
 *   - H_lua_build_result_table with various data inputs
 */

// Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <stdlib.h>
#include <string.h>

// Third-party includes
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <jansson.h>

// Module under test
#include <src/scripting/scripting_api_internal.h>

// Forward declarations (required for -Wmissing-prototypes)
void test_push_json_object_as_table_non_object_returns_nil(void);
void test_push_json_object_as_table_empty_object(void);
void test_push_json_object_as_table_with_fields(void);
void test_push_json_array_as_table_empty(void);
void test_push_json_array_as_table_with_values(void);
void test_push_json_value_as_lua_null(void);
void test_push_json_value_as_lua_json_null(void);
void test_push_json_value_as_lua_json_true(void);
void test_push_json_value_as_lua_json_false(void);
void test_push_json_value_as_lua_json_integer(void);
void test_push_json_value_as_lua_json_real(void);
void test_push_json_value_as_lua_json_string(void);
void test_push_json_value_as_lua_json_array(void);
void test_push_json_value_as_lua_json_object(void);
void test_push_json_value_as_lua_nested(void);
void test_H_lua_build_result_table_empty(void);
void test_H_lua_build_result_table_valid_json(void);
void test_H_lua_build_result_table_null_data(void);
void test_H_lua_build_result_table_invalid_json(void);

// Per-test Lua state. Unity runs tests sequentially, so a file-static
// pointer is sufficient.
static lua_State* L = NULL;

void setUp(void) {
    L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
}

void tearDown(void) {
    if (L) {
        lua_close(L);
        L = NULL;
    }
}

// push_json_object_as_table with non-object returns nil
void test_push_json_object_as_table_non_object_returns_nil(void) {
    push_json_object_as_table(L, NULL);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -1));
    lua_pop(L, 1);

    json_t* arr = json_array();
    push_json_object_as_table(L, arr);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -1));
    lua_pop(L, 1);
    json_decref(arr);
}

// push_json_object_as_table with empty object returns empty table
void test_push_json_object_as_table_empty_object(void) {
    json_t* obj = json_object();
    push_json_object_as_table(L, obj);
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    TEST_ASSERT_EQUAL(0, lua_rawlen(L, -1));
    lua_pop(L, 1);
    json_decref(obj);
}

// push_json_object_as_table with fields produces table
void test_push_json_object_as_table_with_fields(void) {
    json_t* obj = json_object();
    json_object_set_new(obj, "key1", json_integer(42));
    json_object_set_new(obj, "key2", json_string("hello"));
    push_json_object_as_table(L, obj);
    TEST_ASSERT_TRUE(lua_istable(L, -1));

    lua_getfield(L, -1, "key1");
    TEST_ASSERT_EQUAL(LUA_TNUMBER, lua_type(L, -1));
    TEST_ASSERT_EQUAL(42, (int)lua_tointeger(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "key2");
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    TEST_ASSERT_EQUAL_STRING("hello", lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_pop(L, 1);
    json_decref(obj);
}

// push_json_array_as_table with empty array returns empty table
void test_push_json_array_as_table_empty(void) {
    json_t* arr = json_array();
    push_json_array_as_table(L, arr);
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    TEST_ASSERT_EQUAL(0, lua_rawlen(L, -1));
    lua_pop(L, 1);
    json_decref(arr);
}

// push_json_array_as_table with values produces table
void test_push_json_array_as_table_with_values(void) {
    json_t* arr = json_array();
    json_array_append_new(arr, json_integer(1));
    json_array_append_new(arr, json_string("two"));
    json_array_append_new(arr, json_boolean(1));
    push_json_array_as_table(L, arr);
    TEST_ASSERT_TRUE(lua_istable(L, -1));

    lua_rawgeti(L, -1, 1);
    TEST_ASSERT_EQUAL(LUA_TNUMBER, lua_type(L, -1));
    TEST_ASSERT_EQUAL(1, (int)lua_tointeger(L, -1));
    lua_pop(L, 1);

    lua_rawgeti(L, -1, 2);
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    TEST_ASSERT_EQUAL_STRING("two", lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_rawgeti(L, -1, 3);
    TEST_ASSERT_EQUAL(LUA_TBOOLEAN, lua_type(L, -1));
    TEST_ASSERT_EQUAL(1, (int)lua_toboolean(L, -1));
    lua_pop(L, 1);

    lua_pop(L, 1);
    json_decref(arr);
}

// push_json_value_as_lua with NULL pushes nil
void test_push_json_value_as_lua_null(void) {
    push_json_value_as_lua(L, NULL);
    TEST_ASSERT_TRUE(lua_isnil(L, -1));
    lua_pop(L, 1);
}

// push_json_value_as_lua with JSON_NULL pushes nil
void test_push_json_value_as_lua_json_null(void) {
    json_t* val = json_null();
    push_json_value_as_lua(L, val);
    TEST_ASSERT_TRUE(lua_isnil(L, -1));
    lua_pop(L, 1);
    json_decref(val);
}

// push_json_value_as_lua with JSON_TRUE pushes boolean 1
void test_push_json_value_as_lua_json_true(void) {
    json_t* val = json_true();
    push_json_value_as_lua(L, val);
    TEST_ASSERT_EQUAL(LUA_TBOOLEAN, lua_type(L, -1));
    TEST_ASSERT_EQUAL(1, (int)lua_toboolean(L, -1));
    lua_pop(L, 1);
    json_decref(val);
}

// push_json_value_as_lua with JSON_FALSE pushes boolean 0
void test_push_json_value_as_lua_json_false(void) {
    json_t* val = json_false();
    push_json_value_as_lua(L, val);
    TEST_ASSERT_EQUAL(LUA_TBOOLEAN, lua_type(L, -1));
    TEST_ASSERT_EQUAL(0, (int)lua_toboolean(L, -1));
    lua_pop(L, 1);
    json_decref(val);
}

// push_json_value_as_lua with JSON_INTEGER pushes integer
void test_push_json_value_as_lua_json_integer(void) {
    json_t* val = json_integer(123);
    push_json_value_as_lua(L, val);
    TEST_ASSERT_EQUAL(LUA_TNUMBER, lua_type(L, -1));
    TEST_ASSERT_EQUAL(123, (int)lua_tointeger(L, -1));
    lua_pop(L, 1);
    json_decref(val);
}

// push_json_value_as_lua with JSON_REAL pushes number
void test_push_json_value_as_lua_json_real(void) {
    json_t* val = json_real(2.718);
    push_json_value_as_lua(L, val);
    TEST_ASSERT_EQUAL(LUA_TNUMBER, lua_type(L, -1));
    TEST_ASSERT_FLOAT_WITHIN(0.001, 2.718, lua_tonumber(L, -1));
    lua_pop(L, 1);
    json_decref(val);
}

// push_json_value_as_lua with JSON_STRING pushes string
void test_push_json_value_as_lua_json_string(void) {
    json_t* val = json_string("test_value");
    push_json_value_as_lua(L, val);
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    TEST_ASSERT_EQUAL_STRING("test_value", lua_tostring(L, -1));
    lua_pop(L, 1);
    json_decref(val);
}

// push_json_value_as_lua with JSON_ARRAY calls push_json_array_as_table
void test_push_json_value_as_lua_json_array(void) {
    json_t* arr = json_array();
    json_array_append_new(arr, json_integer(99));
    push_json_value_as_lua(L, arr);
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_rawgeti(L, -1, 1);
    TEST_ASSERT_EQUAL(99, (int)lua_tointeger(L, -1));
    lua_pop(L, 2);
    json_decref(arr);
}

// push_json_value_as_lua with JSON_OBJECT calls push_json_object_as_table
void test_push_json_value_as_lua_json_object(void) {
    json_t* obj = json_object();
    json_object_set_new(obj, "inner_key", json_integer(42));
    push_json_value_as_lua(L, obj);
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "inner_key");
    TEST_ASSERT_EQUAL(42, (int)lua_tointeger(L, -1));
    lua_pop(L, 2);
    json_decref(obj);
}

// Nested JSON object with array tests recursive conversion
void test_push_json_value_as_lua_nested(void) {
    json_t* inner = json_object();
    json_object_set_new(inner, "nested_val", json_integer(99));
    json_t* outer = json_object();
    json_object_set_new(outer, "inner", inner);
    push_json_value_as_lua(L, outer);
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "inner");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "nested_val");
    TEST_ASSERT_EQUAL(99, (int)lua_tointeger(L, -1));
    lua_pop(L, 3);
    json_decref(outer);
}

// H_lua_build_result_table with empty data produces table with empty rows
void test_H_lua_build_result_table_empty(void) {
    int n = H_lua_build_result_table(L, "", 0);
    TEST_ASSERT_EQUAL(1, n);
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "rows");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    TEST_ASSERT_EQUAL(0, lua_rawlen(L, -1));
    lua_pop(L, 2);
}

// H_lua_build_result_table with valid JSON produces rows table
void test_H_lua_build_result_table_valid_json(void) {
    const char* data = "[{\"id\":1,\"name\":\"test\"}]";
    int n = H_lua_build_result_table(L, data, 5);
    TEST_ASSERT_EQUAL(1, n);
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "rows");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_rawgeti(L, -1, 1);
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "id");
    TEST_ASSERT_EQUAL(1, (int)lua_tointeger(L, -1));
    lua_pop(L, 3);
}

// H_lua_build_result_table with null data produces table with empty rows
void test_H_lua_build_result_table_null_data(void) {
    int n = H_lua_build_result_table(L, NULL, 0);
    TEST_ASSERT_EQUAL(1, n);
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "rows");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_pop(L, 2);
}

// H_lua_build_result_table with invalid JSON produces empty rows
void test_H_lua_build_result_table_invalid_json(void) {
    int n = H_lua_build_result_table(L, "not json at all", 0);
    TEST_ASSERT_EQUAL(1, n);
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "rows");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    TEST_ASSERT_EQUAL(0, lua_rawlen(L, -1));
    lua_pop(L, 2);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_push_json_object_as_table_non_object_returns_nil);
    RUN_TEST(test_push_json_object_as_table_empty_object);
    RUN_TEST(test_push_json_object_as_table_with_fields);
    RUN_TEST(test_push_json_array_as_table_empty);
    RUN_TEST(test_push_json_array_as_table_with_values);
    RUN_TEST(test_push_json_value_as_lua_null);
    RUN_TEST(test_push_json_value_as_lua_json_null);
    RUN_TEST(test_push_json_value_as_lua_json_true);
    RUN_TEST(test_push_json_value_as_lua_json_false);
    RUN_TEST(test_push_json_value_as_lua_json_integer);
    RUN_TEST(test_push_json_value_as_lua_json_real);
    RUN_TEST(test_push_json_value_as_lua_json_string);
    RUN_TEST(test_push_json_value_as_lua_json_array);
    RUN_TEST(test_push_json_value_as_lua_json_object);
    RUN_TEST(test_push_json_value_as_lua_nested);
    RUN_TEST(test_H_lua_build_result_table_empty);
    RUN_TEST(test_H_lua_build_result_table_valid_json);
    RUN_TEST(test_H_lua_build_result_table_null_data);
    RUN_TEST(test_H_lua_build_result_table_invalid_json);

    return UNITY_END();
}