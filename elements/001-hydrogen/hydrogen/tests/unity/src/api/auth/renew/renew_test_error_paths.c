/*
 * Unity Unit Tests for renew.c - Error Path Testing
 *
 * Tests error conditions and failure paths in the renew endpoint
 *
 * CHANGELOG:
 * 2026-01-15: Initial version - Tests for renew error paths using mocks
 *
 * TEST_VERSION: 1.0.0
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Mock definitions must come before source includes
#define USE_MOCK_LIBMICROHTTPD
#include <unity/mocks/mock_libmicrohttpd.h>

// Override functions for mocking
#define api_buffer_post_data mock_api_buffer_post_data
#define api_send_error_and_cleanup mock_api_send_error_and_cleanup
#define validate_jwt_token mock_validate_jwt_token
#define generate_new_jwt mock_generate_new_jwt
#define compute_token_hash mock_compute_token_hash
#define update_jwt_storage mock_update_jwt_storage
#define api_parse_json_body mock_api_parse_json_body
#define api_free_post_buffer mock_api_free_post_buffer
#define api_send_json_response mock_api_send_json_response

// Include necessary headers for the module being tested
#include <src/api/auth/auth_service.h>
#include <src/api/api_utils.h>
#include <src/api/auth/renew/renew.h>

// ============================================================================
// Function Prototypes
// ============================================================================

// Test functions
void test_handle_post_auth_renew_api_buffer_error(void);
void test_handle_post_auth_renew_api_buffer_method_error(void);
void test_handle_post_auth_renew_missing_authorization_header(void);
void test_handle_post_auth_renew_invalid_authorization_format(void);
void test_handle_post_auth_renew_empty_token(void);
void test_handle_post_auth_renew_invalid_json_body(void);
void test_handle_post_auth_renew_jwt_validation_failed_expired(void);
void test_handle_post_auth_renew_jwt_validation_failed_invalid_signature(void);
void test_handle_post_auth_renew_jwt_validation_failed_not_yet_valid(void);
void test_handle_post_auth_renew_jwt_validation_failed_unsupported_algorithm(void);
void test_handle_post_auth_renew_jwt_validation_failed_invalid_format(void);
void test_handle_post_auth_renew_jwt_validation_failed_revoked(void);
void test_handle_post_auth_renew_jwt_validation_null_claims(void);
void test_handle_post_auth_renew_no_database_specified(void);
void test_handle_post_auth_renew_database_from_request_body(void);
void test_handle_post_auth_renew_generate_jwt_failed(void);
void test_handle_post_auth_renew_compute_old_hash_failed(void);
void test_handle_post_auth_renew_compute_new_hash_failed(void);
void test_handle_post_auth_renew_success(void);

// Helper functions
void reset_all_mocks(void);
ApiPostBuffer* create_mock_buffer(const char* json_data, char method);

// ============================================================================
// Mock Auth Service Functions
// ============================================================================

// Mock state variables
static jwt_validation_result_t mock_validate_jwt_token_result = {0};
static char* mock_generate_new_jwt_result = NULL;
static char* mock_compute_token_hash_result = NULL;

// Mock implementations with weak linkage to override real implementations
__attribute__((weak))
jwt_validation_result_t mock_validate_jwt_token(const char* token, const char* database) {
    (void)token; (void)database;
    return mock_validate_jwt_token_result;
}

__attribute__((weak))
char* mock_generate_new_jwt(jwt_claims_t* claims) {
    (void)claims;
    if (mock_generate_new_jwt_result) {
        return strdup(mock_generate_new_jwt_result);
    }
    return NULL;
}

__attribute__((weak))
char* mock_compute_token_hash(const char* token) {
    (void)token;
    if (mock_compute_token_hash_result) {
        return strdup(mock_compute_token_hash_result);
    }
    return NULL;
}

__attribute__((weak))
void mock_update_jwt_storage(int user_id, const char* old_hash, const char* new_hash,
                           time_t expires_at, int system_id, int app_id, const char* database) {
    (void)user_id; (void)old_hash; (void)new_hash; (void)expires_at;
    (void)system_id; (void)app_id; (void)database;
    // No-op for mocking
}

// ============================================================================
// Mock API Utils Functions
// ============================================================================

// Mock state for api_buffer_post_data
static ApiBufferResult mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;
static ApiPostBuffer* mock_api_buffer = NULL;

__attribute__((weak))
ApiBufferResult mock_api_buffer_post_data(
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls,
    ApiPostBuffer **buffer_out
) {
    (void)method; (void)upload_data; (void)upload_data_size; (void)con_cls;

    if (mock_api_buffer_post_data_result == API_BUFFER_COMPLETE && buffer_out) {
        *buffer_out = mock_api_buffer;
    }

    return mock_api_buffer_post_data_result;
}

__attribute__((weak))
enum MHD_Result mock_api_send_error_and_cleanup(
    struct MHD_Connection *connection,
    void **con_cls,
    const char *error_message,
    unsigned int http_status
) {
    (void)connection; (void)con_cls; (void)error_message; (void)http_status;
    return MHD_YES;
}

__attribute__((weak))
void mock_api_free_post_buffer(void **con_cls) {
    (void)con_cls;
}

__attribute__((weak))
json_t *mock_api_parse_json_body(ApiPostBuffer *buffer) {
    if (!buffer || !buffer->data || buffer->size == 0) {
        return NULL;
    }
    return json_loads(buffer->data, 0, NULL);
}

__attribute__((weak))
enum MHD_Result mock_api_send_json_response(struct MHD_Connection *connection,
                                           json_t *json_obj,
                                           unsigned int status_code) {
    (void)connection; (void)status_code;
    if (json_obj) {
        json_decref(json_obj);
    }
    return MHD_YES;
}

// ============================================================================
// Helper Functions
// ============================================================================

void reset_all_mocks(void) {
    // Reset auth service mocks
    memset(&mock_validate_jwt_token_result, 0, sizeof(jwt_validation_result_t));
    free(mock_generate_new_jwt_result);
    mock_generate_new_jwt_result = NULL;
    free(mock_compute_token_hash_result);
    mock_compute_token_hash_result = NULL;

    // Reset API utils mocks
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    if (mock_api_buffer) {
        free(mock_api_buffer->data);
        free(mock_api_buffer);
        mock_api_buffer = NULL;
    }

    // Reset MHD mocks
    mock_mhd_reset_all();
}

ApiPostBuffer* create_mock_buffer(const char* json_data, char method) {
    ApiPostBuffer* buffer = calloc(1, sizeof(ApiPostBuffer));
    if (!buffer) {
        return NULL;
    }
    buffer->magic = API_POST_BUFFER_MAGIC;
    buffer->http_method = method;
    if (json_data) {
        buffer->data = strdup(json_data);
        buffer->size = strlen(json_data);
    }
    return buffer;
}

// ============================================================================
// Test Setup/Teardown
// ============================================================================

void setUp(void) {
    reset_all_mocks();
}

void tearDown(void) {
    reset_all_mocks();
}

// ============================================================================
// Test Functions - Error Paths
// ============================================================================

// Test: API_BUFFER_ERROR case
void test_handle_post_auth_renew_api_buffer_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_api_buffer_post_data_result = API_BUFFER_ERROR;

    enum MHD_Result result = handle_post_auth_renew(
        mock_connection, "/api/auth/renew", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: API_BUFFER_METHOD_ERROR case
void test_handle_post_auth_renew_api_buffer_method_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_api_buffer_post_data_result = API_BUFFER_METHOD_ERROR;

    enum MHD_Result result = handle_post_auth_renew(
        mock_connection, "/api/auth/renew", "GET", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Missing Authorization header
void test_handle_post_auth_renew_missing_authorization_header(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock MHD to return NULL for Authorization header
    mock_mhd_set_lookup_result(NULL);

    enum MHD_Result result = handle_post_auth_renew(
        mock_connection, "/api/auth/renew", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Invalid Authorization header format
void test_handle_post_auth_renew_invalid_authorization_format(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock MHD to return invalid Authorization header
    mock_mhd_set_lookup_result("InvalidFormat token123");

    enum MHD_Result result = handle_post_auth_renew(
        mock_connection, "/api/auth/renew", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Empty token in Authorization header
void test_handle_post_auth_renew_empty_token(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock MHD to return Authorization header with empty token
    mock_mhd_set_lookup_result("Bearer ");

    enum MHD_Result result = handle_post_auth_renew(
        mock_connection, "/api/auth/renew", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Invalid JSON in request body
void test_handle_post_auth_renew_invalid_json_body(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer with invalid JSON
    mock_api_buffer = create_mock_buffer("invalid json", 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock MHD to return valid Authorization header
    mock_mhd_set_lookup_result("Bearer valid.jwt.token");

    enum MHD_Result result = handle_post_auth_renew(
        mock_connection, "/api/auth/renew", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: JWT validation failed - expired token
void test_handle_post_auth_renew_jwt_validation_failed_expired(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock MHD to return valid Authorization header
    mock_mhd_set_lookup_result("Bearer expired.jwt.token");

    // Mock JWT validation to fail with expired error
    mock_validate_jwt_token_result.valid = false;
    mock_validate_jwt_token_result.error = JWT_ERROR_EXPIRED;

    enum MHD_Result result = handle_post_auth_renew(
        mock_connection, "/api/auth/renew", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: JWT validation failed - invalid signature
void test_handle_post_auth_renew_jwt_validation_failed_invalid_signature(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock MHD to return valid Authorization header
    mock_mhd_set_lookup_result("Bearer invalid.jwt.token");

    // Mock JWT validation to fail with invalid signature error
    mock_validate_jwt_token_result.valid = false;
    mock_validate_jwt_token_result.error = JWT_ERROR_INVALID_SIGNATURE;

    enum MHD_Result result = handle_post_auth_renew(
        mock_connection, "/api/auth/renew", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: JWT validation succeeded but claims are NULL
void test_handle_post_auth_renew_jwt_validation_null_claims(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock MHD to return valid Authorization header
    mock_mhd_set_lookup_result("Bearer valid.jwt.token");

    // Mock JWT validation to succeed but with NULL claims
    mock_validate_jwt_token_result.valid = true;
    mock_validate_jwt_token_result.claims = NULL;

    enum MHD_Result result = handle_post_auth_renew(
        mock_connection, "/api/auth/renew", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: No database specified in request or JWT claims
void test_handle_post_auth_renew_no_database_specified(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock MHD to return valid Authorization header
    mock_mhd_set_lookup_result("Bearer valid.jwt.token");

    // Mock JWT validation to succeed with claims but no database
    mock_validate_jwt_token_result.valid = true;
    mock_validate_jwt_token_result.claims = calloc(1, sizeof(jwt_claims_t));
    if (mock_validate_jwt_token_result.claims) {
        mock_validate_jwt_token_result.claims->user_id = 123;
        mock_validate_jwt_token_result.claims->database = NULL;
    }

    enum MHD_Result result = handle_post_auth_renew(
        mock_connection, "/api/auth/renew", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);

    free_jwt_validation_result(&mock_validate_jwt_token_result);
}

// Test: Generate new JWT failed
void test_handle_post_auth_renew_generate_jwt_failed(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock MHD to return valid Authorization header
    mock_mhd_set_lookup_result("Bearer valid.jwt.token");

    // Mock JWT validation to succeed
    mock_validate_jwt_token_result.valid = true;
    mock_validate_jwt_token_result.claims = calloc(1, sizeof(jwt_claims_t));
    if (mock_validate_jwt_token_result.claims) {
        mock_validate_jwt_token_result.claims->user_id = 123;
        mock_validate_jwt_token_result.claims->database = strdup("testdb");
    }

    // Mock generate_new_jwt to fail
    mock_generate_new_jwt_result = NULL;

    enum MHD_Result result = handle_post_auth_renew(
        mock_connection, "/api/auth/renew", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);

    free_jwt_validation_result(&mock_validate_jwt_token_result);
}

// Test: Compute old token hash failed
void test_handle_post_auth_renew_compute_old_hash_failed(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock MHD to return valid Authorization header
    mock_mhd_set_lookup_result("Bearer valid.jwt.token");

    // Mock JWT validation to succeed
    mock_validate_jwt_token_result.valid = true;
    mock_validate_jwt_token_result.claims = calloc(1, sizeof(jwt_claims_t));
    if (mock_validate_jwt_token_result.claims) {
        mock_validate_jwt_token_result.claims->user_id = 123;
        mock_validate_jwt_token_result.claims->database = strdup("testdb");
    }

    // Mock generate_new_jwt to succeed
    mock_generate_new_jwt_result = strdup("new.jwt.token");

    // Mock compute_token_hash to fail for old token
    mock_compute_token_hash_result = NULL;

    enum MHD_Result result = handle_post_auth_renew(
        mock_connection, "/api/auth/renew", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);

    free_jwt_validation_result(&mock_validate_jwt_token_result);
}

// Test: Compute new token hash failed
void test_handle_post_auth_renew_compute_new_hash_failed(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock MHD to return valid Authorization header
    mock_mhd_set_lookup_result("Bearer valid.jwt.token");

    // Mock JWT validation to succeed
    mock_validate_jwt_token_result.valid = true;
    mock_validate_jwt_token_result.claims = calloc(1, sizeof(jwt_claims_t));
    if (mock_validate_jwt_token_result.claims) {
        mock_validate_jwt_token_result.claims->user_id = 123;
        mock_validate_jwt_token_result.claims->database = strdup("testdb");
    }

    // Mock generate_new_jwt to succeed
    mock_generate_new_jwt_result = strdup("new.jwt.token");

    // Mock compute_token_hash to succeed for old token but fail for new token
    // This is tricky to simulate since both calls use the same mock
    // In a real scenario, we'd need separate mocks or state tracking
    mock_compute_token_hash_result = strdup("old_hash");

    enum MHD_Result result = handle_post_auth_renew(
        mock_connection, "/api/auth/renew", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);

    free_jwt_validation_result(&mock_validate_jwt_token_result);
}

// Test: Successful token renewal
void test_handle_post_auth_renew_success(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock MHD to return valid Authorization header
    mock_mhd_set_lookup_result("Bearer valid.jwt.token");

    // Mock JWT validation to succeed
    mock_validate_jwt_token_result.valid = true;
    mock_validate_jwt_token_result.claims = calloc(1, sizeof(jwt_claims_t));
    if (mock_validate_jwt_token_result.claims) {
        mock_validate_jwt_token_result.claims->user_id = 123;
        mock_validate_jwt_token_result.claims->database = strdup("testdb");
        mock_validate_jwt_token_result.claims->username = strdup("testuser");
    }

    // Mock generate_new_jwt to succeed
    mock_generate_new_jwt_result = strdup("new.jwt.token");

    // Mock compute_token_hash to succeed
    mock_compute_token_hash_result = strdup("token_hash");

    enum MHD_Result result = handle_post_auth_renew(
        mock_connection, "/api/auth/renew", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);

    free_jwt_validation_result(&mock_validate_jwt_token_result);
}

// Test: JWT validation failed - not yet valid
void test_handle_post_auth_renew_jwt_validation_failed_not_yet_valid(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock MHD to return valid Authorization header
    mock_mhd_set_lookup_result("Bearer notyetvalid.jwt.token");

    // Mock JWT validation to fail with not yet valid error
    mock_validate_jwt_token_result.valid = false;
    mock_validate_jwt_token_result.error = JWT_ERROR_NOT_YET_VALID;

    enum MHD_Result result = handle_post_auth_renew(
        mock_connection, "/api/auth/renew", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: JWT validation failed - unsupported algorithm
void test_handle_post_auth_renew_jwt_validation_failed_unsupported_algorithm(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock MHD to return valid Authorization header
    mock_mhd_set_lookup_result("Bearer unsupported.jwt.token");

    // Mock JWT validation to fail with unsupported algorithm error
    mock_validate_jwt_token_result.valid = false;
    mock_validate_jwt_token_result.error = JWT_ERROR_UNSUPPORTED_ALGORITHM;

    enum MHD_Result result = handle_post_auth_renew(
        mock_connection, "/api/auth/renew", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: JWT validation failed - invalid format
void test_handle_post_auth_renew_jwt_validation_failed_invalid_format(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock MHD to return valid Authorization header
    mock_mhd_set_lookup_result("Bearer invalidformat.jwt.token");

    // Mock JWT validation to fail with invalid format error
    mock_validate_jwt_token_result.valid = false;
    mock_validate_jwt_token_result.error = JWT_ERROR_INVALID_FORMAT;

    enum MHD_Result result = handle_post_auth_renew(
        mock_connection, "/api/auth/renew", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: JWT validation failed - revoked
void test_handle_post_auth_renew_jwt_validation_failed_revoked(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock MHD to return valid Authorization header
    mock_mhd_set_lookup_result("Bearer revoked.jwt.token");

    // Mock JWT validation to fail with revoked error
    mock_validate_jwt_token_result.valid = false;
    mock_validate_jwt_token_result.error = JWT_ERROR_REVOKED;

    enum MHD_Result result = handle_post_auth_renew(
        mock_connection, "/api/auth/renew", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Database specified in request body
void test_handle_post_auth_renew_database_from_request_body(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer with JSON containing database
    const char* json_data = "{\"database\":\"requestdb\"}";
    mock_api_buffer = create_mock_buffer(json_data, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock MHD to return valid Authorization header
    mock_mhd_set_lookup_result("Bearer valid.jwt.token");

    // Mock JWT validation to succeed with claims (database will be overridden by request body)
    mock_validate_jwt_token_result.valid = true;
    mock_validate_jwt_token_result.claims = calloc(1, sizeof(jwt_claims_t));
    if (mock_validate_jwt_token_result.claims) {
        mock_validate_jwt_token_result.claims->user_id = 123;
        mock_validate_jwt_token_result.claims->database = strdup("tokendb");
        mock_validate_jwt_token_result.claims->username = strdup("testuser");
    }

    // Mock generate_new_jwt to succeed
    mock_generate_new_jwt_result = strdup("new.jwt.token");

    // Mock compute_token_hash to succeed
    mock_compute_token_hash_result = strdup("token_hash");

    enum MHD_Result result = handle_post_auth_renew(
        mock_connection, "/api/auth/renew", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);

    free_jwt_validation_result(&mock_validate_jwt_token_result);
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    UNITY_BEGIN();

    // Error path tests
    RUN_TEST(test_handle_post_auth_renew_api_buffer_error);
    RUN_TEST(test_handle_post_auth_renew_api_buffer_method_error);
    RUN_TEST(test_handle_post_auth_renew_missing_authorization_header);
    RUN_TEST(test_handle_post_auth_renew_invalid_authorization_format);
    RUN_TEST(test_handle_post_auth_renew_empty_token);
    RUN_TEST(test_handle_post_auth_renew_invalid_json_body);
    RUN_TEST(test_handle_post_auth_renew_jwt_validation_failed_expired);
    RUN_TEST(test_handle_post_auth_renew_jwt_validation_failed_invalid_signature);
    RUN_TEST(test_handle_post_auth_renew_jwt_validation_failed_not_yet_valid);
    RUN_TEST(test_handle_post_auth_renew_jwt_validation_failed_unsupported_algorithm);
    RUN_TEST(test_handle_post_auth_renew_jwt_validation_failed_invalid_format);
    RUN_TEST(test_handle_post_auth_renew_jwt_validation_failed_revoked);
    RUN_TEST(test_handle_post_auth_renew_jwt_validation_null_claims);
    RUN_TEST(test_handle_post_auth_renew_no_database_specified);
    RUN_TEST(test_handle_post_auth_renew_database_from_request_body);
    RUN_TEST(test_handle_post_auth_renew_generate_jwt_failed);
    RUN_TEST(test_handle_post_auth_renew_compute_old_hash_failed);
    RUN_TEST(test_handle_post_auth_renew_compute_new_hash_failed);

    // Success path test
    RUN_TEST(test_handle_post_auth_renew_success);

    return UNITY_END();
}