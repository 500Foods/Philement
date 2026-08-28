/*
 * Default weak stubs for login.c mock redirects (USE_MOCK_API_UTILS).
 * Login Unity tests override these with strong definitions.
 */

#include <src/hydrogen.h>
#define USE_MOCK_API_UTILS
#include "mock_auth_service_login.h"
#include <src/api/auth/auth_service.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <microhttpd.h>

__attribute__((weak))
bool mock_validate_login_input(const char* login_id, const char* password,
                               const char* api_key, const char* tz) {
    (void)login_id; (void)password; (void)api_key; (void)tz;
    return true;
}

__attribute__((weak))
int mock_verify_api_key_code(const char* api_key, const char* database, system_info_t* sys_info) {
    (void)api_key; (void)database;
    if (sys_info) {
        sys_info->system_id = 1;
        sys_info->app_id = 1;
        sys_info->license_expiry = time(NULL) + 86400;
    }
    return 1;
}

__attribute__((weak))
void mock_auth_query_begin_deadline(int budget_seconds) {
    (void)budget_seconds;
}

__attribute__((weak))
void mock_auth_query_end_deadline(void) {
}

__attribute__((weak))
char* mock_api_get_client_ip(struct MHD_Connection *connection) {
    (void)connection;
    return NULL;
}

__attribute__((weak))
bool mock_check_ip_whitelist(const char* client_ip, const char* database) {
    (void)client_ip; (void)database;
    return false;
}

__attribute__((weak))
bool mock_check_ip_blacklist(const char* client_ip, const char* database) {
    (void)client_ip; (void)database;
    return false;
}

__attribute__((weak))
void mock_log_login_attempt(const char* login_id, const char* client_ip,
                            const char* user_agent, time_t timestamp, const char* database) {
    (void)login_id; (void)client_ip; (void)user_agent; (void)timestamp; (void)database;
}

__attribute__((weak))
int mock_check_failed_attempts(const char* login_id, const char* client_ip,
                               time_t window_start, const char* database) {
    (void)login_id; (void)client_ip; (void)window_start; (void)database;
    return 0;
}

__attribute__((weak))
bool mock_handle_rate_limiting(const char* client_ip, int failed_count,
                               bool is_whitelisted, const char* database) {
    (void)client_ip; (void)failed_count; (void)is_whitelisted; (void)database;
    return false;
}

__attribute__((weak))
int mock_lookup_account_code(const char* login_id, const char* database, account_info_t** out) {
    (void)login_id; (void)database;
    if (out) {
        *out = NULL;
    }
    return 0;
}

__attribute__((weak))
account_info_t* mock_lookup_account(const char* login_id, const char* database) {
    (void)login_id; (void)database;
    return NULL;
}

__attribute__((weak))
int mock_verify_password_and_status_code(const char* password, int account_id,
                                         const char* database, account_info_t* account) {
    (void)password; (void)account_id; (void)database; (void)account;
    return 1;
}

__attribute__((weak))
bool mock_verify_password_and_status(const char* password, int account_id,
                                     const char* database, account_info_t* account) {
    (void)password; (void)account_id; (void)database; (void)account;
    return true;
}

__attribute__((weak))
char* mock_auth_roles_from_database(int account_id, const char* database) {
    (void)account_id; (void)database;
    return NULL;
}

__attribute__((weak))
char* mock_generate_jwt(account_info_t* account, system_info_t* system,
                        const char* client_ip, const char* tz, const char* database, time_t issued_at) {
    (void)account; (void)system; (void)client_ip; (void)tz; (void)database; (void)issued_at;
    return NULL;
}

__attribute__((weak))
char* mock_compute_token_hash(const char* token) {
    (void)token;
    return NULL;
}

__attribute__((weak))
void mock_store_jwt(int account_id, const char* jwt_hash, time_t expires_at,
                    int system_id, int app_id, const char* database, const char* client_ip) {
    (void)account_id; (void)jwt_hash; (void)expires_at; (void)system_id; (void)app_id;
    (void)database; (void)client_ip;
}

__attribute__((weak))
void mock_free_account_info(account_info_t* account) {
    if (!account) return;
    free(account->username);
    free(account->email);
    free(account->roles);
    free(account);
}
