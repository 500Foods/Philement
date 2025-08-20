/*
 * Launch Swagger Subsystem
 * 
 * This module handles the initialization of the Swagger subsystem.
 * It provides functions for checking readiness and launching the Swagger UI.
 * 
 * Dependencies:
 * - API subsystem must be initialized and ready
 * - Payload subsystem must be initialized and ready (for serving Swagger files)
 */

// Global includes 
#include "../hydrogen.h"

// Local includes
#include "launch.h"
#include "../config/config_swagger.h"
#include "../swagger/swagger.h"
#include "../webserver/web_server_core.h"  // For WebServerEndpoint

// Registry ID for the Swagger subsystem
int swagger_subsystem_id = -1;

void register_swagger(void) {
    // Only register if not already registered
    if (swagger_subsystem_id < 0) {
        swagger_subsystem_id = register_subsystem_from_launch("Swagger", NULL, NULL, NULL,
                                                (int (*)(void))launch_swagger_subsystem,
                                                NULL);  // No special shutdown needed
        if (swagger_subsystem_id < 0) {
            log_this("Swagger", "Failed to register Swagger subsystem", LOG_LEVEL_ERROR);
        }
    }
}

// Check if the Swagger subsystem is ready to launch
LaunchReadiness check_swagger_launch_readiness(void) {
    const char** messages = malloc(12 * sizeof(char*));  // Space for messages + NULL
    if (!messages) {
        return (LaunchReadiness){ .subsystem = "Swagger", .ready = false, .messages = NULL };
    }
    
    size_t msg_index = 0;
    bool ready = true;

    // First message is subsystem name
    messages[msg_index++] = strdup("Swagger");
    
    // Register with registry first
    register_swagger();
    if (swagger_subsystem_id < 0) {
        messages[msg_index++] = strdup("  No-Go:   Failed to register with registry");
        messages[msg_index++] = NULL;
        return (LaunchReadiness){ .subsystem = "Swagger", .ready = false, .messages = messages };
    }
    
    // Get app config
    const AppConfig *config = get_app_config();
    if (!config) {
        messages[msg_index++] = strdup("  No-Go:   Failed to get app config");
        messages[msg_index++] = NULL;
        return (LaunchReadiness){.subsystem = "Swagger", .ready = false, .messages = messages};
    }

    // Check if Swagger is enabled
    if (!config || !config->swagger.enabled) {
        messages[msg_index++] = strdup("  Skip:    Swagger is disabled");
        messages[msg_index++] = NULL;
        return (LaunchReadiness){.subsystem = "Swagger", .ready = false, .messages = messages};
    }

    // // Check Registry subsystem readiness (primary dependency)
    // LaunchReadiness registry_readiness = get_registry_readiness();
    // if (!registry_readiness.ready) {
    //     messages[msg_index++] = strdup("  No-Go:   Registry subsystem not ready");
    //     messages[msg_index] = NULL;
    //     free_readiness_messages(&registry_readiness);
    //     return (LaunchReadiness){.subsystem = "Swagger", .ready = false, .messages = messages};
    // }
    // messages[msg_index++] = strdup("  Go:      Registry subsystem ready");
    // free_readiness_messages(&registry_readiness);

    // Check Network subsystem readiness (using cached version)
    // LaunchReadiness network_readiness = get_network_readiness();
    // if (!network_readiness.ready) {
    //     messages[msg_index++] = strdup("  No-Go:   Network subsystem not ready");
    //     messages[msg_index] = NULL;
    //     return (LaunchReadiness){.subsystem = "Swagger", .ready = false, .messages = messages};
    // }
    // messages[msg_index++] = strdup("  Go:      Network subsystem ready");

    // Check API subsystem readiness (depends on Registry and Network)
    LaunchReadiness api_readiness = check_api_launch_readiness();
    if (!api_readiness.ready) {
        messages[msg_index++] = strdup("  No-Go:   API subsystem not ready");
        messages[msg_index] = NULL;
        free_readiness_messages(&api_readiness);
        return (LaunchReadiness){.subsystem = "Swagger", .ready = false, .messages = messages};
    }
    messages[msg_index++] = strdup("  Go:      API subsystem ready");
    free_readiness_messages(&api_readiness);

    // Check Payload subsystem readiness
    LaunchReadiness payload_readiness = check_payload_launch_readiness();
    if (!payload_readiness.ready) {
        messages[msg_index++] = strdup("  No-Go:   Payload subsystem not ready");
        messages[msg_index] = NULL;
        free_readiness_messages(&payload_readiness);
        return (LaunchReadiness){.subsystem = "Swagger", .ready = false, .messages = messages};
    }
    messages[msg_index++] = strdup("  Go:      Payload subsystem ready");
    free_readiness_messages(&payload_readiness);

    // Validate prefix
    if (!config->swagger.prefix || strlen(config->swagger.prefix) < 1 ||
        strlen(config->swagger.prefix) > 64 || config->swagger.prefix[0] != '/') {
        messages[msg_index++] = strdup("  No-Go:   Invalid Swagger prefix configuration");
        ready = false;
    } else {
        char* msg = malloc(256);
        if (msg) {
            snprintf(msg, 256, "  Go:      Valid Swagger prefix: %s", 
                    config->swagger.prefix);
            messages[msg_index++] = msg;
        }
    }

    // Validate required metadata
    if (!config->swagger.metadata.title || 
        strlen(config->swagger.metadata.title) < 1 ||
        strlen(config->swagger.metadata.title) > 128) {
        messages[msg_index++] = strdup("  No-Go:   Invalid Swagger title configuration");
        ready = false;
    }

    if (!config->swagger.metadata.version ||
        strlen(config->swagger.metadata.version) < 1 ||
        strlen(config->swagger.metadata.version) > 32) {
        messages[msg_index++] = strdup("  No-Go:   Invalid Swagger version configuration");
        ready = false;
    }

    if (config->swagger.metadata.description && 
        strlen(config->swagger.metadata.description) > 1024) {
        messages[msg_index++] = strdup("  No-Go:   Swagger description too long");
        ready = false;
    }

    // Validate UI options
    if (config->swagger.ui_options.default_models_expand_depth < 0 ||
        config->swagger.ui_options.default_models_expand_depth > 10) {
        messages[msg_index++] = strdup("  No-Go:   Invalid models expand depth");
        ready = false;
    }

    if (config->swagger.ui_options.default_model_expand_depth < 0 ||
        config->swagger.ui_options.default_model_expand_depth > 10) {
        messages[msg_index++] = strdup("  No-Go:   Invalid model expand depth");
        ready = false;
    }

    // Validate doc expansion value
    if (config->swagger.ui_options.doc_expansion) {
        const char* exp = config->swagger.ui_options.doc_expansion;
        if (strcmp(exp, "list") != 0 && strcmp(exp, "full") != 0 && strcmp(exp, "none") != 0) {
            messages[msg_index++] = strdup("  No-Go:   Invalid doc expansion value");
            ready = false;
        }
    }

    if (ready) {
        messages[msg_index++] = strdup("  Go:      All configuration values validated");
    }

    // Final decision
    messages[msg_index++] = strdup(ready ? 
        "  Decide:  Go For Launch of Swagger Subsystem" :
        "  Decide:  No-Go For Launch of Swagger Subsystem");
    messages[msg_index] = NULL;
    
    return (LaunchReadiness){
        .subsystem = "Swagger",
        .ready = ready,
        .messages = messages
    };
}

