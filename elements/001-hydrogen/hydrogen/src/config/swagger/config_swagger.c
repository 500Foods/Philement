/*
 * Swagger UI Configuration Implementation
 */

#include <stdlib.h>
#include <string.h>
#include "../config.h"
#include "config_swagger.h"
#include "../config_utils.h"
#include "../../logging/logging.h"

static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = strdup(str);
    if (!dup) {
        log_this("Config", "Memory allocation failed", LOG_LEVEL_ERROR);
    }
    return dup;
}

int config_swagger_init(SwaggerConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize main configuration
    config->enabled = 1;  // Enable by default
    config->prefix = strdup_safe(DEFAULT_SWAGGER_PREFIX);
    config->payload_available = 0;

    // Initialize metadata
    config->metadata.title = strdup_safe(DEFAULT_SWAGGER_TITLE);
    config->metadata.description = strdup_safe(DEFAULT_SWAGGER_DESCRIPTION);
    config->metadata.version = strdup_safe(VERSION);
    config->metadata.contact.name = NULL;
    config->metadata.contact.email = NULL;
    config->metadata.contact.url = NULL;
    config->metadata.license.name = NULL;
    config->metadata.license.url = NULL;

    // Initialize UI options
    config->ui_options.try_it_enabled = 1;
    config->ui_options.always_expanded = 0;
    config->ui_options.display_operation_id = 0;
    config->ui_options.default_models_expand_depth = 1;
    config->ui_options.default_model_expand_depth = 1;
    config->ui_options.show_extensions = 0;
    config->ui_options.show_common_extensions = 1;
    config->ui_options.doc_expansion = strdup_safe(DEFAULT_DOC_EXPANSION);
    config->ui_options.syntax_highlight_theme = strdup_safe(DEFAULT_SYNTAX_HIGHLIGHT_THEME);

    // Check if any string allocation failed
    if (!config->prefix || !config->metadata.title || !config->metadata.description ||
        !config->metadata.version || !config->ui_options.doc_expansion ||
        !config->ui_options.syntax_highlight_theme) {
        config_swagger_cleanup(config);
        return -1;
    }

    return 0;
}

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

int config_swagger_validate(const SwaggerConfig* config) {
    if (!config) {
        return -1;
    }

    // Skip validation if Swagger is disabled
    if (!config->enabled) {
        return 0;
    }

    // Check required fields
    if (!config->prefix || strlen(config->prefix) == 0) {
        log_this("Config", "Swagger prefix is required", LOG_LEVEL_ERROR);
        return -1;
    }

    if (!config->metadata.title || strlen(config->metadata.title) == 0) {
        log_this("Config", "Swagger title is required", LOG_LEVEL_ERROR);
        return -1;
    }

    if (!config->metadata.version || strlen(config->metadata.version) == 0) {
        log_this("Config", "Swagger version is required", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate UI options
    if (config->ui_options.default_models_expand_depth < 0 || 
        config->ui_options.default_models_expand_depth > 10) {
        log_this("Config", "Invalid models expand depth", LOG_LEVEL_ERROR);
        return -1;
    }

    if (config->ui_options.default_model_expand_depth < 0 || 
        config->ui_options.default_model_expand_depth > 10) {
        log_this("Config", "Invalid model expand depth", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate doc expansion value
    if (config->ui_options.doc_expansion) {
        const char* exp = config->ui_options.doc_expansion;
        if (strcmp(exp, "list") != 0 && 
            strcmp(exp, "full") != 0 && 
            strcmp(exp, "none") != 0) {
            log_this("Config", "Invalid doc expansion value", LOG_LEVEL_ERROR);
            return -1;
        }
    }

    return 0;
}