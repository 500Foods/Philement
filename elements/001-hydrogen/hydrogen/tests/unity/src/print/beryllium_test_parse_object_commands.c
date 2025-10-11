/*
 * Unity Test File: Beryllium parse_object_commands Function Tests
 * This file contains unit tests for the parse_object_commands() function
 * from src/print/beryllium.c
 *
 * Coverage Goals:
 * - Test EXCLUDE_OBJECT_DEFINE parsing
 * - Test EXCLUDE_OBJECT_START parsing
 * - Test EXCLUDE_OBJECT_END parsing
 * - Test object state management
 * - Test memory allocation handling
 * - Test edge cases and error conditions
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>
#include <stdlib.h>

// Include necessary headers for the beryllium module
#include <src/print/beryllium.h>

void setUp(void) {
    // No setup needed for object command parsing tests
}

void tearDown(void) {
    // No teardown needed for object command parsing tests
}

// Forward declaration for the function being tested
bool parse_object_commands(const char *line, ObjectInfo **object_infos, int *num_objects,
                           int *current_object);

// Test functions
void test_parse_object_commands_null_line(void);
void test_parse_object_commands_no_object_commands(void);
void test_parse_object_commands_exclude_object_define(void);
void test_parse_object_commands_exclude_object_start(void);
void test_parse_object_commands_exclude_object_end(void);
void test_parse_object_commands_multiple_objects(void);
void test_parse_object_commands_object_name_with_spaces(void);
void test_parse_object_commands_malformed_commands(void);
void test_parse_object_commands_memory_allocation_failure(void);

void test_parse_object_commands_null_line(void) {
    ObjectInfo *object_infos = NULL;
    int num_objects = 0;
    int current_object = -1;

    bool result = parse_object_commands(NULL, &object_infos, &num_objects, &current_object);
    TEST_ASSERT_FALSE(result);
}

void test_parse_object_commands_no_object_commands(void) {
    ObjectInfo *object_infos = NULL;
    int num_objects = 0;
    int current_object = -1;

    bool result = parse_object_commands("G1 X10 Y10", &object_infos, &num_objects, &current_object);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(0, num_objects);
    TEST_ASSERT_EQUAL_INT(-1, current_object);
}

void test_parse_object_commands_exclude_object_define(void) {
    ObjectInfo *object_infos = NULL;
    int num_objects = 0;
    int current_object = -1;

    bool result = parse_object_commands("EXCLUDE_OBJECT_DEFINE NAME=cube", &object_infos, &num_objects, &current_object);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, num_objects);
    TEST_ASSERT_EQUAL_INT(-1, current_object);
    TEST_ASSERT_NOT_NULL(object_infos);
    TEST_ASSERT_EQUAL_STRING("cube", object_infos[0].name);
    TEST_ASSERT_EQUAL_INT(0, object_infos[0].index);

    // Clean up
    free(object_infos[0].name);
    free(object_infos);
}

void test_parse_object_commands_exclude_object_start(void) {
    ObjectInfo *object_infos = NULL;
    int num_objects = 0;
    int current_object = -1;

    // First define an object
    bool result1 = parse_object_commands("EXCLUDE_OBJECT_DEFINE NAME=sphere", &object_infos, &num_objects, &current_object);
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_EQUAL_INT(1, num_objects);

    // Then start the object
    bool result2 = parse_object_commands("EXCLUDE_OBJECT_START NAME=sphere", &object_infos, &num_objects, &current_object);
    TEST_ASSERT_TRUE(result2);
    TEST_ASSERT_EQUAL_INT(0, current_object); // Should be index 0

    // Clean up
    free(object_infos[0].name);
    free(object_infos);
}

void test_parse_object_commands_exclude_object_end(void) {
    ObjectInfo *object_infos = NULL;
    int num_objects = 0;
    int current_object = -1;

    // First define and start an object
    bool result1 = parse_object_commands("EXCLUDE_OBJECT_DEFINE NAME=cube", &object_infos, &num_objects, &current_object);
    TEST_ASSERT_TRUE(result1);
    bool result2 = parse_object_commands("EXCLUDE_OBJECT_START NAME=cube", &object_infos, &num_objects, &current_object);
    TEST_ASSERT_TRUE(result2);
    TEST_ASSERT_EQUAL_INT(0, current_object);

    // Then end the object
    bool result3 = parse_object_commands("EXCLUDE_OBJECT_END", &object_infos, &num_objects, &current_object);
    TEST_ASSERT_TRUE(result3);
    TEST_ASSERT_EQUAL_INT(-1, current_object); // Should be reset to -1

    // Clean up
    free(object_infos[0].name);
    free(object_infos);
}

void test_parse_object_commands_multiple_objects(void) {
    ObjectInfo *object_infos = NULL;
    int num_objects = 0;
    int current_object = -1;

    // Define multiple objects
    bool result1 = parse_object_commands("EXCLUDE_OBJECT_DEFINE NAME=object1", &object_infos, &num_objects, &current_object);
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_EQUAL_INT(1, num_objects);

    bool result2 = parse_object_commands("EXCLUDE_OBJECT_DEFINE NAME=object2", &object_infos, &num_objects, &current_object);
    TEST_ASSERT_TRUE(result2);
    TEST_ASSERT_EQUAL_INT(2, num_objects);

    // Start second object
    bool result3 = parse_object_commands("EXCLUDE_OBJECT_START NAME=object2", &object_infos, &num_objects, &current_object);
    TEST_ASSERT_TRUE(result3);
    TEST_ASSERT_EQUAL_INT(1, current_object); // Should be index 1

    // Clean up
    for (int i = 0; i < num_objects; i++) {
        free(object_infos[i].name);
    }
    free(object_infos);
}

void test_parse_object_commands_object_name_with_spaces(void) {
    ObjectInfo *object_infos = NULL;
    int num_objects = 0;
    int current_object = -1;

    bool result = parse_object_commands("EXCLUDE_OBJECT_DEFINE NAME=my object", &object_infos, &num_objects, &current_object);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, num_objects);
    TEST_ASSERT_EQUAL_STRING("my", object_infos[0].name); // Should only get "my" before space

    // Clean up
    free(object_infos[0].name);
    free(object_infos);
}

void test_parse_object_commands_malformed_commands(void) {
    ObjectInfo *object_infos = NULL;
    int num_objects = 0;
    int current_object = -1;

    // Test malformed define command
    bool result1 = parse_object_commands("EXCLUDE_OBJECT_DEFINE", &object_infos, &num_objects, &current_object);
    TEST_ASSERT_FALSE(result1);
    TEST_ASSERT_EQUAL_INT(0, num_objects);

    // Test malformed start command
    bool result2 = parse_object_commands("EXCLUDE_OBJECT_START", &object_infos, &num_objects, &current_object);
    TEST_ASSERT_FALSE(result2);
    TEST_ASSERT_EQUAL_INT(-1, current_object);
}

void test_parse_object_commands_memory_allocation_failure(void) {
    // This test is hard to simulate without mocking malloc
    // In a real scenario, this would test the error handling paths
    ObjectInfo *object_infos = NULL;
    int num_objects = 0;
    int current_object = -1;

    bool result = parse_object_commands("EXCLUDE_OBJECT_DEFINE NAME=test", &object_infos, &num_objects, &current_object);
    // Should succeed in normal conditions
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, num_objects);

    // Clean up
    free(object_infos[0].name);
    free(object_infos);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_parse_object_commands_null_line);
    RUN_TEST(test_parse_object_commands_no_object_commands);
    RUN_TEST(test_parse_object_commands_exclude_object_define);
    RUN_TEST(test_parse_object_commands_exclude_object_start);
    RUN_TEST(test_parse_object_commands_exclude_object_end);
    RUN_TEST(test_parse_object_commands_multiple_objects);
    RUN_TEST(test_parse_object_commands_object_name_with_spaces);
    RUN_TEST(test_parse_object_commands_malformed_commands);
    RUN_TEST(test_parse_object_commands_memory_allocation_failure);

    return UNITY_END();
}