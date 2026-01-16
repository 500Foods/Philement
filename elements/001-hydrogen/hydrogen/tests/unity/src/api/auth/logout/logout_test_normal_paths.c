/*
 * Unity Unit Tests for logout.c - Normal Path Testing
 *
 * Tests normal operation and success paths in the logout endpoint
 *
 * CHANGELOG:
 * 2026-01-15: Initial version - Tests for logout normal paths using mocks
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

// Include necessary headers for the module being tested
#include <src/api/auth/auth_service.h>
#include <src/api/api_utils.h>
#include <src/api/auth/logout/logout.h>

// ============================================================================
// Function Prototypes
// ============================================================================

// Test functions
void test_handle_post_auth_logout_success_with_username(void);
void test_handle_post_auth_logout_success_without_username(void);
void test_handle_post_auth_logout_success_with_request_database(void);
void test_handle_post_auth_logout_success_with_token_database(void);
void test_handle_post_auth_logout_success_with_empty_request_body(void);
void test_handle_post_auth_logout_success_with_null_request_body(void);

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
// Test Functions - Normal Paths
// ============================================================================

// Test: Successful logout with username in claims
void test_handle_post_auth_logout_success_with_username(void) {
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

// Test: Successful logout without username in claims
void test_handle_post_auth_logout_success_without_username(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer to complete
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock Authorization header
    mock_mhd_set_lookup_result("Bearer valid.jwt.token");

    // Mock JWT validation to succeed with valid claims but no username
    mock_validate_jwt_for_logout_result.valid = true;
    mock_validate_jwt_for_logout_result.error = JWT_ERROR_NONE;
    mock_validate_jwt_for_logout_result.claims = calloc(1, sizeof(jwt_claims_t));
    if (mock_validate_jwt_for_logout_result.claims) {
        mock_validate_jwt_for_logout_result.claims->user_id = 123;
        mock_validate_jwt_for_logout_result.claims->username = NULL; // No username
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
        free(mock_validate_jwt_for_logout_result.claims->database);
        free(mock_validate_jwt_for_logout_result.claims);
        mock_validate_jwt_for_logout_result.claims = NULL;
    }
}

// Test: Successful logout with database specified in request body
void test_handle_post_auth_logout_success_with_request_database(void) {
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

// Test: Successful logout with database from token claims
void test_handle_post_auth_logout_success_with_token_database(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer with empty request body
    mock_api_buffer = create_mock_buffer("", 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock Authorization header
    mock_mhd_set_lookup_result("Bearer valid.jwt.token");

    // Mock JWT validation to succeed with claims containing database
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

// Test: Successful logout with empty request body
void test_handle_post_auth_logout_success_with_empty_request_body(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer with empty request body
    mock_api_buffer = create_mock_buffer("", 'P');
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

// Test: Successful logout with null request body
void test_handle_post_auth_logout_success_with_null_request_body(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer with null request body
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

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    UNITY_BEGIN();

    // Normal path tests
    RUN_TEST(test_handle_post_auth_logout_success_with_username);
    RUN_TEST(test_handle_post_auth_logout_success_without_username);
    RUN_TEST(test_handle_post_auth_logout_success_with_request_database);
    RUN_TEST(test_handle_post_auth_logout_success_with_token_database);
    RUN_TEST(test_handle_post_auth_logout_success_with_empty_request_body);
    RUN_TEST(test_handle_post_auth_logout_success_with_null_request_body);

    return UNITY_END();
}