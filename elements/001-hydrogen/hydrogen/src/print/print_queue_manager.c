/*
 * Print Job Management for 3D Printing
 * 
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "print_queue_manager.h"

extern volatile sig_atomic_t print_queue_shutdown;
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;

Queue* print_queue = NULL;

// Cleanup handler registered with pthread_cleanup_push
// Ensures proper resource cleanup if thread is cancelled
// Logs cleanup status for debugging
void cleanup_print_queue_manager(void* arg) {
    (void)arg;  // Unused parameter
    
    // Remove this thread from tracking before cleanup
    remove_service_thread(&print_threads, pthread_self());
    
    log_this(SR_QUEUES, "Shutdown: Cleaning up Print Queue Manager", LOG_LEVEL_STATE, 0);
    // Add any necessary cleanup code here
}

// Process a single print job from the queue
// 1. Parses and validates job JSON
// 2. Extracts file information and parameters
// 3. Logs job status and progress
// 4. Handles errors and cleanup
// Returns: void, errors handled via logging
void process_print_job(const char* job_data) {
    fprintf(stderr, "DEBUG: process_print_job called with job_data=%p\n", job_data);
    if (!job_data) {
        fprintf(stderr, "DEBUG: job_data is NULL, about to call log_this\n");
        log_this(SR_QUEUES, "Received null job data", LOG_LEVEL_ERROR, 0);
        fprintf(stderr, "DEBUG: log_this call completed\n");
        return;
    }

    json_error_t error;
    json_t* json = json_loads(job_data, 0, &error);
    if (!json) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Failed to parse job JSON: %s", error.text);
        log_this(SR_QUEUES, error_msg, LOG_LEVEL_ERROR, 0);
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
    log_this(SR_QUEUES, log_buffer, LOG_LEVEL_STATE, 0);

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
        log_this(SR_QUEUES, "Failed to create PrintQueue", LOG_LEVEL_ERROR, 0);
        return 0;
    }
    log_this(SR_QUEUES, "PrintQueue created successfully", LOG_LEVEL_STATE, 0);
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

    log_this(SR_QUEUES, "Print queue manager started", LOG_LEVEL_STATE, 0);

    while (!print_queue_shutdown) {
        pthread_mutex_lock(&terminate_mutex);
        while (queue_size(print_queue) == 0 && !print_queue_shutdown) {
            pthread_cond_wait(&terminate_cond, &terminate_mutex);
        }
        pthread_mutex_unlock(&terminate_mutex);

        if (print_queue_shutdown) {
            log_this(SR_QUEUES, "Shutdown: Print Queue shutdown signal received, processing remaining jobs", LOG_LEVEL_STATE, 0);
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

    log_this(SR_QUEUES, "Shutdown: Print Queue Manager exiting", LOG_LEVEL_STATE, 0);

    pthread_cleanup_pop(1);
    return NULL;
}

// Initiate graceful queue shutdown
// 1. Sets shutdown flag to prevent new jobs
// 2. Allows current job to complete
// 3. Preserves remaining jobs in queue
// 4. Performs resource cleanup
void shutdown_print_queue(void) {
    log_this(SR_QUEUES, "Shutdown: Initiating Print Queue shutdown", LOG_LEVEL_STATE, 0);

    print_queue_shutdown = 1;
    pthread_cond_broadcast(&terminate_cond);

    // Wait for any ongoing job to finish
    usleep(500000); // Wait for 500ms

    // Drain remaining jobs
    size_t remaining = queue_size(print_queue);
    log_this(SR_QUEUES, "Shutdown: Remaining jobs in print queue: %zu", LOG_LEVEL_STATE, 1, remaining);

    while (queue_size(print_queue) > 0) {
        size_t size;
        int priority;
        char* job = queue_dequeue(print_queue, &size, &priority);
        if (job) {
            log_this(SR_QUEUES, "Shutdown: Drained job: %s", LOG_LEVEL_STATE, 1, job);
            free(job);
        }
    }

    // Clear the queue contents but don't destroy it
    if (print_queue) {
        queue_clear(print_queue);
    }
    log_this(SR_QUEUES, "Shutdown: Print Queue shutdown complete", LOG_LEVEL_STATE, 0);
}
