/*
 * Swagger Configuration Implementation
 *
 * Implements the configuration handlers for the Swagger UI subsystem,
 * including JSON parsing, environment variable handling, and validation.
 */

#include <stdlib.h>
#include <string.h>
#include "config_swagger.h"
#include "config.h"
#include "config_utils.h"
#include "../logging/logging.h"

// Helper function for safe string duplication
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = strdup(str);
    if (!dup) {
        log_this("Config-Swagger", "Memory allocation failed", LOG_LEVEL_ERROR);
    }
    return dup;
}

// Load Swagger configuration from JSON
bool load_swagger_config(json_t* root, AppConfig* config) {
    if (!config) {
        return false;
    }

    // Initialize swagger config if not present
    if (!config->swagger) {
        config->swagger = malloc(sizeof(SwaggerConfig));
        if (!config->swagger) {
            log_this("Config-Swagger", "Failed to allocate Swagger configuration", LOG_LEVEL_ERROR);
            return false;
        }
        if (config_swagger_init(config->swagger) != 0) {
            free(config->swagger);
            config->swagger = NULL;
            return false;
        }
    }

    // Process all config items in sequence
    bool success = true;

    // Process Swagger section if present
    success = PROCESS_SECTION(root, "Swagger");
    if (success) {
        // Basic settings
        success = success && PROCESS_BOOL(root, config->swagger, enabled, "Swagger.Enabled", "Swagger");
        success = success && PROCESS_STRING(root, config->swagger, prefix, "Swagger.Prefix", "Swagger");

        // Metadata section
        json_t* metadata = json_object_get(json_object_get(root, "Swagger"), "Metadata");
        if (json_is_object(metadata)) {
            log_config_item("Metadata", "Configured", false, "Swagger");

            success = success && PROCESS_STRING(metadata, &config->swagger->metadata, title, "Title", "Swagger.Metadata");
            success = success && PROCESS_STRING(metadata, &config->swagger->metadata, description, "Description", "Swagger.Metadata");
            success = success && PROCESS_STRING(metadata, &config->swagger->metadata, version, "Version", "Swagger.Metadata");

            // Contact information
            json_t* contact = json_object_get(metadata, "Contact");
            if (json_is_object(contact)) {
                log_config_item("Contact", "Configured", false, "Swagger");
                success = success && PROCESS_STRING(contact, &config->swagger->metadata.contact, name, "Name", "Swagger.Metadata.Contact");
                success = success && PROCESS_STRING(contact, &config->swagger->metadata.contact, email, "Email", "Swagger.Metadata.Contact");
                success = success && PROCESS_STRING(contact, &config->swagger->metadata.contact, url, "URL", "Swagger.Metadata.Contact");
            }

            // License information
            json_t* license = json_object_get(metadata, "License");
            if (json_is_object(license)) {
                log_config_item("License", "Configured", false, "Swagger");
                success = success && PROCESS_STRING(license, &config->swagger->metadata.license, name, "Name", "Swagger.Metadata.License");
                success = success && PROCESS_STRING(license, &config->swagger->metadata.license, url, "URL", "Swagger.Metadata.License");
            }
        }

        // UI Options section
        json_t* ui_options = json_object_get(json_object_get(root, "Swagger"), "UIOptions");
        if (json_is_object(ui_options)) {
            log_config_item("UIOptions", "Configured", false, "Swagger");

            success = success && PROCESS_BOOL(ui_options, &config->swagger->ui_options, try_it_enabled, "TryItEnabled", "Swagger.UIOptions");
            success = success && PROCESS_BOOL(ui_options, &config->swagger->ui_options, always_expanded, "AlwaysExpanded", "Swagger.UIOptions");
            success = success && PROCESS_BOOL(ui_options, &config->swagger->ui_options, display_operation_id, "DisplayOperationId", "Swagger.UIOptions");
            success = success && PROCESS_INT(ui_options, &config->swagger->ui_options, default_models_expand_depth, "DefaultModelsExpandDepth", "Swagger.UIOptions");
            success = success && PROCESS_INT(ui_options, &config->swagger->ui_options, default_model_expand_depth, "DefaultModelExpandDepth", "Swagger.UIOptions");
            success = success && PROCESS_BOOL(ui_options, &config->swagger->ui_options, show_extensions, "ShowExtensions", "Swagger.UIOptions");
            success = success && PROCESS_BOOL(ui_options, &config->swagger->ui_options, show_common_extensions, "ShowCommonExtensions", "Swagger.UIOptions");
            success = success && PROCESS_STRING(ui_options, &config->swagger->ui_options, doc_expansion, "DocExpansion", "Swagger.UIOptions");
            success = success && PROCESS_STRING(ui_options, &config->swagger->ui_options, syntax_highlight_theme, "SyntaxHighlightTheme", "Swagger.UIOptions");
        }
    }

    // Validate the configuration if loaded successfully
    if (success) {
        success = (config_swagger_validate(config->swagger) == 0);
    }

    // Clean up on failure
    if (!success && config->swagger) {
        config_swagger_cleanup(config->swagger);
        free(config->swagger);
        config->swagger = NULL;
    }

    return success;
}

