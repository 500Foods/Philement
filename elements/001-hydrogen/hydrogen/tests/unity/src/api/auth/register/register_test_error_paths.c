/*
 * Unity Unit Tests for register.c - Error Path Testing
 *
 * Tests error conditions and failure paths in the register endpoint
 *
 * CHANGELOG:
 * 2026-01-15: Initial version - Tests for register error paths using mocks
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
#define validate_registration_input mock_validate_registration_input
#define verify_api_key mock_verify_api_key
#define check_license_expiry mock_check_license_expiry
#define check_username_availability mock_check_username_availability
#define create_account_record mock_create_account_record
#define compute_password_hash mock_compute_password_hash
#define api_parse_json_body mock_api_parse_json_body
#define api_free_post_buffer mock_api_free_post_buffer
#define api_send_json_response mock_api_send_json_response

// Include necessary headers for the module being tested
#include <src/api/auth/auth_service.h>
#include <src/api/api_utils.h>
#include <src/api/auth/register/register.h>

// ============================================================================
// Function Prototypes
// ============================================================================

// Test functions
void test_handle_post_auth_register_api_buffer_error(void);
void test_handle_post_auth_register_api_buffer_method_error(void);
void test_handle_post_auth_register_empty_request_body(void);
void test_handle_post_auth_register_invalid_json(void);
void test_handle_post_auth_register_missing_required_parameters(void);
void test_handle_post_auth_register_validation_failed(void);
void test_handle_post_auth_register_api_key_verification_failed(void);
void test_handle_post_auth_register_license_expired(void);
void test_handle_post_auth_register_username_not_available(void);
void test_handle_post_auth_register_create_account_failed(void);
void test_handle_post_auth_register_password_hash_failed(void);
void test_handle_post_auth_register_success(void);

// Helper functions
void reset_all_mocks(void);
ApiPostBuffer* create_mock_buffer(const char* json_data, char method);

// ============================================================================
// Mock Auth Service Functions
// ============================================================================

// Mock state variables
static bool mock_validate_registration_input_result = true;
static bool mock_verify_api_key_result = true;
static system_info_t mock_verify_api_key_sys_info = {0};
static bool mock_check_license_expiry_result = true;
static bool mock_check_username_availability_result = true;
static int mock_create_account_record_result = 123;
static char* mock_compute_password_hash_result = NULL;

// Mock implementations with weak linkage to override real implementations
__attribute__((weak))
bool mock_validate_registration_input(const char* username, const char* password, const char* email, const char* full_name) {
    (void)username; (void)password; (void)email; (void)full_name;
    return mock_validate_registration_input_result;
}

__attribute__((weak))
bool mock_verify_api_key(const char* api_key, const char* database, system_info_t* sys_info) {
    (void)api_key; (void)database;
    if (mock_verify_api_key_result && sys_info) {
        *sys_info = mock_verify_api_key_sys_info;
    }
    return mock_verify_api_key_result;
}

__attribute__((weak))
bool mock_check_license_expiry(time_t license_expiry) {
    (void)license_expiry;
    return mock_check_license_expiry_result;
}

__attribute__((weak))
bool mock_check_username_availability(const char* username, const char* database) {
    (void)username; (void)database;
    return mock_check_username_availability_result;
}

__attribute__((weak))
int mock_create_account_record(const char* username, const char* email, const char* password_hash, const char* full_name, const char* database) {
    (void)username; (void)email; (void)password_hash; (void)full_name; (void)database;
    return mock_create_account_record_result;
}

__attribute__((weak))
char* mock_compute_password_hash(const char* password, int account_id) {
    (void)password; (void)account_id;
    if (mock_compute_password_hash_result) {
        return strdup(mock_compute_password_hash_result);
    }
    return NULL;
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
    mock_validate_registration_input_result = true;
    memset(&mock_verify_api_key_sys_info, 0, sizeof(system_info_t));
    mock_verify_api_key_sys_info.system_id = 1;
    mock_verify_api_key_sys_info.app_id = 1;
    mock_verify_api_key_sys_info.license_expiry = time(NULL) + 86400; // Tomorrow
    mock_verify_api_key_result = true;
    mock_check_license_expiry_result = true;
    mock_check_username_availability_result = true;
    mock_create_account_record_result = 123;
    free(mock_compute_password_hash_result);
    mock_compute_password_hash_result = NULL;

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
void test_handle_post_auth_register_api_buffer_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_api_buffer_post_data_result = API_BUFFER_ERROR;

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: API_BUFFER_METHOD_ERROR case
void test_handle_post_auth_register_api_buffer_method_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    mock_api_buffer_post_data_result = API_BUFFER_METHOD_ERROR;

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "GET", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Empty request body
void test_handle_post_auth_register_empty_request_body(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer with empty data
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Invalid JSON in request body
void test_handle_post_auth_register_invalid_json(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer with invalid JSON
    mock_api_buffer = create_mock_buffer("invalid json", 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Missing required parameters
void test_handle_post_auth_register_missing_required_parameters(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer with JSON missing required parameters
    const char* json_data = "{\"username\":\"testuser\"}"; // Missing password, email, api_key, database
    mock_api_buffer = create_mock_buffer(json_data, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Registration input validation failed
void test_handle_post_auth_register_validation_failed(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer with valid JSON
    const char* json_data = "{\"username\":\"testuser\",\"password\":\"password123\",\"email\":\"test@example.com\",\"api_key\":\"key123\",\"database\":\"testdb\"}";
    mock_api_buffer = create_mock_buffer(json_data, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock validation to fail
    mock_validate_registration_input_result = false;

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: API key verification failed
void test_handle_post_auth_register_api_key_verification_failed(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer with valid JSON
    const char* json_data = "{\"username\":\"testuser\",\"password\":\"password123\",\"email\":\"test@example.com\",\"api_key\":\"invalid_key\",\"database\":\"testdb\"}";
    mock_api_buffer = create_mock_buffer(json_data, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock API key verification to fail
    mock_verify_api_key_result = false;

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: License expired
void test_handle_post_auth_register_license_expired(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer with valid JSON
    const char* json_data = "{\"username\":\"testuser\",\"password\":\"password123\",\"email\":\"test@example.com\",\"api_key\":\"key123\",\"database\":\"testdb\"}";
    mock_api_buffer = create_mock_buffer(json_data, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock license check to fail
    mock_check_license_expiry_result = false;

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Username not available
void test_handle_post_auth_register_username_not_available(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer with valid JSON
    const char* json_data = "{\"username\":\"existinguser\",\"password\":\"password123\",\"email\":\"test@example.com\",\"api_key\":\"key123\",\"database\":\"testdb\"}";
    mock_api_buffer = create_mock_buffer(json_data, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock username availability check to fail
    mock_check_username_availability_result = false;

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Create account record failed
void test_handle_post_auth_register_create_account_failed(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer with valid JSON
    const char* json_data = "{\"username\":\"testuser\",\"password\":\"password123\",\"email\":\"test@example.com\",\"api_key\":\"key123\",\"database\":\"testdb\"}";
    mock_api_buffer = create_mock_buffer(json_data, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock create account to fail
    mock_create_account_record_result = 0;

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Password hash computation failed
void test_handle_post_auth_register_password_hash_failed(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer with valid JSON
    const char* json_data = "{\"username\":\"testuser\",\"password\":\"password123\",\"email\":\"test@example.com\",\"api_key\":\"key123\",\"database\":\"testdb\"}";
    mock_api_buffer = create_mock_buffer(json_data, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock password hash to fail
    mock_compute_password_hash_result = NULL;

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Successful registration
void test_handle_post_auth_register_success(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    // Set up buffer with valid JSON
    const char* json_data = "{\"username\":\"testuser\",\"password\":\"password123\",\"email\":\"test@example.com\",\"api_key\":\"key123\",\"database\":\"testdb\",\"full_name\":\"Test User\"}";
    mock_api_buffer = create_mock_buffer(json_data, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    // Mock password hash to succeed
    mock_compute_password_hash_result = strdup("hashed_password");

    enum MHD_Result result = handle_post_auth_register(
        mock_connection, "/api/auth/register", "POST", "HTTP/1.1",
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
    RUN_TEST(test_handle_post_auth_register_api_buffer_error);
    RUN_TEST(test_handle_post_auth_register_api_buffer_method_error);
    RUN_TEST(test_handle_post_auth_register_empty_request_body);
    RUN_TEST(test_handle_post_auth_register_invalid_json);
    RUN_TEST(test_handle_post_auth_register_missing_required_parameters);
    RUN_TEST(test_handle_post_auth_register_validation_failed);
    RUN_TEST(test_handle_post_auth_register_api_key_verification_failed);
    RUN_TEST(test_handle_post_auth_register_license_expired);
    RUN_TEST(test_handle_post_auth_register_username_not_available);
    RUN_TEST(test_handle_post_auth_register_create_account_failed);
    RUN_TEST(test_handle_post_auth_register_password_hash_failed);

    // Success path test
    RUN_TEST(test_handle_post_auth_register_success);

    return UNITY_END();
}