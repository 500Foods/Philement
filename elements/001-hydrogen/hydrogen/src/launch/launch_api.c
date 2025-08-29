/*
 * Launch API Subsystem
 *
 * This module handles the initialization of the API subsystem.
 * It provides functions for checking readiness and launching the API endpoints.
 *
 * Dependencies:
 * - WebServer subsystem must be initialized and ready
 */

// Global includes
#include "../hydrogen.h"
#include <string.h>  // For memset

// Local includes
#include "launch.h"
#include "../api/api_service.h" // For API service initialization

// Registry ID and cached readiness state
int api_subsystem_id = -1;

// Register the API subsystem with the registry
void register_api(void) {
    // Always register during readiness check if not already registered
    if (api_subsystem_id < 0) {
        api_subsystem_id = register_subsystem_from_launch(SR_API, NULL, NULL, NULL,
                                            (int (*)(void))launch_api_subsystem,
                                            NULL);  // No special shutdown needed
        if (api_subsystem_id < 0) {
            log_this(SR_API, "Failed to register API subsystem", LOG_LEVEL_ERROR);
        }
    }
}

/*
 * Check if the API subsystem is ready to launch
 *
 * This function performs readiness checks for the API subsystem by:
 * - Verifying system state and dependencies (Registry, WebServer)
 * - Checking API configuration (endpoints, prefix, JWT)
 *
 * @return LaunchReadiness structure with readiness status and messages
 */
LaunchReadiness check_api_launch_readiness(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool ready = true;

    // First message is subsystem name
    add_launch_message(&messages, &count, &capacity, strdup(SR_API));

    // Register with registry first
    register_api();
    if (api_subsystem_id < 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to register with registry"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_API, .ready = false, .messages = messages };
    }

    // 1. Check Registry subsystem readiness (primary dependency)
    LaunchReadiness registry_readiness = check_registry_launch_readiness();
    if (!registry_readiness.ready) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Registry subsystem not Go for Launch"));
        finalize_launch_messages(&messages, &count, &capacity);
        cleanup_readiness_messages(&registry_readiness);
        return (LaunchReadiness){ .subsystem = SR_API, .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Registry subsystem Go for Launch"));
    cleanup_readiness_messages(&registry_readiness);

    // 4. Check our configuration
    if (!app_config) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   API configuration not loaded"));
        ready = false;
    } else {
        // Check if API is enabled
        if (!app_config->api.enabled) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   API endpoints disabled in configuration"));
            ready = false;
        } else {
            add_launch_message(&messages, &count, &capacity, strdup("  Go:      API endpoints enabled"));
        }

        // Check API prefix
        if (!app_config->api.prefix || !app_config->api.prefix[0] ||
            app_config->api.prefix[0] != '/') {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid API prefix configuration"));
            ready = false;
        } else {
            char* msg = malloc(256);
            if (msg) {
                snprintf(msg, 256, "  Go:      Valid API prefix: %s", app_config->api.prefix);
                add_launch_message(&messages, &count, &capacity, msg);
            }
        }

        // Check JWT secret
        if (!app_config->api.jwt_secret || !app_config->api.jwt_secret[0]) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   JWT secret not configured"));
            ready = false;
        } else {
            add_launch_message(&messages, &count, &capacity, strdup("  Go:      JWT secret configured"));
        }
    }

    // Final decision
    add_launch_message(&messages, &count, &capacity, strdup(ready ?
        "  Decide:  Go For Launch of API Subsystem" :
        "  Decide:  No-Go For Launch of API Subsystem"));

    finalize_launch_messages(&messages, &count, &capacity);

    LaunchReadiness readiness = {
        .subsystem = SR_API,
        .ready = ready,
        .messages = NULL
    };

    set_readiness_messages(&readiness, messages);

    return readiness;
}

