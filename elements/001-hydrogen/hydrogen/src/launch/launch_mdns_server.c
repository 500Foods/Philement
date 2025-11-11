/*
 * Launch mDNS Server Subsystem
 * 
 * This module handles the initialization of the mDNS server subsystem.
 * It provides functions for checking readiness and launching the mDNS server.
 * 
 * Dependencies:
 * - Network subsystem must be initialized and ready
 * 
 * Note: Shutdown functionality has been moved to landing/landing_mdns_server.c
 */

// Global includes
#include <src/hydrogen.h>

// Local includes
#include "launch.h"
#include <src/state/state_types.h>
#include <src/mdns/mdns_server.h>

// External declarations
extern ServiceThreads mdns_server_threads;

// mDNS server subsystem global variables
static mdns_server_t* mdns_server_instance = NULL;
static pthread_t mdns_server_announce_thread = 0;
static pthread_t mdns_server_responder_thread = 0;

// Registry ID and cached readiness state
int mdns_server_subsystem_id = -1;

// Forward declarations
bool validate_mdns_server_configuration(const char*** messages, size_t* count, size_t* capacity);

// Register the mDNS server subsystem with the registry (for launch)
void register_mdns_server_for_launch(void) {
    // Always register during readiness check if not already registered
    if (mdns_server_subsystem_id < 0) {
        mdns_server_subsystem_id = register_subsystem_from_launch(SR_MDNS_SERVER, &mdns_server_threads, NULL,
                                                                &mdns_server_system_shutdown,
                                                                (int (*)(void))launch_mdns_server_subsystem,
                                                                (void (*)(void))mdns_server_shutdown);
    }
}

// Check if the mDNS server subsystem is ready to launch
LaunchReadiness check_mdns_server_launch_readiness(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool ready = true;

    // First message is subsystem name
    add_launch_message(&messages, &count, &capacity, strdup(SR_MDNS_SERVER));

    // Register with subsystem registry for launch
    register_mdns_server_for_launch();

    // Register dependency on Network subsystem
    int mdns_id = get_subsystem_id_by_name(SR_MDNS_SERVER);
    if (mdns_id >= 0) {
        if (!add_dependency_from_launch(mdns_id, SR_NETWORK)) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to register Network dependency"));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){ .subsystem = SR_MDNS_SERVER, .ready = false, .messages = messages };
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Network dependency registered"));

        // Verify Network subsystem is ready to launch
        if (!is_subsystem_launchable_by_name(SR_NETWORK)) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Network subsystem not launchable"));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){ .subsystem = SR_MDNS_SERVER, .ready = false, .messages = messages };
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Network subsystem will be available"));
    }

    // Validate configuration - extracted to separate function for testability
    if (!validate_mdns_server_configuration(&messages, &count, &capacity)) {
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_MDNS_SERVER, .ready = false, .messages = messages };
    }

    // All checks passed
    add_launch_message(&messages, &count, &capacity, strdup("  Decide:  Go For Launch of mDNS Server Subsystem"));

    finalize_launch_messages(&messages, &count, &capacity);

    return (LaunchReadiness){
        .subsystem = SR_MDNS_SERVER,
        .ready = ready,
        .messages = messages
    };
}

