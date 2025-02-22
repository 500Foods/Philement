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

// Feature test macros must come first
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
#include "configuration.h"
#include "queue.h"
#include "utils.h"

// Global configuration
extern AppConfig* app_config;

// Forward declarations
extern volatile sig_atomic_t log_queue_shutdown;
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;

// Internal implementation details
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

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
    const char* priority_label;
    switch (priority) {
        case 1: priority_label = "DEBUG"; break;
        case 2: priority_label = "INFO"; break;
        case 3: priority_label = "WARNING"; break;
        case 4: priority_label = "ERROR"; break;
        case 5: priority_label = "CRITICAL"; break;
        default: priority_label = "UNKNOWN"; break;
    }
    fprintf(stderr, "[%s] %s: %s\n", subsystem, priority_label, message);
}

// Log a message with configurable output targets and priority
//
// Logging system design prioritizes:
// 1. Thread Safety
//    - Mutex protection for concurrent access
//    - Atomic operations for shutdown
//    - Queue-based message handling
//    - Safe signal handling
//
// 2. Reliability
//    - Fallback console output
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
//    - Multiple output targets
//    - Priority-based handling
//    - Subsystem categorization
//    - Format customization
void log_this(const char* subsystem, const char* format, int priority, bool LogConsole, bool LogDatabase, bool LogFile, ...) {
    pthread_mutex_lock(&log_mutex);

    char details[DEFAULT_LOG_ENTRY_SIZE];
    va_list args;
    va_start(args, LogFile);
    vsnprintf(details, sizeof(details), format, args);
    va_end(args);

    char json_message[DEFAULT_MAX_LOG_MESSAGE_SIZE];
    snprintf(json_message, sizeof(json_message),
             "{\"subsystem\":\"%s\",\"details\":\"%s\",\"priority\":%d,\"LogConsole\":%s,\"LogDatabase\":%s,\"LogFile\":%s}",
             subsystem, details, priority, LogConsole ? "true" : "false", LogDatabase ? "true" : "false", LogFile ? "true" : "false");

    // Try to use queue system if it's available and running
    Queue* log_queue = NULL;
    extern int queue_system_initialized;  // From queue.c
    
    // If queue system is initialized and not shutting down, try to use it
    bool use_console = true;  // Default to using console unless queue succeeds
    if (queue_system_initialized && !log_queue_shutdown) {
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
    // - Queue system is not initialized
    // - Queue system is shutting down
    // - Queue is not found
    // - Queue enqueue failed
    if (LogConsole && use_console) {
        console_log(subsystem, priority, details);
    }

    pthread_mutex_unlock(&log_mutex);
}
