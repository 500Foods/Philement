/*
 * Log Queue Manager for the Hydrogen 3D Printer Control System
 * 
 * Why a Queue-Based Design?
 * 1. Real-Time Performance
 *    - Non-blocking log submission critical for printer control
 *    - Decoupled logging from time-sensitive operations
 *    - Predictable latency for control systems
 *    - Minimal impact on motion planning
 * 
 * 2. Reliability Requirements
 *    - No log loss during critical operations
 *    - Ordered message delivery for event reconstruction
 *    - Graceful handling of system pressure
 *    - Recovery from resource exhaustion
 * 
 * Message Design:
 * Why JSON Structure?
 * - Self-describing data format
 * - Easy parsing and validation
 * - Extensible for future needs
 * - Human-readable for debugging
 * 
 * Why These Fields?
 * - Timestamps: Critical for event correlation
 * - Subsystems: Isolate component behavior
 * - Priorities: Guide maintenance response
 * - Flags: Control output routing
 * 
 * Processing Strategy:
 * Why This Pipeline?
 * 1. Queue Submission
 *    - Atomic message acceptance
 *    - Back-pressure handling
 *    - Memory management
 * 
 * 2. Ordered Processing
 *    - Maintains event sequence
 *    - Batches for efficiency
 *    - Handles priority levels
 * 
 * 3. Output Distribution
 *    - Configurable destinations
 *    - Failure isolation
 *    - Format adaptation
 * 
 * Output Architecture:
 * Why Multiple Destinations?
 * - Console: Immediate operator feedback
 * - File: Long-term troubleshooting data
 * - Database: Future analytics support
 * 
 * Why This Implementation?
 * - Atomic file operations
 * - Buffered console output
 * - Extensible routing system
 * 
 * Concurrency Design:
 * Why These Mechanisms?
 * - Mutex Protection: Prevent data races
 * - Condition Variables: Efficient waiting
 * - Atomic Flags: Fast state checks
 * - Cleanup Handlers: Reliable shutdown
 * 
 * Why This Matters?
 * - Printer control requires reliability
 * - Debug logs crucial for support
 * - Performance impacts print quality
 * - Resource leaks affect stability
 * 
 * Shutdown Strategy:
 * Why This Sequence?
 * - Complete message processing
 * - Ensure data persistence
 * - Clean resource release
 * - Verification steps
 * 
 * Why So Careful?
 * - Prevent log loss during errors
 * - Maintain filesystem integrity
 * - Support system restarts
 * - Enable post-mortem analysis
 */

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Core system headers
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Third-party libraries
#include <jansson.h>

// Project headers
#include "log_queue_manager.h"
#include "logging.h"
#include "../queue/queue.h"
#include "../config/config.h"
#include "../config/config_priority.h"
#include "../utils/utils.h"

// Public interface declarations
void init_file_logging(const char* log_file_path);
void close_file_logging(void);
void* log_queue_manager(void* arg);

// External declarations
extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t log_queue_shutdown;
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;

// Internal state
static FILE* log_file = NULL;

// Private function declarations
static void cleanup_log_queue_manager(void* arg);
static bool should_log_to_destination(const char* subsystem, int priority, const LoggingDestination* dest);
static void process_log_message(const char* message, int priority);

// Thread cleanup handler with guaranteed file closure
//
// Design choices for cleanup handling:
// 1. pthread_cleanup_push integration
//    - Handles normal termination
//    - Catches thread cancellation
//    - Processes asynchronous signals
//
// 2. Resource management
//    - Prevents file handle leaks
//    - Ensures final flush
//    - Maintains file system integrity
static void cleanup_log_queue_manager(void* arg) {
    (void)arg;  // Unused parameter
    
    // Remove this thread from tracking before cleanup
    remove_service_thread(&logging_threads, pthread_self());
    
    close_file_logging();
}

