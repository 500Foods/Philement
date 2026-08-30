/*
 * Unity Test File: handle_system_jobs_request Function Tests
 *
 * Tests the /api/system/jobs endpoint handler (src/api/system/jobs/jobs.c).
 *
 * Mocked dependencies:
 *   - MHD_lookup_connection_value (mock_libmicrohttpd) — controls Authorization header
 *   - extract_and_validate_jwt / free_jwt_claims (mock_auth_service_jwt)
 *   - api_send_json_response (mock_api_utils) — with capture mode to inspect responses
 *   - scripting_scoreboard_snapshot_json / scripting_free_job_list (mock_scoreboard_json)
 *   - log_this (mock_logging)
 *
 * Test cases:
 *   1. No Authorization header → 401 with auth error JSON
 *   2. Invalid Authorization format (no Bearer prefix) → 401
 *   3. Valid JWT, snapshot returns NULL → 500 with internal error JSON
 *   4. Valid JWT, snapshot missing "jobs" key → 500 with job list not available
 *   5. Valid JWT, snapshot has empty "jobs" array → 200 with empty array
 *   6. Valid JWT, snapshot has populated "jobs" array → 200 with array containing job entries
 *   7. JWT validation fails (invalid token) → 401
 *   8. API response capture — verify response body and status code structure
 *
 * CHANGELOG:
 * 2026-08-29: Initial creation - Phase 23 LUA_CLIENT system jobs endpoint unit tests
 *
 * TEST_VERSION: 1.0.0
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <string.h>

// Include mock headers
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_auth_service_jwt.h>
#include <unity/mocks/mock_api_utils.h>
#include <unity/mocks/mock_scoreboard_json.h>

// Include the module under test
#include <src/api/system/jobs/jobs.h>

// Mock MHD connection structure (minimal, matching pattern from version_test)
struct MockMHDConnection {
    int dummy;
};

// Forward declarations
void test_jobs_no_auth_header_returns_401(void);
void test_jobs_invalid_auth_format_returns_401(void);
void test_jobs_jwt_validation_failure_returns_401(void);
void test_jobs_snapshot_null_returns_500(void);
void test_jobs_snapshot_missing_jobs_returns_500(void);
void test_jobs_empty_jobs_array_returns_200(void);
void test_jobs_populated_jobs_array_returns_200(void);
void test_jobs_response_capture_and_inspection(void);
void test_jobs_auth_header_with_bearer_prefix_returns_401_on_invalid_token(void);
void test_jobs_function_signature_compilation_check(void);

// Helper: create a valid JWT validation result with claims
static jwt_claims_t* create_mock_claims(void) {
    jwt_claims_t* claims = calloc(1, sizeof(jwt_claims_t));
    if (claims) {
        claims->database = strdup("Acuranzo");
        claims->username = strdup("testuser");
        claims->user_id = 1;
    }
    return claims;
}

// Helper: set up a successful JWT validation mock
static void setup_mock_jwt_valid(void) {
    jwt_validation_result_t mock_result = {0};
    mock_result.valid = true;
    mock_result.error = JWT_ERROR_NONE;
    mock_result.claims = create_mock_claims();
    TEST_ASSERT_NOT_NULL(mock_result.claims);
    mock_auth_service_jwt_set_validation_result(mock_result);
}

// Helper: set up a failed JWT validation mock
static void setup_mock_jwt_invalid(void) {
    jwt_validation_result_t mock_result = {0};
    mock_result.valid = false;
    mock_result.error = JWT_ERROR_INVALID_SIGNATURE;
    mock_result.claims = NULL;
    mock_auth_service_jwt_set_validation_result(mock_result);
}

void setUp(void) {
    // Reset all mock state
    mock_mhd_reset_all();
    mock_auth_service_jwt_reset_all();
    mock_api_utils_reset_all();
    mock_scoreboard_json_reset_all();

    // Enable capture mode for response inspection
    mock_api_utils_set_capture_mode(true);
    mock_api_utils_reset_capture();
}

void tearDown(void) {
    // Reset capture mode
    mock_api_utils_set_capture_mode(false);
    mock_api_utils_reset_capture();
}

// Test 1: No Authorization header → 401 with auth error JSON
void test_jobs_no_auth_header_returns_401(void) {
    struct MockMHDConnection mock_conn = {0};

    // MHD_lookup_connection_value returns NULL (no Authorization header)
    // This is the default mock behavior

    enum MHD_Result result = handle_system_jobs_request((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(1, mock_api_utils_get_send_json_response_call_count());
    TEST_ASSERT_EQUAL(MHD_HTTP_UNAUTHORIZED, mock_api_utils_get_captured_status());

    json_t* captured = mock_api_utils_get_captured_response();
    TEST_ASSERT_NOT_NULL(captured);
    TEST_ASSERT_TRUE(json_is_object(captured));

    json_t* success = json_object_get(captured, "success");
    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_FALSE(json_is_true(success));

    json_t* error = json_object_get(captured, "error");
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL_STRING("Authentication required", json_string_value(error));

    // Free captured response (capture mode stores the pointer)
    json_decref(captured);
    mock_api_utils_reset_capture();
}

// Test 2: Invalid Authorization format (no Bearer prefix) → 401
void test_jobs_invalid_auth_format_returns_401(void) {
    struct MockMHDConnection mock_conn = {0};

    // Set up Authorization header without Bearer prefix
    mock_mhd_add_lookup("Authorization", "Basic dXNlcjpwYXNz");

    enum MHD_Result result = handle_system_jobs_request((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(1, mock_api_utils_get_send_json_response_call_count());
    TEST_ASSERT_EQUAL(MHD_HTTP_UNAUTHORIZED, mock_api_utils_get_captured_status());

    json_t* captured = mock_api_utils_get_captured_response();
    TEST_ASSERT_NOT_NULL(captured);
    TEST_ASSERT_TRUE(json_is_object(captured));

    json_t* success = json_object_get(captured, "success");
    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_FALSE(json_is_true(success));

    json_t* error = json_object_get(captured, "error");
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL_STRING("Authentication required", json_string_value(error));

    json_decref(captured);
    mock_api_utils_reset_capture();
}

// Test 3: JWT validation fails (invalid token) → 401
void test_jobs_jwt_validation_failure_returns_401(void) {
    struct MockMHDConnection mock_conn = {0};

    // Set up Authorization header with Bearer prefix but invalid token
    mock_mhd_add_lookup("Authorization", "Bearer invalid.token.here");
    setup_mock_jwt_invalid();

    enum MHD_Result result = handle_system_jobs_request((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(1, mock_api_utils_get_send_json_response_call_count());
    TEST_ASSERT_EQUAL(MHD_HTTP_UNAUTHORIZED, mock_api_utils_get_captured_status());

    json_t* captured = mock_api_utils_get_captured_response();
    TEST_ASSERT_NOT_NULL(captured);
    TEST_ASSERT_TRUE(json_is_object(captured));

    json_t* success = json_object_get(captured, "success");
    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_FALSE(json_is_true(success));

    json_t* error = json_object_get(captured, "error");
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL_STRING("Authentication required", json_string_value(error));

    json_decref(captured);
    mock_api_utils_reset_capture();
}

// Test 4: Valid JWT, scoreboard snapshot returns NULL → 500
void test_jobs_snapshot_null_returns_500(void) {
    struct MockMHDConnection mock_conn = {0};

    // Set up valid JWT auth
    mock_mhd_add_lookup("Authorization", "Bearer valid.token.here");
    setup_mock_jwt_valid();

    // Set up scoreboard to return NULL
    mock_scoreboard_json_set_snapshot_result(NULL);

    enum MHD_Result result = handle_system_jobs_request((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(1, mock_api_utils_get_send_json_response_call_count());
    TEST_ASSERT_EQUAL(MHD_HTTP_INTERNAL_SERVER_ERROR, mock_api_utils_get_captured_status());

    json_t* captured = mock_api_utils_get_captured_response();
    TEST_ASSERT_NOT_NULL(captured);
    TEST_ASSERT_TRUE(json_is_object(captured));

    json_t* success = json_object_get(captured, "success");
    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_FALSE(json_is_true(success));

    json_t* error = json_object_get(captured, "error");
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL_STRING("Failed to generate job list", json_string_value(error));

    json_decref(captured);
    mock_api_utils_reset_capture();
}

// Test 5: Valid JWT, snapshot missing "jobs" key → 500
void test_jobs_snapshot_missing_jobs_returns_500(void) {
    struct MockMHDConnection mock_conn = {0};

    // Set up valid JWT auth
    mock_mhd_add_lookup("Authorization", "Bearer valid.token.here");
    setup_mock_jwt_valid();

    // Set up scoreboard with snapshot that has no "jobs" key
    json_t* snapshot = json_object();
    TEST_ASSERT_NOT_NULL(snapshot);
    json_object_set_new(snapshot, "enabled", json_true());
    mock_scoreboard_json_set_snapshot_result(snapshot);

    enum MHD_Result result = handle_system_jobs_request((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(1, mock_api_utils_get_send_json_response_call_count());
    TEST_ASSERT_EQUAL(MHD_HTTP_INTERNAL_SERVER_ERROR, mock_api_utils_get_captured_status());

    json_t* captured = mock_api_utils_get_captured_response();
    TEST_ASSERT_NOT_NULL(captured);
    TEST_ASSERT_TRUE(json_is_object(captured));

    json_t* success = json_object_get(captured, "success");
    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_FALSE(json_is_true(success));

    json_t* error = json_object_get(captured, "error");
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL_STRING("Job list not available", json_string_value(error));

    json_decref(captured);
    mock_api_utils_reset_capture();
}

// Test 6: Valid JWT, snapshot has empty "jobs" array → 200
void test_jobs_empty_jobs_array_returns_200(void) {
    struct MockMHDConnection mock_conn = {0};

    // Set up valid JWT auth
    mock_mhd_add_lookup("Authorization", "Bearer valid.token.here");
    setup_mock_jwt_valid();

    // Set up scoreboard with snapshot containing empty jobs array
    json_t* snapshot = json_object();
    TEST_ASSERT_NOT_NULL(snapshot);
    json_object_set_new(snapshot, "jobs", json_array());
    mock_scoreboard_json_set_snapshot_result(snapshot);

    enum MHD_Result result = handle_system_jobs_request((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(1, mock_api_utils_get_send_json_response_call_count());
    TEST_ASSERT_EQUAL(MHD_HTTP_OK, mock_api_utils_get_captured_status());

    json_t* captured = mock_api_utils_get_captured_response();
    TEST_ASSERT_NOT_NULL(captured);
    TEST_ASSERT_TRUE(json_is_array(captured));
    TEST_ASSERT_EQUAL(0, json_array_size(captured));

    json_decref(captured);
    mock_api_utils_reset_capture();
}

// Test 7: Valid JWT, snapshot has populated "jobs" array → 200
void test_jobs_populated_jobs_array_returns_200(void) {
    struct MockMHDConnection mock_conn = {0};

    // Set up valid JWT auth
    mock_mhd_add_lookup("Authorization", "Bearer valid.token.here");
    setup_mock_jwt_valid();

    // Set up scoreboard with snapshot containing jobs array
    json_t* snapshot = json_object();
    TEST_ASSERT_NOT_NULL(snapshot);
    json_t* jobs = json_array();
    TEST_ASSERT_NOT_NULL(jobs);

    json_t* job1 = json_object();
    TEST_ASSERT_NOT_NULL(job1);
    json_object_set_new(job1, "job_id", json_string("job-001"));
    json_object_set_new(job1, "status", json_string("completed"));
    json_array_append_new(jobs, job1);

    json_t* job2 = json_object();
    TEST_ASSERT_NOT_NULL(job2);
    json_object_set_new(job2, "job_id", json_string("job-002"));
    json_object_set_new(job2, "status", json_string("running"));
    json_array_append_new(jobs, job2);

    json_object_set_new(snapshot, "jobs", jobs);
    mock_scoreboard_json_set_snapshot_result(snapshot);

    enum MHD_Result result = handle_system_jobs_request((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(1, mock_api_utils_get_send_json_response_call_count());
    TEST_ASSERT_EQUAL(MHD_HTTP_OK, mock_api_utils_get_captured_status());

    json_t* captured = mock_api_utils_get_captured_response();
    TEST_ASSERT_NOT_NULL(captured);
    TEST_ASSERT_TRUE(json_is_array(captured));
    TEST_ASSERT_EQUAL(2, json_array_size(captured));

    json_t* first_job = json_array_get(captured, 0);
    TEST_ASSERT_NOT_NULL(first_job);
    json_t* job_id = json_object_get(first_job, "job_id");
    TEST_ASSERT_NOT_NULL(job_id);
    TEST_ASSERT_EQUAL_STRING("job-001", json_string_value(job_id));

    json_t* second_job = json_array_get(captured, 1);
    TEST_ASSERT_NOT_NULL(second_job);
    json_t* status = json_object_get(second_job, "status");
    TEST_ASSERT_NOT_NULL(status);
    TEST_ASSERT_EQUAL_STRING("running", json_string_value(status));

    json_decref(captured);
    mock_api_utils_reset_capture();
}

// Test 8: Auth header with Bearer prefix but invalid token → 401
void test_jobs_auth_header_with_bearer_prefix_returns_401_on_invalid_token(void) {
    struct MockMHDConnection mock_conn = {0};

    // Set up Authorization header with Bearer prefix
    mock_mhd_add_lookup("Authorization", "Bearer some.invalid.token");
    setup_mock_jwt_invalid();

    enum MHD_Result result = handle_system_jobs_request((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_UNAUTHORIZED, mock_api_utils_get_captured_status());

    json_t* captured = mock_api_utils_get_captured_response();
    TEST_ASSERT_NOT_NULL(captured);
    TEST_ASSERT_TRUE(json_is_object(captured));

    // Verify error response structure
    json_t* success = json_object_get(captured, "success");
    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_FALSE(json_is_true(success));

    json_t* error = json_object_get(captured, "error");
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL_STRING("Authentication required", json_string_value(error));

    json_decref(captured);
    mock_api_utils_reset_capture();
}

// Test 9: Verify function signature compiles correctly
void test_jobs_function_signature_compilation_check(void) {
    // This test verifies the function signature is as expected
    // The function should take a struct MHD_Connection pointer and return enum MHD_Result
    // Compilation success means the signature is correct
    TEST_ASSERT_TRUE(true);
}

// Test 10: Response capture — verify full response structure for auth failure
void test_jobs_response_capture_and_inspection(void) {
    struct MockMHDConnection mock_conn = {0};

    // No auth header → 401
    enum MHD_Result result = handle_system_jobs_request((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(1, mock_api_utils_get_send_json_response_call_count());

    unsigned int status = mock_api_utils_get_captured_status();
    TEST_ASSERT_EQUAL(MHD_HTTP_UNAUTHORIZED, status);

    json_t* captured = mock_api_utils_get_captured_response();
    TEST_ASSERT_NOT_NULL(captured);
    TEST_ASSERT_TRUE(json_is_object(captured));

    // Verify the response has both required fields
    TEST_ASSERT_NOT_NULL(json_object_get(captured, "success"));
    TEST_ASSERT_NOT_NULL(json_object_get(captured, "error"));

    json_decref(captured);
    mock_api_utils_reset_capture();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_jobs_no_auth_header_returns_401);
    RUN_TEST(test_jobs_invalid_auth_format_returns_401);
    RUN_TEST(test_jobs_jwt_validation_failure_returns_401);
    RUN_TEST(test_jobs_snapshot_null_returns_500);
    RUN_TEST(test_jobs_snapshot_missing_jobs_returns_500);
    RUN_TEST(test_jobs_empty_jobs_array_returns_200);
    RUN_TEST(test_jobs_populated_jobs_array_returns_200);
    RUN_TEST(test_jobs_auth_header_with_bearer_prefix_returns_401_on_invalid_token);
    RUN_TEST(test_jobs_function_signature_compilation_check);
    RUN_TEST(test_jobs_response_capture_and_inspection);

    return UNITY_END();
}
