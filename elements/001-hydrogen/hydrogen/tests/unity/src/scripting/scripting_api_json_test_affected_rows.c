/*
 * Unity Test File: scripting_api_test_affected_rows.c
 *
 * Phase 14 of the LUA_PLAN. Tests that the H.*query result table
 * surfaces `affected_rows` from the awaited DatabaseQuery.
 *
 * The test calls H_lua_build_result_table directly (the function
 * is non-static per the project rule "Don't use static functions
 * if possible as we can't test those"). This is the exact function
 * that H_lua_wait_one / H_lua_wait call to build the result table
 * from a DatabaseQuery*'s query_template (data_json) and
 * affected_rows fields. End-to-end coverage of the await -> build
 * flow is provided by the dedicated database_queue_await_result
 * Unity test (scripting_test_database_queue_await_result.c).
 *
 * Per the plan: atomic task claiming depends on
 * `res.affected_rows == 1` for the winner and `== 0` for the losers
 * of a conditional UPDATE. This test pins both halves of that
 * contract.
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <string.h>
#include <stdlib.h>

// Third-party includes
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Module under test
#include <src/scripting/scripting_api.h>

// Forward declarations (required for -Wmissing-prototypes)
void test_result_table_affected_rows_zero_for_empty_data(void);
void test_result_table_affected_rows_zero_for_select(void);
void test_result_table_affected_rows_nonzero_for_update(void);
void test_result_table_affected_rows_zero_for_no_match_update(void);
void test_result_table_affected_rows_field_present_alongside_rows(void);
void test_result_table_affected_rows_handles_malformed_json(void);
void test_result_table_affected_rows_handles_null_data_json(void);

// Per-test Lua state. Unity runs tests sequentially, so a file-static
// pointer is sufficient.
static lua_State* L = NULL;

void setUp(void) {
    L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    // Use logging mock so log_this from the production code does
    // not crash; the affected_rows error path in
    // H_lua_build_result_table calls log_this on JSON parse failure.
}

void tearDown(void) {
    if (L) {
        lua_close(L);
        L = NULL;
    }
}

// Helper: peek a Lua integer at the top of the stack (table field)
static int peek_field_int(const char* key) {
    lua_getfield(L, -1, key);
    int v = (int)lua_tointeger(L, -1);
    lua_pop(L, 1);
    return v;
}

// Helper: get the length of the array at the top of the stack
static int peek_table_len(void) {
    return (int)lua_rawlen(L, -1);
}

// Call H_lua_build_result_table and assert no exceptions / no
// unexpected stack growth. The pushed table is on top of the stack.
static void build(const char* data_json, int affected_rows) {
    int rc = H_lua_build_result_table(L, data_json, affected_rows);
    TEST_ASSERT_EQUAL(1, rc);
}

// 1. Empty data_json + affected_rows=0 -> empty rows, affected_rows=0
void test_result_table_affected_rows_zero_for_empty_data(void) {
    build("[]", 0);
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "rows");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    TEST_ASSERT_EQUAL(0, peek_table_len());
    lua_pop(L, 1); // rows
    TEST_ASSERT_EQUAL(0, peek_field_int("affected_rows"));
    lua_pop(L, 1); // result table
}

// 2. SELECT result: rows present, affected_rows=0
void test_result_table_affected_rows_zero_for_select(void) {
    build("[{\"id\":1,\"name\":\"a\"},{\"id\":2,\"name\":\"b\"}]", 0);
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "rows");
    TEST_ASSERT_EQUAL(2, peek_table_len());
    lua_pop(L, 1); // rows
    TEST_ASSERT_EQUAL(0, peek_field_int("affected_rows"));
    lua_pop(L, 1); // result table
}

// 3. UPDATE that matched one row: empty rows, affected_rows=1
void test_result_table_affected_rows_nonzero_for_update(void) {
    build("[]", 1);
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "rows");
    TEST_ASSERT_EQUAL(0, peek_table_len());
    lua_pop(L, 1); // rows
    TEST_ASSERT_EQUAL(1, peek_field_int("affected_rows"));
    lua_pop(L, 1); // result table
}

// 4. UPDATE that matched no row: empty rows, affected_rows=0
//    (the loser's view in an atomic task-claim race)
void test_result_table_affected_rows_zero_for_no_match_update(void) {
    build("[]", 0);
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "rows");
    TEST_ASSERT_EQUAL(0, peek_table_len());
    lua_pop(L, 1); // rows
    TEST_ASSERT_EQUAL(0, peek_field_int("affected_rows"));
    lua_pop(L, 1); // result table
}

// 5. Both fields are present on every result. This pins the
//    contract "result is a table with both rows and
//    affected_rows, never just one of them".
void test_result_table_affected_rows_field_present_alongside_rows(void) {
    build("[{\"a\":1}]", 5);
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    // Both fields exist
    lua_getfield(L, -1, "rows");
    TEST_ASSERT_FALSE(lua_isnil(L, -1));
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    TEST_ASSERT_EQUAL(1, peek_table_len());
    lua_pop(L, 1);
    lua_getfield(L, -1, "affected_rows");
    TEST_ASSERT_FALSE(lua_isnil(L, -1));
    TEST_ASSERT_EQUAL(5, (int)lua_tointeger(L, -1));
    lua_pop(L, 1);
    lua_pop(L, 1); // result table
}

// 6. Malformed JSON -> empty rows, but affected_rows is still set.
//    (The production code logs the parse failure but does not
//    fail the call; affected_rows is independent of data_json
//    parse success because it comes from a separate engine field.)
void test_result_table_affected_rows_handles_malformed_json(void) {
    build("not a json array at all", 3);
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "rows");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    TEST_ASSERT_EQUAL(0, peek_table_len());
    lua_pop(L, 1); // rows
    TEST_ASSERT_EQUAL(3, peek_field_int("affected_rows"));
    lua_pop(L, 1); // result table
}

// 7. NULL data_json -> empty rows, affected_rows still set
void test_result_table_affected_rows_handles_null_data_json(void) {
    build(NULL, 42);
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "rows");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    TEST_ASSERT_EQUAL(0, peek_table_len());
    lua_pop(L, 1); // rows
    TEST_ASSERT_EQUAL(42, peek_field_int("affected_rows"));
    lua_pop(L, 1); // result table
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_result_table_affected_rows_zero_for_empty_data);
    RUN_TEST(test_result_table_affected_rows_zero_for_select);
    RUN_TEST(test_result_table_affected_rows_nonzero_for_update);
    RUN_TEST(test_result_table_affected_rows_zero_for_no_match_update);
    RUN_TEST(test_result_table_affected_rows_field_present_alongside_rows);
    RUN_TEST(test_result_table_affected_rows_handles_malformed_json);
    RUN_TEST(test_result_table_affected_rows_handles_null_data_json);

    return UNITY_END();
}
