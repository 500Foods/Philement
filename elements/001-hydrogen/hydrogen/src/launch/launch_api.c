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

// Local includes
#include "launch.h"
#include "launch_webserver.h"   // For is_web_server_running()
#include "launch_network.h"     // For get_network_readiness()
#include "../api/api_service.h" // For API service initialization

// External declarations
extern AppConfig* app_config;

// Registry ID and cached readiness state
int api_subsystem_id = -1;
static LaunchReadiness cached_readiness = {0};
static bool readiness_cached = false;

// Forward declarations
static void clear_cached_readiness(void);
void register_api(void);

// Helper to clear cached readiness
static void clear_cached_readiness(void) {
    if (readiness_cached && cached_readiness.messages) {
        free_readiness_messages(&cached_readiness);
        readiness_cached = false;
    }
}

// Get cached readiness result
LaunchReadiness get_api_readiness(void) {
    if (readiness_cached) {
        return cached_readiness;
    }
    
    // Perform fresh check and cache result
    cached_readiness = check_api_launch_readiness();
    readiness_cached = true;
    return cached_readiness;
}

// Register the API subsystem with the registry
void register_api(void) {
    // Always register during readiness check if not already registered
    if (api_subsystem_id < 0) {
        api_subsystem_id = register_subsystem_from_launch("API", NULL, NULL, NULL,
                                            (int (*)(void))launch_api_subsystem,
                                            NULL);  // No special shutdown needed
        if (api_subsystem_id < 0) {
            log_this("API", "Failed to register API subsystem", LOG_LEVEL_ERROR);
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
 * Memory Management:
 * - On error paths: Messages are freed before returning
 * - On success path: Caller must free messages (typically handled by process_subsystem_readiness)
 * 
 * Note: Prefer using get_api_readiness() instead of calling this directly
 * to avoid redundant checks and potential memory leaks.
 * 
 * @return LaunchReadiness structure with readiness status and messages
 */
LaunchReadiness check_api_launch_readiness(void) {
    const char** messages = malloc(10 * sizeof(char*));  // Space for messages + NULL
    if (!messages) {
        return (LaunchReadiness){ .subsystem = "API", .ready = false, .messages = NULL };
    }
    
    int msg_index = 0;
    bool ready = true;
    
    // First message is subsystem name
    messages[msg_index++] = strdup("API");
    
    // Register with registry first
    register_api();
    if (api_subsystem_id < 0) {
        messages[msg_index++] = strdup("  No-Go:   Failed to register with registry");
        messages[msg_index++] = NULL;
        return (LaunchReadiness){ .subsystem = "API", .ready = false, .messages = messages };
    }
    
    LaunchReadiness readiness = { .subsystem = "API", .ready = false, .messages = messages };
    
    // 1. Check Registry subsystem readiness (primary dependency)
    LaunchReadiness registry_readiness = check_registry_launch_readiness();
    if (!registry_readiness.ready) {
        messages[msg_index++] = strdup("  No-Go:   Registry subsystem not Go for Launch");
        messages[msg_index] = NULL;
        readiness.ready = false;
        free_readiness_messages(&registry_readiness); // Free registry messages
        free_readiness_messages(&readiness);
        return readiness;
    }
    messages[msg_index++] = strdup("  Go:      Registry subsystem Go for Launch");
    free_readiness_messages(&registry_readiness); // Free registry messages
    
    // 2. Check Network subsystem readiness (using cached version)
    LaunchReadiness network_readiness = get_network_readiness();
    if (!network_readiness.ready) {
        messages[msg_index++] = strdup("  No-Go:   Network subsystem not Go for Launch");
        messages[msg_index] = NULL;
        readiness.ready = false;
        free_readiness_messages(&readiness);
        return readiness;
    }
    messages[msg_index++] = strdup("  Go:      Network subsystem Go for Launch");
    
    // 3. Check WebServer subsystem launch readiness (using cached version)
    LaunchReadiness webserver_readiness = get_webserver_readiness();
    if (!webserver_readiness.ready) {
        messages[msg_index++] = strdup("  No-Go:   WebServer subsystem not Go for Launch");
        messages[msg_index] = NULL;
        readiness.ready = false;
        free_readiness_messages(&readiness);
        return readiness;
    } else {
        messages[msg_index++] = strdup("  Go:      WebServer subsystem Go for Launch");
    }
    
    // 4. Check our configuration
    if (!app_config) {
        messages[msg_index++] = strdup("  No-Go:   API configuration not loaded");
        ready = false;
    } else {
        // Check if API is enabled
        if (!app_config->api.enabled) {
            messages[msg_index++] = strdup("  No-Go:   API endpoints disabled in configuration");
            ready = false;
        } else {
            messages[msg_index++] = strdup("  Go:      API endpoints enabled");
        }
        
        // Check API prefix
        if (!app_config->api.prefix || !app_config->api.prefix[0] || 
            app_config->api.prefix[0] != '/') {
            messages[msg_index++] = strdup("  No-Go:   Invalid API prefix configuration");
            ready = false;
        } else {
            char* msg = malloc(256);
            if (msg) {
                snprintf(msg, 256, "  Go:      Valid API prefix: %s", 
                        app_config->api.prefix);
                messages[msg_index++] = msg;
            }
        }

        // Check JWT secret
        if (!app_config->api.jwt_secret || !app_config->api.jwt_secret[0]) {
            messages[msg_index++] = strdup("  No-Go:   JWT secret not configured");
            ready = false;
        } else {
            messages[msg_index++] = strdup("  Go:      JWT secret configured");
        }
    }
    
    // Final decision
    messages[msg_index++] = strdup(ready ? 
        "  Decide:  Go For Launch of API Subsystem" :
        "  Decide:  No-Go For Launch of API Subsystem");
    messages[msg_index] = NULL;
    
    readiness.ready = ready;
    readiness.messages = messages;
    return readiness;
}

// Launch API subsystem
int launch_api_subsystem(void) {
    extern volatile sig_atomic_t server_stopping;
    extern volatile sig_atomic_t server_starting;
    // Clear any cached readiness before starting
    clear_cached_readiness();
    log_this("API", LOG_LINE_BREAK, LOG_LEVEL_STATE);
    log_this("API", "――――――――――――――――――――――――――――――――――――――――", LOG_LEVEL_STATE);
    log_this("API", "LAUNCH: API", LOG_LEVEL_STATE);

    // Step 1: Register with registry and add dependencies
    log_this("API", "  Step 1: Registering with registry", LOG_LEVEL_STATE);
    register_api();
    if (api_subsystem_id < 0) {
        log_this("API", "    Failed to register API subsystem", LOG_LEVEL_ERROR);
        log_this("API", "LAUNCH: API - Failed: Registration failed", LOG_LEVEL_STATE);
        return 0;
    }
    add_subsystem_dependency(api_subsystem_id, "Registry");
    add_subsystem_dependency(api_subsystem_id, "WebServer");
    log_this("API", "    Registration complete", LOG_LEVEL_STATE);

    // Step 2: Verify system state
    log_this("API", "  Step 2: Verifying system state", LOG_LEVEL_STATE);
    
    if (server_stopping) {
        log_this("API", "    Cannot initialize API during shutdown", LOG_LEVEL_STATE);
        log_this("API", "LAUNCH: API - Failed: System in shutdown", LOG_LEVEL_STATE);
        return 0;
    }

    if (!server_starting) {
        log_this("API", "    Cannot initialize API outside startup phase", LOG_LEVEL_STATE);
        log_this("API", "LAUNCH: API - Failed: Not in startup phase", LOG_LEVEL_STATE);
        return 0;
    }

    if (!app_config) {
        log_this("API", "    API configuration not loaded", LOG_LEVEL_STATE);
        log_this("API", "LAUNCH: API - Failed: No configuration", LOG_LEVEL_STATE);
        return 0;
    }

    if (!app_config->api.enabled) {
        log_this("API", "    API disabled in configuration", LOG_LEVEL_STATE);
        log_this("API", "LAUNCH: API - Disabled by configuration", LOG_LEVEL_STATE);
        return 1; // Not an error if disabled
    }

    log_this("API", "    System state verified", LOG_LEVEL_STATE);

    // Step 3: Verify dependencies
    log_this("API", "  Step 3: Verifying dependencies", LOG_LEVEL_STATE);
    
    // Check Registry first
    if (!is_subsystem_running_by_name("Registry")) {
        log_this("API", "    Registry not running", LOG_LEVEL_ERROR);
        log_this("API", "LAUNCH: API - Failed: Registry dependency not met", LOG_LEVEL_STATE);
        return 0;
    }
    log_this("API", "    Registry dependency verified", LOG_LEVEL_STATE);

    // Then check WebServer
    if (!is_web_server_running()) {
        log_this("API", "    Web server not running", LOG_LEVEL_ERROR);
        log_this("API", "LAUNCH: API - Failed: WebServer dependency not met", LOG_LEVEL_STATE);
        return 0;
    }
    log_this("API", "    WebServer dependency verified", LOG_LEVEL_STATE);
    log_this("API", "    All dependencies verified", LOG_LEVEL_STATE);

    // Step 4: Initialize API endpoints
    log_this("API", "  Step 3: Initializing API endpoints", LOG_LEVEL_STATE);
    
    // Initialize and register API endpoints
    if (!init_api_endpoints()) {
        log_this("API", "    Failed to initialize API endpoints", LOG_LEVEL_ERROR);
        log_this("API", "LAUNCH: API - Failed: Endpoint initialization failed", LOG_LEVEL_STATE);
        return 0;
    }
    
    log_this("API", "    API endpoints initialized successfully", LOG_LEVEL_STATE);

    // Step 5: Update registry and verify state
    log_this("API", "  Step 4: Updating subsystem registry", LOG_LEVEL_STATE);
    update_subsystem_on_startup("API", true);
    
    SubsystemState final_state = get_subsystem_state(api_subsystem_id);
    if (final_state == SUBSYSTEM_RUNNING) {
        log_this("API", "LAUNCH: API - Successfully launched and running", LOG_LEVEL_STATE);
    } else {
        log_this("API", "LAUNCH: API - Warning: Unexpected final state: %s", LOG_LEVEL_ALERT,
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

// Shutdown API subsystem
void shutdown_api(void) {
    log_this("API", LOG_LINE_BREAK, LOG_LEVEL_STATE);
    log_this("API", "LANDING: API", LOG_LEVEL_STATE);

    // Step 1: Verify state
    log_this("API", "  Step 1: Verifying state", LOG_LEVEL_STATE);
    if (!is_api_running()) {
        log_this("API", "API already shut down", LOG_LEVEL_STATE);
        log_this("API", "LANDING: API - Already landed", LOG_LEVEL_STATE);
        return;
    }
    log_this("API", "    State verified", LOG_LEVEL_STATE);

    // Step 2: Clean up API resources
    log_this("API", "  Step 2: Cleaning up API resources", LOG_LEVEL_STATE);
    
    // Clean up API endpoints
    cleanup_api_endpoints();
    log_this("API", "    API endpoints cleaned up", LOG_LEVEL_STATE);

    // Step 3: Update registry state
    log_this("API", "  Step 3: Updating registry state", LOG_LEVEL_STATE);
    update_subsystem_on_shutdown("API");
    
    log_this("API", "LANDING: API - Successfully landed", LOG_LEVEL_STATE);
}