// Launch API subsystem
int launch_api_subsystem(void) {
    extern volatile sig_atomic_t server_stopping;
    extern volatile sig_atomic_t server_starting;

    log_this(SR_API, LOG_LINE_BREAK, LOG_LEVEL_STATE);
    log_this(SR_API, "LAUNCH: API", LOG_LEVEL_STATE);

    // Step 1: Register with registry and add dependencies
    log_this(SR_API, "  Step 1: Registering with registry", LOG_LEVEL_STATE);
    register_api();
    if (api_subsystem_id < 0) {
        log_this(SR_API, "    Failed to register API subsystem", LOG_LEVEL_ERROR);
        log_this(SR_API, "LAUNCH: API - Failed: Registration failed", LOG_LEVEL_STATE);
        return 0;
    }
    add_subsystem_dependency(api_subsystem_id, "Registry");
    add_subsystem_dependency(api_subsystem_id, "WebServer");
    log_this(SR_API, "    Registration complete", LOG_LEVEL_STATE);

    // Step 2: Verify system state
    log_this(SR_API, "  Step 2: Verifying system state", LOG_LEVEL_STATE);

    if (server_stopping) {
        log_this(SR_API, "    Cannot initialize API during shutdown", LOG_LEVEL_STATE);
        log_this(SR_API, "LAUNCH: API - Failed: System in shutdown", LOG_LEVEL_STATE);
        return 0;
    }

    if (!server_starting) {
        log_this(SR_API, "    Cannot initialize API outside startup phase", LOG_LEVEL_STATE);
        log_this(SR_API, "LAUNCH: API - Failed: Not in startup phase", LOG_LEVEL_STATE);
        return 0;
    }

    if (!app_config) {
        log_this(SR_API, "    API configuration not loaded", LOG_LEVEL_STATE);
        log_this(SR_API, "LAUNCH: API - Failed: No configuration", LOG_LEVEL_STATE);
        return 0;
    }

    if (!app_config->api.enabled) {
        log_this(SR_API, "    API disabled in configuration", LOG_LEVEL_STATE);
        log_this(SR_API, "LAUNCH: API - Disabled by configuration", LOG_LEVEL_STATE);
        return 1; // Not an error if disabled
    }

    log_this(SR_API, "    System state verified", LOG_LEVEL_STATE);

    // Step 3: Verify dependencies
    log_this(SR_API, "  Step 3: Verifying dependencies", LOG_LEVEL_STATE);

    // Check Registry first
    if (!is_subsystem_running_by_name("Registry")) {
        log_this(SR_API, "    Registry not running", LOG_LEVEL_ERROR);
        log_this(SR_API, "LAUNCH: API - Failed: Registry dependency not met", LOG_LEVEL_STATE);
        return 0;
    }
    log_this(SR_API, "    Registry dependency verified", LOG_LEVEL_STATE);

    // Then check WebServer
    if (!is_web_server_running()) {
        log_this(SR_API, "    Web server not running", LOG_LEVEL_ERROR);
        log_this(SR_API, "LAUNCH: API - Failed: WebServer dependency not met", LOG_LEVEL_STATE);
        return 0;
    }
    log_this(SR_API, "    WebServer dependency verified", LOG_LEVEL_STATE);
    log_this(SR_API, "    All dependencies verified", LOG_LEVEL_STATE);

    // Step 4: Initialize API endpoints
    log_this(SR_API, "  Step 3: Initializing API endpoints", LOG_LEVEL_STATE);

    // Initialize and register API endpoints
    if (!init_api_endpoints()) {
        log_this(SR_API, "    Failed to initialize API endpoints", LOG_LEVEL_ERROR);
        log_this(SR_API, "LAUNCH: API - Failed: Endpoint initialization failed", LOG_LEVEL_STATE);
        return 0;
    }

    log_this(SR_API, "    API endpoints initialized successfully", LOG_LEVEL_STATE);

    // Step 5: Update registry and verify state
    log_this(SR_API, "  Step 4: Updating subsystem registry", LOG_LEVEL_STATE);
    update_subsystem_on_startup(SR_API, true);

    SubsystemState final_state = get_subsystem_state(api_subsystem_id);
    if (final_state == SUBSYSTEM_RUNNING) {
        log_this(SR_API, "LAUNCH: API - Successfully launched and running", LOG_LEVEL_STATE);
    } else {
        log_this(SR_API, "LAUNCH: API - Warning: Unexpected final state: %s", LOG_LEVEL_ALERT,
                subsystem_state_to_string(final_state));
        return 0;
    }

    return 1;
}

// Check if API is running
int is_api_running(void) {
    extern volatile sig_atomic_t server_stopping;

    // API is running if:
    // 1. It's enabled in config
    // 2. Not in shutdown state
    // 3. Registry is running
    // 4. WebServer is running
    return (app_config &&
            app_config->api.enabled &&
            !server_stopping &&
            is_subsystem_running_by_name("Registry") &&
            is_web_server_running());
}