// Validate mDNS server configuration (extracted for testability)
bool validate_mdns_server_configuration(const char*** messages, size_t* count, size_t* capacity) {
    // Check basic configuration
    if (!app_config || !(app_config->mdns_server.enable_ipv4 || app_config->mdns_server.enable_ipv6)) {
        add_launch_message(messages, count, capacity, strdup("  No-Go:   mDNS server disabled in configuration"));
        return false;
    }
    add_launch_message(messages, count, capacity, strdup("  Go:      mDNS server enabled in configuration"));

    // Validate required configuration fields
    const MDNSServerConfig* config = &app_config->mdns_server;

    if (!config->device_id || !config->device_id[0]) {
        add_launch_message(messages, count, capacity, strdup("  No-Go:   Device ID is required"));
        return false;
    }
    add_launch_message(messages, count, capacity, strdup("  Go:      Device ID configured"));

    if (!config->friendly_name || !config->friendly_name[0]) {
        add_launch_message(messages, count, capacity, strdup("  No-Go:   Friendly name is required"));
        return false;
    }
    add_launch_message(messages, count, capacity, strdup("  Go:      Friendly name configured"));

    if (!config->model || !config->model[0]) {
        add_launch_message(messages, count, capacity, strdup("  No-Go:   Model is required"));
        return false;
    }
    add_launch_message(messages, count, capacity, strdup("  Go:      Model configured"));

    if (!config->manufacturer || !config->manufacturer[0]) {
        add_launch_message(messages, count, capacity, strdup("  No-Go:   Manufacturer is required"));
        return false;
    }
    add_launch_message(messages, count, capacity, strdup("  Go:      Manufacturer configured"));

    if (!config->version || !config->version[0]) {
        add_launch_message(messages, count, capacity, strdup("  No-Go:   Version is required"));
        return false;
    }
    add_launch_message(messages, count, capacity, strdup("  Go:      Version configured"));

    // Validate services array if present
    if (config->num_services > 0) {
        if (!config->services) {
            add_launch_message(messages, count, capacity, strdup("  No-Go:   Services array is NULL but count is non-zero"));
            return false;
        }
        add_launch_message(messages, count, capacity, strdup("  Go:      Services array allocated"));

        // Validate each service
        for (size_t i = 0; i < config->num_services; i++) {
            const mdns_server_service_t* service = &config->services[i];
            char msg_buffer[256];

            if (!service->name || !service->name[0]) {
                snprintf(msg_buffer, sizeof(msg_buffer), "  No-Go:   Service %zu name is required", i + 1);
                add_launch_message(messages, count, capacity, strdup(msg_buffer));
                return false;
            }

            if (!service->type || !service->type[0]) {
                snprintf(msg_buffer, sizeof(msg_buffer), "  No-Go:   Service %zu type is required", i + 1);
                add_launch_message(messages, count, capacity, strdup(msg_buffer));
                return false;
            }

            if (service->port <= 0 || service->port > 65535) {
                snprintf(msg_buffer, sizeof(msg_buffer), "  No-Go:   Service %zu has invalid port number", i + 1);
                add_launch_message(messages, count, capacity, strdup(msg_buffer));
                return false;
            }

            if (service->num_txt_records > 0 && !service->txt_records) {
                snprintf(msg_buffer, sizeof(msg_buffer), "  No-Go:   Service %zu TXT records array is NULL but count is non-zero", i + 1);
                add_launch_message(messages, count, capacity, strdup(msg_buffer));
                return false;
            }

            snprintf(msg_buffer, sizeof(msg_buffer), "  Go:      Service %zu validated", i + 1);
            add_launch_message(messages, count, capacity, strdup(msg_buffer));
        }
    }

    return true;
}

