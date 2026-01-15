/*
 * Unity Unit Tests for logout.c - Error Path Testing
 *
 * Tests error conditions and failure paths in the logout endpoint
 *
 * CHANGELOG:
 * 2026-01-15: Initial version - Tests for logout error paths using mocks
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
#define validate_jwt_for_logout mock_validate_jwt_for_logout
#define compute_token_hash mock_compute_token_hash
#define delete_jwt_from_storage mock_delete_jwt_from_storage
#define api_parse_json_body mock_api_parse_json_body
#define api_free_post_buffer mock_api_free_post_buffer
#define api_send_json_response mock_api_send_json_response

// Extracted functions are declared as weak in source file, so they can be called directly

// Include necessary headers for the module being tested
#include <src/api/auth/auth_service.h>
#include <src/api/api_utils.h>
#include <src/api/auth/logout/logout.h>

// ============================================================================
// Function Prototypes
// ============================================================================

// Test functions
void test_handle_post_auth_logout_api_buffer_error(void);
void test_handle_post_auth_logout_api_buffer_method_error(void);
void test_handle_post_auth_logout_missing_authorization_header(void);
void test_handle_post_auth_logout_invalid_authorization_header_format(void);
void test_handle_post_auth_logout_empty_token(void);
void test_handle_post_auth_logout_jwt_validation_expired(void);
void test_handle_post_auth_logout_jwt_validation_not_yet_valid(void);
void test_handle_post_auth_logout_jwt_validation_invalid_signature(void);
void test_handle_post_auth_logout_jwt_validation_unsupported_algorithm(void);
void test_handle_post_auth_logout_jwt_validation_invalid_format(void);
void test_handle_post_auth_logout_jwt_validation_revoked(void);
void test_handle_post_auth_logout_jwt_validation_unknown_error(void);
void test_handle_post_auth_logout_jwt_validation_claims_null(void);
void test_handle_post_auth_logout_no_database_specified(void);
void test_handle_post_auth_logout_compute_hash_failure(void);
void test_handle_post_auth_logout_success(void);
void test_handle_post_auth_logout_with_database_in_request_body(void);
void test_handle_post_auth_logout_with_invalid_json_in_request_body(void);

// Helper functions
void reset_all_mocks(void);
ApiPostBuffer* create_mock_buffer(const char* json_data, char method);

// ============================================================================
// Mock Auth Service Functions
// ============================================================================

// Mock state variables
static jwt_validation_result_t mock_validate_jwt_for_logout_result = {0};
static char* mock_compute_token_hash_result = NULL;

// Mock implementations with weak linkage to override real implementations
__attribute__((weak))
jwt_validation_result_t mock_validate_jwt_for_logout(const char* token, const char* database) {
    (void)token; (void)database;
    return mock_validate_jwt_for_logout_result;
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
void mock_delete_jwt_from_storage(const char* jwt_hash, const char* database) {
    (void)jwt_hash; (void)database;
    // Mock implementation - do nothing
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
    memset(&mock_validate_jwt_for_logout_result, 0, sizeof(jwt_validation_result_t));
    mock_validate_jwt_for_logout_result.valid = true; // Default to valid
    mock_validate_jwt_for_logout_result.error = JWT_ERROR_NONE;

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

// Test: API_BUFFER_ERROR case (lines 56-58)
void test_handle_post_auth_logout_api_buffer_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_api_buffer_post_data_result = API_BUFFER_ERROR;

    enum MHD_Result result = handle_post_auth_logout(
        mock_connection, "/api/auth/logout", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: API_BUFFER_METHOD_ERROR case (lines 61-63)
void test_handle_post_auth_logout_api_buffer_method_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_api_buffer_post_data_result = API_BUFFER_METHOD_ERROR;

    enum MHD_Result result = handle_post_auth_logout(
        mock_connection, "/api/auth/logout", "PUT", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Missing Authorization header (lines 76-80)
void test_handle_post_auth_logout_missing_authorization_header(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer to complete so we get to authorization check
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock MHD_lookup_connection_value to return NULL (no Authorization header)
    mock_mhd_set_lookup_result(NULL);

    enum MHD_Result result = handle_post_auth_logout(
        mock_connection, "/api/auth/logout", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Invalid Authorization header format (lines 87-91)
void test_handle_post_auth_logout_invalid_authorization_header_format(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer to complete so we get to authorization check
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock MHD_lookup_connection_value to return invalid format (missing "Bearer ")
    mock_mhd_set_lookup_result("InvalidFormat token123");

    enum MHD_Result result = handle_post_auth_logout(
        mock_connection, "/api/auth/logout", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Empty token in Authorization header (lines 98-102)
void test_handle_post_auth_logout_empty_token(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer to complete so we get to authorization check
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock MHD_lookup_connection_value to return "Bearer " with no token
    mock_mhd_set_lookup_result("Bearer ");

    enum MHD_Result result = handle_post_auth_logout(
        mock_connection, "/api/auth/logout", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: JWT validation expired (lines 133-135)
void test_handle_post_auth_logout_jwt_validation_expired(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer to complete so we get to JWT validation
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock Authorization header
    mock_mhd_set_lookup_result("Bearer valid.jwt.token");

    // Mock JWT validation to return expired error
    mock_validate_jwt_for_logout_result.valid = false;
    mock_validate_jwt_for_logout_result.error = JWT_ERROR_EXPIRED;

    enum MHD_Result result = handle_post_auth_logout(
        mock_connection, "/api/auth/logout", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: JWT validation not yet valid (lines 136-137)
void test_handle_post_auth_logout_jwt_validation_not_yet_valid(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer to complete so we get to JWT validation
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock Authorization header
    mock_mhd_set_lookup_result("Bearer valid.jwt.token");

    // Mock JWT validation to return not yet valid error
    mock_validate_jwt_for_logout_result.valid = false;
    mock_validate_jwt_for_logout_result.error = JWT_ERROR_NOT_YET_VALID;

    enum MHD_Result result = handle_post_auth_logout(
        mock_connection, "/api/auth/logout", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: JWT validation invalid signature (lines 139-140)
void test_handle_post_auth_logout_jwt_validation_invalid_signature(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer to complete so we get to JWT validation
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock Authorization header
    mock_mhd_set_lookup_result("Bearer valid.jwt.token");

    // Mock JWT validation to return invalid signature error
    mock_validate_jwt_for_logout_result.valid = false;
    mock_validate_jwt_for_logout_result.error = JWT_ERROR_INVALID_SIGNATURE;

    enum MHD_Result result = handle_post_auth_logout(
        mock_connection, "/api/auth/logout", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: JWT validation unsupported algorithm (lines 142-143)
void test_handle_post_auth_logout_jwt_validation_unsupported_algorithm(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer to complete so we get to JWT validation
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock Authorization header
    mock_mhd_set_lookup_result("Bearer valid.jwt.token");

    // Mock JWT validation to return unsupported algorithm error
    mock_validate_jwt_for_logout_result.valid = false;
    mock_validate_jwt_for_logout_result.error = JWT_ERROR_UNSUPPORTED_ALGORITHM;

    enum MHD_Result result = handle_post_auth_logout(
        mock_connection, "/api/auth/logout", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: JWT validation invalid format (lines 145-146)
void test_handle_post_auth_logout_jwt_validation_invalid_format(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer to complete so we get to JWT validation
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock Authorization header
    mock_mhd_set_lookup_result("Bearer valid.jwt.token");

    // Mock JWT validation to return invalid format error
    mock_validate_jwt_for_logout_result.valid = false;
    mock_validate_jwt_for_logout_result.error = JWT_ERROR_INVALID_FORMAT;

    enum MHD_Result result = handle_post_auth_logout(
        mock_connection, "/api/auth/logout", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: JWT validation revoked (lines 148-149)
void test_handle_post_auth_logout_jwt_validation_revoked(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer to complete so we get to JWT validation
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock Authorization header
    mock_mhd_set_lookup_result("Bearer valid.jwt.token");

    // Mock JWT validation to return revoked error
    mock_validate_jwt_for_logout_result.valid = false;
    mock_validate_jwt_for_logout_result.error = JWT_ERROR_REVOKED;

    enum MHD_Result result = handle_post_auth_logout(
        mock_connection, "/api/auth/logout", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: JWT validation succeeded but claims NULL (lines 163-168)
void test_handle_post_auth_logout_jwt_validation_claims_null(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer to complete so we get to JWT validation
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock Authorization header
    mock_mhd_set_lookup_result("Bearer valid.jwt.token");

    // Mock JWT validation to succeed but with NULL claims
    mock_validate_jwt_for_logout_result.valid = true;
    mock_validate_jwt_for_logout_result.error = JWT_ERROR_NONE;
    mock_validate_jwt_for_logout_result.claims = NULL; // Claims are NULL

    enum MHD_Result result = handle_post_auth_logout(
        mock_connection, "/api/auth/logout", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: No database specified (lines 178-184)
void test_handle_post_auth_logout_no_database_specified(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer to complete so we get to database check
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock Authorization header
    mock_mhd_set_lookup_result("Bearer valid.jwt.token");

    // Mock JWT validation to succeed with claims but no database
    mock_validate_jwt_for_logout_result.valid = true;
    mock_validate_jwt_for_logout_result.error = JWT_ERROR_NONE;
    mock_validate_jwt_for_logout_result.claims = calloc(1, sizeof(jwt_claims_t));
    if (mock_validate_jwt_for_logout_result.claims) {
        mock_validate_jwt_for_logout_result.claims->user_id = 123;
        mock_validate_jwt_for_logout_result.claims->username = strdup("testuser");
        mock_validate_jwt_for_logout_result.claims->database = NULL; // No database in claims
    }

    enum MHD_Result result = handle_post_auth_logout(
        mock_connection, "/api/auth/logout", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);

    // Cleanup
    if (mock_validate_jwt_for_logout_result.claims) {
        free(mock_validate_jwt_for_logout_result.claims->username);
        free(mock_validate_jwt_for_logout_result.claims);
        mock_validate_jwt_for_logout_result.claims = NULL;
    }
}

// Test: Failed to compute JWT hash (lines 194-201)
void test_handle_post_auth_logout_compute_hash_failure(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer to complete so we get to hash computation
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock Authorization header
    mock_mhd_set_lookup_result("Bearer valid.jwt.token");

    // Mock JWT validation to succeed with valid claims and database
    mock_validate_jwt_for_logout_result.valid = true;
    mock_validate_jwt_for_logout_result.error = JWT_ERROR_NONE;
    mock_validate_jwt_for_logout_result.claims = calloc(1, sizeof(jwt_claims_t));
    if (mock_validate_jwt_for_logout_result.claims) {
        mock_validate_jwt_for_logout_result.claims->user_id = 123;
        mock_validate_jwt_for_logout_result.claims->username = strdup("testuser");
        mock_validate_jwt_for_logout_result.claims->database = strdup("testdb");
    }

    // Mock compute_token_hash to return NULL (failure)
    mock_compute_token_hash_result = NULL;

    enum MHD_Result result = handle_post_auth_logout(
        mock_connection, "/api/auth/logout", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);

    // Cleanup
    if (mock_validate_jwt_for_logout_result.claims) {
        free(mock_validate_jwt_for_logout_result.claims->username);
        free(mock_validate_jwt_for_logout_result.claims->database);
        free(mock_validate_jwt_for_logout_result.claims);
        mock_validate_jwt_for_logout_result.claims = NULL;
    }
}

// Test: Successful logout (covers success path and logging)
void test_handle_post_auth_logout_success(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer to complete
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock Authorization header
    mock_mhd_set_lookup_result("Bearer valid.jwt.token");

    // Mock JWT validation to succeed with valid claims and database
    mock_validate_jwt_for_logout_result.valid = true;
    mock_validate_jwt_for_logout_result.error = JWT_ERROR_NONE;
    mock_validate_jwt_for_logout_result.claims = calloc(1, sizeof(jwt_claims_t));
    if (mock_validate_jwt_for_logout_result.claims) {
        mock_validate_jwt_for_logout_result.claims->user_id = 123;
        mock_validate_jwt_for_logout_result.claims->username = strdup("testuser");
        mock_validate_jwt_for_logout_result.claims->database = strdup("testdb");
    }

    // Mock compute_token_hash to succeed
    mock_compute_token_hash_result = strdup("mocked_hash_value");

    enum MHD_Result result = handle_post_auth_logout(
        mock_connection, "/api/auth/logout", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);

    // Cleanup
    if (mock_validate_jwt_for_logout_result.claims) {
        free(mock_validate_jwt_for_logout_result.claims->username);
        free(mock_validate_jwt_for_logout_result.claims->database);
        free(mock_validate_jwt_for_logout_result.claims);
        mock_validate_jwt_for_logout_result.claims = NULL;
    }
}

// Test: Logout with database specified in request body
void test_handle_post_auth_logout_with_database_in_request_body(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer with JSON containing database
    const char* json_data = "{\"database\":\"requestdb\"}";
    mock_api_buffer = create_mock_buffer(json_data, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock Authorization header
    mock_mhd_set_lookup_result("Bearer valid.jwt.token");

    // Mock JWT validation to succeed with claims (database will be overridden by request body)
    mock_validate_jwt_for_logout_result.valid = true;
    mock_validate_jwt_for_logout_result.error = JWT_ERROR_NONE;
    mock_validate_jwt_for_logout_result.claims = calloc(1, sizeof(jwt_claims_t));
    if (mock_validate_jwt_for_logout_result.claims) {
        mock_validate_jwt_for_logout_result.claims->user_id = 123;
        mock_validate_jwt_for_logout_result.claims->username = strdup("testuser");
        mock_validate_jwt_for_logout_result.claims->database = strdup("tokendb"); // This should be overridden
    }

    // Mock compute_token_hash to succeed
    mock_compute_token_hash_result = strdup("mocked_hash_value");

    enum MHD_Result result = handle_post_auth_logout(
        mock_connection, "/api/auth/logout", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);

    // Cleanup
    if (mock_validate_jwt_for_logout_result.claims) {
        free(mock_validate_jwt_for_logout_result.claims->username);
        free(mock_validate_jwt_for_logout_result.claims->database);
        free(mock_validate_jwt_for_logout_result.claims);
        mock_validate_jwt_for_logout_result.claims = NULL;
    }
}

// Test: Logout with invalid JSON in request body
void test_handle_post_auth_logout_with_invalid_json_in_request_body(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer with invalid JSON
    const char* json_data = "invalid json";
    mock_api_buffer = create_mock_buffer(json_data, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock Authorization header
    mock_mhd_set_lookup_result("Bearer valid.jwt.token");

    // Mock JWT validation to succeed with claims (database from claims since JSON parsing fails)
    mock_validate_jwt_for_logout_result.valid = true;
    mock_validate_jwt_for_logout_result.error = JWT_ERROR_NONE;
    mock_validate_jwt_for_logout_result.claims = calloc(1, sizeof(jwt_claims_t));
    if (mock_validate_jwt_for_logout_result.claims) {
        mock_validate_jwt_for_logout_result.claims->user_id = 123;
        mock_validate_jwt_for_logout_result.claims->username = strdup("testuser");
        mock_validate_jwt_for_logout_result.claims->database = strdup("tokendb");
    }

    // Mock compute_token_hash to succeed
    mock_compute_token_hash_result = strdup("mocked_hash_value");

    enum MHD_Result result = handle_post_auth_logout(
        mock_connection, "/api/auth/logout", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);

    // Cleanup
    if (mock_validate_jwt_for_logout_result.claims) {
        free(mock_validate_jwt_for_logout_result.claims->username);
        free(mock_validate_jwt_for_logout_result.claims->database);
        free(mock_validate_jwt_for_logout_result.claims);
        mock_validate_jwt_for_logout_result.claims = NULL;
    }
}

// Test: JWT validation with unknown error (JWT_ERROR_NONE case)
void test_handle_post_auth_logout_jwt_validation_unknown_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer to complete so we get to JWT validation
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock Authorization header
    mock_mhd_set_lookup_result("Bearer valid.jwt.token");

    // Mock JWT validation to fail with JWT_ERROR_NONE (unknown error case)
    mock_validate_jwt_for_logout_result.valid = false;
    mock_validate_jwt_for_logout_result.error = JWT_ERROR_NONE; // This maps to "Unknown error"

    enum MHD_Result result = handle_post_auth_logout(
        mock_connection, "/api/auth/logout", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}


// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    UNITY_BEGIN();

    // Error path tests
    RUN_TEST(test_handle_post_auth_logout_api_buffer_error);
    RUN_TEST(test_handle_post_auth_logout_api_buffer_method_error);
    RUN_TEST(test_handle_post_auth_logout_missing_authorization_header);
    RUN_TEST(test_handle_post_auth_logout_invalid_authorization_header_format);
    RUN_TEST(test_handle_post_auth_logout_empty_token);
    RUN_TEST(test_handle_post_auth_logout_jwt_validation_expired);
    RUN_TEST(test_handle_post_auth_logout_jwt_validation_not_yet_valid);
    RUN_TEST(test_handle_post_auth_logout_jwt_validation_invalid_signature);
    RUN_TEST(test_handle_post_auth_logout_jwt_validation_unsupported_algorithm);
    RUN_TEST(test_handle_post_auth_logout_jwt_validation_invalid_format);
    RUN_TEST(test_handle_post_auth_logout_jwt_validation_revoked);
    RUN_TEST(test_handle_post_auth_logout_jwt_validation_unknown_error);
    RUN_TEST(test_handle_post_auth_logout_jwt_validation_claims_null);
    RUN_TEST(test_handle_post_auth_logout_no_database_specified);
    RUN_TEST(test_handle_post_auth_logout_compute_hash_failure);

    // Success path tests
    RUN_TEST(test_handle_post_auth_logout_success);
    RUN_TEST(test_handle_post_auth_logout_with_database_in_request_body);
    RUN_TEST(test_handle_post_auth_logout_with_invalid_json_in_request_body);

    return UNITY_END();
}