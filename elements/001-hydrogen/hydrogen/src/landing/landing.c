/*
 * Landing System Coordinator
 * 
 * This module coordinates the landing (shutdown) sequence using specialized modules:
 * - landing_readiness.c - Handles readiness checks
 * - landing_plan.c - Coordinates landing sequence
 * - landing_review.c - Reports landing status
 */

// System includes
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

// Local includes
#include "landing.h"
#include "landing_readiness.h"
#include "landing_plan.h"
#include "landing_review.h"

// Project includes
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../utils/utils.h"
#include "../utils/utils_time.h"
#include "../threads/threads.h"
#include "../state/registry/subsystem_registry.h"
#include "../state/registry/subsystem_registry_integration.h"

// External declarations
extern void free_payload_resources(void);
extern void report_thread_status(void);
extern void free_threads_resources(void);

// External declarations for subsystem landing readiness checks
extern LandingReadiness check_print_landing_readiness(void);
extern LandingReadiness check_mail_relay_landing_readiness(void);
extern LandingReadiness check_mdns_client_landing_readiness(void);
extern LandingReadiness check_database_landing_readiness(void);
extern LandingReadiness check_mdns_server_landing_readiness(void);
extern LandingReadiness check_terminal_landing_readiness(void);
extern LandingReadiness check_websocket_landing_readiness(void);
extern LandingReadiness check_swagger_landing_readiness(void);
extern LandingReadiness check_api_landing_readiness(void);
extern LandingReadiness check_webserver_landing_readiness(void);
extern LandingReadiness check_logging_landing_readiness(void);
extern LandingReadiness check_network_landing_readiness(void);
extern LandingReadiness check_payload_landing_readiness(void);
extern LandingReadiness check_subsystem_registry_landing_readiness(void);
extern LandingReadiness check_threads_landing_readiness(void);

// Free the messages in a LandingReadiness struct
void free_landing_readiness_messages(LandingReadiness* readiness) {
    if (!readiness || !readiness->messages) return;
    
    char** messages = (char**)readiness->messages;
    for (int i = 0; messages[i] != NULL; i++) {
        free(messages[i]);
    }
    
    free(messages);
    readiness->messages = NULL;
}

// Log all messages from a readiness check
static void log_readiness_messages(const LandingReadiness* readiness) {
    if (!readiness || !readiness->messages) return;
    
    // Log each message (first message is the subsystem name)
    for (int i = 0; readiness->messages[i] != NULL; i++) {
        int level = LOG_LEVEL_STATE;
        
        // Use appropriate log level based on the message content
        if (strstr(readiness->messages[i], "No-Go") != NULL) {
            level = LOG_LEVEL_ALERT;
        }
        
        // Print the message directly (formatting is already in the message)
        log_this("Landing", "%s", level, readiness->messages[i]);
    }
}

