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
void test_handle_auth_login_success(void);
void test_handle_auth_login_invalid_api_key(void);
void test_handle_auth_login_wrong_password_unauthorized(void);
void test_handle_auth_login_wrong_password_rate_limited(void);
void test_handle_auth_login_roles_fallback_empty(void);
void test_handle_auth_login_jwt_generation_failure(void);

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
static bool mock_check_license_expiry_result = true;
static char* mock_api_get_client_ip_result = NULL;
static bool mock_check_ip_whitelist_result = false;
static bool mock_check_ip_blacklist_result = false;
static int mock_check_failed_attempts_result = 0;
static bool mock_handle_rate_limiting_result = false;
static account_info_t* mock_lookup_account_result = NULL;
static int mock_lookup_account_code = 0;
static int mock_verify_api_key_code = 1;
static bool mock_verify_password_and_status_result = true;
static char* mock_generate_jwt_result = NULL;
static char* mock_compute_token_hash_result = NULL;
static char* mock_auth_roles_from_database_result = NULL;

// Capturing state for api_send_json_response
static bool mock_capture_mode = false;
static json_t *mock_captured_response = NULL;
static unsigned int mock_captured_status = 0;
static int mock_send_json_response_call_count = 0;

// Mock implementations with weak linkage to override real implementations
__attribute__((weak))
bool validate_login_input(const char* login_id, const char* password,
                         const char* api_key, const char* tz) {
    (void)login_id; (void)password; (void)api_key; (void)tz;
    return mock_validate_login_input_result;
}

__attribute__((weak))
int verify_api_key_code(const char* api_key, const char* database, system_info_t* sys_info) {
    (void)api_key; (void)database;
    if (mock_verify_api_key_code == 1 && sys_info) {
        sys_info->system_id = 1;
        sys_info->app_id = 1;
        sys_info->license_expiry = time(NULL) + 86400;
    }
    return mock_verify_api_key_code;
}

__attribute__((weak))
bool verify_api_key(const char* api_key, const char* database, system_info_t* sys_info) {
    return verify_api_key_code(api_key, database, sys_info) == 1;
}

__attribute__((weak))
void auth_query_begin_deadline(int budget_seconds) {
    (void)budget_seconds;
}

