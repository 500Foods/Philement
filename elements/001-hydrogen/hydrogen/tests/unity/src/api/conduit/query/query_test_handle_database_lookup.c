/*
 * Unity Test File: handle_database_lookup
 * This file contains unit tests for handle_database_lookup function
 * in src/api/conduit/query/query.c
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/conduit/query/query.h>

// Enable mocks for testing
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Mock for api_send_json_response and json_decref
enum MHD_Result mock_api_send_json_response(struct MHD_Connection *connection, json_t* response, unsigned int status);
void mock_json_decref(json_t* json);

// Mock MHD_Connection struct (minimal)
typedef struct MockMHDConnection {
    int dummy;
} MockMHDConnection;

// Function prototypes
void test_handle_database_lookup_database_not_found(void);

// Forward declaration for the function being tested
enum MHD_Result handle_database_lookup(struct MHD_Connection *connection, const char* database,
                                      int query_ref, DatabaseQueue** db_queue, QueryCacheEntry** cache_entry,
                                      bool* query_not_found);

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
}

// Mock implementations
enum MHD_Result mock_api_send_json_response(struct MHD_Connection *connection, json_t* response, unsigned int status) {
    (void)connection;
    (void)response;
    (void)status;
    return MHD_YES;
}

void mock_json_decref(json_t* json) {
    (void)json;
}

// Test database not found error path
void test_handle_database_lookup_database_not_found(void) {
    // Mock connection
    MockMHDConnection mock_connection = {0};

    DatabaseQueue* db_queue = NULL;
    QueryCacheEntry* cache_entry = NULL;
    bool query_not_found = false;

    enum MHD_Result result = handle_database_lookup((struct MHD_Connection*)&mock_connection, "nonexistent_db", 123, &db_queue, &cache_entry, &query_not_found);

    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(db_queue);
    TEST_ASSERT_NULL(cache_entry);
    TEST_ASSERT_FALSE(query_not_found);
}


int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_database_lookup_database_not_found);

    return UNITY_END();
}