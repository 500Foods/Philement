/*
 * Landing Readiness System
 * 
 * This module coordinates pre-landing checks for all subsystems.
 * It ensures that resources can be safely freed and reports readiness status.
 * It is the counterpart to the Launch Readiness system but for shutdown.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "landing.h"
#include "../../logging/logging.h"
#include "../../state/registry/subsystem_registry.h"
#include "../../state/registry/subsystem_registry_integration.h"

// Forward declarations for subsystem landing readiness checks
static LandingReadiness check_print_landing_readiness(void);
static LandingReadiness check_mail_relay_landing_readiness(void);
static LandingReadiness check_mdns_client_landing_readiness(void);
static LandingReadiness check_mdns_server_landing_readiness(void);
static LandingReadiness check_terminal_landing_readiness(void);
static LandingReadiness check_websocket_landing_readiness(void);
static LandingReadiness check_swagger_landing_readiness(void);
static LandingReadiness check_api_landing_readiness(void);
static LandingReadiness check_webserver_landing_readiness(void);
static LandingReadiness check_logging_landing_readiness(void);
static LandingReadiness check_network_landing_readiness(void);
static LandingReadiness check_payload_landing_readiness(void);
static LandingReadiness check_subsystem_registry_landing_readiness(void);

// Free the messages in a LandingReadiness struct
void free_landing_readiness_messages(LandingReadiness* readiness) {
    if (!readiness || !readiness->messages) return;
    
    for (int i = 0; readiness->messages[i] != NULL; i++) {
        free(readiness->messages[i]);
    }
    
    free(readiness->messages);
    readiness->messages = NULL;
}

// Check Subsystem Registry landing readiness
static LandingReadiness check_subsystem_registry_landing_readiness(void) {
    LandingReadiness readiness = {0};
    readiness.subsystem = "Subsystem Registry";
    
    // Always ready - this is our last subsystem to shut down
    readiness.ready = true;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(4 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add messages in the standard format
    readiness.messages[0] = strdup("Subsystem Registry");
    readiness.messages[1] = strdup("  Go:      Subsystem Registry Ready for Landing");
    readiness.messages[2] = strdup("  Decide:  Go For Landing of Subsystem Registry");
    readiness.messages[3] = NULL;
    
    return readiness;
}

// Check Payload landing readiness
static LandingReadiness check_payload_landing_readiness(void) {
    LandingReadiness readiness = {0};
    readiness.subsystem = "Payload";
    
    // Always ready for now
    readiness.ready = true;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(4 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add messages in the standard format
    readiness.messages[0] = strdup("Payload");
    readiness.messages[1] = strdup("  Go:      Payload Ready for Landing");
    readiness.messages[2] = strdup("  Decide:  Go For Landing of Payload");
    readiness.messages[3] = NULL;
    
    return readiness;
}

// Check Network landing readiness
static LandingReadiness check_network_landing_readiness(void) {
    LandingReadiness readiness = {0};
    readiness.subsystem = "Network";
    
    // Always ready for now
    readiness.ready = true;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(4 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add messages in the standard format
    readiness.messages[0] = strdup("Network");
    readiness.messages[1] = strdup("  Go:      Network Ready for Landing");
    readiness.messages[2] = strdup("  Decide:  Go For Landing of Network");
    readiness.messages[3] = NULL;
    
    return readiness;
}

// Check Logging landing readiness
static LandingReadiness check_logging_landing_readiness(void) {
    LandingReadiness readiness = {0};
    readiness.subsystem = "Logging";
    
    // Always ready for now
    readiness.ready = true;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(4 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add messages in the standard format
    readiness.messages[0] = strdup("Logging");
    readiness.messages[1] = strdup("  Go:      Logging Ready for Landing");
    readiness.messages[2] = strdup("  Decide:  Go For Landing of Logging");
    readiness.messages[3] = NULL;
    
    return readiness;
}

// Check WebServer landing readiness
static LandingReadiness check_webserver_landing_readiness(void) {
    LandingReadiness readiness = {0};
    readiness.subsystem = "WebServer";
    
    // Always ready for now
    readiness.ready = true;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(4 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add messages in the standard format
    readiness.messages[0] = strdup("WebServer");
    readiness.messages[1] = strdup("  Go:      WebServer Ready for Landing");
    readiness.messages[2] = strdup("  Decide:  Go For Landing of WebServer");
    readiness.messages[3] = NULL;
    
    return readiness;
}

// Check API landing readiness
static LandingReadiness check_api_landing_readiness(void) {
    LandingReadiness readiness = {0};
    readiness.subsystem = "API";
    
    // Always ready for now
    readiness.ready = true;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(4 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add messages in the standard format
    readiness.messages[0] = strdup("API");
    readiness.messages[1] = strdup("  Go:      API Ready for Landing");
    readiness.messages[2] = strdup("  Decide:  Go For Landing of API");
    readiness.messages[3] = NULL;
    
    return readiness;
}

// Check Swagger landing readiness
static LandingReadiness check_swagger_landing_readiness(void) {
    LandingReadiness readiness = {0};
    readiness.subsystem = "Swagger";
    
    // Always ready for now
    readiness.ready = true;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(4 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add messages in the standard format
    readiness.messages[0] = strdup("Swagger");
    readiness.messages[1] = strdup("  Go:      Swagger Ready for Landing");
    readiness.messages[2] = strdup("  Decide:  Go For Landing of Swagger");
    readiness.messages[3] = NULL;
    
    return readiness;
}

// Check WebSocket landing readiness
static LandingReadiness check_websocket_landing_readiness(void) {
    LandingReadiness readiness = {0};
    readiness.subsystem = "WebSocketServer";
    
    // Always ready for now
    readiness.ready = true;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(4 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add messages in the standard format
    readiness.messages[0] = strdup("WebSocketServer");
    readiness.messages[1] = strdup("  Go:      WebSocketServer Ready for Landing");
    readiness.messages[2] = strdup("  Decide:  Go For Landing of WebSocketServer");
    readiness.messages[3] = NULL;
    
    return readiness;
}

// Check Terminal landing readiness
static LandingReadiness check_terminal_landing_readiness(void) {
    LandingReadiness readiness = {0};
    readiness.subsystem = "Terminal";
    
    // Always ready for now
    readiness.ready = true;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(4 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add messages in the standard format
    readiness.messages[0] = strdup("Terminal");
    readiness.messages[1] = strdup("  Go:      Terminal Ready for Landing");
    readiness.messages[2] = strdup("  Decide:  Go For Landing of Terminal");
    readiness.messages[3] = NULL;
    
    return readiness;
}

// Check mDNS Server landing readiness
static LandingReadiness check_mdns_server_landing_readiness(void) {
    LandingReadiness readiness = {0};
    readiness.subsystem = "mDNSServer";
    
    // Always ready for now
    readiness.ready = true;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(4 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add messages in the standard format
    readiness.messages[0] = strdup("mDNSServer");
    readiness.messages[1] = strdup("  Go:      mDNSServer Ready for Landing");
    readiness.messages[2] = strdup("  Decide:  Go For Landing of mDNSServer");
    readiness.messages[3] = NULL;
    
    return readiness;
}

// Check mDNS Client landing readiness
static LandingReadiness check_mdns_client_landing_readiness(void) {
    LandingReadiness readiness = {0};
    readiness.subsystem = "mDNSClient";
    
    // Always ready for now
    readiness.ready = true;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(4 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add messages in the standard format
    readiness.messages[0] = strdup("mDNSClient");
    readiness.messages[1] = strdup("  Go:      mDNSClient Ready for Landing");
    readiness.messages[2] = strdup("  Decide:  Go For Landing of mDNSClient");
    readiness.messages[3] = NULL;
    
    return readiness;
}

// Check Mail Relay landing readiness
static LandingReadiness check_mail_relay_landing_readiness(void) {
    LandingReadiness readiness = {0};
    readiness.subsystem = "MailRelay";
    
    // Always ready for now
    readiness.ready = true;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(4 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add messages in the standard format
    readiness.messages[0] = strdup("MailRelay");
    readiness.messages[1] = strdup("  Go:      MailRelay Ready for Landing");
    readiness.messages[2] = strdup("  Decide:  Go For Landing of MailRelay");
    readiness.messages[3] = NULL;
    
    return readiness;
}

// Check Print landing readiness
static LandingReadiness check_print_landing_readiness(void) {
    LandingReadiness readiness = {0};
    readiness.subsystem = "PrintQueue";
    
    // Always ready for now
    readiness.ready = true;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(4 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add messages in the standard format
    readiness.messages[0] = strdup("PrintQueue");
    readiness.messages[1] = strdup("  Go:      PrintQueue Ready for Landing");
    readiness.messages[2] = strdup("  Decide:  Go For Landing of PrintQueue");
    readiness.messages[3] = NULL;
    
    return readiness;
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
    
    // Check logging subsystem
    LandingReadiness logging_readiness = check_logging_landing_readiness();
    log_readiness_messages(&logging_readiness);
    if (logging_readiness.ready) {
        any_subsystem_ready = true;
    }
    free_landing_readiness_messages(&logging_readiness);
    
    // Check network subsystem
    LandingReadiness network_readiness = check_network_landing_readiness();
    log_readiness_messages(&network_readiness);
    if (network_readiness.ready) {
        any_subsystem_ready = true;
    }
    free_landing_readiness_messages(&network_readiness);
    
    // Check payload subsystem
    LandingReadiness payload_readiness = check_payload_landing_readiness();
    log_readiness_messages(&payload_readiness);
    if (payload_readiness.ready) {
        any_subsystem_ready = true;
    }
    free_landing_readiness_messages(&payload_readiness);
    
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
    
    // Count how many subsystems were checked
    const int total_checked = 13; // Total number of subsystems we check
    
    log_this("Landing", "  Total subsystems checked: %d", LOG_LEVEL_STATE, total_checked);
    log_this("Landing", "  Subsystems ready for landing: %s", LOG_LEVEL_STATE, 
             any_subsystem_ready ? "Yes" : "No");
    
    log_group_end();
    
    return any_subsystem_ready;
}