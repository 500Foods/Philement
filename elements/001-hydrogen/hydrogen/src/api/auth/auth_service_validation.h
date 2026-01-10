/*
 * Auth Service Validation Functions Header
 *
 * Internal header for validation-related operations
 */

#ifndef HYDROGEN_AUTH_SERVICE_VALIDATION_H
#define HYDROGEN_AUTH_SERVICE_VALIDATION_H

#include <stdbool.h>

// Input validation functions
bool validate_login_input(const char* login_id, const char* password,
                         const char* api_key, const char* tz);
bool validate_registration_input(const char* username, const char* password,
                                const char* email, const char* full_name);
bool validate_timezone(const char* tz);

// String validation helpers
bool is_alphanumeric_underscore_hyphen(const char* str);
bool is_valid_email(const char* email);

// IP filtering
bool check_ip_whitelist(const char* client_ip, const char* database);
bool check_ip_blacklist(const char* client_ip, const char* database);

// Rate limiting
bool handle_rate_limiting(const char* client_ip, int failed_count,
                         bool is_whitelisted, const char* database);

#endif /* HYDROGEN_AUTH_SERVICE_VALIDATION_H */
