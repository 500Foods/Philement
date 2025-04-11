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

// Validation limits
#define MIN_PREFIX_LENGTH 1
#define MAX_PREFIX_LENGTH 64
#define MIN_TITLE_LENGTH 1
#define MAX_TITLE_LENGTH 128
#define MIN_VERSION_LENGTH 1
#define MAX_VERSION_LENGTH 32
#define MIN_DESCRIPTION_LENGTH 0
#define MAX_DESCRIPTION_LENGTH 1024
#define MIN_EXPAND_DEPTH 0
#define MAX_EXPAND_DEPTH 10

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
 * Load Swagger configuration from JSON
 *
 * This function loads the Swagger configuration from the provided JSON root,
 * applying any environment variable overrides and using secure defaults
 * where values are not specified.
 *
 * @param root JSON root object containing configuration
 * @param config Pointer to AppConfig structure to update
 * @return true if successful, false on error
 */
bool load_swagger_config(json_t* root, AppConfig* config);

/*
 * Initialize Swagger configuration with default values
 *
 * This function initializes a new SwaggerConfig structure with default
 * values that provide a secure and usable baseline configuration.
 *
 * @param config Pointer to SwaggerConfig structure to initialize
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 * - If memory allocation fails for any string field
 */
int config_swagger_init(SwaggerConfig* config);

/*
 * Free resources allocated for Swagger configuration
 *
 * This function frees all resources allocated by config_swagger_init.
 * It safely handles NULL pointers and partial initialization.
 * After cleanup, the structure is zeroed to prevent use-after-free.
 *
 * @param config Pointer to SwaggerConfig structure to cleanup
 */
void config_swagger_cleanup(SwaggerConfig* config);

/*
 * Validate Swagger configuration values
 *
 * This function performs comprehensive validation of the configuration:
 * - Verifies required fields are present and valid
 * - Validates string lengths
 * - Checks numeric ranges
 * - Validates enumerated values
 *
 * @param config Pointer to SwaggerConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If enabled but required fields are missing
 * - If string lengths exceed limits
 * - If numeric values are out of range
 * - If enumerated values are invalid
 */
int config_swagger_validate(const SwaggerConfig* config);

#endif /* CONFIG_SWAGGER_H */