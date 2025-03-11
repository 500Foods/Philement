/*
 * Print Job Management System for 3D Printer Control
 * 
 * Why Robust Job Management Matters:
 * 1. Print Quality Assurance
 *    - Job validation and preprocessing
 *    - Parameter verification
 *    - Resource availability checks
 *    - Quality monitoring
 * 
 * 2. Safety Considerations
 *    Why These Checks?
 *    - Temperature limits
 *    - Material compatibility
 *    - Hardware readiness
 *    - Emergency handling
 * 
 * 3. Resource Management
 *    Why This Matters?
 *    - Long print durations
 *    - Material consumption
 *    - Power management
 *    - System resources
 * 
 * 4. Job Prioritization
 *    Why Priority Queuing?
 *    - Emergency stops
 *    - Maintenance tasks
 *    - User priorities
 *    - System tasks
 * 
 * 5. Error Recovery
 *    Why These Features?
 *    - Print failure handling
 *    - State preservation
 *    - Job resumption
 *    - Resource cleanup
 * 
 * Implementation Features:
 * - Thread-safe queue system
 * - Priority-based processing
 * - Resource monitoring
 * - Status tracking
 * 
 * Job Processing Pipeline:
 * 1. Validation Phase
 *    - G-code analysis
 *    - Resource checks
 *    - Safety verification
 * 
 * 2. Execution Phase
 *    - Real-time monitoring
 *    - Status updates
 *    - Error detection
 * 
 * 3. Completion Phase
 *    - Resource cleanup
 *    - State updates
 *    - Job archiving
 */

// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Core system headers
#include <sys/types.h>
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
#include "print_queue_manager.h"
#include "../queue/queue.h"
#include "../config/config.h"
#include "../logging/logging.h"
#include "../utils/utils.h"

extern volatile sig_atomic_t print_queue_shutdown;
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;

Queue* print_queue = NULL;

// Cleanup handler registered with pthread_cleanup_push
// Ensures proper resource cleanup if thread is cancelled
// Logs cleanup status for debugging
static void cleanup_print_queue_manager(void* arg) {
    (void)arg;  // Unused parameter
    
    // Remove this thread from tracking before cleanup
    remove_service_thread(&print_threads, pthread_self());
    
    log_this("PrintQueueManager", "Shutdown: Cleaning up Print Queue Manager", LOG_LEVEL_STATE);
    // Add any necessary cleanup code here
}

// Process a single print job from the queue
// 1. Parses and validates job JSON
// 2. Extracts file information and parameters
// 3. Logs job status and progress
// 4. Handles errors and cleanup
// Returns: void, errors handled via logging
static void process_print_job(const char* job_data) {
    if (!job_data) {
        log_this("PrintQueueManager", "Received null job data", LOG_LEVEL_ERROR);
        return;
    }

    json_error_t error;
    json_t* json = json_loads(job_data, 0, &error);
    if (!json) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Failed to parse job JSON: %s", error.text);
        log_this("PrintQueueManager", error_msg, LOG_LEVEL_ERROR);
        return;
    }

    const char* original_filename = json_string_value(json_object_get(json, "original_filename"));
    const char* new_filename = json_string_value(json_object_get(json, "new_filename"));
    json_int_t file_size = json_integer_value(json_object_get(json, "file_size"));

    char log_buffer[256];
    snprintf(log_buffer, sizeof(log_buffer), "Processing print job: %s (original: %s), size: %lld bytes",
             new_filename ? new_filename : "unknown",
             original_filename ? original_filename : "unknown",
             (long long)file_size);
    log_this("PrintQueueManager", log_buffer, LOG_LEVEL_STATE);

    // TODO: Implement actual print job processing here

    json_decref(json);
}

// Initialize the print queue system
// Creates queue with specified attributes
// Sets up job management infrastructure
// Returns: 1 on success, 0 on failure
int init_print_queue(void) {
    QueueAttributes print_queue_attrs = {0}; // Initialize with default values for now
    print_queue = queue_create("PrintQueue", &print_queue_attrs);
    if (!print_queue) {
        log_this("PrintQueueManager", "Failed to create PrintQueue", LOG_LEVEL_ERROR);
        return 0;
    }
    log_this("PrintQueueManager", "PrintQueue created successfully", LOG_LEVEL_STATE);
    return 1;
}

// Main print queue manager thread function
// Implements the consumer side of the job system:
// 1. Waits for jobs using condition variables
// 2. Processes jobs in priority order
// 3. Handles shutdown signals gracefully
// 4. Ensures proper cleanup on exit
void* print_queue_manager(void* arg) {
    (void)arg; // Unused parameter

    // Register this thread with the print service
    add_service_thread(&print_threads, pthread_self());

    pthread_cleanup_push(cleanup_print_queue_manager, NULL);

    log_this("PrintQueueManager", "Print queue manager started", LOG_LEVEL_STATE);

    while (!print_queue_shutdown) {
        pthread_mutex_lock(&terminate_mutex);
        while (queue_size(print_queue) == 0 && !print_queue_shutdown) {
            pthread_cond_wait(&terminate_cond, &terminate_mutex);
        }
        pthread_mutex_unlock(&terminate_mutex);

        if (print_queue_shutdown) {
            log_this("PrintQueueManager", "Shutdown: Print Queue shutdown signal received, processing remaining jobs", LOG_LEVEL_STATE);
        }

        while (queue_size(print_queue) > 0) {
            size_t job_size;
            int priority;
            char* job_data = queue_dequeue(print_queue, &job_size, &priority);
            if (job_data) {
                process_print_job(job_data);
                free(job_data);
            }
        }
    }

    log_this("PrintQueueManager", "Shutdown: Print Queue Manager exiting", LOG_LEVEL_STATE);

    pthread_cleanup_pop(1);
    return NULL;
}

// Initiate graceful queue shutdown
// 1. Sets shutdown flag to prevent new jobs
// 2. Allows current job to complete
// 3. Preserves remaining jobs in queue
// 4. Performs resource cleanup
void shutdown_print_queue() {
    log_this("PrintQueueManager", "Shutdown: Initiating Print Queue shutdown", LOG_LEVEL_STATE);

    print_queue_shutdown = 1;
    pthread_cond_broadcast(&terminate_cond);

    // Wait for any ongoing job to finish
    usleep(500000); // Wait for 500ms

    // Drain remaining jobs
    size_t remaining = queue_size(print_queue);
    log_this("PrintQueueManager", "Shutdown: Remaining jobs in print queue: %zu", LOG_LEVEL_STATE, remaining);

    while (queue_size(print_queue) > 0) {
        size_t size;
        int priority;
        char* job = queue_dequeue(print_queue, &size, &priority);
        if (job) {
            log_this("PrintQueueManager", "Shutdown: Drained job: %s", LOG_LEVEL_STATE, job);
            free(job);
        }
    }

    // Clear the queue contents but don't destroy it
    if (print_queue) {
        queue_clear(print_queue);
    }
    log_this("PrintQueueManager", "Shutdown: Print Queue shutdown complete", LOG_LEVEL_STATE);
}
