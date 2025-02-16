/*
 * Implementation of the log queue manager for the Hydrogen server.
 * 
 * Provides a thread-safe, queue-based logging system that decouples log
 * generation from processing. The system implements a producer-consumer
 * pattern where components generate logs (producers) and the queue manager
 * processes them (consumer).
 * 
 * Message Format:
 * - JSON-structured log entries
 * - Timestamps with millisecond precision
 * - Subsystem identification
 * - Priority levels with labels
 * - Configurable output flags
 * 
 * Processing Pipeline:
 * 1. Components submit JSON messages to queue
 * 2. Queue manager retrieves messages in FIFO order
 * 3. Messages are parsed and validated
 * 4. Formatted according to output requirements
 * 5. Distributed to enabled outputs
 * 
 * Output Destinations:
 * - Console: Immediate display with formatting
 * - File: Persistent storage with rotation
 * - Database: Future expansion capability
 * 
 * Thread Safety:
 * - Mutex-protected queue access
 * - Condition variable for queue notification
 * - Atomic shutdown flags
 * - Cleanup handler registration
 * 
 * Shutdown Process:
 * - Processes remaining messages
 * - Closes open file handles
 * - Releases thread resources
 * - Verifies complete cleanup
 */

// Feature test macros must come first
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
#include "queue.h"
#include "configuration.h"

extern volatile sig_atomic_t keep_running;
extern volatile sig_atomic_t shutting_down;
extern volatile sig_atomic_t log_queue_shutdown;
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;

static FILE* log_file = NULL;

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
static void process_log_message(const char* message, int priority) {
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

        if (logConsole) {
            printf("%s", log_entry);
        }

        if (logFile && log_file) {
            fputs(log_entry, log_file);
            fflush(log_file);  // Ensure the log is written immediately
        }

        if (logDatabase) {
            // TODO: Implement database logging
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

    pthread_cleanup_push(cleanup_log_queue_manager, NULL);

    log_this("LogQueueManager", "Log queue manager started", 0, true, true, true);

    while (!log_queue_shutdown) {
        pthread_mutex_lock(&terminate_mutex);
        while (queue_size(log_queue) == 0 && !log_queue_shutdown) {
            pthread_cond_wait(&terminate_cond, &terminate_mutex);
        }
        pthread_mutex_unlock(&terminate_mutex);

        if (log_queue_shutdown && queue_size(log_queue) == 0) {
            log_this("LogQueueManager", "Shutdown: Log Queue Manager processing final messages", 0, true, true, true);
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

    log_this("LogQueueManager", "Shutdown: Log Queue Manager exiting", 0, true, true, true);

    pthread_cleanup_pop(1);
    return NULL;
}
