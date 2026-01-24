/*
 * Unity Test File: Collect Provided From Param List
 * This file contains unit tests for collect_provided_from_param_list() function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/api/conduit/helpers/param_proc_helpers.h>
#include <src/database/database_params.h>

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

static void test_collect_provided_from_param_list_basic(void) {
    ParameterList* param_list = calloc(1, sizeof(ParameterList));
    TEST_ASSERT_NOT_NULL(param_list);
    
    param_list->count = 3;
    param_list->params = calloc(3, sizeof(TypedParameter*));
    TEST_ASSERT_NOT_NULL(param_list->params);
    
    for (size_t i = 0; i < 3; i++) {
        param_list->params[i] = calloc(1, sizeof(TypedParameter));
        TEST_ASSERT_NOT_NULL(param_list->params[i]);
    }
    
    param_list->params[0]->name = strdup("userId");
    TEST_ASSERT_NOT_NULL(param_list->params[0]->name);
    
    param_list->params[1]->name = strdup("username");
    TEST_ASSERT_NOT_NULL(param_list->params[1]->name);
    
    param_list->params[2]->name = strdup("email");
    TEST_ASSERT_NOT_NULL(param_list->params[2]->name);
    
    size_t count = 0;
    char** params = collect_provided_from_param_list(param_list, &count);
    
    TEST_ASSERT_NOT_NULL(params);
    TEST_ASSERT_EQUAL_INT(3, count);
    
    bool has_userId = false;
    bool has_username = false;
    bool has_email = false;
    
    for (size_t i = 0; i < count; i++) {
        if (strcmp(params[i], "userId") == 0) has_userId = true;
        if (strcmp(params[i], "username") == 0) has_username = true;
        if (strcmp(params[i], "email") == 0) has_email = true;
    }
    
    TEST_ASSERT_TRUE(has_userId);
    TEST_ASSERT_TRUE(has_username);
    TEST_ASSERT_TRUE(has_email);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free(params[i]);
    }
    free(params);
    
    for (size_t i = 0; i < param_list->count; i++) {
        free(param_list->params[i]->name);
        free(param_list->params[i]);
    }
    free(param_list->params);
    free(param_list);
}

static void test_collect_provided_from_param_list_empty(void) {
    ParameterList* param_list = calloc(1, sizeof(ParameterList));
    TEST_ASSERT_NOT_NULL(param_list);
    
    param_list->count = 0;
    param_list->params = NULL;
    
    size_t count = 0;
    char** params = collect_provided_from_param_list(param_list, &count);
    
    TEST_ASSERT_NULL(params);
    TEST_ASSERT_EQUAL_INT(0, count);
    
    free(param_list);
}

static void test_collect_provided_from_param_list_null(void) {
    size_t count = 0;
    char** params = collect_provided_from_param_list(NULL, &count);
    
    TEST_ASSERT_NULL(params);
    TEST_ASSERT_EQUAL_INT(0, count);
}

static void test_collect_provided_from_param_list_duplicates(void) {
    ParameterList* param_list = calloc(1, sizeof(ParameterList));
    TEST_ASSERT_NOT_NULL(param_list);
    
    param_list->count = 4;
    param_list->params = calloc(4, sizeof(TypedParameter*));
    TEST_ASSERT_NOT_NULL(param_list->params);
    
    for (size_t i = 0; i < 4; i++) {
        param_list->params[i] = calloc(1, sizeof(TypedParameter));
        TEST_ASSERT_NOT_NULL(param_list->params[i]);
    }
    
    param_list->params[0]->name = strdup("userId");
    TEST_ASSERT_NOT_NULL(param_list->params[0]->name);
    
    param_list->params[1]->name = strdup("username");
    TEST_ASSERT_NOT_NULL(param_list->params[1]->name);
    
    param_list->params[2]->name = strdup("userId");
    TEST_ASSERT_NOT_NULL(param_list->params[2]->name);
    
    param_list->params[3]->name = strdup("email");
    TEST_ASSERT_NOT_NULL(param_list->params[3]->name);
    
    size_t count = 0;
    char** params = collect_provided_from_param_list(param_list, &count);
    
    TEST_ASSERT_NOT_NULL(params);
    TEST_ASSERT_EQUAL_INT(3, count);
    
    bool has_userId = false;
    bool has_username = false;
    bool has_email = false;
    
    for (size_t i = 0; i < count; i++) {
        if (strcmp(params[i], "userId") == 0) has_userId = true;
        if (strcmp(params[i], "username") == 0) has_username = true;
        if (strcmp(params[i], "email") == 0) has_email = true;
    }
    
    TEST_ASSERT_TRUE(has_userId);
    TEST_ASSERT_TRUE(has_username);
    TEST_ASSERT_TRUE(has_email);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free(params[i]);
    }
    free(params);
    
    for (size_t i = 0; i < param_list->count; i++) {
        free(param_list->params[i]->name);
        free(param_list->params[i]);
    }
    free(param_list->params);
    free(param_list);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_collect_provided_from_param_list_basic);
    RUN_TEST(test_collect_provided_from_param_list_empty);
    RUN_TEST(test_collect_provided_from_param_list_null);
    RUN_TEST(test_collect_provided_from_param_list_duplicates);
    
    return UNITY_END();
}
