/*
 * Hydrogen Server Logging System
 *
 * ==================================================================================
 * CRITICAL: Mock System Integration Requirements
 * ==================================================================================
 *
 * When adding NEW PUBLIC FUNCTIONS to this file, you MUST also update the mock system
 * to prevent linker errors during Unity test builds. Follow these three steps:
 *
 * STEP 1: Update tests/unity/mocks/mock_logging.h
 *   - In the USE_MOCK_LOGGING section, add two lines:
 *     a) Undefine: undef your_function_name
 *     b) Override: define your_function_name mock_your_function_name
 *
 * STEP 2: Update tests/unity/mocks/mock_logging.c
 *   - Add mock implementation function: void mock_your_function_name(...)
 *   - Add function prototype in the "Mock function prototypes" section
 *
 * STEP 3: Protect the REAL implementation in THIS FILE (logging.c)
 *   - Before your function definition, add protection block (see lines 556-562):
 *       ifdef your_function_name
 *       undef your_function_name
 *       // Need prototype after undef to satisfy -Werror=missing-prototypes
 *       void your_function_name(...);
 *       endif
 *
 * WHY: During Unity builds, the preprocessor tries to rename all logging functions
 * via macros. Without Step 3, it would rename the real implementation causing
 * duplicate symbol errors (real function renamed to mock name, but mock already exists).
 *
 * EXAMPLE: See cleanup_log_buffer() implementation at lines 556-575 in this file.
 * ==================================================================================
 */

// Global includes
#include <src/hydrogen.h>

// Prevent mock system from affecting this source file
#ifdef USE_MOCK_LOGGING
#undef USE_MOCK_LOGGING
#endif

// Local includes
#include "logging.h"
#include <src/registry/registry.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Public interface declarations
void log_this(const char* subsystem, const char* format, int priority, int num_args, ...);
void log_group_begin(void);
void log_group_end(void);

// External declarations
extern volatile sig_atomic_t log_queue_shutdown;
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;
extern int queue_system_initialized;  // From queue.c

// Internal state
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned long log_counter = 0;  // Global log counter
static bool log_group_active = false;  // Global log group flag
static pthread_mutex_t log_group_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t log_group_cond = PTHREAD_COND_INITIALIZER;

// TLS Keys (static/internal - no extern needed)
static pthread_key_t logging_operation_key;
static pthread_key_t log_group_key;
static pthread_key_t mutex_operation_key;  // To detect mutex context
static bool tls_keys_initialized = false;

// Destructor for logging flag (frees malloc'd bool)
void free_logging_flag(void *ptr) {
    if (ptr) free(ptr);
}

// Lazy init TLS keys
void init_tls_keys(void) {
    if (!tls_keys_initialized) {
        pthread_key_create(&logging_operation_key, free_logging_flag);
        pthread_key_create(&log_group_key, free_logging_flag);
        pthread_key_create(&mutex_operation_key, free_logging_flag);
        tls_keys_initialized = true;
    }
}

// Accessors for logging_operation
bool* get_logging_operation_flag(void) {
    init_tls_keys();
    bool *flag = pthread_getspecific(logging_operation_key);
    if (!flag) {
        flag = malloc(sizeof(bool));
        if (flag) {
            *flag = false;
            pthread_setspecific(logging_operation_key, flag);
        } else {
            static bool fallback_flag = false;
            return &fallback_flag;
        }
    }
    return flag;
}

bool log_is_in_logging_operation(void) {
    const bool *flag = get_logging_operation_flag();
    return *flag;
}

void set_logging_operation_flag(bool val) {
    bool *flag = get_logging_operation_flag();
    if (flag) *flag = val;
}


// Accessors for mutex_operation
bool* get_mutex_operation_flag(void) {
    init_tls_keys();
    bool *flag = pthread_getspecific(mutex_operation_key);
    if (!flag) {
        flag = malloc(sizeof(bool));
        if (flag) {
            *flag = false;
            pthread_setspecific(mutex_operation_key, flag);
        } else {
            static bool fallback_flag = false;
            return &fallback_flag;
        }
    }
    return flag;
}

void set_mutex_operation_flag(bool val) {
    bool *flag = get_mutex_operation_flag();
    if (flag) *flag = val;
}