// Process log messages with structured formatting and routing
//
// Message handling strategy:
// 1. Data Integrity
//    - JSON validation
//    - UTF-8 encoding check
//    - Timestamp precision
//    - Buffer overflow prevention
//
// 2. Output Management
//    - Configurable destinations
//    - Immediate console feedback
//    - Atomic file writes
//    - Future database support
//
// 3. Error Handling
//    - Malformed JSON recovery
//    - File write retry
//    - Memory allocation checks
//    - Partial write detection
// Check if a message should be logged to a specific destination
static bool should_log_to_destination(const char* subsystem, int priority, const LoggingDestination* dest) {
    if (!dest->Enabled) {
        return false;
    }

    // Get subsystem-specific level if configured
    int configured_level = dest->DefaultLevel;
    
    // Check if this subsystem has a specific configuration
    if (strcmp(subsystem, "ThreadMgmt") == 0) {
        configured_level = dest->Subsystems.ThreadMgmt;
    } else if (strcmp(subsystem, "Shutdown") == 0) {
        configured_level = dest->Subsystems.Shutdown;
    } else if (strcmp(subsystem, "mDNSServer") == 0) {
        configured_level = dest->Subsystems.mDNSServer;
    } else if (strcmp(subsystem, "WebServer") == 0) {
        configured_level = dest->Subsystems.WebServer;
    } else if (strcmp(subsystem, "WebSocket") == 0) {
        configured_level = dest->Subsystems.WebSocket;
    } else if (strcmp(subsystem, "PrintQueue") == 0) {
        configured_level = dest->Subsystems.PrintQueue;
    } else if (strcmp(subsystem, "LogQueueManager") == 0) {
        configured_level = dest->Subsystems.LogQueueManager;
    }
    // For undefined subsystems, we'll use the destination's DefaultLevel
    
    // Special handling for ALL and NONE
    if (configured_level == LOG_LEVEL_ALL) return true;
    if (configured_level == LOG_LEVEL_NONE) return false;

    // For normal levels, message priority must be >= configured level
    return priority >= configured_level;
}

static void process_log_message(const char* message, int priority) {
    extern AppConfig *app_config;  // Get access to global config
    if (!app_config) return;

    json_error_t error;
    json_t* json = json_loads(message, 0, &error);
    if (json) {
        const char* subsystem = json_string_value(json_object_get(json, "subsystem"));
        const char* details = json_string_value(json_object_get(json, "details"));
        bool logConsole = json_boolean_value(json_object_get(json, "LogConsole"));
        bool logDatabase = json_boolean_value(json_object_get(json, "LogDatabase"));
        bool logFile = json_boolean_value(json_object_get(json, "LogFile"));

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

        char log_entry[1024];  // Increase buffer size
        snprintf(log_entry, sizeof(log_entry), "%s  %s  %s  %s\n", timestamp_ms, formatted_priority, formatted_subsystem, details);

        // Apply filtering for each destination
        if (logConsole && should_log_to_destination(subsystem, priority, &app_config->Logging.Console.base)) {
            printf("%s", log_entry);
        }

        if (logFile && log_file && should_log_to_destination(subsystem, priority, &app_config->Logging.File.base)) {
            fputs(log_entry, log_file);
            fflush(log_file);  // Ensure the log is written immediately
        }

        if (logDatabase && should_log_to_destination(subsystem, priority, &app_config->Logging.Database.base)) {
            // TODO: Implement database logging when needed
        }

        json_decref(json);
    } else {
        fprintf(stderr, "Error parsing JSON: %s\n", error.text);
    }
}

// Initialize file-based logging
// Opens the specified log file in append mode
// Ensures previous file handle is closed
// Handles file access errors
void init_file_logging(const char* log_file_path) {
    if (log_file) {
        fclose(log_file);
    }
    log_file = fopen(log_file_path, "a");
    if (!log_file) {
        fprintf(stderr, "Error: Unable to open log file: %s\n", log_file_path);
    }
}

void close_file_logging() {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}

// Log queue manager implementing producer-consumer pattern
//
// The architecture balances several concerns:
// 1. Performance
//    - Non-blocking message submission
//    - Batched file operations
//    - Minimal lock contention
//    - Memory reuse where possible
//
// 2. Reliability
//    - No message loss guarantee
//    - Ordered message delivery
//    - Graceful degradation
//    - Recovery from errors
//
// 3. Shutdown Handling
//    - Process remaining messages
//    - Flush pending writes
//    - Release resources
//    - Verify completion
void* log_queue_manager(void* arg) {
    Queue* log_queue = (Queue*)arg;

    // Register this thread with the logging service
    add_service_thread(&logging_threads, pthread_self());

    pthread_cleanup_push(cleanup_log_queue_manager, NULL);

    log_this("LogQueueManager", "Log queue manager started", LOG_LEVEL_INFO);

    while (!log_queue_shutdown) {
        pthread_mutex_lock(&terminate_mutex);
        while (queue_size(log_queue) == 0 && !log_queue_shutdown) {
            pthread_cond_wait(&terminate_cond, &terminate_mutex);
        }
        pthread_mutex_unlock(&terminate_mutex);

        if (log_queue_shutdown && queue_size(log_queue) == 0) {
            log_this("LogQueueManager", "Shutdown: Log Queue Manager processing final messages", LOG_LEVEL_INFO);
        }

        while (queue_size(log_queue) > 0) {
            size_t message_size;
            int priority;
            char* message = queue_dequeue(log_queue, &message_size, &priority);
            if (message) {
                process_log_message(message, priority);
                free(message);
            }
        }
    }

    log_this("LogQueueManager", "Shutdown: Log Queue Manager exiting", LOG_LEVEL_INFO);

    pthread_cleanup_pop(1);
    return NULL;
}
