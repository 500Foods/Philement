/*
  * Unity Test File: Login Success Paths
  * This file contains unit tests for the success/happy path and remaining
  * branches of the /api/auth/login endpoint that were not exercised by
  * login_test_error_paths.c (notably the full success path through
  * auth_roles_from_database, generate_jwt, compute_token_hash, store_jwt
  * and the 200 response build).
  *
  * Tests: handle_auth_login_request() - Success path and credential branches
  *
  * CHANGELOG:
  * 2026-08-28: Initial version - Tests for login success paths and coverage
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
void test_handle_auth_login_success(void);
void test_handle_auth_login_invalid_api_key(void);
void test_handle_auth_login_wrong_password_unauthorized(void);
void test_handle_auth_login_wrong_password_rate_limited(void);
void test_handle_auth_login_roles_fallback_empty(void);
void test_handle_auth_login_jwt_generation_failure(void);

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
static char* mock_auth_roles_from_database_result = NULL;

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

char* auth_roles_from_database(int account_id, const char* database) {
    (void)account_id; (void)database;
    if (mock_auth_roles_from_database_result) {
        return strdup(mock_auth_roles_from_database_result);
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

    free(mock_auth_roles_from_database_result);
    mock_auth_roles_from_database_result = NULL;

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
// Test Functions - Success Paths and Credential Branches
// ============================================================================

// Full happy path: API key valid, license ok, IP not blacklisted, account
// found/enabled/authorized, password verifies, roles load, JWT generated and
// stored. Verifies the HTTP 200 response body fields.
void test_handle_auth_login_success(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    const char* json = "{\"login_id\":\"testuser\",\"password\":\"Password123!\","
                       "\"api_key\":\"key\",\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_buffer_data(json);
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_lookup_account_result = create_mock_account(1, "testuser", "test@example.com", true, true);
    mock_generate_jwt_result = strdup("test_jwt_token");
    mock_compute_token_hash_result = strdup("test_hash_value");
    mock_auth_roles_from_database_result = strdup("1,3");
    mock_api_utils_set_capture_mode(true);

    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(1, mock_api_utils_get_send_json_response_call_count());
    TEST_ASSERT_EQUAL(MHD_HTTP_OK, mock_api_utils_get_captured_status());

    json_t *response = mock_api_utils_get_captured_response();
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_boolean_value(json_object_get(response, "success")));
    TEST_ASSERT_EQUAL_STRING("test_jwt_token",
                             json_string_value(json_object_get(response, "token")));
    TEST_ASSERT_EQUAL_INT(1,
                          json_integer_value(json_object_get(response, "user_id")));
    TEST_ASSERT_EQUAL_STRING("testuser",
                             json_string_value(json_object_get(response, "username")));
    TEST_ASSERT_EQUAL_STRING("test@example.com",
                             json_string_value(json_object_get(response, "email")));
    TEST_ASSERT_EQUAL_STRING("1,3",
                             json_string_value(json_object_get(response, "roles")));
}

// API key present but invalid (code == 0): must return 401 "Invalid API key".
void test_handle_auth_login_invalid_api_key(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    const char* json = "{\"login_id\":\"testuser\",\"password\":\"Password123!\","
                       "\"api_key\":\"bad-key\",\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_buffer_data(json);
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    g_verify_api_key_code = 0; // invalid key
    mock_api_utils_set_capture_mode(true);

    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_UNAUTHORIZED, mock_api_utils_get_captured_status());
    TEST_ASSERT_NOT_NULL(mock_api_utils_get_captured_response());
    TEST_ASSERT_EQUAL_STRING("Invalid API key",
                             json_string_value(json_object_get(mock_api_utils_get_captured_response(), "error")));
}

// Wrong password (verify fails, no rate limit): 401 "Invalid credentials".
void test_handle_auth_login_wrong_password_unauthorized(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    const char* json = "{\"login_id\":\"testuser\",\"password\":\"wrong\",\"api_key\":\"key\","
                       "\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_buffer_data(json);
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_lookup_account_result = create_mock_account(1, "testuser", "test@example.com", true, true);
    g_verify_password_and_status_code = 0; // wrong password
    mock_handle_rate_limiting_result = false;       // not blocked
    mock_api_utils_set_capture_mode(true);

    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_UNAUTHORIZED, mock_api_utils_get_captured_status());
    TEST_ASSERT_EQUAL_STRING("Invalid credentials",
                             json_string_value(json_object_get(mock_api_utils_get_captured_response(), "error")));
}

// Wrong password with rate limiting triggered: 429 with retry_after.
void test_handle_auth_login_wrong_password_rate_limited(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    const char* json = "{\"login_id\":\"testuser\",\"password\":\"wrong\",\"api_key\":\"key\","
                       "\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_buffer_data(json);
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_lookup_account_result = create_mock_account(1, "testuser", "test@example.com", true, true);
    g_verify_password_and_status_code = 0;
    mock_handle_rate_limiting_result = true;  // blocked
    mock_check_failed_attempts_result = 5;
    mock_api_utils_set_capture_mode(true);

    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_TOO_MANY_REQUESTS, mock_api_utils_get_captured_status());
    TEST_ASSERT_EQUAL_STRING("Too many failed attempts",
                             json_string_value(json_object_get(mock_api_utils_get_captured_response(), "error")));
}

// auth_roles_from_database returns NULL: login.c must fall back to strdup("")
// so the response still carries an (empty) roles field.
void test_handle_auth_login_roles_fallback_empty(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    const char* json = "{\"login_id\":\"testuser\",\"password\":\"Password123!\","
                       "\"api_key\":\"key\",\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_buffer_data(json);
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_lookup_account_result = create_mock_account(1, "testuser", "test@example.com", true, true);
    mock_generate_jwt_result = strdup("test_jwt_token");
    mock_compute_token_hash_result = strdup("test_hash_value");
    // auth_roles_from_database_result left NULL -> fallback path
    mock_api_utils_set_capture_mode(true);

    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_OK, mock_api_utils_get_captured_status());
    TEST_ASSERT_EQUAL_STRING("",
                             json_string_value(json_object_get(mock_api_utils_get_captured_response(), "roles")));
}

// JWT generation returns NULL: 500 "Failed to generate authentication token".
void test_handle_auth_login_jwt_generation_failure(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    const char* json = "{\"login_id\":\"testuser\",\"password\":\"Password123!\","
                       "\"api_key\":\"key\",\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_buffer_data(json);
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_lookup_account_result = create_mock_account(1, "testuser", "test@example.com", true, true);
    mock_generate_jwt_result = NULL; // force failure
    mock_api_utils_set_capture_mode(true);

    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_INTERNAL_SERVER_ERROR, mock_api_utils_get_captured_status());
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    UNITY_BEGIN();

    // Success path and credential branches
    RUN_TEST(test_handle_auth_login_success);
    RUN_TEST(test_handle_auth_login_invalid_api_key);
    RUN_TEST(test_handle_auth_login_wrong_password_unauthorized);
    RUN_TEST(test_handle_auth_login_wrong_password_rate_limited);
    RUN_TEST(test_handle_auth_login_roles_fallback_empty);
    RUN_TEST(test_handle_auth_login_jwt_generation_failure);

    return UNITY_END();
}
