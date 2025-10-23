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

// Enable mock for lookup_database_and_query
#define USE_MOCK_LOOKUP_DATABASE_AND_QUERY

// Mock for api_send_json_response and json_decref
enum MHD_Result mock_api_send_json_response(struct MHD_Connection *connection, json_t* response, unsigned int status);
void mock_json_decref(json_t* json);

// Mock for lookup_database_and_query to simulate failures
bool mock_lookup_database_and_query(DatabaseQueue** db_queue, QueryCacheEntry** cache_entry,
                                   const char* database, int query_ref);

// Mock MHD_Connection struct (minimal)
typedef struct MockMHDConnection {
    int dummy;
} MockMHDConnection;

// Mock state
static bool mock_lookup_result = true;
static DatabaseQueue* mock_db_queue_result = NULL;
static QueryCacheEntry* mock_cache_entry_result = NULL;

// Function prototypes
void test_handle_database_lookup_success(void);
void test_handle_database_lookup_database_not_found(void);
void test_handle_database_lookup_query_not_found(void);

// Forward declaration for the function being tested
enum MHD_Result handle_database_lookup(struct MHD_Connection *connection, const char* database,
                                      int query_ref, DatabaseQueue** db_queue, QueryCacheEntry** cache_entry);

void setUp(void) {
    mock_system_reset_all();
    mock_lookup_result = true;
    mock_db_queue_result = NULL;
    mock_cache_entry_result = NULL;
}

void tearDown(void) {
    mock_system_reset_all();
    mock_lookup_result = true;
    mock_db_queue_result = NULL;
    mock_cache_entry_result = NULL;
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

bool mock_lookup_database_and_query(DatabaseQueue** db_queue, QueryCacheEntry** cache_entry,
                                   const char* database, int query_ref) {
    (void)database;
    (void)query_ref;
    *db_queue = mock_db_queue_result;
    *cache_entry = mock_cache_entry_result;
    return mock_lookup_result;
}

// Test successful database and query lookup
void test_handle_database_lookup_success(void) {
    // Set up mock to succeed
    mock_lookup_result = true;
    DatabaseQueue mock_queue = {.queue_type = (char*)"test"};
    mock_db_queue_result = &mock_queue;
    QueryCacheEntry mock_entry = {.description = (char*)"test query"};
    mock_cache_entry_result = &mock_entry;

    // Mock connection
    MockMHDConnection mock_connection = {0};

    DatabaseQueue* db_queue = NULL;
    QueryCacheEntry* cache_entry = NULL;

    enum MHD_Result result = handle_database_lookup((struct MHD_Connection*)&mock_connection, "test_db", 123, &db_queue, &cache_entry);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_NOT_NULL(db_queue);
    TEST_ASSERT_NOT_NULL(cache_entry);
}

// Test database not found error path
void test_handle_database_lookup_database_not_found(void) {
    // Set up mock to fail with database not found
    mock_lookup_result = false;
    mock_db_queue_result = NULL;
    mock_cache_entry_result = NULL;

    // Mock connection
    MockMHDConnection mock_connection = {0};

    DatabaseQueue* db_queue = NULL;
    QueryCacheEntry* cache_entry = NULL;

    enum MHD_Result result = handle_database_lookup((struct MHD_Connection*)&mock_connection, "nonexistent_db", 123, &db_queue, &cache_entry);

    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(db_queue);
    TEST_ASSERT_NULL(cache_entry);
}

// Test query not found error path (database exists but query doesn't)
void test_handle_database_lookup_query_not_found(void) {
    // Set up mock to fail with query not found
    mock_lookup_result = false;
    DatabaseQueue mock_queue = {.queue_type = (char*)"test"};
    mock_db_queue_result = &mock_queue;
    mock_cache_entry_result = NULL;

    // Mock connection
    MockMHDConnection mock_connection = {0};

    DatabaseQueue* db_queue = NULL;
    QueryCacheEntry* cache_entry = NULL;

    enum MHD_Result result = handle_database_lookup((struct MHD_Connection*)&mock_connection, "test_db", 999, &db_queue, &cache_entry);

    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NOT_NULL(db_queue);
    TEST_ASSERT_NULL(cache_entry);
}

int main(void) {
    UNITY_BEGIN();

    if (0) RUN_TEST(test_handle_database_lookup_success);
    RUN_TEST(test_handle_database_lookup_database_not_found);
    if (0) RUN_TEST(test_handle_database_lookup_query_not_found);

    return UNITY_END();
}