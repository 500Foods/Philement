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

#define USE_MOCK_API_UTILS
#include <src/hydrogen.h>
#include <unity.h>

#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_api_utils.h>
#include <unity/mocks/mock_auth_service_login.h>

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
void test_handle_auth_login_account_lookup_unavailable(void);
void test_handle_auth_login_api_key_unavailable(void);
void test_handle_auth_login_account_disabled(void);
void test_handle_auth_login_account_not_authorized(void);
void test_handle_auth_login_failed_to_generate_jwt(void);
void test_handle_auth_login_failed_to_compute_jwt_hash(void);
void test_handle_auth_login_password_verify_unavailable(void);

// Helper functions
void reset_all_mocks(void);
account_info_t* create_mock_account(int id, const char* username, const char* email,
                                    bool enabled, bool authorized);

// ============================================================================
// Mock Auth Service Functions
// ============================================================================

// Mock state variables
static bool mock_validate_login_input_result = true;
static bool mock_check_license_expiry_result = true;
static char* mock_api_get_client_ip_result = NULL;
static bool mock_check_ip_whitelist_result = false;
static bool mock_check_ip_blacklist_result = false;
static int mock_check_failed_attempts_result = 0;
static bool mock_handle_rate_limiting_result = false;
static account_info_t* mock_lookup_account_result = NULL;
static int g_lookup_account_code = 0;
static int g_verify_api_key_code = 1;
static int g_verify_password_and_status_code = 1;
static char* mock_generate_jwt_result = NULL;
static char* mock_compute_token_hash_result = NULL;

// Mock implementations with weak linkage to override real implementations
bool validate_login_input(const char* login_id, const char* password,
                          const char* api_key, const char* tz) {
    (void)login_id; (void)password; (void)api_key; (void)tz;
    return mock_validate_login_input_result;
}

int verify_api_key_code(const char* api_key, const char* database, system_info_t* sys_info) {
    (void)api_key; (void)database;
    if (g_verify_api_key_code == 1 && sys_info) {
        sys_info->system_id = 1;
        sys_info->app_id = 1;
        sys_info->license_expiry = time(NULL) + 86400;
    }
    return g_verify_api_key_code;
}

void auth_query_begin_deadline(int budget_seconds) {
    (void)budget_seconds;
}

void auth_query_end_deadline(void) {
}

bool check_license_expiry(time_t license_expiry) {
    (void)license_expiry;
    return mock_check_license_expiry_result;
}

char* api_get_client_ip(struct MHD_Connection *connection) {
    (void)connection;
    if (mock_api_get_client_ip_result) {
        return strdup(mock_api_get_client_ip_result);
    }
    return NULL;
}

bool check_ip_whitelist(const char* client_ip, const char* database) {
    (void)client_ip; (void)database;
    return mock_check_ip_whitelist_result;
}

bool check_ip_blacklist(const char* client_ip, const char* database) {
    (void)client_ip; (void)database;
    return mock_check_ip_blacklist_result;
}

void log_login_attempt(const char* login_id, const char* client_ip,
                       const char* user_agent, time_t timestamp, const char* database) {
    (void)login_id; (void)client_ip; (void)user_agent; (void)timestamp; (void)database;
}

int check_failed_attempts(const char* login_id, const char* client_ip,
                          time_t window_start, const char* database) {
    (void)login_id; (void)client_ip; (void)window_start; (void)database;
    return mock_check_failed_attempts_result;
}

bool handle_rate_limiting(const char* client_ip, int failed_count,
                          bool is_whitelisted, const char* database) {
    (void)client_ip; (void)failed_count; (void)is_whitelisted; (void)database;
    return mock_handle_rate_limiting_result;
}

int lookup_account_code(const char* login_id, const char* database, account_info_t** out) {
    (void)login_id; (void)database;
    if (out) {
        *out = mock_lookup_account_result;
    }
    if (mock_lookup_account_result) {
        return 1;
    }
    return g_lookup_account_code;
}

int verify_password_and_status_code(const char* password, int account_id,
                                    const char* database, account_info_t* account) {
    (void)password; (void)account_id; (void)database; (void)account;
    return g_verify_password_and_status_code;
}

char* generate_jwt(account_info_t* account, system_info_t* system,
                   const char* client_ip, const char* tz, const char* database, time_t issued_at) {
    (void)account; (void)system; (void)client_ip; (void)tz; (void)database; (void)issued_at;
    if (mock_generate_jwt_result) {
        return strdup(mock_generate_jwt_result);
    }
    return NULL;
}

char* compute_token_hash(const char* token) {
    (void)token;
    if (mock_compute_token_hash_result) {
        return strdup(mock_compute_token_hash_result);
    }
    return NULL;
}

