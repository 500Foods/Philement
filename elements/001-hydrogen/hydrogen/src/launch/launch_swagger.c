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
#include <src/hydrogen.h>

// Local includes
#include "launch.h"
#include <src/config/config_swagger.h>
#include <src/swagger/swagger.h>
#include <src/webserver/web_server_core.h>  // For WebServerEndpoint
#include <src/payload/payload_cache.h>      // For payload cache functions

// Registry ID for the Swagger subsystem
int swagger_subsystem_id = -1;

// Check if the Swagger subsystem is ready to launch
LaunchReadiness check_swagger_launch_readiness(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool ready = true;

    // First message is subsystem name
    add_launch_message(&messages, &count, &capacity, strdup(SR_SWAGGER));

    // Register with registry first
        if (swagger_subsystem_id < 0) {
        swagger_subsystem_id = register_subsystem_from_launch(SR_SWAGGER, NULL, NULL, NULL,
                                                (int (*)(void))launch_swagger_subsystem,
                                                NULL);  // No special shutdown needed
        if (swagger_subsystem_id < 0) {
            log_this(SR_SWAGGER, "Failed to register Swagger subsystem", LOG_LEVEL_ERROR, 0);
        }
    }

    if (swagger_subsystem_id < 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to register with registry"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_SWAGGER, .ready = false, .messages = messages };
    }

    // Get app config
    if (!app_config) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to get app config"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){.subsystem = SR_SWAGGER, .ready = false, .messages = messages};
    }

    // Check if Swagger is enabled
    if (!app_config || !app_config->swagger.enabled) {
        add_launch_message(&messages, &count, &capacity, strdup("  Skip:    Swagger is disabled"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){.subsystem = SR_SWAGGER, .ready = false, .messages = messages};
    }

    // Check API subsystem readiness (depends on Registry and Network)
    LaunchReadiness api_readiness = check_api_launch_readiness();
    if (!api_readiness.ready) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   API subsystem not ready"));
        finalize_launch_messages(&messages, &count, &capacity);
        cleanup_readiness_messages(&api_readiness);
        return (LaunchReadiness){.subsystem = SR_SWAGGER, .ready = false, .messages = messages};
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      API subsystem ready"));
    cleanup_readiness_messages(&api_readiness);

    // Check Payload subsystem readiness
    LaunchReadiness payload_readiness = check_payload_launch_readiness();
    if (!payload_readiness.ready) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Payload subsystem not ready"));
        finalize_launch_messages(&messages, &count, &capacity);
        cleanup_readiness_messages(&payload_readiness);
        return (LaunchReadiness){.subsystem = SR_SWAGGER, .ready = false, .messages = messages};
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Payload subsystem ready"));
    cleanup_readiness_messages(&payload_readiness);

    // Validate prefix
    if (!app_config->swagger.prefix || strlen(app_config->swagger.prefix) < 1 ||
        strlen(app_config->swagger.prefix) > 64 || app_config->swagger.prefix[0] != '/') {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid Swagger prefix configuration"));
        ready = false;
    } else {
        char* prefix_msg = malloc(256);
        if (prefix_msg) {
            snprintf(prefix_msg, 256, "  Go:      Valid Swagger prefix: %s",
                    app_config->swagger.prefix);
            add_launch_message(&messages, &count, &capacity, prefix_msg);
        }
    }

    // Validate required metadata
    if (!app_config->swagger.metadata.title ||
        strlen(app_config->swagger.metadata.title) < 1 ||
        strlen(app_config->swagger.metadata.title) > 128) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid Swagger title configuration"));
        ready = false;
    }

    if (!app_config->swagger.metadata.version ||
        strlen(app_config->swagger.metadata.version) < 1 ||
        strlen(app_config->swagger.metadata.version) > 32) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid Swagger version configuration"));
        ready = false;
    }

    if (app_config->swagger.metadata.description &&
        strlen(app_config->swagger.metadata.description) > 1024) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Swagger description too long"));
        ready = false;
    }

    // Validate UI options
    if (app_config->swagger.ui_options.default_models_expand_depth < 0 ||
        app_config->swagger.ui_options.default_models_expand_depth > 10) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid models expand depth"));
        ready = false;
    }

    if (app_config->swagger.ui_options.default_model_expand_depth < 0 ||
        app_config->swagger.ui_options.default_model_expand_depth > 10) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid model expand depth"));
        ready = false;
    }

    // Validate doc expansion value
    if (app_config->swagger.ui_options.doc_expansion) {
        const char* exp = app_config->swagger.ui_options.doc_expansion;
        if (strcmp(exp, "list") != 0 && strcmp(exp, "full") != 0 && strcmp(exp, "none") != 0) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid doc expansion value"));
            ready = false;
        }
    }

    if (ready) {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      All configuration values validated"));
    }

    // Final decision
    add_launch_message(&messages, &count, &capacity, strdup(ready ?
        "  Decide:  Go For Launch of Swagger Subsystem" :
        "  Decide:  No-Go For Launch of Swagger Subsystem"));

    finalize_launch_messages(&messages, &count, &capacity);

    return (LaunchReadiness){
        .subsystem = SR_SWAGGER,
        .ready = ready,
        .messages = messages
    };
}

