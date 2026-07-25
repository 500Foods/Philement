/*
 * Mock Auth Service functions for unit testing
 * See mock_auth_service.h for details.
 */

#include "mock_auth_service.h"
#include <stdlib.h>
#include <string.h>

// Static state
static bool g_validate_registration_input_result = true;
static bool g_verify_api_key_result = true;
static system_info_t g_verify_api_key_sys_info = {0};
static bool g_check_license_expiry_result = true;
static bool g_check_username_availability_result = true;
static int g_create_account_record_result = 1;
static char* g_compute_password_hash_result = NULL;
static bool g_execute_auth_query_success = true;
static char* g_execute_auth_query_error = NULL;

// Mock implementations - weak so test files can override with custom behavior
__attribute__((weak))
bool mock_validate_registration_input(const char* username, const char* password,
                                      const char* email, const char* full_name) {
    (void)username; (void)password; (void)email; (void)full_name;
    return g_validate_registration_input_result;
}

__attribute__((weak))
bool mock_verify_api_key(const char* api_key, const char* database, system_info_t* sys_info) {
    (void)api_key; (void)database;
    if (g_verify_api_key_result && sys_info) {
        *sys_info = g_verify_api_key_sys_info;
    }
    return g_verify_api_key_result;
}

__attribute__((weak))
bool mock_check_license_expiry(time_t license_expiry) {
    (void)license_expiry;
    return g_check_license_expiry_result;
}

__attribute__((weak))
bool mock_check_username_availability(const char* username, const char* database) {
    (void)username; (void)database;
    return g_check_username_availability_result;
}

__attribute__((weak))
int mock_create_account_record(const char* username, const char* email,
                               const char* password_hash, const char* full_name,
                               const char* database) {
    (void)username; (void)email; (void)password_hash; (void)full_name; (void)database;
    return g_create_account_record_result;
}

__attribute__((weak))
char* mock_compute_password_hash(const char* password, int account_id) {
    (void)password; (void)account_id;
    if (g_compute_password_hash_result) {
        return strdup(g_compute_password_hash_result);
    }
    return NULL;
}

__attribute__((weak))
QueryResult* mock_execute_auth_query(int query_ref, const char* database, json_t* params) {
    (void)query_ref; (void)database; (void)params;
    QueryResult* result = calloc(1, sizeof(QueryResult));
    if (!result) return NULL;
    result->success = g_execute_auth_query_success;
    if (!g_execute_auth_query_success && g_execute_auth_query_error) {
        result->error_message = strdup(g_execute_auth_query_error);
    }
    return result;
}

__attribute__((weak))
void mock_free_query_result(QueryResult* result) {
    if (!result) return;
    free(result->data_json);
    free(result->error_message);
    free(result);
}

void mock_auth_service_reset_all(void) {
    g_validate_registration_input_result = true;
    memset(&g_verify_api_key_sys_info, 0, sizeof(system_info_t));
    g_verify_api_key_result = true;
    g_check_license_expiry_result = true;
    g_check_username_availability_result = true;
    g_create_account_record_result = 1;
    free(g_compute_password_hash_result);
    g_compute_password_hash_result = NULL;
    g_execute_auth_query_success = true;
    free(g_execute_auth_query_error);
    g_execute_auth_query_error = NULL;
}

void mock_auth_service_set_validate_registration_input_result(bool result) {
    g_validate_registration_input_result = result;
}

void mock_auth_service_set_verify_api_key_result(bool result) {
    g_verify_api_key_result = result;
}

void mock_auth_service_set_verify_api_key_sys_info(system_info_t sys_info) {
    g_verify_api_key_sys_info = sys_info;
}

void mock_auth_service_set_check_license_expiry_result(bool result) {
    g_check_license_expiry_result = result;
}

void mock_auth_service_set_check_username_availability_result(bool result) {
    g_check_username_availability_result = result;
}

void mock_auth_service_set_create_account_record_result(int result) {
    g_create_account_record_result = result;
}

void mock_auth_service_set_compute_password_hash_result(const char* result) {
    free(g_compute_password_hash_result);
    g_compute_password_hash_result = result ? strdup(result) : NULL;
}

void mock_auth_service_set_execute_auth_query_success(bool success) {
    g_execute_auth_query_success = success;
}

void mock_auth_service_set_execute_auth_query_error(const char* error) {
    free(g_execute_auth_query_error);
    g_execute_auth_query_error = error ? strdup(error) : NULL;
}
