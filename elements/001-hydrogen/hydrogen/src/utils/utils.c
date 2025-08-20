// Global includes 
#include "../hydrogen.h"

// Local includes
#include "utils.h"

// Forward declarations of static functions
static void init_all_service_threads(void);

// Initialize service threads when module loads
static void __attribute__((constructor)) init_utils(void) {
    init_all_service_threads();
    init_queue_memory(&log_queue_memory, NULL);
    init_queue_memory(&print_queue_memory, NULL);
}

// Update queue limits after configuration is loaded
void update_queue_limits_from_config(const AppConfig *config) {
    if (!config) return;
    
    update_queue_limits(&log_queue_memory, config);
    update_queue_limits(&print_queue_memory, config);
}

// Initialize all service thread tracking
static void init_all_service_threads(void) {
    init_service_threads(&logging_threads, "Logging");
    init_service_threads(&webserver_threads, "WebServer");
    init_service_threads(&websocket_threads, "WebSocket");
    init_service_threads(&mdns_server_threads, "mDNS Server");
    init_service_threads(&print_threads, "Print");
}

/**
 * Format a number with thousands separators
 * Thread-safe as long as different buffers are used
 */
char* format_number_with_commas(size_t n, char* formatted, size_t size) {
    if (!formatted || size < 2) return NULL;
    
    // Convert to temporary string
    char temp[32];
    snprintf(temp, sizeof(temp), "%zu", n);
    
    // Get the length
    size_t len = strlen(temp);
    size_t j = 0;
    
    // Add commas
    for (size_t i = 0; i < len; i++) {
        // Check buffer space (including null terminator)
        if (j >= size - 1) {
            formatted[size-1] = '\0';
            return formatted;
        }
        
        // Add comma every 3 digits from the right
        if (i > 0 && (len - i) % 3 == 0) {
            formatted[j++] = ',';
            
            // Check buffer again after adding comma
            if (j >= size - 1) {
                formatted[size-1] = '\0';
                return formatted;
            }
        }
        formatted[j++] = temp[i];
    }
    
    // Null terminate
    formatted[j] = '\0';
    
    return formatted;
}

/**
 * Add a formatted message to a message array
 * Thread-safe if different message arrays are used
 */
bool add_message_to_array(const char** messages, int max_messages, int* count, const char* format, ...) {
    if (!messages || !count || *count >= max_messages - 1 || !format) {
        return false;
    }
    
    va_list args;
    va_start(args, format);
    char* message = NULL;
    
    // Format the message
    if (vasprintf(&message, format, args) == -1) {
        va_end(args);
        return false;
    }
    va_end(args);
    
    if (message) {
        messages[*count] = message;
        (*count)++;
        messages[*count] = NULL; // Maintain NULL termination
        return true;
    }
    
    return false;
}