// Launch Swagger subsystem
int launch_swagger_subsystem(void) {

    log_this(SR_SWAGGER, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_SWAGGER, "LAUNCH: " SR_SWAGGER, LOG_LEVEL_DEBUG, 0);

    // Step 1: Register with registry and add dependencies
    log_this(SR_SWAGGER, "Registering with registry", LOG_LEVEL_DEBUG, 0);
    
    if (swagger_subsystem_id < 0) {
        log_this(SR_SWAGGER, "    Failed to register Swagger subsystem", LOG_LEVEL_ERROR, 0);
        log_this(SR_SWAGGER, "LAUNCH: SWAGGER - Failed: Registration failed", LOG_LEVEL_DEBUG, 0);
        return 0;
    }
    add_subsystem_dependency(swagger_subsystem_id, "Registry");
    add_subsystem_dependency(swagger_subsystem_id, "API");
    add_subsystem_dependency(swagger_subsystem_id, "WebServer");
    add_subsystem_dependency(swagger_subsystem_id, "Payload");
    log_this(SR_SWAGGER, "    Registration complete", LOG_LEVEL_DEBUG, 0);

    // Step 2: Verify system state
    log_this(SR_SWAGGER, "  Step 2: Verifying system state", LOG_LEVEL_DEBUG, 0);
    
    if (server_stopping) {
        log_this(SR_SWAGGER, "    Cannot initialize Swagger during shutdown", LOG_LEVEL_DEBUG, 0);
        log_this(SR_SWAGGER, "LAUNCH: SWAGGER - Failed: System in shutdown", LOG_LEVEL_DEBUG, 0);
        return 0;
    }

    if (!server_starting) {
        log_this(SR_SWAGGER, "    Cannot initialize Swagger outside startup phase", LOG_LEVEL_DEBUG, 0);
        log_this(SR_SWAGGER, "LAUNCH: SWAGGER - Failed: Not in startup phase", LOG_LEVEL_DEBUG, 0);
        return 0;
    }

    if (!app_config) {
        log_this(SR_SWAGGER, "    Swagger configuration not loaded", LOG_LEVEL_DEBUG, 0);
        log_this(SR_SWAGGER, "LAUNCH: SWAGGER - Failed: No configuration", LOG_LEVEL_DEBUG, 0);
        return 0;
    }

    if (!app_config->swagger.enabled) {
        log_this(SR_SWAGGER, "    Swagger disabled in configuration", LOG_LEVEL_DEBUG, 0);
        log_this(SR_SWAGGER, "LAUNCH: SWAGGER - Disabled by configuration", LOG_LEVEL_DEBUG, 0);
        return 1; // Not an error if disabled
    }
    log_this(SR_SWAGGER, "    System state verified", LOG_LEVEL_DEBUG, 0);

    // Step 3: Verify dependencies
    log_this(SR_SWAGGER, "  Step 3: Verifying dependencies", LOG_LEVEL_DEBUG, 0);

    // Check Registry first
    if (!is_subsystem_running_by_name("Registry")) {
        log_this(SR_SWAGGER, "    Registry not running", LOG_LEVEL_ERROR, 0);
        log_this(SR_SWAGGER, "LAUNCH: SWAGGER - Failed: Registry dependency not met", LOG_LEVEL_DEBUG, 0);
        return 0;
    }
    log_this(SR_SWAGGER, "    Registry dependency verified", LOG_LEVEL_DEBUG, 0);

    // Then check API subsystem
    if (!is_subsystem_running_by_name(SR_API)) {
        log_this(SR_SWAGGER, "    " SR_API " subsystem not running", LOG_LEVEL_ERROR, 0);
        log_this(SR_SWAGGER, "LAUNCH: SWAGGER - Failed: " SR_API " subsystem dependency not met", LOG_LEVEL_DEBUG, 0);
        return 0;
    }
    log_this(SR_SWAGGER, "    " SR_API " subsystem dependency verified", LOG_LEVEL_DEBUG, 0);

    // Check webserver
    if (!is_subsystem_running_by_name(SR_WEBSERVER)) {
        log_this(SR_SWAGGER, "    " SR_WEBSERVER " subsystem not running", LOG_LEVEL_ERROR, 0);
        log_this(SR_SWAGGER, "LAUNCH: " SR_SWAGGER" Failed: " SR_WEBSERVER " subsystem dependency not met", LOG_LEVEL_DEBUG, 0);
        return 0;
    }
    log_this(SR_SWAGGER, "    " SR_WEBSERVER " subsystem verified", LOG_LEVEL_DEBUG, 0);

    // Check payload subsystem state
    int payload_id = get_subsystem_id_by_name("Payload");
    if (payload_id < 0 || get_subsystem_state(payload_id) != SUBSYSTEM_RUNNING) {
        log_this(SR_SWAGGER, "    " SR_PAYLOAD " subsystem not running", LOG_LEVEL_ERROR, 0);
        log_this(SR_SWAGGER, "LAUNCH: " SR_PAYLOAD " Failed: " SR_PAYLOAD "subsystem dependency not met", LOG_LEVEL_DEBUG, 0);
        return 0;
    }
    log_this(SR_SWAGGER, "    " SR_PAYLOAD " subsystem verified", LOG_LEVEL_DEBUG, 0);

    // Check for swagger files in payload cache
    if (!is_payload_cache_available()) {
        log_this(SR_SWAGGER, "    Payload cache not available", LOG_LEVEL_DEBUG, 0);
        log_this(SR_SWAGGER, "LAUNCH: " SR_SWAGGER " Failed: Payload cache not available", LOG_LEVEL_DEBUG, 0);
        return 0;
    }

    // Verify swagger files are in cache
    PayloadFile *local_swagger_files = NULL;
    size_t local_num_swagger_files = 0;
    size_t capacity = 0;

    bool files_retrieved = get_payload_files_by_prefix("swagger/", &local_swagger_files, &local_num_swagger_files, &capacity);
    if (!files_retrieved || !local_swagger_files || local_num_swagger_files == 0) {
        log_this(SR_SWAGGER, "    No swagger files found in payload cache", LOG_LEVEL_DEBUG, 0);
        log_this(SR_SWAGGER, "LAUNCH: " SR_SWAGGER " Failed: Missing Swagger UI files", LOG_LEVEL_DEBUG, 0);
        return 0;
    }

    // Set payload availability flag and log available files
    if (app_config) {
        AppConfig* mutable_config = (AppConfig*)app_config;
        mutable_config->swagger.payload_available = 1;
    }

    log_this(SR_SWAGGER, "    Swagger files verified (%zu files in cache):", LOG_LEVEL_DEBUG, 1, local_num_swagger_files);
    for (size_t i = 0; i < local_num_swagger_files; i++) {
        log_this(SR_SWAGGER, "      -> %s (%s)", LOG_LEVEL_DEBUG, 2,
                local_swagger_files[i].name,
                local_swagger_files[i].size < 1024 ?
                    local_swagger_files[i].size < 512 ?
                        "small file" : "medium file" : "large file");
    }

    // Set the global config so validators and handlers can access it and load files
    if (!init_swagger_support_from_payload(&app_config->swagger, local_swagger_files, local_num_swagger_files)) {
        log_this(SR_SWAGGER, "    Failed to load swagger files into memory", LOG_LEVEL_ERROR, 0);
        log_this(SR_SWAGGER, "LAUNCH: SWAGGER - Failed: File loading failed", LOG_LEVEL_DEBUG, 0);
        // Free the allocated names before returning
        for (size_t i = 0; i < local_num_swagger_files; i++) {
            free(local_swagger_files[i].name);
        }
        free(local_swagger_files);
        return 0;
    }

    // Free the retrieved files array including names (data is owned by swagger now)
    for (size_t i = 0; i < local_num_swagger_files; i++) {
        free(local_swagger_files[i].name);
    }
    free(local_swagger_files);
    log_this(SR_SWAGGER, "    All dependencies verified", LOG_LEVEL_DEBUG, 0);

    // Register Swagger endpoint with webserver
    WebServerEndpoint swagger_endpoint = {
        .prefix = app_config->swagger.prefix,
        .validator = swagger_url_validator,
        .handler = swagger_request_handler
    };

    if (!register_web_endpoint(&swagger_endpoint)) {
        log_this(SR_SWAGGER, "    Failed to register endpoint", LOG_LEVEL_ERROR, 0);
        log_this(SR_SWAGGER, "LAUNCH: " SR_SWAGGER " Failed: Endpoint registration failed", LOG_LEVEL_DEBUG, 0);
        return 0;
    }
    
    // Log configuration
    log_this(SR_SWAGGER, "Configuration:", LOG_LEVEL_DEBUG, 0);
    log_this(SR_SWAGGER, "― Enabled: yes", LOG_LEVEL_DEBUG, 0);
    log_this(SR_SWAGGER, "― Prefix: %s", LOG_LEVEL_DEBUG, 1, app_config->swagger.prefix);
    log_this(SR_SWAGGER, "― Title: %s", LOG_LEVEL_DEBUG, 1, app_config->swagger.metadata.title);
    log_this(SR_SWAGGER, "― Version: %s", LOG_LEVEL_DEBUG, 1, app_config->swagger.metadata.version);
    log_this(SR_SWAGGER, "― Payload: available", LOG_LEVEL_DEBUG, 0);
    log_this(SR_SWAGGER, "    " SR_SWAGGER " subsystem initialized", LOG_LEVEL_DEBUG, 0);

    // Step 4: Update registry and verify state
    log_this(SR_SWAGGER, "  Updating " SR_REGISTRY, LOG_LEVEL_DEBUG, 0);
    update_subsystem_on_startup(SR_SWAGGER, true);
    
    SubsystemState final_state = get_subsystem_state(swagger_subsystem_id);
    if (final_state == SUBSYSTEM_RUNNING) {
        log_this(SR_SWAGGER, "LAUNCH: " SR_SWAGGER " Success: Launched and running", LOG_LEVEL_DEBUG, 0);
    } else {
        log_this(SR_SWAGGER, "LAUNCH: " SR_SWAGGER " Warning: Unexpected final state: %s", LOG_LEVEL_DEBUG, 1, subsystem_state_to_string(final_state));
        return 0;
    }
    
    return 1;
}
