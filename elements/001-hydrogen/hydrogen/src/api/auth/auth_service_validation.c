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
#include <src/logging/logging.h>

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
 * Validate timezone format
 */
bool validate_timezone(const char* tz) {
    if (!tz || strlen(tz) == 0) return false;

    // Basic validation: allow alphanumeric, /, _, -, +, and common timezone patterns
    for (size_t i = 0; i < strlen(tz); i++) {
        char c = tz[i];
        if (!(isalnum(c) || c == '/' || c == '_' || c == '-' || c == '+')) {
            return false;
        }
    }

    // Check for common timezone prefixes or exact matches
    if (strstr(tz, "America/") == tz || strstr(tz, "Europe/") == tz ||
        strstr(tz, "Asia/") == tz || strstr(tz, "Africa/") == tz ||
        strstr(tz, "Australia/") == tz || strstr(tz, "Pacific/") == tz ||
        strcmp(tz, "UTC") == 0 || strcmp(tz, "GMT") == 0) {
        return true;
    }

    // Allow UTC offsets like UTC+05:00, but basic check
    if (strlen(tz) >= 3 && strncmp(tz, "UTC", 3) == 0) {
        return true;
    }

    return false;
}

/**
 * Validate registration input parameters
 */
bool validate_registration_input(const char* username, const char* password,
                                const char* email, const char* full_name) {
    // Username: 3-50 chars, alphanumeric + underscore/hyphen
    if (!username || strlen(username) < 3 || strlen(username) > 50) return false;
    if (!is_alphanumeric_underscore_hyphen(username)) return false;

    // Password: 8-128 chars
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
 */
bool check_ip_whitelist(const char* client_ip) {
    if (!client_ip) return false;

    // TODO: Load whitelist from config->api->ip_whitelist array
    // For now, no whitelist configured
    (void)client_ip; // Suppress unused parameter warning

    return false;
}

/**
 * Check if IP is in blacklist
 */
bool check_ip_blacklist(const char* client_ip) {
    if (!client_ip) return false;

    // TODO: Load blacklist from config->api->ip_blacklist array
    // For now, no blacklist configured
    (void)client_ip; // Suppress unused parameter warning

    return false;
}

/**
 * Handle rate limiting logic and IP blocking
 */
bool handle_rate_limiting(const char* client_ip, int failed_count,
                          bool is_whitelisted) {
    const int MAX_ATTEMPTS = 5;
    const int BLOCK_DURATION_MINUTES = 15;

    if (failed_count >= MAX_ATTEMPTS && !is_whitelisted) {
        // TODO: Execute QueryRef #007: Block IP Address Temporarily
        // Parameters: { "ip": client_ip, "duration_minutes": BLOCK_DURATION_MINUTES }
        
        (void)BLOCK_DURATION_MINUTES; // Suppress unused variable warning until DB integration
        
        log_this("AUTH", "IP %s blocked due to too many failed attempts (%d)",
                LOG_LEVEL_ALERT, 2, client_ip, failed_count);
        
        return true; // IP blocked
    }

    return false; // No blocking needed
}
