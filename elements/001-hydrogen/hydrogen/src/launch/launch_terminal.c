/*
 * Launch Terminal Subsystem
 * 
 * This module handles the initialization of the terminal subsystem.
 * It provides functions for checking readiness and launching the terminal.
 * 
 * Dependencies:
 * - WebServer subsystem must be initialized and ready
 * - WebSockets subsystem must be initialized and ready
 */

// Global includes
#include <src/hydrogen.h>

// Local includes
#include "launch.h"
#include <src/config/config_terminal.h>       // For TerminalConfig
#include <src/terminal/terminal.h>            // For terminal functions
#include <src/webserver/web_server_core.h>   // For WebServerEndpoint and register_web_endpoint
#include <src/payload/payload_cache.h>        // For payload cache functions

// Registry ID for the Terminal subsystem
int terminal_subsystem_id = -1;

// Check if the terminal subsystem is ready to launch
LaunchReadiness check_terminal_launch_readiness(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool is_ready = true;

    // First message is subsystem name
    add_launch_message(&messages, &count, &capacity, strdup(SR_TERMINAL));

    // Register with registry first
    if (terminal_subsystem_id < 0) {
        terminal_subsystem_id = register_subsystem_from_launch(SR_TERMINAL, NULL, NULL, NULL,
                                                (int (*)(void))launch_terminal_subsystem,
                                                NULL);  // No special shutdown needed
        if (terminal_subsystem_id < 0) {
            log_this(SR_TERMINAL, "Failed to register Terminal subsystem", LOG_LEVEL_ERROR, 0);
        }
    }

    // Check if registration succeeded
    if (terminal_subsystem_id < 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to register with registry"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_TERMINAL, .ready = false, .messages = messages };
    }

    // Check dependencies first - handle NULL config gracefully
    if (!app_config || (!app_config->webserver.enable_ipv4 && !app_config->webserver.enable_ipv6)) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   WebServer Not Enabled"));
        add_launch_message(&messages, &count, &capacity, strdup("  Reason:  Terminal Requires WebServer (IPv4 or IPv6)"));
        is_ready = false;
    }

    if (!app_config || (!app_config->websocket.enable_ipv4 && !app_config->websocket.enable_ipv6)) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   WebSocket Not Enabled"));
        add_launch_message(&messages, &count, &capacity, strdup("  Reason:  Terminal Requires WebSocket"));
        is_ready = false;
    }

    // Check if terminal is enabled - handle NULL config gracefully
    if (!app_config || !app_config->terminal.enabled) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Terminal System Disabled"));
        add_launch_message(&messages, &count, &capacity, strdup("  Reason:  Disabled in Configuration"));
        is_ready = false;
    } else {
        // Validate required strings
        if (!app_config->terminal.web_path) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Missing Web Path"));
            add_launch_message(&messages, &count, &capacity, strdup("  Reason:  Web Path Must Be Set"));
            is_ready = false;
        }

        if (!app_config->terminal.shell_command) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Missing Shell Command"));
            add_launch_message(&messages, &count, &capacity, strdup("  Reason:  Shell Command Must Be Set"));
            is_ready = false;
        }

        // Validate numeric ranges
        if (app_config->terminal.max_sessions < 1 || app_config->terminal.max_sessions > 100) {
            char msg[128];
            snprintf(msg, sizeof(msg), "  No-Go:   Invalid Max Sessions: %d",
                    app_config->terminal.max_sessions);
            add_launch_message(&messages, &count, &capacity, strdup(msg));
            add_launch_message(&messages, &count, &capacity, strdup("  Reason:  Must Be Between 1 and 100"));
            is_ready = false;
        }

        if (app_config->terminal.idle_timeout_seconds < 60 ||
            app_config->terminal.idle_timeout_seconds > 3600) {
            char msg[128];
            snprintf(msg, sizeof(msg), "  No-Go:   Invalid Idle Timeout: %d",
                    app_config->terminal.idle_timeout_seconds);
            add_launch_message(&messages, &count, &capacity, strdup(msg));
            add_launch_message(&messages, &count, &capacity, strdup("  Reason:  Must Be Between 60 and 3600 Seconds"));
            is_ready = false;
        }
    }

    // Add final decision message
    if (is_ready) {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Terminal System Ready"));
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of Terminal"));
    }

    finalize_launch_messages(&messages, &count, &capacity);

    return (LaunchReadiness){
        .subsystem = SR_TERMINAL,
        .ready = is_ready,
        .messages = messages
    };
}

