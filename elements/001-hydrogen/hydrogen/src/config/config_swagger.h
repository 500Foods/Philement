/*
 * Swagger Configuration
 *
 * Defines the configuration structure and handlers for the Swagger UI subsystem.
 * This includes settings for API documentation, metadata, and UI customization.
 */

#ifndef CONFIG_SWAGGER_H
#define CONFIG_SWAGGER_H

#include <stdbool.h>
#include <jansson.h>
#include "config_forward.h"  // For AppConfig forward declaration

// Swagger UI configuration structure
typedef struct SwaggerConfig {
    bool enabled;
    char* prefix;
    int payload_available;  // Track if swagger payload was loaded
    
    struct {
        char* title;
        char* description;
        char* version;
        
        struct {
            char* name;
            char* email;
            char* url;
        } contact;
        
        struct {
            char* name;
            char* url;
        } license;
    } metadata;
    
    struct {
        bool try_it_enabled;
        bool always_expanded;
        bool display_operation_id;
        int default_models_expand_depth;
        int default_model_expand_depth;
        bool show_extensions;
        bool show_common_extensions;
        char* doc_expansion;
        char* syntax_highlight_theme;
    } ui_options;
} SwaggerConfig;

/*
 * Load Swagger configuration with defaults and environment handling
 *
 * Sets secure defaults for all fields and processes any overrides from:
 * - JSON configuration
 * - Environment variables
 * 
 * Uses PROCESS_ macros exclusively for all operations.
 *
 * @param root JSON root object containing configuration
 * @param config Pointer to AppConfig structure to update
 * @return true if successful, false on error
 */
bool load_swagger_config(json_t* root, AppConfig* config);

/*
 * Dump current Swagger configuration state
 * 
 * Outputs the current state of the Swagger configuration using
 * consistent formatting and indentation.
 *
 * @param config Pointer to SwaggerConfig structure
 */
void dump_swagger_config(const SwaggerConfig* config);

/*
 * Clean up Swagger configuration resources
 *
 * Frees all allocated memory in the Swagger configuration structure
 * and zeros it to prevent use-after-free. Safely handles NULL pointers
 * and partially initialized structures.
 *
 * @param config Pointer to SwaggerConfig structure to cleanup
 */
void cleanup_swagger_config(SwaggerConfig* config);

#endif /* CONFIG_SWAGGER_H */