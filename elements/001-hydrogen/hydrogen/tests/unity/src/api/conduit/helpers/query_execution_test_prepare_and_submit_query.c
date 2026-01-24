/*
 * Unity Test File: prepare_and_submit_query()
 * This file contains unit tests for the query preparation and submission function
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/conduit/conduit_helpers.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database_cache.h>
#include <unity/mocks/mock_dbqueue.h>
#include <string.h>
#include <time.h>

// Forward declaration for function being tested
bool prepare_and_submit_query(DatabaseQueue* selected_queue, const char* query_id,
                              const char* sql_template, TypedParameter** ordered_params,
                              size_t param_count, const QueryCacheEntry* cache_entry);

// Use the mocks from mock_dbqueue.h
extern void mock_dbqueue_set_submit_query_result(bool result);
extern const DatabaseQuery* mock_dbqueue_get_last_submitted_query(void);

// Helper function prototypes
static TypedParameter* create_typed_parameter(const char* name, ParameterType type, ...);

// Helper functions for creating test data
static TypedParameter* create_typed_parameter(const char* name, ParameterType type, ...) {
    TypedParameter* param = calloc(1, sizeof(TypedParameter));
    if (param) {
        param->name = strdup(name);
        param->type = type;
        
        va_list args;
        va_start(args, type);
        
        switch (type) {
            case PARAM_TYPE_INTEGER:
                param->value.int_value = va_arg(args, int);
                break;
            case PARAM_TYPE_STRING:
                param->value.string_value = strdup(va_arg(args, char*));
                break;
            case PARAM_TYPE_BOOLEAN:
                param->value.bool_value = va_arg(args, int);
                break;
            case PARAM_TYPE_FLOAT:
                param->value.float_value = va_arg(args, double);
                break;
            case PARAM_TYPE_TEXT:
                param->value.text_value = strdup(va_arg(args, char*));
                break;
            case PARAM_TYPE_DATE:
                param->value.date_value = strdup(va_arg(args, char*));
                break;
            case PARAM_TYPE_TIME:
                param->value.time_value = strdup(va_arg(args, char*));
                break;
            case PARAM_TYPE_DATETIME:
                param->value.datetime_value = strdup(va_arg(args, char*));
                break;
            case PARAM_TYPE_TIMESTAMP:
                param->value.timestamp_value = strdup(va_arg(args, char*));
                break;
        }
        
        va_end(args);
    }
    return param;
}

// Function prototypes for test functions
void test_prepare_and_submit_query_null_parameters(void);
void test_prepare_and_submit_query_param_count_exceeds_limit(void);
void test_prepare_and_submit_query_basic_success(void);
void test_prepare_and_submit_query_with_single_parameter(void);
void test_prepare_and_submit_query_with_multiple_parameter_types(void);
void test_prepare_and_submit_query_no_parameters(void);

void setUp(void) {
    // Reset mock state before each test
    mock_dbqueue_reset_all();
}

void tearDown(void) {
    // Cleanup handled by mock_dbqueue_reset_all()
}

// Test null parameter handling
void test_prepare_and_submit_query_null_parameters(void) {
    // Create dummy test data
    DatabaseQueue test_queue = { .queue_type = strdup("test_queue") };
    QueryCacheEntry test_cache_entry = {
        .sql_template = strdup("SELECT * FROM test"),
        .queue_type = strdup("default"),
        .description = strdup("Test query"),
        .timeout_seconds = 30
    };

    // Test with null selected_queue
    TEST_ASSERT_FALSE(prepare_and_submit_query(NULL, "test_id_1", "SELECT 1", NULL, 0, &test_cache_entry));

    // Test with null query_id
    TEST_ASSERT_FALSE(prepare_and_submit_query(&test_queue, NULL, "SELECT 1", NULL, 0, &test_cache_entry));

    // Test with null sql_template
    TEST_ASSERT_FALSE(prepare_and_submit_query(&test_queue, "test_id_1", NULL, NULL, 0, &test_cache_entry));

    // Test with null cache_entry
    TEST_ASSERT_FALSE(prepare_and_submit_query(&test_queue, "test_id_1", "SELECT 1", NULL, 0, NULL));

    // Cleanup
    free((char*)test_queue.queue_type);
    free((char*)test_cache_entry.sql_template);
    free((char*)test_cache_entry.queue_type);
    free((char*)test_cache_entry.description);
}

// Test parameter count exceeds limit (100)
void test_prepare_and_submit_query_param_count_exceeds_limit(void) {
    // Create dummy test data
    DatabaseQueue test_queue = { .queue_type = strdup("test_queue") };
    QueryCacheEntry test_cache_entry = {
        .sql_template = strdup("SELECT * FROM test"),
        .queue_type = strdup("default"),
        .description = strdup("Test query"),
        .timeout_seconds = 30
    };

    // Create more than 100 parameters
    const size_t PARAM_COUNT = 101;
    TypedParameter** params = calloc(PARAM_COUNT, sizeof(TypedParameter*));
    if (!params) {
        // Cleanup
        free((char*)test_queue.queue_type);
        free((char*)test_cache_entry.sql_template);
        free((char*)test_cache_entry.queue_type);
        free((char*)test_cache_entry.description);
        return;
    }

    for (size_t i = 0; i < PARAM_COUNT; i++) {
        char param_name[32];
        snprintf(param_name, sizeof(param_name), "param%zu", i);
        params[i] = create_typed_parameter(param_name, PARAM_TYPE_INTEGER, (int)i);
        if (!params[i]) {
            // Cleanup
            for (size_t j = 0; j < i; j++) {
                free_typed_parameter(params[j]);
            }
            free(params);
            free((char*)test_queue.queue_type);
            free((char*)test_cache_entry.sql_template);
            free((char*)test_cache_entry.queue_type);
            free((char*)test_cache_entry.description);
            return;
        }
    }

    // Should return false for too many parameters
    TEST_ASSERT_FALSE(prepare_and_submit_query(&test_queue, "test_id_1", "SELECT 1", params, PARAM_COUNT, &test_cache_entry));

    // Cleanup
    for (size_t i = 0; i < PARAM_COUNT; i++) {
        free_typed_parameter(params[i]);
    }
    free(params);
    free((char*)test_queue.queue_type);
    free((char*)test_cache_entry.sql_template);
    free((char*)test_cache_entry.queue_type);
    free((char*)test_cache_entry.description);
}

// Test basic success case with no parameters
void test_prepare_and_submit_query_no_parameters(void) {
    // Create dummy test data
    DatabaseQueue test_queue = { .queue_type = strdup("test_queue") };
    QueryCacheEntry test_cache_entry = {
        .sql_template = strdup("SELECT * FROM test"),
        .queue_type = strdup("default"),
        .description = strdup("Test query"),
        .timeout_seconds = 30
    };

    // Call function with no parameters
    bool result = prepare_and_submit_query(&test_queue, "test_id_1", "SELECT 1", NULL, 0, &test_cache_entry);
    
    // Should succeed
    TEST_ASSERT_TRUE(result);

    // Verify query was properly prepared
    const DatabaseQuery* last_query = mock_dbqueue_get_last_submitted_query();
    TEST_ASSERT_NOT_NULL(last_query);
    
    if (!last_query) {
        printf("DEBUG: last_query is NULL\n");
        // Cleanup
        free((char*)test_queue.queue_type);
        free((char*)test_cache_entry.sql_template);
        free((char*)test_cache_entry.queue_type);
        free((char*)test_cache_entry.description);
        return;
    }
    
    if (!last_query->query_id) {
        printf("DEBUG: last_query->query_id is NULL\n");
        // Cleanup
        free((char*)test_queue.queue_type);
        free((char*)test_cache_entry.sql_template);
        free((char*)test_cache_entry.queue_type);
        free((char*)test_cache_entry.description);
        return;
    }
    
    printf("DEBUG: last_query->query_id = %s\n", last_query->query_id);
    printf("DEBUG: last_query->query_template = %s\n", last_query->query_template);
    
    TEST_ASSERT_EQUAL_STRING("test_id_1", last_query->query_id);
    TEST_ASSERT_EQUAL_STRING("SELECT 1", last_query->query_template);
    TEST_ASSERT_NULL(last_query->parameter_json);

    // Cleanup
    free((char*)test_queue.queue_type);
    free((char*)test_cache_entry.sql_template);
    free((char*)test_cache_entry.queue_type);
    free((char*)test_cache_entry.description);
}

// Test basic success case
void test_prepare_and_submit_query_basic_success(void) {
    // Create dummy test data
    DatabaseQueue test_queue = { .queue_type = strdup("test_queue") };
    QueryCacheEntry test_cache_entry = {
        .sql_template = strdup("SELECT * FROM test"),
        .queue_type = strdup("default"),
        .description = strdup("Test query"),
        .timeout_seconds = 30
    };

    // Call function
    bool result = prepare_and_submit_query(&test_queue, "test_id_1", "SELECT 1", NULL, 0, &test_cache_entry);

    // Should succeed
    TEST_ASSERT_TRUE(result);

    // Cleanup
    free((char*)test_queue.queue_type);
    free((char*)test_cache_entry.sql_template);
    free((char*)test_cache_entry.queue_type);
    free((char*)test_cache_entry.description);
}

// Test with single parameter
void test_prepare_and_submit_query_with_single_parameter(void) {
    // Create dummy test data
    DatabaseQueue test_queue = { .queue_type = strdup("test_queue") };
    QueryCacheEntry test_cache_entry = {
        .sql_template = strdup("SELECT * FROM test WHERE id = ?"),
        .queue_type = strdup("default"),
        .description = strdup("Test query"),
        .timeout_seconds = 30
    };

    // Create single parameter
    TypedParameter** params = calloc(1, sizeof(TypedParameter*));
    if (!params) {
        // Cleanup
        free((char*)test_queue.queue_type);
        free((char*)test_cache_entry.sql_template);
        free((char*)test_cache_entry.queue_type);
        free((char*)test_cache_entry.description);
        return;
    }

    params[0] = create_typed_parameter("id", PARAM_TYPE_INTEGER, 123);
    if (!params[0]) {
        // Cleanup
        free(params);
        free((char*)test_queue.queue_type);
        free((char*)test_cache_entry.sql_template);
        free((char*)test_cache_entry.queue_type);
        free((char*)test_cache_entry.description);
        return;
    }

    // Call function
    bool result = prepare_and_submit_query(&test_queue, "test_id_1", "SELECT * FROM test WHERE id = ?", params, 1, &test_cache_entry);

    // Should succeed
    TEST_ASSERT_TRUE(result);

    // Cleanup
    free_typed_parameter(params[0]);
    free(params);
    free((char*)test_queue.queue_type);
    free((char*)test_cache_entry.sql_template);
    free((char*)test_cache_entry.queue_type);
    free((char*)test_cache_entry.description);
}

// Test with multiple parameter types
void test_prepare_and_submit_query_with_multiple_parameter_types(void) {
    // Create dummy test data
    DatabaseQueue test_queue = { .queue_type = strdup("test_queue") };
    QueryCacheEntry test_cache_entry = {
        .sql_template = strdup("SELECT * FROM test WHERE id = ? AND name = ? AND active = ?"),
        .queue_type = strdup("default"),
        .description = strdup("Test query"),
        .timeout_seconds = 30
    };

    // Create parameters of various types
    TypedParameter** params = calloc(3, sizeof(TypedParameter*));
    if (!params) {
        // Cleanup
        free((char*)test_queue.queue_type);
        free((char*)test_cache_entry.sql_template);
        free((char*)test_cache_entry.queue_type);
        free((char*)test_cache_entry.description);
        return;
    }

    params[0] = create_typed_parameter("id", PARAM_TYPE_INTEGER, 123);
    if (!params[0]) {
        // Cleanup
        free(params);
        free((char*)test_queue.queue_type);
        free((char*)test_cache_entry.sql_template);
        free((char*)test_cache_entry.queue_type);
        free((char*)test_cache_entry.description);
        return;
    }

    params[1] = create_typed_parameter("name", PARAM_TYPE_STRING, "test_name");
    if (!params[1]) {
        // Cleanup
        free_typed_parameter(params[0]);
        free(params);
        free((char*)test_queue.queue_type);
        free((char*)test_cache_entry.sql_template);
        free((char*)test_cache_entry.queue_type);
        free((char*)test_cache_entry.description);
        return;
    }

    params[2] = create_typed_parameter("active", PARAM_TYPE_BOOLEAN, 1);
    if (!params[2]) {
        // Cleanup
        free_typed_parameter(params[0]);
        free_typed_parameter(params[1]);
        free(params);
        free((char*)test_queue.queue_type);
        free((char*)test_cache_entry.sql_template);
        free((char*)test_cache_entry.queue_type);
        free((char*)test_cache_entry.description);
        return;
    }

    // Call function
    bool result = prepare_and_submit_query(&test_queue, "test_id_1", "SELECT * FROM test WHERE id = ? AND name = ? AND active = ?", params, 3, &test_cache_entry);

    // Should succeed
    TEST_ASSERT_TRUE(result);

    // Cleanup
    for (int i = 0; i < 3; i++) {
        free_typed_parameter(params[i]);
    }
    free(params);
    free((char*)test_queue.queue_type);
    free((char*)test_cache_entry.sql_template);
    free((char*)test_cache_entry.queue_type);
    free((char*)test_cache_entry.description);
}


int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_prepare_and_submit_query_null_parameters);
    RUN_TEST(test_prepare_and_submit_query_param_count_exceeds_limit);
    RUN_TEST(test_prepare_and_submit_query_no_parameters);
    RUN_TEST(test_prepare_and_submit_query_basic_success);
    RUN_TEST(test_prepare_and_submit_query_with_single_parameter);
    RUN_TEST(test_prepare_and_submit_query_with_multiple_parameter_types);

    return UNITY_END();
}