// Launch the terminal subsystem
int launch_terminal_subsystem(void) {

    log_this(SR_TERMINAL, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_TERMINAL, "LAUNCH: " SR_TERMINAL, LOG_LEVEL_DEBUG, 0);

    // Step 1: Register with registry and add dependencies
    log_this(SR_TERMINAL, "  Step 1: Registering with registry", LOG_LEVEL_DEBUG, 0);

    if (terminal_subsystem_id < 0) {
        terminal_subsystem_id = register_subsystem_from_launch(SR_TERMINAL, NULL, NULL, NULL,
                                                (int (*)(void))launch_terminal_subsystem,
                                                NULL);  // No special shutdown needed
        if (terminal_subsystem_id < 0) {
            log_this(SR_TERMINAL, "LAUNCH: TERMINAL - Failed: Registration failed", LOG_LEVEL_ALERT, 0);
            return 0;
        }
    }
    add_subsystem_dependency(terminal_subsystem_id, "Registry");
    add_subsystem_dependency(terminal_subsystem_id, "WebServer");
    add_subsystem_dependency(terminal_subsystem_id, "WebSocket");
    add_subsystem_dependency(terminal_subsystem_id, "Payload");
    log_this(SR_TERMINAL, "    Registration complete", LOG_LEVEL_DEBUG, 0);

    // Step 2: Verify system state
    log_this(SR_TERMINAL, "  Step 2: Verifying system state", LOG_LEVEL_DEBUG, 0);

    if (server_stopping) {
        log_this(SR_TERMINAL, "    Cannot initialize Terminal during shutdown", LOG_LEVEL_DEBUG, 0);
        log_this(SR_TERMINAL, "LAUNCH: TERMINAL - Failed: System in shutdown", LOG_LEVEL_DEBUG, 0);
        return 0;
    }

    if (!server_starting) {
        log_this(SR_TERMINAL, "    Cannot initialize Terminal outside startup phase", LOG_LEVEL_DEBUG, 0);
        log_this(SR_TERMINAL, "LAUNCH: TERMINAL - Failed: Not in startup phase", LOG_LEVEL_DEBUG, 0);
        return 0;
    }

    if (!app_config) {
        log_this(SR_TERMINAL, "    Terminal configuration not loaded", LOG_LEVEL_DEBUG, 0);
        log_this(SR_TERMINAL, "LAUNCH: TERMINAL - Failed: No configuration", LOG_LEVEL_DEBUG, 0);
        return 0;
    }

    if (!app_config->terminal.enabled) {
        log_this(SR_TERMINAL, "    Terminal disabled in configuration", LOG_LEVEL_DEBUG, 0);
        log_this(SR_TERMINAL, "LAUNCH: TERMINAL - Disabled by configuration", LOG_LEVEL_DEBUG, 0);
        return 1; // Not an error if disabled
    }
    log_this(SR_TERMINAL, "    System state verified", LOG_LEVEL_DEBUG, 0);

    // Step 3: Verify dependencies
    log_this(SR_TERMINAL, "  Step 3: Verifying dependencies", LOG_LEVEL_DEBUG, 0);

    // Check Registry first
    if (!is_subsystem_running_by_name("Registry")) {
        log_this(SR_TERMINAL, "LAUNCH: TERMINAL - Failed: Registry dependency not met", LOG_LEVEL_ALERT, 0);
        return 0;
    }
    log_this(SR_TERMINAL, "    Registry dependency verified", LOG_LEVEL_DEBUG, 0);

    // Check WebServer subsystem
    if (!is_subsystem_running_by_name(SR_WEBSERVER)) {
        log_this(SR_TERMINAL, "LAUNCH: TERMINAL - Failed: " SR_WEBSERVER " dependency not met", LOG_LEVEL_ALERT, 0);
        return 0;
    }
    log_this(SR_TERMINAL, "    " SR_WEBSERVER " subsystem verified", LOG_LEVEL_DEBUG, 0);

    // Check WebSocket subsystem
    if (!is_subsystem_running_by_name(SR_WEBSOCKET)) {
        log_this(SR_TERMINAL, "LAUNCH: TERMINAL - Failed: " SR_WEBSOCKET " dependency not met", LOG_LEVEL_ALERT, 0);
        return 0;
    }
    log_this(SR_TERMINAL, "    " SR_WEBSOCKET " subsystem verified", LOG_LEVEL_DEBUG, 0);

    // Check Payload subsystem and verify terminal files are available
    int payload_id = get_subsystem_id_by_name("Payload");
    if (payload_id < 0 || get_subsystem_state(payload_id) != SUBSYSTEM_RUNNING) {
        log_this(SR_TERMINAL, "LAUNCH: TERMINAL - Failed: " SR_PAYLOAD " subsystem dependency not met", LOG_LEVEL_ALERT, 0);
        return 0;
    }

    // Check for terminal files in payload cache
    if (!is_payload_cache_available()) {
        log_this(SR_TERMINAL, "LAUNCH: TERMINAL - Failed: Payload cache not available", LOG_LEVEL_ALERT, 0);
        return 0;
    }

    // Verify terminal files are in cache
    PayloadFile *terminal_files = NULL;
    size_t num_terminal_files = 0;
    size_t capacity = 0;

    bool files_retrieved = get_payload_files_by_prefix("terminal/", &terminal_files, &num_terminal_files, &capacity);
    if (!files_retrieved || !terminal_files || num_terminal_files == 0) {
        log_this(SR_TERMINAL, "LAUNCH: TERMINAL - Failed: Missing Terminal UI files", LOG_LEVEL_ALERT, 0);
        return 0;
    }

    log_this(SR_TERMINAL, "    Terminal files verified (%zu files in cache):", LOG_LEVEL_DEBUG, 1, num_terminal_files);
    for (size_t i = 0; i < num_terminal_files; i++) {
        log_this(SR_TERMINAL, "      -> %s (%s)", LOG_LEVEL_DEBUG, 2,
                terminal_files[i].name,
                terminal_files[i].size < 1024 ?
                    terminal_files[i].size < 512 ? 
                        "small file" : "medium file" : "large file");
    }

    // Load terminal files into memory using init function
    // This replaces the payload cache loading we removed earlier
    TerminalConfig *mutable_terminal_config = (TerminalConfig*)&app_config->terminal;
    if (!init_terminal_support(mutable_terminal_config)) {
        log_this(SR_TERMINAL, "LAUNCH: TERMINAL - Failed: File loading failed", LOG_LEVEL_ALERT, 0);
        free(terminal_files);
        return 0;
    }

    // Free the retrieved files array (but not individual data - owned by payload cache now)
    free(terminal_files);
    log_this(SR_TERMINAL, "    All dependencies verified", LOG_LEVEL_DEBUG, 0);
    log_this(SR_TERMINAL, "    " SR_TERMINAL " subsystem initialized", LOG_LEVEL_DEBUG, 0);

    // Register Terminal endpoint with webserver
    WebServerEndpoint terminal_endpoint = {
        .prefix = app_config->terminal.web_path,
        .validator = terminal_url_validator,
        .handler = terminal_request_handler
    };

    if (!register_web_endpoint(&terminal_endpoint)) {
        log_this(SR_TERMINAL, "LAUNCH: TERMINAL - Failed: Endpoint registration failed", LOG_LEVEL_ERROR, 0);
        return 0;
    }

    // Log configuration
    log_this(SR_TERMINAL, "    Configuration:", LOG_LEVEL_DEBUG, 0);
    log_this(SR_TERMINAL, "      -> Enabled: yes", LOG_LEVEL_DEBUG, 0);
    log_this(SR_TERMINAL, "      -> Web Path: %s", LOG_LEVEL_DEBUG, 1, app_config->terminal.web_path);
    log_this(SR_TERMINAL, "      -> WebRoot: %s", LOG_LEVEL_DEBUG, 1, app_config->terminal.webroot);
    log_this(SR_TERMINAL, "      -> Shell: %s", LOG_LEVEL_DEBUG, 1, app_config->terminal.shell_command);
    log_this(SR_TERMINAL, "      -> Max Sessions: %d", LOG_LEVEL_DEBUG, 1, app_config->terminal.max_sessions);
    log_this(SR_TERMINAL, "      -> Payload: available", LOG_LEVEL_DEBUG, 0);
    log_this(SR_TERMINAL, "    " SR_TERMINAL " subsystem initialized", LOG_LEVEL_DEBUG, 0);

    // Step 5: Update registry and verify state
    log_this(SR_TERMINAL, "  Updating " SR_REGISTRY, LOG_LEVEL_DEBUG, 0);
    update_subsystem_on_startup(SR_TERMINAL, true);

    SubsystemState final_state = get_subsystem_state(terminal_subsystem_id);
    if (final_state == SUBSYSTEM_RUNNING) {
        log_this(SR_TERMINAL, "LAUNCH: TERMINAL Success: Launched and running", LOG_LEVEL_DEBUG, 0);
        return 1;
    } else {
        log_this(SR_TERMINAL, "LAUNCH: TERMINAL Warning: Unexpected final state: %s", LOG_LEVEL_ALERT, 1, subsystem_state_to_string(final_state));
        return 0;
    }
}
