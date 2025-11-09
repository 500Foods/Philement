/*
 * Unity Test File: handle_field_extraction
 * This file contains unit tests for handle_field_extraction function
 * in src/api/conduit/query/query.c
 */

// Project includes
#include <src/hydrogen.h>
#include "unity.h"

// Include source header
#include <src/api/conduit/query/query.h>

// Enable mocks for testing
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#define USE_MOCK_LIBMICROHTTPD
#include <unity/mocks/mock_libmicrohttpd.h>

// Enable mock for extract_request_fields
#define USE_MOCK_EXTRACT_REQUEST_FIELDS

// Mock for extract_request_fields
static bool mock_extract_request_fields(json_t* request_json, int* query_ref, const char** database, json_t** params);

// Mock state
static bool mock_extract_result = true;

// Mock MHD_Connection struct (minimal)
typedef struct MockMHDConnection {
    int dummy;
} MockMHDConnection;

// Function prototypes
void test_handle_field_extraction_success(void);
void test_handle_field_extraction_failure(void);

// Forward declaration for the function being tested
enum MHD_Result handle_field_extraction(struct MHD_Connection *connection, json_t* request_json,
                                       int* query_ref, const char** database, json_t** params_json);

void setUp(void) {
    mock_system_reset_all();
    mock_mhd_reset_all();
    mock_extract_result = true;
}

void tearDown(void) {
    mock_system_reset_all();
    mock_mhd_reset_all();
}

// Mock implementations
__attribute__((used)) static bool mock_extract_request_fields(json_t* request_json, int* query_ref, const char** database, json_t** params) {
    (void)request_json;

    if (mock_extract_result) {
        *query_ref = 123;
        *database = "test_db";
        *params = json_object();
        return true;
    } else {
        return false;
    }
}

// Test field extraction failure
void test_handle_field_extraction_failure(void) {
    // Mock connection
    MockMHDConnection mock_connection = {0};

    // Set up mock to fail
    mock_extract_result = false;

    json_t* request_json = json_object();
    int query_ref = 0;
    const char* database = NULL;
    json_t* params_json = NULL;

    enum MHD_Result result = handle_field_extraction((struct MHD_Connection*)&mock_connection, request_json,
                                                   &query_ref, &database, &params_json);

    TEST_ASSERT_EQUAL(MHD_NO, result);

    // Cleanup
    json_decref(request_json);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_field_extraction_failure);

    return UNITY_END();
}