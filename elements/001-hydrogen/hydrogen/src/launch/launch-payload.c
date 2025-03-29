/*
 * Payload Subsystem Launch Readiness Check
 * 
 * This module verifies that all prerequisites for the payload subsystem
 * are satisfied before attempting to initialize it.
 * 
 * The checks here mirror the extraction logic in src/payload/payload.c
 * to ensure the payload can be successfully extracted later.
 * 
 * Note: Shutdown functionality has been moved to landing/landing-payload.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include "launch.h"
#include "launch-payload.h"
#include "../logging/logging.h"
#include "../config/config.h"
#include "../config/files/config_filesystem.h"
#include "../state/registry/subsystem_registry_integration.h"
#include "../payload/payload.h"
#include "../utils/utils.h"

// External declarations
extern AppConfig* app_config;

// External system state flags
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t web_server_shutdown;

// Default payload marker (from Swagger implementation)
#define DEFAULT_PAYLOAD_MARKER "<<< HERE BE ME TREASURE >>>"

// Function to check if a payload is attached to the executable
static bool is_payload_attached(const char* executable_path, const char* marker, size_t* payload_size) {
    bool result = false;
    
    // Open the executable
    int fd = open(executable_path, O_RDONLY);
    if (fd == -1) {
        // Only log error, don't clutter the output with debug info
        log_this("PayloadLaunch", "Failed to open executable: %s", LOG_LEVEL_ERROR, strerror(errno));
        return false;
    }
    
    // Get file size
    struct stat st;
    if (fstat(fd, &st) == -1) {
        log_this("PayloadLaunch", "Failed to get file size: %s", LOG_LEVEL_ERROR, strerror(errno));
        close(fd);
        return false;
    }
    
    size_t file_size = (size_t)st.st_size;
    
    // Map the file
    void* file_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    
    if (file_data == MAP_FAILED) {
        log_this("PayloadLaunch", "Failed to map executable: %s", LOG_LEVEL_ERROR, strerror(errno));
        return false;
    }
    
    // Search for the marker - first check the last 64 bytes
    // This is a performance optimization as payloads are appended
    const char* marker_pos = NULL;
    
    // First try the last portion (payloads are typically appended)
    size_t tail_search_size = file_size < 64 ? file_size : 64;  
    
    marker_pos = memmem((char*)file_data + file_size - tail_search_size, 
                        tail_search_size, marker, strlen(marker));
    
    // If not found in the tail, search the entire file (like the real payload extractor does)
    if (!marker_pos) {
        marker_pos = memmem(file_data, file_size, marker, strlen(marker));
    }
    
    if (!marker_pos) {
        // No payload found, but this is not an error
        munmap(file_data, file_size);
        return false;
    }
    
    // Ensure there's enough space after the marker for size bytes
    size_t marker_offset = (size_t)(marker_pos - (char*)file_data);
    
    if (marker_offset + strlen(marker) + 8 > file_size) {
        // Don't log this error to avoid cluttering the output
        munmap(file_data, file_size);
        return false;
    }
    
    // Get payload size from 8 bytes after the marker
    // This matches the logic in payload.c extract_payload function
    const uint8_t* size_bytes = (uint8_t*)(marker_pos + strlen(marker));
    
    // Read all 8 bytes as payload size (64-bit value)
    // This matches payload.c extraction logic
    *payload_size = 0;
    for (int i = 0; i < 8; i++) {
        *payload_size = (*payload_size << 8) | size_bytes[i];
    }
    
    // Extra validation for size reasonableness
    if (*payload_size == 0) {
        // Don't log this error to avoid cluttering the output
        munmap(file_data, file_size);
        return false;
    }
    
    if (*payload_size > 100 * 1024 * 1024) {  // Limit to 100MB as sanity check
        // Don't log this error to avoid cluttering the output
        munmap(file_data, file_size);
        return false;
    }
    
    // Most important check: payload data must fit before the marker
    if (*payload_size > marker_offset) {
        // Don't log this error to avoid cluttering the output
        munmap(file_data, file_size);
        return false;
    }
    
    result = true;
    
    // Unmap the file
    munmap(file_data, file_size);
    
    return result;
}

// Forward declaration for the launch_payload function from payload.c
extern bool launch_payload(const AppConfig *config, const char *marker);

// Check if the payload subsystem is ready to launch
LaunchReadiness check_payload_launch_readiness(void) {
    bool overall_readiness = true;
    
    // Allocate space for messages (including NULL terminator)
    const char** messages = malloc(15 * sizeof(char*));
    if (!messages) {
        LaunchReadiness readiness = {
            .subsystem = "Payload",
            .ready = false,
            .messages = NULL
        };
        return readiness;
    }
    
    // Initialize all pointers to NULL
    for (int i = 0; i < 15; i++) {
        messages[i] = NULL;
    }
    
    // Add subsystem name as first message
    messages[0] = (const char*)strdup("Payload");
    int msg_index = 1;
    
    // Check 0: System state
    if (server_stopping || web_server_shutdown) {
        messages[msg_index++] = (const char*)strdup("  No-Go:   System State (shutdown in progress)");
        overall_readiness = false;
        messages[msg_index++] = (const char*)strdup("  Decide:  No-Go For Launch of Payload Subsystem");
        messages[msg_index] = NULL;
        
        LaunchReadiness readiness = {
            .subsystem = "Payload",
            .ready = false,
            .messages = messages
        };
        
        return readiness;
    }
    
    if (!server_starting && !server_running) {
        messages[msg_index++] = (const char*)strdup("  No-Go:   System State (not in startup or running state)");
        overall_readiness = false;
        messages[msg_index++] = (const char*)strdup("  Decide:  No-Go For Launch of Payload Subsystem");
        messages[msg_index] = NULL;
        
        LaunchReadiness readiness = {
            .subsystem = "Payload",
            .ready = false,
            .messages = messages
        };
        
        return readiness;
    }
    
    // Check 1: Configuration loaded - required but don't add a message
    bool config_loaded = (app_config != NULL);
    if (!config_loaded) {
        overall_readiness = false;
        
        messages[msg_index++] = (const char*)strdup("  No-Go:   Configuration not loaded");
        messages[msg_index++] = (const char*)strdup("  Decide:  No-Go For Launch of Payload Subsystem");
        messages[msg_index] = NULL;
        
        LaunchReadiness readiness = {
            .subsystem = "Payload",
            .ready = false,
            .messages = messages
        };
        
        return readiness;
    }
    
    // Check 2: Is a payload attached to the executable?
    char* executable_path = get_executable_path();
    size_t payload_size = 0;
    bool payload_attached = false;
    
    if (executable_path) {
        payload_attached = is_payload_attached(executable_path, DEFAULT_PAYLOAD_MARKER, &payload_size);
        
        // Check 3: Is the payload accessible? (Only if payload is attached)
        if (payload_attached) {
            if (access(executable_path, R_OK) != 0) {
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), 
                        "  No-Go:   Payload Accessibility (executable not readable: %s)",
                        strerror(errno));
                messages[msg_index++] = (const char*)strdup(error_msg);
                overall_readiness = false;
            }
        }
        
        // Free the memory allocated by get_executable_path()
        free(executable_path);
        executable_path = NULL;
    } else {
        messages[msg_index++] = (const char*)strdup("  No-Go:   Executable Path (failed to determine)");
        overall_readiness = false;
    }
    
    
    // Report payload status
    if (payload_attached) {
        // Format payload size with thousands separators for readability
        char formatted_size[32];
        format_number_with_commas(payload_size, formatted_size, sizeof(formatted_size));
        char size_msg[256];
        snprintf(size_msg, sizeof(size_msg), "  Go:      Payload (found, %s bytes)", formatted_size);
        messages[msg_index++] = (const char*)strdup(size_msg);
    } else {
        messages[msg_index++] = (const char*)strdup("  No-Go:   Payload (not found)");
        overall_readiness = false;
    }
    
    // Check 4: Is a suitable key available?
    const char* payload_key = app_config->server.payload_key;
    bool key_available = false;
    
    if (payload_key) {
        // Check if it's an environment variable reference
        if (strncmp(payload_key, "${env.", 6) == 0) {
            const char* end = strchr(payload_key + 6, '}');
            if (end) {
                char env_var[256];
                size_t len = end - (payload_key + 6);
                if (len < sizeof(env_var)) {
                    strncpy(env_var, payload_key + 6, len);
                    env_var[len] = '\0';
                    const char* env_value = getenv(env_var);
                    if (env_value && strlen(env_value) > 0) {
                        key_available = true;
                        // Truncate env_var if too long to display
                        char truncated_env[64];
                        snprintf(truncated_env, sizeof(truncated_env), "%.60s", env_var);
                        
                        char env_msg[256];
                        snprintf(env_msg, sizeof(env_msg),
                               "  Go:      Decryption Key (from environment: %.60s)", truncated_env);
                        messages[msg_index++] = (const char*)strdup(env_msg);
                    } else {
                        // Truncate env_var if too long to display
                        char truncated_env[64];
                        snprintf(truncated_env, sizeof(truncated_env), "%.60s", env_var);
                        
                        char env_msg[256];
                        snprintf(env_msg, sizeof(env_msg),
                               "  No-Go:   Decryption Key (environment variable %.60s not set)", truncated_env);
                        messages[msg_index++] = (const char*)strdup(env_msg);
                    }
                } else {
                    messages[msg_index++] = (const char*)strdup("  No-Go:   Decryption Key (environment variable name too long)");
                }
            } else {
                messages[msg_index++] = (const char*)strdup("  No-Go:   Decryption Key (malformed environment variable reference)");
            }
        } else if (strcmp(payload_key, "Missing Key") != 0 && strlen(payload_key) > 0) {
            key_available = true;
            messages[msg_index++] = (const char*)strdup("  Go:      Decryption Key (direct configuration)");
        } else {
            messages[msg_index++] = (const char*)strdup("  No-Go:   Decryption Key (default placeholder value)");
        }
    } else {
        messages[msg_index++] = (const char*)strdup("  No-Go:   Decryption Key (not configured)");
    }
    
    if (!key_available) {
        overall_readiness = false;
    }
    
    // Final decision
    if (overall_readiness) {
        messages[msg_index++] = (const char*)strdup("  Decide:  Go For Launch of Payload Subsystem");
    } else {
        messages[msg_index++] = (const char*)strdup("  Decide:  No-Go For Launch of Payload Subsystem");
    }
    
    // Ensure NULL termination
    messages[msg_index] = NULL;
    
    // Build the readiness structure
    LaunchReadiness readiness = {
        .subsystem = "Payload",
        .ready = overall_readiness,
        .messages = messages
    };
    
    return readiness;
}

/**
 * Launch the payload subsystem
 * 
 * This function launches the payload subsystem by extracting and
 * processing the payload from the executable.
 * 
 * @return true if payload was successfully launched, false otherwise
 */
bool launch_payload_subsystem(void) {
    // Launch the payload directly without additional logging
    bool success = launch_payload(app_config, DEFAULT_PAYLOAD_MARKER);
    
    if (success) {
        // Register the payload subsystem in the registry
        update_subsystem_on_startup("Payload", true);
    } else {
        log_this("Payload", "Failed to launch payload subsystem", LOG_LEVEL_ERROR);
        // Register the payload subsystem in the registry as failed
        update_subsystem_on_startup("Payload", false);
    }
    
    return success;
}