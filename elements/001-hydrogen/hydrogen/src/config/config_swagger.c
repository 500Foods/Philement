/*
 * Swagger Configuration Implementation
 *
 * Implements the configuration handlers for the Swagger UI subsystem,
 * using the standard configuration patterns and PROCESS_ macros.
 */

 // Global includes 
#include <src/hydrogen.h>

// Local includes
#include "config_swagger.h"

// Load Swagger configuration with defaults and environment handling
bool load_swagger_config(json_t* root, AppConfig* config) {
    SwaggerConfig* swagger = &config->swagger;

    // Initialize with defaults
    *swagger = (SwaggerConfig){
        .enabled = true,
        .prefix = strdup("/apidocs"),
        .payload_available = false,

        // NEW: WebRoot defaults
        .webroot = strdup("PAYLOAD:/swagger"),
        .cors_origin = strdup("*"),  // NEW: Allow all origins by default
        .metadata = {
            .title = strdup("Hydrogen API"),
            .description = strdup("Hydrogen Server API"),
            .version = strdup(VERSION),
            .contact = {0},  // Initialize to NULL
            .license = {0}   // Initialize to NULL
        },
        .ui_options = {
            .try_it_enabled = true,
            .always_expanded = false,
            .display_operation_id = false,
            .default_models_expand_depth = 1,
            .default_model_expand_depth = 1,
            .show_extensions = false,
            .show_common_extensions = true,
            .doc_expansion = strdup("list"),
            .syntax_highlight_theme = strdup("agate")
        }
    };

    // Process all config items in sequence
    bool success = true;

    // Process main section and basic settings
    success = PROCESS_SECTION(root, "Swagger");
    success = success && PROCESS_BOOL(root, swagger, enabled, "Swagger.Enabled", "Swagger");
    success = success && PROCESS_STRING(root, swagger, prefix, "Swagger.Prefix", "Swagger");

    // NEW: Process WebRoot and CORS fields
    success = success && PROCESS_STRING(root, swagger, webroot, "Swagger.WebRoot", "Swagger");
    success = success && PROCESS_STRING(root, swagger, cors_origin, "Swagger.CORSOrigin", "Swagger");

    // Process metadata section
    success = success && PROCESS_SECTION(root, "Swagger.Metadata");
    success = success && PROCESS_STRING(root, &swagger->metadata, title, "Swagger.Metadata.Title", "Swagger");
    success = success && PROCESS_STRING(root, &swagger->metadata, description, "Swagger.Metadata.Description", "Swagger");
    success = success && PROCESS_STRING(root, &swagger->metadata, version, "Swagger.Metadata.Version", "Swagger");

    // Process contact subsection
    success = success && PROCESS_SECTION(root, "Swagger.Metadata.Contact");
    success = success && PROCESS_STRING(root, &swagger->metadata.contact, name, "Swagger.Metadata.Contact.Name", "Swagger");
    success = success && PROCESS_STRING(root, &swagger->metadata.contact, email, "Swagger.Metadata.Contact.Email", "Swagger");
    success = success && PROCESS_STRING(root, &swagger->metadata.contact, url, "Swagger.Metadata.Contact.URL", "Swagger");

    // Process license subsection
    success = success && PROCESS_SECTION(root, "Swagger.Metadata.License");
    success = success && PROCESS_STRING(root, &swagger->metadata.license, name, "Swagger.Metadata.License.Name", "Swagger");
    success = success && PROCESS_STRING(root, &swagger->metadata.license, url, "Swagger.Metadata.License.URL", "Swagger");

    // Process UI options section
    success = success && PROCESS_SECTION(root, "Swagger.UIOptions");
    success = success && PROCESS_BOOL(root, &swagger->ui_options, try_it_enabled, "Swagger.UIOptions.TryItEnabled", "Swagger");
    success = success && PROCESS_BOOL(root, &swagger->ui_options, always_expanded, "Swagger.UIOptions.AlwaysExpanded", "Swagger");
    success = success && PROCESS_BOOL(root, &swagger->ui_options, display_operation_id, "Swagger.UIOptions.DisplayOperationId", "Swagger");
    success = success && PROCESS_INT(root, &swagger->ui_options, default_models_expand_depth, "Swagger.UIOptions.DefaultModelsExpandDepth", "Swagger");
    success = success && PROCESS_INT(root, &swagger->ui_options, default_model_expand_depth, "Swagger.UIOptions.DefaultModelExpandDepth", "Swagger");
    success = success && PROCESS_BOOL(root, &swagger->ui_options, show_extensions, "Swagger.UIOptions.ShowExtensions", "Swagger");
    success = success && PROCESS_BOOL(root, &swagger->ui_options, show_common_extensions, "Swagger.UIOptions.ShowCommonExtensions", "Swagger");
    success = success && PROCESS_STRING(root, &swagger->ui_options, doc_expansion, "Swagger.UIOptions.DocExpansion", "Swagger");
    success = success && PROCESS_STRING(root, &swagger->ui_options, syntax_highlight_theme, "Swagger.UIOptions.SyntaxHighlightTheme", "Swagger");

    if (!success) {
        cleanup_swagger_config(swagger);
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

    // NEW: Dump WebRoot and CORS fields
    DUMP_STRING("―― WebRoot", config->webroot);
    DUMP_STRING("―― CORS Origin", config->cors_origin);

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

    // NEW: Free WebRoot and CORS strings
    free(config->webroot);
    free(config->cors_origin);

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