// Launch Swagger subsystem
int launch_swagger_subsystem(void);

int launch_swagger_subsystem(void) {
    extern volatile sig_atomic_t server_stopping;
    extern volatile sig_atomic_t server_starting;

    log_this("Swagger", LOG_LINE_BREAK, LOG_LEVEL_STATE);
    log_this("Swagger", "LAUNCH: SWAGGER", LOG_LEVEL_STATE);

    // Step 1: Register with registry and add dependencies
    log_this("Swagger", "  Step 1: Registering with registry", LOG_LEVEL_STATE);
    register_swagger();
    if (swagger_subsystem_id < 0) {
        log_this("Swagger", "    Failed to register Swagger subsystem", LOG_LEVEL_ERROR);
        log_this("Swagger", "LAUNCH: SWAGGER - Failed: Registration failed", LOG_LEVEL_STATE);
        return 0;
    }
    add_subsystem_dependency(swagger_subsystem_id, "Registry");
    add_subsystem_dependency(swagger_subsystem_id, "API");
    add_subsystem_dependency(swagger_subsystem_id, "WebServer");
    add_subsystem_dependency(swagger_subsystem_id, "Payload");
    log_this("Swagger", "    Registration complete", LOG_LEVEL_STATE);

    // Step 2: Verify system state
    log_this("Swagger", "  Step 2: Verifying system state", LOG_LEVEL_STATE);
    
    if (server_stopping) {
        log_this("Swagger", "    Cannot initialize Swagger during shutdown", LOG_LEVEL_STATE);
        log_this("Swagger", "LAUNCH: SWAGGER - Failed: System in shutdown", LOG_LEVEL_STATE);
        return 0;
    }

    if (!server_starting) {
        log_this("Swagger", "    Cannot initialize Swagger outside startup phase", LOG_LEVEL_STATE);
        log_this("Swagger", "LAUNCH: SWAGGER - Failed: Not in startup phase", LOG_LEVEL_STATE);
        return 0;
    }

    if (!app_config) {
        log_this("Swagger", "    Swagger configuration not loaded", LOG_LEVEL_STATE);
        log_this("Swagger", "LAUNCH: SWAGGER - Failed: No configuration", LOG_LEVEL_STATE);
        return 0;
    }

    if (!app_config->swagger.enabled) {
        log_this("Swagger", "    Swagger disabled in configuration", LOG_LEVEL_STATE);
        log_this("Swagger", "LAUNCH: SWAGGER - Disabled by configuration", LOG_LEVEL_STATE);
        return 1; // Not an error if disabled
    }
    log_this("Swagger", "    System state verified", LOG_LEVEL_STATE);

    // Step 3: Verify dependencies
    log_this("Swagger", "  Step 3: Verifying dependencies", LOG_LEVEL_STATE);

    // Check Registry first
    if (!is_subsystem_running_by_name("Registry")) {
        log_this("Swagger", "    Registry not running", LOG_LEVEL_ERROR);
        log_this("Swagger", "LAUNCH: SWAGGER - Failed: Registry dependency not met", LOG_LEVEL_STATE);
        return 0;
    }
    log_this("Swagger", "    Registry dependency verified", LOG_LEVEL_STATE);

    // Then check API
    if (!is_api_running()) {
        log_this("Swagger", "    API subsystem not running", LOG_LEVEL_ERROR);
        log_this("Swagger", "LAUNCH: SWAGGER - Failed: API dependency not met", LOG_LEVEL_STATE);
        return 0;
    }
    log_this("Swagger", "    API dependency verified", LOG_LEVEL_STATE);

    // Check webserver
    if (!is_web_server_running()) {
        log_this("Swagger", "    Web server not running", LOG_LEVEL_ERROR);
        log_this("Swagger", "LAUNCH: SWAGGER - Failed: WebServer dependency not met", LOG_LEVEL_STATE);
        return 0;
    }
    log_this("Swagger", "    Web server verified", LOG_LEVEL_STATE);

    // Check payload subsystem state
    int payload_id = get_subsystem_id_by_name("Payload");
    if (payload_id < 0 || get_subsystem_state(payload_id) != SUBSYSTEM_RUNNING) {
        log_this("Swagger", "    Payload subsystem not in running state", LOG_LEVEL_ERROR);
        log_this("Swagger", "LAUNCH: SWAGGER - Failed: Payload dependency not met", LOG_LEVEL_STATE);
        return 0;
    }
    log_this("Swagger", "    Payload subsystem verified", LOG_LEVEL_STATE);

    // Verify payload files can be accessed
    size_t payload_size;
    if (!check_payload_exists(PAYLOAD_MARKER, &payload_size)) {
        log_this("Swagger", "    Swagger UI files not found in payload", LOG_LEVEL_STATE);
        log_this("Swagger", "LAUNCH: SWAGGER - Failed: Missing UI files", LOG_LEVEL_STATE);
        return 0;
    }

    // Set payload availability flag since we found it
    if (app_config) {
        AppConfig* mutable_config = (AppConfig*)app_config;
        mutable_config->swagger.payload_available = 1;
    }
    log_this("Swagger", "    Swagger UI files verified (%zu bytes)", LOG_LEVEL_STATE, payload_size);
    log_this("Swagger", "    All dependencies verified", LOG_LEVEL_STATE);

    // Step 4: Initialize Swagger UI
    log_this("Swagger", "  Step 3: Initializing Swagger UI", LOG_LEVEL_STATE);
    
    // Wait for API to be fully running
    int retries = 0;
    while (!is_api_running() && retries < 10) {
        usleep(100000); // Wait 100ms between checks
        retries++;
    }
    
    if (!is_api_running()) {
        log_this("Swagger", "    API subsystem not running after waiting", LOG_LEVEL_ERROR);
        log_this("Swagger", "LAUNCH: SWAGGER - Failed: API not running", LOG_LEVEL_STATE);
        return 0;
    }
    log_this("Swagger", "    API subsystem running", LOG_LEVEL_STATE);

    // Initialize Swagger UI support
    if (!init_swagger_support(&app_config->swagger)) {
        log_this("Swagger", "    Failed to initialize Swagger UI", LOG_LEVEL_ERROR);
        log_this("Swagger", "LAUNCH: SWAGGER - Failed: UI initialization failed", LOG_LEVEL_STATE);
        return 0;
    }
    log_this("Swagger", "    Swagger UI initialized", LOG_LEVEL_STATE);

    // Register Swagger endpoint with webserver using our static wrapper functions
    WebServerEndpoint swagger_endpoint = {
        .prefix = app_config->swagger.prefix,
        .validator = swagger_url_validator,
        .handler = swagger_request_handler
    };

    if (!register_web_endpoint(&swagger_endpoint)) {
        log_this("Swagger", "    Failed to register Swagger endpoint", LOG_LEVEL_ERROR);
        log_this("Swagger", "LAUNCH: SWAGGER - Failed: Endpoint registration failed", LOG_LEVEL_STATE);
        return 0;
    }

    log_this("Swagger", "    Registered endpoint with prefix: %s", LOG_LEVEL_STATE, 
             app_config->swagger.prefix);
    log_this("Swagger", "      -> /", LOG_LEVEL_STATE);
    log_this("Swagger", "      -> /index.html", LOG_LEVEL_STATE);
    log_this("Swagger", "      -> /swagger-ui.css", LOG_LEVEL_STATE);
    log_this("Swagger", "      -> /swagger-ui-bundle.js", LOG_LEVEL_STATE);
    log_this("Swagger", "      -> /swagger-ui-standalone-preset.js", LOG_LEVEL_STATE);
    log_this("Swagger", "      -> /swagger.json", LOG_LEVEL_STATE);
    log_this("Swagger", "    Routes registered", LOG_LEVEL_STATE);
    
    // Log configuration
    log_this("Swagger", "    Configuration:", LOG_LEVEL_STATE);
    log_this("Swagger", "      -> Enabled: yes", LOG_LEVEL_STATE);
    log_this("Swagger", "      -> Prefix: %s", LOG_LEVEL_STATE, 
             app_config->swagger.prefix);
    log_this("Swagger", "      -> Title: %s", LOG_LEVEL_STATE, 
             app_config->swagger.metadata.title);
    log_this("Swagger", "      -> Version: %s", LOG_LEVEL_STATE, 
             app_config->swagger.metadata.version);
    log_this("Swagger", "      -> Payload: available", LOG_LEVEL_STATE);
    log_this("Swagger", "    Swagger UI initialized", LOG_LEVEL_STATE);

    // Step 4: Update registry and verify state
    log_this("Swagger", "  Step 4: Updating subsystem registry", LOG_LEVEL_STATE);
    update_subsystem_on_startup("Swagger", true);
    
    SubsystemState final_state = get_subsystem_state(swagger_subsystem_id);
    if (final_state == SUBSYSTEM_RUNNING) {
        log_this("Swagger", "LAUNCH: SWAGGER - Successfully launched and running", LOG_LEVEL_STATE);
    } else {
        log_this("Swagger", "LAUNCH: SWAGGER - Warning: Unexpected final state: %s", LOG_LEVEL_ALERT,
                subsystem_state_to_string(final_state));
        return 0;
    }
    
    return 1;
}

// Check if Swagger is running
int is_swagger_running(void);

int is_swagger_running(void) {
    extern volatile sig_atomic_t server_stopping;
    
    // Swagger is running if:
    // 1. It's enabled in config
    // 2. Not in shutdown state
    // 3. Registry is running
    // 4. API is running
    // 5. Payload is available
    return (app_config && 
            app_config->swagger.enabled && 
            app_config->swagger.payload_available &&
            !server_stopping &&
            is_subsystem_running_by_name("Registry") &&
            is_api_running());
}
