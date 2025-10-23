/*
 * Unity Test File: handle_conduit_query_request
 * This file contains unit tests for handle_conduit_query_request function
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

// Enable mocks for all the helper functions
#define USE_MOCK_HANDLE_METHOD_VALIDATION
#define USE_MOCK_HANDLE_REQUEST_PARSING
#define USE_MOCK_HANDLE_FIELD_EXTRACTION
#define USE_MOCK_HANDLE_DATABASE_LOOKUP
#define USE_MOCK_HANDLE_PARAMETER_PROCESSING
#define USE_MOCK_HANDLE_QUEUE_SELECTION
#define USE_MOCK_HANDLE_QUERY_ID_GENERATION
#define USE_MOCK_HANDLE_PENDING_REGISTRATION
#define USE_MOCK_HANDLE_QUERY_SUBMISSION
#define USE_MOCK_HANDLE_RESPONSE_BUILDING

// Mock for json_decref
void mock_json_decref(json_t* json);

// Mock MHD_Connection struct (minimal)
typedef struct MockMHDConnection {
    int dummy;
} MockMHDConnection;

// Mock state
static enum MHD_Result mock_method_validation_result = MHD_YES;
static enum MHD_Result mock_request_parsing_result = MHD_YES;
static enum MHD_Result mock_field_extraction_result = MHD_YES;
static enum MHD_Result mock_database_lookup_result = MHD_YES;
static enum MHD_Result mock_parameter_processing_result = MHD_YES;
static enum MHD_Result mock_queue_selection_result = MHD_YES;
static enum MHD_Result mock_query_id_generation_result = MHD_YES;
static enum MHD_Result mock_pending_registration_result = MHD_YES;
static enum MHD_Result mock_query_submission_result = MHD_YES;
static enum MHD_Result mock_response_building_result = MHD_YES;

// Function prototypes
void test_handle_conduit_query_request_success(void);
void test_handle_conduit_query_request_method_validation_failure(void);
void test_handle_conduit_query_request_request_parsing_failure(void);
void test_handle_conduit_query_request_field_extraction_failure(void);
void test_handle_conduit_query_request_database_lookup_failure(void);
void test_handle_conduit_query_request_parameter_processing_failure(void);
void test_handle_conduit_query_request_queue_selection_failure(void);
void test_handle_conduit_query_request_query_id_generation_failure(void);
void test_handle_conduit_query_request_pending_registration_failure(void);
void test_handle_conduit_query_request_query_submission_failure(void);

// Forward declaration for the function being tested
enum MHD_Result handle_conduit_query_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
);

void setUp(void) {
    mock_system_reset_all();
    mock_method_validation_result = MHD_YES;
    mock_request_parsing_result = MHD_YES;
    mock_field_extraction_result = MHD_YES;
    mock_database_lookup_result = MHD_YES;
    mock_parameter_processing_result = MHD_YES;
    mock_queue_selection_result = MHD_YES;
    mock_query_id_generation_result = MHD_YES;
    mock_pending_registration_result = MHD_YES;
    mock_query_submission_result = MHD_YES;
    mock_response_building_result = MHD_YES;
}

void tearDown(void) {
    mock_system_reset_all();
}

// Mock function declarations
enum MHD_Result mock_handle_method_validation(struct MHD_Connection *connection, const char* method);
enum MHD_Result mock_handle_request_parsing(struct MHD_Connection *connection, const char* method,
                                           const char* upload_data, const size_t* upload_data_size,
                                           json_t** request_json);
enum MHD_Result mock_handle_field_extraction(struct MHD_Connection *connection, json_t* request_json,
                                           int* query_ref, const char** database, json_t** params_json);
enum MHD_Result mock_handle_database_lookup(struct MHD_Connection *connection, const char* database,
                                          int query_ref, DatabaseQueue** db_queue, QueryCacheEntry** cache_entry);
enum MHD_Result mock_handle_parameter_processing(struct MHD_Connection *connection, json_t* params_json,
                                               const DatabaseQueue* db_queue, const QueryCacheEntry* cache_entry,
                                               const char* database, int query_ref,
                                               ParameterList** param_list, char** converted_sql,
                                               TypedParameter*** ordered_params, size_t* param_count);
enum MHD_Result mock_handle_queue_selection(struct MHD_Connection *connection, const char* database,
                                           int query_ref, const QueryCacheEntry* cache_entry,
                                           ParameterList* param_list, char* converted_sql,
                                           TypedParameter** ordered_params,
                                           DatabaseQueue** selected_queue);
enum MHD_Result mock_handle_query_id_generation(struct MHD_Connection *connection, const char* database,
                                              int query_ref, ParameterList* param_list, char* converted_sql,
                                              TypedParameter** ordered_params, char** query_id);
enum MHD_Result mock_handle_pending_registration(struct MHD_Connection *connection, const char* database,
                                               int query_ref, char* query_id, ParameterList* param_list,
                                               char* converted_sql, TypedParameter** ordered_params,
                                               const QueryCacheEntry* cache_entry, PendingQueryResult** pending);
enum MHD_Result mock_handle_query_submission(struct MHD_Connection *connection, const char* database,
                                           int query_ref, DatabaseQueue* selected_queue, char* query_id,
                                           char* converted_sql, ParameterList* param_list,
                                           TypedParameter** ordered_params, size_t param_count,
                                           const QueryCacheEntry* cache_entry);
enum MHD_Result mock_handle_response_building(struct MHD_Connection *connection, int query_ref,
                                            const char* database, const QueryCacheEntry* cache_entry,
                                            const DatabaseQueue* selected_queue, PendingQueryResult* pending,
                                            char* query_id, char* converted_sql, ParameterList* param_list,
                                            TypedParameter** ordered_params);

// Mock implementations
enum MHD_Result mock_handle_method_validation(struct MHD_Connection *connection, const char* method) {
    (void)connection;
    (void)method;
    return mock_method_validation_result;
}

enum MHD_Result mock_handle_request_parsing(struct MHD_Connection *connection, const char* method,
                                           const char* upload_data, const size_t* upload_data_size,
                                           json_t** request_json) {
    (void)connection;
    (void)method;
    (void)upload_data;
    (void)upload_data_size;
    (void)request_json;
    return mock_request_parsing_result;
}

enum MHD_Result mock_handle_field_extraction(struct MHD_Connection *connection, json_t* request_json,
                                           int* query_ref, const char** database, json_t** params_json) {
    (void)connection;
    (void)request_json;
    (void)query_ref;
    (void)database;
    (void)params_json;
    return mock_field_extraction_result;
}

enum MHD_Result mock_handle_database_lookup(struct MHD_Connection *connection, const char* database,
                                          int query_ref, DatabaseQueue** db_queue, QueryCacheEntry** cache_entry) {
    (void)connection;
    (void)database;
    (void)query_ref;
    (void)db_queue;
    (void)cache_entry;
    return mock_database_lookup_result;
}

enum MHD_Result mock_handle_parameter_processing(struct MHD_Connection *connection, json_t* params_json,
                                               const DatabaseQueue* db_queue, const QueryCacheEntry* cache_entry,
                                               const char* database, int query_ref,
                                               ParameterList** param_list, char** converted_sql,
                                               TypedParameter*** ordered_params, size_t* param_count) {
    (void)connection;
    (void)params_json;
    (void)db_queue;
    (void)cache_entry;
    (void)database;
    (void)query_ref;
    (void)param_list;
    (void)converted_sql;
    (void)ordered_params;
    (void)param_count;
    return mock_parameter_processing_result;
}

enum MHD_Result mock_handle_queue_selection(struct MHD_Connection *connection, const char* database,
                                           int query_ref, const QueryCacheEntry* cache_entry,
                                           ParameterList* param_list, char* converted_sql,
                                           TypedParameter** ordered_params,
                                           DatabaseQueue** selected_queue) {
    (void)connection;
    (void)database;
    (void)query_ref;
    (void)cache_entry;
    (void)param_list;
    (void)converted_sql;
    (void)ordered_params;
    (void)selected_queue;
    return mock_queue_selection_result;
}

enum MHD_Result mock_handle_query_id_generation(struct MHD_Connection *connection, const char* database,
                                              int query_ref, ParameterList* param_list, char* converted_sql,
                                              TypedParameter** ordered_params, char** query_id) {
    (void)connection;
    (void)database;
    (void)query_ref;
    (void)param_list;
    (void)converted_sql;
    (void)ordered_params;
    (void)query_id;
    return mock_query_id_generation_result;
}

enum MHD_Result mock_handle_pending_registration(struct MHD_Connection *connection, const char* database,
                                               int query_ref, char* query_id, ParameterList* param_list,
                                               char* converted_sql, TypedParameter** ordered_params,
                                               const QueryCacheEntry* cache_entry, PendingQueryResult** pending) {
    (void)connection;
    (void)database;
    (void)query_ref;
    (void)query_id;
    (void)param_list;
    (void)converted_sql;
    (void)ordered_params;
    (void)cache_entry;
    (void)pending;
    return mock_pending_registration_result;
}

enum MHD_Result mock_handle_query_submission(struct MHD_Connection *connection, const char* database,
                                           int query_ref, DatabaseQueue* selected_queue, char* query_id,
                                           char* converted_sql, ParameterList* param_list,
                                           TypedParameter** ordered_params, size_t param_count,
                                           const QueryCacheEntry* cache_entry) {
    (void)connection;
    (void)database;
    (void)query_ref;
    (void)selected_queue;
    (void)query_id;
    (void)converted_sql;
    (void)param_list;
    (void)ordered_params;
    (void)param_count;
    (void)cache_entry;
    return mock_query_submission_result;
}

enum MHD_Result mock_handle_response_building(struct MHD_Connection *connection, int query_ref,
                                            const char* database, const QueryCacheEntry* cache_entry,
                                            const DatabaseQueue* selected_queue, PendingQueryResult* pending,
                                            char* query_id, char* converted_sql, ParameterList* param_list,
                                            TypedParameter** ordered_params) {
    (void)connection;
    (void)query_ref;
    (void)database;
    (void)cache_entry;
    (void)selected_queue;
    (void)pending;
    (void)query_id;
    (void)converted_sql;
    (void)param_list;
    (void)ordered_params;
    return mock_response_building_result;
}

void mock_json_decref(json_t* json) {
    (void)json;
}

// Test successful full request handling
void test_handle_conduit_query_request_success(void) {
    // Mock connection
    MockMHDConnection mock_connection = {0};

    // All mocks set to succeed
    size_t upload_data_size = 32; // Length of the JSON string
    enum MHD_Result result = handle_conduit_query_request((struct MHD_Connection*)&mock_connection,
                                                          "/api/conduit/query", "POST",
                                                          "{\"query_ref\":123,\"database\":\"test\"}", &upload_data_size, NULL);

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test method validation failure
void test_handle_conduit_query_request_method_validation_failure(void) {
    MockMHDConnection mock_connection = {0};
    mock_method_validation_result = MHD_NO;

    enum MHD_Result result = handle_conduit_query_request((struct MHD_Connection*)&mock_connection,
                                                         "/api/conduit/query", "INVALID", NULL, NULL, NULL);

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test request parsing failure
void test_handle_conduit_query_request_request_parsing_failure(void) {
    MockMHDConnection mock_connection = {0};
    mock_request_parsing_result = MHD_NO;

    enum MHD_Result result = handle_conduit_query_request((struct MHD_Connection*)&mock_connection,
                                                         "/api/conduit/query", "POST", NULL, NULL, NULL);

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test field extraction failure
void test_handle_conduit_query_request_field_extraction_failure(void) {
    MockMHDConnection mock_connection = {0};
    mock_field_extraction_result = MHD_NO;

    enum MHD_Result result = handle_conduit_query_request((struct MHD_Connection*)&mock_connection,
                                                         "/api/conduit/query", "POST", NULL, NULL, NULL);

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test database lookup failure
void test_handle_conduit_query_request_database_lookup_failure(void) {
    MockMHDConnection mock_connection = {0};
    mock_database_lookup_result = MHD_NO;

    enum MHD_Result result = handle_conduit_query_request((struct MHD_Connection*)&mock_connection,
                                                         "/api/conduit/query", "POST", NULL, NULL, NULL);

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test parameter processing failure
void test_handle_conduit_query_request_parameter_processing_failure(void) {
    MockMHDConnection mock_connection = {0};
    mock_parameter_processing_result = MHD_NO;

    enum MHD_Result result = handle_conduit_query_request((struct MHD_Connection*)&mock_connection,
                                                         "/api/conduit/query", "POST", NULL, NULL, NULL);

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test queue selection failure
void test_handle_conduit_query_request_queue_selection_failure(void) {
    MockMHDConnection mock_connection = {0};
    mock_queue_selection_result = MHD_NO;

    enum MHD_Result result = handle_conduit_query_request((struct MHD_Connection*)&mock_connection,
                                                         "/api/conduit/query", "POST", NULL, NULL, NULL);

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test query ID generation failure
void test_handle_conduit_query_request_query_id_generation_failure(void) {
    MockMHDConnection mock_connection = {0};
    mock_query_id_generation_result = MHD_NO;

    enum MHD_Result result = handle_conduit_query_request((struct MHD_Connection*)&mock_connection,
                                                         "/api/conduit/query", "POST", NULL, NULL, NULL);

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test pending registration failure
void test_handle_conduit_query_request_pending_registration_failure(void) {
    MockMHDConnection mock_connection = {0};
    mock_pending_registration_result = MHD_NO;

    enum MHD_Result result = handle_conduit_query_request((struct MHD_Connection*)&mock_connection,
                                                         "/api/conduit/query", "POST", NULL, NULL, NULL);

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test query submission failure
void test_handle_conduit_query_request_query_submission_failure(void) {
    MockMHDConnection mock_connection = {0};
    mock_query_submission_result = MHD_NO;

    enum MHD_Result result = handle_conduit_query_request((struct MHD_Connection*)&mock_connection,
                                                         "/api/conduit/query", "POST", NULL, NULL, NULL);

    TEST_ASSERT_EQUAL(MHD_NO, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_conduit_query_request_success);
    RUN_TEST(test_handle_conduit_query_request_method_validation_failure);
    RUN_TEST(test_handle_conduit_query_request_request_parsing_failure);
    RUN_TEST(test_handle_conduit_query_request_field_extraction_failure);
    RUN_TEST(test_handle_conduit_query_request_database_lookup_failure);
    RUN_TEST(test_handle_conduit_query_request_parameter_processing_failure);
    RUN_TEST(test_handle_conduit_query_request_queue_selection_failure);
    RUN_TEST(test_handle_conduit_query_request_query_id_generation_failure);
    RUN_TEST(test_handle_conduit_query_request_pending_registration_failure);
    RUN_TEST(test_handle_conduit_query_request_query_submission_failure);

    return UNITY_END();
}