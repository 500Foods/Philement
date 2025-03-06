/*
 * Hydrogen Server Logging System
 * 
 * Why Advanced Logging in 3D Printing?
 * 1. Print Quality Monitoring
 *    - Track temperature fluctuations
 *    - Monitor motion events
 *    - Record material flow
 *    - Detect anomalies
 * 
 * 2. Safety Critical Events
 *    - Emergency stops
 *    - Temperature excursions
 *    - Motor stalls
 *    - Power issues
 * 
 * 3. Performance Analysis
 *    - Print timing accuracy
 *    - System responsiveness
 *    - Resource utilization
 *    - Command latency
 * 
 * 4. Diagnostic Support
 *    - Error tracing
 *    - Event correlation
 *    - State transitions
 *    - Configuration changes
 * 
 * Implementation Features:
 * - Thread-safe design for concurrent access
 * - JSON formatting for structured analysis
 * - Multiple output targets (console/DB/file)
 * - Priority-based message handling
 * - Asynchronous queue processing
 */

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

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
#include "../config/configuration.h"
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

// Private function declarations
static void console_log(const char* subsystem, int priority, const char* message);

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
static void console_log(const char* subsystem, int priority, const char* message) {
    char formatted_priority[MAX_PRIORITY_LABEL_WIDTH + 5];
    snprintf(formatted_priority, sizeof(formatted_priority), "[ %-*s ]", MAX_PRIORITY_LABEL_WIDTH, get_priority_label(priority));

    char formatted_subsystem[MAX_SUBSYSTEM_LABEL_WIDTH + 5];
    snprintf(formatted_subsystem, sizeof(formatted_subsystem), "[ %-*s ]", MAX_SUBSYSTEM_LABEL_WIDTH, subsystem);

    char timestamp[32];
    struct timeval tv;
    struct tm* tm_info;
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    char timestamp_ms[36];
    snprintf(timestamp_ms, sizeof(timestamp_ms), "%s.%03d", timestamp, (int)(tv.tv_usec / 1000));

    fprintf(stderr, "%s  %s  %s  %s\n", timestamp_ms, formatted_priority, formatted_subsystem, message);
}

// Log a message based on configuration settings
//
// Logging system design prioritizes:
// 1. Thread Safety
//    - Mutex protection for concurrent access
//    - Atomic operations for shutdown
//    - Queue-based message handling
//    - Safe signal handling
//
// 2. Reliability
//    - Configuration-based routing
//    - Buffer overflow prevention
//    - Queue monitoring
//    - Graceful degradation
//
// 3. Performance
//    - Asynchronous processing
//    - Efficient JSON formatting
//    - Minimal blocking time
//    - Memory reuse
//
// 4. Flexibility
//    - Configuration-driven destinations
//    - Priority-based handling
//    - Subsystem categorization
//    - Format customization
void log_group_begin(void) {
    pthread_mutex_lock(&log_mutex);
    in_log_group = true;
}

void log_group_end(void) {
    in_log_group = false;
    pthread_mutex_unlock(&log_mutex);
}

void log_this(const char* subsystem, const char* format, int priority, ...) {
    // Only lock if we're not already in a group
    if (!in_log_group) {
        pthread_mutex_lock(&log_mutex);
    }

    char details[DEFAULT_LOG_ENTRY_SIZE];
    va_list args;
    va_start(args, priority);
    vsnprintf(details, sizeof(details), format, args);
    va_end(args);

    char json_message[DEFAULT_MAX_LOG_MESSAGE_SIZE];
    
    // Create JSON message with all destinations enabled - let queue manager handle filtering
    snprintf(json_message, sizeof(json_message),
             "{\"subsystem\":\"%s\",\"details\":\"%s\",\"priority\":%d,\"LogConsole\":true,\"LogFile\":true,\"LogDatabase\":true}",
             subsystem, details, priority);

    // During very early startup (before queue system init), always use console
    extern int queue_system_initialized;  // From queue.c
    if (!queue_system_initialized) {
        console_log(subsystem, priority, details);
    } else {
        // Try to use queue system if it's running
        Queue* log_queue = NULL;
        bool use_console = true;  // Default to using console unless queue succeeds
        
        if (!log_queue_shutdown) {
            log_queue = queue_find("SystemLog");
            if (log_queue) {
                if (queue_enqueue(log_queue, json_message, strlen(json_message), priority) == 0) {
                    // Queue succeeded, don't need console fallback
                    use_console = false;
                    // Signal the log queue manager
                    pthread_cond_signal(&terminate_cond);
                }
            }
        }

        // Use console output if:
        // 1. app_config isn't initialized yet (early startup)
        // 2. Console logging is enabled and either:
        //    - Queue isn't available
        //    - Queue enqueue failed
        if (!app_config || (app_config->Logging.Console.base.Enabled && use_console)) {
            console_log(subsystem, priority, details);
        }
    }

    // Only unlock if we're not in a group
    if (!in_log_group) {
        pthread_mutex_unlock(&log_mutex);
    }
}
