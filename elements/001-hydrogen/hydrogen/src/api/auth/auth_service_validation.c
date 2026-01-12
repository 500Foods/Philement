/*
 * Auth Service Validation Functions
 *
 * This file contains all input validation operations including:
 * - Login input validation
 * - Registration input validation
 * - Email validation
 * - Timezone validation
 * - Username validation
 */

// Global includes
#include <src/hydrogen.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>
#include <src/logging/logging.h>
#include <jansson.h>

// Local includes
#include "auth_service.h"
#include "auth_service_validation.h"

/**
 * Validate login input parameters
 */
bool validate_login_input(const char* login_id, const char* password,
                         const char* api_key, const char* tz) {
    // Check for NULL pointers
    if (!login_id || !password || !api_key || !tz) return false;

    // Check minimum lengths
    if (strlen(login_id) < 1 || strlen(login_id) > 255) return false;
    if (strlen(password) < 8 || strlen(password) > 128) return false;
    if (strlen(api_key) < 1 || strlen(api_key) > 255) return false;
    if (strlen(tz) < 1 || strlen(tz) > 50) return false;

    // Validate timezone format (basic check)
    if (!validate_timezone(tz)) return false;

    return true;
}

/**
 * Calculate timezone offset in minutes for a given timezone
 * Returns the offset from UTC/GMT in minutes (negative for timezones west of UTC)
 * Examples: PST = -480, CET = +60, IST = +330
 * Returns 0 if timezone is invalid or UTC
 */
int calculate_timezone_offset(const char* tz) {
    if (!tz || strlen(tz) == 0) return 0;
    
    // Save current timezone
    char* old_tz = getenv("TZ");
    char* saved_tz = old_tz ? strdup(old_tz) : NULL;
    
    // Get a consistent reference time (use current time)
    time_t now = time(NULL);
    
    // Set timezone to the target timezone and get local time
    setenv("TZ", tz, 1);
    tzset();
    struct tm local_tm;
    localtime_r(&now, &local_tm);
    
    // Set timezone to UTC and get UTC time
    setenv("TZ", "UTC", 1);
    tzset();
    struct tm utc_tm;
    gmtime_r(&now, &utc_tm);
    
    // Calculate offset by comparing the hour and minute fields
    // Offset = (local time) - (UTC time) in minutes
    int local_minutes = local_tm.tm_hour * 60 + local_tm.tm_min;
    int utc_minutes = utc_tm.tm_hour * 60 + utc_tm.tm_min;
    
    // Handle day boundary crossing
    int day_diff = local_tm.tm_mday - utc_tm.tm_mday;
    // Adjust for month boundary (e.g., day 31 -> day 1 or day 1 -> day 31)
    if (day_diff > 1) day_diff = -1;  // Wrapped backwards (e.g., 31 -> 1)
    if (day_diff < -1) day_diff = 1;  // Wrapped forwards (e.g., 1 -> 31)
    
    int offset_minutes = local_minutes - utc_minutes + (day_diff * 24 * 60);
    
    // Restore original timezone
    if (saved_tz) {
        setenv("TZ", saved_tz, 1);
        free(saved_tz);
    } else {
        unsetenv("TZ");
    }
    tzset();
    
    log_this(SR_AUTH, "Calculated timezone offset for %s: %+d minutes (%+.1f hours)",
             LOG_LEVEL_DEBUG, 3, tz, offset_minutes, (double)offset_minutes / 60.0);
    
    return offset_minutes;
}

/**
 * Validate timezone format using system timezone database
 * This performs a comprehensive check:
 * 1. Basic character validation
 * 2. Checks against system timezone database by attempting to set it
 * 3. Returns true if timezone is valid
 */
