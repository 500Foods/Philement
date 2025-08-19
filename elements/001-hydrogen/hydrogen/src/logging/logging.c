/*
 * Hydrogen Server Logging System
 */

// Global includes 
#include "../hydrogen.h"

// Local includes
#include "logging.h"
#include "../config/config_priority.h"

// Public interface declarations
void log_this(const char* subsystem, const char* format, int priority, ...);
void log_group_begin(void);
void log_group_end(void);

// External declarations
extern AppConfig* app_config;
extern volatile sig_atomic_t log_queue_shutdown;
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;
extern int queue_system_initialized;  // From queue.c

// Internal state
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static __thread bool in_log_group = false;  // Thread-local flag for group logging
static unsigned long log_counter = 0;  // Global log counter

// Rolling buffer for recent messages
static struct {
    char* messages[LOG_BUFFER_SIZE];  // Array of message strings
    size_t head;                      // Index of newest message
    size_t count;                     // Number of messages in buffer
    pthread_mutex_t mutex;            // Buffer mutex
} log_buffer = {
    .messages = {NULL},
    .head = 0,
    .count = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER
};

// Initialize a message slot in the buffer
static bool init_message_slot(size_t index) {
    if (log_buffer.messages[index] == NULL) {
        log_buffer.messages[index] = malloc(MAX_LOG_LINE_LENGTH);
        if (!log_buffer.messages[index]) return false;
    }
    return true;
}

// Add a message to the rolling buffer
static void add_to_buffer(const char* message) {
    pthread_mutex_lock(&log_buffer.mutex);

    // Initialize or move to next slot
    size_t index = log_buffer.head;
    if (!init_message_slot(index)) {
        pthread_mutex_unlock(&log_buffer.mutex);
        return;
    }

    // Copy message to buffer using snprintf for safety
    snprintf(log_buffer.messages[index], MAX_LOG_LINE_LENGTH, "%s", message);

    // Update head and count
    log_buffer.head = (log_buffer.head + 1) % LOG_BUFFER_SIZE;
    if (log_buffer.count < LOG_BUFFER_SIZE) {
        log_buffer.count++;
    }

    pthread_mutex_unlock(&log_buffer.mutex);
}

// Get messages matching a subsystem
char* log_get_messages(const char* subsystem) {
    if (!subsystem) return NULL;

    pthread_mutex_lock(&log_buffer.mutex);

    // Calculate total size needed
    size_t total_size = 0;
    size_t matches = 0;
    for (size_t i = 0; i < log_buffer.count; i++) {
        size_t idx = (log_buffer.head - 1 - i + LOG_BUFFER_SIZE) % LOG_BUFFER_SIZE;
        if (log_buffer.messages[idx] && strstr(log_buffer.messages[idx], subsystem)) {
            total_size += strlen(log_buffer.messages[idx]) + 1;  // +1 for newline
            matches++;
        }
    }

    if (matches == 0) {
        pthread_mutex_unlock(&log_buffer.mutex);
        return NULL;
    }

    // Allocate buffer for matching messages
    char* result = malloc(total_size + 1);  // +1 for null terminator
    if (!result) {
        pthread_mutex_unlock(&log_buffer.mutex);
        return NULL;
    }

    // Copy matching messages
    char* pos = result;
    for (size_t i = 0; i < log_buffer.count; i++) {
        size_t idx = (log_buffer.head - 1 - i + LOG_BUFFER_SIZE) % LOG_BUFFER_SIZE;
        if (log_buffer.messages[idx] && strstr(log_buffer.messages[idx], subsystem)) {
            size_t len = strlen(log_buffer.messages[idx]);
            memcpy(pos, log_buffer.messages[idx], len);
            pos[len] = '\n';
            pos += len + 1;
        }
    }
    *pos = '\0';

    pthread_mutex_unlock(&log_buffer.mutex);
    return result;
}

