/*
 * Unity Test File: scripting_api_scoreboard_test_bytecode_dump_writer.c
 *
 * Tests bytecode_dump_writer:
 *   - Small data fits in initial capacity (no grow needed)
 *   - Large data triggers buffer growth
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <src/scripting/scripting_api_scoreboard.h>

typedef struct {
    void*  data;
    size_t len;
    size_t capacity;
} TestBytecodeDumpBuffer;

void test_dump_writer_small_data_no_grow(void);
void test_dump_writer_large_data_triggers_grow(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_dump_writer_small_data_no_grow(void) {
    TestBytecodeDumpBuffer buf;
    memset(&buf, 0, sizeof(buf));

    const char* data = "hello";
    int rc = bytecode_dump_writer(NULL, data, 5, &buf);
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT_EQUAL(5, buf.len);
    TEST_ASSERT_NOT_NULL(buf.data);
    TEST_ASSERT_EQUAL_MEMORY("hello", buf.data, 5);

    free(buf.data);
}

void test_dump_writer_large_data_triggers_grow(void) {
    TestBytecodeDumpBuffer buf;
    memset(&buf, 0, sizeof(buf));

    char large_data[600];
    memset(large_data, 'X', sizeof(large_data));

    int rc = bytecode_dump_writer(NULL, large_data, sizeof(large_data), &buf);
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT_EQUAL(sizeof(large_data), buf.len);
    TEST_ASSERT_NOT_NULL(buf.data);
    TEST_ASSERT_EQUAL_MEMORY(large_data, buf.data, sizeof(large_data));

    free(buf.data);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_dump_writer_small_data_no_grow);
    RUN_TEST(test_dump_writer_large_data_triggers_grow);

    return UNITY_END();
}