// Initialize Swagger configuration with default values
int config_swagger_init(SwaggerConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize main configuration
    config->enabled = true;
    config->prefix = strdup_safe("/apidocs");
    config->payload_available = false;

    // Initialize metadata
    config->metadata.title = strdup_safe("Hydrogen API");
    config->metadata.description = strdup_safe("Hydrogen 3D Printer Control Server API");
    config->metadata.version = strdup_safe(VERSION);
    config->metadata.contact.name = NULL;
    config->metadata.contact.email = NULL;
    config->metadata.contact.url = NULL;
    config->metadata.license.name = NULL;
    config->metadata.license.url = NULL;

    // Initialize UI options
    config->ui_options.try_it_enabled = true;
    config->ui_options.always_expanded = false;
    config->ui_options.display_operation_id = false;
    config->ui_options.default_models_expand_depth = 1;
    config->ui_options.default_model_expand_depth = 1;
    config->ui_options.show_extensions = false;
    config->ui_options.show_common_extensions = true;
    config->ui_options.doc_expansion = strdup_safe("list");
    config->ui_options.syntax_highlight_theme = strdup_safe("agate");

    // Check if any string allocation failed
    if (!config->prefix || !config->metadata.title || !config->metadata.description ||
        !config->metadata.version || !config->ui_options.doc_expansion ||
        !config->ui_options.syntax_highlight_theme) {
        config_swagger_cleanup(config);
        return -1;
    }

    return 0;
}

// Free resources allocated for Swagger configuration
void config_swagger_cleanup(SwaggerConfig* config) {
    if (!config) {
        return;
    }

    // Free prefix
    free(config->prefix);

    // Free metadata strings
    free(config->metadata.title);
    free(config->metadata.description);
    free(config->metadata.version);
    free(config->metadata.contact.name);
    free(config->metadata.contact.email);
    free(config->metadata.contact.url);
    free(config->metadata.license.name);
    free(config->metadata.license.url);

    // Free UI options strings
    free(config->ui_options.doc_expansion);
    free(config->ui_options.syntax_highlight_theme);

    // Zero out the structure
    memset(config, 0, sizeof(SwaggerConfig));
}

// Validate Swagger configuration values
int config_swagger_validate(const SwaggerConfig* config) {
    if (!config) {
        log_this("Config-Swagger", "Swagger config pointer is NULL", LOG_LEVEL_ERROR);
        return -1;
    }

    // Skip validation if Swagger is disabled
    if (!config->enabled) {
        return 0;
    }

    // Validate prefix
    if (!config->prefix || strlen(config->prefix) < MIN_PREFIX_LENGTH ||
        strlen(config->prefix) > MAX_PREFIX_LENGTH || config->prefix[0] != '/') {
        log_this("Config-Swagger", "Invalid Swagger prefix", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate required metadata
    if (!config->metadata.title || strlen(config->metadata.title) < MIN_TITLE_LENGTH ||
        strlen(config->metadata.title) > MAX_TITLE_LENGTH) {
        log_this("Config-Swagger", "Invalid Swagger title", LOG_LEVEL_ERROR);
        return -1;
    }

    if (!config->metadata.version || strlen(config->metadata.version) < MIN_VERSION_LENGTH ||
        strlen(config->metadata.version) > MAX_VERSION_LENGTH) {
        log_this("Config-Swagger", "Invalid Swagger version", LOG_LEVEL_ERROR);
        return -1;
    }

    if (config->metadata.description &&
        strlen(config->metadata.description) > MAX_DESCRIPTION_LENGTH) {
        log_this("Config-Swagger", "Swagger description too long", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate UI options
    if (config->ui_options.default_models_expand_depth < MIN_EXPAND_DEPTH ||
        config->ui_options.default_models_expand_depth > MAX_EXPAND_DEPTH) {
        log_this("Config-Swagger", "Invalid models expand depth", LOG_LEVEL_ERROR);
        return -1;
    }

    if (config->ui_options.default_model_expand_depth < MIN_EXPAND_DEPTH ||
        config->ui_options.default_model_expand_depth > MAX_EXPAND_DEPTH) {
        log_this("Config-Swagger", "Invalid model expand depth", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate doc expansion value
    if (config->ui_options.doc_expansion) {
        const char* exp = config->ui_options.doc_expansion;
        if (strcmp(exp, "list") != 0 && strcmp(exp, "full") != 0 && strcmp(exp, "none") != 0) {
            log_this("Config-Swagger", "Invalid doc expansion value", LOG_LEVEL_ERROR);
            return -1;
        }
    }

    return 0;
}