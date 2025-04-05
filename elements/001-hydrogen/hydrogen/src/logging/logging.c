/*
 * Hydrogen Server Logging System
 */

// Core system headers
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// Project headers
#include "logging.h"
#include "../config/config.h"
#include "../config/config_priority.h"
#include "../queue/queue.h"
#include "../utils/utils.h"

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

// Private function declarations
static void console_log(const char* subsystem, int priority, const char* message, unsigned long current_count);

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

    // Write to stdout for test compatibility and stderr for console visibility
    fprintf(stdout, "%s  %s  %s  %s  %s\n", counter_prefix, timestamp_ms, formatted_priority, formatted_subsystem, message);
    
    // Ensure output is flushed immediately during shutdown
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