// Get the last N messages
char* log_get_last_n(size_t count) {
    pthread_mutex_lock(&log_buffer.mutex);

    // Adjust count if it's larger than available messages
    if (count > log_buffer.count) {
        count = log_buffer.count;
    }

    if (count == 0) {
        pthread_mutex_unlock(&log_buffer.mutex);
        return NULL;
    }

    // Calculate total size needed
    size_t total_size = 0;
    for (size_t i = 0; i < count; i++) {
        size_t idx = (log_buffer.head - 1 - i + LOG_BUFFER_SIZE) % LOG_BUFFER_SIZE;
        if (log_buffer.messages[idx]) {
            total_size += strlen(log_buffer.messages[idx]) + 1;  // +1 for newline
        }
    }

    // Allocate buffer
    char* result = malloc(total_size + 1);  // +1 for null terminator
    if (!result) {
        pthread_mutex_unlock(&log_buffer.mutex);
        return NULL;
    }

    // Copy messages
    char* pos = result;
    for (size_t i = 0; i < count; i++) {
        size_t idx = (log_buffer.head - 1 - i + LOG_BUFFER_SIZE) % LOG_BUFFER_SIZE;
        if (log_buffer.messages[idx]) {
            size_t len = strlen(log_buffer.messages[idx]);
            memcpy(pos, log_buffer.messages[idx], len);
            pos[len] = '\n';
            pos += len + 1;
        }
    }
    *pos = '\0';

    pthread_mutex_unlock(&log_buffer.mutex);
    return result;
}

// Private function declarations
static void console_log(const char* subsystem, int priority, const char* message, unsigned long current_count);
static bool init_message_slot(size_t index);
static void add_to_buffer(const char* message);

/*
 * INTERNAL USE ONLY - Do not call directly!
 * 
 * This is an internal helper function used by log_this() to handle console output
 * when the logging system is not fully initialized or during shutdown.
 * 
 * All logging should go through log_this() which handles:
 * - Proper initialization checks
 * - Configuration-based filtering
 * - Output routing
 * - Thread safety
 */
// Fallback priority labels for when config is unavailable
static const char* get_fallback_priority_label(int priority) {
    static const char* fallback_labels[] = {
        "TRACE", "DEBUG", "STATE", "ALERT", "ERROR", "FATAL", "QUIET"
    };
    if (priority >= 0 && priority <= LOG_LEVEL_QUIET) {
        return fallback_labels[priority];
    }
    return fallback_labels[LOG_LEVEL_STATE];  // Default to STATE
}

static void console_log(const char* subsystem, int priority, const char* message, unsigned long current_count) {
    // Format the counter as two 3-digit numbers
    char counter_prefix[16];
    snprintf(counter_prefix, sizeof(counter_prefix), "[ %03lu %03lu ]", 
             (current_count / 1000) % 1000, current_count % 1000);

    // Use fallback labels if config system is unavailable
    const char* priority_label = (!app_config || !app_config->logging.levels) 
                               ? get_fallback_priority_label(priority)
                               : get_priority_label(priority);

    char formatted_priority[MAX_PRIORITY_LABEL_WIDTH + 5];
    snprintf(formatted_priority, sizeof(formatted_priority), "[ %-*s ]", MAX_PRIORITY_LABEL_WIDTH, priority_label);

    char formatted_subsystem[MAX_SUBSYSTEM_LABEL_WIDTH + 5];
    snprintf(formatted_subsystem, sizeof(formatted_subsystem), "[ %-*s ]", MAX_SUBSYSTEM_LABEL_WIDTH, subsystem);

    char timestamp[32];
    struct timeval tv;
    struct tm* tm_info;
    gettimeofday(&tv, NULL);
    tm_info = gmtime(&tv.tv_sec);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    char timestamp_ms[48];  // Increased buffer size for safety
    snprintf(timestamp_ms, sizeof(timestamp_ms), "%s.%03dZ", timestamp, (int)(tv.tv_usec / 1000));

    // Format the complete log line
    char log_line[DEFAULT_LOG_ENTRY_SIZE];
    snprintf(log_line, sizeof(log_line), "%s  %s  %s  %s  %s", 
             counter_prefix, timestamp_ms, formatted_priority, formatted_subsystem, message);

    // Add to rolling buffer and output
    add_to_buffer(log_line);
    fprintf(stdout, "%s\n", log_line);
    fflush(stdout);
}