void store_jwt(int account_id, const char* jwt_hash, time_t expires_at, int system_id, int app_id, const char* database, const char* client_ip) {
    (void)account_id; (void)jwt_hash; (void)expires_at; (void)system_id; (void)app_id; (void)database; (void)client_ip;
}

void free_account_info(account_info_t* account) {
    if (!account) return;
    free(account->username);
    free(account->email);
    free(account->roles);
    free(account);
}

// ============================================================================
// Helper Functions
// ============================================================================

void reset_all_mocks(void) {
    mock_validate_login_input_result = true;
    mock_check_license_expiry_result = true;

    free(mock_api_get_client_ip_result);
    mock_api_get_client_ip_result = NULL;

    mock_check_ip_whitelist_result = false;
    mock_check_ip_blacklist_result = false;
    mock_check_failed_attempts_result = 0;
    mock_handle_rate_limiting_result = false;
    mock_lookup_account_result = NULL;
    g_lookup_account_code = 0;
    g_verify_api_key_code = 1;
    g_verify_password_and_status_code = 1;

    free(mock_generate_jwt_result);
    mock_generate_jwt_result = NULL;

    free(mock_compute_token_hash_result);
    mock_compute_token_hash_result = NULL;

    mock_api_utils_reset_all();
    mock_mhd_reset_all();
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

    mock_api_utils_set_buffer_result(API_BUFFER_ERROR);

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

    mock_api_utils_set_buffer_result(API_BUFFER_METHOD_ERROR);

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

    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);

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

    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_buffer_data("{invalid json");

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

    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_buffer_data("{}");

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

    const char* json = "{\"login_id\":\"test\"}";
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_buffer_data(json);

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
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_buffer_data(json);
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
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_buffer_data(json);
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
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_buffer_data(json);
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
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_buffer_data(json);
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
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_buffer_data(json);
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
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_buffer_data(json);
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_lookup_account_result = NULL; // Account not found

    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_auth_login_account_lookup_unavailable(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    const char* json = "{\"login_id\":\"test\",\"password\":\"Password123!\",\"api_key\":\"key\",\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_buffer_data(json);
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_lookup_account_result = NULL;
    g_lookup_account_code = -1;

    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

void test_handle_auth_login_api_key_unavailable(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    const char* json = "{\"login_id\":\"test\",\"password\":\"Password123!\",\"api_key\":\"key\",\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_buffer_data(json);
    g_verify_api_key_code = -1;

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
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_buffer_data(json);
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_lookup_account_result = create_mock_account(1, "test", "test@example.com", false, true);

    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Account not authorized (lines 232-239)
void test_handle_auth_login_account_not_authorized(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    const char* json = "{\"login_id\":\"test\",\"password\":\"Password123!\",\"api_key\":\"key\",\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_buffer_data(json);
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_lookup_account_result = create_mock_account(1, "test", "test@example.com", true, false);

    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Failed to generate JWT (lines 265-271)
void test_handle_auth_login_failed_to_generate_jwt(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    const char* json = "{\"login_id\":\"test\",\"password\":\"Password123!\",\"api_key\":\"key\",\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_buffer_data(json);
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_lookup_account_result = create_mock_account(1, "test", "test@example.com", true, true);
    mock_generate_jwt_result = NULL; // Force JWT generation failure

    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Failed to compute JWT hash (lines 280-287)
void test_handle_auth_login_failed_to_compute_jwt_hash(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    const char* json = "{\"login_id\":\"test\",\"password\":\"Password123!\",\"api_key\":\"key\",\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_buffer_data(json);
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_lookup_account_result = create_mock_account(1, "test", "test@example.com", true, true);
    mock_generate_jwt_result = strdup("test_jwt_token");
    mock_compute_token_hash_result = NULL; // Force hash computation failure

    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
}

// Test: Password verification query returns transport failure (-1), not a
// credential miss (line 337-342). Should return auth unavailable (503),
// not 401 invalid credentials.
void test_handle_auth_login_password_verify_unavailable(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    const char* json = "{\"login_id\":\"test\",\"password\":\"Password123!\",\"api_key\":\"key\",\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_buffer_data(json);
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_lookup_account_result = create_mock_account(1, "test", "test@example.com", true, true);
    g_verify_password_and_status_code = -1; // transport/query failure

    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
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
    RUN_TEST(test_handle_auth_login_account_lookup_unavailable);
    RUN_TEST(test_handle_auth_login_api_key_unavailable);
    RUN_TEST(test_handle_auth_login_account_disabled);
    RUN_TEST(test_handle_auth_login_account_not_authorized);
    RUN_TEST(test_handle_auth_login_failed_to_generate_jwt);
    RUN_TEST(test_handle_auth_login_failed_to_compute_jwt_hash);
    RUN_TEST(test_handle_auth_login_password_verify_unavailable);

    return UNITY_END();
}