bool validate_timezone(const char* tz) {
    if (!tz || strlen(tz) == 0) return false;

    // Check length constraints
    if (strlen(tz) > 50) return false;

    // Basic validation: allow alphanumeric, /, _, -, +, :, and common timezone patterns
    for (size_t i = 0; i < strlen(tz); i++) {
        char c = tz[i];
        if (!(isalnum(c) || c == '/' || c == '_' || c == '-' || c == '+' || c == ':')) {
            return false;
        }
    }

    // Save current timezone
    char* old_tz = getenv("TZ");
    char* saved_tz = old_tz ? strdup(old_tz) : NULL;
    
    // Try to set the timezone - if it's invalid, tzset will fail silently
    // but we can detect it by checking if the timezone name is set
    setenv("TZ", tz, 1);
    tzset();
    
    // Check if timezone was successfully set by trying to get local time
    time_t now = time(NULL);
    struct tm local_tm;
    struct tm* result = localtime_r(&now, &local_tm);
    
    // Restore original timezone
    if (saved_tz) {
        setenv("TZ", saved_tz, 1);
        free(saved_tz);
    } else {
        unsetenv("TZ");
    }
    tzset();
    
    // If localtime_r failed, timezone was invalid
    if (!result) {
        log_this(SR_AUTH, "Invalid timezone: %s (localtime_r failed)", LOG_LEVEL_DEBUG, 1, tz);
        return false;
    }
    
    // Additional validation: check for common valid patterns
    // This catches most valid timezones even if system validation might be permissive
    bool has_valid_pattern = false;
    
    if (strstr(tz, "America/") == tz || strstr(tz, "Europe/") == tz ||
        strstr(tz, "Asia/") == tz || strstr(tz, "Africa/") == tz ||
        strstr(tz, "Australia/") == tz || strstr(tz, "Pacific/") == tz ||
        strstr(tz, "Atlantic/") == tz || strstr(tz, "Indian/") == tz ||
        strstr(tz, "Arctic/") == tz || strstr(tz, "Antarctica/") == tz ||
        strcmp(tz, "UTC") == 0 || strcmp(tz, "GMT") == 0 ||
        strncmp(tz, "Etc/", 4) == 0) {
        has_valid_pattern = true;
    }
    
    // Allow UTC offsets like UTC+05:00 or +05:00
    if (strlen(tz) >= 3 && (strncmp(tz, "UTC", 3) == 0 || tz[0] == '+' || tz[0] == '-')) {
        has_valid_pattern = true;
    }
    
    if (!has_valid_pattern) {
        log_this(SR_AUTH, "Timezone does not match known patterns: %s", LOG_LEVEL_DEBUG, 1, tz);
        return false;
    }
    
    log_this(SR_AUTH, "Timezone validated successfully: %s", LOG_LEVEL_DEBUG, 1, tz);
    return true;
}

/**
 * Validate registration input parameters
 */
bool validate_registration_input(const char* username, const char* password,
                                const char* email, const char* full_name) {
    // Username: 3-50 chars, alphanumeric + underscore/hyphen
    if (!username || strlen(username) < 3 || strlen(username) > 50) return false;
    if (!is_alphanumeric_underscore_hyphen(username)) return false;

    // Check that password is between 8 and 128 characters
    if (!password || strlen(password) < 8 || strlen(password) > 128) return false;

    // Email: valid format, max 255 chars
    if (!email || strlen(email) > 255 || !is_valid_email(email)) return false;

    // Full name: optional, max 255 chars
    if (full_name && strlen(full_name) > 255) return false;

    return true;
}

/**
 * Check if string contains only alphanumeric, underscore, or hyphen characters
 */
bool is_alphanumeric_underscore_hyphen(const char* str) {
    if (!str) return false;

    for (size_t i = 0; i < strlen(str); i++) {
        char c = str[i];
        if (!(isalnum(c) || c == '_' || c == '-')) {
            return false;
        }
    }

    return true;
}

/**
 * Validate email format
 */
bool is_valid_email(const char* email) {
    if (!email || strlen(email) == 0) return false;

    // Basic email validation: must contain @ and . after @
    const char* at_pos = strchr(email, '@');
    if (!at_pos) return false;

    const char* dot_pos = strchr(at_pos + 1, '.');
    if (!dot_pos) return false;

    // Must have at least one character before @, between @ and ., and after .
    if (at_pos == email) return false; // No local part
    if (dot_pos == at_pos + 1) return false; // No domain part
    if (strlen(dot_pos + 1) == 0) return false; // No TLD

    // Check for valid characters (basic check)
    for (size_t i = 0; i < strlen(email); i++) {
        char c = email[i];
        if (!(isalnum(c) || c == '@' || c == '.' || c == '_' || c == '-' || c == '+')) {
            return false;
        }
    }

    return true;
}