// Accessors for log_group
bool* get_log_group_flag(void) {
    init_tls_keys();
    bool *flag = pthread_getspecific(log_group_key);
    if (!flag) {
        flag = malloc(sizeof(bool));
        if (flag) {
            *flag = false;
            pthread_setspecific(log_group_key, flag);
        } else {
            static bool fallback_flag = false;
            return &fallback_flag;
        }
    }
    return flag;
}

void set_log_group_flag(bool val) {
    bool *flag = get_log_group_flag();
    if (flag) *flag = val;
}

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
bool init_message_slot(size_t index) {
    if (log_buffer.messages[index] == NULL) {
        log_buffer.messages[index] = malloc(MAX_LOG_LINE_LENGTH);
        if (!log_buffer.messages[index]) return false;
    }
    return true;
}

// Add a message to the rolling buffer
void add_to_buffer(const char* message) {
    set_mutex_operation_flag(true);  // NEW: Mark mutex context
    MutexResult lock_result = MUTEX_LOCK(&log_buffer.mutex, SR_LOGGING);
    if (lock_result == MUTEX_SUCCESS) {
        // Initialize or move to next slot
        size_t index = log_buffer.head;
        if (!init_message_slot(index)) {
            MUTEX_UNLOCK(&log_buffer.mutex, SR_LOGGING);
            set_mutex_operation_flag(false);
            return;
        }

        // Copy message to buffer using snprintf for safety
        snprintf(log_buffer.messages[index], MAX_LOG_LINE_LENGTH, "%s", message);

        // Update head and count
        log_buffer.head = (log_buffer.head + 1) % LOG_BUFFER_SIZE;
        if (log_buffer.count < LOG_BUFFER_SIZE) {
            log_buffer.count++;
        }

        MUTEX_UNLOCK(&log_buffer.mutex, SR_LOGGING);
    }
    set_mutex_operation_flag(false);  // NEW: Clear mutex context
}

// Get messages matching a subsystem
char* log_get_messages(const char* subsystem) {
    if (!subsystem) return NULL;

    set_mutex_operation_flag(true);  // NEW: Mark mutex context
    MutexResult lock_result = MUTEX_LOCK(&log_buffer.mutex, SR_LOGGING);
    if (lock_result == MUTEX_SUCCESS) {
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
            MUTEX_UNLOCK(&log_buffer.mutex, SR_LOGGING);
            set_mutex_operation_flag(false);
            return NULL;
        }

        // Allocate buffer for matching messages
        char* result = malloc(total_size + 1);  // +1 for null terminator
        if (!result) {
            MUTEX_UNLOCK(&log_buffer.mutex, SR_LOGGING);
            set_mutex_operation_flag(false);
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

        MUTEX_UNLOCK(&log_buffer.mutex, SR_LOGGING);
        set_mutex_operation_flag(false);
        return result;
    }
    set_mutex_operation_flag(false);
    return NULL;
}

