/*
 * Template for configuration JSON parsing include files;
 * 
 * CRITICAL NOTE: These .inc files are meant to be included directly into C files
 * and should IGNORE ALL LINTING ERRORS. A separate, specialized linting system
 * is used to validate these files. Standard C linting tools will report errors
 * that should be ignored.
 * 
 * IMPORTANT: This is not a standalone C file!
 * This file demonstrates the standard patterns for handling configuration values
 * in the Hydrogen system. Each section handler should follow these patterns to
 * ensure consistent behavior and logging.
 * 
 * Configuration Value Handling Rules:
 * 
 * 1. Environment Variables (${env.VAR} syntax):
 *    - Check if value is a string in ${env.VAR} format
 *    - Use get_config_string_with_env() for processing
 *    - Log with Config-Env subsystem
 *    Example from Server section:
 *      json_t* server_name = json_object_get(server, "ServerName");
 *      config->server.server_name = get_config_string_with_env(
 *          "ServerName", server_name, DEFAULT_SERVER_NAME);
 * 
 * 2. Direct Values:
 *    - Use appropriate get_config_* function (int, bool, etc.)
 *    - Log with Config subsystem
 *    Example from Server section:
 *      config->server.startup_delay = get_config_int(
 *          startup_delay, DEFAULT_STARTUP_DELAY);
 * 
 * 3. Sensitive Values:
 *    - Use get_config_string_with_env() for processing
 *    - Values are automatically truncated in logs
 *    - Identified by key names containing: key, token, pass, etc.
 *    Example from Server section:
 *      config->server.payload_key = get_config_string_with_env(
 *          "PayloadKey", payload_key, "Missing Key");
 * 
 * Logging Format Standards:
 * 
 * 1. Environment Variables:
 *    When set:
 *    ― Key: $VAR: value                    (Config-Env subsystem)
 *    When not set:
 *    ― Key: $VAR: not set, using default * (Config-Env subsystem)
 * 
 * 2. Direct Values:
 *    ― Key: value                          (Config subsystem)
 *    With default:
 *    ― Key: value *                        (Config subsystem)
 * 
 * 3. Sensitive Values:
 *    ― Key: $VAR: LS0tL...                 (Config-Env subsystem)
 * 
 * Error Handling Requirements:
 * 
 * 1. Memory Management:
 *    - Free all allocated strings on error
 *    - Free in reverse order of allocation
 *    - Return NULL to indicate failure
 * 
 * 2. Validation:
 *    - Check allocation success
 *    - Validate numeric ranges if applicable
 *    - Log specific error messages
 * 
 * Required context (must be available in load_config):
 * - json_t* root: The root JSON object
 * - AppConfig* config: The configuration structure
 * - All helper functions (get_config_string_with_env, etc.)
 * - All required headers must be included in config.c
 */

// Section Configuration
json_t* section = json_object_get(root, "SectionName");
if (json_is_object(section)) {
    log_config_section_header("SectionName");

    // String value that may be an environment variable
    json_t* name = json_object_get(section, "Name");
    config->section.name = get_config_string_with_env("Name", name, DEFAULT_SECTION_NAME);
    if (!config->section.name) {
        log_this("Config", "Failed to allocate section name", LOG_LEVEL_ERROR);
        return NULL;
    }

    // Direct integer value with units
    json_t* timeout = json_object_get(section, "TimeoutMs");
    if (json_is_string(timeout)) {
        // Handle as environment variable if string
        char* timeout_str = get_config_string_with_env("TimeoutMs", timeout, "1000");
        config->section.timeout_ms = atoi(timeout_str);
        free(timeout_str);
    } else {
        config->section.timeout_ms = get_config_int(timeout, DEFAULT_SECTION_TIMEOUT);
        log_config_section_item("TimeoutMs", "%d", LOG_LEVEL_STATE, !timeout, 0, "ms", "ms", "Config",
            config->section.timeout_ms);
    }

    // Boolean value that may be an environment variable
    json_t* enabled = json_object_get(section, "Enabled");
    if (json_is_string(enabled)) {
        char* enabled_str = get_config_string_with_env("Enabled", enabled, "true");
        config->section.enabled = (strcasecmp(enabled_str, "true") == 0);
        free(enabled_str);
    } else {
        config->section.enabled = get_config_bool(enabled, DEFAULT_SECTION_ENABLED);
        log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 0, NULL, NULL, "Config",
            config->section.enabled ? "true" : "false");
    }

    // Sensitive value (like keys or tokens)
    json_t* secret = json_object_get(section, "SecretKey");
    config->section.secret_key = get_config_string_with_env("SecretKey", secret, "Missing Key");
    if (!config->section.secret_key) {
        log_this("Config", "Failed to allocate secret key", LOG_LEVEL_ERROR);
        free(config->section.name);
        return NULL;
    }

    // Subsection handling with increased indent
    json_t* subsection = json_object_get(section, "SubSection");
    if (json_is_object(subsection)) {
        log_config_section_item("SubSection", "", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config");
        
        // Process subsection properties with increased indent
        json_t* sub_prop = json_object_get(subsection, "Property");
        if (json_is_string(sub_prop)) {
            char* prop_str = get_config_string_with_env("Property", sub_prop, "default");
            config->section.sub.property = atoi(prop_str);
            free(prop_str);
        } else {
            config->section.sub.property = get_config_int(sub_prop, DEFAULT_SUB_PROPERTY);
            log_config_section_item("Property", "%d", LOG_LEVEL_STATE, !sub_prop, 1, NULL, NULL, "Config",
                config->section.sub.property);
        }
    }

    // Array handling
    json_t* items = json_object_get(section, "Items");
    if (json_is_array(items)) {
        size_t count = json_array_size(items);
        log_config_section_item("Items", "%zu configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config", count);
        
        for (size_t i = 0; i < count; i++) {
            json_t* item = json_array_get(items, i);
            if (json_is_string(item)) {
                char item_key[32];
                snprintf(item_key, sizeof(item_key), "Item[%zu]", i);
                char* item_value = get_config_string_with_env(item_key, item, "default");
                // Process item_value...
                free(item_value);
            }
        }
    }
} else {
    // Handle missing section
    log_config_section_header("SectionName");
    log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL, "Config");
    
    // Initialize with defaults
    config->section.name = strdup(DEFAULT_SECTION_NAME);
    config->section.timeout_ms = DEFAULT_SECTION_TIMEOUT;
    config->section.enabled = DEFAULT_SECTION_ENABLED;
    config->section.secret_key = strdup("Missing Key");

    // Validate default allocation
    if (!config->section.name || !config->section.secret_key) {
        log_this("Config", "Failed to allocate default section configuration strings", LOG_LEVEL_ERROR);
        free(config->section.name);
        free(config->section.secret_key);
        return NULL;
    }
}