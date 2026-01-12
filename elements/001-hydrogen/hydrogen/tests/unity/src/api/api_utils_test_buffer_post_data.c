/*
 * Unity Test File: API Utils api_buffer_post_data Function Tests
 * This file contains unit tests for the api_buffer_post_data function in api_utils.c
 * 
 * Target coverage: Lines 380-496 in api_utils.c (POST buffer management)
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/api_utils.h>

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No teardown needed
}

// Test functions
void test_api_buffer_post_data_get_request(void);
void test_api_buffer_post_data_options_request(void);
void test_api_buffer_post_data_post_first_call(void);
void test_api_buffer_post_data_post_accumulate(void);
void test_api_buffer_post_data_post_complete(void);
void test_api_buffer_post_data_post_large_body(void);
void test_api_buffer_post_data_post_exceeds_max(void);
void test_api_buffer_post_data_post_grow_buffer(void);
void test_api_buffer_post_data_unsupported_method(void);
void test_api_buffer_post_data_null_method(void);
void test_api_buffer_post_data_get_subsequent_call(void);
void test_api_buffer_post_data_options_subsequent_call(void);

// Test GET request completes immediately
void test_api_buffer_post_data_get_request(void) {
    void *con_cls = NULL;
    ApiPostBuffer *buffer_out = NULL;
    size_t upload_data_size = 0;
    
    // First call with GET
    ApiBufferResult result = api_buffer_post_data(
        "GET", 
        NULL, 
        &upload_data_size, 
        &con_cls, 
        &buffer_out
    );
    
    // Lines 399-420: GET should complete immediately
    TEST_ASSERT_EQUAL(API_BUFFER_COMPLETE, result);
    TEST_ASSERT_NOT_NULL(con_cls);
    TEST_ASSERT_NOT_NULL(buffer_out);
    TEST_ASSERT_EQUAL('G', buffer_out->http_method);
    TEST_ASSERT_NULL(buffer_out->data);
    TEST_ASSERT_EQUAL(0, buffer_out->size);
    
    // Cleanup
    api_free_post_buffer(&con_cls);
}

// Test OPTIONS request completes immediately
void test_api_buffer_post_data_options_request(void) {
    void *con_cls = NULL;
    ApiPostBuffer *buffer_out = NULL;
    size_t upload_data_size = 0;
    
    // First call with OPTIONS
    ApiBufferResult result = api_buffer_post_data(
        "OPTIONS", 
        NULL, 
        &upload_data_size, 
        &con_cls, 
        &buffer_out
    );
    
    // Lines 403-430: OPTIONS should complete immediately
    TEST_ASSERT_EQUAL(API_BUFFER_COMPLETE, result);
    TEST_ASSERT_NOT_NULL(con_cls);
    TEST_ASSERT_NOT_NULL(buffer_out);
    TEST_ASSERT_EQUAL('O', buffer_out->http_method);
    TEST_ASSERT_NULL(buffer_out->data);
    TEST_ASSERT_EQUAL(0, buffer_out->size);
    
    // Cleanup
    api_free_post_buffer(&con_cls);
}

// Test POST request first call
void test_api_buffer_post_data_post_first_call(void) {
    void *con_cls = NULL;
    ApiPostBuffer *buffer_out = NULL;
    size_t upload_data_size = 0;
    
    // First call with POST
    ApiBufferResult result = api_buffer_post_data(
        "POST", 
        NULL, 
        &upload_data_size, 
        &con_cls, 
        &buffer_out
    );
    
    // Lines 401-443: POST should allocate buffer and continue
    TEST_ASSERT_EQUAL(API_BUFFER_CONTINUE, result);
    TEST_ASSERT_NOT_NULL(con_cls);
    
    ApiPostBuffer *buffer = (ApiPostBuffer *)con_cls;
    TEST_ASSERT_EQUAL('P', buffer->http_method);
    TEST_ASSERT_NOT_NULL(buffer->data);
    TEST_ASSERT_EQUAL(API_INITIAL_BUFFER_CAPACITY, buffer->capacity);
    TEST_ASSERT_EQUAL(0, buffer->size);
    
    // Cleanup
    api_free_post_buffer(&con_cls);
}

// Test POST data accumulation
void test_api_buffer_post_data_post_accumulate(void) {
    void *con_cls = NULL;
    ApiPostBuffer *buffer_out = NULL;
    size_t upload_data_size = 0;
    
    // First call
    api_buffer_post_data("POST", NULL, &upload_data_size, &con_cls, &buffer_out);
    
    // Second call with data
    const char *data = "key1=value1&key2=value2";
    upload_data_size = strlen(data);
    
    ApiBufferResult result = api_buffer_post_data(
        "POST",
        data,
        &upload_data_size,
        &con_cls,
        &buffer_out
    );
    
    // Lines 456-490: Should accumulate data
    TEST_ASSERT_EQUAL(API_BUFFER_CONTINUE, result);
    TEST_ASSERT_EQUAL(0, upload_data_size);  // Should be cleared
    
    ApiPostBuffer *buffer = (ApiPostBuffer *)con_cls;
    TEST_ASSERT_EQUAL(strlen(data), buffer->size);
    TEST_ASSERT_EQUAL_MEMORY(data, buffer->data, strlen(data));
    
    // Cleanup
    api_free_post_buffer(&con_cls);
}

// Test POST complete
void test_api_buffer_post_data_post_complete(void) {
    void *con_cls = NULL;
    ApiPostBuffer *buffer_out = NULL;
    size_t upload_data_size = 0;
    
    // First call
    api_buffer_post_data("POST", NULL, &upload_data_size, &con_cls, &buffer_out);
    
    // Second call with data
    const char *data = "test_data";
    upload_data_size = strlen(data);
    api_buffer_post_data("POST", data, &upload_data_size, &con_cls, &buffer_out);
    
    // Final call with upload_data_size == 0
    upload_data_size = 0;
    ApiBufferResult result = api_buffer_post_data(
        "POST",
        NULL,
        &upload_data_size,
        &con_cls,
        &buffer_out
    );
    
    // Lines 493-495: Should return COMPLETE
    TEST_ASSERT_EQUAL(API_BUFFER_COMPLETE, result);
    TEST_ASSERT_NOT_NULL(buffer_out);
    
    // Cleanup
    api_free_post_buffer(&con_cls);
}

// Test POST exceeding maximum size
void test_api_buffer_post_data_post_exceeds_max(void) {
    void *con_cls = NULL;
    ApiPostBuffer *buffer_out = NULL;
    size_t upload_data_size = 0;
    
    // First call
    api_buffer_post_data("POST", NULL, &upload_data_size, &con_cls, &buffer_out);
    
    // Try to send data larger than API_MAX_POST_SIZE
    char large_data[1000];
    memset(large_data, 'A', sizeof(large_data));
    
    ApiPostBuffer *buffer = (ApiPostBuffer *)con_cls;
    buffer->size = API_MAX_POST_SIZE - 100;  // Set to near max
    upload_data_size = 200;  // This would exceed
    
    ApiBufferResult result = api_buffer_post_data(
        "POST",
        large_data,
        &upload_data_size,
        &con_cls,
        &buffer_out
    );
    
    // Lines 458-461: Should return ERROR
    TEST_ASSERT_EQUAL(API_BUFFER_ERROR, result);
    
    // Cleanup
    api_free_post_buffer(&con_cls);
}

// Test POST buffer growing
void test_api_buffer_post_data_post_grow_buffer(void) {
    void *con_cls = NULL;
    ApiPostBuffer *buffer_out = NULL;
    size_t upload_data_size = 0;
    
    // First call
    api_buffer_post_data("POST", NULL, &upload_data_size, &con_cls, &buffer_out);
    
    // Add data that will cause buffer to grow
    ApiPostBuffer *buffer = (ApiPostBuffer *)con_cls;
    size_t initial_capacity = buffer->capacity;
    
    // Create data larger than initial capacity
    size_t large_size = initial_capacity + 100;
    char *large_data = malloc(large_size);
    TEST_ASSERT_NOT_NULL(large_data);  // Check malloc succeeded
    memset(large_data, 'B', large_size);
    large_data[large_size - 1] = '\0';
    
    upload_data_size = large_size - 1;
    ApiBufferResult result = api_buffer_post_data(
        "POST",
        large_data,
        &upload_data_size,
        &con_cls,
        &buffer_out
    );
    
    // Lines 467-480: Buffer should have grown
    TEST_ASSERT_EQUAL(API_BUFFER_CONTINUE, result);
    TEST_ASSERT_GREATER_THAN(initial_capacity, buffer->capacity);
    
    free(large_data);
    api_free_post_buffer(&con_cls);
}

// Test unsupported HTTP method
void test_api_buffer_post_data_unsupported_method(void) {
    void *con_cls = NULL;
    ApiPostBuffer *buffer_out = NULL;
    size_t upload_data_size = 0;
    
    // Try unsupported method
    ApiBufferResult result = api_buffer_post_data(
        "PUT",
        NULL,
        &upload_data_size,
        &con_cls,
        &buffer_out
    );
    
    // Lines 406-409: Should return METHOD_ERROR
    TEST_ASSERT_EQUAL(API_BUFFER_METHOD_ERROR, result);
    TEST_ASSERT_NULL(con_cls);  // Should not allocate
}

// Test NULL method
void test_api_buffer_post_data_null_method(void) {
    void *con_cls = NULL;
    ApiPostBuffer *buffer_out = NULL;
    size_t upload_data_size = 0;
    
    // Try NULL method
    ApiBufferResult result = api_buffer_post_data(
        NULL,
        NULL,
        &upload_data_size,
        &con_cls,
        &buffer_out
    );
    
    // Lines 406-409: Should return METHOD_ERROR
    TEST_ASSERT_EQUAL(API_BUFFER_METHOD_ERROR, result);
    TEST_ASSERT_NULL(con_cls);
}

// Test GET request on subsequent calls
void test_api_buffer_post_data_get_subsequent_call(void) {
    void *con_cls = NULL;
    ApiPostBuffer *buffer_out = NULL;
    size_t upload_data_size = 0;
    
    // First call with GET
    api_buffer_post_data("GET", NULL, &upload_data_size, &con_cls, &buffer_out);
    
    // Subsequent call
    ApiBufferResult result = api_buffer_post_data(
        "GET",
        NULL,
        &upload_data_size,
        &con_cls,
        &buffer_out
    );
    
    // Lines 450-452: Should return COMPLETE again
    TEST_ASSERT_EQUAL(API_BUFFER_COMPLETE, result);
    TEST_ASSERT_NOT_NULL(buffer_out);
    
    // Cleanup
    api_free_post_buffer(&con_cls);
}

// Test OPTIONS request on subsequent calls
void test_api_buffer_post_data_options_subsequent_call(void) {
    void *con_cls = NULL;
    ApiPostBuffer *buffer_out = NULL;
    size_t upload_data_size = 0;
    
    // First call with OPTIONS
    api_buffer_post_data("OPTIONS", NULL, &upload_data_size, &con_cls, &buffer_out);
    
    // Subsequent call
    ApiBufferResult result = api_buffer_post_data(
        "OPTIONS",
        NULL,
        &upload_data_size,
        &con_cls,
        &buffer_out
    );
    
    // Lines 450-452: Should return COMPLETE again
    TEST_ASSERT_EQUAL(API_BUFFER_COMPLETE, result);
    TEST_ASSERT_NOT_NULL(buffer_out);
    
    // Cleanup
    api_free_post_buffer(&con_cls);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_api_buffer_post_data_get_request);
    RUN_TEST(test_api_buffer_post_data_options_request);
    RUN_TEST(test_api_buffer_post_data_post_first_call);
    RUN_TEST(test_api_buffer_post_data_post_accumulate);
    RUN_TEST(test_api_buffer_post_data_post_complete);
    RUN_TEST(test_api_buffer_post_data_post_exceeds_max);
    RUN_TEST(test_api_buffer_post_data_post_grow_buffer);
    RUN_TEST(test_api_buffer_post_data_unsupported_method);
    RUN_TEST(test_api_buffer_post_data_null_method);
    RUN_TEST(test_api_buffer_post_data_get_subsequent_call);
    RUN_TEST(test_api_buffer_post_data_options_subsequent_call);
    
    return UNITY_END();
}