/**
 * Check if IP is in whitelist
 * Uses QueryRef #002 to check APP.Lists #1 (whitelist)
 * Returns true if IP is whitelisted, false otherwise
 */
bool check_ip_whitelist(const char* client_ip, const char* database) {
    if (!client_ip || !database) {
        log_this(SR_AUTH, "Invalid parameters for whitelist check", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Create parameters for QueryRef #002: Get IP Whitelist
    // Use typed parameter format: {"STRING": {"IPADDRESS": "value"}}
    // Parameter name must match SQL placeholder :IPADDRESS
    json_t* params = json_object();
    json_t* string_params = json_object();
    json_object_set_new(string_params, "IPADDRESS", json_string(client_ip));
    json_object_set_new(params, "STRING", string_params);

    // Execute query against specified database
    QueryResult* result = execute_auth_query(2, database, params);
    json_decref(params);

    if (!result) {
        log_this(SR_AUTH, "Failed to check IP whitelist for %s", LOG_LEVEL_ERROR, 1, client_ip);
        return false; // Fail-safe: assume not whitelisted
    }

    // If row_count > 0, IP is in whitelist
    bool is_whitelisted = result->success && result->row_count > 0;

    // Cleanup
    if (result->error_message) free(result->error_message);
    if (result->data_json) free(result->data_json);
    free(result);

    if (is_whitelisted) {
        log_this(SR_AUTH, "IP %s is whitelisted", LOG_LEVEL_DEBUG, 1, client_ip);
    }

    return is_whitelisted;
}

/**
 * Check if IP is in blacklist
 * Uses QueryRef #003 to check APP.Lists #0 (blacklist)
 * Returns true if IP is blacklisted, false otherwise
 */
bool check_ip_blacklist(const char* client_ip, const char* database) {
    if (!client_ip || !database) {
        log_this(SR_AUTH, "Invalid parameters for blacklist check", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Create parameters for QueryRef #003: Get IP Blacklist
    // Use typed parameter format: {"STRING": {"IPADDRESS": "value"}}
    // Parameter name must match SQL placeholder :IPADDRESS
    json_t* params = json_object();
    json_t* string_params = json_object();
    json_object_set_new(string_params, "IPADDRESS", json_string(client_ip));
    json_object_set_new(params, "STRING", string_params);

    // Execute query against specified database
    QueryResult* result = execute_auth_query(3, database, params);
    json_decref(params);

    if (!result) {
        log_this(SR_AUTH, "Failed to check IP blacklist for %s", LOG_LEVEL_ERROR, 1, client_ip);
        return false; // Fail-safe: assume not blacklisted
    }

    // If row_count > 0, IP is in blacklist
    bool is_blacklisted = result->success && result->row_count > 0;

    // Cleanup
    if (result->error_message) free(result->error_message);
    if (result->data_json) free(result->data_json);
    free(result);

    if (is_blacklisted) {
        log_this(SR_AUTH, "IP %s is blacklisted", LOG_LEVEL_ALERT, 1, client_ip);
    }

    return is_blacklisted;
}

/**
 * Handle rate limiting logic and IP blocking
 * Returns true if IP should be blocked (rate limit exceeded)
 * Returns false if allowed to continue
 */
bool handle_rate_limiting(const char* client_ip, int failed_count,
                          bool is_whitelisted, const char* database) {
    const int MAX_ATTEMPTS = 5;
    const int BLOCK_DURATION_MINUTES = 15;

    if (failed_count >= MAX_ATTEMPTS && !is_whitelisted) {
        // Block IP address using QueryRef #007
        block_ip_address(client_ip, BLOCK_DURATION_MINUTES, database);
        
        log_this(SR_AUTH, "IP %s blocked due to too many failed attempts (%d >= %d)",
                LOG_LEVEL_ALERT, 3, client_ip, failed_count, MAX_ATTEMPTS);
        
        return true; // IP blocked
    }

    return false; // No blocking needed
}
