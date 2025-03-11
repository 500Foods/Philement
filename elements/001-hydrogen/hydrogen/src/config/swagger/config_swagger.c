/*
 * Swagger UI Configuration Implementation
 */

#define _GNU_SOURCE  // For strdup

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "config_swagger.h"
#include "../types/config_string.h"
#include "../config.h"  // For VERSION macro

int config_swagger_init(SwaggerConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize basic settings
    config->enabled = 1;  // Enabled by default
    config->prefix = strdup(DEFAULT_SWAGGER_PREFIX);
    config->payload_available = 0;

    if (!config->prefix) {
        config_swagger_cleanup(config);
        return -1;
    }

    // Initialize metadata
    config->metadata.title = strdup(DEFAULT_SWAGGER_TITLE);
    config->metadata.description = strdup(DEFAULT_SWAGGER_DESCRIPTION);
    config->metadata.version = strdup(VERSION);
    
    if (!config->metadata.title || !config->metadata.description || 
        !config->metadata.version) {
        config_swagger_cleanup(config);
        return -1;
    }

    // Initialize contact info with empty values (optional)
    config->metadata.contact.name = NULL;
    config->metadata.contact.email = NULL;
    config->metadata.contact.url = NULL;

    // Initialize license info with empty values (optional)
    config->metadata.license.name = NULL;
    config->metadata.license.url = NULL;

    // Initialize UI options
    config->ui_options.try_it_enabled = 1;
    config->ui_options.always_expanded = 0;
    config->ui_options.display_operation_id = 0;
    config->ui_options.default_models_expand_depth = 1;
    config->ui_options.default_model_expand_depth = 1;
    config->ui_options.show_extensions = 0;
    config->ui_options.show_common_extensions = 0;
    config->ui_options.doc_expansion = strdup(DEFAULT_DOC_EXPANSION);
    config->ui_options.syntax_highlight_theme = strdup(DEFAULT_SYNTAX_HIGHLIGHT_THEME);

    if (!config->ui_options.doc_expansion || 
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

    // Free basic settings
    free(config->prefix);
    
    // Free metadata
    free(config->metadata.title);
    free(config->metadata.description);
    free(config->metadata.version);
    
    // Free contact info
    free(config->metadata.contact.name);
    free(config->metadata.contact.email);
    free(config->metadata.contact.url);
    
    // Free license info
    free(config->metadata.license.name);
    free(config->metadata.license.url);
    
    // Free UI options
    free(config->ui_options.doc_expansion);
    free(config->ui_options.syntax_highlight_theme);

    // Zero out the structure
    memset(config, 0, sizeof(SwaggerConfig));
}

int config_swagger_validate(const SwaggerConfig* config) {
    if (!config) {
        return -1;
    }

    // If Swagger is enabled, validate required fields
    if (config->enabled) {
        if (!config->prefix || 
            !config->metadata.title || 
            !config->metadata.description || 
            !config->metadata.version) {
            return -1;
        }

        // Validate contact info if partially provided
        if ((config->metadata.contact.name && !config->metadata.contact.email) || 
            (!config->metadata.contact.name && config->metadata.contact.email)) {
            return -1;
        }

        // Validate license info if partially provided
        if ((config->metadata.license.name && !config->metadata.license.url) || 
            (!config->metadata.license.name && config->metadata.license.url)) {
            return -1;
        }

        // Validate UI options
        if (!config->ui_options.doc_expansion || 
            !config->ui_options.syntax_highlight_theme) {
            return -1;
        }
    }

    return 0;
}