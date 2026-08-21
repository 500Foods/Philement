/*
 * API Configuration Implementation
 *
 * Implements the configuration handlers for the API subsystem,
 * including JSON parsing and environment variable handling.
 * Note: Validation moved to launch readiness checks
 */

 // Global includes 
#include <src/hydrogen.h>

// Local includes
#include "config_api.h"

// Load API configuration from JSON
bool load_api_config(json_t* root, AppConfig* config) {
    bool success = true;
    APIConfig* api_config = &config->api;

    // Initialize with defaults
    memset(api_config, 0, sizeof(APIConfig));
    api_config->enabled = true;
    api_config->prefix = strdup("/api");
    api_config->jwt_secret = strdup("${env.JWT_SECRET}");
    api_config->cors_origin = strdup("*");

    // Process all config items in sequence
    success = PROCESS_SECTION(root, "API");
    success = success && PROCESS_BOOL(root, api_config, enabled, "API.Enabled", "API");
    success = success && PROCESS_STRING(root, api_config, prefix, "API.Prefix", "API");
    success = success && PROCESS_SENSITIVE(root, api_config, jwt_secret, "API.JWTSecret", "API");
    success = success && PROCESS_STRING(root, api_config, cors_origin, "API.CORSOrigin", "API");
    success = success && process_api_headers_config(root, api_config);

    // Clean up and return on failure
    if (!success) {
        cleanup_api_config(api_config);
        return false;
    }

    return success;
}

bool process_api_headers_config(json_t* root, APIConfig* api_config) {
    if (!root || !api_config) return true;

    json_t* headers_array = json_object_get(json_object_get(root, "API"), "Headers");
    if (!headers_array || !json_is_array(headers_array)) {
        return true;
    }

    size_t num_rules = json_array_size(headers_array);
    if (num_rules == 0) {
        return true;
    }

    api_config->headers = calloc(num_rules, sizeof(HeaderRule));
    if (!api_config->headers) {
        log_this(SR_CONFIG, "Failed to allocate memory for API header rules", LOG_LEVEL_ERROR, 0);
        return false;
    }

    bool success = true;

    for (size_t i = 0; i < num_rules; i++) {
        json_t* rule_array = json_array_get(headers_array, i);

        if (!json_is_array(rule_array) || json_array_size(rule_array) != 3) {
            log_this(SR_CONFIG, "Invalid API header rule format at index %zu", LOG_LEVEL_ERROR, 1, i);
            success = false;
            continue;
        }

        json_t* pattern_json = json_array_get(rule_array, 0);
        json_t* name_json = json_array_get(rule_array, 1);
        json_t* value_json = json_array_get(rule_array, 2);

        if (!json_is_string(pattern_json) || !json_is_string(name_json) || !json_is_string(value_json)) {
            log_this(SR_CONFIG, "Invalid API header rule elements at index %zu", LOG_LEVEL_ERROR, 1, i);
            success = false;
            continue;
        }

        api_config->headers[i].pattern = strdup(json_string_value(pattern_json));
        api_config->headers[i].header_name = strdup(json_string_value(name_json));
        api_config->headers[i].header_value = strdup(json_string_value(value_json));

        if (!api_config->headers[i].pattern || !api_config->headers[i].header_name ||
            !api_config->headers[i].header_value) {
            log_this(SR_CONFIG, "Memory allocation failed for API header rule at index %zu", LOG_LEVEL_ERROR, 1, i);
            success = false;
            continue;
        }

        api_config->headers_count++;

        log_this(SR_CONFIG, "――――― Headers[%zu]: [%s, %s, %s]", LOG_LEVEL_DEBUG, 4,
                i, api_config->headers[i].pattern, api_config->headers[i].header_name,
                api_config->headers[i].header_value);
    }

    return success;
}

// Clean up API configuration
void cleanup_api_config(APIConfig* config) {
    if (!config) {
        return;
    }

    // Free allocated strings
    free(config->prefix);
    free(config->jwt_secret);
    free(config->cors_origin);

    if (config->headers) {
        for (size_t i = 0; i < config->headers_count; i++) {
            free(config->headers[i].pattern);
            free(config->headers[i].header_name);
            free(config->headers[i].header_value);
        }
        free(config->headers);
    }

    // Zero out the structure
    memset(config, 0, sizeof(APIConfig));
}

// Dump API configuration for debugging
void dump_api_config(const APIConfig* config) {
    if (!config) {
        log_this(SR_CONFIG, "Cannot dump NULL API config", LOG_LEVEL_ERROR, 0);
        return;
    }

    // Dump API settings
    DUMP_BOOL("―― Enabled", config->enabled);
    DUMP_STRING("―― Prefix", config->prefix);
    DUMP_SECRET("―― JWTSecret", config->jwt_secret);
    DUMP_STRING("―― CORS Origin", config->cors_origin);

    if (config->headers_count > 0) {
        DUMP_TEXT("――", "Custom Headers");
        for (size_t i = 0; i < config->headers_count; i++) {
            char header_info[256];
            snprintf(header_info, sizeof(header_info), "[%s, %s, %s]",
                    config->headers[i].pattern,
                    config->headers[i].header_name,
                    config->headers[i].header_value);
            DUMP_TEXT("―――――", header_info);
        }
    }
}