__attribute__((weak))
void auth_query_end_deadline(void) {
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
int lookup_account_code(const char* login_id, const char* database, account_info_t** out) {
    (void)login_id; (void)database;
    if (out) {
        *out = mock_lookup_account_result;
    }
    if (mock_lookup_account_result) {
        return 1;
    }
    return mock_lookup_account_code;
}

__attribute__((weak))
account_info_t* lookup_account(const char* login_id, const char* database) {
    account_info_t* account = NULL;
    if (lookup_account_code(login_id, database, &account) == 1) {
        return account;
    }
    return NULL;
}

__attribute__((weak))
char* auth_roles_from_database(int account_id, const char* database) {
    (void)account_id; (void)database;
    if (mock_auth_roles_from_database_result) {
        return strdup(mock_auth_roles_from_database_result);
    }
    return NULL;
}

__attribute__((weak))
int verify_password_and_status_code(const char* password, int account_id,
                                   const char* database, account_info_t* account) {
    (void)password; (void)account_id; (void)database; (void)account;
    return mock_verify_password_and_status_result ? 1 : 0;
}

__attribute__((weak))
bool verify_password_and_status(const char* password, int account_id,
                               const char* database, account_info_t* account) {
    return verify_password_and_status_code(password, account_id, database, account) == 1;
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
void store_jwt(int account_id, const char* jwt_hash, time_t expires_at, int system_id, int app_id, const char* database, const char* client_ip) {
    (void)account_id; (void)jwt_hash; (void)expires_at; (void)system_id; (void)app_id; (void)database; (void)client_ip;
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

enum MHD_Result api_send_error_and_cleanup(
    struct MHD_Connection *connection,
    void **con_cls,
    const char *error_message,
    unsigned int http_status
) {
    (void)connection; (void)con_cls; (void)error_message; (void)http_status;
    return MHD_YES;
}

void api_free_post_buffer(void **con_cls) {
    (void)con_cls;
}

json_t *api_parse_json_body(ApiPostBuffer *buffer) {
    if (!buffer || !buffer->data || buffer->size == 0) {
        return NULL;
    }
    return json_loads(buffer->data, 0, NULL);
}

// Capturing version: records the response json + status so success-path
// tests can assert on the HTTP 200 body instead of only the return value.
enum MHD_Result api_send_json_response(struct MHD_Connection *connection,
                                     json_t *json_obj,
                                     unsigned int status_code) {
    (void)connection;
    mock_send_json_response_call_count++;
    mock_captured_status = status_code;

    if (mock_capture_mode && json_obj) {
        // Take ownership (do not free) so the test can inspect fields.
        mock_captured_response = json_obj;
    } else if (json_obj) {
        json_decref(json_obj);
    }

    return MHD_YES;
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
    mock_lookup_account_code = 0;
    mock_verify_api_key_code = 1;
    mock_verify_password_and_status_result = true;

    free(mock_generate_jwt_result);
    mock_generate_jwt_result = NULL;

    free(mock_compute_token_hash_result);
    mock_compute_token_hash_result = NULL;

    free(mock_auth_roles_from_database_result);
    mock_auth_roles_from_database_result = NULL;

    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;

    if (mock_api_buffer) {
        free(mock_api_buffer->data);
        free(mock_api_buffer);
        mock_api_buffer = NULL;
    }

    mock_capture_mode = false;
    if (mock_captured_response) {
        json_decref(mock_captured_response);
        mock_captured_response = NULL;
    }
    mock_captured_status = 0;
    mock_send_json_response_call_count = 0;

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
    mock_api_buffer = create_mock_buffer(json, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_lookup_account_result = create_mock_account(1, "testuser", "test@example.com", true, true);
    mock_generate_jwt_result = strdup("test_jwt_token");
    mock_compute_token_hash_result = strdup("test_hash_value");
    mock_auth_roles_from_database_result = strdup("1,3");
    mock_capture_mode = true;

    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(1, mock_send_json_response_call_count);
    TEST_ASSERT_EQUAL(MHD_HTTP_OK, mock_captured_status);

    TEST_ASSERT_NOT_NULL(mock_captured_response);
    TEST_ASSERT_TRUE(json_boolean_value(json_object_get(mock_captured_response, "success")));
    TEST_ASSERT_EQUAL_STRING("test_jwt_token",
                             json_string_value(json_object_get(mock_captured_response, "token")));
    TEST_ASSERT_EQUAL_INT(1,
                          json_integer_value(json_object_get(mock_captured_response, "user_id")));
    TEST_ASSERT_EQUAL_STRING("testuser",
                             json_string_value(json_object_get(mock_captured_response, "username")));
    TEST_ASSERT_EQUAL_STRING("test@example.com",
                             json_string_value(json_object_get(mock_captured_response, "email")));
    TEST_ASSERT_EQUAL_STRING("1,3",
                             json_string_value(json_object_get(mock_captured_response, "roles")));

    free_account_info(mock_lookup_account_result);
}

// API key present but invalid (code == 0): must return 401 "Invalid API key".
void test_handle_auth_login_invalid_api_key(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    const char* json = "{\"login_id\":\"testuser\",\"password\":\"Password123!\","
                       "\"api_key\":\"bad-key\",\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_buffer = create_mock_buffer(json, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_verify_api_key_code = 0; // invalid key
    mock_capture_mode = true;

    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_UNAUTHORIZED, mock_captured_status);
    TEST_ASSERT_NOT_NULL(mock_captured_response);
    TEST_ASSERT_EQUAL_STRING("Invalid API key",
                             json_string_value(json_object_get(mock_captured_response, "error")));
}

// Wrong password (verify fails, no rate limit): 401 "Invalid credentials".
void test_handle_auth_login_wrong_password_unauthorized(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    const char* json = "{\"login_id\":\"testuser\",\"password\":\"wrong\",\"api_key\":\"key\","
                       "\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_buffer = create_mock_buffer(json, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_lookup_account_result = create_mock_account(1, "testuser", "test@example.com", true, true);
    mock_verify_password_and_status_result = false; // wrong password
    mock_handle_rate_limiting_result = false;       // not blocked
    mock_capture_mode = true;

    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_UNAUTHORIZED, mock_captured_status);
    TEST_ASSERT_EQUAL_STRING("Invalid credentials",
                             json_string_value(json_object_get(mock_captured_response, "error")));

    free_account_info(mock_lookup_account_result);
}

// Wrong password with rate limiting triggered: 429 with retry_after.
void test_handle_auth_login_wrong_password_rate_limited(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    const char* json = "{\"login_id\":\"testuser\",\"password\":\"wrong\",\"api_key\":\"key\","
                       "\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_buffer = create_mock_buffer(json, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_lookup_account_result = create_mock_account(1, "testuser", "test@example.com", true, true);
    mock_verify_password_and_status_result = false;
    mock_handle_rate_limiting_result = true;  // blocked
    mock_check_failed_attempts_result = 5;
    mock_capture_mode = true;

    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_TOO_MANY_REQUESTS, mock_captured_status);
    TEST_ASSERT_EQUAL_STRING("Too many failed attempts",
                             json_string_value(json_object_get(mock_captured_response, "error")));

    free_account_info(mock_lookup_account_result);
}

// auth_roles_from_database returns NULL: login.c must fall back to strdup("")
// so the response still carries an (empty) roles field.
void test_handle_auth_login_roles_fallback_empty(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    const char* json = "{\"login_id\":\"testuser\",\"password\":\"Password123!\","
                       "\"api_key\":\"key\",\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_buffer = create_mock_buffer(json, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_lookup_account_result = create_mock_account(1, "testuser", "test@example.com", true, true);
    mock_generate_jwt_result = strdup("test_jwt_token");
    mock_compute_token_hash_result = strdup("test_hash_value");
    // auth_roles_from_database_result left NULL -> fallback path
    mock_capture_mode = true;

    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_OK, mock_captured_status);
    TEST_ASSERT_EQUAL_STRING("",
                             json_string_value(json_object_get(mock_captured_response, "roles")));

    free_account_info(mock_lookup_account_result);
}

// JWT generation returns NULL: 500 "Failed to generate authentication token".
void test_handle_auth_login_jwt_generation_failure(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    void *con_cls = NULL;
    size_t upload_size = 0;

    const char* json = "{\"login_id\":\"testuser\",\"password\":\"Password123!\","
                       "\"api_key\":\"key\",\"tz\":\"America/Vancouver\",\"database\":\"db\"}";
    mock_api_buffer = create_mock_buffer(json, 'P');
    mock_api_buffer_post_data_result = API_BUFFER_COMPLETE;
    mock_api_get_client_ip_result = strdup("192.168.1.1");
    mock_lookup_account_result = create_mock_account(1, "testuser", "test@example.com", true, true);
    mock_generate_jwt_result = NULL; // force failure
    mock_capture_mode = true;

    enum MHD_Result result = handle_auth_login_request(
        NULL, mock_connection, "/api/auth/login", "POST", "HTTP/1.1",
        NULL, &upload_size, &con_cls
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_INTERNAL_SERVER_ERROR, mock_captured_status);

    free_account_info(mock_lookup_account_result);
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
