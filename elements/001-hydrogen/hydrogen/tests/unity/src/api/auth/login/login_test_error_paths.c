/*
 * Unity Test File: Login Error Paths
 * This file contains unit tests for error handling in the login endpoint
 *
 * Tests: handle_auth_login_request() - Error paths and edge cases
 *
 * CHANGELOG:
 * 2026-01-12: Initial version - Tests for login error paths
 *
 * TEST_VERSION: 1.0.0
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Mock definitions must come before source includes
#define USE_MOCK_LIBMICROHTTPD
#include <unity/mocks/mock_libmicrohttpd.h>

// Include necessary headers for the module being tested
#include <src/api/auth/auth_service.h>
#include <src/api/api_utils.h>
#include <src/api/auth/login/login.h>

// ============================================================================
// Function Prototypes
// ============================================================================

// Test functions
void test_handle_auth_login_api_buffer_error(void);
void test_handle_auth_login_api_buffer_method_error(void);
void test_handle_auth_login_empty_request_body(void);
void test_handle_auth_login_invalid_json(void);
void test_handle_auth_login_get_request_not_supported(void);
void test_handle_auth_login_missing_required_parameters(void);
void test_handle_auth_login_validation_failed(void);
void test_handle_auth_login_license_expired(void);
void test_handle_auth_login_failed_to_get_client_ip(void);
void test_handle_auth_login_ip_blacklisted(void);
void test_handle_auth_login_rate_limit_exceeded(void);
void test_handle_auth_login_account_not_found(void);
void test_handle_auth_login_account_disabled(void);
void test_handle_auth_login_account_not_authorized(void);
void test_handle_auth_login_failed_to_generate_jwt(void);
void test_handle_auth_login_failed_to_compute_jwt_hash(void);

// Helper functions
void reset_all_mocks(void);
ApiPostBuffer* create_mock_buffer(const char* json_data, char method);
account_info_t* create_mock_account(int id, const char* username, const char* email,
                                   bool enabled, bool authorized);

// ============================================================================
// Mock Auth Service Functions
// ============================================================================

// Mock state variables
static bool mock_validate_login_input_result = true;
static bool mock_verify_api_key_result = true;
static bool mock_check_license_expiry_result = true;
static char* mock_api_get_client_ip_result = NULL;
static bool mock_check_ip_whitelist_result = false;
static bool mock_check_ip_blacklist_result = false;
static int mock_check_failed_attempts_result = 0;
static bool mock_handle_rate_limiting_result = false;
static account_info_t* mock_lookup_account_result = NULL;
static bool mock_verify_password_and_status_result = true;
static char* mock_generate_jwt_result = NULL;
static char* mock_compute_token_hash_result = NULL;

// Mock implementations with weak linkage to override real implementations
__attribute__((weak))
bool validate_login_input(const char* login_id, const char* password,
                         const char* api_key, const char* tz) {
    (void)login_id; (void)password; (void)api_key; (void)tz;
    return mock_validate_login_input_result;
}

__attribute__((weak))
bool verify_api_key(const char* api_key, const char* database, system_info_t* sys_info) {
    (void)api_key; (void)database;
    if (mock_verify_api_key_result && sys_info) {
        sys_info->system_id = 1;
        sys_info->app_id = 1;
        sys_info->license_expiry = time(NULL) + 86400; // 1 day from now
    }
    return mock_verify_api_key_result;
}

__attribute__((weak))
bool check_license_expiry(time_t license_expiry) {
    (void)license_expiry;
    return mock_check_license_expiry_result;
}

__attribute__((weak))
char* api_get_client_ip(struct MHD_Connection *connection) {
    (void)connection;
    if (mock_api_get_client_ip_result) {
        return strdup(mock_api_get_client_ip_result);
    }
    return NULL;
}

__attribute__((weak))
bool check_ip_whitelist(const char* client_ip, const char* database) {
    (void)client_ip; (void)database;
    return mock_check_ip_whitelist_result;
}

__attribute__((weak))
bool check_ip_blacklist(const char* client_ip, const char* database) {
    (void)client_ip; (void)database;
    return mock_check_ip_blacklist_result;
}

__attribute__((weak))
void log_login_attempt(const char* login_id, const char* client_ip,
                      const char* user_agent, time_t timestamp, const char* database) {
    (void)login_id; (void)client_ip; (void)user_agent; (void)timestamp; (void)database;
}

__attribute__((weak))
int check_failed_attempts(const char* login_id, const char* client_ip,
                         time_t window_start, const char* database) {
    (void)login_id; (void)client_ip; (void)window_start; (void)database;
    return mock_check_failed_attempts_result;
}

__attribute__((weak))
bool handle_rate_limiting(const char* client_ip, int failed_count,
                         bool is_whitelisted, const char* database) {
    (void)client_ip; (void)failed_count; (void)is_whitelisted; (void)database;
    return mock_handle_rate_limiting_result;
}

__attribute__((weak))
account_info_t* lookup_account(const char* login_id, const char* database) {
    (void)login_id; (void)database;
    return mock_lookup_account_result;
}

__attribute__((weak))
bool verify_password_and_status(const char* password, int account_id,
                               const char* database, account_info_t* account) {
    (void)password; (void)account_id; (void)database; (void)account;
    return mock_verify_password_and_status_result;
}

__attribute__((weak))
char* generate_jwt(account_info_t* account, system_info_t* system,
                  const char* client_ip, const char* tz, const char* database, time_t issued_at) {
    (void)account; (void)system; (void)client_ip; (void)tz; (void)database; (void)issued_at;
    if (mock_generate_jwt_result) {
        return strdup(mock_generate_jwt_result);
    }
    return NULL;
}

__attribute__((weak))
char* compute_token_hash(const char* token) {
    (void)token;
    if (mock_compute_token_hash_result) {
        return strdup(mock_compute_token_hash_result);
    }
    return NULL;
}

__attribute__((weak))
void store_jwt(int account_id, const char* jwt_hash, time_t expires_at, const char* database) {
    (void)account_id; (void)jwt_hash; (void)expires_at; (void)database;
}

__attribute__((weak))
void free_account_info(account_info_t* account) {
    if (!account) return;
    free(account->username);
    free(account->email);
    free(account->roles);
    free(account);
}

// ============================================================================
// Mock API Utils Functions
// ============================================================================

// Mock state for api_buffer_post_data
static ApiBufferResult mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;
static ApiPostBuffer* mock_api_buffer = NULL;

__attribute__((weak))
ApiBufferResult api_buffer_post_data(
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
enum MHD_Result api_send_error_and_cleanup(
    struct MHD_Connection *connection,
    void **con_cls,
    const char *error_message,
    unsigned int http_status
) {
    (void)connection; (void)con_cls; (void)error_message; (void)http_status;
    return MHD_YES;
}

__attribute__((weak))
void api_free_post_buffer(void **con_cls) {
    (void)con_cls;
}

__attribute__((weak))
json_t *api_parse_json_body(ApiPostBuffer *buffer) {
    if (!buffer || !buffer->data || buffer->size == 0) {
        return NULL;
    }
    return json_loads(buffer->data, 0, NULL);
}

__attribute__((weak))
enum MHD_Result api_send_json_response(struct MHD_Connection *connection,
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
    mock_validate_login_input_result = true;
    mock_verify_api_key_result = true;
    mock_check_license_expiry_result = true;
    
    free(mock_api_get_client_ip_result);
    mock_api_get_client_ip_result = NULL;
    
    mock_check_ip_whitelist_result = false;
    mock_check_ip_blacklist_result = false;
    mock_check_failed_attempts_result = 0;
    mock_handle_rate_limiting_result = false;
    mock_lookup_account_result = NULL;
    mock_verify_password_and_status_result = true;
    
    free(mock_generate_jwt_result);
    mock_generate_jwt_result = NULL;
    
    free(mock_compute_token_hash_result);
    mock_compute_token_hash_result = NULL;
    
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;
    
    if (mock_api_buffer) {
        free(mock_api_buffer->data);
        free(mock_api_buffer);
        mock_api_buffer = NULL;
    }
    
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

account_info_t* create_mock_account(int id, const char* username, const char* email,
                                   bool enabled, bool authorized) {
    account_info_t* account = calloc(1, sizeof(account_info_t));
    if (!account) {
        return NULL;
    }
    account->id = id;
    account->username = username ? strdup(username) : NULL;
    account->email = email ? strdup(email) : NULL;
    account->enabled = enabled;
    account->authorized = authorized;
    account->roles = strdup("user");
    return account;
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

// Test: API_BUFFER_ERROR case (lines 57-60)
void test_handle_auth_login_api_buffer_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;
    
    mock_api_buffer_post_data_result = API_BUFFER_ERROR;
    
    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );
    
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: API_BUFFER_METHOD_ERROR case (lines 62-65)
void test_handle_auth_login_api_buffer_method_error(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;
    
    mock_api_buffer_post_data_result = API_BUFFER_METHOD_ERROR;
    
    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "PUT", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );
    
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Empty request body (lines 75-77)
void test_handle_auth_login_empty_request_body(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;
    
    // Create empty buffer
    mock_api_buffer = create_mock_buffer(NULL, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;
    
    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );
    
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Invalid JSON (lines 85-86)
void test_handle_auth_login_invalid_json(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;
    
    // Create buffer with invalid JSON
    mock_api_buffer = create_mock_buffer("{invalid json", 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;
    
    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );
    
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: GET request not supported (lines 90-92)
void test_handle_auth_login_get_request_not_supported(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;
    
    // Create GET request buffer
    mock_api_buffer = create_mock_buffer("{}", 'G');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;
    
    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "GET", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );
    
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Missing required parameters (lines 107-111)
void test_handle_auth_login_missing_required_parameters(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;
    
    // Create JSON with missing parameters
    const char* json = "{\"login_id\":\"test\"}";
    mock_api_buffer = create_mock_buffer(json, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;
    
    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );
    
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Login input validation failed (lines 116-120)
void test_handle_auth_login_validation_failed(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;
    
    const char* json = "{\"login_id\":\"test\",\"password\":\"pass\",\"api_key\":\"key\",\"tz\":\"UTC\",\"database\":\"db\"}";
    mock_api_buffer = create_mock_buffer(json, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;
    mock_validate_login_input_result = false;
    
    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );
    
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: License expired (lines 140-144)
void test_handle_auth_login_license_expired(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;
    
    const char* json = "{\"login_id\":\"test\",\"password\":\"Password123!\",\"api_key\":\"key\",\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_buffer = create_mock_buffer(json, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;
    mock_check_license_expiry_result = false;
    
    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );
    
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Failed to retrieve client IP address (lines 152-156)
void test_handle_auth_login_failed_to_get_client_ip(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;
    
    const char* json = "{\"login_id\":\"test\",\"password\":\"Password123!\",\"api_key\":\"key\",\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_buffer = create_mock_buffer(json, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;
    mock_api_get_client_ip_result = NULL; // Force NULL return
    
    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );
    
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: IP blacklisted (lines 165-170)
void test_handle_auth_login_ip_blacklisted(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;
    
    const char* json = "{\"login_id\":\"test\",\"password\":\"Password123!\",\"api_key\":\"key\",\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_buffer = create_mock_buffer(json, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_check_ip_blacklist_result = true;
    
    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );
    
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Rate limit exceeded (lines 194-201)
void test_handle_auth_login_rate_limit_exceeded(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;
    
    const char* json = "{\"login_id\":\"test\",\"password\":\"Password123!\",\"api_key\":\"key\",\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_buffer = create_mock_buffer(json, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_check_failed_attempts_result = 10;
    mock_handle_rate_limiting_result = true; // Should block
    
    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );
    
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Account not found (lines 207-212)
void test_handle_auth_login_account_not_found(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;
    
    const char* json = "{\"login_id\":\"test\",\"password\":\"Password123!\",\"api_key\":\"key\",\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_buffer = create_mock_buffer(json, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_lookup_account_result = NULL; // Account not found
    
    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );
    
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Account disabled (lines 220-227)
void test_handle_auth_login_account_disabled(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;
    
    const char* json = "{\"login_id\":\"test\",\"password\":\"Password123!\",\"api_key\":\"key\",\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_buffer = create_mock_buffer(json, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_lookup_account_result = create_mock_account(1, "test", "test@example.com", false, true);
    
    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );
    
    TEST_ASSERT_EQUAL(MHD_YES, result);
    free_account_info(mock_lookup_account_result);
}

// Test: Account not authorized (lines 232-239)
void test_handle_auth_login_account_not_authorized(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;
    
    const char* json = "{\"login_id\":\"test\",\"password\":\"Password123!\",\"api_key\":\"key\",\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_buffer = create_mock_buffer(json, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_lookup_account_result = create_mock_account(1, "test", "test@example.com", true, false);
    
    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );
    
    TEST_ASSERT_EQUAL(MHD_YES, result);
    free_account_info(mock_lookup_account_result);
}

// Test: Failed to generate JWT (lines 265-271)
void test_handle_auth_login_failed_to_generate_jwt(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;
    
    const char* json = "{\"login_id\":\"test\",\"password\":\"Password123!\",\"api_key\":\"key\",\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_buffer = create_mock_buffer(json, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_lookup_account_result = create_mock_account(1, "test", "test@example.com", true, true);
    mock_generate_jwt_result = NULL; // Force JWT generation failure
    
    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );
    
    TEST_ASSERT_EQUAL(MHD_YES, result);
    free_account_info(mock_lookup_account_result);
}

// Test: Failed to compute JWT hash (lines 280-287)
void test_handle_auth_login_failed_to_compute_jwt_hash(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;
    
    const char* json = "{\"login_id\":\"test\",\"password\":\"Password123!\",\"api_key\":\"key\",\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_buffer = create_mock_buffer(json, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_lookup_account_result = create_mock_account(1, "test", "test@example.com", true, true);
    mock_generate_jwt_result = strdup("test_jwt_token");
    mock_compute_token_hash_result = NULL; // Force hash computation failure
    
    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );
    
    TEST_ASSERT_EQUAL(MHD_YES, result);
    free_account_info(mock_lookup_account_result);
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    UNITY_BEGIN();
    
    // Error path tests
    RUN_TEST(test_handle_auth_login_api_buffer_error);
    RUN_TEST(test_handle_auth_login_api_buffer_method_error);
    RUN_TEST(test_handle_auth_login_empty_request_body);
    RUN_TEST(test_handle_auth_login_invalid_json);
    RUN_TEST(test_handle_auth_login_get_request_not_supported);
    RUN_TEST(test_handle_auth_login_missing_required_parameters);
    RUN_TEST(test_handle_auth_login_validation_failed);
    RUN_TEST(test_handle_auth_login_license_expired);
    RUN_TEST(test_handle_auth_login_failed_to_get_client_ip);
    RUN_TEST(test_handle_auth_login_ip_blacklisted);
    RUN_TEST(test_handle_auth_login_rate_limit_exceeded);
    RUN_TEST(test_handle_auth_login_account_not_found);
    RUN_TEST(test_handle_auth_login_account_disabled);
    RUN_TEST(test_handle_auth_login_account_not_authorized);
    RUN_TEST(test_handle_auth_login_failed_to_generate_jwt);
    RUN_TEST(test_handle_auth_login_failed_to_compute_jwt_hash);
    
    return UNITY_END();
}
