/*
 * Payload Subsystem Launch Readiness Check
 * 
 * This module verifies that all prerequisites for the payload subsystem
 * are satisfied before attempting to initialize it.
 * 
 * The checks here mirror the extraction logic in src/payload/payload.c
 * to ensure the payload can be successfully extracted later.
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#include "launch.h"
#include "launch-payload.h"
#include "../../logging/logging.h"
#include "../../config/config.h"

// External declarations
extern AppConfig* app_config;

// Default payload marker (from Swagger implementation)
#define DEFAULT_PAYLOAD_MARKER "<<< HERE BE ME TREASURE >>>"

// Static message array for readiness check results
static const char* payload_messages[15]; // Up to 15 messages plus NULL terminator
static int message_count = 0;

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
    
    // Log that we're checking for a payload
    log_this("PayloadLaunch", "Checking for payload in %s with marker '%s'", LOG_LEVEL_STATE, executable_path, marker);
    
    // Open the executable
    int fd = open(executable_path, O_RDONLY);
    if (fd == -1) {
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
    log_this("PayloadLaunch", "Executable size: %zu bytes", LOG_LEVEL_STATE, file_size);
    
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
    log_this("PayloadLaunch", "Searching last %zu bytes for marker", LOG_LEVEL_STATE, tail_search_size);
    
    marker_pos = memmem((char*)file_data + file_size - tail_search_size, 
                        tail_search_size, marker, strlen(marker));
    
    // If not found in the tail, search the entire file (like the real payload extractor does)
    if (!marker_pos) {
        log_this("PayloadLaunch", "Marker not found in file tail, searching entire file", LOG_LEVEL_STATE);
        marker_pos = memmem(file_data, file_size, marker, strlen(marker));
    }
    
    if (!marker_pos) {
        log_this("PayloadLaunch", "No payload marker found in executable", LOG_LEVEL_STATE);
        munmap(file_data, file_size);
        return false;
    }
    
    // Ensure there's enough space after the marker for size bytes
    size_t marker_offset = (size_t)(marker_pos - (char*)file_data);
    log_this("PayloadLaunch", "Found marker at offset %zu", LOG_LEVEL_STATE, marker_offset);
    
    if (marker_offset + strlen(marker) + 8 > file_size) {
        log_this("PayloadLaunch", "Marker found too close to end of file", LOG_LEVEL_ERROR);
        munmap(file_data, file_size);
        return false;
    }
    
    // Get payload size from 8 bytes after the marker
    // This matches the logic in payload.c extract_payload function
    const uint8_t* size_bytes = (uint8_t*)(marker_pos + strlen(marker));
    
    // Debug output of raw size bytes
    char size_byte_debug[50];
    snprintf(size_byte_debug, sizeof(size_byte_debug), 
             "%02x %02x %02x %02x %02x %02x %02x %02x", 
             size_bytes[0], size_bytes[1], size_bytes[2], size_bytes[3],
             size_bytes[4], size_bytes[5], size_bytes[6], size_bytes[7]);
    log_this("PayloadLaunch", "Size bytes: %s", LOG_LEVEL_STATE, size_byte_debug);
    
    // Read all 8 bytes as payload size (64-bit value)
    // This matches payload.c extraction logic
    *payload_size = 0;
    for (int i = 0; i < 8; i++) {
        *payload_size = (*payload_size << 8) | size_bytes[i];
    }
    
    log_this("PayloadLaunch", "Calculated payload size: %zu bytes", LOG_LEVEL_STATE, *payload_size);
    
    // Extra validation for size reasonableness
    if (*payload_size == 0) {
        log_this("PayloadLaunch", "Invalid zero payload size", LOG_LEVEL_ERROR);
        munmap(file_data, file_size);
        return false;
    }
    
    if (*payload_size > 100 * 1024 * 1024) {  // Limit to 100MB as sanity check
        log_this("PayloadLaunch", "Payload size too large: %zu (exceeds 100MB)", LOG_LEVEL_ALERT, *payload_size);
        munmap(file_data, file_size);
        return false;
    }
    
    // Most important check: payload data must fit before the marker
    if (*payload_size > marker_offset) {
        log_this("PayloadLaunch", "Invalid payload size: %zu (exceeds available space before marker)", 
                LOG_LEVEL_ALERT, *payload_size);
        munmap(file_data, file_size);
        return false;
    }
    
    log_this("PayloadLaunch", "Payload size validation successful", LOG_LEVEL_STATE);
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
    
    // Check 1: Configuration loaded
    bool config_loaded = (app_config != NULL);
    if (config_loaded) {
        add_message("  Go:      Configuration (loaded)");
    } else {
        add_message("  No-Go:   Configuration (not loaded)");
        overall_readiness = false;
    }
    
    // Only proceed with other checks if configuration is loaded
    if (config_loaded) {
        // Check 2: Is a payload attached to the executable?
        const char* executable_path = app_config->server.exec_file;
        size_t payload_size = 0;
        bool payload_attached = false;
        
        if (executable_path) {
            payload_attached = is_payload_attached(executable_path, DEFAULT_PAYLOAD_MARKER, &payload_size);
        }
        
        if (payload_attached) {
            add_message("  Go:      Payload Attachment (found, %zu bytes)", payload_size);
        } else {
            add_message("  No-Go:   Payload Attachment (not found or invalid)");
            overall_readiness = false;
        }
        
        // Check 3: Is a suitable key available?
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
                            add_message("  Go:      Key Availability (from environment: %s)", env_var);
                        } else {
                            add_message("  No-Go:   Key Availability (environment variable %s not set)", env_var);
                        }
                    } else {
                        add_message("  No-Go:   Key Availability (environment variable name too long)");
                    }
                } else {
                    add_message("  No-Go:   Key Availability (malformed environment variable reference)");
                }
            } else if (strcmp(payload_key, "Missing Key") != 0 && strlen(payload_key) > 0) {
                key_available = true;
                add_message("  Go:      Key Availability (direct configuration)");
            } else {
                add_message("  No-Go:   Key Availability (default placeholder value)");
            }
        } else {
            add_message("  No-Go:   Key Availability (no key configured)");
        }
        
        if (!key_available) {
            overall_readiness = false;
        }
        
        // Check 4: Is the payload accessible?
        if (payload_attached && executable_path) {
            if (access(executable_path, R_OK) == 0) {
                add_message("  Go:      Payload Accessibility (executable readable)");
            } else {
                add_message("  No-Go:   Payload Accessibility (executable not readable: %s)", strerror(errno));
                overall_readiness = false;
            }
        } else {
            add_message("  No-Go:   Payload Accessibility (prerequisite checks failed)");
            overall_readiness = false;
        }
        
        // Check 5: Can we determine the size of the payload?
        if (payload_attached && payload_size > 0) {
            add_message("  Go:      Payload Size (determinable: %zu bytes)", payload_size);
        } else if (payload_attached) {
            add_message("  No-Go:   Payload Size (invalid size or corrupted payload)");
            overall_readiness = false;
        } else {
            add_message("  No-Go:   Payload Size (prerequisite checks failed)");
            overall_readiness = false;
        }
    }
    
    // Final decision
    if (overall_readiness) {
        add_message("  Decide:   Go For Launch of Payload Subsystem");
    } else {
        add_message("  Decide:   No-Go For Launch of Payload Subsystem");
    }
    
    // Build the readiness structure
    LaunchReadiness readiness = {
        .subsystem = "Payload",
        .ready = overall_readiness,
        .messages = payload_messages
    };
    
    return readiness;
}