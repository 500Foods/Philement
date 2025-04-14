/*
 * Swagger Configuration Implementation
 *
 * Implements the configuration handlers for the Swagger UI subsystem,
 * using the standard configuration patterns and PROCESS_ macros.
 */

#include <stdlib.h>
#include <string.h>
#include "config_swagger.h"
#include "config.h"
#include "config_utils.h"
#include "../logging/logging.h"

// Load Swagger configuration with defaults and environment handling
bool load_swagger_config(json_t* root, AppConfig* config) {


    // Process Swagger section
    bool success = PROCESS_SECTION(root, "Swagger");
    if (!success) {
        return false;
    }

    // Set defaults and process overrides for basic settings
    config->swagger.enabled = true;  // Default enabled
    config->swagger.prefix = strdup("/apidocs");  // Default prefix
    config->swagger.payload_available = false;  // Default no payload

    success = success && PROCESS_BOOL(root, &config->swagger, enabled, "Swagger.Enabled", "Swagger");
    success = success && PROCESS_STRING(root, &config->swagger, prefix, "Swagger.Prefix", "Swagger");

    // Process metadata section
    success = success && PROCESS_SECTION(root, "Swagger.Metadata");
    
    // Set defaults and process overrides for metadata
    config->swagger.metadata.title = strdup("Hydrogen API");
    config->swagger.metadata.description = strdup("Hydrogen 3D Printer Control Server API");
    config->swagger.metadata.version = strdup(VERSION);
    config->swagger.metadata.contact.name = NULL;
    config->swagger.metadata.contact.email = NULL;
    config->swagger.metadata.contact.url = NULL;
    config->swagger.metadata.license.name = NULL;
    config->swagger.metadata.license.url = NULL;

    success = success && PROCESS_STRING(root, &config->swagger.metadata, title, "Swagger.Metadata.Title", "Swagger");
    success = success && PROCESS_STRING(root, &config->swagger.metadata, description, "Swagger.Metadata.Description", "Swagger");
    success = success && PROCESS_STRING(root, &config->swagger.metadata, version, "Swagger.Metadata.Version", "Swagger");

    // Process contact subsection
    success = success && PROCESS_SECTION(root, "Swagger.Metadata.Contact");
    success = success && PROCESS_STRING(root, &config->swagger.metadata.contact, name, "Swagger.Metadata.Contact.Name", "Swagger");
    success = success && PROCESS_STRING(root, &config->swagger.metadata.contact, email, "Swagger.Metadata.Contact.Email", "Swagger");
    success = success && PROCESS_STRING(root, &config->swagger.metadata.contact, url, "Swagger.Metadata.Contact.URL", "Swagger");

    // Process license subsection
    success = success && PROCESS_SECTION(root, "Swagger.Metadata.License");
    success = success && PROCESS_STRING(root, &config->swagger.metadata.license, name, "Swagger.Metadata.License.Name", "Swagger");
    success = success && PROCESS_STRING(root, &config->swagger.metadata.license, url, "Swagger.Metadata.License.URL", "Swagger");

    // Process UI options section
    success = success && PROCESS_SECTION(root, "Swagger.UIOptions");

    // Set defaults and process overrides for UI options
    config->swagger.ui_options.try_it_enabled = true;
    config->swagger.ui_options.always_expanded = false;
    config->swagger.ui_options.display_operation_id = false;
    config->swagger.ui_options.default_models_expand_depth = 1;
    config->swagger.ui_options.default_model_expand_depth = 1;
    config->swagger.ui_options.show_extensions = false;
    config->swagger.ui_options.show_common_extensions = true;
    config->swagger.ui_options.doc_expansion = strdup("list");
    config->swagger.ui_options.syntax_highlight_theme = strdup("agate");

    success = success && PROCESS_BOOL(root, &config->swagger.ui_options, try_it_enabled, "Swagger.UIOptions.TryItEnabled", "Swagger");
    success = success && PROCESS_BOOL(root, &config->swagger.ui_options, always_expanded, "Swagger.UIOptions.AlwaysExpanded", "Swagger");
    success = success && PROCESS_BOOL(root, &config->swagger.ui_options, display_operation_id, "Swagger.UIOptions.DisplayOperationId", "Swagger");
    success = success && PROCESS_INT(root, &config->swagger.ui_options, default_models_expand_depth, "Swagger.UIOptions.DefaultModelsExpandDepth", "Swagger");
    success = success && PROCESS_INT(root, &config->swagger.ui_options, default_model_expand_depth, "Swagger.UIOptions.DefaultModelExpandDepth", "Swagger");
    success = success && PROCESS_BOOL(root, &config->swagger.ui_options, show_extensions, "Swagger.UIOptions.ShowExtensions", "Swagger");
    success = success && PROCESS_BOOL(root, &config->swagger.ui_options, show_common_extensions, "Swagger.UIOptions.ShowCommonExtensions", "Swagger");
    success = success && PROCESS_STRING(root, &config->swagger.ui_options, doc_expansion, "Swagger.UIOptions.DocExpansion", "Swagger");
    success = success && PROCESS_STRING(root, &config->swagger.ui_options, syntax_highlight_theme, "Swagger.UIOptions.SyntaxHighlightTheme", "Swagger");

    if (!success) {
        cleanup_swagger_config(&config->swagger);
    }

    return success;
}

// Dump current Swagger configuration state
void dump_swagger_config(const SwaggerConfig* config) {
    if (!config) {
        return;
    }

    DUMP_BOOL("―― Enabled", config->enabled);
    DUMP_STRING("―― Prefix", config->prefix);
    DUMP_BOOL("―― Payload", config->payload_available);

    DUMP_TEXT("――", "Metadata");
    DUMP_STRING2("――――", "Title", config->metadata.title);
    DUMP_STRING2("――――", "Description", config->metadata.description);
    DUMP_STRING2("――――", "Version", config->metadata.version);

    DUMP_TEXT("――――", "Contact");
    DUMP_STRING2("――――――", "Name", config->metadata.contact.name);
    DUMP_STRING2("――――――", "Email", config->metadata.contact.email);
    DUMP_STRING2("――――――", "URL", config->metadata.contact.url);

    DUMP_TEXT("――――", "License");
    DUMP_STRING2("――――――", "Name", config->metadata.license.name);
    DUMP_STRING2("――――――", "URL", config->metadata.license.url);

    DUMP_TEXT("――", "UIOptions");
    DUMP_BOOL2("――――", "TryItEnabled", config->ui_options.try_it_enabled);
    DUMP_BOOL2("――――", "AlwaysExpanded", config->ui_options.always_expanded);
    DUMP_BOOL2("――――", "DisplayOperationID", config->ui_options.display_operation_id);
    DUMP_INT("―――― DefaultModelsExpandDepth", config->ui_options.default_models_expand_depth);
    DUMP_INT("―――― DefaultModelExpandDepth", config->ui_options.default_model_expand_depth);
    DUMP_BOOL2("――――", "ShowExtensions", config->ui_options.show_extensions);
    DUMP_BOOL2("――――", "ShowCommon Extensions", config->ui_options.show_common_extensions);
    DUMP_STRING2("――――", "DocExpansion", config->ui_options.doc_expansion);
    DUMP_STRING2("――――", "SyntaxHighlight Theme", config->ui_options.syntax_highlight_theme);
}

// Clean up Swagger configuration resources
void cleanup_swagger_config(SwaggerConfig* config) {
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