// Get the last N messages
char* log_get_last_n(size_t count) {
    set_mutex_operation_flag(true);  // NEW: Mark mutex context
    MutexResult lock_result = MUTEX_LOCK(&log_buffer.mutex, SR_LOGGING);
    if (lock_result == MUTEX_SUCCESS) {
        // Adjust count if it's larger than available messages
        if (count > log_buffer.count) {
            count = log_buffer.count;
        }

        if (count == 0) {
            MUTEX_UNLOCK(&log_buffer.mutex, SR_LOGGING);
            set_mutex_operation_flag(false);
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
            MUTEX_UNLOCK(&log_buffer.mutex, SR_LOGGING);
            set_mutex_operation_flag(false);
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

        MUTEX_UNLOCK(&log_buffer.mutex, SR_LOGGING);
        set_mutex_operation_flag(false);
        return result;
    }
    set_mutex_operation_flag(false);
    return NULL;
}

// Private function declarations
void console_log(const char* subsystem, int priority, const char* message, unsigned long current_count);

// Fallback priority labels for when config is unavailable
const char* get_fallback_priority_label(int priority) {
    static const char* fallback_labels[] = {
        "TRACE", "DEBUG", "STATE", "ALERT", "ERROR", "FATAL", "QUIET"
    };
    if (priority >= 0 && priority <= LOG_LEVEL_QUIET) {
        return fallback_labels[priority];
    }
    return fallback_labels[LOG_LEVEL_STATE];  // Default to STATE
}

void console_log(const char* subsystem, int priority, const char* message, unsigned long current_count) {
    // Format the counter as three 3-digit numbers
    char counter_prefix[20];
    snprintf(counter_prefix, sizeof(counter_prefix), "[ %03lu %03lu %03lu ]",
             (current_count / 1000000) % 1000, (current_count / 1000) % 1000, current_count % 1000);

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
    const struct tm* tm_info;
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

// Count the number of format specifiers in a format string, excluding %%
size_t count_format_specifiers(const char* format) {
    if (!format) return 0;

    size_t count = 0;
    const char* ptr = format;

    while ((ptr = strchr(ptr, '%')) != NULL) {
        ptr++;  // Move past %
        if (*ptr == '%') {
            // %% is literal %, skip
            ptr++;
            continue;
        }

        // Skip flags
        while (*ptr == '-' || *ptr == '+' || *ptr == ' ' || *ptr == '#' || *ptr == '0' || *ptr == '\'') ptr++;

        // Skip width
        if (*ptr == '*') ptr++;
        else while (isdigit(*ptr)) ptr++;

        // Skip precision
        if (*ptr == '.') {
            ptr++;
            if (*ptr == '*') ptr++;
            else while (isdigit(*ptr)) ptr++;
        }

        // Skip length modifier
        if (*ptr == 'h' || *ptr == 'l' || *ptr == 'L' || *ptr == 'z' || *ptr == 'j' || *ptr == 't') {
            if ((*ptr == 'l' && *(ptr+1) == 'l') || (*ptr == 'h' && *(ptr+1) == 'h')) ptr += 2;
            else ptr++;
        }

        // Check for conversion specifier
        if (strchr("diouxXeEfFgGaAcspn", *ptr)) {
            count++;
            ptr++;
        }
    }

    return count;
}

// Log a message based on configuration settings
void log_group_begin(void) {
    // Acquire the global log group lock - this blocks all other threads from logging
    MutexResult lock_result = MUTEX_LOCK(&log_group_mutex, SR_LOGGING);
    if (lock_result == MUTEX_SUCCESS) {
        log_group_active = true;
        // Also set thread-local flag for backward compatibility
        set_log_group_flag(true);
        // Acquire the normal log mutex for this thread
        MUTEX_LOCK(&log_mutex, SR_LOGGING);
    }
}

void log_group_end(void) {
    // Clear flags
    set_log_group_flag(false);
    log_group_active = false;

    // Release the normal log mutex
    MUTEX_UNLOCK(&log_mutex, SR_LOGGING);

    // Signal waiting threads and release the global log group lock
    pthread_cond_broadcast(&log_group_cond);
    MUTEX_UNLOCK(&log_group_mutex, SR_LOGGING);
}

// Protect from mock system renaming this function
#ifdef log_this
#undef log_this
// Need prototype after undef to satisfy -Werror=missing-prototypes
void log_this(const char* subsystem, const char* format, int priority, int num_args, ...);
#endif

void log_this(const char* subsystem, const char* format, int priority, int num_args, ...) {

    // NEW: Skip logging if in mutex operation to break recursion
    if (*get_mutex_operation_flag()) {
        return;
    }

    // Skip logging if in mutex operation to break recursion
    if (*get_mutex_operation_flag()) {
        return;
    }
    
    // Set thread-local flag to prevent recursive logging
    bool was_in_logging = log_is_in_logging_operation();
    set_logging_operation_flag(true);

    // Validate inputs and normalize priority level early
    if (!subsystem) subsystem = "Unknown";
    if (!format) format = "No message";

    // DEFENSIVE PROGRAMMING: Check that num_args matches the number of format specifiers
    if (format && strlen(format) > 0) {
        size_t specifier_count = count_format_specifiers(format);
        if ((size_t)num_args != specifier_count) {
            fprintf(stderr, "WARNING: log_this parameter mismatch: Format string '%s' has %zu specifiers but received %d arguments\n",
                    format, specifier_count, num_args);
            fflush(stderr);
        }
    }

    // Check if we're in a log group
    bool is_in_group = *get_log_group_flag();

    // If a log group is active but we're not the group thread, wait for it to complete
    if (log_group_active && !is_in_group) {
        // Wait for the log group to complete
        MutexResult wait_result = MUTEX_LOCK(&log_group_mutex, SR_LOGGING);
        if (wait_result == MUTEX_SUCCESS) {
            while (log_group_active) {
                pthread_cond_wait(&log_group_cond, &log_group_mutex);
            }
            MUTEX_UNLOCK(&log_group_mutex, SR_LOGGING);
        }
    }

    // Now proceed with normal logging logic
    if (!is_in_group) {
        MutexResult lock_result = MUTEX_LOCK(&log_mutex, SR_LOGGING);
        if (lock_result != MUTEX_SUCCESS) {
            // Failed to lock, skip logging to avoid infinite recursion
            set_logging_operation_flag(was_in_logging);  // Restore flag
            return;
        }
    }

    char details[DEFAULT_LOG_ENTRY_SIZE];
    va_list args;
    va_start(args, num_args);
    vsnprintf(details, sizeof(details), format, args);
    va_end(args);

    // Get single counter value for both JSON and console output
    unsigned long current_count = __atomic_fetch_add(&log_counter, 1, __ATOMIC_SEQ_CST);
    unsigned long counter_super = (current_count / 1000000) % 1000;
    unsigned long counter_high = (current_count / 1000) % 1000;
    unsigned long counter_low = current_count % 1000;

    // Send to VictoriaLogs immediately (no dependencies, works from startup)
    // This is intentionally independent of the queue system and config
    if (victoria_logs_is_enabled()) {
        victoria_logs_send(subsystem, details, priority);
    }

    char json_message[DEFAULT_MAX_LOG_MESSAGE_SIZE];
    // Create JSON message with all destinations enabled and counter values
    snprintf(json_message, sizeof(json_message),
             "{\"subsystem\":\"%s\",\"details\":\"%s\",\"priority\":%d,\"counter_super\":%lu,\"counter_high\":%lu,\"counter_low\":%lu,\"LogConsole\":true,\"LogFile\":true,\"LogDatabase\":true}",
             subsystem, details, priority, counter_super, counter_high, counter_low);

    // Check if we're in startup phase (logging subsystem not yet running or server not running)
    // or if this is the final shutdown message
    // In these cases, filter by startup log level
    bool use_startup_filtering = false;

    // Check original condition (server not running)
    if (server_running == 0) {
        use_startup_filtering = true;
    }
    // Also check if registry exists and is initialized, then check if logging subsystem is running
    else if (subsystem_registry.subsystems == NULL ||
        subsystem_registry.count == 0 ||
        !is_subsystem_running_by_name(SR_LOGGING)) {
        use_startup_filtering = true;
    }


    // Also use startup filtering for final shutdown message (maintains original behavior)
    if (strcmp(details, "Shutdown complete") == 0) {
        use_startup_filtering = true;
    }

    if (use_startup_filtering) {
        if (priority >= startup_log_level) {
            console_log(subsystem, priority, details, current_count);
        }
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
            if (use_console && app_config && app_config->logging.console.enabled) {
                console_log(subsystem, priority, details, current_count);
            }
        }
    }

    // Only unlock if we're not in a group
    if (!is_in_group) {
        MUTEX_UNLOCK(&log_mutex, SR_LOGGING);
    }

    // Restore the logging operation flag
    set_logging_operation_flag(was_in_logging);
}

// Protect from mock system renaming this function
#ifdef cleanup_log_buffer
#undef cleanup_log_buffer
// Need prototype after undef to satisfy -Werror=missing-prototypes
void cleanup_log_buffer(void);
#endif

// Cleanup function to free all log buffer message slots
void cleanup_log_buffer(void) {
    set_mutex_operation_flag(true);  // Mark mutex context
    MutexResult lock_result = MUTEX_LOCK(&log_buffer.mutex, SR_LOGGING);
    if (lock_result == MUTEX_SUCCESS) {
        // Free all allocated message slots
        for (size_t i = 0; i < LOG_BUFFER_SIZE; i++) {
            if (log_buffer.messages[i] != NULL) {
                free(log_buffer.messages[i]);
                log_buffer.messages[i] = NULL;
            }
        }
        // Reset buffer state
        log_buffer.head = 0;
        log_buffer.count = 0;
        MUTEX_UNLOCK(&log_buffer.mutex, SR_LOGGING);
    }
    set_mutex_operation_flag(false);  // Clear mutex context
}
