/*
 * Payload Subsystem Launch Readiness Check
 * 
 * This module verifies that all prerequisites for the payload subsystem
 * are satisfied before attempting to initialize it.
 * 
 * The checks here mirror the extraction logic in src/payload/payload.c
 * to ensure the payload can be successfully extracted later.
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
#include "../../logging/logging.h"
#include "../../config/config.h"
#include "../../config/files/config_filesystem.h"

// External declarations
extern AppConfig* app_config;

// External system state flags
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t web_server_shutdown;

// Default payload marker (from Swagger implementation)
#define DEFAULT_PAYLOAD_MARKER "<<< HERE BE ME TREASURE >>>"

// Static message array for readiness check results
static const char* payload_messages[15]; // Up to 15 messages plus NULL terminator
static int message_count = 0;

// Helper function to format a number with thousands separators
static char* format_with_commas(size_t n) {
    static char formatted[32];
    char temp[32];
    int i = 0, j = 0;
    
    // Convert to string
    snprintf(temp, sizeof(temp), "%zu", n);
    
    // Get the length
    int len = strlen(temp);
    
    // Add commas
    for (i = 0; i < len; i++) {
        // Add comma every 3 digits from the right
        if (i > 0 && (len - i) % 3 == 0) {
            formatted[j++] = ',';
        }
        formatted[j++] = temp[i];
    }
    
    // Null terminate
    formatted[j] = '\0';
    
    return formatted;
}

// Add a message to the messages array
static void add_message(const char* format, ...) {
    if (message_count >= 14) return; // Leave room for NULL terminator
    
    va_list args;
    va_start(args, format);
    char* message = NULL;
    vasprintf(&message, format, args);
    va_end(args);
    
    if (message) {
        payload_messages[message_count++] = message;
        payload_messages[message_count] = NULL; // Ensure NULL termination
    }
}

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

// Check if the payload subsystem is ready to launch
LaunchReadiness check_payload_launch_readiness(void) {
    bool overall_readiness = true;
    message_count = 0;
    
    // Clear messages array
    for (int i = 0; i < 15; i++) {
        payload_messages[i] = NULL;
    }
    
    // Add subsystem name as first message
    add_message("Payload");
    
    // Check 0: System state
    if (server_stopping || web_server_shutdown) {
        add_message("  No-Go:   System State (shutdown in progress)");
        overall_readiness = false;
        
        // Build the readiness structure with minimal checks during shutdown
        LaunchReadiness readiness = {
            .subsystem = "Payload",
            .ready = false,
            .messages = payload_messages
        };
        
        return readiness;
    }
    
    if (!server_starting && !server_running) {
        add_message("  No-Go:   System State (not in startup or running state)");
        overall_readiness = false;
        
        // Build the readiness structure with minimal checks during invalid state
        LaunchReadiness readiness = {
            .subsystem = "Payload",
            .ready = false,
            .messages = payload_messages
        };
        
        return readiness;
    }
    
    // Check 1: Configuration loaded - required but don't add a message
    bool config_loaded = (app_config != NULL);
    if (!config_loaded) {
        overall_readiness = false;
        
        // Build the readiness structure with minimal checks if config not loaded
        LaunchReadiness readiness = {
            .subsystem = "Payload",
            .ready = false,
            .messages = payload_messages
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
                add_message("  No-Go:   Payload Accessibility (executable not readable: %s)", strerror(errno));
                overall_readiness = false;
            }
        }
        
        // Free the memory allocated by get_executable_path()
        free(executable_path);
        executable_path = NULL;
    } else {
        add_message("  No-Go:   Executable Path (failed to determine)");
        overall_readiness = false;
    }
    
    
    // Report payload status
    if (payload_attached) {
        // Format payload size with thousands separators for readability
        add_message("  Go:      Payload (found, %s bytes)", format_with_commas(payload_size));
    } else {
        add_message("  No-Go:   Payload (not found)");
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
                        add_message("  Go:      Decryption Key (from environment: %s)", env_var);
                    } else {
                        add_message("  No-Go:   Decryption Key (environment variable %s not set)", env_var);
                    }
                } else {
                    add_message("  No-Go:   Decryption Key (environment variable name too long)");
                }
            } else {
                add_message("  No-Go:   Decryption Key (malformed environment variable reference)");
            }
        } else if (strcmp(payload_key, "Missing Key") != 0 && strlen(payload_key) > 0) {
            key_available = true;
            add_message("  Go:      Decryption Key (direct configuration)");
        } else {
            add_message("  No-Go:   Decryption Key (default placeholder value)");
        }
    } else {
        add_message("  No-Go:   Decryption Key (not configured)");
    }
    
    if (!key_available) {
        overall_readiness = false;
    }
    
    // Final decision
    if (overall_readiness) {
        add_message("  Decide:  Go For Launch of Payload Subsystem");
    } else {
        add_message("  Decide:  No-Go For Launch of Payload Subsystem");
    }
    
    // Build the readiness structure
    LaunchReadiness readiness = {
        .subsystem = "Payload",
        .ready = overall_readiness,
        .messages = payload_messages
    };
    
    return readiness;
}