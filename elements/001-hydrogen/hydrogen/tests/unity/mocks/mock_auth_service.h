/*
 * Mock Auth Service functions for unit testing
 *
 * Provides mock implementations of auth_service.h functions that are
 * redirected via #define directives in source files compiled with
 * USE_MOCK_API_UTILS (e.g. register.c). These mocks allow the register
 * endpoint to be linked and tested without the real auth service
 * database calls.
 *
 * Enable with USE_MOCK_API_UTILS (already defined for auth sources).
 */

#ifndef MOCK_AUTH_SERVICE_H
#define MOCK_AUTH_SERVICE_H

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <jansson.h>

#include <src/hydrogen.h>
#include <src/api/auth/auth_service.h>

#ifdef USE_MOCK_API_UTILS
#define validate_registration_input mock_validate_registration_input
#define verify_api_key mock_verify_api_key
#define check_license_expiry mock_check_license_expiry
#define check_username_availability mock_check_username_availability
#define create_account_record mock_create_account_record
#define compute_password_hash mock_compute_password_hash
#define execute_auth_query mock_execute_auth_query
#define free_query_result mock_free_query_result
#endif

// Mock implementations
bool mock_validate_registration_input(const char* username, const char* password,
                                      const char* email, const char* full_name);
bool mock_verify_api_key(const char* api_key, const char* database, system_info_t* sys_info);
bool mock_check_license_expiry(time_t license_expiry);
bool mock_check_username_availability(const char* username, const char* database);
int mock_create_account_record(const char* username, const char* email,
                               const char* password_hash, const char* full_name,
                               const char* database);
char* mock_compute_password_hash(const char* password, int account_id);
QueryResult* mock_execute_auth_query(int query_ref, const char* database, json_t* params);
void mock_free_query_result(QueryResult* result);

// Control functions
void mock_auth_service_reset_all(void);
void mock_auth_service_set_validate_registration_input_result(bool result);
void mock_auth_service_set_verify_api_key_result(bool result);
void mock_auth_service_set_verify_api_key_sys_info(system_info_t sys_info);
void mock_auth_service_set_check_license_expiry_result(bool result);
void mock_auth_service_set_check_username_availability_result(bool result);
void mock_auth_service_set_create_account_record_result(int result);
void mock_auth_service_set_compute_password_hash_result(const char* result);
void mock_auth_service_set_execute_auth_query_success(bool success);
void mock_auth_service_set_execute_auth_query_error(const char* error);

#endif /* MOCK_AUTH_SERVICE_H */