// Launch the mDNS server subsystem
int launch_mdns_server_subsystem(void) {
    
    // Step 1: Log Launch
    log_this(SR_MDNS_SERVER, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    log_this(SR_MDNS_SERVER, "LAUNCH: " SR_MDNS_SERVER, LOG_LEVEL_STATE, 0);

    // Step 2: Reset shutdown state

    // Step 3: Verify system state
    if (server_stopping || server_starting != 1) {
        log_this(SR_MDNS_SERVER, "Cannot initialize " SR_MDNS_SERVER " subsystem outside startup phase", LOG_LEVEL_STATE, 0);
        log_this(SR_MDNS_SERVER, "LAUNCH: " SR_MDNS_SERVER " Failed: Not in startup phase", LOG_LEVEL_STATE, 0);
        return 0;
    }

    // Step 4: Verify config state
    if (!app_config) {
        log_this(SR_MDNS_SERVER, "Configuration not loaded", LOG_LEVEL_ERROR, 0);
        log_this(SR_MDNS_SERVER, "LAUNCH: " SR_MDNS_SERVER " Failed: No configuration", LOG_LEVEL_STATE, 0);
        return 0;
    }

    // Initialize subsystem instance
    log_this(SR_MDNS_SERVER, "Initializing " SR_MDNS_SERVER " with device configuration", LOG_LEVEL_STATE, 0);

    // Get server configuration
    const MDNSServerConfig* server_config = &app_config->mdns_server;

    // Initialize mDNS server
    mdns_server_instance = mdns_server_init(
        "Hydrogen",                           // Application name
        server_config->device_id,             // Unique device ID
        server_config->friendly_name,         // Human-readable name
        server_config->model,                 // Device model
        server_config->manufacturer,          // Manufacturer
        VERSION,                              // Software version
        "1.0.0",                              // Hardware version (placeholder)
        "/config",                            // Config URL (placeholder)
        server_config->services,              // Services array
        server_config->num_services,         // Number of services
        server_config->enable_ipv6           // IPv6 support flag
    );

    if (!mdns_server_instance) {
        log_this(SR_MDNS_SERVER, "Failed to initialize " SR_MDNS_SERVER, LOG_LEVEL_ERROR, 0);
        log_this(SR_MDNS_SERVER, "LAUNCH: " SR_MDNS_SERVER " Failed to initialize", LOG_LEVEL_STATE, 0);
        return 0;
    }

    // Initialize and start threads
    log_this(SR_MDNS_SERVER, "Starting " SR_MDNS_SERVER" background threads", LOG_LEVEL_STATE, 0);

    // Create thread argument structure
    mdns_server_thread_arg_t *thread_arg = calloc(1, sizeof(mdns_server_thread_arg_t));
    if (!thread_arg) {
        log_this(SR_MDNS_SERVER, "Failed to allocate thread arguments", LOG_LEVEL_ERROR, 0);
        mdns_server_shutdown(mdns_server_instance);
        return 0;
    }

    thread_arg->mdns_server = mdns_server_instance;
    thread_arg->port = 0; // For future use
    thread_arg->net_info = NULL; // Thread will get network info when needed
    thread_arg->running = &mdns_server_system_shutdown;

    // Initialize mDNS server thread structure
    init_service_threads(&mdns_server_threads, SR_MDNS_SERVER);

    // Create announcer thread
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);

    if (pthread_create(&mdns_server_announce_thread, &thread_attr, mdns_server_announce_loop, thread_arg) != 0) {
        log_this(SR_MDNS_SERVER, "Failed to start " SR_MDNS_SERVER " announcer thread", LOG_LEVEL_ERROR, 0);
        pthread_attr_destroy(&thread_attr);
        free(thread_arg);
        mdns_server_shutdown(mdns_server_instance);
        return 0;
    }

    // Create responder thread
    if (pthread_create(&mdns_server_responder_thread, &thread_attr, mdns_server_responder_loop, thread_arg) != 0) {
        log_this(SR_MDNS_SERVER, "Failed to start " SR_MDNS_SERVER " responder thread", LOG_LEVEL_ERROR, 0);
        mdns_server_system_shutdown = 1; 
        pthread_join(mdns_server_announce_thread, NULL);
        pthread_attr_destroy(&thread_attr);
        free(thread_arg);
        mdns_server_shutdown(mdns_server_instance);
        return 0;
    }

    pthread_attr_destroy(&thread_attr);

    // Register thread references
    add_service_thread(&mdns_server_threads, mdns_server_announce_thread);
    add_service_thread(&mdns_server_threads, mdns_server_responder_thread);

    // Send initial announcements
    log_this(SR_MDNS_SERVER, "Sending initial " SR_MDNS_SERVER " announcements", LOG_LEVEL_STATE, 0);
    mdns_server_send_announcement(mdns_server_instance, thread_arg->net_info);

    // Update subsystem registry
    log_this(SR_MDNS_SERVER, "Updating subsystem registry", LOG_LEVEL_STATE, 0);
    update_subsystem_on_startup(SR_MDNS_SERVER, true);

    // Verify final state
    SubsystemState final_state = get_subsystem_state(mdns_server_subsystem_id);

    if (final_state == SUBSYSTEM_RUNNING) {
        log_this(SR_MDNS_SERVER, "LAUNCH: " SR_MDNS_SERVER "Success: Launched and running", LOG_LEVEL_STATE, 0);
        return 1;
    } else {
        log_this(SR_MDNS_SERVER, "LAUNCH: " SR_MDNS_SERVER" Warning: Unexpected final state: %s", LOG_LEVEL_ALERT, 1, subsystem_state_to_string(final_state));
        return 0;
    }
}
