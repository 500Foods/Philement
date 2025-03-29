/*
 * Logging Subsystem Launch Implementation
 */

#include <stdbool.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "launch-logging.h"
#include "../logging/logging.h"
#include "../state/registry/subsystem_registry.h"
#include "../state/state.h"
#include "../config/config.h"

// External system state flags
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t server_running;

// Shutdown flag
volatile sig_atomic_t logging_stopping = 0;

// Helper functions for message formatting
static void add_message(const char** messages, int* count, const char* message) {
    if (message) {
        messages[*count] = message;
        (*count)++;
    }
}

static void add_go_message(const char** messages, int* count, const char* prefix, const char* format, ...) {
    char temp[256];
    va_list args;
    va_start(args, format);
    vsnprintf(temp, sizeof(temp), format, args);
    va_end(args);
    
    char buffer[512];
    if (strcmp(prefix, "No-Go") == 0) {
        snprintf(buffer, sizeof(buffer), "  %s:   %s", prefix, temp);
    } else {
        snprintf(buffer, sizeof(buffer), "  %s:      %s", prefix, temp);
    }
    
    add_message(messages, count, strdup(buffer));
}

static void add_decision_message(const char** messages, int* count, const char* format, ...) {
    char temp[256];
    va_list args;
    va_start(args, format);
    vsnprintf(temp, sizeof(temp), format, args);
    va_end(args);
    
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "  Decide:  %s", temp);
    add_message(messages, count, strdup(buffer));
}

// Check logging subsystem launch readiness
LaunchReadiness check_logging_launch_readiness(void) {
    bool overall_readiness = false;
    
    // Allocate space for messages
    const char** messages = malloc(10 * sizeof(const char*));
    if (!messages) {
        return (LaunchReadiness){0};
    }
    int msg_count = 0;
    
    // Add the subsystem name as the first message
    add_message(messages, &msg_count, strdup("Logging"));
    
    // Early return cases with cleanup
    if (server_stopping) {
        add_go_message(messages, &msg_count, "No-Go", "System shutdown in progress");
        messages[msg_count] = NULL;
        return (LaunchReadiness){
            .subsystem = "Logging",
            .ready = false,
            .messages = messages
        };
    }
    
    if (!server_starting && !server_running) {
        add_go_message(messages, &msg_count, "No-Go", "System not in startup or running state");
        messages[msg_count] = NULL;
        return (LaunchReadiness){
            .subsystem = "Logging",
            .ready = false,
            .messages = messages
        };
    }
    
    const AppConfig* app_config = get_app_config();
    if (!app_config) {
        add_go_message(messages, &msg_count, "No-Go", "Configuration not loaded");
        messages[msg_count] = NULL;
        return (LaunchReadiness){
            .subsystem = "Logging",
            .ready = false,
            .messages = messages
        };
    }
    
    // Basic readiness check - just verify we can get a subsystem ID
    int subsystem_id = get_subsystem_id_by_name("Logging");
    if (subsystem_id >= 0) {
        add_go_message(messages, &msg_count, "Go", "Logging subsystem registered");
        add_decision_message(messages, &msg_count, "Go For Launch of Logging Subsystem");
        overall_readiness = true;
    } else {
        add_go_message(messages, &msg_count, "No-Go", "Logging subsystem not registered");
        add_decision_message(messages, &msg_count, "No-Go For Launch of Logging Subsystem");
        overall_readiness = false;
    }
    
    messages[msg_count] = NULL;
    return (LaunchReadiness){
        .subsystem = "Logging",
        .ready = overall_readiness,
        .messages = messages
    };
}

// Launch the logging subsystem
int launch_logging_subsystem(void) {
    // Reset shutdown flag
    logging_stopping = 0;
    
    log_this("Logging", "Initializing logging subsystem", LOG_LEVEL_STATE);
    
    // Get subsystem ID and update state
    int subsystem_id = get_subsystem_id_by_name("Logging");
    if (subsystem_id >= 0) {
        update_subsystem_state(subsystem_id, SUBSYSTEM_RUNNING);
        log_this("Logging", "Logging subsystem initialized", LOG_LEVEL_STATE);
        return 1;
    }
    
    log_this("Logging", "Failed to initialize logging subsystem", LOG_LEVEL_ERROR);
    return 0;
}

// Shutdown handler - defined in launch-logging.h, implemented here
void shutdown_logging(void) {
    if (!logging_stopping) {
        logging_stopping = 1;
        log_this("Logging", "Logging subsystem shutting down", LOG_LEVEL_STATE);
    }
}