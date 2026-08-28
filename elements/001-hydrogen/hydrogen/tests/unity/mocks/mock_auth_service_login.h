/*
  * Mock Auth Service Login functions for unit testing
  *
  * Provides #define redirects so login.c's calls to auth service functions
  * are rewritten to mock_ prefixed names at compile time, matching the
  * __attribute__((weak)) mock implementations defined in the login test files.
  * This avoids multiple definition conflicts with the real implementations
  * compiled into libhydrogen_unity.a when USE_MOCK_API_UTILS is defined.
  */

#ifndef MOCK_AUTH_SERVICE_LOGIN_H
#define MOCK_AUTH_SERVICE_LOGIN_H

#include <stdbool.h>
#include <time.h>
#include <jansson.h>

#include <src/api/auth/auth_service.h>

#ifdef USE_MOCK_API_UTILS

/* api_get_client_ip is in api_utils.h; redirect to mock to avoid pulling
   api_utils.o from the archive, which would override the test's weak mock. */
#define api_get_client_ip mock_api_get_client_ip

/* Auth service functions called by login.c */
#define validate_login_input mock_validate_login_input
#define verify_api_key_code mock_verify_api_key_code
#define auth_query_begin_deadline mock_auth_query_begin_deadline
#define auth_query_end_deadline mock_auth_query_end_deadline
#define check_license_expiry mock_check_license_expiry
#define check_ip_whitelist mock_check_ip_whitelist
#define check_ip_blacklist mock_check_ip_blacklist
#define log_login_attempt mock_log_login_attempt
#define check_failed_attempts mock_check_failed_attempts
#define handle_rate_limiting mock_handle_rate_limiting
#define lookup_account_code mock_lookup_account_code
#define lookup_account mock_lookup_account
#define verify_password_and_status_code mock_verify_password_and_status_code
#define verify_password_and_status mock_verify_password_and_status
#define auth_roles_from_database mock_auth_roles_from_database
#define generate_jwt mock_generate_jwt
#define compute_token_hash mock_compute_token_hash
#define store_jwt mock_store_jwt
#define free_account_info mock_free_account_info

/* Declarations for the mock_ prefixed functions */
bool mock_validate_login_input(const char* login_id, const char* password,
                               const char* api_key, const char* tz);
int mock_verify_api_key_code(const char* api_key, const char* database, system_info_t* sys_info);
void mock_auth_query_begin_deadline(int budget_seconds);
void mock_auth_query_end_deadline(void);
bool mock_check_license_expiry(time_t license_expiry);
char* mock_api_get_client_ip(struct MHD_Connection *connection);
bool mock_check_ip_whitelist(const char* client_ip, const char* database);
bool mock_check_ip_blacklist(const char* client_ip, const char* database);
void mock_log_login_attempt(const char* login_id, const char* client_ip,
                            const char* user_agent, time_t timestamp, const char* database);
int mock_check_failed_attempts(const char* login_id, const char* client_ip,
                               time_t window_start, const char* database);
bool mock_handle_rate_limiting(const char* client_ip, int failed_count,
                               bool is_whitelisted, const char* database);
int mock_lookup_account_code(const char* login_id, const char* database, account_info_t** out);
account_info_t* mock_lookup_account(const char* login_id, const char* database);
int mock_verify_password_and_status_code(const char* password, int account_id,
                                         const char* database, account_info_t* account);
bool mock_verify_password_and_status(const char* password, int account_id,
                                     const char* database, account_info_t* account);
char* mock_auth_roles_from_database(int account_id, const char* database);
char* mock_generate_jwt(account_info_t* account, system_info_t* system,
                        const char* client_ip, const char* tz, const char* database, time_t issued_at);
char* mock_compute_token_hash(const char* token);
void mock_store_jwt(int account_id, const char* jwt_hash, time_t expires_at,
                    int system_id, int app_id, const char* database, const char* client_ip);
void mock_free_account_info(account_info_t* account);

#endif /* USE_MOCK_API_UTILS */

#endif /* MOCK_AUTH_SERVICE_LOGIN_H */
