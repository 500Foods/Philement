/*
 * Swagger UI Configuration
 *
 * Defines the configuration structure and defaults for the Swagger UI subsystem.
 * This includes settings for API documentation and UI customization.
 */

#ifndef CONFIG_SWAGGER_H
#define CONFIG_SWAGGER_H

// Default values for Swagger UI
#define DEFAULT_SWAGGER_PREFIX "/swagger"
#define DEFAULT_SWAGGER_TITLE "Hydrogen API"
#define DEFAULT_SWAGGER_DESCRIPTION "Hydrogen 3D Printer Control Server API"
#define DEFAULT_DOC_EXPANSION "list"
#define DEFAULT_SYNTAX_HIGHLIGHT_THEME "agate"

// Swagger UI configuration structure
typedef struct {
    int enabled;
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
        int try_it_enabled;
        int always_expanded;
        int display_operation_id;
        int default_models_expand_depth;
        int default_model_expand_depth;
        int show_extensions;
        int show_common_extensions;
        char* doc_expansion;
        char* syntax_highlight_theme;
    } ui_options;
} SwaggerConfig;

/*
 * Initialize Swagger configuration with default values
 *
 * @param config Pointer to SwaggerConfig structure to initialize
 * @return 0 on success, -1 on failure
 */
int config_swagger_init(SwaggerConfig* config);

/*
 * Free resources allocated for Swagger configuration
 *
 * @param config Pointer to SwaggerConfig structure to cleanup
 */
void config_swagger_cleanup(SwaggerConfig* config);

/*
 * Validate Swagger configuration values
 *
 * @param config Pointer to SwaggerConfig structure to validate
 * @return 0 if valid, -1 if invalid
 */
int config_swagger_validate(const SwaggerConfig* config);

#endif /* CONFIG_SWAGGER_H */