/*
 * Unity Test File: scripting_api_scoreboard_test_split_module_name.c
 *
 * Tests split_module_name:
 *   - NULL name returns false
 *   - NULL group_out returns false
 *   - NULL script_out returns false
 *   - Valid name returns true with correct group and script
 *   - Name without dot returns false
 *   - Name starting with dot returns false
 *   - Name ending with dot returns false
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>

#include <src/scripting/scripting_api_internal.h>

void test_split_null_name_returns_false(void);
void test_split_null_group_out_returns_false(void);
void test_split_null_script_out_returns_false(void);
void test_split_valid_name_returns_true(void);
void test_split_name_without_dot_returns_false(void);
void test_split_name_starting_with_dot_returns_false(void);
void test_split_name_ending_with_dot_returns_false(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_split_null_name_returns_false(void) {
    char* group = NULL;
    char* script = NULL;
    bool ok = split_module_name(NULL, &group, &script);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NULL(group);
    TEST_ASSERT_NULL(script);
}

void test_split_null_group_out_returns_false(void) {
    char* script = NULL;
    bool ok = split_module_name("group.script", NULL, &script);
    TEST_ASSERT_FALSE(ok);
}

void test_split_null_script_out_returns_false(void) {
    char* group = NULL;
    bool ok = split_module_name("group.script", &group, NULL);
    TEST_ASSERT_FALSE(ok);
}

void test_split_valid_name_returns_true(void) {
    char* group = NULL;
    char* script = NULL;
    bool ok = split_module_name("mygroup.myscript", &group, &script);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_NOT_NULL(group);
    TEST_ASSERT_NOT_NULL(script);
    TEST_ASSERT_EQUAL_STRING("mygroup", group);
    TEST_ASSERT_EQUAL_STRING("myscript", script);
    free(group);
    free(script);
}

void test_split_name_without_dot_returns_false(void) {
    char* group = NULL;
    char* script = NULL;
    bool ok = split_module_name("nodothere", &group, &script);
    TEST_ASSERT_FALSE(ok);
}

void test_split_name_starting_with_dot_returns_false(void) {
    char* group = NULL;
    char* script = NULL;
    bool ok = split_module_name(".script", &group, &script);
    TEST_ASSERT_FALSE(ok);
}

void test_split_name_ending_with_dot_returns_false(void) {
    char* group = NULL;
    char* script = NULL;
    bool ok = split_module_name("group.", &group, &script);
    TEST_ASSERT_FALSE(ok);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_split_null_name_returns_false);
    RUN_TEST(test_split_null_group_out_returns_false);
    RUN_TEST(test_split_null_script_out_returns_false);
    RUN_TEST(test_split_valid_name_returns_true);
    RUN_TEST(test_split_name_without_dot_returns_false);
    RUN_TEST(test_split_name_starting_with_dot_returns_false);
    RUN_TEST(test_split_name_ending_with_dot_returns_false);

    return UNITY_END();
}