// Log a message based on configuration settings
void log_group_begin(void) {
    pthread_mutex_lock(&log_mutex);
    in_log_group = true;
}

void log_group_end(void) {
    in_log_group = false;
    pthread_mutex_unlock(&log_mutex);
}

void log_this(const char* subsystem, const char* format, int priority, ...) {
    // Validate inputs and normalize priority level early
    if (!subsystem) subsystem = "Unknown";
    if (!format) format = "No message";
    
    // Ensure priority is valid, defaulting to STATE if:
    // 1. Priority is out of bounds
    // 2. Priority is corrupted (e.g., during shutdown)
    // 3. app_config is not available
    if (priority < LOG_LEVEL_TRACE || priority > LOG_LEVEL_QUIET || 
        !app_config || !app_config->logging.levels) {
        priority = LOG_LEVEL_STATE;
    }

    // Only lock if we're not already in a group
    if (!in_log_group) {
        pthread_mutex_lock(&log_mutex);
    }

    char details[DEFAULT_LOG_ENTRY_SIZE];
    va_list args;
    va_start(args, priority);
    vsnprintf(details, sizeof(details), format, args);
    va_end(args);

    // Get single counter value for both JSON and console output
    unsigned long current_count = __atomic_fetch_add(&log_counter, 1, __ATOMIC_SEQ_CST);
    unsigned long counter_high = (current_count / 1000) % 1000;
    unsigned long counter_low = current_count % 1000;

    char json_message[DEFAULT_MAX_LOG_MESSAGE_SIZE];
    // Create JSON message with all destinations enabled and counter values
    snprintf(json_message, sizeof(json_message),
             "{\"subsystem\":\"%s\",\"details\":\"%s\",\"priority\":%d,\"counter_high\":%lu,\"counter_low\":%lu,\"LogConsole\":true,\"LogFile\":true,\"LogDatabase\":true}",
             subsystem, details, priority, counter_high, counter_low);

    // Check if app_config is NULL (during startup or after cleanup_application_config)
    // or if this is the final shutdown message
    // In these cases, always use console_log regardless of queue status
    if (!app_config || (strcmp(details, "Shutdown complete") == 0)) {
        // During startup, shutdown, or for final message, always use console output
        console_log(subsystem, priority, details, current_count);
    }
    // During very early startup (before queue system init), always use console
    else if (!queue_system_initialized) {
        console_log(subsystem, priority, details, current_count);
    } 
    // Normal operation with initialized queue system
    else {
        // During shutdown, always use console output like we do for NULL app_config
        if (log_queue_shutdown) {
            console_log(subsystem, priority, details, current_count);
        } else {
            // Try to use queue system if it's running
            Queue* log_queue = NULL;
            bool use_console = true;  // Default to using console unless queue succeeds
            
            log_queue = queue_find("SystemLog");
            if (log_queue) {
                if (queue_enqueue(log_queue, json_message, strlen(json_message), priority) == 0) {
                    // Queue succeeded, don't need console fallback
                    use_console = false;
                    // Signal the log queue manager
                    pthread_cond_signal(&terminate_cond);
                }
            }

            // Only check console enabled during normal operation
            if (use_console && app_config->logging.console.enabled) {
                console_log(subsystem, priority, details, current_count);
            }
        }
    }

    // Only unlock if we're not in a group
    if (!in_log_group) {
        pthread_mutex_unlock(&log_mutex);
    }
}
