// Global includes 
#include "../hydrogen.h"

// Local includes
#include "utils.h"

// Static variables for program arguments storage
static int stored_argc;
static char** stored_argv;

// Forward declarations of static functions
static void init_all_service_threads(void);

// Initialize service threads when module loads
static void __attribute__((constructor)) init_utils(void) {
    // Thread initialization moved to launch_threads_subsystem() to avoid early MUTEX logging
    // init_all_service_threads();
    init_queue_memory(&log_queue_memory, NULL);
    init_queue_memory(&webserver_queue_memory, NULL);
    init_queue_memory(&websocket_queue_memory, NULL);
    init_queue_memory(&mdns_server_queue_memory, NULL);
    init_queue_memory(&print_queue_memory, NULL);
    init_queue_memory(&database_queue_memory, NULL);
    init_queue_memory(&mail_relay_queue_memory, NULL);
    init_queue_memory(&notify_queue_memory, NULL);
}

// Update queue limits after configuration is loaded
void update_queue_limits_from_config(const AppConfig *config) {
    if (!config) return;
 
    log_this(SR_QUEUES, "― Configuring queue limits for " SR_LOGGING, LOG_LEVEL_DEBUG, 0);
    update_queue_limits(&log_queue_memory, config);

    log_this(SR_QUEUES, "― Configuring queue limits for " SR_WEBSERVER, LOG_LEVEL_DEBUG, 0);
    update_queue_limits(&log_queue_memory, config);

    log_this(SR_QUEUES, "― Configuring queue limits for " SR_WEBSOCKET, LOG_LEVEL_DEBUG, 0);
    update_queue_limits(&log_queue_memory, config);

    log_this(SR_QUEUES, "― Configuring queue limits for " SR_MDNS_SERVER, LOG_LEVEL_DEBUG, 0);
    update_queue_limits(&log_queue_memory, config);

    log_this(SR_QUEUES, "― Configuring queue limits for " SR_PRINT, LOG_LEVEL_DEBUG, 0);
    update_queue_limits(&print_queue_memory, config);

    log_this(SR_QUEUES, "― Configuring queue limits for " SR_DATABASE, LOG_LEVEL_DEBUG, 0);
    update_queue_limits(&database_queue_memory, config);

    log_this(SR_QUEUES, "― Configuring queue limits for " SR_MAIL_RELAY, LOG_LEVEL_DEBUG, 0);
    update_queue_limits(&mail_relay_queue_memory, config);

    log_this(SR_QUEUES, "― Configuring queue limits for " SR_NOTIFY, LOG_LEVEL_DEBUG, 0);
    update_queue_limits(&notify_queue_memory, config);
}

// Initialize all service thread tracking
static void __attribute__((unused)) init_all_service_threads(void) {
    init_service_threads(&logging_threads, SR_LOGGING);
    init_service_threads(&webserver_threads, SR_WEBSERVER);
    init_service_threads(&websocket_threads, SR_WEBSOCKET);
    init_service_threads(&mdns_server_threads, SR_MDNS_SERVER);
    init_service_threads(&print_threads, SR_PRINT);
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
        if (j + 1 >= size) {
            formatted[size-1] = '\0';
            return formatted;
        }

        // Add comma every 3 digits from the right
        if (i > 0 && (len - i) % 3 == 0) {
            formatted[j++] = ',';

            // Check buffer again after adding comma
            if (j + 1 >= size) {
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
 * Format a double with thousands separators
 * Thread-safe as long as different buffers are used
 * Only adds commas to the integer part, decimal part remains unchanged
 */
char* format_double_with_commas(double value, int decimals, char* formatted, size_t size) {
    if (!formatted || size < 2) return NULL;

    // Format the number with specified decimal places
    char temp[64];
    if (decimals >= 0) {
        snprintf(temp, sizeof(temp), "%.*f", decimals, value);
    } else {
        // No decimal formatting - convert to integer-like string
        snprintf(temp, sizeof(temp), "%.0f", value);
    }

    // Find decimal point position
    const char* decimal_point = strchr(temp, '.');

    // Calculate integer part length
    size_t integer_part_len = decimal_point ? (size_t)(decimal_point - temp) : strlen(temp);
    size_t j = 0;

    // Process integer part with commas
    for (size_t i = 0; i < integer_part_len; i++) {
        // Check buffer space
        if (j + 1 >= size) {
            formatted[size-1] = '\0';
            return formatted;
        }

        // Add comma every 3 digits from the right (before decimal point)
        if (i > 0 && (integer_part_len - i) % 3 == 0) {
            formatted[j++] = ',';

            // Check buffer again after adding comma
            if (j + 1 >= size) {
                formatted[size-1] = '\0';
                return formatted;
            }
        }
        formatted[j++] = temp[i];
    }

    // Add decimal part as-is (if it exists)
    if (decimal_point) {
        size_t remaining_len = strlen(decimal_point);
        if (j + remaining_len >= size) {
            // Not enough space for decimal part
            formatted[size-1] = '\0';
            return formatted;
        }
        strcpy(formatted + j, decimal_point);
        j += remaining_len;
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

/**
 * Store program arguments for later retrieval
 * Called by main() to preserve original command line arguments
 */
void store_program_args(int argc, char* argv[]) {
    stored_argc = argc;
    stored_argv = argv;
}

/**
 * Get stored program arguments
 * Used by restart functionality to preserve original command line arguments
 */
char** get_program_args(void) {
    return stored_argv;
}
