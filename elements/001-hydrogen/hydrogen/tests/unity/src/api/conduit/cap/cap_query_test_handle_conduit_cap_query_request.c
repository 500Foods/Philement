/*
 * Unity Test File: handle_conduit_cap_query_request
 * Tests the handle_conduit_cap_query_request function in
 * src/api/conduit/cap/cap_query.c.
 *
 * The database-dependent conduit helpers are redirected to fakes via the
 * USE_MOCK_CONDUIT_HELPERS seam (see cap_query.c), letting every
 * early-return / error branch be exercised without a live database.
 *
 * CHANGELOG:
 * 2026-07-16: Initial creation covering buffer-phase, method/parse/field
 *             errors, Cap verification paths, and the full success flow.
 *
 * TEST_VERSION: 1.0.0
 */

#include <src/hydrogen.h>
#include <unity.h>

// Mock conduit helpers (USE_MOCK_CONDUIT_HELPERS is applied to this TU by CMake)
#include <unity/mocks/mock_conduit_helpers.h>

// cap_query.h exposes the three previously-static helpers for direct testing
#include <src/api/conduit/cap/cap_query.h>

// Function prototypes for test functions
void test_cap_request_buffer_method_error(void);
void test_cap_request_method_validation_fails(void);
void test_cap_request_parsing_fails(void);
void test_cap_request_field_extraction_fails(void);
void test_cap_request_missing_token(void);
void test_cap_request_verify_hard_fail(void);
void test_cap_request_verify_token_rejected(void);
void test_cap_request_verify_fallback(void);
void test_cap_request_database_lookup_fails(void);
void test_cap_request_no_db_queue(void);
void test_cap_request_query_not_found(void);
void test_cap_request_statement_type_not_allowed(void);
void test_cap_request_parameter_processing_fails(void);
void test_cap_request_queue_selection_fails(void);
void test_cap_request_query_id_generation_fails(void);
void test_cap_request_pending_registration_fails(void);
void test_cap_request_query_submission_fails(void);
void test_cap_request_success(void);

void setUp(void) {
    mock_conduit_helpers_reset_all();
}

void tearDown(void) {
    mock_conduit_helpers_reset_all();
}