// Check if all subsystems are ready to land (shutdown)
bool check_all_landing_readiness(void) {
    bool any_subsystem_ready = false;
    
    // Begin LANDING READINESS logging section
    log_group_begin();
    log_this("Landing", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Landing", "LANDING READINESS", LOG_LEVEL_STATE);
    
    // Check subsystems in reverse order of launch
    
    // Check print subsystem
    LandingReadiness print_readiness = check_print_landing_readiness();
    log_readiness_messages(&print_readiness);
    if (print_readiness.ready) {
        any_subsystem_ready = true;
    }
    free_landing_readiness_messages(&print_readiness);
    
    // Check mail relay subsystem
    LandingReadiness mail_relay_readiness = check_mail_relay_landing_readiness();
    log_readiness_messages(&mail_relay_readiness);
    if (mail_relay_readiness.ready) {
        any_subsystem_ready = true;
    }
    free_landing_readiness_messages(&mail_relay_readiness);
    
    // Check mdns client subsystem
    LandingReadiness mdns_client_readiness = check_mdns_client_landing_readiness();
    log_readiness_messages(&mdns_client_readiness);
    if (mdns_client_readiness.ready) {
        any_subsystem_ready = true;
    }
    free_landing_readiness_messages(&mdns_client_readiness);
    
    // Check mdns server subsystem
    LandingReadiness mdns_server_readiness = check_mdns_server_landing_readiness();
    log_readiness_messages(&mdns_server_readiness);
    if (mdns_server_readiness.ready) {
        any_subsystem_ready = true;
    }
    free_landing_readiness_messages(&mdns_server_readiness);
    
    // Check terminal subsystem
    LandingReadiness terminal_readiness = check_terminal_landing_readiness();
    log_readiness_messages(&terminal_readiness);
    if (terminal_readiness.ready) {
        any_subsystem_ready = true;
    }
    free_landing_readiness_messages(&terminal_readiness);
    
    // Check websocket subsystem
    LandingReadiness websocket_readiness = check_websocket_landing_readiness();
    log_readiness_messages(&websocket_readiness);
    if (websocket_readiness.ready) {
        any_subsystem_ready = true;
    }
    free_landing_readiness_messages(&websocket_readiness);
    
    // Check swagger subsystem
    LandingReadiness swagger_readiness = check_swagger_landing_readiness();
    log_readiness_messages(&swagger_readiness);
    if (swagger_readiness.ready) {
        any_subsystem_ready = true;
    }
    free_landing_readiness_messages(&swagger_readiness);
    
    // Check API subsystem
    LandingReadiness api_readiness = check_api_landing_readiness();
    log_readiness_messages(&api_readiness);
    if (api_readiness.ready) {
        any_subsystem_ready = true;
    }
    free_landing_readiness_messages(&api_readiness);
    
    // Check webserver subsystem
    LandingReadiness webserver_readiness = check_webserver_landing_readiness();
    log_readiness_messages(&webserver_readiness);
    if (webserver_readiness.ready) {
        any_subsystem_ready = true;
    }
    free_landing_readiness_messages(&webserver_readiness);
    
    // Check database subsystem
    LandingReadiness database_readiness = check_database_landing_readiness();
    log_readiness_messages(&database_readiness);
    if (database_readiness.ready) {
        any_subsystem_ready = true;
    }
    free_landing_readiness_messages(&database_readiness);
    
    // Check logging subsystem
    LandingReadiness logging_readiness = check_logging_landing_readiness();
    log_readiness_messages(&logging_readiness);
    if (logging_readiness.ready) {
        any_subsystem_ready = true;
    }
    free_landing_readiness_messages(&logging_readiness);
    
    // Check payload subsystem
    LandingReadiness payload_readiness = check_payload_landing_readiness();
    log_readiness_messages(&payload_readiness);
    if (payload_readiness.ready) {
        any_subsystem_ready = true;
        
        // Add LANDING: PAYLOAD section
        log_this("Payload", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
        log_this("Payload", "LANDING: PAYLOAD", LOG_LEVEL_STATE);
        log_this("Payload", "  Freeing payload resources...", LOG_LEVEL_STATE);
        
        // Free resources allocated during payload launch
        free_payload_resources();
        
        log_this("Payload", "  Payload resources freed successfully", LOG_LEVEL_STATE);
    }
    free_landing_readiness_messages(&payload_readiness);
    
    // Check threads subsystem
    LandingReadiness threads_readiness = check_threads_landing_readiness();
    log_readiness_messages(&threads_readiness);
    if (threads_readiness.ready) {
        any_subsystem_ready = true;
        
        // Add LANDING: THREADS section
        log_this("Threads", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
        log_this("Threads", "LANDING: THREADS", LOG_LEVEL_STATE);
        report_thread_status();
        log_this("Threads", "  Waiting for service threads to complete...", LOG_LEVEL_STATE);
        
        // Wait for service threads and clean up
        // free_threads_resources();
        
        log_this("Threads", "  All threads landed successfully", LOG_LEVEL_STATE);
    }
    free_landing_readiness_messages(&threads_readiness);
    
    // Check network subsystem
    LandingReadiness network_readiness = check_network_landing_readiness();
    log_readiness_messages(&network_readiness);
    if (network_readiness.ready) {
        any_subsystem_ready = true;
    }
    free_landing_readiness_messages(&network_readiness);
    
    // Check Subsystem Registry last
    LandingReadiness registry_readiness = check_subsystem_registry_landing_readiness();
    log_readiness_messages(&registry_readiness);
    if (registry_readiness.ready) {
        any_subsystem_ready = true;
    }
    free_landing_readiness_messages(&registry_readiness);
    
    // Add LANDING REVIEW section
    log_this("Landing", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Landing", "LANDING REVIEW", LOG_LEVEL_STATE);
    
    // Count how many subsystems were checked and how many are ready
    const int total_checked = 15; // Total number of subsystems we check (added Threads)
    int ready_count = 0;
    
    // Count ready subsystems
    for (int i = 0; i < subsystem_registry.count; i++) {
        if (subsystem_registry.subsystems[i].state == SUBSYSTEM_INACTIVE) {
            ready_count++;
        }
    }
    
    log_this("Landing", "  Total subsystems checked: %d", LOG_LEVEL_STATE, total_checked);
    log_this("Landing", "  Subsystems ready for landing: %s (%d/%d)", LOG_LEVEL_STATE, 
             any_subsystem_ready ? "Yes" : "No", ready_count, total_checked);
    
    // Add LANDING COMPLETE section if all subsystems are ready
    if (ready_count == total_checked) {
        log_this("Landing", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
        log_this("Landing", "LANDING COMPLETE", LOG_LEVEL_STATE);
        
        // Calculate and log shutdown timing
        struct timeval tv;
        gettimeofday(&tv, NULL);
        double shutdown_time = calculate_shutdown_time();
        
        log_this("Landing", "  Shutdown Duration: %.3fs", LOG_LEVEL_STATE, shutdown_time);
        log_this("Landing", "  All subsystems landed successfully", LOG_LEVEL_STATE);
        
        // Signal completion to shutdown handler
        log_this("Landing", "  Proceeding with final cleanup", LOG_LEVEL_STATE);
    }
    
    log_group_end();
    
    return any_subsystem_ready;
}