// Unsupported HTTP method -> api_buffer_post_data returns API_BUFFER_METHOD_ERROR
void test_cap_request_buffer_method_error(void) {
    struct MHD_Connection *conn = (void *)0x123;
    const char *url = "/api/conduit/cap_query";
    const char *method = "PUT";
    const char *upload_data = NULL;
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    enum MHD_Result result = handle_conduit_cap_query_request(
        conn, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// handle_method_validation returns failure
void test_cap_request_method_validation_fails(void) {
    struct MHD_Connection *conn = (void *)0x123;
    const char *url = "/api/conduit/cap_query";
    const char *method = "GET";
    const char *upload_data = NULL;
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_conduit_set_method_validation_result(MHD_NO);

    enum MHD_Result result = handle_conduit_cap_query_request(
        conn, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// handle_request_parsing_with_buffer returns failure
void test_cap_request_parsing_fails(void) {
    struct MHD_Connection *conn = (void *)0x123;
    const char *url = "/api/conduit/cap_query";
    const char *method = "GET";
    const char *upload_data = NULL;
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_conduit_set_request_parsing_result(MHD_NO);

    enum MHD_Result result = handle_conduit_cap_query_request(
        conn, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// handle_field_extraction returns failure
void test_cap_request_field_extraction_fails(void) {
    struct MHD_Connection *conn = (void *)0x123;
    const char *url = "/api/conduit/cap_query";
    const char *method = "GET";
    const char *upload_data = NULL;
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    mock_conduit_set_field_extraction_result(MHD_NO);

    enum MHD_Result result = handle_conduit_cap_query_request(
        conn, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Request JSON present but chachaToken missing -> CAP_TOKEN_MISSING
void test_cap_request_missing_token(void) {
    struct MHD_Connection *conn = (void *)0x123;
    const char *url = "/api/conduit/cap_query";
    const char *method = "GET";
    const char *upload_data = NULL;
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    json_t *body = json_object();
    json_object_set_new(body, "query_ref", json_integer(1));
    mock_conduit_set_request_json(body);

    enum MHD_Result result = handle_conduit_cap_query_request(
        conn, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Cap verification hard fail (generic) -> CAP_VERIFY_FAILED
void test_cap_request_verify_hard_fail(void) {
    struct MHD_Connection *conn = (void *)0x123;
    const char *url = "/api/conduit/cap_query";
    const char *method = "GET";
    const char *upload_data = NULL;
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    json_t *body = json_object();
    json_object_set_new(body, "query_ref", json_integer(1));
    json_object_set_new(body, "chachaToken", json_string("tok"));
    mock_conduit_set_request_json(body);

    mock_conduit_set_cap_verify_result(CAP_VERIFY_HARD_FAIL);
    mock_conduit_set_cap_verify_error("CAP_VERIFY_FAILED: generic");

    enum MHD_Result result = handle_conduit_cap_query_request(
        conn, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Cap verification hard fail with "token rejected" -> CAP_TOKEN_INVALID
void test_cap_request_verify_token_rejected(void) {
    struct MHD_Connection *conn = (void *)0x123;
    const char *url = "/api/conduit/cap_query";
    const char *method = "GET";
    const char *upload_data = NULL;
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    json_t *body = json_object();
    json_object_set_new(body, "query_ref", json_integer(1));
    json_object_set_new(body, "chachaToken", json_string("tok"));
    mock_conduit_set_request_json(body);

    mock_conduit_set_cap_verify_result(CAP_VERIFY_HARD_FAIL);
    mock_conduit_set_cap_verify_error("CAP_VERIFY_FAILED: token rejected");

    enum MHD_Result result = handle_conduit_cap_query_request(
        conn, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Cap verification transient failure -> fallback accepted
void test_cap_request_verify_fallback(void) {
    struct MHD_Connection *conn = (void *)0x123;
    const char *url = "/api/conduit/cap_query";
    const char *method = "GET";
    const char *upload_data = NULL;
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    json_t *body = json_object();
    json_object_set_new(body, "query_ref", json_integer(1));
    json_object_set_new(body, "chachaToken", json_string("tok"));
    mock_conduit_set_request_json(body);

    mock_conduit_set_cap_verify_result(CAP_VERIFY_FALLBACK);
    mock_conduit_set_cap_verify_error("CAP_VERIFY_FAILED: HTTP 502");

    enum MHD_Result result = handle_conduit_cap_query_request(
        conn, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// handle_database_lookup returns failure
void test_cap_request_database_lookup_fails(void) {
    struct MHD_Connection *conn = (void *)0x123;
    const char *url = "/api/conduit/cap_query";
    const char *method = "GET";
    const char *upload_data = NULL;
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    json_t *body = json_object();
    json_object_set_new(body, "query_ref", json_integer(1));
    json_object_set_new(body, "chachaToken", json_string("tok"));
    mock_conduit_set_request_json(body);

    mock_conduit_set_cap_verify_result(CAP_VERIFY_OK);
    mock_conduit_set_database_lookup_result(MHD_NO);

    enum MHD_Result result = handle_conduit_cap_query_request(
        conn, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// database lookup succeeds but returns no db_queue
void test_cap_request_no_db_queue(void) {
    struct MHD_Connection *conn = (void *)0x123;
    const char *url = "/api/conduit/cap_query";
    const char *method = "GET";
    const char *upload_data = NULL;
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    json_t *body = json_object();
    json_object_set_new(body, "query_ref", json_integer(1));
    json_object_set_new(body, "chachaToken", json_string("tok"));
    mock_conduit_set_request_json(body);

    mock_conduit_set_cap_verify_result(CAP_VERIFY_OK);
    mock_conduit_set_database_lookup_result(MHD_YES);
    mock_conduit_set_db_queue(NULL);

    enum MHD_Result result = handle_conduit_cap_query_request(
        conn, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// database lookup reports query_not_found -> invalid queryref response
void test_cap_request_query_not_found(void) {
    struct MHD_Connection *conn = (void *)0x123;
    const char *url = "/api/conduit/cap_query";
    const char *method = "GET";
    const char *upload_data = NULL;
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    json_t *body = json_object();
    json_object_set_new(body, "query_ref", json_integer(1));
    json_object_set_new(body, "chachaToken", json_string("tok"));
    mock_conduit_set_request_json(body);

    mock_conduit_set_cap_verify_result(CAP_VERIFY_OK);
    mock_conduit_set_database_lookup_result(MHD_YES);
    mock_conduit_set_db_queue((DatabaseQueue *)0x1);
    mock_conduit_set_query_not_found(true);

    enum MHD_Result result = handle_conduit_cap_query_request(
        conn, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// statement type not allowed for the protected query
void test_cap_request_statement_type_not_allowed(void) {
    struct MHD_Connection *conn = (void *)0x123;
    const char *url = "/api/conduit/cap_query";
    const char *method = "GET";
    const char *upload_data = NULL;
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    json_t *body = json_object();
    json_object_set_new(body, "query_ref", json_integer(1));
    json_object_set_new(body, "chachaToken", json_string("tok"));
    mock_conduit_set_request_json(body);

    QueryCacheEntry entry = {0};
    entry.query_type = 11;
    entry.sql_template = (char *)"DROP TABLE x";

    mock_conduit_set_cap_verify_result(CAP_VERIFY_OK);
    mock_conduit_set_database_lookup_result(MHD_YES);
    mock_conduit_set_db_queue((DatabaseQueue *)0x1);
    mock_conduit_set_cache_entry(&entry);
    mock_conduit_set_query_not_found(false);
    mock_conduit_set_statement_type_allowed(false);

    enum MHD_Result result = handle_conduit_cap_query_request(
        conn, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// parameter processing fails
void test_cap_request_parameter_processing_fails(void) {
    struct MHD_Connection *conn = (void *)0x123;
    const char *url = "/api/conduit/cap_query";
    const char *method = "GET";
    const char *upload_data = NULL;
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    json_t *body = json_object();
    json_object_set_new(body, "query_ref", json_integer(1));
    json_object_set_new(body, "chachaToken", json_string("tok"));
    mock_conduit_set_request_json(body);

    QueryCacheEntry entry = {0};
    entry.query_type = 11;

    mock_conduit_set_cap_verify_result(CAP_VERIFY_OK);
    mock_conduit_set_database_lookup_result(MHD_YES);
    mock_conduit_set_db_queue((DatabaseQueue *)0x1);
    mock_conduit_set_cache_entry(&entry);
    mock_conduit_set_query_not_found(false);
    mock_conduit_set_statement_type_allowed(true);
    mock_conduit_set_parameter_processing_result(MHD_NO);

    enum MHD_Result result = handle_conduit_cap_query_request(
        conn, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// queue selection fails (handle_cap_queue_selection returns MHD_NO)
void test_cap_request_queue_selection_fails(void) {
    struct MHD_Connection *conn = (void *)0x123;
    const char *url = "/api/conduit/cap_query";
    const char *method = "GET";
    const char *upload_data = NULL;
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    json_t *body = json_object();
    json_object_set_new(body, "query_ref", json_integer(1));
    json_object_set_new(body, "chachaToken", json_string("tok"));
    mock_conduit_set_request_json(body);

    QueryCacheEntry entry = {0};
    entry.query_type = 11;

    mock_conduit_set_cap_verify_result(CAP_VERIFY_OK);
    mock_conduit_set_database_lookup_result(MHD_YES);
    mock_conduit_set_db_queue((DatabaseQueue *)0x1);
    mock_conduit_set_cache_entry(&entry);
    mock_conduit_set_query_not_found(false);
    mock_conduit_set_statement_type_allowed(true);
    mock_conduit_set_parameter_processing_result(MHD_YES);
    mock_conduit_set_converted_sql(strdup("SELECT 1"));
    mock_conduit_set_queue_selection_result(MHD_NO);

    enum MHD_Result result = handle_conduit_cap_query_request(
        conn, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// query id generation fails
void test_cap_request_query_id_generation_fails(void) {
    struct MHD_Connection *conn = (void *)0x123;
    const char *url = "/api/conduit/cap_query";
    const char *method = "GET";
    const char *upload_data = NULL;
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    json_t *body = json_object();
    json_object_set_new(body, "query_ref", json_integer(1));
    json_object_set_new(body, "chachaToken", json_string("tok"));
    mock_conduit_set_request_json(body);

    QueryCacheEntry entry = {0};
    entry.query_type = 11;

    mock_conduit_set_cap_verify_result(CAP_VERIFY_OK);
    mock_conduit_set_database_lookup_result(MHD_YES);
    mock_conduit_set_db_queue((DatabaseQueue *)0x1);
    mock_conduit_set_cache_entry(&entry);
    mock_conduit_set_query_not_found(false);
    mock_conduit_set_statement_type_allowed(true);
    mock_conduit_set_parameter_processing_result(MHD_YES);
    mock_conduit_set_converted_sql(strdup("SELECT 1"));
    mock_conduit_set_queue_selection_result(MHD_YES);
    mock_conduit_set_query_id_generation_result(MHD_NO);

    enum MHD_Result result = handle_conduit_cap_query_request(
        conn, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// pending registration fails
void test_cap_request_pending_registration_fails(void) {
    struct MHD_Connection *conn = (void *)0x123;
    const char *url = "/api/conduit/cap_query";
    const char *method = "GET";
    const char *upload_data = NULL;
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    json_t *body = json_object();
    json_object_set_new(body, "query_ref", json_integer(1));
    json_object_set_new(body, "chachaToken", json_string("tok"));
    mock_conduit_set_request_json(body);

    QueryCacheEntry entry = {0};
    entry.query_type = 11;

    mock_conduit_set_cap_verify_result(CAP_VERIFY_OK);
    mock_conduit_set_database_lookup_result(MHD_YES);
    mock_conduit_set_db_queue((DatabaseQueue *)0x1);
    mock_conduit_set_cache_entry(&entry);
    mock_conduit_set_query_not_found(false);
    mock_conduit_set_statement_type_allowed(true);
    mock_conduit_set_parameter_processing_result(MHD_YES);
    mock_conduit_set_converted_sql(strdup("SELECT 1"));
    mock_conduit_set_queue_selection_result(MHD_YES);
    mock_conduit_set_query_id_generation_result(MHD_YES);
    mock_conduit_set_pending_registration_result(MHD_NO);

    enum MHD_Result result = handle_conduit_cap_query_request(
        conn, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// query submission fails
void test_cap_request_query_submission_fails(void) {
    struct MHD_Connection *conn = (void *)0x123;
    const char *url = "/api/conduit/cap_query";
    const char *method = "GET";
    const char *upload_data = NULL;
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    json_t *body = json_object();
    json_object_set_new(body, "query_ref", json_integer(1));
    json_object_set_new(body, "chachaToken", json_string("tok"));
    mock_conduit_set_request_json(body);

    QueryCacheEntry entry = {0};
    entry.query_type = 11;

    mock_conduit_set_cap_verify_result(CAP_VERIFY_OK);
    mock_conduit_set_database_lookup_result(MHD_YES);
    mock_conduit_set_db_queue((DatabaseQueue *)0x1);
    mock_conduit_set_cache_entry(&entry);
    mock_conduit_set_query_not_found(false);
    mock_conduit_set_statement_type_allowed(true);
    mock_conduit_set_parameter_processing_result(MHD_YES);
    mock_conduit_set_converted_sql(strdup("SELECT 1"));
    mock_conduit_set_queue_selection_result(MHD_YES);
    mock_conduit_set_query_id_generation_result(MHD_YES);
    mock_conduit_set_pending_registration_result(MHD_YES);
    mock_conduit_set_query_submission_result(MHD_NO);

    enum MHD_Result result = handle_conduit_cap_query_request(
        conn, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Full success path through to response building
void test_cap_request_success(void) {
    struct MHD_Connection *conn = (void *)0x123;
    const char *url = "/api/conduit/cap_query";
    const char *method = "GET";
    const char *upload_data = NULL;
    size_t upload_data_size = 0;
    void *con_cls = NULL;

    json_t *body = json_object();
    json_object_set_new(body, "query_ref", json_integer(1));
    json_object_set_new(body, "chachaToken", json_string("tok"));
    json_object_set_new(body, "chachaSite", json_string("site1"));
    mock_conduit_set_request_json(body);

    QueryCacheEntry entry = {0};
    entry.query_type = 11;

    mock_conduit_set_cap_verify_result(CAP_VERIFY_OK);
    mock_conduit_set_database_lookup_result(MHD_YES);
    mock_conduit_set_db_queue((DatabaseQueue *)0x1);
    mock_conduit_set_cache_entry(&entry);
    mock_conduit_set_query_not_found(false);
    mock_conduit_set_statement_type_allowed(true);
    mock_conduit_set_parameter_processing_result(MHD_YES);
    mock_conduit_set_converted_sql(strdup("SELECT 1"));
    mock_conduit_set_queue_selection_result(MHD_YES);
    mock_conduit_set_query_id_generation_result(MHD_YES);
    mock_conduit_set_pending_registration_result(MHD_YES);
    mock_conduit_set_query_submission_result(MHD_YES);
    mock_conduit_set_response_building_result(MHD_YES);

    enum MHD_Result result = handle_conduit_cap_query_request(
        conn, url, method, upload_data, &upload_data_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_cap_request_buffer_method_error);
    RUN_TEST(test_cap_request_method_validation_fails);
    RUN_TEST(test_cap_request_parsing_fails);
    RUN_TEST(test_cap_request_field_extraction_fails);
    RUN_TEST(test_cap_request_missing_token);
    RUN_TEST(test_cap_request_verify_hard_fail);
    RUN_TEST(test_cap_request_verify_token_rejected);
    RUN_TEST(test_cap_request_verify_fallback);
    RUN_TEST(test_cap_request_database_lookup_fails);
    RUN_TEST(test_cap_request_no_db_queue);
    RUN_TEST(test_cap_request_query_not_found);
    RUN_TEST(test_cap_request_statement_type_not_allowed);
    RUN_TEST(test_cap_request_parameter_processing_fails);
    RUN_TEST(test_cap_request_queue_selection_fails);
    RUN_TEST(test_cap_request_query_id_generation_fails);
    RUN_TEST(test_cap_request_pending_registration_fails);
    RUN_TEST(test_cap_request_query_submission_fails);
    RUN_TEST(test_cap_request_success);

    return UNITY_END();
}
